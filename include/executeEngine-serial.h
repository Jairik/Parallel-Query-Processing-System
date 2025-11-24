/* Serial Execute Engine */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logType.h"

struct engineS {
    node **bplus_tree_roots; // Array of roots for all B+ tree indexes
    int num_indexes; // Number of indexes
    char **indexed_attributes; // Names of indexed attributes
    int *attribute_types; // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
    record **all_records; // Array of all records in the table (for full table scans on non-indexed queries and for assigning row pointers)
    // Any additional fields can be added here as needed for query execution context
};

// Function pointers for non-numerical comparisons
typedef bool (*compare_func_t)(const char *, const char *);  // Comparing strings
typedef bool (*compare_func_int_t)(const bool, const bool);  // Comparing booleans

// Select function - main entry point for any queries. Returns results as a string
char *executeQuerySelectSerial(
    struct engineS *engine,  // Constant engine object
    const char *selectItems[],  // Attributes to select (SELECT clause)
    int numItems,  // Number of attributes to select (NULL for all)
    const char *tableName,  // Table to query from (FROM clause)
    compare_func_t whereFunctionPointer  // Function pointer for WHERE clause filtering
);

// Insert function - main entry point for insert queries. Returns success/failure
bool executeQueryInsertSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to insert into
    const char *values[],  // Values to insert
    int numValues  // Number of values to insert
);

// Update function - main entry point for update queries. Returns number of rows updated
int executeQueryUpdateSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to update
    const char *setItems[][2],  // Array of attribute-value pairs to set
    int numSetItems,  // Number of attribute-value pairs
    compare_func_t whereFunctionPointer  // Function pointer for WHERE clause filtering
);

// Delete function - main entry point for delete queries. Returns number of rows deleted
int executeQueryDeleteSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table to delete from
    compare_func_t whereFunctionPointer  // Function pointer for WHERE clause filtering
);  

// Initialize the engine, returning a pointer to the engine object
struct engineS *initializeEngineSerial(int num_indexes);

// Destroy the engine and free resources
void destroyEngineSerial(struct engineS *engine);

// Declare an attribute to be indexed in the serial engine
bool addAttributeIndexSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table name
    const char *attributeName,  // Name of the attribute to index
    int attributeType  // Attribute type to index (0 = integer, 1 = string, 2 = boolean)
);