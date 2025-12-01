#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sql.h"

// ---- TOKENIZER ----
int tokenize(const char *input, Token tokens[], int max_tokens) {
    int i = 0, pos = 0;

    while (input[pos] && i < max_tokens - 1) {
        while (isspace(input[pos])) pos++;

        if (!input[pos]) break;

        // Comments
        if (input[pos] == '-' && input[pos+1] == '-') {
            while (input[pos] && input[pos] != '\n') pos++;
            continue;
        }

        // Symbols
        if (strchr(";,()*=", input[pos])) {
            tokens[i].type = TOKEN_SYMBOL;
            tokens[i].value[0] = input[pos];
            tokens[i].value[1] = '\0';
            pos++; i++;
            continue;
        }
        
        // Operators like >=, <=, !=, >
        if (strchr("><!", input[pos])) {
             tokens[i].type = TOKEN_SYMBOL;
             tokens[i].value[0] = input[pos];
             int len = 1;
             if (input[pos+1] == '=') {
                 tokens[i].value[1] = '=';
                 len++;
             }
             tokens[i].value[len] = '\0';
             pos += len;
             i++;
             continue;
        }

        // Strings (quoted)
        if (input[pos] == '"' || input[pos] == '\'') {
            char quote = input[pos++];
            int start = pos;
            while (input[pos] && input[pos] != quote) pos++;
            
            int len = pos - start;
            strncpy(tokens[i].value, input + start, len);
            tokens[i].value[len] = '\0';
            tokens[i].type = TOKEN_STRING;
            if (input[pos] == quote) pos++;
            i++;
            continue;
        }

        // Identifiers / Keywords / Numbers
        if (isalnum(input[pos]) || input[pos] == '_') {
            int start = pos;
            // Check if it's a number
            if (isdigit(input[pos])) {
                 while (isdigit(input[pos])) pos++;
                 if (isalpha(input[pos])) { // It was actually an identifier starting with digit? Assume not for now
                 } else {
                     int len = pos - start;
                     strncpy(tokens[i].value, input + start, len);
                     tokens[i].value[len] = '\0';
                     tokens[i].type = TOKEN_NUMBER;
                     i++;
                     continue;
                 }
            }

            while (isalnum(input[pos]) || input[pos] == '_')
                pos++;

            int len = pos - start;
            strncpy(tokens[i].value, input + start, len);
            tokens[i].value[len] = '\0';
            
            // Check if keyword (case insensitive for SQL keywords usually, but let's normalize)
            // Actually, let's keep original case for identifiers, but uppercase for keywords check
            char upper[256];
            strcpy(upper, tokens[i].value);
            for(int k=0; upper[k]; k++) upper[k] = toupper(upper[k]);

            if (strcmp(upper, "SELECT") == 0 || strcmp(upper, "FROM") == 0 || 
                strcmp(upper, "WHERE") == 0 || strcmp(upper, "ORDER") == 0 || 
                strcmp(upper, "BY") == 0 || strcmp(upper, "DESC") == 0 || 
                strcmp(upper, "ASC") == 0 ||
                strcmp(upper, "AND") == 0 || strcmp(upper, "OR") == 0 || 
                strcmp(upper, "TRUE") == 0 || strcmp(upper, "FALSE") == 0 || 
                strcmp(upper, "DESCRIBE") == 0 ||
                strcmp(upper, "INSERT") == 0 || strcmp(upper, "INTO") == 0 ||
                strcmp(upper, "VALUES") == 0 || strcmp(upper, "DELETE") == 0) {
                tokens[i].type = TOKEN_KEYWORD;
                strcpy(tokens[i].value, upper); // Store as uppercase
            } else {
                tokens[i].type = TOKEN_IDENTIFIER;
            }
            i++;
            continue;
        }

        pos++;
    }

    tokens[i].type = TOKEN_EOF;
    strcpy(tokens[i].value, "");
    return i;
}


// ---- CONDITION NODE HELPERS ----
// Creates a new ConditionNode. Returns NULL if memory allocation fails.
ConditionNode *create_condition_node(void) {
    ConditionNode *node = (ConditionNode *)malloc(sizeof(ConditionNode));
    if (node) {
        node->is_sub_expression = false;
        memset(&node->condition, 0, sizeof(Condition));
        node->sub = NULL;
        node->logic_op = LOGIC_NONE;
        node->next = NULL;
    }
    return node;
}

void free_condition_tree(ConditionNode *node) {
    while (node) {
        ConditionNode *next = node->next;
        if (node->sub) {
            free_condition_tree(node->sub);
        }
        free(node);
        node = next;
    }
}

