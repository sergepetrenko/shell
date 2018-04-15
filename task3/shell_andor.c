#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	if(argc < 6) {
		fprintf(stderr, "Not enough arguments passed\n");
		return 1;
	} else {
		const pid_t pid1 = fork();
		if(pid1 < 0) {
			perror("Couldn't create a child process");
			return 1;
		} else if(pid1 == 0) {
			execlp(argv[1], argv[1], argv[2], NULL);
			fprintf(stderr, "Couldn't execute %s:%s\n", argv[1], strerror(errno));
			return 1;
		} else {
			pid_t pid2;
			int status;
			do {
				pid2 = wait(&status);
			} while((pid2 == -1) && ((errno == EINTR) || (errno == ERESTART)));
			if(pid2 == -1) {
				fprintf(stderr, "Couldn't wait for %s:%s\n", argv[1], strerror(errno));
				return 1;
			} else if(!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
				if(strcmp(argv[3],"&&") == 0) {
					return 1;
				} else {
					const pid_t pid3 = fork();
					if(pid3 < 0) {
						perror("Couldn't create a child process");
						return 1;
					} else if(pid3 == 0) {
						execlp(argv[4], argv[4], argv[5], NULL);
						fprintf(stderr, "Couldn't execute %s:%s\n", argv[4], strerror(errno));
						return 1;
					} else {
						pid_t pid4;
						do {
							pid4 = wait(&status);
						} while((pid4 == -1) && ((errno == EINTR) || (errno == ERESTART)));
						if(pid4 == -1) {
							fprintf(stderr, "Couldn't wait for %s:%s\n", argv[4], strerror(errno));
							return 1;
						} else if(!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
							return 1;
						} else {
							return 0;
						}
					}
				}
			} else {
				if(strcmp(argv[3], "||") == 0) {
					return 0;
				} else {
					const pid_t pid5 = fork();
					if(pid5 < 0) {
						perror("Couldn't create a child process");
						return 1;
					} else if(pid5 == 0) {
						execlp(argv[4], argv[4], argv[5], NULL);
						fprintf(stderr, "Couldn't execute %s:%s\n", argv[4], strerror(errno));
						return 1;
					} else {
						pid_t pid6;
						do {
							pid6 = wait(&status);
						} while((pid6 == -1) && (errno == EINTR));
						if(pid6 == -1) {
							fprintf(stderr, "Couldn't wait for %s:%s\n", argv[4], strerror(errno));
							return 1;
						} else if(!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
							if(strcmp(argv[3], ";") == 0) {
								return 0;
							} else {
								return 1;
							}
						} else {
							return 0;
						}
					}
				}
			}
		}
	}
	return 0;
}
