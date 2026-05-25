#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define POOL_SIZE 4

int listener;

// Hang doi ket noi
int client_queue[1024];
int front = 0;
int rear = 0;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void push_client(int client_fd) {
    pthread_mutex_lock(&queue_mutex);

    client_queue[rear] = client_fd;
    rear = (rear + 1) % 1024;

    pthread_cond_signal(&queue_cond);

    pthread_mutex_unlock(&queue_mutex);
}

int pop_client() {
    pthread_mutex_lock(&queue_mutex);

    while (front == rear) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    int client_fd = client_queue[front];
    front = (front + 1) % 1024;

    pthread_mutex_unlock(&queue_mutex);

    return client_fd;
}

void *process(void *arg) {
    char buf[4096];

    char *http_response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>DungSieuCapDepTrai HTTP Server</h1></body></html>\r\n";

    while (1) {

        // Moi thread lay 1 client tu hang doi
        int client_fd = pop_client();

        int received = recv(client_fd, buf, sizeof(buf) - 1, 0);

        if (received > 0) {
            buf[received] = '\0';

            printf("Thread %lu dang xu ly request cua client %d\n",
                   (unsigned long)pthread_self(),
                   client_fd);

            send(client_fd,
                 http_response,
                 strlen(http_response),
                 0);

            printf("Thread %lu da xu ly xong client %d\n",
                   (unsigned long)pthread_self(),
                   client_fd);
        }

        close(client_fd); // Khong duy tri ket noi
    }

    return NULL;
}

int main() {

    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;

    if (setsockopt(listener,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt))) {

        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener,
             (struct sockaddr *)&addr,
             sizeof(addr))) {

        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 1024)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Prethreading HTTP Server dang chay tren cong 8080 voi %d luong...\n",
           POOL_SIZE);

    pthread_t threads[POOL_SIZE];

    // Tao san dung 4 thread worker
    for (int i = 0; i < POOL_SIZE; i++) {

        if (pthread_create(&threads[i],
                           NULL,
                           process,
                           NULL) != 0) {

            perror("pthread_create() failed");
        }
    }

    while (1) {

        // Main thread chi accept client
        int client_fd = accept(listener, NULL, NULL);

        if (client_fd < 0) {
            continue;
        }

        printf("Main thread nhan client moi %d\n", client_fd);

        // Dua client vao hang doi
        push_client(client_fd);
    }

    close(listener);
    return 0;
}