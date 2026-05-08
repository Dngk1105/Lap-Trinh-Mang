#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#define POOL_SIZE 4

void process(int listener) {
    char buf[4096];
    char *http_response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/html; charset=UTF-8\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "<html><body><h1>DungSieuCapDepTrai HTTP Server</h1></body></html>\r\n";

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        int received = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (received > 0) {
            buf[received] = '\0';
            printf("Process PID %d dang xu ly request\n", getpid());
            send(client_fd, http_response, strlen(http_response), 0);
        }
        
        close(client_fd); // Khong duy tri ket noi
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
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
    
    if (listen(listener, 1024)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Preforking HTTP Server dang chay tren cong 8080 voi %d tiến trình...\n", POOL_SIZE);

    for (int i = 0; i < POOL_SIZE; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            process(listener);
            exit(0);
        } else if (pid < 0) {
            perror("fork() failed");
        }
    }

    while (wait(NULL) > 0);

    close(listener);
    return 0;
}