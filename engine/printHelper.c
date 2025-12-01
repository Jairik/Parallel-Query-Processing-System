#include "../../include/printHelper.h"

/* Helper function to print the header of the table (column names) 
 * Parameters:
 *   output - file pointer to print to (stdout if NULL)
 *   result - pointer to resultSet struct containing column names
*/
void print_header_table(FILE *output, struct resultSetS *result) {
    
    // Check that result and column names are valid
    if (result == NULL || result->columnNames == NULL) {
        fprintf(output, "0 Results.\n");
        return;
    }

    // If no output specified, default to stdout
    if (output == NULL) {
        output = stdout;
    }

    // Print the header information
    fprintf(output, "Header Columns (%d):\n", result->numColumns);
    for (int i = 0; i < result->numColumns; i++) {
        fprintf(output, " - %s\n", result->columnNames[i]);
    }
}

/* Pretty print the full result table (headers and data rows)
 * Parameters:
 *   output - file pointer to print to (stdout if NULL)
 *   result - pointer to resultSet struct containing data
*/
void print_full_table(FILE *output, struct resultSetS *result){
    
    // Print headers first
    print_header_table(output, result);
    if (result == NULL || result->data == NULL) {
        return; // Nothing more to print
    }

    // Calculate column widths for proper alignment
    int *colWidths = (int *)malloc(result->numColumns * sizeof(int));
    
    // Initialize with header widths
    for (int j = 0; j < result->numColumns; j++) {
        colWidths[j] = strlen(result->columnNames[j]);
    }
    
    // Update widths based on data
    for (int i = 0; i < result->numRecords; i++) {
        for (int j = 0; j < result->numColumns; j++) {
            int dataLen = strlen(result->data[i][j]);
            if (dataLen > colWidths[j]) {
                colWidths[j] = dataLen;
            }
        }
    }
    
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
    for (int i = 0; i < result->numRecords; i++) {
        fprintf(output, "|");
        for (int j = 0; j < result->numColumns; j++) {
            fprintf(output, " %-*s |", colWidths[j], result->data[i][j]);
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

    // Print runtime summary
    fprintf(output, "Total Records: %d | Query Time: %.4f seconds\n", result->numRecords, result->queryTime);
    
    free(colWidths);

    // NOTE: Caller must free to resultSet struct after use
}