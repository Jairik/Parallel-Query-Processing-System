#include "../include/executeEngine-serial.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Mock implementation or linking required. 
// For this test, we assume we compile with executeEngine-serial.c

void test_evaluateWhereClause() {
    printf("Testing evaluateWhereClause...\n");

    // Create a dummy record
    record r;
    r.command_id = 100;
    r.risk_level = 5;
    r.user_id = 10;
    strcpy(r.user_name, "admin");
    r.sudo_used = true;
    // Initialize other fields to avoid undefined behavior if accessed (though test won't access them)
    r.exit_code = 0;
    strcpy(r.raw_command, "ls -la");
    strcpy(r.base_command, "ls");
    strcpy(r.shell_type, "bash");
    strcpy(r.timestamp, "2023-01-01");
    strcpy(r.working_directory, "/home/admin");
    strcpy(r.host_name, "localhost");


    // Test 1: Simple condition (risk_level > 3)
    struct whereClauseS wc1;
    wc1.attribute = "risk_level";
    wc1.operator = ">";
    wc1.value = "3";
    wc1.value_type = 0;
    wc1.next = NULL;
    wc1.sub = NULL;
    wc1.logical_op = NULL;

    assert(evaluateWhereClause(&r, &wc1) == true);
    printf("Test 1 Passed: Simple Condition\n");

    // Test 2: Nested condition ((risk_level > 3 AND user_id = 10))
    struct whereClauseS wc2_sub2;
    wc2_sub2.attribute = "user_id";
    wc2_sub2.operator = "=";
    wc2_sub2.value = "10";
    wc2_sub2.value_type = 0;
    wc2_sub2.next = NULL;
    wc2_sub2.sub = NULL;
    wc2_sub2.logical_op = NULL;

    struct whereClauseS wc2_sub1;
    wc2_sub1.attribute = "risk_level";
    wc2_sub1.operator = ">";
    wc2_sub1.value = "3";
    wc2_sub1.value_type = 0;
    wc2_sub1.next = &wc2_sub2;
    wc2_sub1.sub = NULL;
    wc2_sub1.logical_op = "AND";

    struct whereClauseS wc2;
    wc2.attribute = NULL;
    wc2.operator = NULL;
    wc2.value = NULL;
    wc2.value_type = 0;
    wc2.next = NULL;
    wc2.sub = &wc2_sub1; // Parentheses around the whole thing
    wc2.logical_op = NULL;

    assert(evaluateWhereClause(&r, &wc2) == true);
    printf("Test 2 Passed: Nested AND\n");

    // Test 3: Complex ((risk_level > 10) OR (user_id = 10))
    // Sub1: risk_level > 10 (False)
    struct whereClauseS wc3_sub1;
    wc3_sub1.attribute = "risk_level";
    wc3_sub1.operator = ">";
    wc3_sub1.value = "10";
    wc3_sub1.value_type = 0;
    wc3_sub1.next = NULL;
    wc3_sub1.sub = NULL;
    wc3_sub1.logical_op = NULL;

    // Sub2: user_id = 10 (True)
    struct whereClauseS wc3_sub2;
    wc3_sub2.attribute = "user_id";
    wc3_sub2.operator = "=";
    wc3_sub2.value = "10";
    wc3_sub2.value_type = 0;
    wc3_sub2.next = NULL;
    wc3_sub2.sub = NULL;
    wc3_sub2.logical_op = NULL;

    // Top level: (Sub1) OR (Sub2)
    struct whereClauseS wc3_node2;
    wc3_node2.attribute = NULL;
    wc3_node2.operator = NULL;
    wc3_node2.value = NULL;
    wc3_node2.value_type = 0;
    wc3_node2.next = NULL;
    wc3_node2.sub = &wc3_sub2;
    wc3_node2.logical_op = NULL;

    struct whereClauseS wc3_node1;
    wc3_node1.attribute = NULL;
    wc3_node1.operator = NULL;
    wc3_node1.value = NULL;
    wc3_node1.value_type = 0;
    wc3_node1.next = &wc3_node2;
    wc3_node1.sub = &wc3_sub1;
    wc3_node1.logical_op = "OR";

    assert(evaluateWhereClause(&r, &wc3_node1) == true);
    printf("Test 3 Passed: Complex OR with Nesting\n");
}

int main() {
    test_evaluateWhereClause();
    return 0;
}
