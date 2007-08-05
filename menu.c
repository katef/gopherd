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

#include "gopherd.h"

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
 * Create a listing for a single file item.
 */
static void listfile(const char *filename, const char *ext, const char *parent, const char *server, const unsigned short port) {
	char *s;
	size_t slen;
	enum filetype ft;

	assert(filename);
	assert(parent);

	slen = strlen(filename) + strlen(parent) + strlen("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	snprintf(s, slen + 1, !strcmp(parent, "/") ? "%s%s" : "%s/%s", parent, filename);

	ft = findtype(ext, s);

	/* TODO append directory listing details: date etc */
	{
		char *size;
		struct stat sb;

		if(stat(s, &sb) == -1) {
			listerror("stat");
		}

		size = humanreadable(sb.st_size);
		menuitem(ft, striproot(s), server, port, "%s - %s", filename, size);
		free(size);
	}

	free(s);
}

/*
 * Create a listing for a single directory item. Note that trailing
 * slashes are not appended, as the display is the client's choice.
 */
static void listdir(const char *dirname, const char *parent, const char *server, const unsigned short port) {
	char *s;
	size_t slen;

	assert(dirname);
	assert(parent);

	slen = strlen(dirname) + strlen(parent) + strlen("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	/* TODO append directory details here (number of entries) */
	snprintf(s, slen + 1, !strcmp(parent, "/") ? "%s%s" : "%s/%s", parent, dirname);
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

	i = 0;
	for(;;) {
		errno = 0;
		if(readdir_r(od, &de, &dep)) {
			listerror("readdir_r");
		}

		if(!dep) {
			break;
		}

		/* TODO option to show hidden files (-a?) */
		if(de.d_name[0] == '.') {
			continue;
		}

		switch(de.d_type) {
		case DT_DIR:
			/* TODO possibly show a configurable level of subentries? */
			listdir(de.d_name, path, server, port);
			i++;
			break;

		case DT_REG:
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

	listinfo("");
	listinfo("%d item%s total", i, i == 1 ? "" : "s");

	closedir(od);
}

