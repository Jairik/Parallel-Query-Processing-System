/* Helper functions for QPE files. Used for testing with query text files. Cannot be reliably parralelized */

#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>
#include "../include/executeEngine-serial.h"
#include "../include/sql.h"
#include "../include/printHelper.h"
#include "../include/connectEngine.h"
#include <time.h>

// Forward declarations B+ tree implementation
typedef struct node node;  // Pull node declaration from serial bplus
typedef struct record record;  // Pull record declaration from serial bplus

// Helper to safely copy strings with truncation
static inline void safe_copy(char *dst, size_t n, const char *src) {
    snprintf(dst, n, "%.*s", (int)n - 1, src);
}

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

// Optimal indexes constant
const char* optimalIndexes[] = {
    "command_id",
    "user_id",
    "risk_level",
    "exit_code",
    "sudo_used"
};
const FieldType optimalIndexTypes[] = {
    FIELD_UINT64,
    FIELD_INT,
    FIELD_INT,
    FIELD_INT,
    FIELD_BOOL
};
const int numOptimalIndexes = 5;

// Helper to convert ParsedSQL conditions to engine's whereClauseS linked list
struct whereClauseS* convert_conditions(ParsedSQL *parsed) {
    if (parsed->num_conditions == 0) return NULL;

    struct whereClauseS *head = NULL;
    struct whereClauseS *current = NULL;

    for (int i = 0; i < parsed->num_conditions; i++) {
        struct whereClauseS *node = malloc(sizeof(struct whereClauseS));
        node->attribute = parsed->conditions[i].column;
        node->operator = get_operator_string(parsed->conditions[i].op);
        node->value = parsed->conditions[i].value;
        
        // Simple type inference for the test
        if (parsed->conditions[i].is_numeric) {
            node->value_type = 0; // Integer/Number
        } else {
            node->value_type = 1; // String
        }

        // TODO - Handle sub-expressions and nested conditions
        // Note: Nested conditions are not currently supported by the test
        node->next = NULL;
        node->sub = NULL;

        // Set logical operator for the *previous* node to connect to this one
        // The parser stores logic ops between conditions. logic_ops[0] is between cond[0] and cond[1]
        if (i < parsed->num_conditions - 1) {
            node->logical_op = get_logic_op_string(parsed->logic_ops[i]);
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
        free(temp);
    }
}

// Main test runner for a single query string
void run_test_query(struct engineS *engine, const char *query, int max_rows) {

    // Print query for testing
    printf("Executing Query: %s\n", query);

    // Tokenize
    Token tokens[MAX_TOKENS];
    int num_tokens = tokenize(query, tokens, MAX_TOKENS);
    if (num_tokens <= 0) {
        printf("Tokenization failed.\n");
        return;
    }

    // Parse - Determine which command is ran and extract components
    ParsedSQL parsed = parse_tokens(tokens);

    // Convert to Engine Arguments
    const char *selectItems[parsed.num_columns > 0 ? parsed.num_columns : 1];
    int numSelectItems = 0;

    if (!parsed.select_all) {
        numSelectItems = parsed.num_columns;
        for (int i = 0; i < numSelectItems; i++) {
            selectItems[i] = parsed.columns[i];
        }
    }

    // Handle non-SELECT commands here and return early
    switch (parsed.command) {
        case CMD_INSERT: {
            if (parsed.num_values != 12) {
                printf("Error: INSERT requires exactly 12 values.\n");
                return;
            }
            record r;
            r.command_id = strtoull(parsed.insert_values[0], NULL, 10);
            safe_copy(r.raw_command, sizeof(r.raw_command), parsed.insert_values[1]);
            safe_copy(r.base_command, sizeof(r.base_command), parsed.insert_values[2]);
            safe_copy(r.shell_type, sizeof(r.shell_type), parsed.insert_values[3]);
            r.exit_code = atoi(parsed.insert_values[4]);
            safe_copy(r.timestamp, sizeof(r.timestamp), parsed.insert_values[5]);
            
            // Handle boolean sudo_used
            r.sudo_used = (strcasecmp(parsed.insert_values[6], "true") == 0 || strcmp(parsed.insert_values[6], "1") == 0);

            safe_copy(r.working_directory, sizeof(r.working_directory), parsed.insert_values[7]);
            r.user_id = atoi(parsed.insert_values[8]);
            safe_copy(r.user_name, sizeof(r.user_name), parsed.insert_values[9]);
            safe_copy(r.host_name, sizeof(r.host_name), parsed.insert_values[10]);
            r.risk_level = atoi(parsed.insert_values[11]);

            if (executeQueryInsertSerial(engine, parsed.table, &r)) {
                printf("Insert successful.\n\n");
            } else {
                printf("Insert failed.\n\n");
            }
            return;
        }

        case CMD_DELETE: {
            struct whereClauseS *whereClause = convert_conditions(&parsed);
            struct resultSetS *result = executeQueryDeleteSerial(engine, parsed.table, whereClause);
            if (result) {
                printf("Delete successful. Rows affected: %d\n", result->numRecords);
                freeResultSet(result);
            } else {
                printf("Delete failed.\n");
            }
            free_where_clause_list(whereClause);
            return;
        }

        case CMD_SELECT: {
            // Handled below
            break;
        }

        case CMD_NONE: {
            printf("No command detected.\n");
            return;
        }

        default: {
            printf("Unsupported command.\n");
            return;
        }
    }

    // Retreive the WHERE clause
    struct whereClauseS *whereClause = convert_conditions(&parsed);

    // Execute
    struct resultSetS *result = executeQuerySelectSerial(
        engine,
        selectItems,
        numSelectItems,
        parsed.table,
        whereClause
    );

    // Verify and Print
    printTable(NULL, result, max_rows);

    // Cleanup
    if (result) freeResultSet(result);  // Free the results object
    free_where_clause_list(whereClause);  // Free where clause linked list
    printf("\n");
}