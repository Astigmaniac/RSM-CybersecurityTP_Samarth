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
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    int seq = 0;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* ---- HANDSHAKE ---- */

    sprintf(buffer, "SYN|%d|0|", seq);
    sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&server_addr, addr_len);

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&server_addr, &addr_len);

        if (n < 0) {
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, addr_len);
            continue;
        }

        buffer[n] = '\0';

        if (strncmp(buffer, "SYN-ACK", 7) == 0) {
            seq++;
            sprintf(buffer, "ACK|%d|1|", seq);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, addr_len);
            break;
        }
    }

    printf("Connection established\n");

    /*  Data trans */

    char *messages[] = {"Hello", "This", "is", "TCP", "over", "UDP"};
    int total = 6;

    for (int i = 0; i < total; i++) {
        while (1) {
            sprintf(buffer, "DATA|%d|0|%s", seq, messages[i]);
            sendto(sockfd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, addr_len);

            int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                             (struct sockaddr *)&server_addr, &addr_len);

            if (n < 0) {
                printf("Retransmitting: %s\n", messages[i]);
                continue;
            }

            buffer[n] = '\0';
            int ack;
            sscanf(buffer, "ACK|0|%d|", &ack);

            if (ack == seq + 1) {
                seq++;
                break;
            }
        }
    }

/*Term*/

    sprintf(buffer, "FIN|%d|0|", seq);
    sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&server_addr, addr_len);

    close(sockfd);
    return 0;
}
