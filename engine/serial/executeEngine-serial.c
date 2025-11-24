/* Main engine functionality and whatnot */

#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#define VERBOSE 1

// Function pointer type for WHERE condition evaluation
typedef bool (*where_condition_func)(void *record, void *value);

/* Skeleton function to create a WHERE condition function pointer
 * Parmeters:
 *  attribute - the attribute to filter on
 *  operator - the comparison operator (e.g., "=", "<", ">")
 *  value - the value to compare against
 * Returns:
 *  A function pointer that evaluates the condition on a record
*/
where_condition_func create_where_condition(const char *attribute, const char *operator, void *value) {
    // Skeleton: This function should parse the WHERE condition and return a function pointer
    // that evaluates the condition on a record.
    // For now, return NULL as a placeholder.
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