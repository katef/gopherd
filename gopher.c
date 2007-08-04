/*
 * A gopher server. Menus are generated for directory listings.
 * This is intended to be launched from inetd. Something like:
 * 
 *  gopher stream tcp nowait nobody /path/to/gopher gopher
 *
 * For the moment, the server and port are hardcoded here.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>

#define SERVER "localhost"
#define PORT 70

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
	ft_error	= '3'
};

/*
 * Output a single menu item. The strings passed in are unescaped.
 */
static void vmenuitem(enum filetype ft, const char *path, const char *server, unsigned short port, const char *namefmt, va_list ap) {
	assert(path);
	assert(namefmt);

	/* TODO omit paths with odd characters? */

	printf("%c", ft);

	/* TODO urlencode name here: vsnprintf to string, and urlencode that string */
	vprintf(namefmt, ap);

	/* TODO urlencode strings here */
	printf("\t%s\t%s\t%d\r\n",
		path, server, port);
}

/*
 * Output a single menu item. The strings passed in are unescaped.
 */
static void menuitem(enum filetype ft, const char *path, char *server, unsigned short port, const char *namefmt, ...) {
	va_list ap;

	assert(path);
	assert(namefmt);

	va_start(ap, namefmt);
	vmenuitem(ft, path, server, port, namefmt, ap);
	va_end(ap);
}

enum filetype findtype(const char *ext) {
	/* TODO: replace with binary-search table. Or maybe a list of regexps */
	/* TODO find usual extensions for BinHex (4) and UUE (6) */
	if(ext == NULL) {
		/* unknown: binary */
		return ft_binary;
	} else if(!strcasecmp(ext, "txt")) {
		/* plain text */
		return ft_text;
	} else if(!strcasecmp(ext, "png")
		|| !strcasecmp(ext, "jpg")
		|| !strcasecmp(ext, "jpeg")
		|| !strcasecmp(ext, "bmp")) {
		/* image */
		return ft_image;
	} else if(!strcasecmp(ext, "gif")) {
		return ft_gif;
	} else if(!strcasecmp(ext, "wav")
		|| !strcasecmp(ext, "ogg")
		|| !strcasecmp(ext, "mp3")) {
		/* audio */
		return ft_audio;
	}

	/* unrecognised: binary */
	return ft_binary;
}

/*
 * Create an error listing and exit.
 */
static void listerror(const char *msg) {
	menuitem(ft_error, "fake", "(NULL)", 0, "%s: %s", msg, strerror(errno));
	exit(EXIT_FAILURE);
}

/*
 * Create a listing for informational text. printf-style formatting is
 * provided.
 */
static void listinfo(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vmenuitem(ft_info, "fake", "(NULL)", 0, fmt, ap);
	va_end(ap);
}

/*
 * Find if a string is entirely uppercase.
 */
static bool isupperstr(const char *s) {
	while(*s) {
		if(!isupper((int)*s)) {
			return false;
		}

		s++;
	}

	return true;
}

/*
 * Create a listing for a single file item.
 */
static void listfile(const char *filename, const char *ext, const char *parent) {
	char *s;
	size_t slen;
	enum filetype ft;

	assert(filename);
	assert(parent);

	slen = strlen(filename) + strlen(parent) + sizeof("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	ft = findtype(ext);
	if(ft == ft_binary && isupperstr(filename)) {
		/* Entirely uppercase files are usually text */
		ft = ft_text;
	}

	/* TODO append directory listing details: filesize, date etc */
	snprintf(s, slen, !strcmp(parent, "/") ? "%s%s" : "%s/%s", parent, filename);
	menuitem(ft, s, SERVER, PORT, "%s - %s", filename, "53Kb" /* TODO */);

	free(s);
}

/*
 * Create a listing for a single directory item. Note that trailing
 * slashes are not appended, as the display is the client's choice.
 */
static void listdir(const char *dirname, const char *parent) {
	char *s;
	size_t slen;

	assert(dirname);
	assert(parent);

	slen = strlen(dirname) + strlen(parent) + sizeof("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	/* TODO append directory details here (number of entries) */
	snprintf(s, slen, !strcmp(parent, "/") ? "%s%s" : "%s/%s", parent, dirname);
	menuitem(ft_dir, s, SERVER, PORT, "%s", dirname);

	free(s);
}

/*
 * Create a menu listing all the contents of the given directory.
 */
static void dirmenu(const char *path) {
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

	/* Simplify the path a little */
	/* TODO cleanup paths: strip prepending ./ etc, Empty paths can become '/' */
	if(!strncmp(path, "./", 2)) {
		path = path + 1;
	} else if(!strcmp(path, ".")) {
		path = "/";
	}

	listinfo("Index of %s", path);
	listinfo("");

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
			listdir(de.d_name, path);
			i++;
			break;

		case DT_REG:
			{
				char *ext;

				ext = strrchr(de.d_name, '.');
				listfile(de.d_name, ext ? ext + 1 : NULL, path);
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

/*
 * Output the given file.
 */
static void mapfile(const char *path, size_t len) {
	void *mm;
	int fd;

	assert(path);

	if(len == 0) {
		return;
	}

	errno = 0;
	fd = open(path, O_RDONLY);
	/* TODO handle error gracefully */
	if(fd == -1) {
		listerror("open");
	}

	errno = 0;
	mm = mmap(NULL, len, PROT_READ, MAP_FILE, fd, 0);
	if(mm == MAP_FAILED) {
		listerror("mmap");
	}

	fwrite(mm, len, 1, stdout);

	munmap(mm, len);
	close(fd);
}

int main(void) {
	char selector[PATH_MAX + 1];
	struct stat sb;

	/* TODO getopt: at least -h. probably also the server name and port */
	/* TODO does inetd provide those as environment variables? */

	fgets(selector, PATH_MAX + 1, stdin);
	selector[strcspn(selector, "\r\n")] = '\0';

	if(strlen(selector) == 0) {
		dirmenu(".");
		exit(EXIT_SUCCESS);
	}

	errno = 0;
	if(stat(selector, &sb) == -1) {
		listerror("stat");
	}

	if(S_ISDIR(sb.st_mode)) {
		dirmenu(selector);
		printf(".\r\n");
	} else {
		char *ext;

		mapfile(selector, sb.st_size);

		/* If it's not binary, end with a '.' */
		/* TODO do pictures need this? */
		ext = strrchr(selector, '.');
		if(findtype(ext) != ft_binary) {
			printf(".\r\n");
		}
	}

	return EXIT_SUCCESS;
}

