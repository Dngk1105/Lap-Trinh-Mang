#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>
#include <poll.h>

#define MAX_CLIENTS 512

struct Client {
    int client_fd;
    int xac_thuc;
};

void removeClient(struct Client *clients, int *nClients, int index) {
    if (index < *nClients - 1) {
        clients[index] = clients[*nClients - 1]; 
    }
    (*nClients)--;
}

void runCommand(int client_fd, char *cmd){
    char sys_cmd[512];
    char filename[32];
    snprintf(filename, sizeof(filename), "out_%d.txt", client_fd);
    snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", cmd, filename);
    
    system(sys_cmd); //Chay lenh nay de he thong thuc hien lenh

    FILE *f = fopen(filename, "r");
    if (!f){
        char *msg = "Khong mo file doc ket qua lenh duoc!\n";
        send(client_fd, msg, strlen(msg), 0);
    } else {
        char file_buff[1024];
        int bytes_read;
        while ((bytes_read = fread(file_buff, 1, sizeof(file_buff), f)) > 0){
            send(client_fd, file_buff, bytes_read, 0);
        }
        fclose(f);
        remove(filename);
    }

    char *prompt = "\nDungSieuCapDepTrai> ";
    send(client_fd, prompt, strlen(prompt), 0);
}

int check(char* input){
    char user[128], pass[128];
    if (sscanf(input, "%s %s", user, pass) != 2){
        return 0;
    }

    FILE *f = fopen("database.txt", "r");
    if (!f){
        perror("Loi mo file database");
        return -1;
    }

    char line[256];
    char db_user[128], db_pass[128];
    while (fgets(line, sizeof(line), f)) { 
        if (sscanf(line, "%s %s", db_user, db_pass) == 2){
            if (strcmp(user, db_user) == 0
            &&  strcmp(pass, db_pass) == 0){
                fclose(f);
                return 1;
            }
        }
    } 

    fclose(f);
    return -2;
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

    printf("Telnet Server dang lang nghe tai cong 8080 \n");

    // Danh sach Clients
    struct Client clients[MAX_CLIENTS];
    int nClients = 0;

    struct pollfd fds[MAX_CLIENTS + 1];
    char buf[256];
    
    while(1) {
        int nfds = 0;

        fds[0].fd = listener;
        fds[0].events = POLLIN;
        nfds++;

        for (int i = 0; i < nClients; i++){
            fds[i+1].fd = clients[i].client_fd;
            fds[i+1].events = POLLIN;
            nfds++;
        }
        
        int ret = poll(fds, nfds, 5000);
        if (ret < 0) {
            perror("loi select");
            return 1;
        } else if (ret == 0) {
            continue;
        }

        if (fds[0].revents & POLLIN) { 
            int client_fd = accept(listener, NULL, NULL);
            if (client_fd < 0) continue;

            if (nClients < MAX_CLIENTS) {
                // Ghi nhan mot client moi
                clients[nClients].client_fd = client_fd;
                clients[nClients].xac_thuc = 0;
                nClients++;

                printf("Co client moi ket noi: %d\n", client_fd);
                char msg[512] = "Chao ban nhe\nVui long Dang nhap nhe, cu phap la:\n[username] [password]\n";
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
            
            if (fds[i+1].revents & (POLLIN | POLLERR)) {
                ret = recv(client_fd, buf, sizeof(buf) - 1, 0);

                if (ret <= 0) {
                    printf("Client %d mat/ngat ket noi\n", client_fd);
                    

                    close(client_fd);
                    removeClient(clients, &nClients, i);
                    i--;
                } else {
                    buf[ret] = '\0';
                    buf[strcspn(buf, "\r\n")] = 0; 

                    if (strlen(buf) == 0) continue;

                    printf("Received from %d: %s\n", client_fd, buf);

                    if (clients[i].xac_thuc) {
                        printf("Thuc thi lenh tu Client %d: %s\n", client_fd, buf);
                        runCommand(client_fd, buf);
                    } else { //Chua dang nhap
                        int status = check(buf);
                        if (status == 1) {
                            clients[i].xac_thuc = 1;
                                
                            char user[128], pass[128];
                            sscanf(buf, "%s %s", user, pass);
                            printf("Client %d ket noi thanh cong voi ten\n   user: %s\n   mat khau: %s\n", client_fd, user, pass);
                            
                            char msg[512];
                            snprintf(msg, sizeof(msg), "Dang nhap tai khoan thanh cong voi ten\n   user: %s\n   mat khau: %s\nNhap lenh duoc roi nha:\nDungSieuCapDepTrai> ", user, pass);
                            send(client_fd, msg, strlen(msg), 0);
                        } else if (status == 0){
                            char msg[512];
                            snprintf(msg, sizeof(msg), "Sai dinh dang. Vui long nhap:\n[username] [password]\n");
                            send(client_fd, msg, strlen(msg), 0);
                        } else if (status == -1){
                            char msg[512] = "Loi he thong: Khong truy cap duoc file database\n";
                            send(client_fd, msg, strlen(msg), 0);
                        } else {
                            char msg[512] = "Sai user va mat khau\n";
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