#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Dinh dang: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }

    int listener = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in src_addr = {0}, dest_addr = {0};
    
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(atoi(argv[1]));
    src_addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&src_addr, sizeof(src_addr));

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(atoi(argv[3]));
    dest_addr.sin_addr.s_addr = inet_addr(argv[2]);

    fd_set readfds;
    char buffer[1024];

    printf("Da cau hinh xong! Bat dau chat ('exit' thoat):\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(listener, &readfds);

        select(listener + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(listener, &readfds)) {
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);
            int n = recvfrom(listener, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&sender_addr, &sender_len);
            if (n > 0) {
                buffer[n] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;
                printf("[%s:%d]: %s\n", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port), buffer);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;
                
                if (strcmp(buffer, "exit") == 0) break;
                
                if (strlen(buffer) > 0) {
                    sendto(listener, buffer, strlen(buffer), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                }
            }
        }
    }

    close(listener);
    return 0;
}