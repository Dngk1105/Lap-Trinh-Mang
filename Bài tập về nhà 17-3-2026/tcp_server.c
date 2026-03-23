    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main(int argc, char *argv[]){
        if (argc != 4){
            printf("Nhap sai so tham so cua dong lenh: \n");
            printf("Nhap dung phai la:\n");
            printf("tcp_server <cổng> <tệp tin chứa câu chào> <tệp tin lưu nội dung client gửi đến>\n");
            return 1;
        }

        FILE *f_hi = fopen(argv[2], "r");
        if (f_hi == NULL){
            perror("Khong mo duoc tep loi chao!\n");
            return 1;
        }

        FILE *f_save = fopen(argv[3], "w");
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
        int client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
        if (client_fd < 0) {
                perror("Loi accept");
                return 1;
            }
        printf("Da co client ket noi!\n");

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        if (fgets(buffer, sizeof(buffer), f_hi) != NULL){
            buffer[strcspn(buffer, "\n")] = 0;
            int bytes_sents = send(client_fd, buffer, strlen(buffer), 0);
        }

        int bytes_received;
        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

            if (bytes_received <= 0){
                break;
            }

            fwrite(buffer, 1, bytes_received, f_save);
            fwrite("\n", 1, 1, f_save);
            fflush(f_save);
        }
        
        printf("Client da ngat ket noi. Dong server.\n");
        close(client_fd);
        close(server_fd);
        fclose(f_hi);
        fclose(f_save);
        return 0;
    }