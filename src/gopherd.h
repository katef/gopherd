/*
 * $Id$
 */

#ifndef _GOPHERD_H
#define _GOPHERD_H

#include <sys/types.h>
#include <stdbool.h>

#define VERSION "0.1"
#define AUTHOR "Kate F"

/*
 * Gopher menu types. These are the first character in menus.
 */
enum filetype {
	ft_binary	= '9',
	ft_gif  	= 'g',
	ft_html 	= 'h',
	ft_image	= 'I',
	ft_audio	= 's',
	ft_text 	= '0',
	ft_dir		= '1',
	ft_info		= 'i',
	ft_error	= '3',
	ft_binhex	= '4',
	ft_telnet	= '8'
};

extern char *root;
extern bool chrooted;
extern bool showhidden;
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
void menuitem(enum filetype ft, const char *path, const char *server, const unsigned short port, const char *namefmt, ...);
void listerror(const char *msg);
void listinfo(const char *fmt, ...);

/* banner.c */
void listbanner(const char *path);

#endif

