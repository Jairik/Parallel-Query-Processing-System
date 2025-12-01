#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/executeEngine-serial.h"
#include "../include/sql.h"
#include "../include/printHelper.h"

// Constants for the test environment
#define DATA_FILE "../data-generation/commands_50k.csv"
#define TABLE_NAME "commands"
#define MAX_TOKENS 100

// Helper to map Parser OperatorType to string
const char* get_operator_string(OperatorType op) {
    switch (op) {
        case OP_EQ: return "=";
        case OP_NEQ: return "!=";
        case OP_GT: return ">";
        case OP_LT: return "<";
        case OP_GTE: return ">=";
        case OP_LTE: return "<=";
        default: return "=";
    }
}

// Helper to map Parser LogicOperator to string
const char* get_logic_op_string(LogicOperator op) {
    switch (op) {
        case LOGIC_AND: return "AND";
        case LOGIC_OR: return "OR";
        default: return "AND";
    }
}

// Helper to convert ParsedSQL conditions to engine's whereClauseS linked list
struct whereClauseS* convert_conditions(ParsedSQL *parsed) {
    if (parsed->num_conditions == 0) return NULL;

    struct whereClauseS *head = NULL;
    struct whereClauseS *current = NULL;

    for (int i = 0; i < parsed->num_conditions; i++) {
        struct whereClauseS *node = malloc(sizeof(struct whereClauseS));
        node->attribute = strdup(parsed->conditions[i].column);
        node->operator = strdup(get_operator_string(parsed->conditions[i].op));
        node->value = strdup(parsed->conditions[i].value);
        
        // Simple type inference for the test
        if (parsed->conditions[i].is_numeric) {
            node->value_type = 0; // Integer/Number
        } else {
            node->value_type = 1; // String
        }

        node->next = NULL;
        node->sub = NULL; // Not handling nested conditions in this simple test converter

        // Set logical operator for the *previous* node to connect to this one
        // The parser stores logic ops between conditions. logic_ops[0] is between cond[0] and cond[1]
        if (i < parsed->num_conditions - 1) {
            node->logical_op = strdup(get_logic_op_string(parsed->logic_ops[i]));
        } else {
            node->logical_op = NULL;
        }

        if (head == NULL) {
            head = node;
            current = node;
        } else {
            current->next = node;
            current = node;
        }
    }
    return head;
}

// Helper to free the manually constructed where clause
void free_where_clause_list(struct whereClauseS *head) {
    while (head) {
        struct whereClauseS *temp = head;
        head = head->next;
        // In a real scenario, we'd free the const char* strings if we strdup'd them
        // For this test, we rely on OS cleanup or simple leaks as it's a short-lived test
        free(temp);
    }
}

// Main test runner for a single query string
void run_test_query(struct engineS *engine, const char *query, int max_rows) {
    printf("Testing Query: %s\n", query);

    // 1. Tokenize
    Token tokens[MAX_TOKENS];
    int num_tokens = tokenize(query, tokens, MAX_TOKENS);
    if (num_tokens <= 0) {
        printf("Tokenization failed.\n");
        return;
    }

    // 2. Parse
    ParsedSQL parsed = parse_tokens(tokens);
    if (parsed.command != CMD_SELECT) {
        printf("Parser did not recognize SELECT command.\n");
        return;
    }

    // 3. Convert to Engine Arguments
    const char **selectItems = NULL;
    int numSelectItems = 0;

    if (!parsed.select_all) {
        numSelectItems = parsed.num_columns;
        selectItems = malloc(numSelectItems * sizeof(char*));
        for (int i = 0; i < numSelectItems; i++) {
            selectItems[i] = parsed.columns[i];
        }
    }

    struct whereClauseS *whereClause = convert_conditions(&parsed);

    // 4. Execute
    struct resultSetS *result = executeQuerySelectSerial(
        engine,
        selectItems,
        numSelectItems,
        parsed.table,
        whereClause
    );

    // 5. Verify and Print
    printTable(NULL, result, max_rows);

    // 6. Cleanup
    if (result) freeResultSet(result);
    if (selectItems) free(selectItems);
    // free_where_clause_list(whereClause); // Optional for test
    printf("\n");
}

int main() {
    printf("=== Starting Serial SELECT Test ===\n");

    // 1. Initialize the Engine
    // We will index 'command_id' (UINT64) and 'risk_level' (INT) for this test
    const char *indexed_attrs[] = {"command_id", "risk_level"};
    const int attr_types[] = {0, 1}; // 0=UINT64, 1=INT
    int num_indexes = 2;

    printf("Initializing Engine with data from: %s\n", DATA_FILE);
    struct engineS *engine = initializeEngineSerial(
        num_indexes,
        indexed_attrs,
        attr_types,
        DATA_FILE,
        TABLE_NAME
    );

    if (!engine) {
        fprintf(stderr, "Failed to initialize engine.\n");
        return 1;
    }
    printf("Engine initialized successfully. Loaded %d records.\n\n", engine->num_records);

    // 2. Run Test Cases

    // Case 1: Select All (Limit 3 printed to avoid console flood)
    run_test_query(engine, "SELECT * FROM commands", 3);

    // Case 2: Select Specific Columns
    run_test_query(engine, "SELECT command_id, user_name, risk_level FROM commands", 10);

    // Case 3: Simple Filter (Numeric)
    // Should use the index on risk_level if implemented correctly, or full scan
    run_test_query(engine, "SELECT * FROM commands WHERE risk_level > 2", 10);

    // Case 4: Simple Filter (String)
    run_test_query(engine, "SELECT * FROM commands WHERE shell_type = 'bash'", 10);

    // Case 5: Complex Filter (AND)
    run_test_query(engine, "SELECT command_id, raw_command FROM commands WHERE risk_level > 2 AND sudo_used = 1", 10);

    // Case 6: Filter with non-indexed attribute
    run_test_query(engine, "SELECT * FROM commands WHERE exit_code = 0", 10);

    // 3. Cleanup
    destroyEngineSerial(engine);
    printf("=== Test Completed Successfully ===\n");

    return 0;
}
