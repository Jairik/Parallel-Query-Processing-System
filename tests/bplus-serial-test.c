#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "../include/bplus.h"

// Helper to create a KEY_T from uint64
static inline KEY_T make_key_u64(uint64_t val) {
    KEY_T k = { .type = KEY_UINT64, .v.u64 = val };
    return k;
}

// Test building and looking up in a small serial B+ tree
int main() {
    // Defining the tree root
    node *root = NULL;

    // Create some dummy row pointers (just casting ints for demo)
    ROW_PTR row1 = (ROW_PTR)(intptr_t)33;
    ROW_PTR row2 = (ROW_PTR)(intptr_t)21;
    ROW_PTR row3 = (ROW_PTR)(intptr_t)31;
    ROW_PTR row4 = (ROW_PTR)(intptr_t)41;
    ROW_PTR row5 = (ROW_PTR)(intptr_t)10;

    // Inserting keys and row pointers
    root = insert(root, make_key_u64(5), row1);
    root = insert(root, make_key_u64(15), row2);
    root = insert(root, make_key_u64(25), row3);
    root = insert(root, make_key_u64(35), row4);
    root = insert(root, make_key_u64(45), row5);

    // Printing the tree structure
    printTree(root);

    // Testing lookups
    printf("\n--- Single key lookup: key 15 ---\n");
    findAndPrint(root, make_key_u64(15));
    
    printf("\n--- Range query: keys 10-30 (should find 15, 25) ---\n");
    findAndPrintRange(root, make_key_u64(10), make_key_u64(30), false);
    
    printf("\n--- Range query: keys 5-45 (should find all) ---\n");
    findAndPrintRange(root, make_key_u64(5), make_key_u64(45), false);

    return 0;
}