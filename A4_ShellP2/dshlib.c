#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

/* 
 * Helper routines that allocate/clear/free the cmd_buff_t.
 * You may adjust or omit these if you prefer to manage memory differently.
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (!cmd_buff)
        return ERR_MEMORY;

    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (!cmd_buff->_cmd_buffer)
        return ERR_MEMORY;

    // Initialize internal buffer and argv
    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;

    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (!cmd_buff || !cmd_buff->_cmd_buffer)
        return ERR_MEMORY;

    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;

    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (!cmd_buff)
        return ERR_MEMORY;

    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    // Just set argv pointers to NULL
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->argc = 0;

    return OK;
}

/*
 * match_command()
 * 
 * Identifies if the first token matches any built-in command 
 * that we plan to handle. You can add more if needed.
 */
Built_In_Cmds match_command(const char *input)
{
    if (!input)
        return BI_NOT_BI;

    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;  // Extra credit in a future assignment
    } else if (strcmp(input, "rc") == 0) {
        return BI_RC;          // Extra credit (return code)
    }

    return BI_NOT_BI;
}

/*
 * build_cmd_buff()
 *
 * Parses the string in cmd_line into cmd_buff->argv[].
 * We handle double-quoted arguments so that spaces inside quotes
 * are preserved as part of a single argument.
 * 
 * Returns:
 *   OK or WARN_NO_CMDS on success,
 *   or possibly ERR_MEMORY if something fails with allocation 
 *   (though here we assume alloc_cmd_buff() was already successful).
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    if (!cmd_line || !cmd_buff) {
        return ERR_MEMORY;
    }

    // Trim leading and trailing whitespace just for safety.
    // (Not strictly required if we handle them gracefully in the parser.)
    while (isspace((unsigned char)*cmd_line)) cmd_line++;
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end >= cmd_line && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    // If the line is empty, return WARN_NO_CMDS
    if (*cmd_line == '\0') {
        cmd_buff->argc = 0;
        cmd_buff->argv[0] = NULL;
        return WARN_NO_CMDS;
    }

    // Copy into cmd_buff->_cmd_buffer so we can tokenize in-place
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';  // ensure null-termination

    int argc = 0;
    bool in_quotes = false;
    char *p = cmd_buff->_cmd_buffer;
    char *arg_start = NULL;

    while (*p != '\0') {
        // Skip leading spaces (outside of quotes)
        if (!in_quotes && isspace((unsigned char)*p)) {
            p++;
            continue;
        }

        // Check if we are toggling quotes
        if (*p == '\"') {
            if (!in_quotes) {
                // starting a quoted token
                in_quotes = true;
                p++;
                arg_start = p; 
            } else {
                // ending a quoted token
                in_quotes = false;
                *p = '\0'; // terminate the argument
                if (argc < CMD_MAX) {
                    cmd_buff->argv[argc++] = arg_start;
                }
                p++;
                arg_start = NULL;
            }
            continue;
        }

        // If we're not in quotes, a space ends the token
        if (!in_quotes && isspace((unsigned char)*p)) {
            // We found the end of a normal token
            *p = '\0';
            if (arg_start && argc < CMD_MAX) {
                cmd_buff->argv[argc++] = arg_start;
            }
            arg_start = NULL;
            p++;
            continue;
        }

        // Otherwise, if we haven't yet started an argument, mark it
        if (!arg_start) {
            arg_start = p;
        }
        p++;
    }

    // If we ended while still "in_quotes" the user had an unmatched quote.
    // We'll treat everything read so far as one argument. 
    // Or you might handle it differently (error out, etc.). 
    if (arg_start) {
        // We have a trailing argument
        if (argc < CMD_MAX) {
            cmd_buff->argv[argc++] = arg_start;
        }
    }

    cmd_buff->argc = argc;
    cmd_buff->argv[argc] = NULL;

    // If no arguments, that's a WARN_NO_CMDS
    if (argc == 0) {
        return WARN_NO_CMDS;
    }
    return OK;
}

/*
 * exec_local_cmd_loop()
 *
 * Implements the main loop of our shell.  Repeatedly:
 *   1) Prints the prompt
 *   2) Reads a line from stdin
 *   3) Parses the line into cmd_buff_t
 *   4) Checks for built-in commands or forks/execs external commands
 *   5) Waits for child processes to finish
 *
 * Exits when user types "exit" or we hit EOF.
 */
int exec_local_cmd_loop()
{
    // We'll allocate one cmd_buff that we reuse each iteration
    cmd_buff_t cmd;
    if (alloc_cmd_buff(&cmd) != OK) {
        fprintf(stderr, "error: memory allocation for command buffer.\n");
        return ERR_MEMORY;
    }

    while (1) {
        // Print prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // Read line
        char input_line[SH_CMD_MAX];
        if (fgets(input_line, SH_CMD_MAX, stdin) == NULL) {
            // EOF or read error.  We'll just exit the shell.
            printf("\n");
            break;
        }

        // Remove trailing newline, if present
        input_line[strcspn(input_line, "\n")] = '\0';

        // Clear out old parse data
        clear_cmd_buff(&cmd);

        // Parse into cmd_buff
        int parse_rc = build_cmd_buff(input_line, &cmd);
        if (parse_rc == WARN_NO_CMDS) {
            // user typed blank line or only spaces
            if (cmd.argc == 0) {
                printf(CMD_WARN_NO_CMD);  // "warning: no commands provided\n"
            }
            continue;
        } else if (parse_rc < 0 && parse_rc != WARN_NO_CMDS) {
            // Some other error
            // Could print details, e.g. memory error
            continue;
        }

        // We have at least one token
        // Check if it's a built-in
        Built_In_Cmds bic = match_command(cmd.argv[0]);
        if (bic == BI_CMD_EXIT) {
            // 'exit' => break out of loop
            break;
        }
        else if (bic == BI_CMD_CD) {
            // Syntax: cd [directory]
            // If no directory is provided, do nothing (per assignment spec)
            if (cmd.argc > 1) {
                // try chdir
                if (chdir(cmd.argv[1]) != 0) {
                    // If it fails, print a small error message
                    perror("cd");
                }
            }
            // done
        }
        else if (bic == BI_CMD_DRAGON) {
            // Extra credit references
            // e.g. printing a fancy ASCII dragon
            // Stub: do nothing or a placeholder
            printf("(dragon) Not implemented yet.\n");
        }
        else if (bic == BI_RC) {
            // Extra credit
            // Print return code of last command
            printf("(rc) Not implemented yet.\n");
        }
        else {
            // Not a built-in => external command
            pid_t pid = fork();
            if (pid < 0) {
                // fork error
                perror("fork");
                continue;
            } 
            else if (pid == 0) {
                // Child: exec the command
                execvp(cmd.argv[0], cmd.argv);
                // If we get here, exec failed
                // Per assignment spec: print an error message
                fprintf(stderr, CMD_ERR_EXECUTE); // "error: could not run external command\n"
                // or also a more detailed error:
                fprintf(stderr, ": %s\n", strerror(errno));
                _exit(127);  // typical 'command not found' exit code
            } 
            else {
                // Parent: wait for child
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }

    // Exited the loop, clean up
    free_cmd_buff(&cmd);
    return OK;
}


