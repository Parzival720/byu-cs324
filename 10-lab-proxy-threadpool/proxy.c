#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include "sockhelper.h"
#include "sbuf.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define NTHREADS  8
#define SBUFSIZE  5

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
// void *handle_client_thread(void *vargp);
void *handle_clients(void *vargp);
int open_sfd(char *[]);
void handle_client(int);
void test_parser();
void print_bytes(unsigned char *, int);

sbuf_t sbuf; /* Shared buffer of connected descriptors */

int main(int argc, char *argv[])
{
	int sfd = open_sfd(argv);

	sbuf_init(&sbuf, SBUFSIZE);
	pthread_t tid;
	for (int i = 0; i < NTHREADS; i++) {
		pthread_create(&tid, NULL, handle_clients, NULL);
	}
	while (1) {

		// int temp_socket;
		struct sockaddr_storage remote_addr_ss;
		struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		// temp_socket = accept(sfd, remote_addr, &addr_len);
		
		// int *connfd = malloc(sizeof(int));
		// *connfd = accept(sfd, remote_addr, &addr_len);
	
		// pthread_t tid;
		// pthread_create(&tid, NULL, handle_client_thread, connfd);

		int connfd = accept(sfd, remote_addr, &addr_len);

		sbuf_insert(&sbuf, connfd);
	}


	printf("%s\n", user_agent_hdr);
	return 0;
}

int complete_request_received(char *request) {
	// printf("Check if request is complete: \n%s\n", request);
	if (strstr(request, "\r\n\r\n") != NULL) {
		return 1;
	} else {
		return 0;
	}
}

void parse_request(char *request, char *method,
		char *hostname, char *port, char *path) {
	memset(method, '\0', 16);
	memset(hostname, '\0', 64);
	memset(port, '\0', 8);
	memset(path, '\0', 64);
	char *start_of_thing = request;
	char *end_of_thing = strstr(start_of_thing, " ");
	strncpy(method, start_of_thing, end_of_thing - start_of_thing);
	start_of_thing = end_of_thing + 1;

	end_of_thing = strstr(start_of_thing, "://");
	start_of_thing = end_of_thing + 3;
	end_of_thing = strstr(start_of_thing, ":");
	char *first_line = strstr(start_of_thing, "\r\n");
	if (end_of_thing == NULL || first_line < end_of_thing) {
		strcpy(port, "80");
		end_of_thing = strstr(start_of_thing, "/");
		strncpy(hostname, start_of_thing, end_of_thing - start_of_thing);
		start_of_thing = end_of_thing;
	} else {
		strncpy(hostname, start_of_thing, end_of_thing - start_of_thing);
		start_of_thing = end_of_thing + 1;

		end_of_thing = strstr(start_of_thing, "/");
		strncpy(port, start_of_thing, end_of_thing - start_of_thing);
		start_of_thing = end_of_thing;
	}

	end_of_thing = strstr(start_of_thing, " ");
	strncpy(path, start_of_thing, end_of_thing - start_of_thing);
}

void *handle_client_thread(void *vargp) {
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	free(vargp);
	handle_client(connfd);
	close(connfd);
	return NULL;
}

void *handle_clients(void *vargp) {
	pthread_detach(pthread_self());
	while (1) {
		int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */
		handle_client(connfd);
		close(connfd);
	}
}

