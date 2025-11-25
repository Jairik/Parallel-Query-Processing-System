/* Serial Execute Engine */

#ifndef EXECUTE_ENGINE_SERIAL_H
#define EXECUTE_ENGINE_SERIAL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logType.h"
#include "recordSchema.h"

/* Struct for the engine */
/* Holds the state of the database engine, including all data records and active indexes. */
struct engineS {
    char *tableName; // Name of the table represented by this engine
    node **bplus_tree_roots; // Array of roots for all B+ tree indexes
    int num_indexes; // Number of indexes
    char **indexed_attributes; // Names of indexed attributes
    FieldType *attribute_types; // Types of indexed attributes (from record schema)
    record **all_records; // Array of all records in the table (for full table scans on non-indexed queries and for assigning row pointers)
    int num_records; // Total number of records in the table
    char *datafile; // Path to the data file
};

/* Result set - The results of any given query */
struct resultSet {
    int numRecords;  // Number of rows found/affected
    int numColumns;  // Number of columns selected/affected (NULL or 0 if not applicable)
    char **columnNames;  // Array of column names (headers/attribute names)
    FieldType *columnTypes;  // Array of column types (corresponding to columnNames)
    char ***data;  // 2D Matrix of result data as strings: data[row][col]
    double queryTime;  // Time taken to execute the query
    bool success;  // Whether the query was successful
} resultSet;

/* Frees the memory allocated for a result set */
void freeResultSet(struct resultSetS *result);

/* WHERE clause struct to hold filtering conditions */
/* 
 * Represents a single condition in a WHERE clause (e.g., "risk_level > 2").
 * Can be chained together to form complex queries (e.g., "risk_level > 2 AND user_id = 101").
 */
struct whereClauseS {
    const char *attribute;  // Attribute name to filter on (e.g., "risk_level")
    const char *operator;   // Comparison operator (=, !=, <, >, <=, >=)
    const char *value;      // Value to compare against (as string, converted internally based on type)
    int value_type;         // Type of value (0 = integer, 1 = string, 2 = boolean)
    struct whereClauseS *next;  // Pointer to the next condition in the chain (or NULL)
    const char *logical_op; // Logical operator connecting to next condition ("AND", "OR")
};

// Function pointers for non-numerical comparisons
typedef bool (*compare_func_t)(const char *, const char *);  // Comparing strings
typedef bool (*compare_func_int_t)(const bool, const bool);  // Comparing booleans

// Select function - main entry point for SELECT queries. Returns a ResultSet
/* 
 * Executes a SELECT query.
 * - selectItems: Array of column names to retrieve.
 * - whereClause: Linked list of filtering conditions.
 * Returns a pointer to a ResultSet containing the selected columns and data.
 */
struct resultSetS *executeQuerySelectSerial(
    struct engineS *engine,        // Engine object
    const char **selectItems,      // Attributes to select (NULL for all)
    int numSelectItems,            // Number of attributes to select
    const char *tableName,         // Table to query from
    struct whereClauseS *whereClause  // WHERE clause (NULL if no filtering)
);

// Insert function - main entry point for INSERT queries. Returns success/failure
/* 
 * Executes an INSERT query.
 * - insertItems: Array of [Attribute, Value] pairs to insert.
 * Updates both the main record storage and any relevant B+ tree indexes.
 */
bool executeQueryInsertSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to insert into
    const record *r       // Record to insert
);

// Update function - main entry point for UPDATE queries. Returns number of rows updated
/* 
 * Executes an UPDATE query.
 * - setItems: Array of [Attribute, Value] pairs to update.
 * - whereClause: Filters which rows to update.
 * Returns the count of modified records.
 */
int executeQueryUpdateSerial(
    struct engineS *engine,              // Engine object
    const char *tableName,               // Table to update
    const char *(*setItems)[2],          // Array of attribute-value pairs to set
    int numSetItems,                     // Number of attribute-value pairs
    struct whereClauseS *whereClause     // WHERE clause (NULL for all rows)
);

// Delete function - main entry point for DELETE queries. Returns number of rows deleted
/* 
 * Executes a DELETE query.
 * - whereClause: Filters which rows to delete.
 * Returns the count of deleted records.
 */
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

// Helper function that returns if a given attribute is already indexed, returning the index or -1 if not found
int isAttributeIndexed(
    struct engineS *engine,  // Constant engine object
    const char *attributeName  // Name of the attribute to check
);

// Helper function that performs a linear search through all records based on the WHERE clause
record **linearSearchRecords(
    struct engineS *engine,  // Constant engine object
    struct whereClauseS *whereClause,  // WHERE clause
    int *matchingRecords  // Output parameter for number of matching records
);

#endif // EXECUTE_ENGINE_SERIAL_H