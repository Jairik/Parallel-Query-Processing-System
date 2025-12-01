/* Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include "../include/executeEngine-serial.h"
#include "../include/sql.h"
#include "../include/printHelper.h"

// Constants for the test environment
#define DATA_FILE "../data-generation/commands_50k.csv"
#define TABLE_NAME "commands"
#define MAX_TOKENS 100

// Forward declarations B+ tree implementation
typedef struct node node;  // Pull node declaration from serial bplus
typedef struct record record;  // Pull record declaration from serial bplus

/* --- Helper functions --- */

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

    // Parse - Determine which command is ran and extract components
    ParsedSQL parsed = parse_tokens(tokens);
    // if (parsed.command != CMD_SELECT) {
    //     printf("Parser did not recognize SELECT command.\n");
    //     return;
    // }

    // Convert to Engine Arguments
    const char **selectItems = NULL;
    int numSelectItems = 0;

    if (!parsed.select_all) {
        numSelectItems = parsed.num_columns;
        selectItems = malloc(numSelectItems * sizeof(char*));
        for (int i = 0; i < numSelectItems; i++) {
            selectItems[i] = parsed.columns[i];
        }
    }
    // Handle non-SELECT commands here and return early
    // TODO - Handle other commands like INSERT, DELETE, etc.
    switch (parsed.command) {
        case CMD_INSERT: {
        
        }
        
        case CMD_DELETE: {

        }

        case CMD_SELECT: {
            // Handled below
            break;
        }

        case CMD_NONE: {

        }
    }

    // These will go in above switch statement
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
    if (selectItems) free(selectItems);  // Free selected items array
    free_where_clause_list(whereClause);  // Free where clause linked list
    printf("\n");
}

/* --- Main Functionality - intake commands from text file and process */

int main(int argc, char *argv[]) {

    // TODO load the COMMANDS into memory (from COMMAND text file)


    // TODO instantiate an engine object to handle the execution of the query
    struct engineS *engine = initializeEngineSerial(
        1,
        (const char *[]){"command_id"}, // Index on command_id for testing
        (const int[]){0}, // Type 0 = integer
        DATA_FILE,
        TABLE_NAME
    );

    // TODO run each command from the command input file
    

    return EXIT_SUCCESS;
}