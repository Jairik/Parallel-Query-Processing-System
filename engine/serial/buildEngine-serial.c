/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#include "../../include/buildEngine-serial.h"
#include "../../include/executeEngine-serial.h"
#define VERBOSE 1

// Main function to create a serial B+ tree from data file, returns the tree root
node *makeIndexSerial(engineS *engine, const char *indexName) {
    // Grab the dataset name from command line arguments (with default)
    char *datafile;
    if (argc != 2) {
        if(VERBOSE) {
            fprintf(stdout, "Usage: %s <datafile>. Defaulting to data-generation/commands_50k.csv", argv[0]);
        }
        *datafile = "data/commands_sample.csv";
    }
    else{
        *datafile = argv[1];
    }

    // Initialize the B+ tree
    node *root = load_into_bplus_tree(datafile);
    if (VERBOSE && root == NULL) {
        fprintf(stderr, "Failed to load data into B+ tree from file: %s\n", datafile);
    }

    return root;  // Return the root of the constructed B+ tree
}

/* Loads in all data from the database into a bplus tree
 * Parameters:
 *   filename - path to the data file
 * Returns:
 *  root of the B+ tree
*/
node *load_into_bplus_tree(const char *filename) {
    // Open the data file
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return NULL;
    }

    // Instantiate the B+ tree root
    node *root = NULL;
    
    // Iterate through each line in the file and insert into the B+ tree
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Get a new record from the line
        struct record *new_record = getRecordFromLine(line);
        
        // Insert the record into the B+ tree using command_id as the key
        if (root == NULL) {
            root = insertIntoLeaf(root, new_record->command_id, new_record);
        } 
        else {
            root = insert(root, new_record->command_id, new_record);
        }

        // Validate insertion via printing
        printTree(root);
    }

    return root;  // Return the root of the constructed B+ tree
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