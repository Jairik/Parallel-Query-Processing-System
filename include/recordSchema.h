#ifndef RECORD_SCHEMA_H
#define RECORD_SCHEMA_H

#include <stddef.h>
#include <stdint.h>
#include "logType.h"  // Record structs
#include "bplus.h"  // KEY_T

// Metadata about each field in the record struct
typedef enum {
    FIELD_UINT64,
    FIELD_INT,
    FIELD_STRING,
    FIELD_BOOL
} FieldType;

// Structure to hold field metadata
typedef struct {
    const char *name;
    size_t offset;
    FieldType type;
} FieldInfo;

// Helper that provides the offset and type of the given attribute
const FieldInfo *get_field_info(const char *name);
// Helper that extracts the key value from a record given the attribute name
KEY_T extract_key_from_record(const record *rec, const char *attr_name);
// Helper for comparing two KEY_T values
int compare_key(KEY_T key1, KEY_T key2);

#endif  // RECORD_SCHEMA_H
