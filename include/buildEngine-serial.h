/* Header file for Serial B+ Tree Construction */

#ifndef BUILDTREE_SERIAL_H
#define BUILDTREE_SERIAL_H

#include "bplus-serial.h"
#include "executeEngine-serial.h"
#include "logType.h"  // record struct
#include "recordSchema.h"  // record schema helpers
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * makeIndexSerial: Creates a B+ tree index from engine records
 * 
 * Builds a B+ tree from the records stored in the engine structure,
 * using command_id as the indexing key. The index is associated with
 * the specified index name.
 * 
 * Parameters:
 *   engine - pointer to the engine structure containing all_records
 *   indexName - name identifier for this index
 *   attributeType - type of the attribute to index (0 = Uinteger, 1= int, 2 = string, 3 = boolean)
 * Returns:
 *   Boolean of success (true) or failure (false) of index creation
 */
bool makeIndexSerial(struct engineS *engine, const char *indexName, int attributeType);

/*
 * loadIntoBplusTree: Loads an array of records into a B+ tree
 * 
 * Iterates through the provided array of record pointers and inserts each
 * record into a B+ tree using command_id as the key. The tree is built
 * incrementally with automatic splitting when nodes become full.
 * 
 * Parameters:
 *   engine - pointer to the engine structure (for attribute info)
 *   records - array of record pointers to insert
 *   num_records - number of records in the array
 * 
 * Returns:
 *   Pointer to the root node of the populated B+ tree, or NULL on error
 */
node *loadIntoBplusTree(record **records, int num_records, const char *attributeName);

/*
 * getAllRecordsFromFile: Loads CSV file into memory as record array
 * 
 * Reads a CSV file line-by-line, parsing each line into a record struct
 * and building a dynamically allocated array of all records. The array
 * is resized as needed during reading.
 * 
 * Parameters:
 *   filepath - path to the CSV data file
 *   num_records - output parameter set to the count of loaded records
 * 
 * Returns:
 *   Pointer to dynamically allocated array of record pointers, or NULL on error.
 *   Caller is responsible for freeing the array and individual records.
 */
record **getAllRecordsFromFile(const char *filepath, int *num_records);

/*
 * getRecordFromLine: Parses a CSV line into a record struct
 * 
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
 *   memory allocation failure. Caller is responsible for freeing.
 */
record *getRecordFromLine(char *line);

#endif // BUILDTREE_SERIAL_H
