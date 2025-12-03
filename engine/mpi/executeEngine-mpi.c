/* Main engine functionality and whatnot */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include <strings.h> // For strcasecmp
#include <time.h> // For clock_t, clock(), CLOCKS_PER_SEC
#include <limits.h> // For INT_MAX, INT_MIN, UINT64_MAX
#include "../../include/buildEngine-mpi.h"
#include "../../include/executeEngine-mpi.h"
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#define VERBOSE 0

// Function pointer type for WHERE condition evaluation
typedef bool (*where_condition_func)(void *record, void *value);

// Helper macros for generating comparison functions
// These macros create type-specific comparison functions for different fields
// CMP_NUM handles numeric types (int, unsigned long long, bool)
// CMP_STR handles string types using strcmp
#define CMP_NUM(field, type, opname, op) \
    bool field##_##opname(void *r, void *v) { \
        return ((record *)r)->field op (*(type *)v); \
    }

#define CMP_STR(field, opname, op) \
    bool field##_##opname(void *r, void *v) { \
        return strcmp(((record *)r)->field, (char *)v) op 0; \
    }

// Generate comparison functions for supported fields
// Each block generates a set of functions (eq, neq, gt, lt, gte, lte) for a specific field
// These functions are used by the query engine to evaluate WHERE clauses

// command_id (unsigned long long)
CMP_NUM(command_id, unsigned long long, eq, ==)
CMP_NUM(command_id, unsigned long long, neq, !=)
CMP_NUM(command_id, unsigned long long, gt, >)
CMP_NUM(command_id, unsigned long long, lt, <)
CMP_NUM(command_id, unsigned long long, gte, >=)
CMP_NUM(command_id, unsigned long long, lte, <=)

// exit_code (int)
CMP_NUM(exit_code, int, eq, ==)
CMP_NUM(exit_code, int, neq, !=)
CMP_NUM(exit_code, int, gt, >)
CMP_NUM(exit_code, int, lt, <)
CMP_NUM(exit_code, int, gte, >=)
CMP_NUM(exit_code, int, lte, <=)

// risk_level (int)
CMP_NUM(risk_level, int, eq, ==)
CMP_NUM(risk_level, int, neq, !=)
CMP_NUM(risk_level, int, gt, >)
CMP_NUM(risk_level, int, lt, <)
CMP_NUM(risk_level, int, gte, >=)
CMP_NUM(risk_level, int, lte, <=)

// user_id (int)
CMP_NUM(user_id, int, eq, ==)
CMP_NUM(user_id, int, neq, !=)
CMP_NUM(user_id, int, gt, >)
CMP_NUM(user_id, int, lt, <)
CMP_NUM(user_id, int, gte, >=)
CMP_NUM(user_id, int, lte, <=)

// raw_command (string)
CMP_STR(raw_command, eq, ==)
CMP_STR(raw_command, neq, !=)
// String inequality is lexicographical
CMP_STR(raw_command, gt, >)
CMP_STR(raw_command, lt, <)
CMP_STR(raw_command, gte, >=)
CMP_STR(raw_command, lte, <=)

// user_name (string)
CMP_STR(user_name, eq, ==)
CMP_STR(user_name, neq, !=)
CMP_STR(user_name, gt, >)
CMP_STR(user_name, lt, <)
CMP_STR(user_name, gte, >=)
CMP_STR(user_name, lte, <=)

// base_command (string)
CMP_STR(base_command, eq, ==)
CMP_STR(base_command, neq, !=)
CMP_STR(base_command, gt, >)
CMP_STR(base_command, lt, <)
CMP_STR(base_command, gte, >=)
CMP_STR(base_command, lte, <=)

// shell_type (string)
CMP_STR(shell_type, eq, ==)
CMP_STR(shell_type, neq, !=)
CMP_STR(shell_type, gt, >)
CMP_STR(shell_type, lt, <)
CMP_STR(shell_type, gte, >=)
CMP_STR(shell_type, lte, <=)

// timestamp (string)
CMP_STR(timestamp, eq, ==)
CMP_STR(timestamp, neq, !=)
CMP_STR(timestamp, gt, >)
CMP_STR(timestamp, lt, <)
CMP_STR(timestamp, gte, >=)
CMP_STR(timestamp, lte, <=)

// working_directory (string)
CMP_STR(working_directory, eq, ==)
CMP_STR(working_directory, neq, !=)
CMP_STR(working_directory, gt, >)
CMP_STR(working_directory, lt, <)
CMP_STR(working_directory, gte, >=)
CMP_STR(working_directory, lte, <=)

// host_name (string)
CMP_STR(host_name, eq, ==)
CMP_STR(host_name, neq, !=)
CMP_STR(host_name, gt, >)
CMP_STR(host_name, lt, <)
CMP_STR(host_name, gte, >=)
CMP_STR(host_name, lte, <=)

