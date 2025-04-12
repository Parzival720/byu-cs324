#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "sockhelper.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define MAXEVENTS 64
#define MAXLINE 1024
#define MAXRESPONSE 16384
#define READ_REQUEST 1
#define SEND_REQUEST 2
#define READ_RESPONSE 3
#define SEND_RESPONSE 4

struct request_info{
	int clientToProxy;
	int proxyToServer;
	int requestState;
	char readRequestBuffer[1024];
	char writeRequestBuffer[1024];
	char readResponseBuffer[16384];
	char writeResponseBuffer[16384];
	int bytesReadClient;
	int bytesToWriteServer;
	int bytesWrittenServer;
	int bytesReadServer;
	int bytesWrittenClient;
};

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
int open_sfd(char *[]);
void handle_new_clients(int, int);
void handle_client(int, struct request_info*);
void test_parser();
void print_bytes(unsigned char *, int);


int main(int argc, char *argv[])
{
	/* Check usage */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int efd;
	if ((efd = epoll_create1(0)) < 0) {
		perror("Error with epoll_create1");
		exit(EXIT_FAILURE);
	}

	int sfd = open_sfd(argv);

	// allocate memory for a new struct client_info, and populate it with
	// info for the listening socket
	struct request_info *listener =
		malloc(sizeof(struct request_info));
	listener->clientToProxy = sfd;

	// register the listening file descriptor for incoming events using
	// edge-triggered monitoring
	struct epoll_event event;
	event.data.ptr = listener;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(EXIT_FAILURE);
	}

	struct epoll_event events[MAXEVENTS];
	while(1) {
		// wait for event to happen (-1 == no timeout)
		// printf("before epoll_wait()\n"); fflush(stdout);
		int n = epoll_wait(efd, events, MAXEVENTS, 1000);
		// printf("after epoll_wait()\n"); fflush(stdout);

		if (n < 0) {
			printf("Error from epoll_wait: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < n; i++) {
			// grab the data structure from the event, and cast it
			// (appropriately) to a struct request_info *.
			struct request_info *active_client =
				(struct request_info *)(events[i].data.ptr);

			if ((events[i].events & EPOLLERR) ||
				(events[i].events & EPOLLHUP) ||
				(events[i].events & EPOLLRDHUP)) {
				/* An error has occured on this fd */
				close(active_client->clientToProxy);
				free(active_client);
				continue;
			}
			
			if (sfd == active_client->clientToProxy) {
				handle_new_clients(sfd, efd);
			} else {
				handle_client(efd, active_client);
			}
		}
	}
	free(listener);
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

	// set listening file descriptor nonblocking
	if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

	printf("Listening on socket %d\n", port);

	return sfd;
}

void handle_new_clients(int sfd, int efd) {
	while(1) {
		int connfd;
		struct sockaddr_storage remote_addr_ss;
		struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
		char remote_ip[INET6_ADDRSTRLEN];
		unsigned short remote_port;
		socklen_t addr_len = sizeof(struct sockaddr_storage);
		connfd = accept(sfd, remote_addr, &addr_len);

		if (connfd < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				// no more clients ready to accept
				break;
			} else {
				perror("accept");
				exit(EXIT_FAILURE);
			}
		}

		parse_sockaddr(remote_addr, remote_ip, &remote_port);
		printf("Connection from %s:%d\n", remote_ip, remote_port);

		// set client file descriptor nonblocking
		if (fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}

		// allocate memory for a new struct
		// request_info, and populate it with
		// info for the new client
		struct request_info *new_client =
			(struct request_info *)malloc(sizeof(struct request_info));
		new_client->clientToProxy = connfd;
		new_client->proxyToServer = 0;
		new_client->requestState = READ_REQUEST;
		new_client->bytesReadClient = 0;
		new_client->bytesToWriteServer = 0;
		new_client->bytesWrittenServer = 0;
		new_client->bytesReadServer = 0;
		new_client->bytesWrittenClient = 0;
		char buf[1024];
		memset(buf, '\0', 1024);
		sprintf(new_client->readRequestBuffer, "%s", buf);
		sprintf(new_client->writeRequestBuffer, "%s", buf);

		// register the client file descriptor for incoming events
		// using edge-triggered monitoring
		struct epoll_event event;
		event.data.ptr = new_client;
		event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		printf("New client: %d\n", connfd);
	}
}

