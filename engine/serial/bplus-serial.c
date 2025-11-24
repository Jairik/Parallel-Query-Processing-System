/*
 * B+ Tree Implementation
 * Inspired by: https://www.programiz.com/dsa/b-plus-tree
 *
 * This implementation provides a minimal B+ tree storing unique integer keys
 * mapped to integer values. Duplicate inserts perform an upsert (overwrite
 * existing value). The tree supports single-key lookup, range queries, and
 * insertion with automatic node splitting.
 *
 * Key properties:
 * - ORDER defines maximum children per internal node (fanout) and drives
 *   memory footprint & height. Internal nodes hold up to ORDER-1 keys; leaves
 *   hold up to ORDER-1 records.
 * - Leaves are linked via the last pointer slot (pointers[ORDER-1]) enabling
 *   efficient range scans without re-traversing from the root.
 * - Height complexity: O(log_ORDER N) for search/insert; range scans are
 *   O(log N + K) (K = number of results).
 *
 * Simplifications / omissions for brevity:
 * - No deletion or rebalancing beyond insertion splits.
 * - No memory reclamation (nodes/records are never freed). Production code
 *   should add destroy functions.
 * - Not thread-safe; external synchronization required for concurrent access.
 * - Uses a simple queue built from node->next only during printing.
 */

#include "../../include/bplus-serial.h"
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
void enqueue(node *new_node);
node *dequeue(void);
/* Search / traversal utilities */
int height(node *const root);                                                   // Returns leaf depth below root.
int pathToLeaves(node *const root, node *child);                                // Distance (#edges) from child to root.
void printLeaves(node *const root);                                             // Prints leaf keys in sorted order.
void printTree(node *const root);                                               // Level-order visualization.
void findAndPrint(node *const root, KEY_T key, bool verbose);                     // Lookup with output.
void findAndPrintRange(node *const root, KEY_T range1, KEY_T range2, bool verbose); // Range output.
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[]);    // Range core logic.
node *findLeaf(node *const root, KEY_T key, bool verbose);          // Descend to target leaf.
ROW_PTR find_row(node *root, KEY_T key);                            // Row lookup.
int cut(int length);                                              // Split helper (ceil(length/2)).

/* Allocation helpers */
node *makeNode(void);
node *makeLeaf(void);

/* Insertion helpers */
int getLeftIndex(node *parent, node *left);
node *insertIntoLeaf(node *leaf, KEY_T key, ROW_PTR row_ptr);
node *insertIntoLeafAfterSplitting(node *root, node *leaf, KEY_T key,
                                   ROW_PTR row_ptr);
node *insertIntoNode(node *root, node *parent,
                     int left_index, KEY_T key, node *right);
node *insertIntoNodeAfterSplitting(node *root, node *parent,
                                   int left_index,
                                   KEY_T key, node *right);
node *insertIntoParent(node *root, node *left, KEY_T key, node *right);
node *insertIntoNewRoot(node *left, KEY_T key, node *right);
node *startNewTree(KEY_T key, ROW_PTR row_ptr);
node *insert(node *root, KEY_T key, ROW_PTR row_ptr); // Public upsert API.

/* enqueue: Append node to BFS queue (O(q)). */
void enqueue(node *new_node)
{
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

/* dequeue: Remove head of BFS queue (O(1)). */
node *dequeue(void)
{
    node *n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}

/* printLeaves: Emits sorted keys by traversing leaf linked list. */
void printLeaves(node *const root)
{
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
            printf("%" PRIu64 " ", c->keys[i]);
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
int height(node *const root)
{
    int h = 0;
    node *c = root;
    while (!c->is_leaf)
    {
        c = c->pointers[0];
        h++;
    }
    return h;
}

/* pathToLeaves: Distance (#edges) from child up to root. */
int pathToLeaves(node *const root, node *child)
{
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
void printTree(node *const root)
{
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
            printf("%" PRIu64 " ", n->keys[i]);
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

void findAndPrint(node *const root, KEY_T key, bool verbose)
{
    ROW_PTR row = find_row(root, key);
    if (row == NULL)
        printf("Row not found under key %llu.\n", (unsigned long long)key);
    else
        printf("Found row pointer %p for key %llu.\n", row, (unsigned long long)key);
}

void findAndPrintRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose)
{
    int array_size = (int)(key_end - key_start + 1);
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
            printf("Key %llu -> row %p\n",
                   (unsigned long long)returned_keys[i],
                   returned_ptrs[i]);
        }
    }

    free(returned_keys);
    free(returned_ptrs);
}

/* findRange: Core range scan populating returned arrays; returns count. */
int findRange(node *const root, KEY_T key_start, KEY_T key_end, bool verbose,
              KEY_T returned_keys[], ROW_PTR returned_pointers[])
{
    int i, num_found;
    num_found = 0;
    node *n = findLeaf(root, key_start, verbose);
    if (n == NULL)
        return 0;
    
    // Skip keys less than key_start
    for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++)
        ;
    
    // If we exhausted this leaf, move to next
    if (i == n->num_keys) {
        n = n->pointers[order - 1];
        i = 0;
    }
    
    while (n != NULL)
    {
        for (; i < n->num_keys && n->keys[i] <= key_end; i++)
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
node *findLeaf(node *const root, KEY_T key, bool verbose)
{
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
            for (i = 0; i < c->num_keys - 1; i++)
                printf("%llu ", (unsigned long long)c->keys[i]);
            printf("%llu] ", (unsigned long long)c->keys[i]);
        }
        i = 0;
        while (i < c->num_keys && key >= c->keys[i])
            i++;
        if (verbose)
            printf("%d ->\n", i);
        c = (node *)c->pointers[i];
    }
    if (verbose)
    {
        printf("Leaf [");
        for (i = 0; i < c->num_keys - 1; i++)
            printf("%llu ", (unsigned long long)c->keys[i]);
        printf("%llu] ->\n", (unsigned long long)c->keys[i]);
    }
    return c;
}

