/* Skeleton for the Serial Implementation - uses the bplus serial engine and tokenizer to execute a provided SQL query */

#include "include/bplus.h"
#include "include/executeEngine-serial.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VERBOSE 1

// Forward declarations B+ tree implementation
typedef struct node node;  // Pull node declaration from serial bplus
typedef struct record record;  // Pull record declaration from serial bplus

int main(int argc, char *argv[]) {

    // TODO load the command into the parser and somehow tokenize it to determine specific desired commands
    // TODO instantiate an engine object to handle the execution of the query
    // TODO call the specific command from the executeEngine-serial.h file
    

    return EXIT_SUCCESS;
}