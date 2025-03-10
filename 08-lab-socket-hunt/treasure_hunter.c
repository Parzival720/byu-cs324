// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823691275
#define BUFSIZE 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "sockhelper.h"

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: %s server port level seed\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *server = argv[1];
	char *port = argv[2];
	int portInt = atoi(port);
	int level = atoi(argv[3]);
	int seed = atoi(argv[4]);

	// printf("Server: %s\n", server);
	// printf("Port: %d\n", portInt);
	// printf("Level: %d\n", level);
	// printf("Seed: %d\n", seed);
	
	unsigned char buf[BUFSIZE];
	bzero(buf, BUFSIZE);
	buf[0] = 0;
	buf[1] = level;
	unsigned int val1 = htonl(USERID);
	memcpy(&buf[2], &val1, sizeof(unsigned int));
	unsigned short val2 = htons(seed);
	memcpy(&buf[6], &val2, sizeof(unsigned short));

	// print_bytes(buf, BUFSIZE);

	int af = AF_INET;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_DGRAM;

	struct addrinfo *result;
	int s;
	s = getaddrinfo(server, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	int sfd;
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

	sfd = socket(result->ai_family, result->ai_socktype, 0);
	addr_fam = result->ai_family;
	addr_len = result->ai_addrlen;
	memcpy(remote_addr, result->ai_addr, sizeof(struct sockaddr_storage));
	parse_sockaddr(remote_addr, remote_ip, &remote_port);
	populate_sockaddr(remote_addr, addr_fam, remote_ip, remote_port);

	addr_len = sizeof(struct sockaddr_storage);
	s = getsockname(sfd, local_addr, &addr_len);
	parse_sockaddr(local_addr, local_ip, &local_port);
	// fprintf(stderr, "Local socket info: %s:%d (addr family: %d)\n", local_ip, local_port, addr_fam);
	

	ssize_t nwritten = sendto(sfd, buf, BUFSIZE, 0, remote_addr, addr_len);
	if (nwritten < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}
	// printf("Sent %d bytes: %s\n", BUFSIZE, buf);

	char treasure[1024];
	int totread = 0;

	while(1) {
		unsigned char buf2[256];
		ssize_t nread = recvfrom(sfd, buf2, 256, 0, remote_addr, &addr_len);
		buf2[nread] = '\0';
		if (nread < 0) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		// printf("Received %zd bytes: %s\n", nread, buf2);
		// print_bytes(buf2, nread);
		
		unsigned char chunk_length = buf2[0];
		if (chunk_length == 0) {
			break;
		}
		char chunk[chunk_length + 1];
		int i;
		for (i = 0; i < chunk_length; i++) {
			chunk[i] = buf2[i + 1];
			treasure[totread + i] = chunk[i];
		}
		chunk[i] = '\0';
		totread += chunk_length;

		unsigned char op_code = buf2[chunk_length + 1];
		unsigned short op_param = 0;
		memcpy(&op_param, &buf2[chunk_length + 2], sizeof(unsigned short));
		op_param = ntohs(op_param);
		unsigned int nonce = 0;
		memcpy(&nonce, &buf2[chunk_length + 4], sizeof(unsigned int));
		nonce = ntohl(nonce);

		// printf("%x\n", chunk_length);
		// printf("%s\n", chunk);
		// printf("%x\n", op_code);
		// printf("%x\n", op_param);
		// printf("%x\n", nonce);

		if (op_code == 1) {
			remote_port = op_param;
			populate_sockaddr(remote_addr, addr_fam, remote_ip, remote_port);
		}
		else if (op_code == 2) {
			close(sfd);
			local_port = op_param;
			populate_sockaddr(local_addr, addr_fam, NULL, local_port);
			sfd = socket(result->ai_family, result->ai_socktype, 0);
			if (bind(sfd, local_addr, sizeof(struct sockaddr_storage)) < 0) {
				perror("bind()");
			}
		}
		else if (op_code == 3) {
			struct sockaddr_storage remote_addr_ss_temp;
			struct sockaddr *remote_addr_temp = (struct sockaddr *)&remote_addr_ss_temp;
			memcpy(remote_addr_temp, result->ai_addr, sizeof(struct sockaddr_storage));
			parse_sockaddr(remote_addr_temp, remote_ip, &remote_port);
			populate_sockaddr(remote_addr_temp, addr_fam, remote_ip, remote_port);
			unsigned short temp_remote_port;
			unsigned int sum = 0;
			for (int i = 0; i < op_param; i++) {
				unsigned char tempbuf[256];
				ssize_t nread = recvfrom(sfd, tempbuf, 256, 0, remote_addr_temp, &addr_len);
				parse_sockaddr(remote_addr_temp, remote_ip, &temp_remote_port);
				sum += temp_remote_port;
				tempbuf[nread] = '\0';
				// printf("Temp port: %x", temp_remote_port);
				// parse_sockaddr
				// if (nread < 0) {
				// 	perror("read");
				// 	exit(EXIT_FAILURE);
				// }
			}
			nonce = sum;
		}
		else if (op_code == 4) {
			close(sfd);
			remote_port = op_param;
			populate_sockaddr(remote_addr, addr_fam, remote_ip, remote_port);
			if (hints.ai_family == AF_INET) {
				hints.ai_family = AF_INET6;
			} 
			else {
				hints.ai_family = AF_INET;
			}
			char port_string[8];
			sprintf(port_string, "%d", remote_port);
			struct addrinfo *new_result;
			s = getaddrinfo(server, port_string, &hints, &new_result);
			sfd = socket(new_result->ai_family, new_result->ai_socktype, 0);
			addr_fam = new_result->ai_family;
			addr_len = new_result->ai_addrlen;
			memcpy(remote_addr, new_result->ai_addr, sizeof(struct sockaddr_storage));
			parse_sockaddr(remote_addr, remote_ip, &remote_port);
			populate_sockaddr(remote_addr, addr_fam, remote_ip, remote_port);
			addr_len = sizeof(struct sockaddr_storage);
			s = getsockname(sfd, local_addr, &addr_len);
			parse_sockaddr(local_addr, local_ip, &local_port);
		}
	
		unsigned char return_msg[4]; 
		bzero(return_msg, 4);
		unsigned int new_nonce = htonl(nonce + 1);
		memcpy(&return_msg[0], &new_nonce, sizeof(unsigned int));
		// print_bytes(return_msg, 4);
	
		nwritten = sendto(sfd, return_msg, 4, 0, remote_addr, addr_len);
		if (nwritten < 0) {
			perror("send");
			exit(EXIT_FAILURE);
		}
		// printf("Sent %d bytes: %s\n", BUFSIZE, buf);

	}

	treasure[totread] = '\0';
	printf("%s\n", treasure);
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
