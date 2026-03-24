#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Khong ket noi duoc!");
        return 1;
    }

    printf("Da ket noi\n");
    printf("Go 'exit' de thoat:\n");

    char buffer[1024];

    while (1) {
        printf("> ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        size_t len = strlen(buffer);
        if (len > 0) {
            send(sock, buffer, len, 0);
            printf("Da gui %d bytes\n", len);
        }
    }

    close(sock);
    printf("Dong ket noi\n");
    return 0;
}