/*
 * A gopher server. Menus are generated for directory listings.
 * This is intended to be launched from inetd. Something like:
 * 
 *  gopher stream tcp nowait nobody /path/to/gopherd gopherd
 *
 * For the moment, the server and port are hardcoded here.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>
#include <magic.h>
#include <pwd.h>

static void listtexterrorf(const char *fmt, ...);

/*
 * Default server address and port.
 */
char *SERVER = "localhost";
unsigned short PORT = 70;
char *root;
bool chrooted;

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
	ft_binhex	= '4'
};

/*
 * Check for presence of illegal characters. Returns false if they exist.
 * Errors within this function are not reported to the client.
 */
bool checkchars(const char *s) {
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
 * Errors within this function are not reported to the client.
 */
static void vmenuitem(enum filetype ft, const char *path, const char *server, unsigned short port, const char *namefmt, va_list ap) {
	char *s;

	assert(path);
	assert(server);
	assert(namefmt);

	vasprintf(&s, namefmt, ap);
	if(!s) {
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

	free(s);
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

/*
 * Attempt to find a file's type. If the extension is not recognised,
 * an attempt is made to guess the file's contents via libmagic.
 */
static void listinfo(const char *fmt, ...) ;
enum filetype findtype(const char *ext, const char *path) {
	magic_t mt;
	const char *ms;
	enum filetype ft;

	/* TODO: replace with binary-search table. Or maybe a list of regexps */
	if(ext == NULL) {
		goto magic;
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
	} else if(!strcasecmp(ext, "hqx")
		|| !strcasecmp(ext, "hcx")) {
		/* BinHex - Also .hex, but we let libmagic determine that */
		return ft_binhex;
	}

magic:
	/* unrecognised extension: attempt to guess by contents */
	mt = magic_open(MAGIC_SYMLINK | MAGIC_ERROR | MAGIC_MIME);
	if(!mt) {
		return ft_binary;
	}

	if(magic_load(mt, NULL) == -1) {
		return ft_binary;
	}

	ms = magic_file(mt, path);
	if(!ms) {
		return ft_binary;
	}

	/* TODO map in more mime types here, including UUE (6) and BinHex (4) */
	if(!strncmp(ms, "text/html", strlen("text/html"))) {
		ft = ft_html;
	} else if(!strncmp(ms, "text/", strlen("text/"))) {
		ft = ft_text;
	} else {
		/* still unrecognised: binary */
		ft = ft_binary;
	}

	magic_close(mt);

	return ft;
}

static void listtexterrorf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vmenuitem(ft_error, "fake", "(NULL)", 0, fmt, ap);
	va_end(ap);
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
 * Strip off the prepended root path if neccessary. This is used to santize the
 * output when the server is unable to chroot and so has prepended the root.
 */
const char *striproot(const char *path) {
	if(root && !chrooted) {
		return path + strlen(root);
	}

	return path;
}

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
static void listfile(const char *filename, const char *ext, const char *parent) {
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
		menuitem(ft, striproot(s), SERVER, PORT, "%s - %s", filename, size);
		free(size);
	}

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

	slen = strlen(dirname) + strlen(parent) + strlen("/");
	errno = 0;
	s = malloc(slen + 1);
	if(!s) {
		listerror("malloc");
	}

	/* TODO append directory details here (number of entries) */
	snprintf(s, slen + 1, !strcmp(parent, "/") ? "%s%s" : "%s/%s", parent, dirname);
	menuitem(ft_dir, striproot(s), SERVER, PORT, "%s", dirname);

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
	if(fd == -1) {
		listerror("open");
	}

	errno = 0;
	mm = mmap(NULL, len, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
	if(mm == MAP_FAILED) {
		listerror("mmap");
	}

	fwrite(mm, len, 1, stdout);

	munmap(mm, len);
	close(fd);
}

/*
 * Returns the path based from the selector read. The two operations are
 * combined here because the selector is prepended with the root path if a
 * chroot cannot be performed. Caller frees.
 */
char *readandchroot(const char *user) {
	char selector[MAXPATHLEN];
	char path[MAXPATHLEN];
	struct passwd *pw;

	/*
	 * Find the user to switch to. This must be done before chroot, because
	 * getpwnam() needs to read /etc/passwd.
	 */
	if(user) {
		errno = 0;
		pw = getpwnam(user);
		if(!pw) {
			listerror("unknown user");
		}
	}

	/*
	 * Perform the chroot. This must be done before changing user as chroot may only
	 * be done by the root user. If the user is not root, we will prepend the root
	 * path to our selector further on, as a substitute.
	 */
	chrooted = false;
	if(root && getuid() == 0) {
		if(chroot(root) == -1) {
			listerror("chroot");
		}
		chrooted = true;
	}

	/*
	 * Switch user.
	 */
	if(pw) {
		if(setgid(pw->pw_gid) == -1) {
			listerror("setgid");
		}

		if(setuid(pw->pw_uid) == -1) {
			listerror("setuid");
		}

		endpwent();
	}

	/*
	 * Find and simplify the given selector into a selection path.
	 */
	fgets(selector, MAXPATHLEN, stdin);
	selector[strcspn(selector, "\r\n")] = '\0';
	if(strlen(selector) == 0) {
		strcpy(selector, "/");
	}
	if(root && !chrooted) {
		char s[MAXPATHLEN];

		strncpy(s, selector, sizeof(s) - 1);
		s[sizeof(s) - 1] = '\0';
		snprintf(selector, MAXPATHLEN, "%s/%s", root, s);
	}
	if(!realpath(selector, path)) {
		listerror("realpath");
	}

	/*
	 * Check that the selection path is inside the given root.
	 */
	if(root && !chrooted) {
		if(strncmp(path, root, strlen(root))) {
			errno = EACCES;
			listerror("root");
		}
	}

	return strdup(path);
}

int main(int argc, char *argv[]) {
	struct stat sb;
	int c;
	char *user = NULL;
	char *path;

	/* TODO some option to specify a banner file for "welcome to such-and-such server" */
	/* TODO does inetd provide those as environment variables? */
	while((c = getopt(argc, argv, "hr:u:")) != -1) {
		switch(c) {
		case 'p':
			PORT = atoi(optarg);
			break;

		case 's':
			SERVER = optarg;
			break;

		case 'r':
			root = optarg;
			break;

		case 'u':
			user = optarg;
			break;

		case 'h':
		case '?':
		default:
			/* TODO document usage in a README or somesuch */
			printf("usage: %s [ -h | -r <root> | -u <user> | -s <server> | -p <port> ]\n", argv[0]);
			return EXIT_SUCCESS;
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * Initialise and read input selector.
	 */
	path = readandchroot(user);
	if(!path) {
		listerror("initialise");
	}

	errno = 0;
	if(stat(path, &sb) == -1) {
		listerror("stat");
	}

	if(S_ISDIR(sb.st_mode)) {
		dirmenu(path);
		printf(".\r\n");
	} else {
		char *ext;

		mapfile(path, sb.st_size);

		/* If it's not binary, end with a '.' */
		/* TODO do pictures need this? */
		ext = strrchr(path, '.');
		if(findtype(ext, path) != ft_binary) {
			printf(".\r\n");
		}
	}

	return EXIT_SUCCESS;
}

