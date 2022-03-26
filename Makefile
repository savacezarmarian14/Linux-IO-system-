#!bin/bash

all: run

run: build

so_stdio.o:
	$(CC) -c -fPIC so_stdio.c -o so_stdio.o

build: so_stdio.o
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
	$(CC) -shared so_stdio.o -o libso_stdio.so

clean:
	rm -f so_stdio.o so_stdio libso_stido.so