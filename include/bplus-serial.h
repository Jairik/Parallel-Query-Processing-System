#ifndef BPLUS_SERIAL_H
#define BPLUS_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include "logType.h"  // Structure of each table entry (record)

// Default order (fanout). Adjust to change branching factor and height.
#define ORDER 3

/* --- Configurable key + value types --- */
typedef uint64_t KEY_T;   // Change this to whatever key type you want.
typedef void *ROW_PTR;    // Pointer to the table row.

// Forward declare node struct
typedef struct node node;

// Node structure for the B+ Tree.
struct node {
    void **pointers;
    KEY_T *keys;
    struct node *parent;
    bool is_leaf;
    int num_keys;
    struct node *next; // Queue linkage for printing.
};

// Function prototypes for B+ Tree operations.
node *insert(node *root, KEY_T key, ROW_PTR row_ptr);
ROW_PTR find_row(node *root, KEY_T key);
void printTree(node *const root);
void printLeaves(node *const root);
int height(node *const root);
int pathToLeaves(node *const root, node *child);
void findAndPrint(node *const root, KEY_T key, bool verbose);
void findAndPrintRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose);
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[]);
node *findLeaf(node *const root, KEY_T key, bool verbose);

#endif // BPLUS_SERIAL_H