CC = gcc
CFLAGS = -Wall -Wextra -g

all: info_client info_server

info_client: info_client.c
	$(CC) $(CFLAGS) -o info_client info_client.c

info_server: info_server.c
	$(CC) $(CFLAGS) -o info_server info_server.c

clean:
	rm -f info_client info_server