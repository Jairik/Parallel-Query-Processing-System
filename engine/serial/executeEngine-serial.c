/* Main engine functionality and whatnot */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#include <time.h>  // Timing
#include "executeEngine-serial.h"
#define VERBOSE 1

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
    } else if (strcmp(attribute, "sudo_used") == 0) {
        if (strcmp(operator, "=") == 0) return sudo_used_eq;
        if (strcmp(operator, "!=") == 0) return sudo_used_neq;
    }
    
    return NULL;
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
    // TODO - IMPLEMENT LOGIC
    // Requirements:
    // Check if any indexed attributes are in the WHERE clause
    // If indexed attributes are present, use the corresponding B+ tree index to quickly locate matching records
    // If no indexed attributes are present, perform a full table scan of engine->all_records

    // Flags to track if any indexed attributes are in the WHERE clause
    bool anyIndexExists = false;  // Flag for quick fallback to full table scan
    bool indexExists[engine->num_indexes];  // Track which indexes exist for WHERE attributes

    // Allocate space for matching records and result struct
    record **matchingRecords = (record **)malloc(engine->num_records * sizeof(record *));
    struct resultSet *queryResults = (struct resultSet *)malloc(sizeof(struct resultSet));
    int matchCount = 0;

    // Get all indexed attributes in the WHERE clause, using the B+ tree indexes where possible
    // NOTE: THIS IS ONLY A PLACEHOLDER. CHECK TOKENIZER BEHAVIOR FOR WHERE CLAUSE PARSING
    struct whereClauseS *wc = whereClause;
    while (wc != NULL) {
        for (int i = 0; i < engine->num_indexes; i++) {
            if (strcmp(wc->attribute, engine->indexed_attributes[i]) == 0) {
                anyIndexExists = true;
                indexExists[i] = true;

                // TODO - USE B+ TREE TO GET MATCHING RECORDS FOR THIS ATTRIBUTE & WHERE CLAUSE
            }
            else {
                indexExists[i] = false;
            }
        }
        wc = wc->next;
    }

    // Perform index scans on all indexed attributes in the WHERE clause. For non-indexed attributes, fall back to linear search
    // TODO - Filter records based on WHERE clause using indexes where possible


    // Fallback: If no index exists for a WHERE clause attributes, perform a full table scan on currently known attributes
    clock_t start = clock();  // Start a timer
    
    // No indexes exist for any WHERE attributes, search entire table
    if(!anyIndexExists){
        matchingRecords = linearSearchRecords(engine->all_records, whereClause, &matchCount);
    }
    // There were some indexes in the WHERE clause, so we can use the known matching records to reduce linear search
    else{
        matchingRecords = linearSearchRecords(matchingRecords, whereClause, &matchCount);
    }
    double time_taken = ((double)clock() - start) / CLOCKS_PER_SEC;  // Time in seconds
    if (VERBOSE) {
        printf("Linear search took %f seconds\n", time_taken);
    }

    // Extract the requested attributes from matching records and format the result


    // Assign the results to a resultSet struct


    return NULL;  // Placeholder for now
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
    // Check that the record is valid/missing no fields
    if(newRecord->command_id == 0 || strlen(newRecord->raw_command) == 0 || strlen(newRecord->base_command) == 0 ||
       strlen(newRecord->shell_type) == 0 || strlen(newRecord->timestamp) == 0 || strlen(newRecord->working_directory) == 0 ||
       strlen(newRecord->user_name) == 0 || strlen(newRecord->host_name) == 0) {
        if (VERBOSE) {
            fprintf(stderr, "Invalid record: missing required fields\n");
        }
        return false;
    }

    // Append the new record to the end of the data file (as a CSV)
    FILE *file = fopen(engine->datafile, "a");
    if (file == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Failed to open data file for appending: %s\n", engine->datafile);
        }
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

    // Append the new record to engine->all_records in memory
    engine->all_records = (record **)realloc(engine->all_records, (engine->num_records + 1) * sizeof(record *));
    if (engine->all_records == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Memory reallocation failed for all_records\n");
        }
        return false;
    }
    record *record_copy = (record *)malloc(sizeof(record));
    if (record_copy == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Memory allocation failed for new record\n");
        }
        return false;
    }
    *record_copy = *newRecord;  // Copy the contents of newRecord (properly allocating memory outside function scope)
    engine->all_records[engine->num_records] = record_copy;

    // Increment the record count
    engine->num_records += 1;

    // Update any relevant B+ tree indexes to include the new record
    for (int i = 0; i < engine->num_indexes; i++) {
        const char *indexed_attr = engine->indexed_attributes[i];
        
        // Insert the new record into the B+ tree for this indexed attribute
        node *root = engine->bplus_tree_roots[i];
        KEY_T key = extract_key_from_record(record_copy, indexed_attr);
        root = insert(root, key, (ROW_PTR)record_copy);
        engine->bplus_tree_roots[i] = root;
        
        if(root == NULL) {
            if (VERBOSE) {
                fprintf(stderr, "Failed to insert new record into B+ tree for attribute: %s\n", indexed_attr);
            }
            return false;
        }
    }

    return true;  // Placeholder for now
}

