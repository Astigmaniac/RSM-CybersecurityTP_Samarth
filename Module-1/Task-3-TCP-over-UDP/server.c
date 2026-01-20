#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define TIMEOUT 2

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int expected_seq = 0;
    int connected = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    printf("Server running...\n");

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &addr_len);

        if (n < 0)
            continue;

        buffer[n] = '\0';

        char type[20];
        int seq, ack;
        char data[512];

        sscanf(buffer, "%[^|]|%d|%d|%[^\n]", type, &seq, &ack, data);

        /* shake it bebe */
        if (strcmp(type, "SYN") == 0) {
            printf("SYN received\n");
            sprintf(buffer, "SYN-ACK|0|%d|", seq + 1);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&client_addr, addr_len);
        }

        else if (strcmp(type, "ACK") == 0 && !connected) {
            connected = 1;
            printf("Connection established\n");
        }

        /* ---- DATA ---- */
        else if (strcmp(type, "DATA") == 0) {
            if (seq == expected_seq) {
                printf("Received: %s\n", data);
                expected_seq++;
            }

            sprintf(buffer, "ACK|0|%d|", expected_seq);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&client_addr, addr_len);
        }

        /* nini tem */
        else if (strcmp(type, "FIN") == 0) {
            printf("FIN received\n");
            sprintf(buffer, "ACK|0|%d|", seq + 1);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&client_addr, addr_len);
            break;
        }
    }

    close(sockfd);
    return 0;
}
