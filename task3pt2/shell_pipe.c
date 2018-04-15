#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#define FGETS_BUFSIZE 8

char *getline_unlim(void) {
	char *tmp, *str = NULL;
	int str_size = 0, j;
	_Bool end_of_str = 0;
	do {
		str = (char*)realloc(str, str_size + FGETS_BUFSIZE);
		if(str == NULL) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		tmp = fgets(str + str_size, FGETS_BUFSIZE, stdin);
		if(tmp != NULL) {
			for(j = 0; j < FGETS_BUFSIZE; j++) {
				if (tmp[j] == '\n') {
					tmp[j] = '\0';
					end_of_str = 1;
					break;
				}
			}
		} else if(str_size == 0) {
			free(str);
			return NULL;
		}
		str_size += FGETS_BUFSIZE - 1;
	} while(tmp != NULL && !end_of_str);
	if(tmp == NULL && !feof(stdin)) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	return str;
}

typedef struct lopr {
	char *name;
	char **argv;
	pid_t pid;
	_Bool was_collected;
	struct lopr *next;
} prlist;

typedef struct lopi {
	struct lopr *load;
	int status;
	struct lopi *next;
} pilist;

_Bool wait_flag;

prlist *pradd(prlist elem, prlist *list) {
	if(list == NULL) {
		list = (prlist *)malloc(sizeof(prlist));
		list->name = elem.name;
		list->argv = elem.argv;
		list->next = NULL;
	} else {
		prlist *tmp = list;
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = (prlist *)malloc(sizeof(prlist));
		tmp = tmp->next;
		tmp->name = elem.name;
		tmp->argv = elem.argv;
		tmp->next = NULL;
	}
	return list;
}

prlist *getnextexpr(void) {
	prlist *list = NULL;
	prlist elem;
	int i = 0;
	char *str, *tmp, *tmpold, *name = NULL;
	char **argv = NULL;
	_Bool end_of_str = 0;
	str = getline_unlim();
	tmpold = str;
	tmp = str;
	elem.name = NULL;
	wait_flag = 1;
	if(str) {
		if(!*str || *str == '\n') {
			return list;
		}
		do {
			if(*tmp == ' ') {
				name = (char *)malloc(tmp - tmpold + 1);
				strncpy(name, tmpold, tmp - tmpold);
				name[tmp-tmpold] = '\0';
				if(!elem.name) {
					elem.name = name;
				}
				argv = (char **)realloc(argv, (i+1) * sizeof(char *));
				argv[i] = name;
				i++;
				tmp++;
				tmpold = tmp;
				continue;
			}
			if(*tmp == '|' || *tmp == '&') {
				argv = (char **)realloc(argv, (i+1) * sizeof(char *));
				argv[i] = NULL;
				elem.argv = argv;
				list = pradd(elem, list);
				i = 0;
				argv = NULL;
				elem.name = NULL;
				if(*tmp == '&') {
					wait_flag = 0;
					break;
				}
				tmp += 2;
				tmpold = tmp;
				continue;
			}
			if(!*tmp) {
				name = (char *)malloc(tmp - tmpold + 1);
				strncpy(name, tmpold, tmp - tmpold);
				name[tmp-tmpold] = '\0';
				if(!elem.name) {
					elem.name = name;
				}
				argv = (char **)realloc(argv, (i+2) * sizeof(char *));
				argv[i] = name;
				i++;
				argv[i] = NULL;
				elem.argv = argv;
				list = pradd(elem, list);
				end_of_str = 1;
			}
			tmp++;
		} while(!end_of_str);
		free(str);
	}
	return list;
}

