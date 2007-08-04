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

/* TODO menuerror() wrapper in place of perror to provide properly-formatted messages */
/* TODO menuinfo() (variadic) to provide properly-formatted informational messages */

#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
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
	ft_dir		= '1'
};

/*
 * Output a single menu item. The strings passed in are unescaped.
 */
void menuitem(enum filetype ft, const char *name, const char *path, const char *parent) {
	assert(name);
	assert(path);

	/* TODO urlencode name here */
	/* TODO omit paths with odd characters */
	/* TODO cleanup paths: strip prepending ./ etc, Empty paths can become '/' */

	printf("%c%s\t%s/%s\t%s\t%d\r\n",
		ft, name, parent, path, SERVER, PORT);
}

enum filetype findtype(const char *ext) {
	/* TODO: replace with binary-search table */
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
 * Create a listing for a single file item.
 */
void listfile(const char *path, const char *ext, const char *parent) {
	enum filetype ft;

	assert(path);
	assert(parent);

	ft = findtype(ext);

	/* TODO append directory listing details: filesize, date etc */
	menuitem(ft, path, path, parent);
}

/*
 * Create a listing for a single directory item.
 */
void listdir(const char *path, const char *parent) {
	char *s;
	size_t slen;

	assert(path);
	assert(parent);

	slen = strlen(path) + sizeof("/") + strlen(parent);
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		perror("3malloc");
		exit(EXIT_FAILURE);
	}

	/* TODO append directory details here (number of entries) */
	snprintf(s, slen + sizeof("/"), "%s/", path);
	menuitem(ft_dir, s, s, parent);

	free(s);
}

/*
 * Create a menu listing all the contents of the given directory.
 */
void dirmenu(const char *path) {
	DIR *od;
	struct dirent de;
	struct dirent *dep;
   
	assert(path);

	errno = 0;
	od = opendir(path);
	if(!od) {
		perror("3opendir");
		exit(EXIT_FAILURE);
	}

	/* TODO escape path */
	printf("iDirectory listing for %s:\tfake\t(NULL)\t0\r\n", path);

	for(;;) {
		errno = 0;
		if(readdir_r(od, &de, &dep)) {
			perror("3readdir_r");
			exit(EXIT_FAILURE);
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
			break;

		case DT_REG:
			{
				char *ext;

				ext = strrchr(de.d_name, '.');
				listfile(de.d_name, ext ? ext + 1 : NULL, path);
			}
			break;
	
		default:
			/* Omit unrecognised types */
			break;
		}
	}

	closedir(od);
}

/*
 * Output the given file.
 */
void mapfile(const char *path, size_t len) {
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
		perror("3open");
		exit(EXIT_SUCCESS);
	}

	errno = 0;
	mm = mmap(NULL, len, PROT_READ, MAP_FILE, fd, 0);
	if(mm == MAP_FAILED) {
		perror("3mmap");
		exit(EXIT_FAILURE);
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
		perror("3stat");
		exit(EXIT_SUCCESS);
	}

	if(S_ISDIR(sb.st_mode)) {
		dirmenu(selector);
		printf(".\r\n");
	} else {
		char *ext;

		mapfile(selector, sb.st_size);

		/* If it's not binary, end with a '.' */
		ext = strrchr(selector, '.');
		if(findtype(ext) != ft_binary) {
			printf(".\r\n");
		}
	}

	return EXIT_SUCCESS;
}