// sudo_used (bool)
CMP_NUM(sudo_used, bool, eq, ==)
CMP_NUM(sudo_used, bool, neq, !=)


// Factory function to create a WHERE condition function pointer
// Takes an attribute name, operator, and value, and returns the appropriate comparison function
// This allows the query engine to dynamically select the correct comparison logic at runtime
where_condition_func create_where_condition(const char *attribute, const char *operator, void *value) {
    if (strcmp(attribute, "command_id") == 0) {
        if (strcmp(operator, "=") == 0) return command_id_eq;
        if (strcmp(operator, "!=") == 0) return command_id_neq;
        if (strcmp(operator, ">") == 0) return command_id_gt;
        if (strcmp(operator, "<") == 0) return command_id_lt;
        if (strcmp(operator, ">=") == 0) return command_id_gte;
        if (strcmp(operator, "<=") == 0) return command_id_lte;
    } else if (strcmp(attribute, "exit_code") == 0) {
        if (strcmp(operator, "=") == 0) return exit_code_eq;
        if (strcmp(operator, "!=") == 0) return exit_code_neq;
        if (strcmp(operator, ">") == 0) return exit_code_gt;
        if (strcmp(operator, "<") == 0) return exit_code_lt;
        if (strcmp(operator, ">=") == 0) return exit_code_gte;
        if (strcmp(operator, "<=") == 0) return exit_code_lte;
    } else if (strcmp(attribute, "risk_level") == 0) {
        if (strcmp(operator, "=") == 0) return risk_level_eq;
        if (strcmp(operator, "!=") == 0) return risk_level_neq;
        if (strcmp(operator, ">") == 0) return risk_level_gt;
        if (strcmp(operator, "<") == 0) return risk_level_lt;
        if (strcmp(operator, ">=") == 0) return risk_level_gte;
        if (strcmp(operator, "<=") == 0) return risk_level_lte;
    } else if (strcmp(attribute, "user_id") == 0) {
        if (strcmp(operator, "=") == 0) return user_id_eq;
        if (strcmp(operator, "!=") == 0) return user_id_neq;
        if (strcmp(operator, ">") == 0) return user_id_gt;
        if (strcmp(operator, "<") == 0) return user_id_lt;
        if (strcmp(operator, ">=") == 0) return user_id_gte;
        if (strcmp(operator, "<=") == 0) return user_id_lte;
    } else if (strcmp(attribute, "raw_command") == 0) {
        if (strcmp(operator, "=") == 0) return raw_command_eq;
        if (strcmp(operator, "!=") == 0) return raw_command_neq;
        if (strcmp(operator, ">") == 0) return raw_command_gt;
        if (strcmp(operator, "<") == 0) return raw_command_lt;
        if (strcmp(operator, ">=") == 0) return raw_command_gte;
        if (strcmp(operator, "<=") == 0) return raw_command_lte;
    } else if (strcmp(attribute, "user_name") == 0) {
        if (strcmp(operator, "=") == 0) return user_name_eq;
        if (strcmp(operator, "!=") == 0) return user_name_neq;
        if (strcmp(operator, ">") == 0) return user_name_gt;
        if (strcmp(operator, "<") == 0) return user_name_lt;
        if (strcmp(operator, ">=") == 0) return user_name_gte;
        if (strcmp(operator, "<=") == 0) return user_name_lte;
    } else if (strcmp(attribute, "base_command") == 0) {
        if (strcmp(operator, "=") == 0) return base_command_eq;
        if (strcmp(operator, "!=") == 0) return base_command_neq;
        if (strcmp(operator, ">") == 0) return base_command_gt;
        if (strcmp(operator, "<") == 0) return base_command_lt;
        if (strcmp(operator, ">=") == 0) return base_command_gte;
        if (strcmp(operator, "<=") == 0) return base_command_lte;
    } else if (strcmp(attribute, "shell_type") == 0) {
        if (strcmp(operator, "=") == 0) return shell_type_eq;
        if (strcmp(operator, "!=") == 0) return shell_type_neq;
        if (strcmp(operator, ">") == 0) return shell_type_gt;
        if (strcmp(operator, "<") == 0) return shell_type_lt;
        if (strcmp(operator, ">=") == 0) return shell_type_gte;
        if (strcmp(operator, "<=") == 0) return shell_type_lte;
    } else if (strcmp(attribute, "timestamp") == 0) {
        if (strcmp(operator, "=") == 0) return timestamp_eq;
        if (strcmp(operator, "!=") == 0) return timestamp_neq;
        if (strcmp(operator, ">") == 0) return timestamp_gt;
        if (strcmp(operator, "<") == 0) return timestamp_lt;
        if (strcmp(operator, ">=") == 0) return timestamp_gte;
        if (strcmp(operator, "<=") == 0) return timestamp_lte;
    } else if (strcmp(attribute, "working_directory") == 0) {
        if (strcmp(operator, "=") == 0) return working_directory_eq;
        if (strcmp(operator, "!=") == 0) return working_directory_neq;
        if (strcmp(operator, ">") == 0) return working_directory_gt;
        if (strcmp(operator, "<") == 0) return working_directory_lt;
        if (strcmp(operator, ">=") == 0) return working_directory_gte;
        if (strcmp(operator, "<=") == 0) return working_directory_lte;
    } else if (strcmp(attribute, "host_name") == 0) {
        if (strcmp(operator, "=") == 0) return host_name_eq;
        if (strcmp(operator, "!=") == 0) return host_name_neq;
        if (strcmp(operator, ">") == 0) return host_name_gt;
        if (strcmp(operator, "<") == 0) return host_name_lt;
        if (strcmp(operator, ">=") == 0) return host_name_gte;
        if (strcmp(operator, "<=") == 0) return host_name_lte;
    } else if (strcmp(attribute, "sudo_used") == 0) {
        if (strcmp(operator, "=") == 0) return sudo_used_eq;
        if (strcmp(operator, "!=") == 0) return sudo_used_neq;
    }
    
    return NULL;
}

