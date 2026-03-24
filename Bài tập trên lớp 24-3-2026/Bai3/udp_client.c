#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Khong tao duoc socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    int n;
    socklen_t len;

    printf("Go 'exit' de thoat.\n");

    while (1) {
        printf("> ");
        if (fgets(buffer, BUF_SIZE, stdin) == NULL) break;

        if (strncmp(buffer, "exit", 4) == 0) break;

        sendto(sockfd, (const char *)buffer, strlen(buffer), MSG_CONFIRM, 
              (const struct sockaddr *)&servaddr, sizeof(servaddr));

        len = sizeof(servaddr);
        n = recvfrom(sockfd, (char *)buffer, BUF_SIZE - 1, MSG_WAITALL, 
                    (struct sockaddr *)&servaddr, &len);
        buffer[n] = '\0';

        printf("Echoed: %s", buffer);
    }

    close(sockfd);
    return 0;
}