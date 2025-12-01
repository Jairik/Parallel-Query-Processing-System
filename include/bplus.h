#ifndef BPLUS_SERIAL_H
#define BPLUS_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include "logType.h"  // Structure of each table entry (record)

// Default order (fanout). Adjust to change branching factor and height.
#define ORDER 3

/* --- Configurable key + value types --- */

// Type of the key used for indexing.
typedef enum {
    KEY_INT,
    KEY_UINT64,
    KEY_BOOL,
    KEY_STRING
} KeyType;

// Generic union type to hold different key types
typedef struct {
    KeyType type;
    union {
        uint64_t u64;
        int i32;
        bool b;
        const char *str;
    } v;
} KEY_T;

typedef void *ROW_PTR;  // Pointer to the table row.


// Node structure for the B+ Tree.
typedef struct node node;
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
node *delete(node *root, KEY_T key, ROW_PTR row_ptr);
int find_rows(node *root, KEY_T key, ROW_PTR **results);
void printTree(node *const root);
void printLeaves(node *const root);
int height(node *const root);
int pathToLeaves(node *const root, node *child);
void findAndPrint(node *const root, KEY_T key);
void findAndPrintRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose);
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[]);
node *findLeaf(node *const root, KEY_T key, bool verbose);

// Key comparison function
int compare_keys(const KEY_T *key1, const KEY_T *key2);

#endif // BPLUS_SERIAL_H