// Helper to check if current token indicates end of WHERE clause context
static int is_where_terminator(Token *t) {
    if (t->type == TOKEN_EOF) return 1;
    if (t->type == TOKEN_SYMBOL && strcmp(t->value, ";") == 0) return 1;
    if (t->type == TOKEN_SYMBOL && strcmp(t->value, ")") == 0) return 1;
    if (t->type == TOKEN_KEYWORD && strcmp(t->value, "ORDER") == 0) return 1;
    return 0;
}

// Forward declaration for recursive parsing
static ConditionNode *parse_where_conditions(Token tokens[], int *pos);

// Parse a single condition (column op value) or a sub-expression in parentheses
static ConditionNode *parse_single_condition(Token tokens[], int *pos) {
    int i = *pos;
    
    // Check for opening parenthesis - indicates a sub-expression
    if (tokens[i].type == TOKEN_SYMBOL && strcmp(tokens[i].value, "(") == 0) {
        i++; // Skip '('
        *pos = i;
        
        ConditionNode *node = create_condition_node();
        node->is_sub_expression = true;
        node->sub = parse_where_conditions(tokens, pos);
        
        i = *pos;
        // Expect closing parenthesis
        if (tokens[i].type == TOKEN_SYMBOL && strcmp(tokens[i].value, ")") == 0) {
            i++; // Skip ')'
        }
        *pos = i;
        return node;
    }
    
    // Parse a simple condition: column operator value
    if (tokens[i].type != TOKEN_IDENTIFIER) {
        return NULL;
    }
    
    ConditionNode *node = create_condition_node();
    node->is_sub_expression = false;
    
    // Column
    strcpy(node->condition.column, tokens[i].value);
    i++;
    
    // Operator
    if (strcmp(tokens[i].value, "=") == 0) node->condition.op = OP_EQ;
    else if (strcmp(tokens[i].value, "!=") == 0) node->condition.op = OP_NEQ;
    else if (strcmp(tokens[i].value, ">") == 0) node->condition.op = OP_GT;
    else if (strcmp(tokens[i].value, "<") == 0) node->condition.op = OP_LT;
    else if (strcmp(tokens[i].value, ">=") == 0) node->condition.op = OP_GTE;
    else if (strcmp(tokens[i].value, "<=") == 0) node->condition.op = OP_LTE;
    else node->condition.op = OP_NONE;
    i++;
    
    // Value
    if (tokens[i].type == TOKEN_STRING) {
        strcpy(node->condition.value, tokens[i].value);
        node->condition.is_numeric = false;
        i++;
    } else if (tokens[i].type == TOKEN_NUMBER) {
        strcpy(node->condition.value, tokens[i].value);
        node->condition.is_numeric = true;
        i++;
    } else if (tokens[i].type == TOKEN_KEYWORD && 
               (strcmp(tokens[i].value, "TRUE") == 0 || strcmp(tokens[i].value, "FALSE") == 0)) {
        strcpy(node->condition.value, tokens[i].value);
        node->condition.is_numeric = false;
        i++;
    }
    
    *pos = i;
    return node;
}

// Parse a chain of conditions with AND/OR operators
static ConditionNode *parse_where_conditions(Token tokens[], int *pos) {
    int i = *pos;
    ConditionNode *head = NULL;
    ConditionNode *current = NULL;
    
    while (!is_where_terminator(&tokens[i])) {
        ConditionNode *node = parse_single_condition(tokens, &i);
        if (!node) break;
        
        if (head == NULL) {
            head = node;
            current = node;
        } else {
            current->next = node;
            current = node;
        }
        
        // Check for logical operator
        if (tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].value, "AND") == 0) {
            current->logic_op = LOGIC_AND;
            i++;
        } else if (tokens[i].type == TOKEN_KEYWORD && strcmp(tokens[i].value, "OR") == 0) {
            current->logic_op = LOGIC_OR;
            i++;
        } else {
            current->logic_op = LOGIC_NONE;
            break; // No more conditions
        }
    }
    
    *pos = i;
    return head;
}


