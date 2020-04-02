# gopherd

This is a Gopher server, run under inetd.

The server is intended to be run as an inetd(8) service.
It will serve one request and then exit.
A suitable inetd.conf(5) configuration line looks like so:

    gopher stream tcp nowait nobody /path/to/gopherd gopherd [-options]

## Installing

Clone with submodules (contains required .mk files):

    ; git clone --recursive https://github.com/katef/gopherd.git

To build and install:

    ; pmake -r install

You can override a few things:

    ; CC=clang PREFIX=$HOME pmake -r install

Building depends on:

 * libmagic. This is packaged for Debian as libmagic-dev.

 * Any BSD make. This includes OpenBSD, FreeBSD and NetBSD make(1)
   and sjg's portable bmake (also packaged as pmake).

 * A C compiler. Any should do, but GCC and clang are best supported.

 * ar, ld, and a bunch of other stuff you probably already have.

To build the manpage, the following dependencies are required:

 * xsltproc from libxslt

 * docbook XSL

Ideas, comments or bugs: kate@elide.org