int open_sfd(char *argv[]) {
	int addr_fam = AF_INET;
	int sock_type = SOCK_STREAM;
	unsigned short port = atoi(argv[1]);

	struct sockaddr_storage local_addr_ss;
	struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;

	populate_sockaddr(local_addr, addr_fam, NULL, port);

	int sfd;
	if ((sfd = socket(addr_fam, sock_type, 0)) < 0) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	int optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	if (bind(sfd, local_addr, sizeof(struct sockaddr_storage)) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	if(listen(sfd, 100) < 0) {
		perror("Error listening");
		exit(EXIT_FAILURE);
	}

	printf("Listening on socket %d\n", port);

	return sfd;
}

void handle_client(int client_socket)
{
    char buf[1024];
	memset(buf, '\0', 1024);
	ssize_t totread = 0;
	ssize_t nread = 0;

	// printf("Handling client\n");
	
	while (1) {
		nread = recv(client_socket, &buf[totread], 1024 - totread, 0);
		if (nread < 0) {
			perror("receiving message");
			exit(EXIT_FAILURE);
		} 
		// else if (nread == 0) {
		// 	break;
		// }
		totread += nread;
		// printf("Read: %ld bytes\n", nread);

		if (strstr(buf, "\r\n\r\n") != NULL) {
			// printf("Found carriage return newline twice\n");
			// printf("%s\n", buf);
			// printf("Total bytes read: %ld\n", totread);
			break;
		}
	}

	print_bytes((unsigned char *)buf, totread);

	buf[totread] = '\0';

	char method[16], hostname[64], port[8], path[64];
	if (complete_request_received(buf)) {
		printf("REQUEST COMPLETE\n");
		parse_request(buf, method, hostname, port, path);
		printf("METHOD: %s\n", method);
		printf("HOSTNAME: %s\n", hostname);
		printf("PORT: %s\n", port);
		printf("PATH: %s\n", path);
	} else {
		printf("REQUEST INCOMPLETE\n");
	}

	// close(client_socket);

	char modified_request[1024];
	char new_hostname[256];
	if (strcmp(port, "80") == 0) {
		sprintf(new_hostname, "%s", hostname);
	} else {
		sprintf(new_hostname, "%s:%s", hostname, port);
	}
	sprintf(modified_request, "%s %s HTTP/1.0\r\nHost: %s\r\n%s\r\n"
		"Connection: close\r\nProxy-Connection: close\r\n\r\n", 
		method, path, new_hostname, user_agent_hdr);
	// print_bytes((unsigned char *)modified_request, strlen(modified_request));

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *result;
	int s;
	s = getaddrinfo(hostname,
			port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}


	int server_socket;
	int addr_fam;
	socklen_t addr_len;

	struct sockaddr_storage local_addr_ss;
	struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;
	char local_ip[INET6_ADDRSTRLEN];
	unsigned short local_port;

	struct sockaddr_storage remote_addr_ss;
	struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
	char remote_ip[INET6_ADDRSTRLEN];
	unsigned short remote_port;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	addr_fam = result->ai_family;
	addr_len = result->ai_addrlen;
	memcpy(remote_addr, result->ai_addr, sizeof(struct sockaddr_storage));

	parse_sockaddr(remote_addr, remote_ip, &remote_port);
	fprintf(stderr, "Connecting to %s:%d (addr family: %d)\n",
				remote_ip, remote_port, addr_fam);
	connect(server_socket, remote_addr, addr_len);

	freeaddrinfo(result);

	addr_len = sizeof(struct sockaddr_storage);
	s = getsockname(server_socket, local_addr, &addr_len);
	parse_sockaddr(local_addr, local_ip, &local_port);
	fprintf(stderr, "Local socket info: %s:%d (addr family: %d)\n",
			local_ip, local_port, addr_fam);

	printf("Trying to send to new server socket \n");
	ssize_t nwritten = send(server_socket, modified_request, strlen(modified_request), 0);
	if (nwritten < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	printf("Sent %zd bytes: %s\n", strlen(modified_request), modified_request);

	char server_response[16384];
	nread = 0;
	totread = 0;
	while (1) {
		nread = recv(server_socket, &server_response[totread], 16384 - totread, 0);
		if (nread < 0) {
			perror("receiving message");
			exit(EXIT_FAILURE);
		} 
		else if (nread == 0) {
			break;
		}
		totread += nread;
		printf("Read: %ld bytes\n", nread);
	}

	server_response[totread] = '\0';
	printf("Read %zd bytes: %s\n", totread, server_response);
	// print_bytes((unsigned char *)server_response, totread);
	close(server_socket);

	send(client_socket, server_response, totread, 0);
	close(client_socket);
}

void test_parser() {
	int i;
	char method[16], hostname[64], port[8], path[64];

       	char *reqs[] = {
		"GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

		NULL
	};
	
	for (i = 0; reqs[i] != NULL; i++) {
		printf("Testing %s\n", reqs[i]);
		if (complete_request_received(reqs[i])) {
			printf("REQUEST COMPLETE\n");
			parse_request(reqs[i], method, hostname, port, path);
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
		} else {
			printf("REQUEST INCOMPLETE\n");
		}
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
	fflush(stdout);
}