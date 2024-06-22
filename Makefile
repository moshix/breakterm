# If CC is not set, default to gcc
CC ?= gcc

# Compiler flags
CFLAGS = -O3 -Wall -g

# Source file
SRC = breakterm.c

# Executable name
EXEC = breakterm

# Default target
.PHONY: all
all: $(EXEC)

# Rule to build the executable
$(EXEC):
	$(CC) $(CFLAGS) -o $@ $(SRC) -lncurses

# Clean target to remove the generated executable
.PHONY: clean
clean:
	rm -f $(EXEC)

# Clear target (same as clean)
.PHONY: clear
clear: clean
