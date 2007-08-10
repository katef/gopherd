/*
 * Menu listing.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>

#include "gopherd.h"

bool showhidden;
bool hidesize;

/*
 * Return a string of human-readable digits in the form "2.34KB".
 * Caller frees.
 */
static char *humanreadable(double size) {
#define DIGITS 255
	char buffer[DIGITS];

	if(size < 1) {
		snprintf(buffer, DIGITS, "0");
	} else if(size < 1000) {
		snprintf(buffer, DIGITS, "%dB", (int)size);
	} else if(size < 1000000) {
		snprintf(buffer, DIGITS, "%.2fkB", size / 1024);
	} else {
		const char unit[] = "MGTP";
		int i;

		for(i = 3; ; i++) {
			if(size >= pow(1000, i) && unit[i - 2]) {
				continue;
			}

			snprintf(buffer, DIGITS, "%.02f%cB", size / pow(1024, i - 1), unit[i - 3]);
			break;
		}
	}

	return strdup(buffer);
#undef DIGITS
}

/*
 * Concaternate a filename onto a path. Caller frees.
 */
static char *allocpath(const char *filename, const char *path) {
	char *s;
	size_t slen;

	assert(filename);
	assert(path);

	slen = strlen(filename) + strlen(path) + strlen("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	snprintf(s, slen + 1, !strcmp(path, "/") ? "%s%s" : "%s/%s", path, filename);
	return s;
}

/*
 * Create a listing for a single file item.
 */
static void listfile(const char *filename, const char *ext, const char *parent, const char *server, const unsigned short port) {
	enum filetype ft;
	char *s;
	char *size;
	struct stat sb;

	assert(filename);
	assert(parent);

	ft = findtype(ext, filename);

	/* TODO append directory listing details: date etc */
	s = allocpath(filename, parent);

	if(stat(s, &sb) == -1) {
		listerror("stat");
	}

	if(hidesize) {
		menuitem(ft, striproot(s), server, port, "%s", filename);
	} else {
		size = humanreadable(sb.st_size);
		menuitem(ft, striproot(s), server, port, "%s - %s", filename, size);
	}
	free(size);
	free(s);
}

/*
 * Create a listing for a single directory item. Note that trailing
 * slashes are not appended, as the display is the client's choice.
 */
static void listdir(const char *dirname, const char *parent, const char *server, const unsigned short port) {
	char *s;

	assert(dirname);
	assert(parent);

	s = allocpath(dirname, parent);

	/* TODO append directory details here (number of entries) */
	menuitem(ft_dir, striproot(s), server, port, "%s", dirname);

	free(s);
}

/*
 * Create a menu listing all the contents of the given directory.
 */
void dirmenu(const char *path, const char *server, const unsigned short port) {
	DIR *od;
	struct dirent de;
	struct dirent *dep;
	unsigned int i;
   
	assert(path);

	errno = 0;
	od = opendir(path);
	if(!od) {
		listerror("opendir");
	}

	/*
	 * Show the banner file, if specified.
	 */
	if(bannerfile) {
		listbanner(path);
	}

	i = 0;
	for(;;) {
		char *s;
		struct stat sb;

		errno = 0;
		if(readdir_r(od, &de, &dep)) {
			listerror("readdir_r");
		}

		if(!dep) {
			break;
		}

		/*
		 * Skip parent and current directories.
		 */
		if(!strcmp(de.d_name, ".")
		|| !strcmp(de.d_name, "..")) {
			continue;
		}

		/*
		 * Skip hidden files, if appropiate.
		 */
		if(!showhidden && de.d_name[0] == '.') {
			continue;
		}

		/*
		 * Skip the banner file here, if specified, since it needn't be
		 * included in listings.
		 */
		if(bannerfile && !strcmp(de.d_name, bannerfile)) {
			continue;
		}

		s = allocpath(de.d_name, path);

		if(stat(s, &sb) == -1) {
			listerror("stat");
		}
		free(s);

		switch(sb.st_mode & S_IFMT) {
		case S_IFDIR:
			/* TODO possibly show a configurable level of subentries? */
			listdir(de.d_name, path, server, port);
			i++;
			break;

		case S_IFREG:
			{
				char *ext;

				ext = strrchr(de.d_name, '.');
				listfile(de.d_name, ext ? ext + 1 : NULL, path, server, port);
			}
			i++;
			break;
	
		default:
			/* Omit unrecognised types */
			break;
		}
	}

	/*
	 * If there are no items, this may be a banner-only directory.
	 */
	if(bannerfile && i) {
		listinfo("");
		listinfo("%d item%s total", i, i == 1 ? "" : "s");
	}

	closedir(od);
}

