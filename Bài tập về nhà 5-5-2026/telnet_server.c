#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>

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

void runCommand(int client_fd, char *cmd) {
    char sys_cmd[512];
    char filename[32];
    snprintf(filename, sizeof(filename), "out_%d.txt", client_fd);
    snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s 2>&1", cmd, filename);
    
    system(sys_cmd);

    FILE *f = fopen(filename, "r");
    if (!f) {
        char *msg = "Khong mo file doc ket qua lenh duoc!\n";
        send(client_fd, msg, strlen(msg), 0);
    } else {
        char file_buff[1024];
        int bytes_read;
        while ((bytes_read = fread(file_buff, 1, sizeof(file_buff), f)) > 0) {
            send(client_fd, file_buff, bytes_read, 0);
        }
        fclose(f);
        remove(filename);
    }

    char *prompt = "\nDungSieuCapDepTrai> ";
    send(client_fd, prompt, strlen(prompt), 0);
}

void signal_handler(int sig) {
    int pid = wait(NULL);
    if (pid > 0) {
        printf("Child process terminated: %d\n", pid);
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    
    printf("Telnet Server is listening on port 8080...\n");

    signal(SIGCHLD, signal_handler);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;
        
        printf("Co client moi ket noi: %d\n", client_fd);
        
        if (fork() == 0) {
            close(listener);
            
            int xac_thuc = 0;
            
            char msg[512] = "Chao ban nhe\nVui long Dang nhap nhe, cu phap la:\n[username] [password]\n";
            send(client_fd, msg, strlen(msg), 0);

            char buf[256];
            while (1) {
                int len = recv(client_fd, buf, sizeof(buf) - 1, 0);
                if (len <= 0) {
                    printf("Client %d mat/ngat ket noi\n", client_fd);
                    break;
                }
                
                buf[len] = '\0';
                buf[strcspn(buf, "\r\n")] = 0;
                if (strlen(buf) == 0) continue;

                printf("Received from %d: %s\n", client_fd, buf);

                if (xac_thuc) {
                    printf("Thuc thi lenh tu Client %d: %s\n", client_fd, buf);
                    runCommand(client_fd, buf);
                } else {
                    int status = check(buf);
                    if (status == 1) {
                        xac_thuc = 1;
                        char user[128], pass[128];
                        sscanf(buf, "%s %s", user, pass);
                        printf("Client %d ket noi thanh cong voi ten\n   user: %s\n   mat khau: %s\n", client_fd, user, pass);
                        
                        char msg_success[512];
                        snprintf(msg_success, sizeof(msg_success), "Dang nhap tai khoan thanh cong voi ten\n   user: %s\n   mat khau: %s\nNhap lenh duoc roi nha:\nDungSieuCapDepTrai> ", user, pass);
                        send(client_fd, msg_success, strlen(msg_success), 0);
                    } else if (status == 0) {
                        char msg_err[512] = "Sai dinh dang. Vui long nhap:\n[username] [password]\n";
                        send(client_fd, msg_err, strlen(msg_err), 0);
                    } else if (status == -1) {
                        char msg_err[512] = "Loi he thong: Khong truy cap duoc file database\n";
                        send(client_fd, msg_err, strlen(msg_err), 0);
                    } else {
                        char msg_err[512] = "Sai user va mat khau\n";
                        send(client_fd, msg_err, strlen(msg_err), 0);
                    }
                }
            }
            close(client_fd);
            exit(0);
        }
        
        close(client_fd);
    }

    close(listener);
    return 0;
}