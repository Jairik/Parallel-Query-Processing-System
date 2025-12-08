# Engine Documentation

This document is a comprehensive technical reference for the engines used by the Parallel-Query-Processing-System repository. It describes the B+ tree index implementation, build utilities that load CSV data and construct indexes, and the execute engines that implement SQL-like operations (SELECT, INSERT, DELETE). While the Serial engine is described in detail, the OpenMP and MPI implementations follow a similar structure.

Table of Contents
- Section 1 — B+ Tree (structure, public API, internals)
- Section 2 — Build Engine (CSV loading, index construction)
- Section 3 — Execute Engine (query execution, projection, predicates)
- Section 4 — File / Function Cross Reference
- Section 5 — Examples, recipes, and developer notes

---

## Section 1 — B+ Tree

Files: `include/bplus.h`, `engine/bplus.c`

Purpose
- Provide ordered, efficient indexing for attributes of records. Supports insertion/upsert, deletion (with rebalancing), single-key lookup, and range scan queries.

Key types and structures
- `KEY_T` (union) — stores keys with a `type` field. Supported types:
	- `KEY_INT`, `KEY_UINT64`, `KEY_BOOL`, `KEY_STRING`.
- `node` — core node structure with fields:
	- `void **pointers` — children or row pointers (leaf entries); leaf nodes keep the last pointer as `next` leaf
	- `KEY_T *keys` — list of separator keys (length up to `order-1`)
	- `node *parent`, `bool is_leaf`, `int num_keys`, `node *next`

Constants
- `ORDER` — compile-time fanout. Implementation assumes `keys` length is `ORDER - 1` and `pointers` length is `ORDER`.

Public functions (API)
- `node *insert(node *root, KEY_T key, ROW_PTR row_ptr)`
	- Insert or overwrite a key in the tree. Returns the (possibly new) root pointer.
	- Upsert semantics: if key exists in leaf, pointer is replaced.

- `ROW_PTR find_row(node *root, KEY_T key)`
	- Finds and returns the row pointer for an exact key, or `NULL` when not present.

- `int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose, KEY_T returned_keys[], ROW_PTR returned_pointers[])`
	- Performs a range query from `key_start` through `key_end` (inclusive). Writes results into the provided arrays and returns the number of matches.

- `node *delete(node *root, KEY_T key)`
	- Removes the key and its row pointer from the tree. Rebalances internal nodes and may return a new root.

Internal helper functions (important ones)
- `node *findLeaf(node *const root, KEY_T key, bool verbose)` — descend to the candidate leaf.
- `node *startNewTree(KEY_T key, ROW_PTR row_ptr)` — create a root leaf node for first insertion.
- `insertIntoLeaf`, `insertIntoLeafAfterSplitting`, `insertIntoNode`, `insertIntoNodeAfterSplitting` — insertion/split internals.
- `removeEntryFromNode`, `deleteEntry`, `coalesceNodes`, `redistributeNodes`, `adjustRoot` — deletion/underflow internals.

Complexity
- Insert / Delete / Find: average O(log n) (height of the tree). Range scan complexity: O(log n + k) where k is number of returned rows.

Design notes and caveats
- The code uses fixed arrays sized by `ORDER`. Changing `ORDER` changes the node layout and requires the cut/split logic to remain correct.
- Keys inserted into a specific index must all be the same `KEY_T.type`. Mixing types may produce inconsistent ordering.
- The B+ tree implementation includes a leaf-level linked-list for efficient range traversal.
- **Parallelization Note:** This B+ tree implementation is strictly serial and **cannot be safely parallelized** without significant architectural changes (e.g., fine-grained locking or latch crabbing). There is one shared version of the B+ tree used by all execution engines (Serial, OpenMP, MPI). Concurrent modifications will lead to race conditions and data corruption. Therefore, concurrentcy optimizations will be made *outside* of the data structure.

---

## Section 2 — Build Engine

Files: `engine/*/buildEngine-*.c`, `include/buildEngine-*.h`, `engine/recordSchema.c`, `include/recordSchema.h`

Purpose
- Load CSV data into an in-memory `record **` representation and provide helpers to build B+ tree indexes from those records.
- Note: Each implementation (Serial, OpenMP, MPI) has its own build engine file (e.g., `buildEngine-serial.c`, `buildEngine-omp.c`, `buildEngine-mpi.c`).

Key functions and behavior
- `record **getAllRecordsFromFile(const char *filepath, int *num_records)`
	- Opens the CSV file and calls `getRecordFromLine` for each line.
	- Returns a `malloc`-allocated array of `record *` and writes the record count into `*num_records`.

- `record *getRecordFromLine(char *line)`
	- Parses a single CSV line into a `record` object. Fields are converted and copied into the in-memory `record` structure.

- `node *loadIntoBplusTree(record **records, int num_records, const char *attributeName)`
	- Iterates the `records` array and for each `record` uses `extract_key_from_record` to produce a `KEY_T` and then `insert()` into a `node *` root.

- `bool makeIndexSerial(struct engineS *engine, const char *indexName, int attributeType)`
	- Wrapper used by `initializeEngineSerial` to create indexes. Stores root pointers in `engine->bplus_tree_roots` and tracks attribute names.

Helper utilities
- `const FieldInfo *get_field_info(const char *name)` and `KEY_T extract_key_from_record(const record *rec, const char *attr_name)` (in `engine/recordSchema.c`) map names to offsets and create `KEY_T` values using the correct underlying type.

