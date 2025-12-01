#include "../include/executeEngine-serial.h"
#include "../include/bplus.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void create_dup_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "command_id,raw_command,base_command,shell_type,exit_code,timestamp,sudo_used,working_directory,user_id,user_name,host_name,risk_level\n");
    fprintf(f, "1,cmd1,base,bash,0,ts,0,wd,1001,user,host,1\n");
    fprintf(f, "2,cmd2,base,bash,0,ts,0,wd,1001,user,host,1\n"); // Duplicate risk_level 1
    fprintf(f, "3,cmd3,base,bash,0,ts,0,wd,1001,user,host,2\n");
    fprintf(f, "4,cmd4,base,bash,0,ts,0,wd,1001,user,host,1\n"); // Another duplicate risk_level 1
    fclose(f);
}

int main() {
    printf("Testing B+ Tree Duplicates...\n");
    const char *temp_file = "temp_dup_test.csv";
    create_dup_csv(temp_file);

    // Initialize engine with index on risk_level (int)
    const char *indexed_attrs[] = {"risk_level"};
    int attr_types[] = {1}; // 1 for int
    struct engineS *engine = initializeEngineSerial(1, indexed_attrs, attr_types, temp_file, "test_table");

    // Verify count
    assert(engine->num_records == 4);

    // Search for risk_level = 1
    KEY_T key;
    key.type = KEY_INT;
    key.v.i32 = 1;

    ROW_PTR *results;
    int count = find_rows(engine->bplus_tree_roots[0], key, &results);
    
    printf("Found %d records with risk_level=1\n", count);
    assert(count == 3);

    // Verify the records are correct
    for (int i = 0; i < count; i++) {
        record *r = (record *)results[i];
        assert(r->risk_level == 1);
        printf("  Record %d: command_id=%llu\n", i, r->command_id);
    }
    free(results);

    // Search for risk_level = 2
    key.v.i32 = 2;
    count = find_rows(engine->bplus_tree_roots[0], key, &results);
    printf("Found %d records with risk_level=2\n", count);
    assert(count == 1);
    free(results);

    destroyEngineSerial(engine);
    remove(temp_file);
    printf("Duplicate Test Passed!\n");
    return 0;
}
