#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 8192

void send_response(int client, char* content, char* type){
    char header[512];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            type, strlen(content));
    send(client, header, strlen(header), 0);
    send(client, content, strlen(content), 0);
}

void send_file(int client, char *path){
    FILE *f = fopen(path, "rb");
    if (!f){
        char *msg = "<h1>404 Not Found</h1>";
        send_response(client, msg, "text/html");
        return; 
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char header[256];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %ld\r\n\r\n", size);
    send(client, header, strlen(header), 0);

    char buffer[1024];
    int n;
    while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0){
        send(client, buffer, n, 0);
    }

    fclose(f);
}

void list_directory(int client, char *path){
    DIR *dir = opendir(path);
    if (!dir){
        char *msg = "<h1>Cannot open directory</h1>";
        send_response(client, msg, "text/html");
        return;
    }

    char body[8192] = "<html><body><h1>Index of Directory</h1><hr>";
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0) continue;

        char fullpath[1024];
        sprintf(fullpath, "%s/%s", path, entry->d_name);
        struct stat st;
        stat(fullpath, &st);

        strcat(body, "<a href=\"");
        strcat(body, entry->d_name);
        strcat(body, "\">");

        if (S_ISDIR(st.st_mode)){
            strcat(body, "<b>");
            strcat(body, entry->d_name);
            strcat(body, "/</b>");
        } else {
            strcat(body, entry->d_name);
        }
        strcat(body, "</a><br>");
    }
    strcat(body, "</body></html>");
    closedir(dir);
    send_response(client, body, "text/html");
}

double calculate(double a, double b, char *op){
    if (strcmp(op, "add") == 0) return a + b;
    if (strcmp(op, "sub") == 0) return a - b;
    if (strcmp(op, "mul") == 0) return a * b;
    if (strcmp(op, "div") == 0) return (b != 0) ? a / b : 0;
    return 0;
}

void handle_calculator(int client, char *data){
    double a = 0, b = 0;
    char op[16] = "";
    sscanf(data, "a=%lf&b=%lf&op=%15s", &a, &b, op);
    double result = calculate(a, b, op);

    char body[1024];
    sprintf(body, "<html><body><h1>Calculator Result</h1>"
                  "<p>%.2f %s %.2f = %.4f</p></body></html>", 
                  a, op, b, result);
    send_response(client, body, "text/html");
}

int main(){
    int listener, client;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);
    printf("Server running on http://localhost:%d\n", PORT);

    while(1){
        client = accept(listener, NULL, NULL);
        if (client < 0) continue;

        memset(buffer, 0, BUFFER_SIZE);
        recv(client, buffer, BUFFER_SIZE-1, 0);

        printf("Request:\n%s\n", buffer);

        char method[16] = "", path[1024] = "";
        sscanf(buffer, "%s %s", method, path);

        // Xử lý Calculator
        if (strncmp(path, "/calc", 5) == 0){
            if (strcmp(method, "GET") == 0){
                char *query = strchr(buffer, '?');
                if (query){
                    query++;
                    char *space = strchr(query, ' ');
                    if (space) *space = '\0';
                    handle_calculator(client, query);
                }
            } 
            else if (strcmp(method, "POST") == 0){
                char *body_start = strstr(buffer, "\r\n\r\n");
                if (body_start){
                    body_start += 4;
                    handle_calculator(client, body_start);
                }
            }
        } 
        else {
            // Xử lý file/directory
            char *real_path = path;

            if (strcmp(path, "/") == 0){
                real_path = ".";
            } 
            else if (path[0] == '/'){
                real_path = path + 1;   // Bỏ dấu / đầu
            }

            struct stat st;
            if (stat(real_path, &st) == 0){
                if (S_ISDIR(st.st_mode)){
                    list_directory(client, real_path);
                } else {
                    send_file(client, real_path);
                }
            } else {
                char *msg = "<h1>404 Not Found</h1>";
                send_response(client, msg, "text/html");
            }
        }

        close(client);
    }

    close(listener);
    return 0;
}