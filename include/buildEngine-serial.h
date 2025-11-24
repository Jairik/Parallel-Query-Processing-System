/*
 * Header file for Serial B+ Tree Construction
 *
 * Dependencies:
 *   - bplus-serial.h: Core B+ tree implementation (node, record, insert, etc.)
 *   - Standard C libraries for I/O and string processing
 */

#ifndef BUILDTREE_SERIAL_H
#define BUILDTREE_SERIAL_H

#include "bplus-serial.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Main entry point for serial B+ tree construction
 * Parses command-line arguments, loads data from the specified CSV file,
 * and constructs a B+ tree indexed by command_id.
 * 
 * Parameters:
 *   argc - argument count from command line
 *   argv - argument vector; argv[1] should contain the data file path
 * 
 * Returns:
 *   Pointer to the root node of the constructed B+ tree, or NULL on failure
 */
node *makeTreeS(int argc, char *argv[]);

/*
 * Helper function for loading CSV data into a B+ tree structure
 * Reads a CSV file line-by-line, parsing each line into a record struct
 * and inserting it into the B+ tree using command_id as the key.
 * 
 * Parameters:
 *   filename - path to the CSV data file
 * 
 * Returns:
 *   Pointer to the root node of the populated B+ tree, or NULL on error
 */
node *load_into_bplus_tree(const char *filename);

/*
 * Helper function for parsing a CSV line into a record struct
 * Tokenizes a comma-separated line and populates all fields of a record
 * struct according to the expected CSV schema:
 *   command_id, raw_command, base_command, shell_type, exit_code,
 *   timestamp, sudo_used, working_directory, user_id, user_name,
 *   host_name, risk_level
 * 
 * Parameters:
 *   line - null-terminated string containing one line of CSV data
 * 
 * Returns:
 *   Pointer to a newly allocated and populated record struct, or NULL on
 *   memory allocation failure
 */
record *getRecordFromLine(char *line);

#endif // BUILDTREE_SERIAL_H
