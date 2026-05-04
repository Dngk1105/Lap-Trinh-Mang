#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>

#define MAX_CLIENTS 512

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        return 1;
    }

    if (listen(listener, 5) != 0) {
        return 1;
    }

    int clients[MAX_CLIENTS];
    int nClients = 0;

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int max_fd = listener;
    char buf[1024];

    while(1) {
        fdtest = fdread; 
        int ret = select(max_fd + 1, &fdtest, NULL, NULL, NULL);
        if (ret < 0) {
            return 1;
        }

        if (FD_ISSET(listener, &fdtest)) { 
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd >= 0) {
                if (nClients < MAX_CLIENTS) {
                    clients[nClients] = client_fd;
                    nClients++;

                    FD_SET(client_fd, &fdread);
                    if (max_fd < client_fd) max_fd = client_fd;

                    char msg[256];
                    snprintf(msg, sizeof(msg), "Xin chao. Hien co %d clients dang ket noi.\n", nClients);
                    send(client_fd, msg, strlen(msg), 0);
                } else {
                    close(client_fd);
                }
            }
        }

        for (int i = 0; i < nClients; i++) {
            int client_fd = clients[i];
            
            if (FD_ISSET(client_fd, &fdtest)) {
                ret = recv(client_fd, buf, sizeof(buf) - 1, 0);

                if (ret <= 0) {
                    FD_CLR(client_fd, &fdread);
                    close(client_fd);
                    clients[i] = clients[nClients - 1];
                    nClients--;
                    i--;
                } else {
                    buf[ret] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0; 

                    if (strcmp(buf, "exit") == 0) {
                        char *msg = "Tam biet!\n";
                        send(client_fd, msg, strlen(msg), 0);
                        
                        FD_CLR(client_fd, &fdread);
                        close(client_fd);
                        clients[i] = clients[nClients - 1];
                        nClients--;
                        i--;
                    } else {
                        for (int j = 0; buf[j] != '\0'; j++) {
                            if (isalpha(buf[j])) {
                                if (buf[j] == 'z') buf[j] = 'a';
                                else if (buf[j] == 'Z') buf[j] = 'A';
                                else buf[j] = buf[j] + 1;
                            } else if (isdigit(buf[j])) {
                                buf[j] = '9' - (buf[j] - '0');
                            }
                        }
                        strcat(buf, "\n");
                        send(client_fd, buf, strlen(buf), 0);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}