#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080
#define TARGET_STR "0123456789"
#define TARGET_LEN 10
#define OVERLAP_LEN (TARGET_LEN - 1)

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    printf("Server dang lang nghe %d...\n", PORT);

    int client = accept(server_fd, NULL, NULL);
    printf("Client da ket noi\n");

    char recv_buf[1024];
    char overlap[OVERLAP_LEN + 1] = {0}; 
    int overlap_size = 0;
    int total_count = 0;
    int bytes_read;

    // Nhan stream du lieu
    while ((bytes_read = recv(client, recv_buf, sizeof(recv_buf) - 1, 0)) > 0) {
        int combined_len = overlap_size + bytes_read;
        char *combined = malloc(combined_len + 1);
        
        memcpy(combined, overlap, overlap_size);
        memcpy(combined + overlap_size, recv_buf, bytes_read);
        combined[combined_len] = '\0';

        //Tim chuoi
        char *ptr = combined;
        int chunk_count = 0;
        while ((ptr = strstr(ptr, TARGET_STR)) != NULL) {
            chunk_count++;
            ptr += TARGET_LEN; // Trượt con trỏ qua chuỗi vừa tìm thấy
        }
        total_count += chunk_count;
        printf("Tim thay chuoi, Tong = %d\n", total_count);

        // Cap nhat overlap buff
        if (combined_len >= OVERLAP_LEN) {
            overlap_size = OVERLAP_LEN;
            memcpy(overlap, combined + combined_len - OVERLAP_LEN, OVERLAP_LEN);
        } else {
            overlap_size = combined_len;
            memcpy(overlap, combined, combined_len);
        }
        overlap[overlap_size] = '\0';
        
        free(combined);
    }

    printf("\nTong so lan xuat hien cua xau '%s': %d\n", TARGET_STR, total_count);
    close(client);
    close(server_fd);
    return 0;
}