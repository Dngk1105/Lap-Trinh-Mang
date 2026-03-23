#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]){
    if (argc != 3){
        printf("Nhap sai so tham so cua dong lenh.\n");
        printf("Cach dung: ./sv_client <dia_chi_IP> <cong>\n");
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0){
        perror("Loi tao socket");
        return 1;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        printf("Loi ket noi toi server\n");
        return 1;
    }

    char mssv[20], name[50], birth[20], gpa[10];
    char buffer[1024];

    printf("Nhap exit de thoat bat ki luc nao!!!!");
    while(1){
        printf("Nhap MSSV: ");
        if(fgets(mssv, sizeof(mssv), stdin) == NULL) break;
        mssv[strcspn(mssv, "\n")] = 0;
        
        if (strcmp(mssv, "exit") == 0) break;

        printf("Nhap Ho ten: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = 0;

        printf("Nhap Ngay sinh (YYYY-MM-DD): ");
        fgets(birth, sizeof(birth), stdin);
        birth[strcspn(birth, "\n")] = 0;

        printf("Nhap Diem TB: ");
        fgets(gpa, sizeof(gpa), stdin);
        gpa[strcspn(gpa, "\n")] = 0;

        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%s %s %s %s", mssv, name, birth, gpa);

        int bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
        if (bytes_sent < 0){
            perror("Loi gui du lieu");
            break;
        }
        printf("Da gui du lieu thanh cong!\n\n");
    }

    close(sockfd);
    return 0;
}