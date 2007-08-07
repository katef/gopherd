# $Id$

all:
	cd doc && ${MAKE} all
	cd src && ${MAKE} all

clean:
	cd doc && ${MAKE} clean
	cd src && ${MAKE} clean

