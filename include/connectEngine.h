#ifndef CONNECT_ENGINE_H
#define CONNECT_ENGINE_H

#include "../include/executeEngine-serial.h"
#include "../include/sql.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// Constants for the test environment
#define DATA_FILE "data-generation/commands_50k.csv"
#define TABLE_NAME "commands"
#define MAX_TOKENS 100
#define ROW_LIMIT 20  // Max rows to print in output

// Helper to trim whitespace from a string
static inline char* trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

// Helper to map Parser OperatorType to string
const char* get_operator_string(OperatorType op);

// Helper to map Parser LogicOperator to string
const char* get_logic_op_string(LogicOperator op);

// Helper to convert ParsedSQL conditions to engine's whereClauseS linked list
struct whereClauseS* convert_conditions(ParsedSQL *parsed);

// Helper to free the manually constructed where clause
void free_where_clause_list(struct whereClauseS *head);

// Main test runner for a single query string
void run_test_query(struct engineS *engine, const char *query, int max_rows);

// Optimal indexes constants
extern const char* optimalIndexes[];
extern const FieldType optimalIndexTypes[];
extern const int numOptimalIndexes;

#endif // CONNECT_ENGINE_H
