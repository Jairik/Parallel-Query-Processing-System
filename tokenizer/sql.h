#ifndef SQL_H
#define SQL_H

// ---- ENUMS ----
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_SYMBOL,
    TOKEN_EOF
} TokenType;

typedef enum {
    CMD_NONE,
    CMD_DESCRIBE,
    CMD_SELECT,
    CMD_UNKNOWN
} CommandType;


// ---- STRUCTS ----
typedef struct {
    TokenType type;
    char value[64];
} Token;

typedef struct {
    CommandType command;
    char identifier[64];
} ParsedSQL;


// ---- FUNCTION PROTOTYPES ----

// Tokenizer
int tokenize(const char *input, Token tokens[], int max_tokens);

// Parser
ParsedSQL parse_tokens(Token tokens[]);

// Dispatcher
void dispatch(ParsedSQL *sql);

// Command handlers
void describe_table(const char *tablename);

#endif
