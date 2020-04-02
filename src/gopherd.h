#ifndef GOPHERD_H
#define GOPHERD_H

#include <sys/types.h>

#include <stdbool.h>

#define VERSION "0.1"
#define AUTHOR "Kate F"

/*
 * Gopher menu types.
 * These are the first character in menus.
 */
enum filetype {
	FT_BINARY	= '9',
	FT_GIF  	= 'g',
	FT_HTML 	= 'h',
	FT_IMAGE	= 'I',
	FT_AUDIO	= 's',
	FT_TEXT 	= '0',
	FT_DIR		= '1',
	FT_INFO		= 'i',
	FT_ERROR	= '3',
	FT_BINHEX	= '4',
	FT_TELNET	= '8'
};

extern char *root;
extern bool chrooted;
extern bool showhidden;
extern bool hidesize;
extern char *bannerfile;

/* main.c */
unsigned short getservport(const char *service);

/* file.c */
void mapfile(const char *path, size_t len);
enum filetype findtype(const char *ext, const char *path);

/* menu.c */
void dirmenu(const char *path, const char *server, unsigned short port);

/* root.c */
const char *striproot(const char *path);
char *readandchroot(const char *user);

/* output.c */
/* TODO make enums const */
void menuitem(enum filetype ft, const char *path,
	const char *server, const unsigned short port,
	const char *namefmt, ...);
void listerror(const char *msg);
void listinfo(const char *fmt, ...);

/* banner.c */
void listbanner(const char *path);

#endif

