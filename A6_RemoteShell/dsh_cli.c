#include <stdio.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include "dshlib.h"
 #include "rshlib.h"
 
 // Forward declarations from remote files.
 int exec_remote_cmd_loop(const char *server_ip, int port);
 int start_server(const char *bind_iface, int port, int is_threaded);
 
 void usage() {
     printf("Usage: %s [-c | -s] [-i IP] [-p PORT]\n", "./dsh");
     exit(1);
 }
 
 int main(int argc, char *argv[]) {
     enum { LOCAL, CLIENT, SERVER } mode = LOCAL;
     char *ip = NULL;
     int port = 0;
     int opt;
     while ((opt = getopt(argc, argv, "csi:p:h")) != -1) {
         switch(opt) {
             case 'c':
                 mode = CLIENT;
                 break;
             case 's':
                 mode = SERVER;
                 break;
             case 'i':
                 ip = optarg;
                 break;
             case 'p':
                 port = atoi(optarg);
                 break;
             case 'h':
             default:
                 usage();
         }
     }
     if (port == 0)
         port = RDSH_DEF_PORT;
     if (ip == NULL) {
         if (mode == CLIENT)
             ip = RDSH_DEF_CLI_CONNECT;
         else if (mode == SERVER)
             ip = RDSH_DEF_SVR_INTFACE;
     }
     
     if (mode == LOCAL) {
         // Execute local command loop; note that the "localmode" output is now handled inside the loop.
         int rc = exec_local_cmd_loop();
         printf("cmd loop returned %d\n", rc);
         return 0;
     } else if (mode == CLIENT) {
         return exec_remote_cmd_loop(ip, port);
     } else if (mode == SERVER) {
         return start_server(ip, port, 0);
     }
     return 0;
 }