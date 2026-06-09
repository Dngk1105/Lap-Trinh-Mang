#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_DOMAIN "lebavui.io.vn" //lebavui.io.vn
#define PORT 21
#define BUFFER_SIZE 4096


// Tạo kết nối TCP tới hostname/IP và port
int ftp_connect(const char *hostname, int port) {
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *host;

    host = gethostbyname(hostname);
    if (host == NULL) {
        perror("Khong the phan giai ten mien");
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Khong the tao socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    bzero(&(server_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0) {
        perror("Loi ket noi");
        close(sock);
        return -1;
    }
    return sock;
}

// Gửi lệnh FTP thô
void ftp_send_cmd(int sock, const char *cmd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    send(sock, buffer, strlen(buffer), 0);
}

// Đọc phản hồi từ Control Channel
void ftp_recv_res(int sock, char *response) {
    memset(response, 0, BUFFER_SIZE);
    recv(sock, response, BUFFER_SIZE - 1, 0);
    printf("%s", response);
}


// Đăng nhập
int ftp_login(int sock, const char *user, const char *pass) {
    char cmd[256], res[BUFFER_SIZE];
    
    snprintf(cmd, sizeof(cmd), "USER %s", user);
    ftp_send_cmd(sock, cmd);
    ftp_recv_res(sock, res);

    snprintf(cmd, sizeof(cmd), "PASS %s", pass);
    ftp_send_cmd(sock, cmd);
    ftp_recv_res(sock, res);

    if (strncmp(res, "230", 3) == 0) return 1; // 230 User logged in
    return 0;
}

// Parse PASV và trả về Data Socket
int ftp_enter_pasv(int ctrl_sock) {
    char res[BUFFER_SIZE];
    ftp_send_cmd(ctrl_sock, "PASV");
    ftp_recv_res(ctrl_sock, res);

    // Parse IP và Port từ chuỗi phản hồi (VD: 227 Entering Passive Mode (192,168,1,1,192,52))
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(res, "%*[^(](%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        printf("Loi parse PASV\n");
        return -1;
    }

    char data_ip[32];
    snprintf(data_ip, sizeof(data_ip), "%d.%d.%d.%d", h1, h2, h3, h4);
    int data_port = (p1 * 256) + p2;

    return ftp_connect(data_ip, data_port);
}


// Lệnh không yêu cầu kênh dữ liệu (Tạo thư mục, xóa, đổi tên)
void ftp_simple_cmd(int ctrl_sock, const char *cmd) {
    char res[BUFFER_SIZE];
    ftp_send_cmd(ctrl_sock, cmd);
    ftp_recv_res(ctrl_sock, res);
}

// Lấy danh sách file (LIST)
void ftp_list(int ctrl_sock) {
    int data_sock = ftp_enter_pasv(ctrl_sock);
    if (data_sock < 0) return;

    char res[BUFFER_SIZE];
    ftp_send_cmd(ctrl_sock, "LIST");
    ftp_recv_res(ctrl_sock, res); // 150 File status okay

    printf("\n  Danh sach File  \n");
    int bytes_read;
    while ((bytes_read = recv(data_sock, res, BUFFER_SIZE - 1, 0)) > 0) {
        res[bytes_read] = '\0';
        printf("%s", res);
    }
    printf("\n\n");

    close(data_sock);
    ftp_recv_res(ctrl_sock, res); // 226 Transfer complete
}

// Download File
void ftp_download(int ctrl_sock, const char *filename) {
    int data_sock = ftp_enter_pasv(ctrl_sock);
    if (data_sock < 0) return;

    char res[BUFFER_SIZE], cmd[256];
    snprintf(cmd, sizeof(cmd), "RETR %s", filename);
    ftp_send_cmd(ctrl_sock, cmd);
    ftp_recv_res(ctrl_sock, res); // Đọc mã 150

    if (strncmp(res, "150", 3) == 0) {
        FILE *file = fopen(filename, "wb");
        int bytes_read;
        while ((bytes_read = recv(data_sock, res, BUFFER_SIZE, 0)) > 0) {
            fwrite(res, 1, bytes_read, file);
        }
        fclose(file);
        printf("Tai file thanh cong!\n");
    }

    close(data_sock);
    ftp_recv_res(ctrl_sock, res); // Đọc mã 226
}

// Upload File
void ftp_upload(int ctrl_sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Khong tim thay file %s tren may!\n", filename);
        return;
    }

    int data_sock = ftp_enter_pasv(ctrl_sock);
    if (data_sock < 0) {
        fclose(file);
        return;
    }

    char res[BUFFER_SIZE], cmd[256];
    snprintf(cmd, sizeof(cmd), "STOR %s", filename);
    ftp_send_cmd(ctrl_sock, cmd);
    ftp_recv_res(ctrl_sock, res); // Đọc mã 150

    if (strncmp(res, "150", 3) == 0) {
        int bytes_read;
        while ((bytes_read = fread(res, 1, BUFFER_SIZE, file)) > 0) {
            send(data_sock, res, bytes_read, 0);
        }
        printf("Upload file thanh cong!\n");
    }
    fclose(file);
    close(data_sock);
    ftp_recv_res(ctrl_sock, res); // Đọc mã 226
}


// Bài tập số 4
void process_answer_file(const char *question_filename) {
    char answer_filename[256];
    
    //question... -> answer...
    snprintf(answer_filename, sizeof(answer_filename), "answer_%s", question_filename + 9);
    
    //Doc file question
    FILE *fq = fopen(question_filename, "r");
    if (!fq) {
        printf("Loi: Khong the mo file %s tren may!\n", question_filename);
        return;
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), fq);
    fclose(fq);

    buffer[strcspn(buffer, "\r\n")] = 0;    
    int len = strlen(buffer);
    
    // Dao nguoc lai chuoi
    for (int i = 0; i < len / 2; i++) {
        char temp = buffer[i];
        buffer[i] = buffer[len - 1 - i];
        buffer[len - 1 - i] = temp;
    }

    // Ghi chuoi
    FILE *fa = fopen(answer_filename, "w");
    if (!fa) {
        printf("Loi: Khong the tao file %s!\n", answer_filename);
        return;
    }
    fputs(buffer, fa);
    fclose(fa);

    printf("Da tao thanh cong: [%s]\n", answer_filename);
    printf("Noi dung dao nguoc: %s\n", buffer);
}

