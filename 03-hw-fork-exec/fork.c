#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
	int pid;
	FILE *filestream;
	int fd;
	int pipefd[2];
	char buf[64];
	char *newenviron[] = { NULL };
	// int child_status;

	printf("Starting program; process has pid %d\n", getpid());

	filestream = fopen("fork-output.txt", "w");
	fd = fileno(filestream);
	fprintf(filestream, "BEFORE FORK (%d)\n", fd);
	fflush(filestream);

	if(pipe(pipefd) < 0) {
		fprintf(stderr, "Could not create pipe");
		exit(1);
	}

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */

	printf("Section A;  pid %d\n", getpid());
	// sleep(5);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

		printf("Section B\n");
		// sleep(5);
		fprintf(filestream, "SECTION B (%d)\n", fd);
		fflush(filestream);

		close(pipefd[0]);
		sleep(10);
		write(pipefd[1], "hello from Section B", 21);
		sleep(10);
		close(pipefd[1]);
		// sleep(30);
		// sleep(30);
		// printf("Section B done sleeping\n");

		printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());

		if (argc <= 1) {
			printf("No program to exec.  Exiting...\n");
			exit(0);
		}

		printf("Running exec of \"%s\"\n", argv[1]);
		dup2(fileno(filestream), STDOUT_FILENO);
		execve(argv[1], &argv[1], newenviron);
		printf("End of program \"%s\".\n", argv[0]);

		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */

		printf("Section C\n");
		fprintf(filestream, "SECTION C (%d)\n", fd);
		fclose(filestream);
		close(pipefd[1]);
		int nread = 0;
		int totread = 0;
		nread = read(pipefd[0], buf, 48);
		totread += nread;
		buf[totread + 1] = '\0';
		printf("Received %d bytes\n", nread);
		printf("%s\n", buf);

		nread = read(pipefd[0], buf, 48);
		totread += nread;
		buf[totread + 1] = '\0';
		printf("Received %d bytes\n", nread);
		close(pipefd[0]);
		// sleep(5);
		// waitpid(pid, &child_status, 0);
		// sleep(30);
		// printf("Section C done sleeping\n");

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
	// sleep(30);

	/* END SECTION D */
}

