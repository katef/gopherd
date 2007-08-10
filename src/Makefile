# $Id$

CFLAGS=-Wall -Werror -pedantic -ansi -D_XOPEN_SOURCE=600
LDFLAGS=-lmagic -lz -lm

REMOVE=rm -f
INSTALL=install

TARGETS=file.o main.o menu.o output.o root.o banner.o

PREFIX?=/usr/local
LIBEXEC?=${PREFIX}/libexec

all: gopherd

install: all
	${INSTALL} -m 755 -d ${LIBEXEC}
	${INSTALL} -m 755 gopherd ${LIBEXEC}/

clean:
	${REMOVE} $(TARGETS) gopherd

gopherd: $(TARGETS)
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $(TARGETS)

