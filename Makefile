# If CC is not set, default to gcc
CC ?= gcc

# Compiler flags
CFLAGS = -O3 -Wall

# Source file
SRC = breakterm.c

# Executable name
EXEC = breakterm

# Object file
OBJ = $(SRC:.c=.o)

# Default target
.PHONY: all
all: $(EXEC)

# Rule to link the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

# Rule to compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove generated files
.PHONY: clean
clean:
	rm -f $(OBJ) $(EXEC)

# Clear target (same as clean)
.PHONY: clear
clear: clean
