// TODO - Will be a pretty massive file that includes all the necessary functions to execute queries in serial mode

struct engineS {
    node **bplus_tree_roots; // Array of roots for all B+ tree indexes
    int num_indexes; // Number of indexes
    // Additional fields can be added here as needed for query execution context
};