#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 5012

typedef struct Client{
    int client_fd;
    struct Client *friend;
    int is_paired;
    struct Client *next;
} Client;

Client* queue_head = NULL;
Client* queue_tail = NULL;
int queue_count = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void addQueue(Client* client){
    pthread_mutex_lock(&queue_mutex);

    client->next = NULL;
    client->is_paired = 0;
    client->friend = NULL;

    if (!queue_head){
        queue_head = client;
        queue_tail = queue_head;
    } else {
        queue_tail->next = client;
        queue_tail = client;
    }
    queue_count++;

    printf("Client %d vào hàng đợi (%d/2)\n", client->client_fd, queue_count);
    pthread_mutex_unlock(&queue_mutex);
}

Client* popQueue() {
    pthread_mutex_lock(&queue_mutex);
    
    if (queue_head == NULL) {
        pthread_mutex_unlock(&queue_mutex);
        return NULL;
    }

    Client* client = queue_head;
    queue_head = queue_head->next;
    if (queue_head == NULL) queue_tail = NULL;
    queue_count--;
    client->next = NULL;

    pthread_mutex_unlock(&queue_mutex);
    return client;
}

void try_pair(){
    while (queue_head != NULL && queue_head->next != NULL){
        Client *c1 = popQueue();
        Client *c2 = popQueue();

        if(c1 && c2){
            c1->friend = c2;
            c2->friend = c1;
            c1->is_paired = 1;
            c2->is_paired = 1;

            const char* msg = "Da ghep doi!!\n";
            send(c1->client_fd, msg, strlen(msg), 0);
            send(c2->client_fd, msg, strlen(msg), 0);

            printf("Da ghép đôi: %d <-> %d\n", c1->client_fd, c2->client_fd);
        }
    }
}

void remove_from_queue(Client* client) {
    if (!client) return;
    
    pthread_mutex_lock(&queue_mutex);
    
    if (queue_head == client) {
        queue_head = client->next;
        if (queue_head == NULL) queue_tail = NULL;
    } else {
        Client* temp = queue_head;
        while (temp && temp->next != client) {
            temp = temp->next;
        }
        if (temp && temp->next == client) {
            temp->next = client->next;
            if (temp->next == NULL) queue_tail = temp;
        }
    }
    queue_count--;
    client->next = NULL;
    
    pthread_mutex_unlock(&queue_mutex);
}

void* client_handler(void *arg) {
    Client* client = (Client*)arg;
    printf("Client connected: %d\n", client->client_fd);

    addQueue(client);
    try_pair();
    
    char buffer[BUFFER_SIZE];
    while (1) {
        int len = recv(client->client_fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            printf("Client disconnected: %d\n", client->client_fd);
            break;
        }

        buffer[len] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';
        if (strlen(buffer) == 0) continue;

        if (client->is_paired && client->friend){
            char msg[BUFFER_SIZE+64];
            snprintf(msg, sizeof(msg), "Client %d: %s\n", client->client_fd, buffer);
            send(client->friend->client_fd, msg, strlen(msg), 0);
        } else {
            send(client->client_fd, "Dang doi doi phuong...\n", 24, 0);
        }
    }

    if (client->is_paired && client->friend) {
        const char* end_msg = "Nguoi kia ngat ket noi\n";
        send(client->friend->client_fd, end_msg, strlen(end_msg), 0);
        close(client->friend->client_fd);
    }

    remove_from_queue(client);
    close(client->client_fd);
    free(client);
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

    printf("Chat Server đang chạy trên port %d...\n", PORT);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        Client* client = (Client*)malloc(sizeof(Client));
        client->client_fd = client_fd;
        client->friend = NULL;
        client->is_paired = 0;
        client->next = NULL;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, client);
        pthread_detach(tid);
    }

    close(listener);
    return 0;
}