/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#include "include/bplus-serial.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VERBOSE 1

// Forward declarations B+ tree implementation
typedef struct node node;  // Pull node declaration from serial bplus
extern struct record record;  // Pull record declaration from serial bplus
// extern node *insert(node *root, int key, int value);
// extern void printTree(node *const root);
// extern void findAndPrint(node *const root, int key, bool verbose);


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
    

}

int main(int argc, char *argv[]) {
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
    if (load_into_bplus_tree(datafile) != 0) {
        fprintf(stderr, "Failed to load data into B+ tree from file: %s\n", datafile);
        return EXIT_FAILURE;
    }

    // TODO load the command into the parser, tokenize it, and execute it using the B+ tree

    return EXIT_SUCCESS;
}