/* find_row: returns the row pointer for a given key, or NULL. */
ROW_PTR find_row(node *root, KEY_T key)
{
    if (root == NULL)
        return NULL;

    node *leaf = findLeaf(root, key, false);
    if (leaf == NULL)
        return NULL;

    for (int i = 0; i < leaf->num_keys; i++) {
        if (leaf->keys[i] == key)
            return (ROW_PTR)leaf->pointers[i];
    }
    return NULL;
}

/* cut: Returns split index favoring left side when odd length. */
int cut(int length)
{
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}

/* makeNode: Allocates an internal node with key/pointer arrays. */
node *makeNode(void)
{
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

/* makeLeaf: Wraps makeNode marking it as a leaf. */
node *makeLeaf(void)
{
    node *leaf = makeNode();
    leaf->is_leaf = true;
    return leaf;
}

/* getLeftIndex: Finds child's index in parent->pointers. */
int getLeftIndex(node *parent, node *left)
{
    int left_index = 0;
    while (left_index <= parent->num_keys &&
           parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}

/* insertIntoLeaf: Inserts key / row_ptr into non-full leaf maintaining order. */
node *insertIntoLeaf(node *leaf, KEY_T key, ROW_PTR row_ptr)
{
    int i, insertion_point = 0;

    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
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
node *insertIntoLeafAfterSplitting(node *root, node *leaf, KEY_T key, ROW_PTR row_ptr)
{
    node *new_leaf = makeLeaf();
    KEY_T *temp_keys = malloc(order * sizeof(KEY_T));
    void **temp_pointers = malloc(order * sizeof(void *));
    if (!temp_keys || !temp_pointers) {
        perror("Temporary arrays allocation failed");
        exit(EXIT_FAILURE);
    }

    int insertion_index = 0;
    while (insertion_index < order - 1 && leaf->keys[insertion_index] < key)
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

/* insertIntoNode: Inserts separator & right child into non-full internal node. */
node *insertIntoNode(node *root, node *n,
                     int left_index, KEY_T key, node *right)
{
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

/* insertIntoNodeAfterSplitting: Splits full internal node and promotes middle key. */
node *insertIntoNodeAfterSplitting(node *root, node *old_node, int left_index,
                                   KEY_T key, node *right)
{
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
node *insertIntoParent(node *root, node *left, KEY_T key, node *right)
{
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
node *insertIntoNewRoot(node *left, KEY_T key, node *right)
{
    node *root = makeNode();
    root->keys[0] = key;
    root->pointers[0] = left;
    root->pointers[1] = right;
    root->num_keys++;
    root->parent = NULL;
    left->parent = root;
    right->parent = root;
    return root;
}

/* startNewTree: Initializes first leaf (root) with single key / row_ptr. */
node *startNewTree(KEY_T key, ROW_PTR row_ptr)
{
    node *root = makeLeaf();
    root->keys[0] = key;
    root->pointers[0] = row_ptr;
    root->pointers[order - 1] = NULL;
    root->parent = NULL;
    root->num_keys = 1;
    return root;
}

/* insert: Public operation; handles empty tree, leaf insert, or splits. */
node *insert(node *root, KEY_T key, ROW_PTR row_ptr)
{
    /* Upsert behavior: if key exists, overwrite the row pointer. */
    node *leaf = NULL;
    ROW_PTR existing = NULL;

    existing = find_row(root, key);
    if (existing != NULL) {
        // Find the leaf again and overwrite its pointer.
        leaf = findLeaf(root, key, false);
        if (leaf != NULL) {
            for (int i = 0; i < leaf->num_keys; i++) {
                if (leaf->keys[i] == key) {
                    leaf->pointers[i] = row_ptr;
                    return root;
                }
            }
        }
    }

    if (root == NULL)
        return startNewTree(key, row_ptr);

    leaf = findLeaf(root, key, false);

    if (leaf->num_keys < order - 1) {
        insertIntoLeaf(leaf, key, row_ptr);
        return root;
    }

    return insertIntoLeafAfterSplitting(root, leaf, key, row_ptr);
}