/* Helper: Converts a specific attribute of a record to a string */
char *get_attribute_string_value(record *r, const char *attribute) {
    char buffer[4096]; // Large buffer for safety
    
    if (strcmp(attribute, "command_id") == 0) {
        sprintf(buffer, "%llu", r->command_id);
    } else if (strcmp(attribute, "raw_command") == 0) {
        return strdup(r->raw_command); // Already a string
    } else if (strcmp(attribute, "base_command") == 0) {
        return strdup(r->base_command);
    } else if (strcmp(attribute, "shell_type") == 0) {
        return strdup(r->shell_type);
    } else if (strcmp(attribute, "exit_code") == 0) {
        sprintf(buffer, "%d", r->exit_code);
    } else if (strcmp(attribute, "timestamp") == 0) {
        return strdup(r->timestamp);
    } else if (strcmp(attribute, "sudo_used") == 0) {
        return strdup(r->sudo_used ? "true" : "false");
    } else if (strcmp(attribute, "working_directory") == 0) {
        return strdup(r->working_directory);
    } else if (strcmp(attribute, "user_id") == 0) {
        sprintf(buffer, "%d", r->user_id);
    } else if (strcmp(attribute, "user_name") == 0) {
        return strdup(r->user_name);
    } else if (strcmp(attribute, "host_name") == 0) {
        return strdup(r->host_name);
    } else if (strcmp(attribute, "risk_level") == 0) {
        sprintf(buffer, "%d", r->risk_level);
    } else {
        return strdup("NULL"); // Unknown attribute
    }
    
    return strdup(buffer);
}

/* Helper function to check a single condition against a record */
bool checkCondition(record *r, struct whereClauseS *condition) {
    void *valuePtr;
    bool result = false;
    
    // 1. Determine specific C type based on Attribute Name
    if (strcmp(condition->attribute, "command_id") == 0) {
        unsigned long long *val = malloc(sizeof(unsigned long long));
        *val = strtoull(condition->value, NULL, 10);
        valuePtr = val;
    } 
    else if (strcmp(condition->attribute, "risk_level") == 0 || 
             strcmp(condition->attribute, "exit_code") == 0 ||
             strcmp(condition->attribute, "user_id") == 0) {
        int *val = malloc(sizeof(int));
        *val = atoi(condition->value);
        valuePtr = val;
    }
    else if (strcmp(condition->attribute, "sudo_used") == 0) {
        bool *val = malloc(sizeof(bool));
        *val = (strcasecmp(condition->value, "true") == 0 || strcmp(condition->value, "1") == 0);
        valuePtr = val;
    }
    else {
        // String types
        valuePtr = (void *)condition->value;
    }

    where_condition_func conditionFunc = create_where_condition(condition->attribute, condition->operator, valuePtr);
    if (conditionFunc != NULL && conditionFunc((void *)r, valuePtr)) {
        result = true;
    }
    
    // Cleanup
    if (valuePtr != condition->value) {
        free(valuePtr);
    }
    
    return result;
}

/* Recursive evaluator for WHERE clause */
bool evaluateWhereClause(record *r, struct whereClauseS *wc) {
    if (wc == NULL) return true;

    bool currentResult = false;

    if (wc->sub != NULL) {
        currentResult = evaluateWhereClause(r, wc->sub);
    } else {
        currentResult = checkCondition(r, wc);
    }

    if (wc->next == NULL) {
        return currentResult;
    }

    if (wc->logical_op != NULL) {
        if (strcmp(wc->logical_op, "OR") == 0) {
            return currentResult || evaluateWhereClause(r, wc->next);
        } else if (strcmp(wc->logical_op, "AND") == 0) {
            return currentResult && evaluateWhereClause(r, wc->next);
        }
    }
    
    return currentResult && evaluateWhereClause(r, wc->next);
}

