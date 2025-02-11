#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "dshlib.h"

#ifdef USE_COMPRESSED_DRAGON
#include <zlib.h>
#endif

/* Forward declaration for printing the dragon */
void print_dragon(void);

int main(void)
{
    char cmd_buff[ARG_MAX];

    while (1)
    {
        /* Print the shell prompt */
        printf("%s", SH_PROMPT);

        /* Read one line of input */
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        /* Remove the trailing newline (if present) */
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        /* Check for built-in commands.
         * We first trim leading whitespace.
         */
        char *trimmed = cmd_buff;
        while (*trimmed && isspace((unsigned char)*trimmed))
            trimmed++;

        if (strcmp(trimmed, "exit") == 0) {
            /* Exit the shell with code 0 */
            break;
        }
        if (strcmp(trimmed, "dragon") == 0) {
            /* Extra Credit: Print the Drexel dragon */
            print_dragon();
            continue;
        }

        /* Build the command list from the input string */
        command_list_t cmd_list;
        cmd_list.num_commands = 0;
        if (build_cmd_list(cmd_buff, &cmd_list) != 0) {
            /* An error occurred during parsing (for example, too many commands) */
            continue;
        }
        if (cmd_list.num_commands == 0) {
            printf("warning: no commands provided\n");
        } else {
            /* Print the parsed command line details */
            printf("PARSED COMMAND LINE - TOTAL COMMANDS %d\n", cmd_list.num_commands);
            for (int i = 0; i < cmd_list.num_commands; i++) {
                printf("<%d> %s", i + 1, cmd_list.commands[i].name);
                if (cmd_list.commands[i].argc > 0) {
                    printf(" [");
                    for (int j = 0; j < cmd_list.commands[i].argc; j++) {
                        printf("%s", cmd_list.commands[i].args[j]);
                        if (j < cmd_list.commands[i].argc - 1)
                            printf(" ");
                    }
                    printf("]");
                }
                printf("\n");
            }
        }
        free_cmd_list(&cmd_list);
    }
    return 0;
}

/*
 * print_dragon()
 *
 * Implements the extra credit command. When the user enters the command "dragon"
 * the Drexel dragon is printed in ASCII art.
 *
 * Two implementations are provided:
 * 1. If USE_COMPRESSED_DRAGON is defined, the dragon is stored as compressed binary
 *    data (using zlib compression) and is decompressed at runtime.
 * 2. Otherwise, the ASCII art is printed directly.
 */
