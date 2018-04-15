#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

typedef struct pinfo {
	int exitcode;
	struct timeval utime;
	struct timeval stime;
} pinfo;

void chexst(pid_t pid, pinfo *prinf) {
	pid_t wpid;
	struct rusage rusage;
	int status;
	do {
		wpid = wait4(pid, &status, 0, &rusage);
	} while((wpid == -1) && (errno == EINTR));
	if(wpid == -1) {
		fprintf(stderr, "Error: couldn't wait for process with pid %d:%s\n", pid, strerror(errno));
		exit(EXIT_FAILURE);
	} else {
		if(!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
			prinf->exitcode = 1;
		} else {
			prinf->exitcode = 0;
		}
		prinf->utime = rusage.ru_utime;
		prinf->stime = rusage.ru_stime;
	}
	return;
}

int main(void) {
	const char ret_args[8][6] = {
		"false", "false",
		"false", "true",
		"true", "false",
		"true", "true"
	};
	const char op_args[3][3] = {
		"&&",
		"||",
		";"
	};
	const int results[12] = {0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1};
	pid_t pid[12];
	for(int i = 0; i < 12; i++) {
		pid[i] = fork();
		if(pid[i] < 0) {
			perror("Couldn't fork");
			_exit(EXIT_FAILURE);
		} else {
			if(pid[i] == 0) {
				execlp("./shell_andor", "./shell_andor", ret_args[(2*i)%8], "", op_args[i/4], ret_args[(2*i+1)%8], "", NULL);
				fprintf(stderr, "Couldn't execute shell_andor\n");
				_exit(EXIT_FAILURE);	   
			} else {
				continue;
			}
		}
	}
	pinfo prinf;
	int errcount = 0;
	for(int i = 0; i < 12; i++) {
		chexst(pid[i], &prinf);
		if(prinf.exitcode == results[i]) {
			printf("\x1B[31mTest %d returned incorrect results\x1B[0m\n", i + 1);
			errcount++;
		} else {
			printf("\x1B[32mReturn test %d passed!\x1B[0m\n", i + 1);
		}

	}
	if(errcount > 0) {
		printf("Return values were incorrect in %d tests, test failed\n", errcount);
		return 1;
	} else {
		const char t_ops[3][3] = {
			"&&",
			"||",
			";"
		};
		const char t_args[12][6] = {
			"sleep", "sleep",
			"false", "sleep",
			"sleep", "sleep",
			"true", "sleep",
			"true", "sleep",
			"false", "sleep"
		};
		const long int res[6] = {8, 0, 4, 0, 4, 4};
		for(int i = 0; i < 6; i++) {
			time_t start = time(NULL);
			pid_t tpid = fork();
			if(tpid < 0) {
				perror("Couldn't fork");
				_exit(EXIT_FAILURE);
			} else if(tpid == 0) {
				execlp("./shell_andor", "./shell_andor", t_args[2*i], "4", t_ops[i/2], t_args[2*i+1], "4", NULL);
				fprintf(stderr, "Couldn't execute shell_andor\n");
				_exit(EXIT_FAILURE);
			} else {
				chexst(tpid, &prinf);
				time_t end = time(NULL);
				time_t eta = end - start;
				if(labs(eta - res[i]) <= 1) {
					printf("\x1B[32mTime test %d passed\x1B[0m\n", i + 1);
				} else {
					printf("\x1B[31mTime test %d failed\x1B[0m\n", i + 1);
					errcount++;
				}
				printf("Elapsed time was %ld, while expected time was %ld\n", eta, res[i]);
			}
		}
		if(errcount > 0) {
			printf("Worktimes weve inclorrtect in %d tests, test failed\n", errcount);
			return 1;
		} else {
			return 0;
		}
	}
}
