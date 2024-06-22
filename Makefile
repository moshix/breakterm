all: breakterm
breakterm: breakterm.c
	gcc -O3 -o breakterm breakterm.c -lncurses

clean:
	rm breakterm

clear:
	rm breakterm
