.include "../../share/mk/top.mk"

SRC += src/banner.c
SRC += src/file.c
SRC += src/main.c
SRC += src/menu.c
SRC += src/output.c
SRC += src/root.c

.for src in ${SRC:M*.c}
#CFLAGS.${src} += -I src
.endfor

CFLAGS += -D_XOPEN_SOURCE=600

PROG += gopherd

# XXX: would prefer to use pkgconf for libmagic,
# but it isn't packaged with a .pc file (at least for Debian).
LFLAGS.gopherd += -lmagic -lz -lm

.for src in ${SRC:Msrc/*.c}
${BUILD}/bin/gopherd: ${BUILD}/${src:R}.o
.endfor

