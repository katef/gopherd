CFLAGS=-Wall -pedantic -ansi
LDFLAGS=-lmagic -lz -lm

REMOVE=rm -f

all: gopherd

clean:
	${REMOVE} gopherd.o gopherd

gopherd: gopherd.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ gopherd.o

