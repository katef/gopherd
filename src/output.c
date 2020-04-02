/*
 * Menu output.
 */

#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "gopherd.h"

static void listtexterrorf(const char *fmt, ...);

/*
 * Check for presence of illegal characters. Returns false if they exist.
 * Errors within this function are not reported to the client to avoid an
 * infinite loop.
 */
static bool checkchars(const char *s) {
	assert(s);

	/*
	 * If the complement of this set spans to the end of the string
	 * (that is, s[span] is '\0', then no invalid characters are
	 * found.
	 */
	return !s[strcspn(s, "\t\r\n")];
}

/*
 * Output a single menu item. The strings passed in are unescaped.
 * Errors within this function are not reported to the client to
 * avoid an infinite loop.
 */
static void vmenuitem(enum filetype ft, const char *path, const char *server, unsigned short port, const char *namefmt, va_list ap) {
	char s[1024];

	assert(path);
	assert(server);
	assert(namefmt);

	if(vsnprintf(s, sizeof s, namefmt, ap) >= sizeof s) {
		exit(EXIT_FAILURE);
	}

	if(!checkchars(s)) {
		listtexterrorf("Illegal character in filename");
		return;
	}
	if(!checkchars(path)) {
		listtexterrorf("Illegal character in path to file");
		return;
	}
	if(!checkchars(server)) {
		listtexterrorf("Illegal character in hostname");
		return;
	}

	printf("%c%s\t%s\t%s\t%d\r\n",
		ft, s, path, server, port);
}

/*
 * Output a single menu item. The strings passed in are unescaped.
 */
void menuitem(enum filetype ft, const char *path, const char *server, const unsigned short port, const char *namefmt, ...) {
	va_list ap;

	assert(path);
	assert(namefmt);

	va_start(ap, namefmt);
	vmenuitem(ft, path, server, port, namefmt, ap);
	va_end(ap);
}

/*
 * Create an error listing and exit.
 */
static void listtexterrorf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vmenuitem(ft_error, "fake", "(NULL)", 0, fmt, ap);
	va_end(ap);
}

/*
 * Create an error listing and exit.
 */
void listerror(const char *msg) {
	menuitem(ft_error, "fake", "(NULL)", 0, "%s: %s", msg, strerror(errno));
	printf(".\r\n");
	exit(EXIT_FAILURE);
}

/*
 * Create a listing for informational text. printf-style formatting is
 * provided.
 */
void listinfo(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vmenuitem(ft_info, "fake", "(NULL)", 0, fmt, ap);
	va_end(ap);
}