void callist(prlist *list) {
	if(list != NULL) {
		int ll = 0;
		prlist *tmp;
		for(tmp = list; tmp != NULL; tmp = tmp->next) {
			ll++;
		}
		if(ll == 1) {
			list->was_collected = 0;
			pid_t pid = fork();
			if(pid == -1) {
				perror("fork");
				exit(EXIT_FAILURE);
			} else if(!pid) {
				execvp(list->name, list->argv);
			} else {
				list->pid = pid;
			}
		} else {
			int pipefd[ll-1][2];
			int i;
			tmp = list;
			pid_t pid;
			for(i = 0; i < ll - 1; i++) {
				if(pipe(pipefd[i]) == -1) {
					perror("pipe");
					exit(EXIT_FAILURE);
				}
			}
			if((pid = fork()) == -1) {
				perror("fork");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				if(dup2(pipefd[0][1], 1) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				close(pipefd[0][1]);
				execvp(tmp->name, tmp->argv);
			} else {
				close(pipefd[0][1]);
				tmp->pid = pid;
				tmp->was_collected = 0;
			}
			tmp = tmp->next;
			for(i = 1; i < ll - 1; i++) {
				if((pid = fork()) == -1) {
					perror("fork");
					exit(EXIT_FAILURE);
				} else if(pid == 0) {
					if(dup2(pipefd[i-1][0], 0) == -1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
					if(dup2(pipefd[i][1], 1) == -1) {
						perror("dup2");
						exit(EXIT_FAILURE);
					}
					close(pipefd[i-1][0]);
					close(pipefd[i][1]);
					execvp(tmp->name, tmp->argv);
				} else {
					close(pipefd[i-1][0]);
					close(pipefd[i][1]);
					tmp->pid = pid;
					tmp->was_collected = 0;
				}
				tmp = tmp->next;
			}
			if((pid = fork()) == -1) {
				perror("fork");
				exit(EXIT_FAILURE);
			} else if(pid == 0) {
				if(dup2(pipefd[i-1][0], 0) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
				close(pipefd[i-1][0]);
				execvp(tmp->name, tmp->argv);
			} else {
				close(pipefd[i-1][0]);
				tmp->pid = pid;
				tmp->was_collected = 0;
			}
		}
	}
	return;
}

void waitlist(prlist *list, _Bool wait_flag) {
	if(list) {
		int status;
		prlist *tmp = list;
		while(tmp != NULL) {
			pid_t wpid;
			if(wait_flag) {
				do {
					wpid = waitpid(tmp->pid, &status, 0);
				} while((wpid == -1) && (errno == EINTR));
				if(wpid == -1) {
					perror("waitpid");
					exit(EXIT_FAILURE);
				}
				tmp->was_collected = 1;
			} else {
				//asdfa;skfas;fkajs;faklsjdf;l
			}
			tmp = tmp->next;
		}
		if(wait_flag) {
			tmp = list;
			printf("Finished: ");
			while(tmp != NULL) {
				for(int i = 0; tmp->argv[i] != NULL; i++) {
					printf("%s ", tmp->argv[i]);
				}
				if(tmp->next != NULL) {
					printf("| ");
				}
				tmp = tmp->next;
			}
			if(WIFEXITED(status)) {
				printf("with exit status %d\n", WEXITSTATUS(status));
			} else {
				printf("with terminating signal %d\n", WTERMSIG(status));
			}
			fflush(stdout);
		}
	}
	return;
}

pilist *piadd(prlist *list, pilist *heap) {
	if(!heap) {
		heap = (pilist *)malloc(sizeof(pilist));
		heap->load = list;
		heap->next = NULL;
	} else {
		pilist *tmp = heap;
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = (pilist *)malloc(sizeof(pilist));
		tmp = tmp->next;
		tmp->load = list;
		tmp->next = NULL;
	}
	return heap;
}

void prfree(prlist *list) {
	if(list) {
		free(list->name);
		for(int i = 1; list->argv[i] != NULL; i++) {
			free(list->argv[i]);
		}
		free(list->argv);
		prfree(list->next);
		free(list);
	}
	return;
}

void pifree(pilist *heap) {
	if(heap) {
		prfree(heap->load);
		pifree(heap->next);
		free(heap);
	}
	return;
}

int collect_zombies(pilist *heap) {
	_Bool awflag;
	_Bool emptyflag = 1;
	if(heap) {
		pilist *tmp;
		for(tmp = heap; tmp != NULL; tmp = tmp->next) {
			prlist *ltmp;
			awflag = 1;
			int status;
			if(tmp->load) {
				emptyflag = 0;
				for(ltmp = tmp->load; ltmp != NULL; ltmp = ltmp->next) {
					if(!ltmp->was_collected) {
						pid_t wpid = waitpid(ltmp->pid, &status, WNOHANG);
						if(wpid == -1) {
							perror("waitpid");
							exit(EXIT_FAILURE);
						} else if(wpid) {
							ltmp->was_collected = 1;
							if(ltmp->next == NULL) {
								tmp->status = status;
							}
						} else {
							awflag = 0;
						}
					}
				}
				if(awflag) {
					printf("Finished [background]: ");
					for(ltmp = tmp->load; ltmp != NULL; ltmp = ltmp->next) {
						for(int i = 0; ltmp->argv[i] != NULL; i++) {
							printf("%s ", ltmp->argv[i]);
						}
						if(ltmp->next != NULL) {
							printf("| ");
						} else {
							printf("& ");
						}
					}
					status = tmp->status;
					if(WIFEXITED(status)) {
						printf("with exit status %d\n", WEXITSTATUS(status));
					} else {
						printf("with terminating signal %d\n", WTERMSIG(status));
					}
					fflush(stdout);
					prfree(tmp->load);
					tmp->load = NULL;
				}
			}
		}
	}
	if(emptyflag) {
		return 1;
	} else {
		return 0;
	}
}

int main(void) {
	prlist *list;
	pilist *heapofshit = NULL;
	do {
		list = getnextexpr();
		if(list) {
			callist(list);
			if(wait_flag) {
				waitlist(list, wait_flag);
				prfree(list);
			} else {
				heapofshit = piadd(list, heapofshit);
			}
			collect_zombies(heapofshit);
		}
	} while(!feof(stdin));
	int j;
	do {
		j = collect_zombies(heapofshit);
	} while(!j);
	pifree(heapofshit);
	return 0;
}
