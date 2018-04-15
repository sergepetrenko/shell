#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define MAXINTLEN 12

char *pidtostr(pid_t pid) {
	char *str = (char *)malloc(MAXINTLEN * sizeof(char));
	char *tmp = str + MAXINTLEN;
	str[MAXINTLEN - 1] = '\0';
	while(pid > 0) {
		tmp--;
		*tmp  = (pid % 10) + '0';
		pid /= 10;
	}
	strcpy(str, tmp);
	str = (char *)realloc(str, strlen(str) + 1);
	return str;
}

int main(void) {
	pid_t pid;
	int inpipefd[2], outpipefd[2];
	if(pipe(inpipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if(pipe(outpipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if((pid = fork()) == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	} else if(pid == 0) {
		if(dup2(inpipefd[0], 0) == -1) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		if(dup2(outpipefd[1], 1) == -1) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		close(inpipefd[0]);
		close(inpipefd[1]);
		close(outpipefd[0]);
		close(outpipefd[1]);
		execlp("./shell_pipe", "./shell_pipe", NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	}
	close(inpipefd[0]);
	close(outpipefd[1]);
	char tbuf[30] = "sleep 2 | sleep 2 | sleep 2\n";
	char rbuf[80], rbuf1[80], rbuf2[160];
	int passed = 0, failed = 0;
	printf("Parrallel execution test...\n");
	write(inpipefd[1], tbuf, 29);
	time_t start = time(NULL);
	read(outpipefd[0], rbuf, 80);
	printf("%s\n", rbuf);
	time_t end = time(NULL);
	if(end - start == 2) {
		printf("\x1B[32mParrallel execution test passed: execution time was %ld, just as expected!\n\x1B[0m", end - start);
		passed++;
	} else {
		printf("\x1B[31mParrallel execution test failed: execution time was %ld, which wasn't expected.\n\x1B[0m", end - start);
		failed++;
	}
	printf("Piping test...\n");
	char tbuf1[14] = "ls | ls | wc\n";
	write(inpipefd[1], tbuf1, 13);
	char tbuf2[15] = "ls | wc | cat\n";
	write(inpipefd[1], tbuf2, 14);
	sleep(1);
	read(outpipefd[0], rbuf, 80);
	write(inpipefd[1], tbuf1, 13);
	sleep(1);
	read(outpipefd[0], rbuf1, 80);
	rbuf1[27] = '\0';
	rbuf[27] = '\0';
	if(strcmp(rbuf, rbuf1) == 0) {
		printf("\x1B[32mPiping test passed, everything is connected properly\n\x1B[0m");
		passed++;
	} else {
		printf("\x1B[31mPiping test failed, something is connected in a wrong way\n\x1B[0m");
		failed++;
	}
	char tbuf3[6] = "ls &\n";
	printf("Background and zombie collecting test...\n");
	for(int i = 0; i < 3; i++) {
		write(inpipefd[1], tbuf3, 5);
	}
	write(inpipefd[1], tbuf1, 13);
	write(inpipefd[1], tbuf1, 13);
	sleep(1);
	pid_t pid2;
	int pspipe[2];
	if(pipe(pspipe) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	if((pid2 = fork()) == -1) {
		perror("Couldn't fork");
	} else if(pid2 == 0) {
		if(dup2(pspipe[1], 1) == -1) {
			perror("dup2");
			exit(EXIT_FAILURE);
		}
		execlp("ps", "ps", "--no-headers", "--ppid", pidtostr(pid), NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	}
	close(pspipe[1]);
	sleep(2);
	read(pspipe[0], rbuf2, 160);
	if(strlen(rbuf2) == 0) {
		printf("\x1B[32mBackground and zombie collecting test passed!\n\x1B[0m");
		passed++;
	} else {
		printf("\x1B[31mBackground and zombie collecting test failed.\n\x1B[0m");
		failed++;
	}
	printf("Out of %d tests, %d were successful and %d failed, which means that ", passed + failed, passed, failed);
	if(failed == 0) {
		printf("program passed all the tests!\n");
		close(pspipe[0]);
		close(inpipefd[1]);
		close(inpipefd[0]);
		wait(NULL);
		return 0;
	} else {
		printf("program failed the test.\n");
		close(pspipe[0]);
		close(inpipefd[1]);
		close(inpipefd[0]);
		wait(NULL);
		return 1;
	}
}
