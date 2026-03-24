#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    int opt = 1; 
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }

    int ret = bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    ret = listen(listener, 5);
    if (ret < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for client...\n");
    
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    int client = accept(listener, (struct sockaddr *)&client_addr, (socklen_t*) &client_addr_len);
    if (client < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Client connected: %d\n", client);
    printf("IP: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("Port: %d\n", ntohs(client_addr.sin_port));

    char buf[256];
    char *package = NULL;
    int total = 0;
    while (1) {
        int len = recv(client, buf, sizeof(buf), 0);
        
        if (len <= 0) {
            printf("Disconnected\n");
            break;
        }
        total += len;
        package = realloc(package, total + 1);
        memcpy(package + total - len, buf, len);
    }

    char path[256];
    strcpy(path, package);
    char filename[256];
    long filesize;
    printf("%s\n", path);
    int total1 = strlen(path) + 1;

    while (total1 < total) {
        strcpy(filename, package + total1);
        total1 += strlen(filename) + 1;
        memcpy(&filesize, package + total1, sizeof(filesize));
        total1 += sizeof(filesize);
        printf("%s - %ld bytes\n", filename, (long)filesize);
    }

    close(client);
    close(listener);
}