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

// Recursive condition node for handling nested conditions (parentheses)
typedef struct ConditionNode {
    bool is_sub_expression;     // If true, this node contains a sub-expression (sub is valid)
    Condition condition;        // Used when is_sub_expression is false
    struct ConditionNode *sub;  // Sub-expression for parenthesized conditions
    LogicOperator logic_op;     // Logical operator to connect to next node (AND/OR)
    struct ConditionNode *next; // Next condition in the chain
} ConditionNode;

typedef struct {
    CommandType command;
    char table[64];
    char columns[10][64]; // Up to 10 columns selected
    int num_columns;
    bool select_all;      // *
    
    Condition conditions[5]; // Up to 5 conditions (legacy flat storage)
    LogicOperator logic_ops[4]; // Logic between conditions (AND/OR)
    int num_conditions;
    
    ConditionNode *condition_tree; // Recursive condition tree for nested conditions

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

// ConditionNode helpers
ConditionNode *create_condition_node(void);
void free_condition_tree(ConditionNode *node);

// Dispatcher removed

#endif
