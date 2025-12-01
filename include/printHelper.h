#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/executeEngine-serial.h"  // Same data structure for all engines

// Helper function to print the header table - column names
void print_header_table(FILE *output, struct resultSetS *result);

// Pretty print the full result table - headers and data rows
void print_full_table(FILE *output, struct resultSetS *result);