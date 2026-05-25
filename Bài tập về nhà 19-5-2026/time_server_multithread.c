#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

char formats[4][32] = {
    "dd/mm/yyyy",
    "dd/mm/yy",
    "mm/dd/yyyy",
    "mm/dd/yy"
};

char mapped_formats[4][32] = {
    "%d/%m/%Y\n",
    "%d/%m/%y\n",
    "%m/%d/%Y\n",
    "%m/%d/%y\n",
};

int check_format(char *format){
    for (int i = 0; i < sizeof(formats) / sizeof(formats[0]); i++){
        if (strcmp(format, formats[i]) == 0){
            return i;
        }
    }
    return -1;
}

void *clientThread(void *arg){
    int client = *(int *)arg;
    free(arg);

    char buf[512];

    while (1){
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0){
            printf("Client %d mat/ngat ket noi\n", client);
            break;
        }

        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;

        if (strlen(buf) == 0) continue;

        char cmd[32], format[32];
        int word = sscanf(buf, "%s %s", cmd, format);

        if (word != 2){
            char *msg = "Loi Lenh, Nhap lai di!\n";
            send(client, msg, strlen(msg), 0);
            continue;
        }

        if (strcmp(cmd, "GET_TIME") != 0){
            char *msg = "Loi CMD, Nhap lai di!\n";
            send(client, msg, strlen(msg), 0);
            continue;
        }

        int nformat = check_format(format);
        if (nformat == -1){
            char *msg = "Loi Format, Nhap lai di!\n";
            send(client, msg, strlen(msg), 0);
            continue;
        }

        time_t curr_time = time(NULL);
        struct tm *local = localtime(&curr_time);

        char msg[512];
        strftime(msg, sizeof(msg), mapped_formats[nformat], local);

        printf("Gui thoi gian toi client %d voi format %s\n", client, format);

        send(client, msg, strlen(msg), 0);
    }

    close(client);
    return NULL;
}

int main(){
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1){
        perror("Socket error");
        return -1;
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in address = {0};
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    if (bind(listener, (const struct sockaddr*) &address, sizeof(address))){
        perror("bind() error");
        close(listener);
        return -1;
    }

    if (listen(listener, 5)){
        perror("listen() error");
        close(listener);
        return -1;
    }

    printf("Time Server is listening on port 8080...\n");

    while(1){
        int client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        printf("Co ket noi moi %d\n", client);

        pthread_t tid;

        int *pclient = malloc(sizeof(int));
        *pclient = client;

        pthread_create(&tid, NULL, clientThread, pclient);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}