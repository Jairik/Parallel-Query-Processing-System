/* Serial Execute Engine */
/* 
 * This header defines the interface for the serial query execution engine.
 * 
 * REFACTORING NOTES:
 * The API has been redesigned to move away from passing large query structs (e.g., querySelectS)
 * and instead uses direct function arguments for better flexibility and performance.
 * 
 * Key Changes:
 * 1. Deconstructed Query Structs: Functions like executeQuerySelectSerial now take individual 
 *    parameters (selectItems, tableName, whereClause) instead of a single struct.
 * 2. Dynamic WHERE Clauses: Introduced `struct whereClauseS`, a linked list structure that 
 *    allows for complex filtering conditions (AND/OR logic) to be constructed dynamically.
 * 3. In-Memory Data Loading: The engine now operates on an in-memory array of records 
 *    (`all_records`) for faster access, with B+ tree indexes used for optimized lookups.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logType.h"

/* Struct for the engine */
/* Holds the state of the database engine, including all data records and active indexes. */
struct engineS {
    char *tableName; // Name of the table represented by this engine
    node **bplus_tree_roots; // Array of roots for all B+ tree indexes
    int num_indexes; // Number of indexes
    char **indexed_attributes; // Names of indexed attributes
    int *attribute_types; // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
    record **all_records; // Array of all records in the table (for full table scans on non-indexed queries and for assigning row pointers)
    int num_records; // Total number of records in the table
    char *datafile; // Path to the data file
} engineS;

/* WHERE clause struct to hold filtering conditions */
/* 
 * Represents a single condition in a WHERE clause (e.g., "risk_level > 2").
 * Can be chained together to form complex queries (e.g., "risk_level > 2 AND user_id = 101").
 */
struct whereClauseS {
    const char *attribute;  // Attribute name to filter on (e.g., "risk_level")
    const char *operator;   // Comparison operator (=, !=, <, >, <=, >=)
    const char *value;      // Value to compare against (as string, converted internally based on type)
    int value_type;         // Type of value (0 = integer, 1 = string, 2 = boolean)
    struct whereClauseS *next;  // Pointer to the next condition in the chain (or NULL)
    const char *logical_op; // Logical operator connecting to next condition ("AND", "OR")
} whereClauseS;

// Function pointers for non-numerical comparisons
typedef bool (*compare_func_t)(const char *, const char *);  // Comparing strings
typedef bool (*compare_func_int_t)(const bool, const bool);  // Comparing booleans

// Select function - main entry point for SELECT queries. Returns results as a string
/* 
 * Executes a SELECT query.
 * - selectItems: Array of column names to retrieve.
 * - whereClause: Linked list of filtering conditions.
 * Returns a formatted string containing the query results.
 */
char *executeQuerySelectSerial(
    struct engineS *engine,        // Engine object
    const char **selectItems,      // Attributes to select (NULL for all)
    int numSelectItems,            // Number of attributes to select
    const char *tableName,         // Table to query from
    struct whereClauseS *whereClause  // WHERE clause (NULL if no filtering)
);

// Insert function - main entry point for INSERT queries. Returns success/failure
/* 
 * Executes an INSERT query.
 * - insertItems: Array of [Attribute, Value] pairs to insert.
 * Updates both the main record storage and any relevant B+ tree indexes.
 */
bool executeQueryInsertSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to insert into
    const record r,       // Record to insert as array of [Attribute, Value] pairs
);

// Update function - main entry point for UPDATE queries. Returns number of rows updated
/* 
 * Executes an UPDATE query.
 * - setItems: Array of [Attribute, Value] pairs to update.
 * - whereClause: Filters which rows to update.
 * Returns the count of modified records.
 */
int executeQueryUpdateSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to update
    const char *(*setItems)[2],          // Array of attribute-value pairs to set
    int numSetItems,                     // Number of attribute-value pairs
    struct whereClauseS *whereClause     // WHERE clause (NULL for all rows)
);

// Delete function - main entry point for DELETE queries. Returns number of rows deleted
/* 
 * Executes a DELETE query.
 * - whereClause: Filters which rows to delete.
 * Returns the count of deleted records.
 */
int executeQueryDeleteSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to delete from
    struct whereClauseS *whereClause     // WHERE clause (NULL for all rows)
);  

// Initialize the engine, returning a pointer to the engine object
struct engineS *initializeEngineSerial(
    int num_indexes,  // Total number of indexes to start 
    const char *indexed_attributes[],  // Names of indexed attributes
    const int attribute_types[],   // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
    const char *datafile,  // Path to the data file
    const char *tableName  // Name of the table
);

// Destroy the engine and free resources
void destroyEngineSerial(struct engineS *engine);

// Declare an attribute to be indexed in the serial engine
bool addAttributeIndexSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table name
    const char *attributeName,  // Name of the attribute to index
    int attributeType  // Attribute type to index (0 = integer, 1 = string, 2 = boolean)
);