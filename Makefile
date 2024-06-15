all: breakterm
CC=gcc
breakterm: breakterm.c
	$(CC) -g -O3 -o breakterm breakterm.c -lncurses

clean:
	rm breakterm

clear:
	rm breakterm
