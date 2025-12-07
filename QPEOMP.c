/* Shared-Memory OpenMP Parallel Implementation - Runs commands from the query text file using connectEngine */

#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <ctype.h>
#include <omp.h>
#include "../include/executeEngine-omp.h"

// Fix for include conflict: printHelper.h includes executeEngine-serial.h which conflicts with executeEngine-omp.h
// We will manually declare printTable and NOT include printHelper.h
// #include "../include/printHelper.h"
void printTable(FILE *output, struct resultSetS *result, int limit);

#include "../include/sql.h"

// Constants
#define DATA_FILE "data-generation/commands_50k.csv"
#define TABLE_NAME "commands"
#define MAX_TOKENS 100
#define ROW_LIMIT 20

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

// ANSI color codes for pretty printing
#define CYAN    "\x1b[36m"
#define YELLOW  "\x1b[33m"
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"

// Helper to trim whitespace from a string
static inline char* trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    // Also trim trailing
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
    return s;
}

// Helper to safely copy strings with truncation (local version)
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

// Helper to convert ParsedSQL conditions to engine's whereClauseS linked list
struct whereClauseS* convert_conditions(ParsedSQL *parsed) {
    if (parsed->num_conditions == 0) return NULL;

    struct whereClauseS *head = NULL;
    struct whereClauseS *current = NULL;

