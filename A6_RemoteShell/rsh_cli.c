#include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <errno.h>
 #include "rshlib.h"
 
 int exec_remote_cmd_loop(const char *server_ip, int port) {
     int sock = socket(AF_INET, SOCK_STREAM, 0);
     if (sock < 0) {
         perror("socket");
         exit(1);
     }
     struct sockaddr_in serv_addr;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port = htons(port);
     if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
         fprintf(stderr, "Invalid address or address not supported\n");
         exit(1);
     }
     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("connect");
         exit(1);
     }
     char send_buf[ARG_MAX];
     char recv_buf[RDSH_COMM_BUFF_SZ];
     while (1) {
         printf("dsh3> ");
         fflush(stdout);
         if (fgets(send_buf, sizeof(send_buf), stdin) == NULL)
             break;
         send_buf[strcspn(send_buf, "\n")] = '\0';
         if (strcmp(send_buf, "exit") == 0)
             break;
         int send_len = strlen(send_buf) + 1;
         if (send(sock, send_buf, send_len, 0) != send_len) {
             perror("send");
             break;
         }
         int done = 0;
         while (!done) {
             int bytes_received = recv(sock, recv_buf, RDSH_COMM_BUFF_SZ, 0);
             if (bytes_received < 0) {
                 perror("recv");
                 done = 1;
             } else if (bytes_received == 0) {
                 done = 1;
             } else {
                 if ((char)recv_buf[bytes_received - 1] == RDSH_EOF_CHAR) {
                     recv_buf[bytes_received - 1] = '\0';
                     done = 1;
                 }
                 printf("%s", recv_buf);
                 fflush(stdout);
             }
         }
     }
     close(sock);
     return 0;
 }