/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#define _POSIX_C_SOURCE 200809L  // Enable strdup
#include "../../include/buildEngine-omp.h"
#include <string.h>
#include <strings.h>
#include <omp.h>
#define VERBOSE 0  // Essentially testing mode

// Forward declarations
void fillRecordFromLineOMP(char *line, record *new_record);
void parseCSVFieldToBuffer(char **cursor, char *buffer, size_t max_len);

// Forward declaration
FieldType mapAttributeTypeOMP(int attributeType);

// Creates a serial B+ tree from data file, returns the tree root
bool makeIndexOMP(struct engineS *engine, const char *indexName, int attributeType) {
    // Load all records from the engine's data source
    record **records = engine->all_records;
    int numRecords = engine->num_records;
    
    // Build the B+ tree from the records array
    node *root = loadIntoBplusTreeOMP(records, numRecords, indexName);
    if (VERBOSE && root == NULL) {
        fprintf(stderr, "Failed to load data into B+ tree\n");
    }

    // Add to the engine's known tree roots
    engine->bplus_tree_roots[engine->num_indexes] = root;
    engine->indexed_attributes[engine->num_indexes] = strdup(indexName);
    engine->num_indexes += 1;
    engine->attribute_types[engine->num_indexes-1] = mapAttributeTypeOMP(attributeType);

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
node *loadIntoBplusTreeOMP(record **records, int num_records, const char *attributeName) {
    // Instantiate the B+ tree root
    node *root = NULL;
    
    // Parallelize the iteration and insertion into the B+ tree
    // Use critical section to protect tree modifications
    //#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < num_records; i++) {
        
        // Extract the current record as a key (can be done in parallel)
        record *currentRecord = records[i];
        KEY_T key = extract_key_from_record(currentRecord, attributeName);

        // Insert the record into the B+ tree using the key
        // Critical section needed as tree structure is modified
        //#pragma omp critical
        //{
            root = insert(root, key, (ROW_PTR)currentRecord);
        //}

        // Validate insertion via printing (only in verbose mode)
        if (VERBOSE) {
            //#pragma omp critical
            {
                printTree(root);
            }
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
record **getAllRecordsFromFileOMP(const char *filepath, int *num_records, void **record_block_out) {
    // Attempt to open the file from the provided path
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (filesize == 0) {
        fclose(file);
        *num_records = 0;
        return NULL;
    }

    char *content = malloc(filesize + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, filesize, file);
    content[filesize] = '\0';
    fclose(file);

    // Count lines to pre-allocate arrays
    int line_count = 0;
    for (long i = 0; i < filesize; i++) {
        if (content[i] == '\n') line_count++;
    }
    if (filesize > 0 && content[filesize-1] != '\n') line_count++;

    if (line_count == 0) {
        free(content);
        *num_records = 0;
        return NULL;
    }

    // Create line pointers
    char **lines = malloc(line_count * sizeof(char*));
    int idx = 0;
    lines[idx++] = content;
    for (long i = 0; i < filesize; i++) {
        if (content[i] == '\n') {
            content[i] = '\0';
            if (idx < line_count && i + 1 < filesize) {
                lines[idx++] = &content[i+1];
            }
        }
    }

    // Allocate records array (max possible size)
    record **records = malloc(idx * sizeof(record*));
    
    // Allocate a single block for all records to reduce malloc overhead and fragmentation
    // We allocate idx records, though 0 is unused (header)
    record *record_block = calloc(idx, sizeof(record));
    if (!record_block) {
        free(records);
        free(lines);
        free(content);
        return NULL;
    }

    if (record_block_out) {
        *record_block_out = record_block;
    }

    // Parallel parse
    // Skip header (index 0)
    #pragma omp parallel for schedule(dynamic)
    for (int i = 1; i < idx; i++) {
        if (lines[i] && *lines[i] != '\0') {
            records[i] = &record_block[i];
            fillRecordFromLineOMP(lines[i], records[i]);
        } else {
            records[i] = NULL;
        }
    }

    // Compact the array (remove NULLs and header slot)
    int count = 0;
    for (int i = 1; i < idx; i++) {
        if (records[i] != NULL) {
            records[count++] = records[i];
        }
    }
    
    free(lines);
    // content cannot be freed because we might have pointers into it?
    // fillRecordFromLineOMP copies data into fixed size arrays in record struct.
    // So we can free content.
    free(content);

    if(VERBOSE) {
        printf("Loaded %d records from file: %s\n", count, filepath);
    }
    *num_records = count;
    return records;
}

/* Helper to parse a CSV field, handling quotes and commas */
void parseCSVFieldToBuffer(char **cursor, char *buffer, size_t max_len) {
    char *start = *cursor;
    if (*start == '\0' || *start == '\n' || *start == '\r') {
        buffer[0] = '\0';
        return;
    }

    size_t i = 0;
    bool in_quotes = false;

    if (*start == '"') {
        in_quotes = true;
        start++; // Skip opening quote
    }

    while (*start != '\0' && *start != '\n' && *start != '\r' && i < max_len - 1) {
        if (in_quotes) {
            if (*start == '"') {
                if (*(start + 1) == '"') {
                    // Escaped quote
                    buffer[i++] = '"';
                    start += 2;
                } else {
                    // End of quoted field
                    in_quotes = false;
                    start++;
                }
            } else {
                buffer[i++] = *start++;
            }
        } else {
            if (*start == ',') {
                start++; // Skip comma
                break;
            }
            buffer[i++] = *start++;
        }
    }
    
    buffer[i] = '\0';
    *cursor = start;
}

/* Fill a record struct from a line of CSV data */
void fillRecordFromLineOMP(char *line, record *new_record) {
    char *cursor = line;
    char buffer[1024];

    // command_id
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    new_record->command_id = strtoull(buffer, NULL, 10);

    // raw_command
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->raw_command, buffer, sizeof(new_record->raw_command));

    // base_command
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->base_command, buffer, sizeof(new_record->base_command));

    // shell_type
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->shell_type, buffer, sizeof(new_record->shell_type));

    // exit_code
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    new_record->exit_code = atoi(buffer);

    // timestamp
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->timestamp, buffer, sizeof(new_record->timestamp));

    // sudo_used
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    new_record->sudo_used = (strcasecmp(buffer, "true") == 0 || strcmp(buffer, "1") == 0);

    // working_directory
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->working_directory, buffer, sizeof(new_record->working_directory));

    // user_id
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    new_record->user_id = atoi(buffer);

    // user_name
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->user_name, buffer, sizeof(new_record->user_name));

    // host_name
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    strncpy(new_record->host_name, buffer, sizeof(new_record->host_name));

    // risk_level
    parseCSVFieldToBuffer(&cursor, buffer, sizeof(buffer));
    new_record->risk_level = atoi(buffer);
}

/* Get a record struct from a line of CSV data
 * Parameters:
 *   line - character array containing a line of CSV data
 * Returns:
 *   record struct populated with data from the line
*/
record *getRecordFromLineOMP(char *line){
    record *new_record = (record *)calloc(1, sizeof(record));
    if (new_record == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    fillRecordFromLineOMP(line, new_record);
    return new_record;  // Return the fully populated record
}

/* Helper to map an int representation (0, 1, 2, 3) to a FieldType object for storing */
FieldType mapAttributeTypeOMP(int attributeType) {
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
