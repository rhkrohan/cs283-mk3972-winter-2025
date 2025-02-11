#ifndef DSHLIB_H
#define DSHLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Maximum length of an input line */
#define ARG_MAX 1024

/* Maximum number of piped commands */
#define MAX_COMMANDS 8

/* Shell prompt string */
#define SH_PROMPT "dsh> "

/* A return code for functions not yet implemented (not used in final version) */
#define EXIT_NOT_IMPL -1

/* Data structure representing one command (a tokenized string) */
typedef struct {
    char *name;     /* the command name */
    char **args;    /* an array of argument strings (if any) */
    int argc;       /* number of arguments */
} command_t;

/* Data structure for a list of commands (possibly connected via pipes) */
typedef struct {
    int num_commands;
    command_t commands[MAX_COMMANDS];
} command_list_t;

/* Function prototypes */
int build_cmd_list(char *cmd_line, command_list_t *clist);
void free_cmd_list(command_list_t *clist);

#endif /* DSHLIB_H */

