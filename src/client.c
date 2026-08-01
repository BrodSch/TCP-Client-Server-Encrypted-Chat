/* Program for the client interface */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#include "../samples/lite/chat_lite.h"
#include "../samples/standard/chat_standard.h"
#include "../samples/pro/chat_pro.h"
#include "../samples/heavy/chat_heavy.h"

int main(int argc, char* argv[]) {
    // Declare variables
    char* ip_no;
    int socketfd, port_no, status;
    struct sockaddr_in server_address;

    // Check if argument amount is satisfied
    if (argc < 3) {
        printf("Usage: ./client {hostname} {port}\n");
        exit(EXIT_FAILURE);
    }

    // Create client-side socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        printf("Client socket failed \n");
        exit(EXIT_FAILURE);
    }
    printf("Client socket created successfully\n");

    // Obtain client's input port number
    port_no = atoi(argv[2]);
    if (port_no <= 0 || port_no > 65535) {
        printf("Invalid port\n");
        exit(EXIT_FAILURE);
    }

    // Set all memory stored in server address as 0
    memset(&server_address, 0, sizeof(server_address));

    // Customise server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_no);

    // Check for ip address and store it
    ip_no = argv[1];
    if (strcmp(ip_no, "loopback") == 0) {
        printf("Loopback detected\n");
        server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0) {
        printf("Invalid address / not supported\n");
        return 1;
    }
    printf("Address authorised\n");

    // Connect client to server
    status = connect(socketfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if (status < 0) {
        printf("Client unable to connect\n");
        return 1;
    }
    printf("Connected to server\n");

    // Allow client and server to chat
    chat_client(socketfd);

    // Close socket
    close(socketfd);

    return 0;
}
