#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sql.h"

void print_token(Token t) {
    printf("Type: %d, Value: %s\n", t.type, t.value);
}

void test_insert() {
    printf("Testing INSERT...\n");
    const char *input = "INSERT INTO users VALUES (1, 'john', true);";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Tokens found: %d\n", count);
    for (int i = 0; i < count; i++) {
        print_token(tokens[i]);
    }

    ParsedSQL sql = parse_tokens(tokens);
    if (sql.command == CMD_INSERT) {
        printf("Command: INSERT\n");
        printf("Table: %s\n", sql.table);
        printf("Values: \n");
        for (int i = 0; i < sql.num_values; i++) {
            printf("  %s\n", sql.insert_values[i]);
        }
    } else {
        printf("FAILED: Command is not INSERT (got %d)\n", sql.command);
    }
}

void test_delete() {
    printf("\nTesting DELETE...\n");
    const char *input = "DELETE FROM users WHERE id = 1;";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);

    printf("Tokens found: %d\n", count);
    for (int i = 0; i < count; i++) {
        print_token(tokens[i]);
    }

    ParsedSQL sql = parse_tokens(tokens);
    if (sql.command == CMD_DELETE) {
        printf("Command: DELETE\n");
        printf("Table: %s\n", sql.table);
        printf("Conditions: %d\n", sql.num_conditions);
        if (sql.num_conditions > 0) {
            printf("  %s %d %s\n", sql.conditions[0].column, sql.conditions[0].op, sql.conditions[0].value);
        }
    } else {
        printf("FAILED: Command is not DELETE (got %d)\n", sql.command);
    }
}

int main() {
    test_insert();
    test_delete();
    return 0;
}
