#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <ctype.h>
#include <errno.h>

int main(int argc, char* argv[]){
    // Xu li tham so 
    if (argc != 4){
        printf("Khong dung dinh dang!\n"
                "Dinh dang dung:\n"
                "%s <port_s> <ip_d> <port_d>\n", argv[0] 
            );
        return 1;
    }
    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    //Khoi tao socket
    int listener = socket(AF_INET, SOCK_DGRAM, 0);
    if (listener < -1){
        perror("Khong tao duoc sokcet");
        return 1;
    }

    
    struct sockaddr_in src_addr;
    struct sockaddr_in dest_addr;
    //Gan ip,port vao listener (bind)
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(port_s);
    src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listener, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0){
        perror("Khong bind duoc socket!");
        close(listener);
        return 1;
    }

    //Dia chi nguoi nhan
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_d);
    dest_addr.sin_addr.s_addr = inet_addr(ip_d);    //inet_addr(): Chuyen dia chi mang (vd 192.168.3.1) thanh chuoi nhi phan

    // Chuyen socket listener sang che do non blocking
    // fcntl(): Thao tac voi file descripter
    int flags = fcntl(listener, F_GETFL, 0);
    fcntl(listener, F_SETFL, flags | O_NONBLOCK);

    // Chuyen trang thai cua nhap ban phim sang non blocking 
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK); 
    
    printf(
        "Da cau hinh thanh cong!\n"
        "Lang nghe cong: %d\n"
        "Gui toi %s:%d\n"
        "Nhap tin nhan: \n"
        ,port_s, ip_d, port_d
    );
    fflush(stdout);


    //Phan xu li linh tinh
    char buffer[1024];
    
    while (1){
        // Kiem tra tin nhan tu ngoai
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        int recv_len = recvfrom(
            listener, 
            buffer, 
            sizeof(buffer) -1, 
            0, 
            (struct sockaddr*)&sender_addr, 
            &sender_len
        );
        
        if (recv_len > 0){
            //Xu li buffer
            buffer[recv_len] = '\0';
            buffer[strcspn(buffer, "\r\n")] = 0;

            printf("[%s:%d]: %s\n",
                inet_ntoa(sender_addr.sin_addr),
                ntohs(sender_addr.sin_port),
                buffer
            );
            fflush(stdout);
        } else if (recv_len < 0){
            if (errno != EWOULDBLOCK && errno != EAGAIN){
                perror("Loi nhan du lieu!");
            }
        }

        // Kiem tra nhap du lieu
        int read_len = read(STDIN_FILENO, buffer, sizeof(buffer)-1);
        if (read_len > 0){
            buffer[read_len] = '\0';
            buffer[strcspn(buffer, "\r\n")] = 0;

            if (strcmp(buffer, "exit") == 0){
                printf("Thoat chuong trinh!\n");
                break;
            }

            if (strlen(buffer) > 0){
                sendto(
                    listener, 
                    buffer, 
                    strlen(buffer), 
                    0, 
                    (struct sockaddr*)&dest_addr,
                    sizeof(dest_addr)
                );

            }
        }
            
        usleep(10000);
    }

    close(listener);
    return 0;
}