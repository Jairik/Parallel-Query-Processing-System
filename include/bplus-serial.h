#ifndef BPLUS_SERIAL_H
#define BPLUS_SERIAL_H

#include <stdbool.h>

// Default order (fanout). Adjust to change branching factor and height.
#define ORDER 3

// Record structure for storing command execution details.
typedef struct record {
    unsigned long long command_id; // Unique key for the record.
    char raw_command[512]; // Full command string.
    char base_command[100]; // Base command without arguments.
    char shell_type[20]; // Type of shell (e.g., bash, zsh).
    int exit_code; // Exit code of the command.
    char timestamp[30]; // Execution timestamp.
    bool sudo_used; // Whether the command was run with sudo.
    char working_directory[200]; // Directory where the command was executed.
    int user_id; // ID of the user who executed the command.
    char user_name[50]; // Name of the user who executed the command.
    char host_name[100]; // Hostname of the machine.
    int risk_level; // Risk level associated with the command.
} record;

// Node structure for the B+ Tree.
typedef struct node {
    void **pointers;
    int *keys;
    struct node *parent;
    bool is_leaf;
    int num_keys;
    struct node *next; // Queue linkage for printing.
} node;

// Function prototypes for B+ Tree operations.
node *insert(node *root, int key, int value);
record *find(node *root, int key, bool verbose, node **leaf_out);
void printTree(node *const root);
void printLeaves(node *const root);
int height(node *const root);
int pathToLeaves(node *const root, node *child);
void findAndPrint(node *const root, int key, bool verbose);
void findAndPrintRange(node *const root, int key_start, int key_end, bool verbose);

#endif // BPLUS_SERIAL_H