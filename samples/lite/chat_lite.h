// Declares a chat's functionality
#ifndef CHAT_LITE_H
#define CHAT_LITE_H

// Constants
#define MAX_SIZE 1024

// Prototypes
void chat_server(int client_server);
void chat_client(int socketfd);

#endif
