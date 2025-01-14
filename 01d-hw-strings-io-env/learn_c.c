#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>

#define BUFSIZE 30

void memprint(char *, char *, int);

void intro();
void part1();
void part2();
void part3();
void part4();
void part5(char *);
void part6();

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		exit(1);
	}
	intro();
	part1();
	part2();
	part3();
	part4();
	part5(argv[1]);
	part6();
}

void memprint(char *s, char *fmt, int len) {
	// iterate through each byte/character of s, and print each out
	int i;
	char fmt_with_space[8];

	sprintf(fmt_with_space, "%s ", fmt);
	for (i = 0; i < len; i++) {
		printf(fmt_with_space, s[i]);
	}
	printf("\n");
}

void intro() {
	printf("===== Intro =====\n");

	// Note: STDOUT_FILENO is defined in /usr/include/unistd.h:
	// #define	STDOUT_FILENO	1

	char s1[] = { 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x0a };
	write(STDOUT_FILENO, s1, 6);

	char s2[] = { 0xe5, 0x8f, 0xb0, 0xe7, 0x81, 0xa3, 0x0a };
	write(STDOUT_FILENO, s2, 7);

	char s3[] = { 0xf0, 0x9f, 0x98, 0x82, 0x0a };
	write(STDOUT_FILENO, s3, 5);
}

void part1() {

	printf("===== Question 1 =====\n");
	char s1[] = "hello";
	int s1_len = sizeof(s1);
	printf("%d\n", s1_len);

	printf("===== Question 2 =====\n");
	memprint(s1, "%02x", s1_len);
	memprint(s1, "%d", s1_len);
	memprint(s1, "%c", s1_len);

	printf("===== Question 3 (no code changes) =====\n");

	printf("===== Question 4 (no code changes) =====\n");

	printf("===== Question 5 =====\n");
	char s2[10];
	int s2_len = sizeof(s2);
	printf("%d\n", s2_len);

	printf("===== Question 6 =====\n");
	char *s3 = s1;
	int s3_len = sizeof(s3);
	printf("%d\n", s3_len);

	printf("===== Question 7 =====\n");
	char *s4 = malloc(1024 * sizeof(char));
	int s4_len = sizeof(s4);
	printf("%d\n", s4_len);

	printf("===== Question 8 (no code changes) =====\n");

	printf("===== Question 9 =====\n");
	free(s4);
}

void part2() {
	char s1[] = "hello";
	char s2[1024];
	char *s3 = s1;

	// Copy sizeof(s1) bytes of s1 to s2.
	memcpy(s2, s1, sizeof(s1));

	printf("===== Question 10 =====\n");
	printf("%lu\n", (long unsigned int)&s1);
	printf("%lu\n", (long unsigned int)&s2);
	printf("%lu\n", (long unsigned int)&s3);

	printf("===== Question 11 =====\n");
	printf("%lu\n", (long unsigned int)s1);
	printf("%lu\n", (long unsigned int)s2);
	printf("%lu\n", (long unsigned int)s3);

	printf("===== Question 12 (no code changes) =====\n");

	printf("===== Question 13 =====\n");
	printf("%s\n", s1);
	printf("%s\n", s2);
	printf("%s\n", s3);

	printf("===== Question 14 =====\n");
	if(&s1[0] == &s2[0]) {
		printf("s1 == s2\n");
	}
	if(&s1[0] == &s3[0]) {
		printf("s1 == s3\n");
	}
	if(&s2[0] == &s3[0]) {
		printf("s2 == s3\n");
	}

	printf("===== Question 15 =====\n");
	if(strcmp(s1, s2) == 0) {
		printf("s1 == s2\n");
	}
	if(strcmp(s1, s3) == 0) {
		printf("s1 == s3\n");
	}
	if(strcmp(s2, s3) == 0) {
		printf("s2 == s3\n");
	}

	printf("===== Question 16 =====\n");
	s1[1] = 'u';
	printf("%s\n", s1);
	printf("%s\n", s2);
	printf("%s\n", s3);

	printf("===== Question 17 =====\n");
	if(&s1[0] == &s2[0]) {
		printf("s1 == s2\n");
	}
	if(&s1[0] == &s3[0]) {
		printf("s1 == s3\n");
	}
	if(&s2[0] == &s3[0]) {
		printf("s2 == s3\n");
	}

	printf("===== Question 18 =====\n");
	if(strcmp(s1, s2) == 0) {
		printf("s1 == s2\n");
	}
	if(strcmp(s1, s3) == 0) {
		printf("s1 == s3\n");
	}
	if(strcmp(s2, s3) == 0) {
		printf("s2 == s3\n");
	}
}

