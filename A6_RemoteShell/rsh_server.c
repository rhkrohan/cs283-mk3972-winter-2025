#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "dshlib.h"
#include "rshlib.h"

/**
 * start_server:
 *   - boots the server on the specified interface/port
 *   - uses fork() per connection (no threads here)
 */
int start_server(const char *ifaces, int port, int is_threaded) {
    (void)is_threaded;  // not used in this basic version
    int svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket;
    }

    // Loop accepting connections and forking a child for each.
    int rc = process_cli_requests(svr_socket);

    // Once the loop ends, shut down.
    stop_server(svr_socket);
    return rc;
}

/**
 * stop_server:
 *   - closes the main listening socket
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/**
 * boot_server:
 *   - creates the socket, binds to ifaces/port
 *   - listen() for incoming connections
 */
int boot_server(const char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    int enable = 1;
    ret = setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid interface address\n");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    ret = listen(svr_socket, 20);
    if (ret < 0) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/**
 * process_cli_requests:
 *   - loop accepting connections
 *   - fork a child to handle each one (exec_client_requests)
 *   - parent closes client socket, waits for next
 */
int process_cli_requests(int svr_socket) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addrlen);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(cli_socket);
            continue;
        } else if (pid == 0) {
            // Child process (handles this one client).
            close(svr_socket);
            int rc = exec_client_requests(cli_socket);
            close(cli_socket);
            _exit(rc);
        } else {
            // Parent process.
            close(cli_socket);
            // Reap any finished children (non-blocking).
            while (waitpid(-1, NULL, WNOHANG) > 0) {
                // do nothing
            }
        }
    }
    // Normally never reached unless you design a stop mechanism.
    return OK;
}

/**
 * exec_client_requests:
 *   - reads commands from cli_socket (one null-terminated at a time)
 *   - checks for built-in commands ("exit", "stop-server", "cd")
 *   - for external commands, fork/exec with dup2(cli_socket, ...)
 *     so that output is sent back to the client
 */
int exec_client_requests(int cli_socket) {
    char *cmd_buffer = malloc(RDSH_COMM_BUFF_SZ);
    if (!cmd_buffer) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        // 1) Read a single null-terminated command from client.
        memset(cmd_buffer, 0, RDSH_COMM_BUFF_SZ);
        int total_bytes = 0;
        while (1) {
            int chunk = recv(cli_socket,
                             cmd_buffer + total_bytes,
                             RDSH_COMM_BUFF_SZ - total_bytes, 0);
            if (chunk < 0) {
                perror("recv");
                free(cmd_buffer);
                return ERR_RDSH_COMMUNICATION;
            } else if (chunk == 0) {
                // Client closed the connection
                free(cmd_buffer);
                return OK;
            }

            total_bytes += chunk;
            // If we see a '\0', we've got a complete command.
            if (memchr(cmd_buffer, '\0', total_bytes)) {
                break;
            }
            // Avoid overfilling.
            if (total_bytes >= RDSH_COMM_BUFF_SZ) {
                cmd_buffer[RDSH_COMM_BUFF_SZ - 1] = '\0';
                break;
            }
        }

        // 2) Check built-in commands like "exit" / "stop-server".
        if (strcmp(cmd_buffer, "exit") == 0) {
            // End this client's session.
            break;
        } else if (strcmp(cmd_buffer, "stop-server") == 0) {
            // "stop-server" is a custom built-in that kills the entire server.
            // We typically let the parent interpret OK_EXIT to shut down the main loop.
            send_message_eof(cli_socket);  // Let client know we're done
            free(cmd_buffer);
            return OK_EXIT;
        } else if (strncmp(cmd_buffer, "cd ", 3) == 0) {
            // "cd" built-in
            char *path = cmd_buffer + 3;
            if (chdir(path) != 0) {
                perror("cd");
            }
            // We send an EOF so the client knows the command is finished.
            send_message_eof(cli_socket);
            continue;
        }

        // 3) If not a built-in, fork and exec the external command (or pipeline).
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            break;
        } else if (pid == 0) {
            // Child process to run the command.
            // Redirect stdout/stderr to the client socket.
            dup2(cli_socket, STDIN_FILENO);
            dup2(cli_socket, STDOUT_FILENO);
            dup2(cli_socket, STDERR_FILENO);

            // If you have pipeline logic:
            command_list_t clist;
            memset(&clist, 0, sizeof(clist));

            int rc = build_cmd_list(cmd_buffer, &clist);
            if (rc == OK && clist.num > 0) {
                execute_pipeline(&clist);
                free_cmd_list(&clist);
            }

            // Send EOF so client knows output is done.
            send_message_eof(cli_socket);

            _exit(0);
        } else {
            // Parent waits for the child to finish before reading the next command.
            int status;
            waitpid(pid, &status, 0);
        }
    }

    free(cmd_buffer);
    return OK;
}

/**
 * send_message_eof:
 *   - sends a single 0x04 byte to indicate end-of-output for a command
 */
int send_message_eof(int cli_socket) {
    // We just send 1 byte: RDSH_EOF_CHAR
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    if (sent_len != 1) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/**
 * send_message_string:
 *   - Optional helper if you want to send a string then EOF
 *   - Not actually used in the new approach, since we rely on child’s direct output
 */
int send_message_string(int cli_socket, char *buff) {
    int len = strlen(buff);
    int sent = send(cli_socket, buff, len, 0);
    if (sent != len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return send_message_eof(cli_socket);
}

/**
 * rsh_execute_pipeline:
 *   - example helper if you want to do pipeline logic specifically for remote usage
 *   - here we just call the local pipeline function
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    (void)cli_sock;  // not used in this simple example
    return execute_pipeline(clist);
}

/******** OPTIONAL BUILT-IN HANDLING (if you want a separate approach) ********/
Built_In_Cmds rsh_match_command(const char *input) {
    if (strcmp(input, "exit") == 0)         return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)       return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)           return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)  return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)           return BI_RC;
    return BI_NOT_BI;
}

Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = rsh_match_command(cmd->argv[0]);
    switch (ctype) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
        case BI_CMD_STOP_SVR:
            return BI_CMD_STOP_SVR;
        case BI_CMD_CD:
            if (cmd->argv[1]) {
                chdir(cmd->argv[1]);
            }
            return BI_EXECUTED;
        // etc. for other built-ins
        default:
            return BI_NOT_BI;
    }
}
