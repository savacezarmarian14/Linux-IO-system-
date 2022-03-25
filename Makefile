#!bin/bash

all: run

run: build
	./so_stdio

so_stdio.o:
	$(CC) -c so_stdio.c -o so_stdio.o

build: so_stdio.o
	$(CC) so_stdio.o -o so_stdio 

clean:
	rm so_stdio.o so_stdio