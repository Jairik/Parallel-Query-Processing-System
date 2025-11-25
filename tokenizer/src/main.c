#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "sql.h"

// Helper: check if file exists
int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Helper: process a single SQL command
void process_sql_command(const char *command) {
    // Ignore empty commands
    if (strlen(command) == 0)
        return;

    Token tokens[64];
    tokenize(command, tokens, 64);

    ParsedSQL sql = parse_tokens(tokens);

    printf("Parsed Command Type: %d\n", sql.command);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <SQL command or filename>\n", argv[0]);
        return 1;
    }

    const char *arg = argv[1];

    // CASE 1: It's a file
    if (file_exists(arg)) {
        FILE *fp = fopen(arg, "r");
        if (!fp) {
            perror("Could not open file");
            return 1;
        }

        char buffer[1024];
        char command[2048] = "";   // accumulator for multi-line commands

        while (fgets(buffer, sizeof(buffer), fp)) {
            strcat(command, buffer);

            // Check for semicolon (end of SQL command)
            char *semicolon;
            while ((semicolon = strchr(command, ';')) != NULL) {
                // Extract one full command
                char single_cmd[2048];
                size_t len = semicolon - command + 1;
                strncpy(single_cmd, command, len);
                single_cmd[len] = '\0';

                // Process this one command
                process_sql_command(single_cmd);

                // Shift remainder left (for multiple commands in same line)
                memmove(command, semicolon + 1, strlen(semicolon + 1) + 1);
            }
        }

        // In case file ends without semicolon
        if (strlen(command) > 0)
            process_sql_command(command);

        fclose(fp);
        return 0;
    }

    // CASE 2: Treat argument as a single SQL command string
    process_sql_command(arg);

    return 0;
}