/* Main functionality for a SELECT query
 * Parameters:
*   engine - constant engine object
*   selectItems - attributes to select (SELECT clause)
*   numItems - number of attributes to select (NULL for all)
*   tableName - table to query from (FROM clause)
*   whereClause - WHERE clause (NULL if no filtering)
* Returns:
*    A 
*/
struct resultSetS *executeQuerySelectSerial(
    struct engineS *engine,  // Constant engine object
    const char *selectItems[],  // Attributes to select (SELECT clause)
    int numItems,  // Number of attributes to select (NULL for all)
    const char *tableName,  // Table to query from (FROM clause)
    struct whereClauseS *whereClause  // WHERE clause (NULL if no filtering)
) {

    // Flags to track if any indexed attributes are in the WHERE clause
    bool anyIndexExists = false;  // Flag for quick fallback to full table scan
    bool indexExists[engine->num_indexes];  // Track which indexes exist for WHERE attributes

    // Allocate space for matching records and result struct
    // Use num_records as upper bound to avoid reallocating during index search
    record **matchingRecords = (record **)malloc(engine->num_records * sizeof(record *));
    struct resultSetS *queryResults = (struct resultSetS *)malloc(sizeof(struct resultSetS));
    int matchCount = 0;

    // Initialize queryResults object
    queryResults->numRecords = 0;
    queryResults->numColumns = 0;
    queryResults->columnNames = NULL;
    queryResults->data = NULL;
    queryResults->queryTime = 0.0;
    queryResults->success = false;

    // Start the timer
    clock_t start = clock();  // Start a timer

    // Get all indexed attributes in the WHERE clause, using the B+ tree indexes where possible
    struct whereClauseS *wc = whereClause;
    while (wc != NULL) {
        for (int i = 0; i < engine->num_indexes; i++) {
            if (strcmp(wc->attribute, engine->indexed_attributes[i]) == 0) {
                anyIndexExists = true;
                indexExists[i] = true;

                // Use B+ tree index for this attribute
                node *cur_root = engine->bplus_tree_roots[i]; // B+ tree root for this indexed attribute
                FieldType type = engine->attribute_types[i];
                
                KEY_T key_start, key_end;
                bool typeSupported = true;

                if (type == FIELD_UINT64) {
                    unsigned long long val = strtoull(wc->value, NULL, 10);
                    key_start.type = KEY_UINT64;
                    key_end.type = KEY_UINT64;
                    
                    if (strcmp(wc->operator, "=") == 0) {
                        key_start.v.u64 = val;
                        key_end.v.u64 = val;
                    } else if (strcmp(wc->operator, ">") == 0) {
                        key_start.v.u64 = val + 1;
                        key_end.v.u64 = UINT64_MAX;
                    } else if (strcmp(wc->operator, ">=") == 0) {
                        key_start.v.u64 = val;
                        key_end.v.u64 = UINT64_MAX;
                    } else if (strcmp(wc->operator, "<") == 0) {
                        key_start.v.u64 = 0;
                        key_end.v.u64 = val - 1;
                    } else if (strcmp(wc->operator, "<=") == 0) {
                        key_start.v.u64 = 0;
                        key_end.v.u64 = val;
                    } else {
                        key_start.v.u64 = 0;
                        key_end.v.u64 = UINT64_MAX;
                    }
                } else if (type == FIELD_INT) {
                    int val = atoi(wc->value);
                    key_start.type = KEY_INT;
                    key_end.type = KEY_INT;
                    
                    if (strcmp(wc->operator, "=") == 0) {
                        key_start.v.i32 = val;
                        key_end.v.i32 = val;
                    } else if (strcmp(wc->operator, ">") == 0) {
                        key_start.v.i32 = val + 1;
                        key_end.v.i32 = INT_MAX;
                    } else if (strcmp(wc->operator, ">=") == 0) {
                        key_start.v.i32 = val;
                        key_end.v.i32 = INT_MAX;
                    } else if (strcmp(wc->operator, "<") == 0) {
                        key_start.v.i32 = INT_MIN;
                        key_end.v.i32 = val - 1;
                    } else if (strcmp(wc->operator, "<=") == 0) {
                        key_start.v.i32 = INT_MIN;
                        key_end.v.i32 = val;
                    } else {
                        key_start.v.i32 = INT_MIN;
                        key_end.v.i32 = INT_MAX;
                    }
                } else {
                    // Fallback for unsupported types in index search
                    typeSupported = false;
                    indexExists[i] = false;
                }

                if (!typeSupported) {
                    continue;
                }

                // Allocating for keys, using num_records as upper bound.
                KEY_T *returned_keys = malloc(engine->num_records * sizeof(KEY_T));
                ROW_PTR *returned_pointers = malloc(engine->num_records * sizeof(ROW_PTR));
                
                int num_found = findRange(cur_root, key_start, key_end, false, returned_keys, returned_pointers);
                
                // Add found records to matchingRecords
                if (num_found > 0) {
                    // Reallocate matchingRecords if needed (though we alloc'd max size initially)
                    for (int k = 0; k < num_found; k++) {
                        matchingRecords[matchCount++] = (record *)returned_pointers[k];
                    }
                }
                
                free(returned_keys);
                free(returned_pointers);
            }
            else {
                indexExists[i] = false;
            }
        }
        wc = wc->next;
    }


    // Perform linear search if on all non-indexed attributes or no indexes matched
    // No indexes exist for any WHERE attributes, search entire table
    if(!anyIndexExists){
        free(matchingRecords); // Free the empty one we made
        matchingRecords = linearSearchRecords(engine->all_records, engine->num_records, whereClause, &matchCount);
    }
    // There were some indexes in the WHERE clause, so we can use the known matching records to reduce linear search
    else{
        // Filter the candidate records found via index against the full WHERE clause
        record **filteredRecords = linearSearchRecords(matchingRecords, matchCount, whereClause, &matchCount);
        free(matchingRecords);
        matchingRecords = filteredRecords;
    }
    double end = (double) clock();
    double time_taken = ((double) end - start) / CLOCKS_PER_SEC;  // Time in seconds
    if (VERBOSE) {
        printf("Linear search took %f seconds\n", time_taken);
    }

    // Extract the requested attributes from matching records and format the result
    queryResults->numRecords = matchCount;
    
    // Handle "SELECT *" case (if selectItems is NULL or empty)
    const char *all_columns[] = {"command_id", "raw_command", "base_command", "shell_type", 
                                 "exit_code", "timestamp", "sudo_used", "working_directory", 
                                 "user_id", "user_name", "host_name", "risk_level"};
    int total_columns_count = 12;

    if (selectItems == NULL || numItems == 0) {
        queryResults->numColumns = total_columns_count;
        selectItems = all_columns; // Point to our static list
    } else {
        queryResults->numColumns = numItems;
    }

    // Allocate memory for column headers
    queryResults->columnNames = (char **)malloc(queryResults->numColumns * sizeof(char *));
    for (int i = 0; i < queryResults->numColumns; i++) {
        queryResults->columnNames[i] = strdup(selectItems[i]);
    }

    // Allocate memory for data matrix [rows][cols]
    queryResults->data = (char ***)malloc(queryResults->numRecords * sizeof(char **));

    // Populate the matrix
    for (int i = 0; i < queryResults->numRecords; i++) {
        queryResults->data[i] = (char **)malloc(queryResults->numColumns * sizeof(char *));
        record *r = matchingRecords[i];
        
        for (int j = 0; j < queryResults->numColumns; j++) {
            // Extract ONLY the attribute requested in selectItems[j]
            queryResults->data[i][j] = get_attribute_string_value(r, selectItems[j]);
        }
    }

    // Clean up the temporary array of pointers (temp array, NOT records themselves)
    free(matchingRecords);
    
    queryResults->queryTime = time_taken;
    queryResults->success = true;
    
    // Allocate column types (placeholder)
    queryResults->columnTypes = (FieldType *)malloc(queryResults->numColumns * sizeof(FieldType));
    memset(queryResults->columnTypes, 0, queryResults->numColumns * sizeof(FieldType));

    return queryResults;
}

