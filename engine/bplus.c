/*
 * B+ Tree Implementation
 * Inspired by: https://www.programiz.com/dsa/b-plus-tree
 */

#include "../include/bplus.h"
#include "../include/recordSchema.h"  // for compare_key and KEY_T
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

// Default order (fanout). Adjust to change branching factor and height.
#define ORDER 3 // TODO Optimize this (may need to be way higher)

// Function definitions
int order = ORDER;           // Runtime copy (could allow dynamic tuning).
node *queue = NULL;          // Head of BFS print queue.
bool verbose_output = false; // When true, emit pointer addresses for debugging.

/* Queue helpers (internal use for breadth-first printing) */
static void enqueue(node *new_node);
static node *dequeue(void);

/* Internal helpers */
static void print_key(KEY_T k);

/* Search / traversal utilities */
int height(node *const root);                                                   // Returns leaf depth below root.
int pathToLeaves(node *const root, node *child);                                // Distance (#edges) from child to root.
void printLeaves(node *const root);                                             // Prints leaf keys in sorted order.
void printTree(node *const root);                                               // Level-order visualization.
void findAndPrint(node *const root, KEY_T key);                   // Lookup with output.
void findAndPrintRange(node *const root, KEY_T range1, KEY_T range2, bool verbose); // Range output.
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[]);              // Range core logic.
node *findLeaf(node *const root, KEY_T key, bool verbose);                      // Descend to target leaf.
ROW_PTR find_row(node *root, KEY_T key);                                        // Row lookup.
int cut(int length);                                                            // Split helper (ceil(length/2)).

/* Allocation helpers */
static node *makeNode(void);
static node *makeLeaf(void);

/* Insertion helpers */
static int getLeftIndex(node *parent, node *left);
static node *insertIntoLeaf(node *leaf, KEY_T key, ROW_PTR row_ptr);
static node *insertIntoLeafAfterSplitting(node *root, node *leaf, KEY_T key,
                                          ROW_PTR row_ptr);
static node *insertIntoNode(node *root, node *parent,
                            int left_index, KEY_T key, node *right);
static node *insertIntoNodeAfterSplitting(node *root, node *parent,
                                          int left_index,
                                          KEY_T key, node *right);
static node *insertIntoParent(node *root, node *left, KEY_T key, node *right);
static node *insertIntoNewRoot(node *left, KEY_T key, node *right);
static node *startNewTree(KEY_T key, ROW_PTR row_ptr);

node *insert(node *root, KEY_T key, ROW_PTR row_ptr); // Public upsert API.

/* ==================== Queue helpers ==================== */

