CC=gcc
CFLAGS=-I.
DEPS = tinywav.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: tinywav.o main.o 
	$(CC) -o main tinywav.o main.o -lm