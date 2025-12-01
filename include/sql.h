#ifndef SQL_H
#define SQL_H

#include <stdbool.h>

// ---- ENUMS ----
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_SYMBOL,
    TOKEN_STRING, 
    TOKEN_NUMBER,
    TOKEN_EOF
} TokenType;

typedef enum {
    CMD_NONE,
    CMD_DESCRIBE,
    CMD_SELECT,
    CMD_INSERT,
    CMD_DELETE,
    CMD_UNKNOWN
} CommandType;

typedef enum {
    OP_NONE,
    OP_EQ,      // =
    OP_NEQ,     // !=
    OP_GT,      // >
    OP_LT,      // <
    OP_GTE,     // >=
    OP_LTE      // <=
} OperatorType;

typedef enum {
    LOGIC_NONE,
    LOGIC_AND,
    LOGIC_OR
} LogicOperator;

// ---- STRUCTS ----
typedef struct {
    TokenType type;
    char value[256]; // Increased size for long strings
} Token;

typedef struct {
    char column[64];
    OperatorType op;
    char value[256]; // Value to compare against
    bool is_numeric; // Whether the value is a number or string/bool
} Condition;

typedef struct {
    CommandType command;
    char table[64];
    char columns[10][64]; // Up to 10 columns selected
    int num_columns;
    bool select_all;      // *
    
    Condition conditions[5]; // Up to 5 conditions
    LogicOperator logic_ops[4]; // Logic between conditions (AND/OR)
    int num_conditions;

    char insert_values[10][256];
    int num_values;

    char order_by[64];
    bool order_desc;
} ParsedSQL;


// ---- FUNCTION PROTOTYPES ----

// Tokenizer
int tokenize(const char *input, Token tokens[], int max_tokens);

// Parser
ParsedSQL parse_tokens(Token tokens[]);

// Dispatcher removed

#endif
