/* Program for the server interface */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include "../samples/lite/chat_lite.h"
#include "../samples/standard/chat_standard.h"
#include "../samples/pro/chat_pro.h"
#include "../samples/heavy/chat_heavy.h"

int main(int argc, char* argv[]) {
    // Declaration
    int server_sock, client_sock, port_no;
    int option = 1;
    struct sockaddr_in server_address;
    socklen_t addrlen = sizeof(server_address);

    // Check if correct number of arguments
    if (argc != 2) {
        printf("Usage: ./server {port}\n");
        exit(EXIT_FAILURE);
    }

    // Create a socket from the server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Server socket Failed");
        exit(EXIT_FAILURE);
    }
    printf("Server socket created successfully\n");

    // Customising socket options to eliminate any corner cases relating to the socket usage
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))) {
        perror("Setsockopt SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }

    // Designate a port number
    port_no = atoi(argv[1]);
    if (port_no <= 0 || port_no > 65535) {
        printf("Invalid port\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port_no);

    // Send the evaluation to the OS kernel via bind()
    if (bind(server_sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Server bind Failed");
        exit(EXIT_FAILURE);
    }
    printf("Server socket successfully binded\n");

    // Listen for a response from the client
    if (listen(server_sock, 5) < 0) {
        // Check
        perror("Server listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", port_no);

    client_sock = accept(server_sock, (struct sockaddr*)&server_address, &addrlen);
    if (client_sock < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    };
    printf("Client connected\n");

    // TODO: Add chat
    chat_server(client_sock);

    // Close sockets
    close(client_sock);
    close(server_sock);

    return 0;
}
