.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep prog
dep::
gen::
test:: all
install:: all
uninstall::
clean::

# things to override
CC     ?= gcc
BUILD  ?= build
PREFIX ?= /usr/local

# layout
SUBDIR += src

.include <subdir.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

