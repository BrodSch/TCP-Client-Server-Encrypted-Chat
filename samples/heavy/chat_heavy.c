// Libraries
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "chat_heavy.h"

// Define constants
#define MAX_SIZE 1024
#define DATE_TIME_SIZE 64
#define COMBO_SIZE 256
#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))
#define QR(a, b, c, d) \
    a += b; d ^= a; d = ROTL32(d, 16); \
    c += d; b ^= c; b = ROTL32(b, 12); \
    a += b; d ^= a; d = ROTL32(d, 8);  \
    c += d; b ^= c; b = ROTL32(b, 7);

// Encryption function

/*
    You can customise your encryption algorithm here,
    and place it in the samples directory.
*/

void chacha20_block(uint32_t out[16], const uint32_t key[8], const uint32_t nonce[3], uint32_t counter) {
    int i;
    out[0] = 0x61707865; out[1] = 0x3320646e; out[2] = 0x79622d32; out[3] = 0x6b206574;
    for (i = 0; i < 8; i++) out[4 + i] = key[i];
    out[12] = counter;
    out[13] = nonce[0]; out[14] = nonce[1]; out[15] = nonce[2];
    uint32_t x[16];
    memcpy(x, out, sizeof(x));
    for (i = 0; i < 20; i += 2) {
        QR(x[0], x[4], x[8], x[12]);  QR(x[1], x[5], x[9], x[13]);
        QR(x[2], x[6], x[10], x[14]); QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]); QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8], x[13]);  QR(x[3], x[4], x[9], x[14]);
    }
    for (i = 0; i < 16; i++) out[i] += x[i];
}

void poly1305_auth(uint8_t tag[16], const uint8_t *ciphertext, size_t len, const uint8_t key[32]) {
    uint64_t r0, r1, r2, s1, s2;
    uint64_t h0 = 0, h1 = 0, h2 = 0;
    uint64_t c;

    // Clamp the 'r' key component according to Poly1305 standard specification
    r0 = ((uint64_t*)key)[0] & 0x0ffffffc0fffffffULL;
    r1 = ((uint64_t*)key)[1] & 0x0ffffffc0ffffffcULL;
    r2 = r1 >> 32; r1 &= 0xffffffffULL;
    s1 = r1 * 5; s2 = r2 * 5;

    while (len > 0) {
        size_t block_len = (len > 16) ? 16 : len;
        uint8_t block[17] = {0};
        memcpy(block, ciphertext, block_len);
        // Pad byte indicator bit
        block[block_len] = 1;

        uint64_t b0 = *(uint64_t*)&block[0];
        uint64_t b1 = *(uint64_t*)&block[8];
        uint64_t b2 = block[16];

        h0 += b0; h1 += b1; h2 += b2;

        // Modular arithmetic calculation modulo (2^130 - 5)
        uint64_t d0 = h0 * r0 + h1 * s2 + h2 * s1;
        uint64_t d1 = h0 * r1 + h1 * r0 + h2 * s2;
        uint64_t d2 = h0 * r2 + h1 * r1 + h2 * r0;

        c = d0 >> 32; h0 = d0 & 0xffffffffULL; d1 += c;
        c = d1 >> 32; h1 = d1 & 0xffffffffULL; d2 += c;
        c = d2 >> 32; h2 = d2 & 0x3ULL;
        h0 += c * 5;

        ciphertext += block_len;
        len -= block_len;
    }

    // Mix in the 's' key component and store the tag output buffer
    uint64_t s_val0 = *(uint64_t*)&key[16];
    uint64_t s_val1 = *(uint64_t*)&key[24];
    *(uint64_t*)&tag[0] = h0 + (h1 << 32) + s_val0;
    *(uint64_t*)&tag[8] = (h1 >> 32) + (h2 << 32) + s_val1;
}

void encrypt(char* input, size_t input_len) {
    uint32_t master_key[8] = {0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c, 0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c};
    uint32_t nonce[3] = {0, 0, 0};
    uint32_t block[16];

    // Generate a unique 32-byte subkey specifically for Poly1305 tag processing
    chacha20_block(block, master_key, nonce, 0);
    uint8_t poly_key[32];
    memcpy(poly_key, block, 32);

    // Encrypt the plain chat message buffer in-place using standard ChaCha20 stream
    size_t idx = 0;
    size_t remaining = input_len;
    uint32_t block_counter = 1;
    while (remaining > 0) {
        chacha20_block(block, master_key, nonce, block_counter++);
        size_t chunk = (remaining > 64) ? 64 : remaining;
        uint8_t* bytes = (uint8_t*)block;
        for (size_t j = 0; j < chunk; j++) {
            input[idx] ^= bytes[j];
            idx++;
        }
        remaining -= chunk;
    }

    // Generate a 16-byte Poly1305 authentication tag over the newly encrypted data
    uint8_t auth_tag[16];
    poly1305_auth(auth_tag, (uint8_t*)input, input_len, poly_key);

    // Append the 16-byte tag directly to the end of the encrypted buffer string
    memcpy(&input[input_len], auth_tag, 16);
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

    encrypt(new_input, len);

    char hex_output[MAX_SIZE * 2 + 1] = { 0 };
    for(size_t i = 0; i < (len + 16); i++) {
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
