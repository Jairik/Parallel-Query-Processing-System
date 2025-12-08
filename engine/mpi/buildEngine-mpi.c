/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include "../../include/buildEngine-mpi.h"
#include <string.h>
#include <strings.h>
#include <mpi.h>
#define VERBOSE 0  // Essentially testing mode

// Forward declaration
FieldType mapAttributeTypeMPI(int attributeType);

// Creates a serial B+ tree from data file, returns the tree root
bool makeIndexMPI(struct engineS *engine, const char *indexName, int attributeType) {
    // Load all records from the engine's data source
    record **records = engine->all_records;
    int numRecords = engine->num_records;
    
    // Build the B+ tree from the records array
    node *root = loadIntoBplusTreeMPI(records, numRecords, indexName);
    if (VERBOSE && root == NULL) {
        fprintf(stderr, "Failed to load data into B+ tree\n");
    }

    // Add to the engine's known tree roots
    engine->bplus_tree_roots[engine->num_indexes] = root;
    engine->indexed_attributes[engine->num_indexes] = strdup(indexName);
    engine->num_indexes += 1;
    engine->attribute_types[engine->num_indexes-1] = mapAttributeTypeMPI(attributeType);

    return (engine->bplus_tree_roots[engine->num_indexes-1]) != NULL;  // Return success status
}

/* Loads in all data from the array of records into a B+ tree
 * Parameters:
 *   records - array of record pointers
 *   num_records - number of records in the array
 *   attributeName - name of the attribute to index
 * Returns:
 *  root of the B+ tree
*/
node *loadIntoBplusTreeMPI(record **records, int num_records, const char *attributeName) {
    // Instantiate the B+ tree root
    node *root = NULL;
    
    // Iterate through each record and insert into the B+ tree
    for (int i = 0; i < num_records; i++) {
        
        // Extract the current record as a key
        record *currentRecord = records[i];
        KEY_T key = extract_key_from_record(currentRecord, attributeName);

        // Insert the record into the B+ tree using the key
        root = insert(root, key, (ROW_PTR)currentRecord);

        // Validate insertion via printing (only in verbose mode)
        if (VERBOSE) {
            printTree(root);
        }
    }

    return root;  // Return the root of the constructed B+ tree
}