static void enqueue(node *new_node) {
    node *c;
    if (queue == NULL)
    {
        queue = new_node;
        queue->next = NULL;
    }
    else
    {
        c = queue;
        while (c->next != NULL)
        {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}

static node *dequeue(void) {
    node *n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}

/* ==================== Key printing helper ==================== */

static void print_key(KEY_T k) {
    switch (k.type)
    {
    case KEY_UINT64:
        printf("%" PRIu64, k.v.u64);
        break;
    case KEY_INT:
        printf("%d", k.v.i32);
        break;
    case KEY_BOOL:
        printf("%s", k.v.b ? "true" : "false");
        break;
    case KEY_STRING:
        printf("%s", k.v.str ? k.v.str : "(null)");
        break;
    default:
        printf("<key?>");
        break;
    }
}

/* ==================== Utility / printing ==================== */

/* printLeaves: Emits sorted keys by traversing leaf linked list. */
void printLeaves(node *const root) {
    if (root == NULL)
    {
        printf("Empty tree.\n");
        return;
    }
    int i;
    node *c = root;
    while (!c->is_leaf)
        c = c->pointers[0];
    while (true)
    {
        for (i = 0; i < c->num_keys; i++)
        {
            if (verbose_output)
                printf("%p ", c->pointers[i]);
            print_key(c->keys[i]);
            printf(" ");
        }
        if (verbose_output)
            printf("%p ", c->pointers[order - 1]);
        if (c->pointers[order - 1] != NULL)
        {
            printf(" | ");
            c = c->pointers[order - 1];
        }
        else
            break;
    }
    printf("\n");
}

/* height: Number of downward edges from root to leaves. */
int height(node *const root) {
    int h = 0;
    node *c = root;
    if (c == NULL) return 0;
    while (!c->is_leaf)
    {
        c = c->pointers[0];
        h++;
    }
    return h;
}

/* pathToLeaves: Distance (#edges) from child up to root. */
int pathToLeaves(node *const root, node *child) {
    int length = 0;
    node *c = child;
    while (c != root)
    {
        c = c->parent;
        length++;
    }
    return length;
}

/* printTree: Level-order traversal using queue (node->next as linkage). */
void printTree(node *const root) {
    node *n = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root == NULL)
    {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root);
    while (queue != NULL)
    {
        n = dequeue();
        if (n->parent != NULL && n == n->parent->pointers[0])
        {
            new_rank = pathToLeaves(root, n);
            if (new_rank != rank)
            {
                rank = new_rank;
                printf("\n");
            }
        }
        if (verbose_output)
            printf("(%p)", n);
        for (i = 0; i < n->num_keys; i++)
        {
            if (verbose_output)
                printf("%p ", n->pointers[i]);
            print_key(n->keys[i]);
            printf(" ");
        }
        if (!n->is_leaf)
            for (i = 0; i <= n->num_keys; i++)
                enqueue(n->pointers[i]);
        if (verbose_output)
        {
            if (n->is_leaf)
                printf("%p ", n->pointers[order - 1]);
            else
                printf("%p ", n->pointers[n->num_keys]);
        }
        printf("| ");
    }
    printf("\n");
}

/* ==================== Lookup wrappers ==================== */

void findAndPrint(node *const root, KEY_T key) {
    ROW_PTR *rows;
    int count = find_rows(root, key, &rows);
    if (count == 0)
    {
        printf("Row not found under key ");
        print_key(key);
        printf(".\n");
    }
    else
    {
        for (int i = 0; i < count; i++) {
            printf("Found row pointer %p for key ", rows[i]);
            print_key(key);
            printf(".\n");
        }
        free(rows);
    }
}

void findAndPrintRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose) {
    int array_size = 128; // arbitrary upper bound
    KEY_T *returned_keys = malloc(array_size * sizeof(KEY_T));
    ROW_PTR *returned_ptrs = malloc(array_size * sizeof(ROW_PTR));
    if (!returned_keys || !returned_ptrs) {
        perror("Range arrays");
        exit(EXIT_FAILURE);
    }

    int num_found = findRange(root, key_start, key_end, verbose,
                              returned_keys, returned_ptrs);
    if (!num_found) {
        printf("None found.\n");
    } else {
        for (int i = 0; i < num_found; i++) {
            printf("Key ");
            print_key(returned_keys[i]);
            printf(" -> row %p\n", returned_ptrs[i]);
        }
    }

    free(returned_keys);
    free(returned_ptrs);
}

/* findRange: Core range scan populating returned arrays; returns count. */
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[]) {
    int i, num_found = 0;
    node *n = findLeaf(root, key_start, verbose);
    if (n == NULL)
        return 0;

    /* Skip keys less than key_start in the first leaf */
    for (i = 0; i < n->num_keys &&
                compare_key(n->keys[i], key_start) < 0;
         i++)
        ;

    if (i == n->num_keys) {
        n = n->pointers[order - 1];
        i = 0;
    }

    while (n != NULL)
    {
        for (; i < n->num_keys &&
               compare_key(n->keys[i], key_end) <= 0;
             i++)
        {
            returned_keys[num_found] = n->keys[i];
            returned_pointers[num_found] = (ROW_PTR)n->pointers[i];
            num_found++;
        }
        n = n->pointers[order - 1];
        i = 0;
    }
    return num_found;
}

/* findLeaf: Descends separators to leaf potentially containing key. */
node *findLeaf(node *const root, KEY_T key, bool verbose) {
    if (root == NULL)
    {
        if (verbose)
            printf("Empty tree.\n");
        return NULL;
    }
    int i = 0;
    node *c = root;
    while (!c->is_leaf)
    {
        if (verbose)
        {
            printf("[");
            for (i = 0; i < c->num_keys; i++)
            {
                print_key(c->keys[i]);
                if (i != c->num_keys - 1) printf(" ");
            }
            printf("] ");
        }
        i = 0;
        // Modified for duplicates: use > instead of >= to go to the leftmost child
        while (i < c->num_keys &&
               compare_key(key, c->keys[i]) > 0)
            i++;
        if (verbose)
            printf("%d ->\n", i);
        c = (node *)c->pointers[i];
    }
    if (verbose)
    {
        printf("Leaf [");
        for (i = 0; i < c->num_keys; i++)
        {
            print_key(c->keys[i]);
            if (i != c->num_keys - 1) printf(" ");
        }
        printf("] ->\n");
    }
    return c;
}

