# If CC is not set, default to gcc
CC ?= gcc

# Compiler flags
CFLAGS = -O3 -Wall -g
all: breakterm
breakterm: breakterm.c
	$(CC) $(CFLAGS) -o breakterm breakterm.c -lncurses

clean:
	rm breakterm

clear:
	rm breakterm