/* Load the full CSV file into memory as an array of record structs 
 * Parameters:
 *   filepath - path to the CSV data file
 * Returns:
 *   Array of all record structs in the file
*/
record **getAllRecordsFromFileMPI(const char *filepath, int *num_records) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    char *file_content = NULL;
    long file_size = 0;

    // Rank 0 reads the file from disk
    if (rank == 0) {
        FILE *file = fopen(filepath, "r");
        if (file == NULL) {
            fprintf(stderr, "Error opening file: %s\n", filepath);
            file_size = -1; // Signal error
        } else {
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            file_content = (char *)malloc(file_size + 1);
            if (file_content) {
                size_t read_size = fread(file_content, 1, file_size, file);
                if (read_size != (size_t)file_size) {
                    fprintf(stderr, "Error reading file content\n");
                    free(file_content);
                    file_content = NULL;
                    file_size = -1;
                } else {
                    file_content[file_size] = '\0';
                }
            } else {
                file_size = -1; // Memory error
            }
            fclose(file);
        }
    }

    // Broadcast file size to all ranks
    MPI_Bcast(&file_size, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (file_size < 0) {
        return NULL; // Propagate error
    }

    // Allocate memory on other ranks
    if (rank != 0) {
        file_content = (char *)malloc(file_size + 1);
        if (!file_content) {
            fprintf(stderr, "Rank %d failed to allocate memory for file content\n", rank);
            // In a real app, we should signal abort here
            return NULL;
        }
    }

    // Broadcast file content to all ranks
    // Note: MPI_Bcast count is int. If file_size > INT_MAX, this needs chunking.
    // For this project, we assume file_size fits in int.
    MPI_Bcast(file_content, (int)file_size + 1, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Parse records from memory buffer
    record **records = NULL;
    int count = 0;
    
    char *cursor = file_content;
    
    // Skip header line
    char *eol = strchr(cursor, '\n');
    if (eol) {
        cursor = eol + 1;
    }

    while (*cursor) {
        // Find end of line
        eol = strchr(cursor, '\n');
        if (!eol) {
            // Handle last line without newline
            if (*cursor) eol = cursor + strlen(cursor);
            else break;
        }

        if (eol > cursor) {
            // Temporarily null-terminate the line to use existing parser
            char save = *eol;
            *eol = '\0';
            
            record *new_record = getRecordFromLineMPI(cursor);
            if (new_record) {
                records = realloc(records, (count + 1) * sizeof(record *));
                records[count] = new_record;
                count++;
            }
            
            *eol = save; // Restore (good practice)
        }
        
        if (*eol == '\0') break;
        cursor = eol + 1;
    }

    free(file_content);
    
    if(VERBOSE && rank == 0) {
        printf("Loaded %d records from file: %s\n", count, filepath);
    }
    *num_records = count;
    return records;
}

/* Helper to parse a CSV field, handling quotes and commas */
char *parseCSVField(char **cursor) {
    char *start = *cursor;
    if (*start == '\0' || *start == '\n' || *start == '\r') return NULL;

    char *field = malloc(1024); // Allocate buffer for field
    int i = 0;
    bool in_quotes = false;

    if (*start == '"') {
        in_quotes = true;
        start++; // Skip opening quote
    }

    while (*start != '\0' && *start != '\n' && *start != '\r') {
        if (in_quotes) {
            if (*start == '"') {
                if (*(start + 1) == '"') {
                    // Escaped quote
                    field[i++] = '"';
                    start += 2;
                } else {
                    // End of quoted field
                    in_quotes = false;
                    start++;
                }
            } else {
                field[i++] = *start++;
            }
        } else {
            if (*start == ',') {
                start++; // Skip comma
                break;
            }
            field[i++] = *start++;
        }
    }
    
    field[i] = '\0';
    *cursor = start;
    return field;
}

/* Get a record struct from a line of CSV data
 * Parameters:
 *   line - character array containing a line of CSV data
 * Returns:
 *   record struct populated with data from the line
*/
record *getRecordFromLineMPI(char *line){
    record *new_record = (record *)calloc(1, sizeof(record));
    if (new_record == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    char *cursor = line;
    char *token;

    // command_id
    token = parseCSVField(&cursor);
    if (token) { new_record->command_id = strtoull(token, NULL, 10); free(token); }

    // raw_command
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->raw_command, token, sizeof(new_record->raw_command)); free(token); }

    // base_command
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->base_command, token, sizeof(new_record->base_command)); free(token); }

    // shell_type
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->shell_type, token, sizeof(new_record->shell_type)); free(token); }

    // exit_code
    token = parseCSVField(&cursor);
    if (token) { new_record->exit_code = atoi(token); free(token); }

    // timestamp
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->timestamp, token, sizeof(new_record->timestamp)); free(token); }

    // sudo_used
    token = parseCSVField(&cursor);
    if (token) { 
        new_record->sudo_used = (strcasecmp(token, "true") == 0 || strcmp(token, "1") == 0); 
        free(token); 
    }

    // working_directory
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->working_directory, token, sizeof(new_record->working_directory)); free(token); }

    // user_id
    token = parseCSVField(&cursor);
    if (token) { new_record->user_id = atoi(token); free(token); }

    // user_name
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->user_name, token, sizeof(new_record->user_name)); free(token); }

    // host_name
    token = parseCSVField(&cursor);
    if (token) { strncpy(new_record->host_name, token, sizeof(new_record->host_name)); free(token); }

    // risk_level
    token = parseCSVField(&cursor);
    if (token) { new_record->risk_level = atoi(token); free(token); }

    return new_record;  // Return the fully populated record
}

/* Helper to map an int representation (0, 1, 2, 3) to a FieldType object for storing */
FieldType mapAttributeTypeMPI(int attributeType) {
    switch (attributeType) {
        case 0:
            return FIELD_UINT64;
        case 1:
            return FIELD_INT;
        case 2:
            return FIELD_STRING;
        case 3:
            return FIELD_BOOL;
        default:
            return -1; // Invalid type
    }
}