/* find_rows: returns the number of rows found and populates results array. */
int find_rows(node *root, KEY_T key, ROW_PTR **results) {
    if (root == NULL) {
        *results = NULL;
        return 0;
    }

    node *leaf = findLeaf(root, key, false);
    if (leaf == NULL) {
        *results = NULL;
        return 0;
    }

    int count = 0;
    int capacity = 10;
    *results = malloc(capacity * sizeof(ROW_PTR));
    if (*results == NULL) {
        perror("Failed to allocate memory for results");
        exit(EXIT_FAILURE);
    }
    
    // Find the first occurrence in the leaf
    int i = 0;
    while (i < leaf->num_keys && compare_key(leaf->keys[i], key) < 0)
        i++;

    // Traverse keys in this leaf and subsequent leaves
    while (leaf != NULL) {
        for (; i < leaf->num_keys; i++) {
            if (compare_key(leaf->keys[i], key) == 0) {
                if (count >= capacity) {
                    capacity *= 2;
                    ROW_PTR *temp = realloc(*results, capacity * sizeof(ROW_PTR));
                    if (temp == NULL) {
                        perror("Failed to reallocate memory for results");
                        free(*results);
                        exit(EXIT_FAILURE);
                    }
                    *results = temp;
                }
                (*results)[count++] = (ROW_PTR)leaf->pointers[i];
            } else if (compare_key(leaf->keys[i], key) > 0) {
                // Since keys are sorted, we can stop if we see a greater key
                return count;
            }
        }
        // Move to next leaf
        leaf = leaf->pointers[order - 1];
        i = 0;
    }
    return count;
}

/* cut: Returns split index favoring left side when odd length. */
int cut(int length) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

/* ==================== Node allocation ==================== */

static node *makeNode(void) {
    node *new_node;
    new_node = malloc(sizeof(node));
    if (new_node == NULL)
    {
        perror("Node creation.");
        exit(EXIT_FAILURE);
    }
    new_node->keys = malloc((order - 1) * sizeof(KEY_T));
    if (new_node->keys == NULL)
    {
        perror("New node keys array.");
        exit(EXIT_FAILURE);
    }
    new_node->pointers = malloc(order * sizeof(void *));
    if (new_node->pointers == NULL)
    {
        perror("New node pointers array.");
        exit(EXIT_FAILURE);
    }
    new_node->is_leaf = false;
    new_node->num_keys = 0;
    new_node->parent = NULL;
    new_node->next = NULL;
    return new_node;
}

static node *makeLeaf(void) {
    node *leaf = makeNode();
    leaf->is_leaf = true;
    return leaf;
}

