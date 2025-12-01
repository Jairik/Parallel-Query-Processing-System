#include "../include/printHelper.h"

/* Helper function to print the header of the table (column names) 
 * Parameters:
 *   output - file pointer to print to (stdout if NULL)
 *   result - pointer to resultSet struct containing column names
 *   colWidths - array of column widths for alignment
*/
void printHeader(FILE *output, struct resultSetS *result, int *colWidths) {
    
    // Check that result and column names are valid
    if (result == NULL || result->columnNames == NULL) {
        return;
    }

    // If no output specified, default to stdout
    if (output == NULL) {
        output = stdout;
    }

    // Print the header row
    fprintf(output, "|");
    for (int i = 0; i < result->numColumns; i++) {
        fprintf(output, " %-*s |", colWidths[i], result->columnNames[i]);
    }
    fprintf(output, "\n");
}

/* Pretty print the full result table (headers and data rows)
 * Parameters:
 *   output - file pointer to print to (stdout if NULL)
 *   result - pointer to resultSet struct containing data
 *   limit  - max number of rows to print (0 or negative for all)
*/
void printTable(FILE *output, struct resultSetS *result, int limit){
    
    if (result == NULL || result->data == NULL) {
        if (output == NULL) output = stdout;
        fprintf(output, "No data found.\n");
        return; // Nothing more to print
    }

    // If no output specified, default to stdout
    if (output == NULL) {
        output = stdout;
    }

    // Determine how many rows to print
    int rows_to_print = result->numRecords;
    if (limit > 0 && limit < rows_to_print) {
        rows_to_print = limit;
    }

    // Calculate column widths for proper alignment
    int *colWidths = (int *)malloc(result->numColumns * sizeof(int));
    
    // Initialize with header widths
    for (int j = 0; j < result->numColumns; j++) {
        colWidths[j] = strlen(result->columnNames[j]);
    }
    
    // Update widths based on data (only for the rows we will print)
    for (int i = 0; i < rows_to_print; i++) {
        if (result->data[i] == NULL) continue;
        for (int j = 0; j < result->numColumns; j++) {
            if (result->data[i][j] == NULL) continue;
            int dataLen = strlen(result->data[i][j]);
            if (dataLen > colWidths[j]) {
                colWidths[j] = dataLen;
            }
        }
    }
    
    // Print top separator line
    fprintf(output, "+");
    for (int j = 0; j < result->numColumns; j++) {
        for (int k = 0; k < colWidths[j] + 2; k++) {
            fprintf(output, "-");
        }
        fprintf(output, "+");
    }
    fprintf(output, "\n");

    // Print Header
    printHeader(output, result, colWidths);
    
    // Print separator line
    fprintf(output, "+");
    for (int j = 0; j < result->numColumns; j++) {
        for (int k = 0; k < colWidths[j] + 2; k++) {
            fprintf(output, "-");
        }
        fprintf(output, "+");
    }
    fprintf(output, "\n");
    
    // Print data rows
    for (int i = 0; i < rows_to_print; i++) {
        fprintf(output, "|");
        if (result->data[i] == NULL) {
             fprintf(output, " NULL ROW |\n");
             continue;
        }
        for (int j = 0; j < result->numColumns; j++) {
            fprintf(output, " %-*s |", colWidths[j], result->data[i][j] ? result->data[i][j] : "NULL");
        }
        fprintf(output, "\n");
    }
    
    // Print bottom separator
    fprintf(output, "+");
    for (int j = 0; j < result->numColumns; j++) {
        for (int k = 0; k < colWidths[j] + 2; k++) {
            fprintf(output, "-");
        }
        fprintf(output, "+");
    }
    fprintf(output, "\n");

    // Print truncation message if needed
    if (limit > 0 && result->numRecords > limit) {
        fprintf(output, "... (%d more records) ...\n", result->numRecords - limit);
    }

    // Print runtime summary
    fprintf(output, "Total Records: %d | Query Time: %.4f seconds\n", result->numRecords, result->queryTime);
    
    free(colWidths);

    // NOTE: Caller must free to resultSet struct after use
}