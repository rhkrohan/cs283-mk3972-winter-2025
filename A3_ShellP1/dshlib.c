#include "dshlib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 * trim_whitespace()
 *
 * Removes any leading or trailing whitespace from the given string.
 * This function works in–place and returns a pointer to the trimmed string.
 */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';
    return str;
}

/*
 * build_cmd_list()
 *
 * Given a complete command line in cmd_line, this function splits it into
 * individual commands separated by the pipe (|) symbol. For each command,
 * it further tokenizes the string by whitespace to obtain the command name and
 * its arguments. The parsed commands are stored in the provided command_list_t
 * structure.
 *
 * If there are more than MAX_COMMANDS commands, an error is printed and a nonzero
 * value is returned.
 *
 * Returns 0 on success, or a nonzero value on error.
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || clist == NULL) {
        return 1;
    }
    clist->num_commands = 0;

    /* Use strtok_r so that we can safely split by '|' */
    char *saveptr;
    int cmd_index = 0;
    char *token = strtok_r(cmd_line, "|", &saveptr);
    while (token != NULL) {
        /* Trim extra whitespace from this command token */
        char *cmd_str = trim_whitespace(token);
        if (strlen(cmd_str) > 0) {
            if (cmd_index >= MAX_COMMANDS) {
                fprintf(stderr, "error: piping limited to %d commands\n", MAX_COMMANDS);
                return 1;
            }
            /* Copy the command string because we will use strtok_r on it */
            char *cmd_copy = strdup(cmd_str);
            if (!cmd_copy) {
                fprintf(stderr, "Memory allocation error\n");
                return 1;
            }
            /* Tokenize the command string by whitespace */
            char *token2;
            char *saveptr2;
            token2 = strtok_r(cmd_copy, " \t", &saveptr2);
            if (token2 == NULL) {
                free(cmd_copy);
                token = strtok_r(NULL, "|", &saveptr);
                continue;
            }
            /* The first token is the command name */
            clist->commands[cmd_index].name = strdup(token2);
            if (!clist->commands[cmd_index].name) {
                fprintf(stderr, "Memory allocation error\n");
                free(cmd_copy);
                return 1;
            }
            clist->commands[cmd_index].argc = 0;
            int max_args = 4;  /* initial capacity for arguments */
            clist->commands[cmd_index].args = malloc(max_args * sizeof(char *));
            if (!clist->commands[cmd_index].args) {
                fprintf(stderr, "Memory allocation error\n");
                free(cmd_copy);
                return 1;
            }
            int arg_count = 0;
            /* The remaining tokens (if any) are arguments */
            while ((token2 = strtok_r(NULL, " \t", &saveptr2)) != NULL) {
                if (arg_count >= max_args) {
                    max_args *= 2;
                    char **temp = realloc(clist->commands[cmd_index].args, max_args * sizeof(char *));
                    if (!temp) {
                        fprintf(stderr, "Memory allocation error\n");
                        free(cmd_copy);
                        return 1;
                    }
                    clist->commands[cmd_index].args = temp;
                }
                clist->commands[cmd_index].args[arg_count] = strdup(token2);
                if (!clist->commands[cmd_index].args[arg_count]) {
                    fprintf(stderr, "Memory allocation error\n");
                    free(cmd_copy);
                    return 1;
                }
                arg_count++;
            }
            clist->commands[cmd_index].argc = arg_count;
            free(cmd_copy);
            cmd_index++;
        }
        token = strtok_r(NULL, "|", &saveptr);
    }
    clist->num_commands = cmd_index;
    return 0;
}

/*
 * free_cmd_list()
 *
 * Releases any memory allocated for storing the commands and their arguments.
 */
void free_cmd_list(command_list_t *clist)
{
    if (!clist)
        return;
    for (int i = 0; i < clist->num_commands; i++) {
        free(clist->commands[i].name);
        for (int j = 0; j < clist->commands[i].argc; j++) {
            free(clist->commands[i].args[j]);
        }
        free(clist->commands[i].args);
    }
    clist->num_commands = 0;
}

