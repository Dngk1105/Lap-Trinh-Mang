#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8000);

    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    char cwd[1024];
    int len = 0;
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        len = send(client, cwd, strlen(cwd) + 1, 0);
    } else {
        perror("Error getting cwd");
        exit(EXIT_FAILURE);
    }

    DIR *dir;
    dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    struct stat file_stat;
    while ((entry = readdir(dir)) != NULL) {
        if (stat(entry->d_name, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                len += send(client, entry->d_name, strlen(entry->d_name) + 1, 0);
                len += send(client, &file_stat.st_size, sizeof(file_stat.st_size), 0);
            }
        }
    }

    closedir(dir);
    close(client);
    return 0;
}