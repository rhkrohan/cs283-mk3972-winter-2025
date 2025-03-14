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
 
 int start_server(const char *ifaces, int port, int is_threaded) {
     (void)is_threaded;  // Mark as unused if not implemented.
     int svr_socket;
     int rc;
 
     svr_socket = boot_server(ifaces, port);
     if (svr_socket < 0) {
         return svr_socket;
     }
 
     rc = process_cli_requests(svr_socket);
 
     stop_server(svr_socket);
 
     return rc;
 }
 
 int stop_server(int svr_socket) {
     return close(svr_socket);
 }
 
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
 
 int process_cli_requests(int svr_socket) {
     int cli_socket;
     int rc = OK;
     while (1) {
         struct sockaddr_in client_addr;
         socklen_t addrlen = sizeof(client_addr);
         cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addrlen);
         if (cli_socket < 0) {
             perror("accept");
             rc = ERR_RDSH_COMMUNICATION;
             break;
         }
         pid_t pid = fork();
         if (pid < 0) {
             perror("fork");
             close(cli_socket);
             continue;
         } else if (pid == 0) {
             close(svr_socket);
             dup2(cli_socket, STDIN_FILENO);
             dup2(cli_socket, STDOUT_FILENO);
             dup2(cli_socket, STDERR_FILENO);
             rc = exec_client_requests(cli_socket);
             close(cli_socket);
             exit(rc);
         } else {
             close(cli_socket);
             while (waitpid(-1, NULL, WNOHANG) > 0);
         }
     }
     return rc;
 }
 
 int exec_client_requests(int cli_socket) {
     int io_size;
     command_list_t cmd_list;
     int rc = OK;
     char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
     if (io_buff == NULL) {
         return ERR_RDSH_SERVER;
     }
     cmd_list.num = 0;
     while (1) {
         memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
         io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ, 0);
         if (io_size < 0) {
             perror("recv");
             rc = ERR_RDSH_COMMUNICATION;
             break;
         } else if (io_size == 0) {
             rc = OK;
             break;
         }
         if (strcmp(io_buff, "exit") == 0) {
             rc = OK;
             break;
         }
         rc = send_message_string(cli_socket, io_buff);
         if (rc != OK) {
             rc = ERR_RDSH_COMMUNICATION;
             break;
         }
         rc = send_message_eof(cli_socket);
         if (rc != OK) {
             rc = ERR_RDSH_COMMUNICATION;
             break;
         }
     }
     free(io_buff);
     return rc;
 }
 
 int send_message_eof(int cli_socket) {
     int send_len = (int)sizeof(RDSH_EOF_CHAR);
     int sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);
     if (sent_len != send_len) {
         return ERR_RDSH_COMMUNICATION;
     }
     return OK;
 }
 
 int send_message_string(int cli_socket, char *buff) {
     int len = strlen(buff);
     int sent = send(cli_socket, buff, len, 0);
     if (sent != len) {
         return ERR_RDSH_COMMUNICATION;
     }
     return send_message_eof(cli_socket);
 }
 
 int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
     (void)cli_sock;  // Mark as unused in this basic example.
     return execute_pipeline(clist);
 }
 
 /******** OPTIONAL BUILT-IN HANDLING FUNCTIONS ********/
 Built_In_Cmds rsh_match_command(const char *input) {
     if (strcmp(input, "exit") == 0)
         return BI_CMD_EXIT;
     if (strcmp(input, "dragon") == 0)
         return BI_CMD_DRAGON;
     if (strcmp(input, "cd") == 0)
         return BI_CMD_CD;
     if (strcmp(input, "stop-server") == 0)
         return BI_CMD_STOP_SVR;
     if (strcmp(input, "rc") == 0)
         return BI_RC;
     return BI_NOT_BI;
 }
 
 Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
     Built_In_Cmds ctype = rsh_match_command(cmd->argv[0]);
     switch (ctype) {
         // case BI_CMD_DRAGON:
         //     print_dragon();
         //     return BI_EXECUTED;
         case BI_CMD_EXIT:
             return BI_CMD_EXIT;
         case BI_CMD_STOP_SVR:
             return BI_CMD_STOP_SVR;
         case BI_RC:
             return BI_RC;
         case BI_CMD_CD:
             chdir(cmd->argv[1]);
             return BI_EXECUTED;
         default:
             return BI_NOT_BI;
     }
 }