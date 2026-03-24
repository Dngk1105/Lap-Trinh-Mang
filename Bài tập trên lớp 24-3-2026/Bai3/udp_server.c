#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Khong tao duoc socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind thất bại");
        exit(EXIT_FAILURE);
    }

    printf("Dang cho tai cong %d...\n", PORT);

    socklen_t len;
    int n;
    char buffer[BUF_SIZE];

    while (1) {
        len = sizeof(cliaddr);
        
        n = recvfrom(sockfd, (char *)buffer, BUF_SIZE - 1, MSG_WAITALL, 
                    (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';
        
        printf("Nhan tu nguoi dung [%s:%d]: %s", 
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buffer);
        
        sendto(sockfd, (const char *)buffer, n, MSG_CONFIRM, 
              (const struct sockaddr *)&cliaddr, len);
    }

    close(sockfd);
    return 0;
}