/* Main functionality for INSERT logic
 * Parameters:
 *   engine - constant engine object
 *   tableName - name of the table
 *   newRecord - record to insert as Record type
 * Returns:
 *   success/failure
*/
bool executeQueryInsertSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to insert into
    const record *newRecord  // Record to insert as array of [Attribute, Value] pairs
) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Check that the record is valid/missing no fields
    if(newRecord->command_id == 0 || strlen(newRecord->raw_command) == 0 || strlen(newRecord->base_command) == 0 ||
       strlen(newRecord->shell_type) == 0 || strlen(newRecord->timestamp) == 0 || strlen(newRecord->working_directory) == 0 ||
       strlen(newRecord->user_name) == 0 || strlen(newRecord->host_name) == 0) {
        if (VERBOSE && rank == 0) {
            fprintf(stderr, "Invalid record: missing required fields\n");
        }
        return false;
    }

    // Append the new record to the end of the data file (as a CSV)
    // Only Rank 0 writes to the file to avoid race conditions and file corruption
    if (rank == 0) {
        FILE *file = fopen(engine->datafile, "a");
        if (file == NULL) {
            if (VERBOSE) {
                fprintf(stderr, "Failed to open data file for appending: %s\n", engine->datafile);
            }
            // In a real system we should broadcast this error, but for now we'll just return false on rank 0
            // Ideally we'd have an MPI_Bcast for success/failure here.
            return false;
        }
        // Write the new record as a CSV line
        fprintf(file, "%llu,%s,%s,%s,%d,%s,%d,%s,%d,%s,%s,%d\n",
                newRecord->command_id,
                newRecord->raw_command,
                newRecord->base_command,
                newRecord->shell_type,
                newRecord->exit_code,
                newRecord->timestamp,
                newRecord->sudo_used,
                newRecord->working_directory,
                newRecord->user_id,
                newRecord->user_name,
                newRecord->host_name,
                newRecord->risk_level);
        fclose(file);
    }

    // Append the new record to engine->all_records in memory
    // ALL ranks must update their local copy of the records so they can be used for future queries
    engine->all_records = (record **)realloc(engine->all_records, (engine->num_records + 1) * sizeof(record *));
    if (engine->all_records == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Memory reallocation failed for all_records on rank %d\n", rank);
        }
        return false;
    }
    record *record_copy = (record *)malloc(sizeof(record));
    if (record_copy == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Memory allocation failed for new record on rank %d\n", rank);
        }
        return false;
    }
    *record_copy = *newRecord;  // Copy the contents of newRecord (properly allocating memory outside function scope)
    engine->all_records[engine->num_records] = record_copy;

    // Increment the record count
    engine->num_records += 1;

    // Update any relevant B+ tree indexes to include the new record
    // Distribute the index updates among the ranks
    // Each rank is responsible for a subset of the indexes: i % size == rank
    bool success = true;
    for (int i = 0; i < engine->num_indexes; i++) {
        if (i % size == rank) {
            const char *indexed_attr = engine->indexed_attributes[i];
            
            // Insert the new record into the B+ tree for this indexed attribute
            node *root = engine->bplus_tree_roots[i];
            KEY_T key = extract_key_from_record(record_copy, indexed_attr);
            root = insert(root, key, (ROW_PTR)record_copy);
            engine->bplus_tree_roots[i] = root;
            
            if(root == NULL) {
                if (VERBOSE) {
                    fprintf(stderr, "Failed to insert new record into B+ tree for attribute: %s on rank %d\n", indexed_attr, rank);
                }
                success = false;
            }
        }
    }

    // Optional: Synchronize success status across all ranks?
    // For now, we return local success. The query engine might need to aggregate this.
    
    return success;
}

