/* Structure of each sample (log) in the database */

#ifndef LOGTYPE_H
#define LOGTYPE_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h> // for strcmp in get_field_info

// Record structure representing a command log entry
typedef struct record {
    unsigned long long command_id; // Unique key for the record
    char raw_command[512]; // Full command string
    char base_command[100]; // Base command without arguments
    char shell_type[20]; // Type of shell (eg, bash, zsh)
    int exit_code; // Exit code of the command
    char timestamp[30]; // Execution timestamp
    bool sudo_used; // Whether the command was run with sudo
    char working_directory[200]; // Directory where the command was executed
    int user_id; // ID of the user who executed the command
    char user_name[50]; // Name of the user who executed the command
    char host_name[100]; // Hostname of the machine
    int risk_level; // Risk level associated with the command
} record;

#endif // LOGTYPE_H