#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>

#define MAX_CLIENTS 512

struct Client {
    int client_fd;
    char client_name[256];
    int room_joined;
};

void removeClient(struct Client *clients, int *nClients, int index) {
    if (index < *nClients - 1) {
        clients[index] = clients[*nClients - 1]; 
    }
    (*nClients)--;
}

void sendToAll(struct Client *clients, int *nClients, char *buf, int send_client_fd, char *send_client_name) {
    char newbuf[768];
    snprintf(newbuf, sizeof(newbuf), "%s: %s\n", send_client_name, buf);
    
    for (int i = 0; i < *nClients; i++) {
        if (clients[i].client_fd != send_client_fd && clients[i].room_joined) {
            send(clients[i].client_fd, newbuf, strlen(newbuf), 0);
        }
    }
}

int processJoinRequest(char *buf, int client_fd, char *extracted_name) {
    char id_check[64];
    snprintf(id_check, sizeof(id_check), "client_%d: ", client_fd);
    
    if (strncmp(buf, id_check, strlen(id_check)) == 0) {
        char *name = buf + strlen(id_check);
        
        if (strlen(name) == 0 || strchr(name, ' ') != NULL) {
            return 0;
        }
        
        strcpy(extracted_name, name);
        return 1;
    }
    return 0;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket error");
        return 1;
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) != 0) {
        perror("setsockopt failed!");
        return 1;
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed!");
        close(listener);
        return 1;
    }

    if (listen(listener, 5) != 0) {
        perror("listen failed ");
        close(listener);
        return 1;
    }

    printf("Server dang lang nghe tai cong 8080 \n");

    // Danh sach Clients
    struct Client clients[MAX_CLIENTS];
    int nClients = 0;

    fd_set fdread, fdtest;
    
    // Gan listener vao mang bit fd_set de theo doi cac ket noi oi
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int max_fd = listener;
    struct timeval tv;
    char buf[256];
    
    while(1) {
        fdtest = fdread; 

        // timeout
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        int ret = select(max_fd + 1, &fdtest, NULL, NULL, &tv);
        if (ret < 0) {
            perror("loi select");
            return 1;
        } else if (ret == 0) {
            continue;
        }

        if (FD_ISSET(listener, &fdtest)) { 
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd < 0) continue;

            if (nClients < MAX_CLIENTS) {
                // Ghi nhan mot client moi
                clients[nClients].client_fd = client_fd;
                clients[nClients].room_joined = 0;
                memset(clients[nClients].client_name, 0, sizeof(clients[nClients].client_name));
                nClients++;

                // Them client vao danh sach theo doi
                FD_SET(client_fd, &fdread);
                if (max_fd < client_fd) max_fd = client_fd;

                printf("Co client moi ket noi: %d\n", client_fd);
                char msg[512];
                snprintf(msg, sizeof(msg), "Chao ban nhe\nDe tham gia vao phong chat ban can nhap dung dinh dang sau:\nclient_%d: client_name\n", client_fd);
                send(client_fd, msg, strlen(msg), 0);
            } else {
                printf("Qua %d ket noi roi:(\n", MAX_CLIENTS);
                char *msg = "Day roi ban oi\n";
                send(client_fd, msg, strlen(msg), 0);
                close(client_fd);
            }
        }

        for (int i = 0; i < nClients; i++) {
            int client_fd = clients[i].client_fd;
            
            if (FD_ISSET(client_fd, &fdtest)) {
                ret = recv(client_fd, buf, sizeof(buf) - 1, 0);

                if (ret <= 0) {
                    printf("Client %d mat/ngat ket noi\n", client_fd);
                    char msg[512];
                    snprintf(msg, sizeof(msg), "Client %d mat/ngat ket noi\n", client_fd);
                    sendToAll(clients, &nClients, msg, listener, "Server");
                    
                    FD_CLR(client_fd, &fdread);
                    close(client_fd);

                    removeClient(clients, &nClients, i);
                    i--;
                } else {
                    buf[ret] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0; 

                    if (strlen(buf) == 0) continue;

                    printf("Received from %d: %s\n", client_fd, buf);

                    if (clients[i].room_joined) {
                        printf("Gui tin nhan tu %d toi ca phong\n", client_fd);
                        sendToAll(clients, &nClients, buf, client_fd, clients[i].client_name);
                    } else { // Chua join room
                        if (processJoinRequest(buf, client_fd, clients[i].client_name)) {
                            clients[i].room_joined = 1;
                            
                            printf("Tao ket noi voi Client %d thanh cong voi ten %s\n", client_fd, clients[i].client_name);
                            char msg[512];
                            snprintf(msg, sizeof(msg), "Dang nhap thanh cong voi ten: %s\n", clients[i].client_name);
                            send(client_fd, msg, strlen(msg), 0);
                        } else {
                            char msg[512];
                            snprintf(msg, sizeof(msg), "Sai dinh dang hoac ten co khoang trang. Vui long nhap:\nclient_%d: client_name\n", client_fd);
                            send(client_fd, msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}