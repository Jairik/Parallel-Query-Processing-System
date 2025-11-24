/* Serial Execute Engine */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logType.h"

/* Struct for the engine */
struct engineS {
    char *tableName; // Name of the table represented by this engine
    node **bplus_tree_roots; // Array of roots for all B+ tree indexes
    int num_indexes; // Number of indexes
    char **indexed_attributes; // Names of indexed attributes
    int *attribute_types; // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
    record **all_records; // Array of all records in the table (for full table scans on non-indexed queries and for assigning row pointers)
    int num_records; // Total number of records in the table
} engineS;

/* WHERE clause struct to hold filtering conditions */
struct whereClauseS {
    const char *attribute;  // Attribute name to filter on
    const char *operator;   // Comparison operator (=, !=, <, >, <=, >=, etc.)
    const char *value;      // Value to compare against (as string, converted as needed)
    int value_type;         // Type of value (0 = integer, 1 = string, 2 = boolean)
    struct whereClauseS *next;  // Linked list for multiple WHERE conditions
    const char *logical_op; // Logical operator connecting to next condition (AND, OR, NULL if last)
} whereClauseS;

// Function pointers for non-numerical comparisons
typedef bool (*compare_func_t)(const char *, const char *);  // Comparing strings
typedef bool (*compare_func_int_t)(const bool, const bool);  // Comparing booleans

// Select function - main entry point for SELECT queries. Returns results as a string
char *executeQuerySelectSerial(
    struct engineS *engine,        // Engine object
    const char **selectItems,      // Attributes to select (NULL for all)
    int numSelectItems,            // Number of attributes to select
    const char *tableName,         // Table to query from
    struct whereClauseS *whereClause  // WHERE clause (NULL if no filtering)
);

// Insert function - main entry point for INSERT queries. Returns success/failure
bool executeQueryInsertSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to insert into
    const char *(*insertItems)[2],       // Array of attribute-value pairs to insert
    int numInsertItems                   // Number of attribute-value pairs
);

// Update function - main entry point for UPDATE queries. Returns number of rows updated
int executeQueryUpdateSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to update
    const char *(*setItems)[2],          // Array of attribute-value pairs to set
    int numSetItems,                     // Number of attribute-value pairs
    struct whereClauseS *whereClause     // WHERE clause (NULL for all rows)
);

// Delete function - main entry point for DELETE queries. Returns number of rows deleted
int executeQueryDeleteSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to delete from
    struct whereClauseS *whereClause     // WHERE clause (NULL for all rows)
);  

// Initialize the engine, returning a pointer to the engine object
struct engineS *initializeEngineSerial(
    int num_indexes,  // Total number of indexes to start 
    const char *indexed_attributes[],  // Names of indexed attributes
    const int attribute_types[],   // Types of indexed attributes (0 = integer, 1 = string, 2 = boolean)
    const char *datafile,  // Path to the data file
    const char *tableName  // Name of the table
);

// Destroy the engine and free resources
void destroyEngineSerial(struct engineS *engine);

// Declare an attribute to be indexed in the serial engine
bool addAttributeIndexSerial(
    struct engineS *engine,  // Constant engine object
    const char *tableName,  // Table name
    const char *attributeName,  // Name of the attribute to index
    int attributeType  // Attribute type to index (0 = integer, 1 = string, 2 = boolean)
);