/* Main functionality for DELETE logic
 * Parameters:
 *   engine - constant engine object
 *   tableName - name of the table
 *   whereClause - WHERE clause (NULL for all rows)
 * Returns:
 *   number of deleted records
 */
int executeQueryDeleteSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to delete from
    struct whereClauseS *whereClause  // WHERE clause (NULL for all rows)
) {
    // TODO - IMPLEMENT LOGIC
    // Requirements:
    // Identify records matching the WHERE clause conditions
    // Remove matching records from engine->all_records and update the array accordingly
    // Update any relevant B+ tree indexes to remove references to deleted records
    return 0;  // Placeholder for now (returns number of deleted records)
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
    engine->num_indexes = num_indexes;
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
        // Update the indexed attributes and types for the engine
        engine->indexed_attributes[i] = strdup(indexed_attributes[i]);
        engine->attribute_types[i] = mapAttributeType(attribute_types[i]);
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
        free(engine->bplus_tree_roots);
        free(engine->indexed_attributes);
        free(engine->attribute_types);
        free(engine->all_records);
        free(engine);
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

/* Performs a linear search when no index is found (fallback) */
record **linearSearchRecords(struct engineS *engine, struct whereClauseS *whereClause, int *matchingRecords) {
    // Allocate space for results array
    record **results = malloc(sizeof(record *));
    *matchingRecords = 0;

    // Iterate through all records and apply WHERE clause filtering
    for(int i = 0; i < engine->num_records; i++) {
        record *currentRecord = engine->all_records[i];
        bool matches = true;

        // Evaluate each condition in the WHERE clause
        struct whereClauseS *currentCondition = whereClause;
        while (currentCondition != NULL) {
            // Create comparison function for this condition
            void *valuePtr;
            int valueType = currentCondition->value_type;
            if (valueType == 0) { // Integer
                int *intValue = malloc(sizeof(int));
                *intValue = atoi(currentCondition->value);
                valuePtr = intValue;
            } else if (valueType == 1) { // String
                valuePtr = (void *)currentCondition->value;
            } else if (valueType == 2) { // Boolean
                bool *boolValue = malloc(sizeof(bool));
                *boolValue = (strcmp(currentCondition->value, "true") == 0) ? true : false;
                valuePtr = boolValue;
            } else {
                matches = false; // Unsupported type
                break;
            }

            where_condition_func conditionFunc = create_where_condition(currentCondition->attribute, currentCondition->operator, valuePtr);
            if (conditionFunc == NULL || !conditionFunc((void *)currentRecord, valuePtr)) {
                matches = false; // Condition not met
            }

            // Free allocated memory for valuePtr if needed
            if (valueType == 0) free(valuePtr);
            if (valueType == 2) free(valuePtr);

            currentCondition = currentCondition->next; // Move to next condition
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