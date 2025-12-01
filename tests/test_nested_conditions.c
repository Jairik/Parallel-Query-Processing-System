/* Test for nested conditions and sub-expressions in WHERE clauses */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/sql.h"

void print_token(Token t) {
    printf("  Type: %d, Value: %s\n", t.type, t.value);
}

// Helper to recursively print condition tree
void print_condition_tree(ConditionNode *node, int depth) {
    while (node) {
        for (int i = 0; i < depth; i++) printf("  ");
        
        if (node->is_sub_expression) {
            printf("SUB-EXPRESSION {\n");
            print_condition_tree(node->sub, depth + 1);
            for (int i = 0; i < depth; i++) printf("  ");
            printf("}\n");
        } else {
            printf("CONDITION: %s ", node->condition.column);
            switch (node->condition.op) {
                case OP_EQ: printf("= "); break;
                case OP_NEQ: printf("!= "); break;
                case OP_GT: printf("> "); break;
                case OP_LT: printf("< "); break;
                case OP_GTE: printf(">= "); break;
                case OP_LTE: printf("<= "); break;
                default: printf("?? "); break;
            }
            printf("%s (numeric: %d)\n", node->condition.value, node->condition.is_numeric);
        }
        
        if (node->logic_op != LOGIC_NONE) {
            for (int i = 0; i < depth; i++) printf("  ");
            printf("%s\n", node->logic_op == LOGIC_AND ? "AND" : "OR");
        }
        
        node = node->next;
    }
}

void test_simple_conditions() {
    printf("=== Testing simple conditions (no parentheses) ===\n");
    const char *input = "SELECT * FROM users WHERE id = 1 AND name = 'john';";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_SELECT);
    assert(sql.select_all == true);
    assert(sql.num_conditions == 2);
    assert(strcmp(sql.conditions[0].column, "id") == 0);
    assert(strcmp(sql.conditions[1].column, "name") == 0);
    
    printf("Condition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // Verify condition tree structure
    assert(sql.condition_tree != NULL);
    assert(sql.condition_tree->is_sub_expression == false);
    assert(strcmp(sql.condition_tree->condition.column, "id") == 0);
    assert(sql.condition_tree->logic_op == LOGIC_AND);
    assert(sql.condition_tree->next != NULL);
    assert(strcmp(sql.condition_tree->next->condition.column, "name") == 0);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

void test_nested_conditions_simple() {
    printf("=== Testing simple nested conditions ===\n");
    const char *input = "SELECT * FROM users WHERE (id = 1);";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_SELECT);
    
    printf("Condition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // Verify condition tree structure
    assert(sql.condition_tree != NULL);
    assert(sql.condition_tree->is_sub_expression == true);
    assert(sql.condition_tree->sub != NULL);
    assert(strcmp(sql.condition_tree->sub->condition.column, "id") == 0);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

void test_nested_conditions_complex() {
    printf("=== Testing complex nested conditions ===\n");
    const char *input = "SELECT * FROM commands WHERE (user_id = 1 OR user_id = 2) AND risk_level > 3;";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    for (int i = 0; i < count; i++) {
        print_token(tokens[i]);
    }
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_SELECT);
    
    printf("\nCondition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // Verify condition tree structure
    assert(sql.condition_tree != NULL);
    
    // First node should be a sub-expression
    assert(sql.condition_tree->is_sub_expression == true);
    assert(sql.condition_tree->sub != NULL);
    
    // Inside the sub-expression, we should have "user_id = 1 OR user_id = 2"
    ConditionNode *sub = sql.condition_tree->sub;
    assert(strcmp(sub->condition.column, "user_id") == 0);
    assert(sub->logic_op == LOGIC_OR);
    assert(sub->next != NULL);
    assert(strcmp(sub->next->condition.column, "user_id") == 0);
    
    // After the sub-expression, we should have AND risk_level > 3
    assert(sql.condition_tree->logic_op == LOGIC_AND);
    assert(sql.condition_tree->next != NULL);
    assert(sql.condition_tree->next->is_sub_expression == false);
    assert(strcmp(sql.condition_tree->next->condition.column, "risk_level") == 0);
    
    // Verify legacy conditions were also populated
    assert(sql.num_conditions == 3);
    assert(strcmp(sql.conditions[0].column, "user_id") == 0);
    assert(strcmp(sql.conditions[1].column, "user_id") == 0);
    assert(strcmp(sql.conditions[2].column, "risk_level") == 0);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

void test_delete_nested_conditions() {
    printf("=== Testing DELETE with nested conditions ===\n");
    const char *input = "DELETE FROM users WHERE (status = 'inactive' AND age < 18);";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_DELETE);
    assert(strcmp(sql.table, "users") == 0);
    
    printf("Condition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // Verify condition tree structure
    assert(sql.condition_tree != NULL);
    assert(sql.condition_tree->is_sub_expression == true);
    assert(sql.condition_tree->sub != NULL);
    
    ConditionNode *sub = sql.condition_tree->sub;
    assert(strcmp(sub->condition.column, "status") == 0);
    assert(sub->logic_op == LOGIC_AND);
    assert(sub->next != NULL);
    assert(strcmp(sub->next->condition.column, "age") == 0);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

void test_deeply_nested_conditions() {
    printf("=== Testing deeply nested conditions ===\n");
    const char *input = "SELECT * FROM commands WHERE ((user_id = 1) AND (risk_level > 2));";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_SELECT);
    
    printf("Condition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // First level is a sub-expression
    assert(sql.condition_tree != NULL);
    assert(sql.condition_tree->is_sub_expression == true);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

void test_mixed_conditions() {
    printf("=== Testing mixed nested and flat conditions ===\n");
    const char *input = "SELECT * FROM commands WHERE exit_code = 0 AND (user_id = 1 OR user_id = 2);";
    Token tokens[100];
    int count = tokenize(input, tokens, 100);
    
    printf("Query: %s\n", input);
    printf("Tokens found: %d\n", count);
    
    ParsedSQL sql = parse_tokens(tokens);
    
    assert(sql.command == CMD_SELECT);
    
    printf("Condition tree:\n");
    print_condition_tree(sql.condition_tree, 1);
    
    // First node should be a flat condition "exit_code = 0"
    assert(sql.condition_tree != NULL);
    assert(sql.condition_tree->is_sub_expression == false);
    assert(strcmp(sql.condition_tree->condition.column, "exit_code") == 0);
    assert(sql.condition_tree->logic_op == LOGIC_AND);
    
    // Second node should be a sub-expression
    assert(sql.condition_tree->next != NULL);
    assert(sql.condition_tree->next->is_sub_expression == true);
    
    free_condition_tree(sql.condition_tree);
    printf("PASSED\n\n");
}

int main() {
    printf("Running nested conditions tests...\n\n");
    
    test_simple_conditions();
    test_nested_conditions_simple();
    test_nested_conditions_complex();
    test_delete_nested_conditions();
    test_deeply_nested_conditions();
    test_mixed_conditions();
    
    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