Design notes
- Index creation currently performs repeated insertions in a loop; a bulk-loading approach would be much faster for large CSV datasets.
- `getRecordFromLine` uses truncated copies into fixed-size `char` fields in `record`. Long CSV fields may be truncated — the codebase uses bounded `strncpy` or similar.

---

## Section 3 — Execute Engine

Files: `engine/*/executeEngine-*.c`, `include/executeEngine-*.h`

Purpose
- Implements application-level query execution (SELECT / INSERT / DELETE) and query predicate evaluation. Connects in-memory records, persistent CSV storage, and B+ tree indexes.
- Note: Each implementation (Serial, OpenMP, MPI) has its own execute engine file.

Core types
- `struct engineS` — engine state with fields for in-memory records, index roots, index metadata, and the CSV datafile path.
- `struct resultSetS` — used to return query results with column names, types, and a `char ***` matrix of data.
- `struct whereClauseS` — representation for parsed WHERE expressions; supports chaining and nested sub-expressions.

Predicate evaluation
- `where_condition_func create_where_condition(const char *attribute, const char *operator, void *value)`
	- A factory returning function pointers for typed comparisons. The comparisons are implemented by macros `CMP_NUM` and `CMP_STR` which emit small typed functions for attributes such as `command_id`, `exit_code`, `user_id`, `raw_command`, etc.

- `bool checkCondition(record *r, struct whereClauseS *condition)`
	- Converts the `condition->value` string to the appropriate C type depending on `condition->attribute` (e.g., `unsigned long long` for `command_id`, `int` for `user_id`, `bool` for `sudo_used`).
	- Calls the typed comparison function returned by `create_where_condition`.

- `bool evaluateWhereClause(record *r, struct whereClauseS *wc)`
	- Recursively evaluates `wc`, supporting `sub` expressions (parentheses) and `next` with `logical_op` (AND/OR).

SELECT: `executeQuerySelectSerial`
- Steps taken by the implementation:
	1. Scan the WHERE clause to find indexed attributes. For indexed numeric attributes, translate operators into a `KEY_T` range.
	2. For each indexed attribute match call `findRange` to retrieve candidate row pointers.
	3. If no index applies, perform `linearSearchRecords` across `engine->all_records`.
	4. When candidate results exist, apply `evaluateWhereClause` to each candidate to ensure full predicate match.
	5. Project requested columns into a `resultSetS` (2D string matrix), converting types via `get_attribute_string_value`.

INSERT: `executeQueryInsertSerial`
- Appends a CSV line to `engine->datafile`, adds a heap-copied `record` into `engine->all_records`, increments `engine->num_records`, and updates each B+ tree index using `insert()`.

DELETE: `executeQueryDeleteSerial`
- Steps performed:
	1. Iterate `engine->all_records` and use `evaluateWhereClause` to determine deletion candidates.
	2. For each deleted record, extract indexed keys and call `delete()` on the corresponding B+ tree, updating `engine->bplus_tree_roots[j]` with the return value.
	3. Free deleted `record`s, compact `engine->all_records`, set new `engine->num_records`.
	4. Rewrites the CSV file by opening it with `w` and reprinting remaining records in CSV format.

Behavioral notes
- The delete logic persists changes by rewriting the CSV file. This is simple and reliable but can be slow for large files; alternatives include append-only logs and compaction.
- Index deletions rely on the implemented B+ tree `delete()` — if deletion is broken, indexes will become stale and must be rebuilt via `makeIndexSerial`.

Utilities
- `get_attribute_string_value(record *r, const char *attribute)` — return a `strdup`'d string representation for any attribute. Useful for result projection.
- `linearSearchRecords(...)` — helper to filter arrays of records according to a `whereClauseS`.

---

## Section 4 — File / Function Cross Reference

- `include/bplus.h` — `node`, `KEY_T`, prototypes: `insert`, `delete`, `find_row`, `findRange`, `findLeaf`, `compare_keys`.
- `engine/bplus.c` — B+ tree insertion, split, deletion, find, and printing.
- `engine/serial/buildEngine-serial.c` — `getAllRecordsFromFile`, `getRecordFromLine`, `loadIntoBplusTree`, `makeIndexSerial`.
- `engine/recordSchema.c`, `include/recordSchema.h` — `extract_key_from_record`, `compare_key`, and `get_field_info`.
- `engine/serial/executeEngine-serial.c` — query execution, WHERE evaluation, result formatting, persistence.

---

## Section 5 — Examples and Recipes

1. Create engine and index at startup (conceptual):

```c
const char *indexes[] = {"command_id"};
int types[] = {0}; // 0 -> uint64
struct engineS *engine = initializeEngineSerial(1, indexes, types, "data/commands_50k.csv", "commands");
// engine->bplus_tree_roots[0] now contains a B+ tree indexing command_id
```

2. Simple SELECT using an index (conceptual):

```c
// Parser produces whereClauseS for `command_id = 123`
struct resultSetS *res = executeQuerySelectSerial(engine, (const char*[]){"raw_command"}, 1, "commands", &whereClause);
// Iterate res->data[r][c]
freeResultSet(res);
```

3. DELETE with index maintenance (runtime behavior):

```c
// executeQueryDeleteSerial removes matching records from engine->all_records, updates B+ trees via delete(),
// and rewrites the CSV file.
struct resultSetS *res = executeQueryDeleteSerial(engine, "commands", &whereClause);
```
