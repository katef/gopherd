# $Id$
# Because I always forget: groff -man -Tascii gopherd.8

XSLTPROC=xsltproc
XSLTFLAGS=--stringparam man.hyphenate 1 --stringparam man.justify 1 --stringparam man.hyphenate.urls 0 \
	--stringparam man.break.after.slash 1 --stringparam man.hyphenate.filenames 0 \
	--stringparam man.hyphenate.computer.inlines 0 --stringparam man.links.are.numbered 1 \
	--stringparam man.links.list.enabled 1
DOCBOOK=http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl
REMOVE=rm -f

all: gopherd.8

clean:
	${REMOVE} gopherd.8

gopherd.8: gopherd.8.xml
	${XSLTPROC} ${XSLTFLAGS} -o gopherd.8 ${DOCBOOK} gopherd.8.xml

