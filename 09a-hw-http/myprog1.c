#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    char *content_length = getenv("CONTENT_LENGTH");
    int c_length = 0;
    if (content_length != NULL) {
        c_length = atoi(content_length);
    }
    char *query_string = getenv("QUERY_STRING");

    char request_body[50];
    int nread = 0;
	nread = read(stdin, request_body, c_length);
    request_body[nread] = '\0';

    char response_body[512];
    sprintf(response_body, "Hello CS324\nQuery string: %s\nRequest body: %s\n", query_string, request_body);

    fprintf(stdout, "Content-type: text/plain\r\n");
    fprintf(stdout, "Content-length: %ld\r\n\r\n", strlen(response_body));
    fprintf(stdout, "%s", response_body);

}