CFLAGS=-Wall -pedantic -ansi
LDFLAGS=-lmagic -lz -lm

REMOVE=rm -f

TARGETS=file.o main.o menu.o output.o root.o banner.o

all: gopherd

clean:
	${REMOVE} $(TARGETS) gopherd

gopherd: $(TARGETS)
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $(TARGETS)

