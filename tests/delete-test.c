#include "../include/executeEngine-serial.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Creating a temporary test csv */
void create_temp_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "command_id,raw_command,base_command,shell_type,exit_code,timestamp,sudo_used,working_directory,user_id,user_name,host_name,risk_level\n");
    fprintf(f, "1,ls -la,ls,bash,0,2023-01-01,0,/home/user,1001,user1,host1,1\n");
    fprintf(f, "2,rm -rf /,rm,bash,1,2023-01-02,1,/root,0,root,host1,5\n");
    fprintf(f, "3,echo hello,echo,bash,0,2023-01-03,0,/home/user,1001,user1,host1,1\n");
    fclose(f);
}

void test_delete_persistence() {
    printf("Testing DELETE persistence...\n");
    const char *temp_file = "temp_delete_test.csv";
    create_temp_csv(temp_file);

    // Initialize engine
    const char *indexed_attrs[] = {"command_id"};
    int attr_types[] = {0}; // 0 for int/uint
    struct engineS *engine = initializeEngineSerial(1, indexed_attrs, attr_types, temp_file, "test_table");

    // Verify initial count
    assert(engine->num_records == 3);

    // Delete command_id = 2
    struct whereClauseS wc;
    wc.attribute = "command_id";
    wc.operator = "=";
    wc.value = "2";
    wc.value_type = 0;
    wc.next = NULL;
    wc.sub = NULL;
    wc.logical_op = NULL;

    struct resultSetS *res = executeQueryDeleteSerial(engine, "test_table", &wc);
    assert(res->success == true);
    assert(res->numRecords == 1);
    assert(engine->num_records == 2);
    freeResultSet(res);

    // Destroy engine to close everything (though file is closed in delete)
    destroyEngineSerial(engine);

    // Verify file content
    FILE *f = fopen(temp_file, "r");
    char buffer[1024];
    int line_count = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        line_count++;
        // Check that command_id 2 is gone
        if (strstr(buffer, "2,rm -rf") != NULL) {
            printf("FAILED: Deleted record still in file!\n");
            assert(false);
        }
    }
    fclose(f);
    
    assert(line_count == 2);
    printf("Test Passed: File updated correctly.\n");
    
    // Cleanup
    remove(temp_file);
}

void test_delete_index_runtime() {
    printf("Testing DELETE index runtime update...\n");
    const char *temp_file = "temp_delete_index_test.csv";
    create_temp_csv(temp_file);

    // Initialize engine
    const char *indexed_attrs[] = {"command_id"};
    int attr_types[] = {0}; // 0 for int/uint
    struct engineS *engine = initializeEngineSerial(1, indexed_attrs, attr_types, temp_file, "test_table");

    // Check that key 2 exists
    KEY_T key2;
    key2.type = KEY_UINT64;
    key2.v.u64 = 2;
    ROW_PTR *results;
    int count = find_rows(engine->bplus_tree_roots[0], key2, &results);
    assert(count > 0);
    free(results);

    // Delete command_id = 2
    struct whereClauseS wc;
    wc.attribute = "command_id";
    wc.operator = "=";
    wc.value = "2";
    wc.value_type = 0;
    wc.next = NULL;
    wc.sub = NULL;
    wc.logical_op = NULL;

    struct resultSetS *res = executeQueryDeleteSerial(engine, "test_table", &wc);
    assert(res->success == true);
    freeResultSet(res);

    // Check that key 2 is gone from index
    count = find_rows(engine->bplus_tree_roots[0], key2, &results);
    assert(count == 0);
    printf("Test Passed: Index updated correctly at runtime.\n");

    destroyEngineSerial(engine);
    remove(temp_file);
}

int main() {
    test_delete_persistence();
    test_delete_index_runtime();
    return 0;
}
