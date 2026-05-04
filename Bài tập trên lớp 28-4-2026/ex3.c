#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS 256
#define MAX_TOPICS 10

typedef struct {
    int fd;
    char topics[MAX_TOPICS][64];
    int num_topics;
} Client;

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    Client clients[MAX_CLIENTS];
    int n_clients = 0;
    
    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);
    int max_fd = listener;

    char buf[1024];

    printf("Pub/Sub Server dang chay tai cong 9000...\n");

    while(1) {
        fdtest = fdread;
        select(max_fd + 1, &fdtest, NULL, NULL, NULL);

        if (FD_ISSET(listener, &fdtest)) {
            int new_fd = accept(listener, NULL, NULL);
            if (new_fd >= 0) {
                if (n_clients < MAX_CLIENTS) {
                    clients[n_clients].fd = new_fd;
                    clients[n_clients].num_topics = 0;
                    n_clients++;
                    
                    FD_SET(new_fd, &fdread);
                    if (new_fd > max_fd) max_fd = new_fd;
                } else {
                    close(new_fd);
                }
            }
        }

        for (int i = 0; i < n_clients; i++) {
            if (FD_ISSET(clients[i].fd, &fdtest)) {
                int ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (ret <= 0) {
                    FD_CLR(clients[i].fd, &fdread);
                    close(clients[i].fd);
                    clients[i] = clients[n_clients - 1];
                    n_clients--;
                    i--;
                } else {
                    buf[ret] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0;

                    char cmd[16], topic[64], msg[512];
                    int recv = sscanf(buf, "%s %s %[^\n]", cmd, topic, msg);

                    if (recv >= 2 && strcmp(cmd, "SUB") == 0) {
                        if (clients[i].num_topics < MAX_TOPICS) {
                            strcpy(clients[i].topics[clients[i].num_topics], topic);
                            clients[i].num_topics++;
                            char *ack = "Da dang ky chu de thanh cong!\n";
                            send(clients[i].fd, ack, strlen(ack), 0);
                        }
                    } 
                    else if (recv == 3 && strcmp(cmd, "PUB") == 0) {
                        char out_buf[1024];
                        snprintf(out_buf, sizeof(out_buf), "[%s]: %s\n", topic, msg);
                        
                        for (int j = 0; j < n_clients; j++) {
                            for (int k = 0; k < clients[j].num_topics; k++) {
                                if (strcmp(clients[j].topics[k], topic) == 0) {
                                    send(clients[j].fd, out_buf, strlen(out_buf), 0);
                                    break; 
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}