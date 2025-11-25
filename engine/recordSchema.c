/* Helpers for dynamically resolving attribute names and performing multi-type comparisons on B+ Tree */

#include "../include/recordSchema.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Predefined attribute offsets for easy access
 * To be used when we need an attribute that is only known at runtime. For this, we can 
 * get the offset from the corresponding attribute name and use that (since record->{x} is not allowed)
*/
static const FieldInfo record_fields[] = {
    { "command_id", offsetof(record, command_id), FIELD_UINT64 },
    { "raw_command", offsetof(record, raw_command), FIELD_STRING },
    { "base_command", offsetof(record, base_command), FIELD_STRING },
    { "shell_type", offsetof(record, shell_type), FIELD_STRING },
    { "exit_code", offsetof(record, exit_code), FIELD_INT },
    { "timestamp", offsetof(record, timestamp), FIELD_STRING },
    { "sudo_used", offsetof(record, sudo_used), FIELD_BOOL },
    { "working_directory", offsetof(record, working_directory), FIELD_STRING },
    { "user_id", offsetof(record, user_id), FIELD_INT },
    { "user_name", offsetof(record, user_name), FIELD_STRING },
    { "host_name", offsetof(record, host_name), FIELD_STRING },
    { "risk_level", offsetof(record, risk_level), FIELD_INT }
};

static const size_t NUM_RECORD_FIELDS = sizeof(record_fields) / sizeof(record_fields[0]);

/* Lookup FieldInfo by attribute name */
const FieldInfo *get_field_info(const char *name)
{
    for (size_t i = 0; i < NUM_RECORD_FIELDS; i++) {
        if (strcmp(record_fields[i].name, name) == 0) {
            return &record_fields[i];
        }
    }
    return NULL;
}

/* Extract a KEY_T suitable for indexing from a record field */
KEY_T extract_key_from_record(const record *rec, const char *attr_name) {
    // Get field info for the attribute
    const FieldInfo *info = get_field_info(attr_name);
    if (!info) {
        fprintf(stderr, "Unknown index attribute: %s\n", attr_name);
        exit(EXIT_FAILURE);
    }

    // Declare a pointer to the field data
    const void *ptr = (const char *)rec + info->offset;
    KEY_T key = {0};

    // Set the KEY_T and relevant attributes based on field type
    switch (info->type) {
    case FIELD_UINT64:
        key.type = KEY_UINT64;
        key.v.u64 = *(const uint64_t *)ptr;
        break;

    case FIELD_INT:
        key.type = KEY_INT;
        key.v.i32 = *(const int *)ptr;
        break;

    case FIELD_BOOL:
        key.type = KEY_BOOL;
        key.v.b = *(const bool *)ptr;
        break;

    case FIELD_STRING:
        key.type = KEY_STRING;
        key.v.str = (const char *)ptr;  // Will contain the actual string content
        break;

    default:
        fprintf(stderr, "Unsupported key type for attribute: %s\n", attr_name);
        exit(EXIT_FAILURE);
    }

    return key;
}

/* Compare two KEY_T values; returns:
 *  < 0 if key1 < key2
 *  = 0 if key1 == key2
 *  > 0 if key1 > key2
 */
int compare_key(KEY_T key1, KEY_T key2) {
    // Type mismatch: cannot compare (treat as unequal, type precedence order)
    if (key1.type != key2.type) {
        return (int)key1.type - (int)key2.type;
    }

    // Same type: perform type-specific comparison
    switch (key1.type) {
    
    // Long integer comparison
    case KEY_UINT64:
        if (key1.v.u64 < key2.v.u64) return -1;
        if (key1.v.u64 > key2.v.u64) return 1;
        return 0;

    // Integer comparison
    case KEY_INT:
        if (key1.v.i32 < key2.v.i32) return -1;
        if (key1.v.i32 > key2.v.i32) return 1;
        return 0;
    
    // Boolean comparison
    case KEY_BOOL:
        // false < true
        if (key1.v.b == key2.v.b) return 0;
        return key1.v.b ? 1 : -1;

    // String comparison
    case KEY_STRING:
        if (!key1.v.str && !key2.v.str) return 0;
        if (!key1.v.str) return -1;
        if (!key2.v.str) return 1;
        return strcmp(key1.v.str, key2.v.str);

    // Should never happen but a safegaurd
    default:
        fprintf(stderr, "Unknown KEY_T type in compare_key\n");
        return 0;
    }
}
