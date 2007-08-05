CFLAGS=-Wall -pedantic -ansi -lmagic -lz -lm

REMOVE=rm -f

gopherd: gopherd.o

clean:
	${REMOVE} gopherd.o gopherd

