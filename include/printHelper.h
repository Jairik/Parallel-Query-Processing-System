#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/executeEngine-serial.h"  // Same data structure for all engines

// Helper function to print the header table - column names
void printHeader(FILE *output, struct resultSetS *result, int *colWidths);

// Pretty print the full result table - headers and data rows
// limit: max number of rows to print (0 or negative for all)
void printTable(FILE *output, struct resultSetS *result, int limit);