    for (int i = 0; i < parsed->num_conditions; i++) {
        struct whereClauseS *node = malloc(sizeof(struct whereClauseS));
        
        if (parsed->conditions[i].is_nested && parsed->conditions[i].nested_sql) {
            node->attribute = NULL;
            node->operator = NULL;
            node->value = NULL;
            node->value_type = 0;
            node->sub = convert_conditions(parsed->conditions[i].nested_sql);
        } else {
            node->attribute = parsed->conditions[i].column;
            node->operator = get_operator_string(parsed->conditions[i].op);
            node->value = parsed->conditions[i].value;
            
            // Simple type inference for the test
            if (parsed->conditions[i].is_numeric) {
                node->value_type = 0; // Integer/Number
            } else {
                node->value_type = 1; // String
            }
            node->sub = NULL;
        }

        node->next = NULL;

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

int main(int argc, char *argv[]) {
    printf("Starting main...\n"); fflush(stdout);
        
    // Pull out number of defined threads from CLI args (default to 8)
    int num_threads = 8;
    if (argc >= 2) {
        num_threads = atoi(argv[1]);
    }
    omp_set_num_threads(num_threads);

    // Start a timer for total runtime statistics
    double totalStart = omp_get_wtime();

    printf("Initializing Engine...\n"); fflush(stdout);
    // Instantiate an engine object to handle the execution of the query
    struct engineS *engine = initializeEngineOMP(
        numOptimalIndexes,  // Number of indexes
        optimalIndexes,  // Indexes to build B+ trees for
        (const int *)optimalIndexTypes,  // Index types
        DATA_FILE,
        TABLE_NAME
    );
    printf("Engine Initialized.\n"); fflush(stdout);

    // Wait for all threads to sync before proceeding
    #pragma omp barrier

    // End timer for engine initialization
    double initTimeTaken = omp_get_wtime() - totalStart;

    // Load the COMMANDS into memory (from COMMAND text file)
    const char *query_file = "sample-queries.txt";
    FILE *fp = fopen(query_file, "r");
    if (!fp) {
        perror("Failed to open query file");
        destroyEngineOMP(engine);
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    if (!buffer) {
        perror("Failed to allocate memory for query file");
        fclose(fp);
        destroyEngineOMP(engine);
        return EXIT_FAILURE;
    }
    size_t read_size = fread(buffer, 1, fsize, fp);
    if (read_size != (size_t)fsize) {
        perror("Failed to read query file");
        free(buffer);
        fclose(fp);
        destroyEngineOMP(engine);
        return EXIT_FAILURE;
    }
    buffer[fsize] = 0;
    fclose(fp);

    // End timer for loading queries
    double loadTimeTaken = omp_get_wtime() - totalStart;

    // Split queries into an array to avoid strtok race conditions
    #define MAX_QUERIES 1000
    char *queries[MAX_QUERIES];
    int query_count = 0;
    
    // Run sequentially to populate queries array
    char *token = strtok(buffer, ";");
    while (token && query_count < MAX_QUERIES) {
        queries[query_count++] = token;
        token = strtok(NULL, ";");
    }

    // Parallel Execution with Ordered Output
    #pragma omp parallel for ordered schedule(dynamic)
    for (int i = 0; i < query_count; i++) {
        char *query = trim(queries[i]);
        if (!*query) continue;
        
        // Tokenize each query
        Token tokens[MAX_TOKENS];
        int num_tokens = tokenize(query, tokens, MAX_TOKENS);
        
        // Parse tokens and instantiate benchmarking variables
        ParsedSQL parsed;
        struct resultSetS *result = NULL;
        bool success = false;
        double execTime = 0;
        int rowsAffected = 0;
        bool parseFailed = false;
        if (num_tokens > 0) {
            parsed = parse_tokens(tokens);
            
            // Prepare Select Items
            const char *selectItems[parsed.num_columns > 0 ? parsed.num_columns : 1];
            int numSelectItems = 0;
            if (!parsed.select_all) {
                numSelectItems = parsed.num_columns;
                for (int k = 0; k < numSelectItems; k++) selectItems[k] = parsed.columns[k];
            }

            double start = omp_get_wtime();  // Start timing for benchmarking

            // Execute based on command type
            if (parsed.command == CMD_INSERT) {
                if (parsed.num_values == 12) {
                    record r;
                    r.command_id = strtoull(parsed.insert_values[0], NULL, 10);
                    safe_copy(r.raw_command, sizeof(r.raw_command), parsed.insert_values[1]);
                    safe_copy(r.base_command, sizeof(r.base_command), parsed.insert_values[2]);
                    safe_copy(r.shell_type, sizeof(r.shell_type), parsed.insert_values[3]);
                    r.exit_code = atoi(parsed.insert_values[4]);
                    safe_copy(r.timestamp, sizeof(r.timestamp), parsed.insert_values[5]);
                    r.sudo_used = (strcasecmp(parsed.insert_values[6], "true") == 0 || strcmp(parsed.insert_values[6], "1") == 0);
                    safe_copy(r.working_directory, sizeof(r.working_directory), parsed.insert_values[7]);
                    r.user_id = atoi(parsed.insert_values[8]);
                    safe_copy(r.user_name, sizeof(r.user_name), parsed.insert_values[9]);
                    safe_copy(r.host_name, sizeof(r.host_name), parsed.insert_values[10]);
                    r.risk_level = atoi(parsed.insert_values[11]);

                    success = executeQueryInsertOMP(engine, parsed.table, &r);
                }
            } 
            else if (parsed.command == CMD_DELETE) {
                struct whereClauseS *whereClause = convert_conditions(&parsed);
                result = executeQueryDeleteOMP(engine, parsed.table, whereClause);
                if (result) rowsAffected = result->numRecords;
                free_where_clause_list(whereClause);
            } 
            else if (parsed.command == CMD_SELECT) {
                struct whereClauseS *whereClause = convert_conditions(&parsed);
                result = executeQuerySelectOMP(engine, selectItems, numSelectItems, parsed.table, whereClause);
                free_where_clause_list(whereClause);
            }
            
            execTime = omp_get_wtime() - start;
        } else {
            parseFailed = true;
        }

        // Print all results in order
        #pragma omp ordered
        {
            printf("Executing Query: %s\n", query);
            
            if (parseFailed) {
                printf("Tokenization failed.\n");
            } else {
                if (parsed.command == CMD_INSERT) {
                    if (parsed.num_values != 12) {
                        printf("Error: INSERT requires exactly 12 values.\n");
                    } else if (success) {
                        printf("Insert successful. Execution Time: %.4f seconds\n\n", execTime);
                    } else {
                        printf("Insert failed. Execution Time: %.4f seconds\n\n", execTime);
                    }
                } else if (parsed.command == CMD_DELETE) {
                    if (result) {
                        printf("Delete successful. Rows affected: %d. Execution Time: %.4f seconds\n\n", rowsAffected, execTime);
                    } else {
                        printf("Delete failed. Execution Time: %.4f seconds\n\n", execTime);
                    }
                } else if (parsed.command == CMD_SELECT) {
                    printTable(NULL, result, ROW_LIMIT);
                    printf("\n");
                } else if (parsed.command == CMD_NONE) {
                    printf("No command detected.\n");
                } else {
                    fprintf(stderr, "Unsupported command.\n");
                }
            }
        }

        // Cleanup (Local)
        if (result) freeResultSet(result);
    }

    free(buffer);
    destroyEngineOMP(engine);

    // Print total runtime statistics in pretty colors
    double totalTimeTaken = omp_get_wtime() - totalStart;
    printf(CYAN "======= Execution Summary =======" RESET "\n");
    printf(CYAN "Engine Initialization Time: " RESET YELLOW "%.4f seconds\n" RESET, initTimeTaken);
    printf(CYAN "Query Loading Time: " RESET YELLOW "%.4f seconds\n" RESET, loadTimeTaken - initTimeTaken);
    printf(CYAN "Query Execution Time: " RESET YELLOW "%.4f seconds\n" RESET, totalTimeTaken - loadTimeTaken);
    printf(BOLD CYAN "Total Execution Time: " RESET BOLD YELLOW "%.4f seconds" RESET "\n", totalTimeTaken);
    printf(CYAN "=================================" RESET "\n");

    return EXIT_SUCCESS;
}