void print_dragon(void)
{
#ifdef USE_COMPRESSED_DRAGON
    /* The compressed binary representation is defined below. */
    unsigned long decompressed_size = 5000;  /* Adjust if necessary */
    char *decompressed = malloc(decompressed_size);
    if (!decompressed) {
        fprintf(stderr, "Memory allocation error\n");
        return;
    }
    int ret = uncompress((unsigned char *)decompressed, &decompressed_size,
                         compressed_dragon, compressed_dragon_len);
    if (ret != Z_OK) {
        fprintf(stderr, "Decompression error: %d\n", ret);
        free(decompressed);
        return;
    }
    printf("%s", decompressed);
    free(decompressed);
#else
    /* Uncompressed version of the Drexel dragon ASCII art */
    printf("                                                                        @%%%%                       \n");
    printf("                                                                     %%%%%%                         \n");
    printf("                                                                    %%%%%%                          \n");
    printf("                                                                 % %%%%%%%           @              \n");
    printf("                                                                %%%%%%%%%%        %%%%%%%           \n");
    printf("                                       %%%%%%%  %%%%@         %%%%%%%%%%%%@    %%%%%%  @%%%%        \n");
    printf("                                  %%%%%%%%%%%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%%%%%%%%%%%%          \n");
    printf("                                %%%%%%%%%%%%%%%%%%%%%%%%%%   %%%%%%%%%%%% %%%%%%%%%%%%%%%           \n");
    printf("                               %%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%     %%%            \n");
    printf("                             %%%%%%%%%%%%%%%%%%%%%%%%%%%%@ @%%%%%%%%%%%%%%%%%%        %%            \n");
    printf("                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%                \n");
    printf("                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              \n");
    printf("                            %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%@%%%%%%@              \n");
    printf("      %%%%%%%%@           %%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%      %%                \n");
    printf("    %%%%%%%%%%%%%         %%@%%%%%%%%%%%%           %%%%%%%%%%% %%%%%%%%%%%%      @%                \n");
    printf("  %%%%%%%%%%   %%%        %%%%%%%%%%%%%%            %%%%%%%%%%%%%%%%%%%%%%%%                        \n");
    printf(" %%%%%%%%%       %         %%%%%%%%%%%%%             %%%%%%%%%%%%@%%%%%%%%%%%                       \n");
    printf("%%%%%%%%%@                % %%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%                     \n");
    printf("%%%%%%%%@                 %%@%%%%%%%%%%%%            @%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  \n");
    printf("%%%%%%%@                   %%%%%%%%%%%%%%%           %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              \n");
    printf("%%%%%%%%%%                  %%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%      %%%%  \n");
    printf("%%%%%%%%%@                   @%%%%%%%%%%%%%%         %%%%%%%%%%%%@ %%%% %%%%%%%%%%%%%%%%%   %%%%%%%%\n");
    printf("%%%%%%%%%%                  %%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%      %%%%%%%%%%%%%%%%%% %%%%%%%%%\n");
    printf("%%%%%%%%%@%%@                %%%%%%%%%%%%%%%%@       %%%%%%%%%%%%%%     %%%%%%%%%%%%%%%%%%%%%%%%  %%\n");
    printf(" %%%%%%%%%%                  % %%%%%%%%%%%%%%@        %%%%%%%%%%%%%%   %%%%%%%%%%%%%%%%%%%%%%%%%% %%\n");
    printf("  %%%%%%%%%%%%  @           %%%%%%%%%%%%%%%%%%        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%  %%% \n");
    printf("   %%%%%%%%%%%%% %%  %  %@ %%%%%%%%%%%%%%%%%%          %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%% \n");
    printf("    %%%%%%%%%%%%%%%%%% %%%%%%%%%%%%%%%%%%%%%%           @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%    %%%%%%% \n");
    printf("     %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%              %%%%%%%%%%%%%%%%%%%%%%%%%%%%        %%%   \n");
    printf("      @%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                  %%%%%%%%%%%%%%%%%%%%%%%%%               \n");
    printf("        %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%%%%%%  %%%%%%%          \n");
    printf("           %%%%%%%%%%%%%%%%%%%%%%%%%%                           %%%%%%%%%%%%%%%  @%%%%%%%%%         \n");
    printf("              %%%%%%%%%%%%%%%%%%%%           @%@%                  @%%%%%%%%%%%%%%%%%%   %%%        \n");
    printf("                  %%%%%%%%%%%%%%%        %%%%%%%%%%                    %%%%%%%%%%%%%%%    %         \n");
    printf("                %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%                      %%%%%%%%%%%%%%            \n");
    printf("                %%%%%%%%%%%%%%%%%%%%%%%%%%  %%%% %%%                      %%%%%%%%%%  %%%@          \n");
    printf("                     %%%%%%%%%%%%%%%%%%% %%%%%% %%                          %%%%%%%%%%%%%@          \n");
    printf("                                                                                 %%%%%%%@       \n");
#endif
}

#ifdef USE_COMPRESSED_DRAGON

#include <zlib.h>
static const unsigned char compressed_dragon[] = {
    /* Placeholder data – in a real implementation, replace this with the actual 
       gzipped binary data for the Drexel dragon. */
    0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    /* ... additional compressed bytes ... */
};
static const unsigned int compressed_dragon_len = sizeof(compressed_dragon);
#endif