// ---- PARSER ----
ParsedSQL parse_tokens(Token tokens[]) {
    ParsedSQL sql = { CMD_NONE };
    sql.condition_tree = NULL;  // Initialize condition tree
    int i = 0;

    if (tokens[i].type == TOKEN_KEYWORD) {
        if (strcmp(tokens[i].value, "DESCRIBE") == 0) {
            sql.command = CMD_DESCRIBE;
            i++;
            if (tokens[i].type == TOKEN_IDENTIFIER) {
                strcpy(sql.table, tokens[i].value);
            }
        }
        else if (strcmp(tokens[i].value, "SELECT") == 0) {
            sql.command = CMD_SELECT;
            i++;
            
            // Parse columns
            while (tokens[i].type != TOKEN_EOF) {
                if (strcmp(tokens[i].value, "*") == 0) {
                    sql.select_all = true;
                    i++;
                } else if (tokens[i].type == TOKEN_IDENTIFIER) {
                    strcpy(sql.columns[sql.num_columns++], tokens[i].value);
                    i++;
                }
                
                if (strcmp(tokens[i].value, ",") == 0) {
                    i++;
                    continue;
                }
                if (strcmp(tokens[i].value, "FROM") == 0) {
                    break;
                }
                // Safety break
                if (tokens[i].type == TOKEN_EOF) break;
            }

            // Parse FROM
            if (strcmp(tokens[i].value, "FROM") == 0) {
                i++;
                if (tokens[i].type == TOKEN_IDENTIFIER) {
                    strcpy(sql.table, tokens[i].value);
                    i++;
                }
            }

            // Parse WHERE
            if (strcmp(tokens[i].value, "WHERE") == 0) {
                i++;
                int where_start = i;
                
                // Build the condition tree for nested conditions
                sql.condition_tree = parse_where_conditions(tokens, &i);
                
                // Also populate legacy flat conditions array for backward compatibility
                int legacy_i = where_start;
                while (tokens[legacy_i].type != TOKEN_EOF && strcmp(tokens[legacy_i].value, "ORDER") != 0 && strcmp(tokens[legacy_i].value, ";") != 0) {
                    if (sql.num_conditions >= 5) break;
                    
                    // Skip parentheses for legacy parsing
                    if (tokens[legacy_i].type == TOKEN_SYMBOL && 
                        (strcmp(tokens[legacy_i].value, "(") == 0 || strcmp(tokens[legacy_i].value, ")") == 0)) {
                        legacy_i++;
                        continue;
                    }
                    
                    Condition *cond = &sql.conditions[sql.num_conditions];
                    
                    // Column
                    if (tokens[legacy_i].type == TOKEN_IDENTIFIER) {
                        strcpy(cond->column, tokens[legacy_i].value);
                        legacy_i++;
                    } else {
                        break; // Not a valid condition start
                    }

                    // Operator
                    if (strcmp(tokens[legacy_i].value, "=") == 0) cond->op = OP_EQ;
                    else if (strcmp(tokens[legacy_i].value, "!=") == 0) cond->op = OP_NEQ;
                    else if (strcmp(tokens[legacy_i].value, ">") == 0) cond->op = OP_GT;
                    else if (strcmp(tokens[legacy_i].value, "<") == 0) cond->op = OP_LT;
                    else if (strcmp(tokens[legacy_i].value, ">=") == 0) cond->op = OP_GTE;
                    else if (strcmp(tokens[legacy_i].value, "<=") == 0) cond->op = OP_LTE;
                    else cond->op = OP_NONE;
                    legacy_i++;

                    // Value
                    if (tokens[legacy_i].type == TOKEN_STRING) {
                        strcpy(cond->value, tokens[legacy_i].value);
                        cond->is_numeric = false;
                        legacy_i++;
                    } else if (tokens[legacy_i].type == TOKEN_NUMBER) {
                        strcpy(cond->value, tokens[legacy_i].value);
                        cond->is_numeric = true;
                        legacy_i++;
                    } else if (tokens[legacy_i].type == TOKEN_KEYWORD && (strcmp(tokens[legacy_i].value, "TRUE") == 0 || strcmp(tokens[legacy_i].value, "FALSE") == 0)) {
                         strcpy(cond->value, tokens[legacy_i].value);
                         cond->is_numeric = false; // Treat boolean as string for now
                         legacy_i++;
                    }

                    sql.num_conditions++;

                    // Logic Op
                    // Skip closing parentheses before checking for logic op
                    while (tokens[legacy_i].type == TOKEN_SYMBOL && strcmp(tokens[legacy_i].value, ")") == 0) {
                        legacy_i++;
                    }
                    
                    if (strcmp(tokens[legacy_i].value, "AND") == 0) {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_AND;
                        legacy_i++;
                    } else if (strcmp(tokens[legacy_i].value, "OR") == 0) {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_OR;
                        legacy_i++;
                    } else {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_NONE;
                    }
                    
                    // Skip opening parentheses before next condition
                    while (tokens[legacy_i].type == TOKEN_SYMBOL && strcmp(tokens[legacy_i].value, "(") == 0) {
                        legacy_i++;
                    }
                }
            }

            // Parse ORDER BY
            if (strcmp(tokens[i].value, "ORDER") == 0) {
                i++;
                if (strcmp(tokens[i].value, "BY") == 0) {
                    i++;
                    if (tokens[i].type == TOKEN_IDENTIFIER) {
                        strcpy(sql.order_by, tokens[i].value);
                        i++;
                    }
                    if (strcmp(tokens[i].value, "DESC") == 0) {
                        sql.order_desc = true;
                        i++;
                    } else if (strcmp(tokens[i].value, "ASC") == 0) {
                        sql.order_desc = false;
                        i++;
                    }
                }
            }
        }
        else if (strcmp(tokens[i].value, "INSERT") == 0) {
            sql.command = CMD_INSERT;
            i++;
            if (strcmp(tokens[i].value, "INTO") == 0) i++;
            if (tokens[i].type == TOKEN_IDENTIFIER) {
                strcpy(sql.table, tokens[i].value);
                i++;
            }
            if (strcmp(tokens[i].value, "VALUES") == 0) i++;
            if (strcmp(tokens[i].value, "(") == 0) i++;
            
            while (tokens[i].type != TOKEN_EOF && strcmp(tokens[i].value, ")") != 0) {
                if (strcmp(tokens[i].value, ",") == 0) {
                    i++;
                    continue;
                }
                strcpy(sql.insert_values[sql.num_values++], tokens[i].value);
                i++;
            }
        }
        else if (strcmp(tokens[i].value, "DELETE") == 0) {
            sql.command = CMD_DELETE;
            i++;
            if (strcmp(tokens[i].value, "FROM") == 0) i++;
            if (tokens[i].type == TOKEN_IDENTIFIER) {
                strcpy(sql.table, tokens[i].value);
                i++;
            }

            // Parse WHERE
            if (strcmp(tokens[i].value, "WHERE") == 0) {
                i++;
                int where_start = i;
                
                // Build the condition tree for nested conditions
                sql.condition_tree = parse_where_conditions(tokens, &i);
                
                // Also populate legacy flat conditions array for backward compatibility
                int legacy_i = where_start;
                while (tokens[legacy_i].type != TOKEN_EOF && strcmp(tokens[legacy_i].value, ";") != 0) {
                    if (sql.num_conditions >= 5) break;
                    
                    // Skip parentheses for legacy parsing
                    if (tokens[legacy_i].type == TOKEN_SYMBOL && 
                        (strcmp(tokens[legacy_i].value, "(") == 0 || strcmp(tokens[legacy_i].value, ")") == 0)) {
                        legacy_i++;
                        continue;
                    }
                    
                    Condition *cond = &sql.conditions[sql.num_conditions];
                    
                    // Column
                    if (tokens[legacy_i].type == TOKEN_IDENTIFIER) {
                        strcpy(cond->column, tokens[legacy_i].value);
                        legacy_i++;
                    } else {
                        break; // Not a valid condition start
                    }

                    // Operator
                    if (strcmp(tokens[legacy_i].value, "=") == 0) cond->op = OP_EQ;
                    else if (strcmp(tokens[legacy_i].value, "!=") == 0) cond->op = OP_NEQ;
                    else if (strcmp(tokens[legacy_i].value, ">") == 0) cond->op = OP_GT;
                    else if (strcmp(tokens[legacy_i].value, "<") == 0) cond->op = OP_LT;
                    else if (strcmp(tokens[legacy_i].value, ">=") == 0) cond->op = OP_GTE;
                    else if (strcmp(tokens[legacy_i].value, "<=") == 0) cond->op = OP_LTE;
                    else cond->op = OP_NONE;
                    legacy_i++;

                    // Value
                    if (tokens[legacy_i].type == TOKEN_STRING) {
                        strcpy(cond->value, tokens[legacy_i].value);
                        cond->is_numeric = false;
                        legacy_i++;
                    } else if (tokens[legacy_i].type == TOKEN_NUMBER) {
                        strcpy(cond->value, tokens[legacy_i].value);
                        cond->is_numeric = true;
                        legacy_i++;
                    } else if (tokens[legacy_i].type == TOKEN_KEYWORD && (strcmp(tokens[legacy_i].value, "TRUE") == 0 || strcmp(tokens[legacy_i].value, "FALSE") == 0)) {
                         strcpy(cond->value, tokens[legacy_i].value);
                         cond->is_numeric = false; // Treat boolean as string for now
                         legacy_i++;
                    }

                    sql.num_conditions++;

                    // Logic Op
                    // Skip closing parentheses before checking for logic op
                    while (tokens[legacy_i].type == TOKEN_SYMBOL && strcmp(tokens[legacy_i].value, ")") == 0) {
                        legacy_i++;
                    }
                    
                    if (strcmp(tokens[legacy_i].value, "AND") == 0) {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_AND;
                        legacy_i++;
                    } else if (strcmp(tokens[legacy_i].value, "OR") == 0) {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_OR;
                        legacy_i++;
                    } else {
                        sql.logic_ops[sql.num_conditions-1] = LOGIC_NONE;
                    }
                    
                    // Skip opening parentheses before next condition
                    while (tokens[legacy_i].type == TOKEN_SYMBOL && strcmp(tokens[legacy_i].value, "(") == 0) {
                        legacy_i++;
                    }
                }
            }
        }
        else {
            sql.command = CMD_UNKNOWN;
        }
    }

    return sql;
}