int main() {
    char user[64], pass[64], res[BUFFER_SIZE];
    
    printf("     FTP CLIENT     \n");
    printf("IP/Domain: %s\n", SERVER_DOMAIN);
    printf("Username: "); scanf("%s", user);
    printf("Password: "); scanf("%s", pass);

    int ctrl_sock = ftp_connect(SERVER_DOMAIN, PORT);
    if (ctrl_sock < 0) return 1;

    ftp_recv_res(ctrl_sock, res); // Đọc welcome message 220

    if (!ftp_login(ctrl_sock, user, pass)) {
        printf("Dang nhap that bai!\n");
        close(ctrl_sock);
        return 1;
    }

    int choice;
    char param1[256], param2[256], cmd[512];

    while (1) {
        printf("\n      MENU        \n");
        printf("1. Hien thi danh sach file/thu muc (LIST)\n");
        printf("2. Download file\n");
        printf("3. Upload file\n");
        printf("4. Tao thu muc (MKD)\n");
        printf("5. Xoa thu muc (RMD)\n");
        printf("6. Xoa file (DELE)\n");
        printf("7. Doi ten file/thu muc\n");
        printf("8. Lam bai 4 thi chon day nhe\n");
        printf("0. Thoat\n");
        printf("Chon: ");
        if (scanf("%d", &choice) != 1) break;

        switch (choice) {
            case 1:
                ftp_list(ctrl_sock);
                break;
            case 2:
                printf("Nhap ten file can tai: "); scanf("%s", param1);
                ftp_download(ctrl_sock, param1);
                break;
            case 3:
                printf("Nhap ten file can day len: "); scanf("%s", param1);
                ftp_upload(ctrl_sock, param1);
                break;
            case 4:
                printf("Nhap ten thu muc can tao: "); scanf("%s", param1);
                snprintf(cmd, sizeof(cmd), "MKD %s", param1);
                ftp_simple_cmd(ctrl_sock, cmd);
                break;
            case 5:
                printf("Nhap ten thu muc can xoa: "); scanf("%s", param1);
                snprintf(cmd, sizeof(cmd), "RMD %s", param1);
                ftp_simple_cmd(ctrl_sock, cmd);
                break;
            case 6:
                printf("Nhap ten file can xoa: "); scanf("%s", param1);
                snprintf(cmd, sizeof(cmd), "DELE %s", param1);
                ftp_simple_cmd(ctrl_sock, cmd);
                break;
            case 7:
                printf("Nhap ten hien tai: "); scanf("%s", param1);
                printf("Nhap ten moi: "); scanf("%s", param2);
                snprintf(cmd, sizeof(cmd), "RNFR %s", param1);
                ftp_simple_cmd(ctrl_sock, cmd);
                snprintf(cmd, sizeof(cmd), "RNTO %s", param2);
                ftp_simple_cmd(ctrl_sock, cmd);
                break;
            case 8: 
                printf("Nhap ten file de bai: "); 
                scanf("%s", param1);
                
                ftp_download(ctrl_sock, param1); 
                
                process_answer_file(param1); //Chuyen file
                
                char answer_name[256];
                snprintf(answer_name, sizeof(answer_name), "answer_%s", param1 + 9);
                ftp_upload(ctrl_sock, answer_name); 
                break;    
            case 0:
                ftp_send_cmd(ctrl_sock, "QUIT");
                close(ctrl_sock);
                printf("Ngat ket noi!\n");
                return 0;
            default:
                printf("Lua chon khong hop le.\n");
        }
    }
    return 0;
}