#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "sql.h"

// ---- TOKENIZER ----
int tokenize(const char *input, Token tokens[], int max_tokens) {
    int i = 0, pos = 0;

    while (input[pos] && i < max_tokens - 1) {
        while (isspace(input[pos])) pos++;

        if (!input[pos]) break;

        if (strchr(";,()", input[pos])) {
            tokens[i].type = TOKEN_SYMBOL;
            tokens[i].value[0] = input[pos];
            tokens[i].value[1] = '\0';
            pos++; i++;
            continue;
        }

        if (isalpha(input[pos])) {
            int start = pos;
            while (isalnum(input[pos]) || input[pos] == '_')
                pos++;

            int len = pos - start;
            strncpy(tokens[i].value, input + start, len);
            tokens[i].value[len] = '\0';

            for (int k = 0; k < len; k++)
                tokens[i].value[k] = toupper(tokens[i].value[k]);

            tokens[i].type = TOKEN_KEYWORD;
            i++;
            continue;
        }

        pos++;
    }

    tokens[i].type = TOKEN_EOF;
    strcpy(tokens[i].value, "");
    return i;
}


// ---- PARSER ----
ParsedSQL parse_tokens(Token tokens[]) {
    ParsedSQL sql = { CMD_NONE, "" };

    if (tokens[0].type == TOKEN_KEYWORD) {
        if (strcmp(tokens[0].value, "DESCRIBE") == 0) {
            sql.command = CMD_DESCRIBE;
            strcpy(sql.identifier, tokens[1].value);
        }
        else if (strcmp(tokens[0].value, "SELECT") == 0) {
            sql.command = CMD_SELECT;
        }
        else {
            sql.command = CMD_UNKNOWN;
        }
    }

    return sql;
}


// ---- DISPATCHER ----
void describe_table(const char *tablename) {
    printf("DESCRIBE %s\n", tablename);
}

void dispatch(ParsedSQL *sql) {
    switch (sql->command) {
        case CMD_DESCRIBE:
            describe_table(sql->identifier);
            break;
        default:
            printf("Unknown or unsupported SQL command.\n");
    }
}
