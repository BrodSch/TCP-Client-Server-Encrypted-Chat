// Libraries
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "chat_pro.h"

// Define constants
#define MAX_SIZE 1024
#define DATE_TIME_SIZE 64
#define COMBO_SIZE 256

// Encryption function

/*
    You can customise your encryption algorithm here,
    and place it in the samples directory.
*/

void encrypt(char* input, size_t input_len, const char* timestamp) {
    // Base 32-byte master key pad sequence
    uint8_t crypto_key[32] = {
        0x5A, 0xA5, 0x1F, 0x2E, 0x3D, 0x4C, 0x5B, 0x6A,
        0x79, 0x88, 0x97, 0xA6, 0xB5, 0xC4, 0xD3, 0xE2,
        0xF1, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
        0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE
    };

    // Generate a unique hash variation based on the exact timestamp string (DJB2 Hash)
    unsigned long time_hash = 5381;
    while (*timestamp) {
        time_hash = ((time_hash << 5) + time_hash) + (unsigned char)*timestamp++;
    }

    // Permute/scramble the base key using the derived timestamp hash token
    for (int k = 0; k < 32; k++) {
        crypto_key[k] ^= (uint8_t)(time_hash >> (k % 4 * 8));
        crypto_key[k] += (uint8_t)(k * 7); // Extra bit dispersion pass
    }

    // Perform the cycling in-place cipher operation across your data bytes
    for (size_t i = 0; i < input_len; i++) {
        input[i] = input[i] ^ crypto_key[i % 32];
    }
}

// Display function
void display(FILE* log, const char* sender, const char* receiver, char* input) {
    if (log == NULL) {
        printf("Error opening file\n");
        return;
    }

    size_t len;
    time_t raw_time = time(NULL);
    struct tm* info = localtime(&raw_time);

    char timestamp[DATE_TIME_SIZE];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", info);

    char new_input[MAX_SIZE];
    strcpy(new_input, input);
    new_input[sizeof(new_input) - 1] = '\0';
    new_input[strcspn(new_input, "\r\n")] = '\0';

    len = strlen(new_input);

    encrypt(new_input, len, timestamp);

    char hex_output[MAX_SIZE * 2 + 1] = { 0 };
    for(size_t i = 0; i < len; i++) {
        sprintf(&hex_output[i * 2], "%02X", (unsigned char)new_input[i]);
    }

    /*

    For future reference, change the placeholder for new_input
    and place new_input within the parameters of the encrypt function.

    */

    fprintf(log, "%s, %s, %s, \"%s\"\n", timestamp, sender, receiver, hex_output);
    fflush(log);
}

// Chat function for server.c
void chat_server(int client_sock) {
    int max_fd;
    char buffer[MAX_SIZE];
    char response[MAX_SIZE];
    char command[COMBO_SIZE];
    char filename[DATE_TIME_SIZE] = { 0 };
    ssize_t bytes_read;
    time_t start_time = time(NULL);
    struct tm* start_info = localtime(&start_time);

    if (client_sock > STDIN_FILENO) {
        max_fd = client_sock;
    }
    else {
        max_fd = STDIN_FILENO;
    }

    if (strftime(filename, sizeof(filename), "%Y%m%d%H%M%S.csv", start_info) == 0) {
        printf("Error: Failed to format filename.\n");
        return;
    }

    FILE* log = fopen(filename, "a");
    if (log == NULL) {
        printf("Couldn't open chat log file\n");
        return;
    }

    fprintf(log, "Timestamp, Sender, Receiver, Log\n");

    fd_set read_fds;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            break;
        }

        // Input from client
        if (FD_ISSET(client_sock, &read_fds)) {
            memset(buffer, 0, MAX_SIZE);

            bytes_read = read(client_sock, buffer, MAX_SIZE - 1);
            if (bytes_read < 0) {
                perror("Read failed");
                break;
            }
            else if (bytes_read == 0) {
                printf("Client disconnected abruptly\n");
                break;
            }

            buffer[bytes_read] = '\0';

            char placeholder[MAX_SIZE];
            strcpy(placeholder, buffer);

            placeholder[strcspn(placeholder, "\r\n")] = '\0';
            if (strcmp(placeholder, "exit") == 0) {
                printf("Client disconnected successfully\n");
                break;
            }

            printf("\nClient: %s", buffer);
            printf("Server: ");

            fflush(stdout);

            display(log, "Client", "Server", buffer);
        }

        // Input from keyboard
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(response, 0, MAX_SIZE);

            if (fgets(response, MAX_SIZE, stdin) == NULL) {
                break;
            }

            write(client_sock, response, strlen(response));

            char new_response[MAX_SIZE];
            strcpy(new_response, response);
            new_response[strcspn(new_response, "\r\n")] = '\0';

            if (strcmp(new_response, "exit") == 0) {
                printf("Server disconnected successfully\n");
                break;
            }

            display(log, "Server", "Client", response);

            printf("Server: ");
            fflush(stdout);

        }
    }

    fclose(log);

    snprintf(command, sizeof(command), "python3 src/transfer.py %s", filename);

    system(command);
}

// Chat function for client.c
void chat_client(int socketfd) {
    int max_fd;
    char buffer[MAX_SIZE];
    char message[MAX_SIZE];
    ssize_t bytes_read;

    if (socketfd > STDIN_FILENO) {
        max_fd = socketfd;
    }
    else {
        max_fd = STDIN_FILENO;
    }

    fd_set read_fds;

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(socketfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select failed");
            break;
        }

        // Input from server
        if (FD_ISSET(socketfd, &read_fds)) {
            memset(buffer, 0, MAX_SIZE);

            bytes_read = read(socketfd, buffer, MAX_SIZE - 1);
            if (bytes_read < 0) {
                perror("Read failed");
                break;
            }
            else if (bytes_read == 0) {
                printf("Server disconnected abruptly\n");
                break;
            }
            else if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
            }

            char placeholder[MAX_SIZE];
            strcpy(placeholder, buffer);

            placeholder[strcspn(placeholder, "\r\n")] = '\0';
            if (strcmp(placeholder, "exit") == 0) {
                printf("Server disconnected successfully\n");
                break;
            }

            printf("\nServer: %s", buffer);
            printf("Client: ");

            fflush(stdout);
        }

        // Input from keyboard
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(message, 0, MAX_SIZE);
            if (fgets(message, MAX_SIZE, stdin) == NULL) {
                break;
            }

            write(socketfd, message, strlen(message));

            message[strcspn(message, "\r\n")] = '\0';
            if (strcmp(message, "exit") == 0) {
                printf("Client disconnected successfully\n");
                break;
            }

            printf("Client: ");
            fflush(stdout);
        }
    }
}