/* getLeftIndex: Finds child's index in parent->pointers. */
static int getLeftIndex(node *parent, node *left) {
    int left_index = 0;
    while (left_index <= parent->num_keys &&
           parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}

/* ==================== Leaf insertion ==================== */

/* insertIntoLeaf: Inserts key / row_ptr into non-full leaf maintaining order. */
static node *insertIntoLeaf(node *leaf, KEY_T key, ROW_PTR row_ptr) {
    int i, insertion_point = 0;

    while (insertion_point < leaf->num_keys &&
           compare_key(leaf->keys[insertion_point], key) < 0)
        insertion_point++;

    for (i = leaf->num_keys; i > insertion_point; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    leaf->keys[insertion_point] = key;
    leaf->pointers[insertion_point] = row_ptr;
    leaf->num_keys++;
    return leaf;
}

/* insertIntoLeafAfterSplitting: Splits full leaf and promotes first key of new leaf. */
static node *insertIntoLeafAfterSplitting(node *root, node *leaf, KEY_T key, ROW_PTR row_ptr) {
    node *new_leaf = makeLeaf();
    KEY_T *temp_keys = malloc(order * sizeof(KEY_T));
    void **temp_pointers = malloc(order * sizeof(void *));
    if (!temp_keys || !temp_pointers) {
        perror("Temporary arrays allocation failed");
        exit(EXIT_FAILURE);
    }

    int insertion_index = 0;
    while (insertion_index < order - 1 &&
           compare_key(leaf->keys[insertion_index], key) < 0)
        insertion_index++;

    int i, j;
    for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
        if (j == insertion_index)
            j++;
        temp_keys[j] = leaf->keys[i];
        temp_pointers[j] = leaf->pointers[i];
    }

    temp_keys[insertion_index] = key;
    temp_pointers[insertion_index] = row_ptr;

    leaf->num_keys = 0;

    int split = cut(order - 1);

    for (i = 0; i < split; i++) {
        leaf->keys[i] = temp_keys[i];
        leaf->pointers[i] = temp_pointers[i];
        leaf->num_keys++;
    }

    for (i = split, j = 0; i < order; i++, j++) {
        new_leaf->keys[j] = temp_keys[i];
        new_leaf->pointers[j] = temp_pointers[i];
        new_leaf->num_keys++;
    }

    free(temp_keys);
    free(temp_pointers);

    new_leaf->pointers[order - 1] = leaf->pointers[order - 1];
    leaf->pointers[order - 1] = new_leaf;

    for (i = leaf->num_keys; i < order - 1; i++)
        leaf->pointers[i] = NULL;
    for (i = new_leaf->num_keys; i < order - 1; i++)
        new_leaf->pointers[i] = NULL;

    new_leaf->parent = leaf->parent;
    KEY_T new_key = new_leaf->keys[0];

    return insertIntoParent(root, leaf, new_key, new_leaf);
}

/* ==================== Internal node insertion ==================== */

static node *insertIntoNode(node *root, node *n,
                            int left_index, KEY_T key, node *right) {
    int i;

    for (i = n->num_keys; i > left_index; i--)
    {
        n->pointers[i + 1] = n->pointers[i];
        n->keys[i] = n->keys[i - 1];
    }
    n->pointers[left_index + 1] = right;
    n->keys[left_index] = key;
    n->num_keys++;
    return root;
}

static node *insertIntoNodeAfterSplitting(node *root, node *old_node, int left_index,
                                          KEY_T key, node *right) {
    int i, j, split;
    KEY_T k_prime;
    node *new_node, *child;
    KEY_T *temp_keys;
    node **temp_pointers;

    temp_pointers = malloc((order + 1) * sizeof(node *));
    if (temp_pointers == NULL)
    {
        exit(EXIT_FAILURE);
    }
    temp_keys = malloc(order * sizeof(KEY_T));
    if (temp_keys == NULL)
    {
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++)
    {
        if (j == left_index + 1)
            j++;
        temp_pointers[j] = old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++)
    {
        if (j == left_index)
            j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    split = cut(order);
    new_node = makeNode();
    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++)
    {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < order; i++, j++)
    {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pointers[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++)
    {
        child = new_node->pointers[i];
        child->parent = new_node;
    }

    return insertIntoParent(root, old_node, k_prime, new_node);
}

/* insertIntoParent: Chooses between simple insert, split, or new root creation. */
static node *insertIntoParent(node *root, node *left, KEY_T key, node *right) {
    int left_index;
    node *parent;

    parent = left->parent;

    if (parent == NULL)
        return insertIntoNewRoot(left, key, right);

    left_index = getLeftIndex(parent, left);

    if (parent->num_keys < order - 1)
        return insertIntoNode(root, parent, left_index, key, right);

    return insertIntoNodeAfterSplitting(root, parent, left_index, key, right);
}

/* insertIntoNewRoot: Builds new root after old root splits. */
static node *insertIntoNewRoot(node *left, KEY_T key, node *right) {
    node *root = makeNode();
    root->keys[0] = key;
    root->pointers[0] = left;
    root->pointers[1] = right;
    root->num_keys = 1;
    root->parent = NULL;
    left->parent = root;
    right->parent = root;
    return root;
}

/* startNewTree: Initializes first leaf (root) with single key / row_ptr. */
static node *startNewTree(KEY_T key, ROW_PTR row_ptr) {
    node *root = makeLeaf();
    root->keys[0] = key;
    root->pointers[0] = row_ptr;
    root->pointers[order - 1] = NULL;
    root->parent = NULL;
    root->num_keys = 1;
    return root;
}

/* ==================== Public insert (upsert) ==================== */

node *insert(node *root, KEY_T key, ROW_PTR row_ptr) {
    if (root == NULL)
        return startNewTree(key, row_ptr);

    node *leaf = findLeaf(root, key, false);

    /* Duplicates allowed: removed upsert check */

    /* Key not present or duplicate: insert new entry */
    if (leaf->num_keys < order - 1) {
        insertIntoLeaf(leaf, key, row_ptr);
        return root;
    }

    return insertIntoLeafAfterSplitting(root, leaf, key, row_ptr);
}

/* ==================== Deletion ==================== */

static node *adjustRoot(node *root);
static node *coalesceNodes(node *root, node *n, node *neighbor, int neighbor_index, KEY_T k_prime);
static node *redistributeNodes(node *root, node *n, node *neighbor, int neighbor_index, int k_prime_index, KEY_T k_prime);
static node *deleteEntry(node *root, node *n, KEY_T key, void *pointer);
static node *removeEntryFromNode(node *n, KEY_T key, node *pointer);
static int getNeighborIndex(node *n);

/* removeEntryFromNode: Removes key and pointer from node. */
static node *removeEntryFromNode(node *n, KEY_T key, node *pointer) {
    int i, num_pointers;

    // Remove the key and shift other keys to the left
    i = 0;
    while (i < n->num_keys && compare_key(n->keys[i], key) != 0)
        i++;
    
    if (i == n->num_keys) {
        printf("Error: Key not found in node during deletion.\n");
        return n;
    }

    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers to the left
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (i < num_pointers && n->pointers[i] != pointer)
        i++;
    
    if (i == num_pointers) {
        printf("Error: Pointer not found in node during deletion.\n");
        return n;
    }

    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];

    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}

/* adjustRoot: Handles case where root has become empty. */
static node *adjustRoot(node *root) {
    node *new_root;

    /* Case: nonempty root. */
    if (root->num_keys > 0)
        return root;

    /* Case: empty root. */

    // If it has a child, promote the first (only) child as the new root.
    if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }
    // If it is a leaf (has no children), then the whole tree is empty.
    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}