// assumes same external declarations as in the OpenMP version

struct resultSetS *executeQueryDeleteMPI(
    struct engineS *engine,
    const char *tableName,
    struct whereClauseS *whereClause,
    MPI_Comm comm
) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    struct resultSetS *result = (struct resultSetS *)malloc(sizeof(struct resultSetS));
    if (!result) {
        return NULL;
    }

    result->numRecords   = 0;
    result->numColumns   = 0;
    result->columnNames  = NULL;
    result->columnTypes  = NULL;
    result->data         = NULL;
    result->queryTime    = 0.0;
    result->success      = false;

    double start = MPI_Wtime();

    // We assume num_records is the same on all ranks (e.g., broadcasted beforehand if needed)
    int num_records = engine->num_records;

    // Block partition of records across ranks
    int base = num_records / size;
    int rem  = num_records % size;

    int local_n;
    int local_start;

    if (rank < rem) {
        local_n     = base + 1;
        local_start = rank * (base + 1);
    } else {
        local_n     = base;
        local_start = rem * (base + 1) + (rank - rem) * base;
    }

    // Local flags for this rank's chunk
    int *localFlags = (local_n > 0)
        ? (int *)calloc(local_n, sizeof(int))
        : NULL;

    int localDeleted = 0;

    // Local WHERE evaluation
    for (int i = 0; i < local_n; i++) {
        int globalIdx = local_start + i;
        record *currentRecord = engine->all_records[globalIdx];
        bool shouldDelete;

        if (whereClause == NULL) {
            shouldDelete = true;
        } else {
            shouldDelete = evaluateWhereClause(currentRecord, whereClause);
        }

        if (shouldDelete) {
            localFlags[i] = 1;
            localDeleted++;
        }
    }

    // Total deleted count across all ranks
    int globalDeleted = 0;
    MPI_Reduce(&localDeleted, &globalDeleted, 1, MPI_INT, MPI_SUM, 0, comm);

    // Gather all flags on rank 0
    int *recvCounts = NULL;
    int *displs     = NULL;
    int *globalFlags = NULL;

    if (rank == 0) {
        recvCounts = (int *)malloc(size * sizeof(int));
        displs     = (int *)malloc(size * sizeof(int));
    }

    // Each rank sends its local_n
    MPI_Gather(&local_n, 1, MPI_INT,
               recvCounts, 1, MPI_INT,
               0, comm);

    if (rank == 0) {
        int offset = 0;
        for (int r = 0; r < size; r++) {
            displs[r]     = offset;
            offset       += recvCounts[r];
        }

        globalFlags = (int *)calloc(num_records, sizeof(int));
    }

    MPI_Gatherv(localFlags, local_n, MPI_INT,
                globalFlags, recvCounts, displs, MPI_INT,
                0, comm);

    if (localFlags) {
        free(localFlags);
    }

    // Rank 0 applies deletions, updates indexes, compacts array, writes CSV
    if (rank == 0) {
        int writeIndex = 0;

        for (int i = 0; i < num_records; i++) {
            record *currentRecord = engine->all_records[i];

            if (globalFlags[i]) {
                // Remove from B+ Tree Indexes
                for (int j = 0; j < engine->num_indexes; j++) {
                    const char *indexed_attr = engine->indexed_attributes[j];
                    KEY_T key = extract_key_from_record(currentRecord, indexed_attr);
                    engine->bplus_tree_roots[j] =
                        delete(engine->bplus_tree_roots[j], key, (ROW_PTR)currentRecord);
                }

                // Free record memory
                free(currentRecord);
            } else {
                // Keep record and compact
                if (writeIndex != i) {
                    engine->all_records[writeIndex] = currentRecord;
                }
                writeIndex++;
            }
        }

        engine->num_records = writeIndex;

        // Rewrite CSV with remaining records
        FILE *file = fopen(engine->datafile, "w");
        if (file != NULL) {
            for (int i = 0; i < engine->num_records; i++) {
                record *r = engine->all_records[i];
                fprintf(file, "%llu,%s,%s,%s,%d,%s,%d,%s,%d,%s,%s,%d\n",
                    r->command_id,
                    r->raw_command,
                    r->base_command,
                    r->shell_type,
                    r->exit_code,
                    r->timestamp,
                    r->sudo_used,
                    r->working_directory,
                    r->user_id,
                    r->user_name,
                    r->host_name,
                    r->risk_level);
            }
            fclose(file);
        } else {
            if (VERBOSE) {
                fprintf(stderr,
                        "Failed to open data file for rewriting: %s\n",
                        engine->datafile);
            }
        }

        double time_taken = MPI_Wtime() - start;

        result->numRecords = globalDeleted;
        result->queryTime  = time_taken;
        result->success    = true;

        if (globalFlags) free(globalFlags);
        if (recvCounts)  free(recvCounts);
        if (displs)      free(displs);
    } else {
        // Non-root ranks: result is not meaningful; optionally set success=false explicitly
        result->success = false;
    }

    return result;
}