void part3() {
	char s1[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	char s2[] = { 97, 98, 99, 100, 101, 102 };
	char s3[] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

	printf("===== Question 19 =====\n");
	if(memcmp(s1, s2, sizeof(s1)) == 0) {
		printf("s1 == s2\n");
	}
	if(memcmp(s1, s3, sizeof(s1)) == 0) {
		printf("s1 == s3\n");
	}
	if(memcmp(s2, s3, sizeof(s2)) == 0) {
		printf("s2 == s3\n");
	}
}

void part4() {
	char s1[] = { 'a', 'b', 'c', '\0', 'd', 'e', 'f', '\0' };
	char s2[] = { 'a', 'b', 'c', '\0', 'x', 'y', 'z', '\0' };

	printf("===== Question 20 =====\n");
	if(memcmp(s1, s2, sizeof(s1)) == 0) {
		printf("s1 == s2\n");
	}

	printf("===== Question 21 =====\n");
	if(strcmp(s1, s2) == 0) {
		printf("s1 == s2\n");
	}

	printf("===== Question 22 =====\n");
	char s3[16];
	memset(s3, 'z', sizeof(s3));
	memprint(s3, "%02x", sizeof(s3));
	char s4[16];
	memset(s4, 'z', sizeof(s4));
	memprint(s4, "%02x", sizeof(s4));

	printf("===== Question 23 =====\n");
	strcpy(s3, s1);
	memprint(s3, "%02x", sizeof(s3));

	printf("===== Question 24 =====\n");
	int myval = 42;
	sprintf(s4, "%s %d\n", s1, myval);
	memprint(s4, "%02x", sizeof(s4));

	printf("===== Question 25 =====\n");
	char *s5;
	char *s6 = NULL;
	char *s7 = s4;
	memprint(s1, "%02x", 8);
	memprint(s3, "%02x", 8);
	memprint(s7, "%02x", 8);
}

void part5(char *filename) {
	printf("===== Question 26 =====\n");
	printf("stdin: %d\n", fileno(stdin));
	printf("stdout: %d\n", fileno(stdout));
	printf("stderr: %d\n", fileno(stderr));

	printf("===== Question 27 =====\n");

	char buf[BUFSIZE];
	memset(buf, 'z', sizeof(buf));
	buf[24] = '\0';
	memprint(buf, "%02x", sizeof(buf));

	printf("===== Question 28 =====\n");
	printf("%s\n", buf);
	write(STDOUT_FILENO, buf, BUFSIZE);
	printf("\n");

	fprintf(stderr, "===== Question 29 =====\n");
	fprintf(stderr, buf, BUFSIZE);
	fprintf(stderr, "\n");
	write(STDERR_FILENO, buf, BUFSIZE);
	fprintf(stderr, "\n");

	printf("===== Question 30 (no code changes) =====\n");

	printf("===== Question 31 =====\n");
	int fd1, fd2;
	fd1 = open(filename, O_RDONLY);
	fd2 = fd1;
	printf("%d\n", fd1);
	printf("%d\n", fd2);

	printf("===== Question 32 =====\n");
	size_t nread = 0;
	size_t totread = 0;
	nread = read(fd1, buf, 4);
	totread += nread;
	printf("%lu\n", nread);
	printf("%lu\n", totread);
	memprint(buf, "%02x", BUFSIZE);

	printf("===== Question 33 (no code changes) =====\n");

	printf("===== Question 34 =====\n");
	nread = read(fd2, &buf[totread], 4);
	totread += nread;
	printf("%lu\n", nread);
	printf("%lu\n", totread);
	memprint(buf, "%02x", BUFSIZE);

	printf("===== Question 35 (no code changes) =====\n");

	printf("===== Question 36 (no code changes) =====\n");

	printf("===== Question 37 =====\n");
	nread = read(fd2, &buf[totread], BUFSIZE - totread);
	totread += nread;
	printf("%lu\n", nread);
	printf("%lu\n", totread);
	memprint(buf, "%02x", BUFSIZE);

	printf("===== Question 38 (no code changes) =====\n");

	printf("===== Question 39 (no code changes) =====\n");

	printf("===== Question 40 (no code changes) =====\n");

	printf("===== Question 41 =====\n");
	nread = read(fd2, &buf[totread], BUFSIZE - totread);
	totread += nread;
	printf("%lu\n", nread);
	printf("%lu\n", totread);
	memprint(buf, "%02x", BUFSIZE);

	printf("===== Question 42 =====\n");
	printf("%s\n", buf);

	printf("===== Question 43 =====\n");
	buf[totread] = '\0';
	printf("%s\n", buf);

	printf("===== Question 44 =====\n");
	int closeVal = close(fd1);
	printf("%d\n", closeVal);

	printf("===== Question 45 =====\n");
	int ret = close(fd2);
	printf("%d\n", ret);

	printf("===== Question 46 =====\n");
	fprintf(stdout, "abc");
	fprintf(stderr, "def");
	fprintf(stdout, "ghi\n");
	write(STDOUT_FILENO, "abc", 3);
	write(STDERR_FILENO, "def", 3);
	write(STDOUT_FILENO, "ghi\n", 4);

	printf("===== Question 47 =====\n");
	fprintf(stdout, "abc");
	fflush(stdout);
	fprintf(stderr, "def");
	fprintf(stdout, "ghi\n");
	write(STDOUT_FILENO, "abc", 3);
	write(STDERR_FILENO, "def", 3);
	write(STDOUT_FILENO, "ghi\n", 4);
}

void part6() {
	printf("===== Question 48 =====\n");
	char *s1 = getenv("CS324_VAR");
	if(s1 != NULL) {
		printf("CS324_VAR is %s\n", s1);
	} else {
		printf("CS324_VAR not found\n");
	}

	printf("===== Question 49 (no code changes) =====\n");
	
}
