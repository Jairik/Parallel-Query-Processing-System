/* Main engine functionality and whatnot */

#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#define VERBOSE 1

// Function pointer type for WHERE condition evaluation
typedef bool (*where_condition_func)(void *record, void *value);

// Skeleton function to create a WHERE condition function pointer
// Helper macros for generating comparison functions
#define CMP_NUM(field, type, opname, op) \
    bool field##_##opname(void *r, void *v) { \
        return ((record *)r)->field op (*(type *)v); \
    }

#define CMP_STR(field, opname, op) \
    bool field##_##opname(void *r, void *v) { \
        return strcmp(((record *)r)->field, (char *)v) op 0; \
    }

// Generate comparison functions for supported fields
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


// Skeleton function to create a WHERE condition function pointer
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

/* Main functionality for a SELECT query */
char *executeQuerySelectSerial(
    struct engineS *engine,  // Constant engine object
    const char *selectItems[],  // Attributes to select (SELECT clause)
    int numItems,  // Number of attributes to select (NULL for all)
    const char *tableName,  // Table to query from (FROM clause)
    compare_func_t *whereFunctionPointers  // List of Function pointers for WHERE clause filtering
) {
    // TODO - IMPLEMENT LOGIC
    // Requirements:
    // Check if any indexed attributes are in the WHERE clause
    // If indexed attributes are present, use the corresponding B+ tree index to quickly locate matching records
    // If no indexed attributes are present, perform a full table scan of engine->all_records
    return NULL;  // Placeholder for now
}

/* Main functionality for INSERT logic */
bool executeQueryInsertSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to insert into
    const char *(*insertItems)[2],  // Array of attribute-value pairs to insert
    int numInsertItems  // Number of attribute-value pairs
) {
    // TODO - IMPLEMENT LOGIC
    // Requirements:
    // Create a new record struct and populate it with the provided attribute-value pairs
    // Append the new record to engine->all_records, resizing the array as needed
    // Update any relevant B+ tree indexes to include the new record
    return true;  // Placeholder for now
}

/* Main functionality for DELETE logic */
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
    return 0;  // Placeholder for now
}

/* Initialize the engine, returning a pointer to the engine object */
struct engineS *initializeEngineSerial(
    int num_indexes,  // Total number of indexes to start 
    const char *indexed_attributes[],  // Names of indexed attributes
    const int attribute_types[],   // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
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
    engine->attribute_types = (int *)malloc(num_indexes * sizeof(int));
    engine->all_records = NULL; // Initialize to NULL, will be set later
    engine->num_records = 0; // Initialize record count to 0
    if (engine->bplus_tree_roots == NULL || engine->indexed_attributes == NULL || engine->attribute_types == NULL) {
        perror("Failed to allocate memory for engine components");
        free(engine);
        exit(EXIT_FAILURE);
    }

    // Read all records from the database into memory and store in engine->all_records
    if(!datafile){ datafile = "../data/commands_50k.csv"; }; // Filepath default
    engine->all_records = getAllRecordsFromFile(datafile, &engine->num_records);  // Directly update record count

    // Copy indexed attribute names and types into engine struct (defaults)
    for (int i = 0; i < num_indexes; i++) {
        // Update the indexed attributes and types for the engine
        engine->indexed_attributes[i] = strdup(indexed_attributes[i]);
        engine->attribute_types[i] = attribute_types[i];
        // Build B+ tree index for each indexed attribute
        engine->bplus_tree_roots[i] = makeIndexSerial(engine, indexed_attributes[i]);
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
    int attributeType  // Attribute type to index (0 = integer, 1 = string, 2 = boolean)
) {
    // Create a new B+ tree index for the specified attribute
    node *new_index_root = makeIndexSerial(engine, attributeName);
    if (new_index_root == NULL) {
        if (VERBOSE) {
            fprintf(stderr, "Failed to create B+ tree index for attribute: %s\n", attributeName);
        }
        return false;  // Failed to create index
    }

    // Append the new index root to the engine's array of B+ tree roots
    engine->bplus_tree_roots[engine->num_indexes] = new_index_root;
    engine->indexed_attributes[engine->num_indexes] = strdup(attributeName);
    engine->attribute_types[engine->num_indexes] = attributeType;
    engine->num_indexes += 1;  // Increment the number of indexes

    return true;  // Successfully added the index
}