/* Wrapper for Serial API compatibility */
struct resultSetS *executeQueryDeleteSerial(
    struct engineS *engine,
    const char *tableName,
    struct whereClauseS *whereClause
) {
    return executeQueryDeleteMPI(engine, tableName, whereClause, MPI_COMM_WORLD);
}

/* Initialize the engine, allocating space for default values, loading indexes, and loading the data
 * Parameters:
 *   num_indexes - number of indexes to create
 *   indexed_attributes - names of indexed attributes
 *   attribute_types - types of indexed attributes
 *   datafile - path to the data file
 *   tableName - name of the table
 * Returns:
 *   pointer to initialized engine struct 
*/
struct engineS *initializeEngineSerial(
    int num_indexes,  // Total number of indexes to start 
    const char *indexed_attributes[],  // Names of indexed attributes
    const int attribute_types[],   // Types of indexed attributes (0 = uint, 1 = int, 2 = string, 3 = boolean)
    const char *datafile,  // Path to the data file
    const char *tableName  // Name of the table
) {
    // Allocate memory for a new engine struct object
    struct engineS *engine = (struct engineS *)malloc(sizeof(struct engineS));
    if (engine == NULL) {
        perror("Failed to allocate memory for engine");
        exit(EXIT_FAILURE);
    }

    // Initialize indexes and tree roots
    engine->tableName = strdup(tableName);
    engine->num_indexes = 0; // Start at 0, makeIndexSerial will increment
    engine->bplus_tree_roots = (node **)malloc(num_indexes * sizeof(node *));
    engine->indexed_attributes = (char **)malloc(num_indexes * sizeof(char *));
    engine->attribute_types = (FieldType *)malloc(num_indexes * sizeof(FieldType));
    engine->all_records = NULL; // Initialize to NULL, will be set later
    engine->num_records = 0; // Initialize record count to 0
    if (engine->bplus_tree_roots == NULL || engine->indexed_attributes == NULL || engine->attribute_types == NULL) {
        perror("Failed to allocate memory for engine components");
        free(engine);
        exit(EXIT_FAILURE);
    }

    // Read all records from the database into memory and store in engine->all_records
    if(!datafile){ datafile = "../data/commands_50k.csv"; }; // Filepath default
    engine->datafile = strdup(datafile);
    engine->all_records = getAllRecordsFromFile(datafile, &engine->num_records);  // Directly update record count

    // Copy indexed attribute names and types into engine struct (defaults)
    for (int i = 0; i < num_indexes; i++) {
        // Build B+ tree index for each indexed attribute
        bool success = makeIndexSerial(engine, indexed_attributes[i], attribute_types[i]);
        if (!success) {
            fprintf(stderr, "Failed to create index for attribute: %s\n", indexed_attributes[i]);
        }
    }

    return engine;  // Return the initialized engine
}

