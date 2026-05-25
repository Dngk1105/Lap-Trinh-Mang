#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define FOLDER_PATH "./server_files"

char* scan_folder(int *file_count) {
    DIR *dir;
    struct dirent *entry;
    *file_count = 0;

    dir = opendir(FOLDER_PATH);
    if (!dir) {
        return strdup("Khong mo duoc folder chua file\r\n"); //strdup nhằm tạo bản sao trong heap, vi can free() sau nay
    }

    // Đếm file trước
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            (*file_count)++;
        }
    }

    if (*file_count == 0) {
        closedir(dir);
        return strdup("Khong co files\r\n");
    }

    // Quay lại đầu thư mục
    rewinddir(dir);

    // Tính kích thước buffer
    size_t total_size = 128; // header + margin
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            total_size += strlen(entry->d_name) + 3; // + \r\n
        }
    }

    char *response = (char*)malloc(total_size);
    if (!response) {
        closedir(dir);
        return strdup("Khong cap phat duoc response\r\n");
    }

    memset(response, 0, total_size);
    sprintf(response, "OK %d\r\n", *file_count);

    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            strcat(response, entry->d_name);
            strcat(response, "\r\n");
        }
    }

    closedir(dir);
    return response;
}

// Gửi file
int send_file(int client, const char *filename) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", FOLDER_PATH, filename);

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        const char *err = "Khong tim thay file\r\n";
        send(client, err, strlen(err), 0);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    char header[128];
    sprintf(header, "OK %ld\r\n", filesize);
    send(client, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, f)) > 0) {
        send(client, buffer, bytes_read, 0);
    }

    fclose(f);
    return 0;
}

void* client_handler(void *arg) {
    int client = *(int*)arg;
    free(arg);  // Giải phóng bộ nhớ đã malloc

    printf("Client connected: %d\n", client);

    int file_count;
    char *file_list = scan_folder(&file_count);

    send(client, file_list, strlen(file_list), 0);
    free(file_list);

    if (file_count <= 0) {
        close(client);
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        int len = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            printf("Client disconnected: %d\n", client);
            break;
        }

        buffer[len] = '\0';
        // Loại bỏ \r\n
        buffer[strcspn(buffer, "\r\n")] = '\0';

        if (strlen(buffer) == 0)
            continue;

        printf("Client requested: %s\n", buffer);
        
        if (send_file(client, buffer) == 0) {
            break;
        }
    }

    close(client);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listener);
        return 1;
    }

    if (listen(listener, SOMAXCONN) < 0) {
        perror("listen");
        close(listener);
        return 1;
    }

    printf("File Server đang chạy trên port %d...\n", PORT);
    printf("Thư mục chia sẻ: %s\n", FOLDER_PATH);

    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        int *client_ptr = malloc(sizeof(int)); // Tao heap an toan de truyen vao thread
        *client_ptr = client;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, client_ptr);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}