/* Shared-Memory OpenMP Parallel Implementation - Runs commands from the query text file using connectEngine */

#define _POSIX_C_SOURCE 200809L // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "../include/executeEngine-serial.h"
#include "../include/connectEngine.h"
// ANSI color codes for pretty printing
#define CYAN    "\x1b[36m"
#define YELLOW  "\x1b[33m"
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"

int main(int argc, char *argv[]) {
    
    // NOTE: defaulting to sample queries file for ease of testing. Can implement CLI arg later.
    (void)argc; (void)argv; // Unused for now. Prevent warnings.

    // Start a timer for total runtime statistics
    clock_t totalStart = clock();

    // Instantiate an engine object to handle the execution of the query
    struct engineS *engine = initializeEngineSerial(
        numOptimalIndexes,  // Number of indexes
        optimalIndexes,  // Indexes to build B+ trees for
        (const int *)optimalIndexTypes,  // Index types
        DATA_FILE,
        TABLE_NAME
    );

    // End timer for engine initialization
    double initTimeTaken = ((double)clock() - totalStart) / CLOCKS_PER_SEC;

    // Load the COMMANDS into memory (from COMMAND text file)
    const char *query_file = "sample-queries.txt";
    FILE *fp = fopen(query_file, "r");
    if (!fp) {
        perror("Failed to open query file");
        destroyEngineSerial(engine);
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    if (!buffer) {
        perror("Failed to allocate memory for query file");
        fclose(fp);
        destroyEngineSerial(engine);
        return EXIT_FAILURE;
    }
    size_t read_size = fread(buffer, 1, fsize, fp);
    if (read_size != (size_t)fsize) {
        perror("Failed to read query file");
        free(buffer);
        fclose(fp);
        destroyEngineSerial(engine);
        return EXIT_FAILURE;
    }
    buffer[fsize] = 0;
    fclose(fp);

    // End timer for loading queries
    double loadTimeTaken = ((double)clock() - totalStart) / CLOCKS_PER_SEC;

    // Run each command from the command input file
    char *query = strtok(buffer, ";");
    while (query) {
        // Trim whitespace
        query = trim(query);
        if (*query) {
            run_test_query(engine, query, ROW_LIMIT); // Limit output to 10 rows for testing
        }
        query = strtok(NULL, ";");
    }

    free(buffer);
    destroyEngineSerial(engine);

    // Print total runtime statistics in pretty colors
    double totalTimeTaken = ((double)clock() - totalStart) / CLOCKS_PER_SEC;
    printf(CYAN "======= Execution Summary =======" RESET "\n");
    printf(CYAN "Engine Initialization Time: " RESET YELLOW "%.4f seconds\n" RESET, initTimeTaken);
    printf(CYAN "Query Loading Time: " RESET YELLOW "%.4f seconds\n" RESET, loadTimeTaken - initTimeTaken);
    printf(CYAN "Query Execution Time: " RESET YELLOW "%.4f seconds\n" RESET, totalTimeTaken - loadTimeTaken);
    printf(BOLD CYAN "Total Execution Time: " RESET BOLD YELLOW "%.4f seconds" RESET "\n", totalTimeTaken);
    printf(CYAN "=================================" RESET "\n");

    return EXIT_SUCCESS;
}