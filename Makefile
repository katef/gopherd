CFLAGS=-Wall -pedantic -ansi -lmagic -lz

REMOVE=rm -f

gopher: gopher.o

clean:
	${REMOVE} gopher.o gopher

