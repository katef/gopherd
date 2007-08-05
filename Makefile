CFLAGS=-Wall -pedantic -ansi -lmagic -lz

REMOVE=rm -f

gopherd: gopherd.o

clean:
	${REMOVE} gopherd.o gopherd

