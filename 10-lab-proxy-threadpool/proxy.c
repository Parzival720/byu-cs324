#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sockhelper.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
int open_sfd(char **);
void handle_client();
void test_parser();
void print_bytes(unsigned char *, int);

int main(int argc, char *argv[])
{
	int sfd = open_sfd(argv);
	while (1) {

		int temp_socket;
		struct sockaddr_storage remote_addr_ss;
		struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		temp_socket = accept(sfd, remote_addr, &addr_len);
		
		handle_client(temp_socket);
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
		start_of_thing = end_of_thing + 1;
	} else {
		strncpy(hostname, start_of_thing, end_of_thing - start_of_thing);
		start_of_thing = end_of_thing + 1;

		end_of_thing = strstr(start_of_thing, "/");
		strncpy(port, start_of_thing, end_of_thing - start_of_thing);
		start_of_thing = end_of_thing + 1;
	}

	end_of_thing = strstr(start_of_thing, " ");
	strncpy(path, start_of_thing, end_of_thing - start_of_thing);
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

void handle_client(int socket) {
	char buf[1024];
	memset(buf, '\0', 1024);
	ssize_t totread = 0;
	ssize_t nread = 0;

	// printf("Handling client\n");
	
	while (1) {
		nread = recv(socket, &buf[totread], 1024 - totread, 0);
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

	close(socket);

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
	print_bytes((unsigned char *)modified_request, strlen(modified_request));
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
