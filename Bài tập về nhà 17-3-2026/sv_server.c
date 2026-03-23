#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

int main(int argc, char *argv[]){
    if (argc != 3){
        printf("Nhap sai so tham so cua dong lenh.\n");
        printf("Cach dung: ./sv_server <cong> <tep_tin_log>\n");
        return 1;
    }

    FILE *f_save = fopen(argv[2], "a");
    if (f_save == NULL){
        perror("Khong mo duoc tep de luu!\n");
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0){
        perror("Loi khoi tao sockfd\n");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        printf("Khong bind duoc!\n");
        return 1;
    }

    if (listen(server_fd, 5) < 0) { 
        perror("Loi listen");
        return 1;
    }
    
    printf("Server dang lang nghe tai cong %s...\n", argv[1]);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while(1){
        int client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
        if (client_fd < 0) {
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        char buffer[1024];
        int bytes_received;

        while (1) {
            memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received <= 0){
                break;
            }

            buffer[bytes_received] = '\0';

            time_t t = time(NULL);
            struct tm *tm_info = localtime(&t);
            char time_str[26];
            strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

            printf("%s %s %s\n", client_ip, time_str, buffer);

            fprintf(f_save, "%s %s %s\n", client_ip, time_str, buffer);
            fflush(f_save);
        }
        
        close(client_fd);
    }

    close(server_fd);
    fclose(f_save);
    return 0;
}