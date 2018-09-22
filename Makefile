CC=gcc
CFLAGS=-std=c11 -O3 -Wall -Werror
LIBS=-lm -lmylib

lssh: lssh.o
	$(CC) lssh.o -o lssh $(LIBS)

.PHONY: clean
clean:
	rm -f *.o
