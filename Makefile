# If CC is not set, default to gcc
CC ?= gcc

# Compiler flags, have not seen issues with z/OS with O3
CFLAGS = -O3 -Wall -g

# Source files
SRC = breakterm.c

# Executable name
EXEC = breakterm

# Object files
OBJ = $(SRC:.c=.o)

all: breakterm
breakterm: breakterm.c
	$(CC) $(CFLAGS) -o breakterm breakterm.c -lncurses

clean:
	rm $(EXEC)

clear:
	rm $(EXEC)
