/* Distributed-Memory MPI Parallel Implementation - Runs commands from the query text file using connectEngine */

#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // For strcasecmp
#include <time.h>
#include <ctype.h>
#include <mpi.h>
#include "../include/executeEngine-mpi.h"
#include "../include/printHelper.h"
#include "../include/sql.h"

// Constants
#define DATA_FILE "data-generation/commands_50k.csv"
#define TABLE_NAME "commands"
#define MAX_TOKENS 100
#define ROW_LIMIT 20
#define MAX_QUERIES 1000

// Optimal indexes constant
static const char* optimalIndexes[] = {
    "command_id",
    "user_id",
    "risk_level",
    "exit_code",
    "sudo_used"
};
static const FieldType optimalIndexTypes[] = {
    FIELD_UINT64,
    FIELD_INT,
    FIELD_INT,
    FIELD_INT,
    FIELD_BOOL
};
static const int numOptimalIndexes = 5;

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

// Helper to safely copy strings with truncation
static inline void safe_copy(char *dst, size_t n, const char *src) {
    snprintf(dst, n, "%.*s", (int)n - 1, src);
}

// Helper to map Parser OperatorType to string
static const char* get_operator_string(OperatorType op) {
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
static const char* get_logic_op_string(LogicOperator op) {
    switch (op) {
        case LOGIC_AND: return "AND";
        case LOGIC_OR: return "OR";
        default: return "AND";
    }
}

// Helper to convert ParsedSQL conditions to engine's whereClauseS linked list
static struct whereClauseS* convert_conditions(ParsedSQL *parsed) {
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
static void free_where_clause_list(struct whereClauseS *head) {
    while (head) {
        struct whereClauseS *temp = head;
        head = head->next;
        free(temp);
    }
}

// MAJOR TO-DOs: Initialize engine on Rank 0 only and broadcast to others. Currently each rank initializes its own engine which is inefficient for large datasets.
// Process all queries in one rank (must be serial), then scatter results to other ranks for printing. Currently each rank processes its own queries which may lead to unbalanced workloads.
// GATHER results from all ranks to Rank 0 for unified output. Currently each rank prints its own results which may be disorganized.

int main(int argc, char *argv[]) {
    
    // Initialize MPI Environment
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    set_rank(rank);

    // Start a timer for total runtime statistics
    double totalStart = MPI_Wtime();

    const char *data_file = DATA_FILE;
    if (argc > 1) {
        data_file = argv[1];
    }

    // Instantiate an engine object to handle the execution of the query
    struct engineS *engine = initializeEngineMPI(
        numOptimalIndexes,  // Number of indexes
        optimalIndexes,  // Indexes to build B+ trees for
        (const int *)optimalIndexTypes,  // Index types
        data_file,
        TABLE_NAME
    );

    // End timer for engine initialization
    double initTimeTaken = MPI_Wtime() - totalStart;

    // Load the COMMANDS into memory (from COMMAND text file)
    const char *query_file = "sample-queries.txt";
    FILE *fp = fopen(query_file, "r");
    if (!fp) {
        if (rank == 0) perror("Failed to open query file");
        destroyEngineMPI(engine);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    if (!buffer) {
        if (rank == 0) perror("Failed to allocate memory for query file");
        fclose(fp);
        destroyEngineMPI(engine);
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    size_t read_size = fread(buffer, 1, fsize, fp);
    if (read_size != (size_t)fsize) {
        if (rank == 0) perror("Failed to read query file");
        free(buffer);
        fclose(fp);
        destroyEngineMPI(engine);
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    buffer[fsize] = 0;
    fclose(fp);

    // End timer for loading queries
    double loadTimeTaken = MPI_Wtime() - totalStart;

    // Split queries into an array
    char *queries[MAX_QUERIES];
    int query_count = 0;
    
    char *token = strtok(buffer, ";");
    while (token && query_count < MAX_QUERIES) {
        queries[query_count++] = token;
        token = strtok(NULL, ";");
    }

    // Execute Queries - Distribute across MPI ranks
    for (int i = 0; i < query_count; i++) {
        char *query = trim(queries[i]);
        if (!*query) continue;
        
        // Tokenize
        Token tokens[MAX_TOKENS];
        int num_tokens = tokenize(query, tokens, MAX_TOKENS);
        
        ParsedSQL parsed;
        struct resultSetS *result = NULL;
        bool success = false;
        double execTime = 0;
        int rowsAffected = 0;
        bool parseFailed = false;

        if (num_tokens > 0) {
            parsed = parse_tokens(tokens);
        } else {
            parseFailed = true;
        }

        bool is_owner = (i % size == rank);
        bool is_collective = (parsed.command == CMD_INSERT || parsed.command == CMD_DELETE);
        bool should_execute = is_owner || is_collective;

        if (should_execute && num_tokens > 0) {
            
            // Prepare Select Items
            const char *selectItems[parsed.num_columns > 0 ? parsed.num_columns : 1];
            int numSelectItems = 0;
            if (!parsed.select_all) {
                numSelectItems = parsed.num_columns;
                for (int k = 0; k < numSelectItems; k++) selectItems[k] = parsed.columns[k];
            }

            double start = MPI_Wtime();

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

                    success = executeQueryInsertMPI(engine, parsed.table, &r);
                }
            } 
            else if (parsed.command == CMD_DELETE) {
                struct whereClauseS *whereClause = convert_conditions(&parsed);
                result = executeQueryDeleteMPI(engine, parsed.table, whereClause);
                if (result) rowsAffected = result->numRecords;
                free_where_clause_list(whereClause);
            } 
            else if (parsed.command == CMD_SELECT) {
                struct whereClauseS *whereClause = convert_conditions(&parsed);
                result = executeQuerySelectMPI(engine, selectItems, numSelectItems, parsed.table, whereClause);
                free_where_clause_list(whereClause);
            }
            
            execTime = MPI_Wtime() - start;
        }

        // Print results (Owner only) - No barriers for performance
        if (is_owner) {
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

        // Cleanup
        if (result) freeResultSet(result);
    }

    // Print total runtime statistics in pretty colors (Rank 0 only)
    if (rank == 0) {
        double totalTimeTaken = MPI_Wtime() - totalStart;
        printf(CYAN "======= MPI Execution Summary =======" RESET "\n");
        printf(CYAN "Engine Initialization Time: " RESET YELLOW "%.4f seconds\n" RESET, initTimeTaken);
        printf(CYAN "Query Loading Time: " RESET YELLOW "%.4f seconds\n" RESET, loadTimeTaken - initTimeTaken);
        printf(CYAN "Query Execution Time: " RESET YELLOW "%.4f seconds\n" RESET, totalTimeTaken - loadTimeTaken);
        printf(BOLD CYAN "Total Execution Time: " RESET BOLD YELLOW "%.4f seconds" RESET "\n", totalTimeTaken);
        printf(CYAN "=====================================" RESET "\n");
    }

    // printf("Rank %d: Freeing buffer...\n", rank);
    free(buffer);
    // printf("Rank %d: Destroying engine...\n", rank);
    destroyEngineMPI(engine);
    // printf("Rank %d: Finalizing MPI...\n", rank);

    MPI_Finalize();
    return EXIT_SUCCESS;
}
