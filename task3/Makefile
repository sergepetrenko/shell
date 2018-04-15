CFLAGS = -Wall -std=c99

all: shell_andor test

shell_andor: shell_andor.c
	gcc $(CFLAGS) -o $@ shell_andor.c

test: test.c
	gcc $(CFLAGS) -o $@ test.c

clean:
	rm test shell_andor
