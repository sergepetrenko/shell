CFLAGS = -Wall -std=c99

all: shell_pipe test

shell_pipe: shell_pipe.c
	gcc $(CFLAGS) -o $@ shell_pipe.c

test: test.c
	gcc $(CFLAGS) -o $@ test.c

clean:
	rm test shell_pipe
