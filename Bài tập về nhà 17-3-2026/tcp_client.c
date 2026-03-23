    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>

    int main(int argc, char *argv[]){
        if (argc != 3){
            printf("Nhap sai so tham so cua dong lenh: \n");
            return 1;
        }

        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(argv[2]));

        int a = inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
                
        if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
            printf("Loi ket noi toi server\n");
            return 1;
        }
        printf("Da ket noi den server %s:%s\n", argv[1], argv[2]);

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0){
            printf("Khong nhan duoc loi chao tu server!\n");
        } else {
            printf("%s\n", buffer);
        }



        while(1){
            memset(buffer, 0, sizeof(buffer));
            if(fgets(buffer, sizeof(buffer), stdin) != NULL){
                buffer[strcspn(buffer, "\n")] = 0;

                if (strcmp(buffer, "exit") == 0){
                    break;
                }

                int bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
                if (bytes_sent < 0){
                    perror("Loi gui du lieu");
                    break;
                }
            }
        }
        close(sockfd);
        printf("Da dong ket noi.\n");
        
        return 0;
    }