/* coalesceNodes: Merges a node that has become too small with a neighbor. */
static node *coalesceNodes(node *root, node *n, node *neighbor, int neighbor_index, KEY_T k_prime) {
    int i, j, neighbor_insertion_index, n_end;
    node *tmp;

    /* Swap neighbor with node if node is on the extreme left and neighbor is to its right. */
    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying keys and pointers from n.
     * Recall that n and neighbor have swapped places in the special case of n being a leftmost child.
     */
    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  non-leaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */
    if (!n->is_leaf) {

        /* Append k_prime. */
        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;

        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always one more than the number of keys. */
        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent. */
        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

    /* Case:  leaf node.
     * Append all pointers and keys from the neighbor.
     */
    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = deleteEntry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n);
    return root;
}

/* redistributeNodes: Redistributes entries between two nodes when one has become too small. */
static node *redistributeNodes(node *root, node *n, node *neighbor, int neighbor_index,
                               int k_prime_index, KEY_T k_prime) {
    int i;
    node *tmp;

    /* Case: n has a neighbor to the left.
     * Pull the neighbor's last key-pointer pair over from the neighbor's right end to n's left end.
     */
    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        } else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair to n's rightmost position.
     */
    else {
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        } else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}

/* getNeighborIndex: Gets the index of a node's nearest neighbor (left or right) in the parent. */
static int getNeighborIndex(node *n) {
    int i;

    /* Return the index of the key to the left of the pointer in the parent pointers array. */
    /* If n is the leftmost child, this means return -1. */
    for (i = 0; i <= n->parent->num_keys; i++)
        if (n->parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %p\n", (void *)n);
    exit(EXIT_FAILURE);
}

/* deleteEntry: Deletes a key and its associated pointer from a node. */
static node *deleteEntry(node *root, node *n, KEY_T key, void *pointer) {
    int min_keys;
    node *neighbor;
    int neighbor_index;
    int k_prime_index;
    KEY_T k_prime;
    int capacity;

    // Remove key and pointer from node.
    n = removeEntryFromNode(n, key, pointer);

    /* Case:  deletion from the root. */
    if (n == root)
        return adjustRoot(root);

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */
    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */
    if (n->num_keys >= min_keys)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */
    neighbor_index = getNeighborIndex(n);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? n->parent->pointers[1] : n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */
    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesceNodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */
    else
        return redistributeNodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}

/* delete: Master deletion function. */
node *delete(node *root, KEY_T key, ROW_PTR row_ptr) {
    node *key_leaf;

    // Find the leaf containing this specific pointer
    key_leaf = findLeaf(root, key, false);
    
    // Since findLeaf returns the first leaf >= key, we might need to traverse right
    // if the key spans multiple leaves.
    while (key_leaf != NULL) {
        bool found_in_node = false;
        for (int i = 0; i < key_leaf->num_keys; i++) {
            if (compare_key(key_leaf->keys[i], key) == 0 && key_leaf->pointers[i] == row_ptr) {
                found_in_node = true;
                break;
            }
        }
        
        if (found_in_node) {
            return deleteEntry(root, key_leaf, key, row_ptr);
        }

        // If we went past the key, stop
        if (key_leaf->num_keys > 0 && compare_key(key_leaf->keys[0], key) > 0)
            break;
            
        key_leaf = key_leaf->pointers[order - 1];
    }
    
    return root;
}