/* Safety destroys the engine, freeing all memory */
void destroyEngineSerial(struct engineS *engine) {
    if (engine != NULL) {
        /* Free: B+ tree roots (and their nodes) */
        if (engine->bplus_tree_roots != NULL) {
            for (int i = 0; i < engine->num_indexes; i++) {
                if (engine->bplus_tree_roots[i] != NULL) {
                    destroy_tree(engine->bplus_tree_roots[i]);
                }
            }
            free(engine->bplus_tree_roots);
        }

        /* Free: indexed attribute names */
        if (engine->indexed_attributes != NULL) {
            for (int i = 0; i < engine->num_indexes; i++) {
                if (engine->indexed_attributes[i] != NULL) free(engine->indexed_attributes[i]);
            }
            free(engine->indexed_attributes);
        }

        /* Free: attribute types array */
        if (engine->attribute_types != NULL) free(engine->attribute_types);

        /* Free: all records allocated from file */
        if (engine->all_records != NULL) {
            for (int i = 0; i < engine->num_records; i++) {
                if (engine->all_records[i] != NULL) free(engine->all_records[i]);
            }
            free(engine->all_records);
        }

        /* Free: duplicated strings */
        if (engine->tableName) free(engine->tableName);
        if (engine->datafile) free(engine->datafile);

        /* FREE ENGINE: END */
        free(engine);
    } else {  // Engine should not be NULL, debug check
        fprintf(stderr, "Attempted to destroy a NULL engine pointer\n");
    }
}

/* Adds an index to the engine, generating a new B-tree and appending the index information to the engine struct 
 * Parameters:
 *   engine - constant engine object
 *   tableName - name of the table
 *   attributeName - name of the attribute to index
 *   attributeType - type of the attribute to index (0 = integer, 1 = string, 2 = boolean)
 * Returns:
 *  exit code (true = success, false = failure)
*/
bool addAttributeIndexSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table name
    const char *attributeName,  // Name of the attribute to index
    int attributeType  // Attribute type to index (0 = Uinteger, 1= = int, 2 = string, 3 = boolean)
) {
    // Create a new B+ tree index for the specified attribute
    bool makeIndexSuccess = makeIndexSerial(engine, attributeName, attributeType);
    if (makeIndexSuccess) {
        if (VERBOSE) {
            fprintf(stderr, "Failed to create B+ tree index for attribute: %s\n", attributeName);
        }
        return false;  // Failed to create index
    }

    return true;  // Successfully added the index
}

/* Helper function for determining if a given attribute is indexed */
int isAttributeIndexed(struct engineS *engine, const char *attributeName) {
    for (int i = 0; i < engine->num_indexes; i++) {
        if (strcmp(engine->indexed_attributes[i], attributeName) == 0) {
            return i;  // Return the found attribute's index
        }
    }
    return -1;  // Attribute is not indexed, return -1 as signal value
}

/* Performs a linear search through a given array of records based on the WHERE clause */
record **linearSearchRecords(record **records, int num_records, struct whereClauseS *whereClause, int *matchingRecords) {
    // Allocate space for results array
    record **results = malloc(sizeof(record *));
    *matchingRecords = 0;

    // Iterate through all records and apply WHERE clause filtering
    for(int i = 0; i < num_records; i++) {
        record *currentRecord = records[i];
        bool matches = true;

        if (whereClause != NULL) {
            matches = evaluateWhereClause(currentRecord, whereClause);
        }

        // If record matches all conditions, add to results
        if (matches) {
            results = realloc(results, (*matchingRecords + 1) * sizeof(record *));
            results[*matchingRecords] = currentRecord;
            (*matchingRecords)++;
        }
    }

    // Return the array of matching records
    return results;
}

// Helper function to free a resultSet struct
void freeResultSet(struct resultSetS * result){
    if(result != NULL){
        // Free column names
        if(result->columnNames != NULL){
            for(int i = 0; i < result->numColumns; i++){
                free(result->columnNames[i]);
            }
            free(result->columnNames);
        }
        // Free column types
        if(result->columnTypes != NULL){
            free(result->columnTypes);
        }
        // Free data matrix
        if(result->data != NULL){
            for(int i = 0; i < result->numRecords; i++){
                if(result->data[i] != NULL){
                    for(int j = 0; j < result->numColumns; j++){
                        free(result->data[i][j]);
                    }
                    free(result->data[i]);
                }
            }
            free(result->data);
        }
        // Finally, free the result struct itself
        free(result);
    }
}
