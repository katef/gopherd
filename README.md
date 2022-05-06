# gopherd

This is a Gopher server, run under inetd.

The server is intended to be run as an inetd(8) service.
It will serve one request and then exit.
A suitable inetd.conf(5) configuration line looks like so:

    gopher stream tcp nowait nobody /path/to/gopherd gopherd [-options]

### Building from source

Clone with submodules (contains required .mk files):

    ; git clone --recursive https://github.com/katef/gopherd.git

To build and install:

    ; bmake -r install

You can override a few things:

    ; CC=clang bmake -r
    ; PREFIX=$HOME bmake -r install

You need bmake for building. In order of preference:

 1. If you use some kind of BSD (NetBSD, OpenBSD, FreeBSD, ...) this is make(1).
    They all differ slightly. Any of them should work.
 2. If you use Linux or MacOS and you have a package named bmake, use that.
 3. If you use Linux and you have a package named pmake, use that.
    It's the same thing.
    Some package managers have bmake packaged under the name pmake.
    I don't know why they name it pmake.
 4. Otherwise if you use MacOS and you only have a package named bsdmake, use that.
    It's Apple's own fork of bmake.
    It should also work but it's harder for me to test.
 5. If none of these are options for you, you can build bmake from source.
    You don't need mk.tar.gz, just bmake.tar.gz. This will always work.
    https://www.crufty.net/help/sjg/bmake.html

When you see "bmake" in the build instructions above, it means any of these.

Building depends on:

 * libmagic. This is packaged for Debian as libmagic-dev.
 * Any BSD make.
 * A C compiler. Any should do, but GCC and clang are best supported.
 * ar, ld, and a bunch of other stuff you probably already have.

To build the manpage, the following dependencies are required:

 * xsltproc from libxslt
 * docbook XSL

Ideas, comments or bugs: kate@elide.org