void handle_client(int efd, struct request_info *request) {
	printf("File descriptor: %d\n", request->clientToProxy);
	printf("Request state: %d\n", request->requestState);

	if (request->requestState == READ_REQUEST) {
		while (1) {
			int len = recv(request->clientToProxy, &request->readRequestBuffer[request->bytesReadClient], MAXLINE - request->bytesReadClient, 0);
			if (len < 0) {
				if (errno == EWOULDBLOCK || errno == EAGAIN) {
					// no more data to be read
					return;
				} else {
				
					perror("client recv");
					close(request->clientToProxy);
					free(request);
					return;
				}
				break;
			} else {
				request->bytesReadClient += len;
				printf("Received %d bytes (total: %d)\n", len, request->bytesReadClient);
				if (strstr(request->readRequestBuffer, "\r\n\r\n") != NULL) {
					// char *pos = strstr(request->readRequestBuffer, "\r\n\r\n");
					// printf("Found carriage return newline twice at position %td\n", pos - request->readRequestBuffer);
					// printf("%s\n", request->readRequestBuffer);
					// printf("%s\n", request->readBuffer);
					// printf("Total bytes read: %ld\n", totread);
					break;
				}
			}
		}

		print_bytes((unsigned char *)request->readRequestBuffer, request->bytesReadClient);

		request->readRequestBuffer[request->bytesReadClient] = '\0';

		char method[16], hostname[64], port[8], path[64];
		if (complete_request_received(request->readRequestBuffer)) {
			printf("REQUEST COMPLETE\n");
			parse_request(request->readRequestBuffer, method, hostname, port, path);
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
		} else {
			printf("REQUEST INCOMPLETE\n");
		}

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
		sprintf(request->writeRequestBuffer, "%s", modified_request);
		request->bytesToWriteServer = strlen(modified_request);

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

		// set listening file descriptor nonblocking
		if (fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0) | O_NONBLOCK) < 0) {
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}

		// deregister the client-to-proxy socket with the epoll instance that you created
		if (epoll_ctl(efd, EPOLL_CTL_DEL, request->clientToProxy, NULL) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		// Add proxy-to-server socket to request_info
		request->proxyToServer = server_socket;

		// Register the proxy-to-server socket with the epoll 
		// instance that you created, for writing (i.e., EPOLLOUT).
		struct epoll_event event;
		event.data.ptr = request;
		event.events = EPOLLOUT | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, server_socket, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		request->requestState = SEND_REQUEST;
	} else if (request->requestState == SEND_REQUEST) {
		//printf("Trying to send to new server socket \n");
		while(1) {
			int nwritten = send(request->proxyToServer, &request->writeRequestBuffer[request->bytesWrittenServer], request->bytesToWriteServer - request->bytesWrittenServer, 0);
			if (nwritten < 0) {
				if (errno == EWOULDBLOCK ||
						errno == EAGAIN) {
					// no buffer space for writing
					return;
				} else {
					perror("send");
					close(request->clientToProxy);
					close(request->proxyToServer);
					free(request);
				}
				break;
			}
			request->bytesWrittenServer += nwritten;
			printf("Sent %d/%d bytes: %s\n", request->bytesWrittenServer, request->bytesToWriteServer, request->writeRequestBuffer);
			if (request->bytesWrittenServer == request->bytesToWriteServer) {
				break;
			}
		}

		// Deregister the proxy-to-server socket with the epoll instance for writing
		if (epoll_ctl(efd, EPOLL_CTL_DEL, request->proxyToServer, NULL) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		// Register the proxy-to-server socket with the epoll instance for reading
		struct epoll_event event;
		event.data.ptr = request;
		event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, request->proxyToServer, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		request->requestState = READ_RESPONSE;
	} else if (request->requestState == READ_RESPONSE) {
		while (1) {
			int len = recv(request->proxyToServer, &request->readResponseBuffer[request->bytesReadServer], MAXRESPONSE - request->bytesReadServer, 0);
			if (len < 0) {
				if (errno == EWOULDBLOCK ||
						errno == EAGAIN) {
					// no more data to be read
					return;
				} else {
				
					perror("client recv");
					close(request->clientToProxy);
					close(request->proxyToServer);
					free(request);
				}
				break;
			} else if (len == 0) {
				break;
			} else {
				request->bytesReadServer += len;
				printf("Received %d bytes (total: %d)\n", len, request->bytesReadServer);
				
			}
		}

		close(request->proxyToServer);
		print_bytes((unsigned char *)request->readResponseBuffer, request->bytesReadServer);

		// Register the client-to-proxy socket with the epoll instance for writing
		struct epoll_event event;
		event.data.ptr = request;
		event.events = EPOLLOUT | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, request->clientToProxy, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		request->requestState = SEND_RESPONSE;
	} else if (request->requestState == SEND_RESPONSE) {
		while(1) {
			int nwritten = send(request->clientToProxy, &request->readResponseBuffer[request->bytesWrittenClient], request->bytesReadServer - request->bytesWrittenClient, 0);
			if (nwritten < 0) {
				if (errno == EWOULDBLOCK ||
						errno == EAGAIN) {
					// no buffer space for writing
					return;
				} else {
					perror("send");
					close(request->clientToProxy);
					free(request);
				}
				break;
			}
			request->bytesWrittenClient += nwritten;
			printf("Sent %d/%d  bytes: %s\n", request->bytesWrittenClient, request->bytesReadServer, request->readResponseBuffer);
			if (request->bytesWrittenClient == request->bytesReadServer) {
				break;
			}
		}

		close(request->clientToProxy);
		free(request);
	}

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
