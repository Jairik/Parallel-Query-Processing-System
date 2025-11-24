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
