/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#include "include/bplus-serial.h"
#include "include/makeTree-serial.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VERBOSE 1

// Forward declarations B+ tree implementation
typedef struct node node;  // Pull node declaration from serial bplus
typedef struct record record;  // Pull record declaration from serial bplus

int main(int argc, char *argv[]) {
    // Create the B+ tree from the provided data file
    node *root = makeTreeS(argc, argv);
    if (root == NULL) {
        if (VERBOSE){fprintf(stderr, "Failed to load data into B+ tree from file: %s\n", argv[1]);}
        return EXIT_FAILURE;
    }

    // TODO load the command into the parser, tokenize it, and execute it using the B+ tree
    

    return EXIT_SUCCESS;
}