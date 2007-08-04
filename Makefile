CFLAGS=-Wall -pedantic -ansi

REMOVE=rm -f

gopher: gopher.o

clean:
	${REMOVE} gopher.o gopher

