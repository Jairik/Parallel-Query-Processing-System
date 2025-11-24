/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#define VERBOSE 1  // Essentially testing mode

// Main function to create a serial B+ tree from data file, returns the tree root
node *makeIndexSerial(struct engineS *engine, const char *indexName) {
    // Load all records from the engine's data source
    record **records = engine->all_records;
    int num_records = 0;
    
    // Count the records (assuming NULL-terminated or we need a counter in engine)
    // For now, we'll need to count them
    while (records != NULL && records[num_records] != NULL) {
        num_records++;
    }
    
    // Build the B+ tree from the records array
    node *root = load_into_bplus_tree(records, num_records);
    if (VERBOSE && root == NULL) {
        fprintf(stderr, "Failed to load data into B+ tree\n");
    }

    return root;  // Return the root of the constructed B+ tree
}

/* Loads in all data from the array of records into a B+ tree
 * Parameters:
 *   records - array of record pointers
 *   num_records - number of records in the array
 * Returns:
 *  root of the B+ tree
*/
node *loadIntoBplusTree(record **records, int num_records) {
    // Instantiate the B+ tree root
    node *root = NULL;
    
    // Iterate through each record and insert into the B+ tree
    for (int i = 0; i < num_records; i++) {
        record *current_record = records[i];
        
        // Insert the record into the B+ tree using command_id as the key
        if (root == NULL) {
            root = startNewTree(current_record->command_id, current_record);
        } 
        else {
            root = insert(root, current_record->command_id, 0);
            // Note: insert creates a new record internally, so we need to replace it
            // with our actual record pointer
            record *found = find(root, current_record->command_id, false, NULL);
            if (found != NULL) {
                // Copy the actual record data over
                memcpy(found, current_record, sizeof(record));
            }
        }

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
record **getAllRecordsFromFile(const char *filepath, int *num_records) {
    // Attempt to open the file from the provided path
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filepath);
        return NULL;
    }

    // Initialize the records and count
    record **records = NULL;
    char line[1024];
    int count = 0;

    // Read each line and populate the records array
    while (fgets(line, sizeof(line), file)) {
        record *new_record = getRecordFromLine(line);
        records = realloc(records, (count + 1) * sizeof(record *));
        if (records == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            fclose(file);
            return NULL;
        }
        records[count] = new_record;
        count++;
    }

    // Close the file, set the output count, and return the records array
    fclose(file);
    if(VERBOSE) {
        printf("Loaded %d records from file: %s\n", count, filepath);
    }
    *num_records = count;
    return records;  // Return the array of all records
}

/* Get a record struct from a line of CSV data
 * Parameters:
 *   line - character array containing a line of CSV data
 * Returns:
 *   record struct populated with data from the line
*/
record *getRecordFromLine(char *line){
    record *new_record = (record *)malloc(sizeof(record));
    if (new_record == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Tokenize the line and populate the record fields
    char *token = strtok(line, ",");  // command_id
    if (token != NULL) new_record->command_id = strtoull(token, NULL, 10);

    token = strtok(NULL, ",");  // raw_command
    if (token != NULL) strncpy(new_record->raw_command, token, sizeof(new_record->raw_command));

    token = strtok(NULL, ",");  // base_command
    if (token != NULL) strncpy(new_record->base_command, token, sizeof(new_record->base_command));

    token = strtok(NULL, ",");  // shell_type
    if (token != NULL) strncpy(new_record->shell_type, token, sizeof(new_record->shell_type));

    token = strtok(NULL, ",");  // exit_code
    if (token != NULL) new_record->exit_code = atoi(token);

    token = strtok(NULL, ",");  // timestamp
    if (token != NULL) strncpy(new_record->timestamp, token, sizeof(new_record->timestamp));

    token = strtok(NULL, ",");  // sudo_used
    if (token != NULL) new_record->sudo_used = atoi(token);

    token = strtok(NULL, ",");  // working_directory
    if (token != NULL) strncpy(new_record->working_directory, token, sizeof(new_record->working_directory));

    token = strtok(NULL, ",");  // user_id
    if (token != NULL) new_record->user_id = atoi(token);

    token = strtok(NULL, ",");  // user_name
    if (token != NULL) strncpy(new_record->user_name, token, sizeof(new_record->user_name));

    token = strtok(NULL, ",");  // host_name
    if (token != NULL) strncpy(new_record->host_name, token, sizeof(new_record->host_name));

    token = strtok(NULL, ",");  // risk_level
    if (token != NULL) new_record->risk_level = atoi(token);

    return new_record;  // Return the fully populated record
}