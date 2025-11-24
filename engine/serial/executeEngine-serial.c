/* Main engine functionality and whatnot */

#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#define VERBOSE 1


// Function pointer type for WHERE condition evaluation
typedef bool (*where_condition_func)(void *record, void *value);

// Skeleton function to create a WHERE condition function pointer
where_condition_func create_where_condition(const char *attribute, const char *operator, void *value) {
    // Skeleton: This function should parse the WHERE condition and return a function pointer
    // that evaluates the condition on a record.
    // For now, return NULL as a placeholder.
    return NULL;
}

// Combines numerous WHERE clauses into one function pointer
// TODO DETERMINE HOW THIS WILL BE PASSED WITH THE DIFFERNET OPERATORS AND AND/ORS
where_condition_func combine_where_conditions(where_condition_func *conditions, int num_conditions) {
    // Skeleton: This function should combine multiple WHERE condition functions into one.
    // For now, return NULL as a placeholder.
    return NULL;
}

/* Main functionality for selecting an attribute to be indexed in the serial engine */
char *executeQuerySelectSerial(
    struct engineS *engine,  // Constant engine object
    const char *selectItems[],  // Attributes to select (SELECT clause)
    int numItems,  // Number of attributes to select (NULL for all)
    const char *tableName,  // Table to query from (FROM clause)
    compare_func_t whereFunctionPointer  // Function pointer for WHERE clause filtering
) {
    // TODO - IMPLEMENT LOGIC
    // Requirements - Traverse B+ tree indexes using the whereFunctionPointer to filter records
    // Collect selected attributes from matching records and format as a result string
    return NULL;  // Placeholder for now
}

/* Initialize the engine, returning a pointer to the engine object */
struct engineS *initializeEngineSerial(int num_indexes){
    struct engineS *engine = (struct engineS *)malloc(sizeof(struct engineS));
    if (engine == NULL) {
        perror("Failed to allocate memory for engine");
        exit(EXIT_FAILURE);
    }

    engine->num_indexes = num_indexes;
    engine->bplus_tree_roots = (node **)malloc(num_indexes * sizeof(node *));
    engine->indexed_attributes = (char **)malloc(num_indexes * sizeof(char *));
    engine->attribute_types = (int *)malloc(num_indexes * sizeof(int));
    engine->all_records = NULL; // Initialize to NULL; can be set later as needed

    if (engine->bplus_tree_roots == NULL || engine->indexed_attributes == NULL || engine->attribute_types == NULL) {
        perror("Failed to allocate memory for engine components");
        free(engine);
        exit(EXIT_FAILURE);
    }

    return engine;
}
