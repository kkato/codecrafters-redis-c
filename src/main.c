#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

void *handle_client(void *arg) {
	int client_fd = *(int *)arg;
	free(arg);

	char buf[1024];
	ssize_t bytes_read;
	const char *pong = "+PONG\r\n";

	while ((bytes_read = read(client_fd, buf, sizeof(buf))) > 0) {
		if (write(client_fd, pong, strlen(pong)) == -1) {
			printf("Write failed: %s \n", strerror(errno));
			break;
		}
	}

	close(client_fd);
	return NULL;
}

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	printf("Logs from your program will appear here!\n");

	int server_fd;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = { .sin_family = AF_INET,
	                                 .sin_port = htons(6379),
	                                 .sin_addr = { htonl(INADDR_ANY) },
	                               };

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for clients to connect...\n");

	while (1) {
		int *client_fd = malloc(sizeof(int));
		if (!client_fd) {
			printf("malloc failed\n");
			continue;
		}

		*client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		if (*client_fd == -1) {
			printf("Accept failed: %s \n", strerror(errno));
			free(client_fd);
			continue;
		}

		printf("Client connected\n");

		pthread_t thread;
		if (pthread_create(&thread, NULL, handle_client, client_fd) != 0) {
			printf("Thread creation failed: %s \n", strerror(errno));
			close(*client_fd);
			free(client_fd);
			continue;
		}
		pthread_detach(thread);
	}

	close(server_fd);
	return 0;
}
