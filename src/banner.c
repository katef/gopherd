/*
 * Banner file handling. Banners are a simple file format which is output
 * line-by-line as info menu types. The file itself is not included in
 * the menu listing.
 *
 * Within the file, URLs (on their own lines) are output as links.
 */

#include <sys/mman.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "gopherd.h"

char *bannerfile;

/*
 * Tokenise a URL out to the server, port and path. If the port is not given,
 * it defaults as given by the protocol name. The url given is written over.
 * The output path may be NULL if none is given.
 *
 * Specifiying no port is permitted; the default will be used.
 *
 * Returns false on parse errors.
 */
static bool
urlsplit(char *url, char **server, unsigned short *port,
	char **path, bool *defaultport, char **service)
{
	char *p;

	*server = strstr(url, "://");
	if (!*server) {
		return false;
	}
	**server = '\0';
	*server += strlen("://");
	*service = url;

	/*
	 * Note that we find the path before the port, so that we don't confuse
	 * any colons in the path with the port, if a port is not given.
	 */
	*path = strchr(*server, '/');
	if (*path == NULL) {
		/* There is no path given. */
		*path = "";
	} else {
		**path = '\0';
		(*path)++;
	}

	/*
	 * Find the port, if given.
	 */
	p = strchr(*server, ':');
	if (p != NULL) {
		*p = '\0';
		p++;

		*port = strtol(p, NULL, 10);
		if (*port) {
			*defaultport = false;
			return true;
		}
	}

	/*
	 * A port was not given, so we can look it up from the services database.
	 * url has been terminated at the "://" so that it contains only a service.
	 */
	*port = getservport(url);
	*defaultport = true;

	return true;
}

/*
 * Create a menu item, only showing the port if it is not the default for that service.
 */
static void
showurl(const enum filetype ft, const char *href,
	const char *server, const unsigned short port, const char *service,
	const char *linkpath, const bool defaultport)
{
	if (defaultport) {
		menuitem(ft, href, server, port, "%s://%s/%s", service, server, linkpath);
	} else {
		menuitem(ft, href, server, port, "%s://%s:%d/%s", service, server, port, linkpath);
	}
}

/*
 * Decode a URL.
 * May output to the same string from which it reads.
 */
static void
urldecode(const char *in, char *out)
{
	char hex[3] = { '\0' };

	while (*in != '\0') {
		switch(*in++) {
		case '%':
			strncpy(hex, in, 2);
			*out = strtol(hex, NULL, 16);
			in += 2;
			break;

		case '+':
			*out = ' ';
			break;

		default:
			*out = *(in - 1);
			break;
		}

		out++;
	}

	*out = '\0';
}

/*
 * Will write over the memory it is passed.
 */
static void
showbanner(char *banner)
{
	char *p;

	for ( ; banner != NULL && *banner != '\0'; banner = p) {
		char *server;
		unsigned short port;
		char *path;
		char *service;
		bool defaultport;

		p = strchr(banner, '\n');
		if (p != NULL) {
			*p++ = '\0';
		}

		if (!strstr(banner, "://")) {
			listinfo(banner);
			continue;
		}

		/*
		 * Here we have a url. Attempt to parse it.
		 */
		urldecode(banner, banner);
		if (!urlsplit(banner, &server, &port, &path, &defaultport, &service)) {
			listinfo(banner);
			continue;
		}

		if (!strncmp(service, "http", strlen("http"))) {
			char *s;
			size_t slen;

			slen = strlen(path) + strlen("GET /");
			s = malloc(slen + 1);
			if (s == NULL) {
				listerror("malloc");
			}

			snprintf(s, slen + 1, "GET %s%s", path[0] == '/' ? "" : "/", path);
			showurl(FT_HTML, s, server, port, service, path, defaultport);
			free(s);
			continue;
		} else if (!strncmp(service, "gopher", strlen("gopher"))) {
			showurl(FT_DIR, path, server, port, service, path, defaultport);
			continue;
		} else if (!strncmp(service, "telnet", strlen("telnet"))) {
			showurl(FT_TELNET, path, server, port, service, path, defaultport);
			continue;
		}

		/*
		 * An unrecognised service.
		 */
		listinfo(banner);
	}

	listinfo("");
}

/* TODO show banner here, if specified.
 * ^http:// and ^gopher:// etc (including telnet) can be made links. */
void
listbanner(const char *path)
{
	char *s;
	size_t slen;
	int fd;
	char *mm;
	struct stat sb;

	slen = strlen(bannerfile) + strlen(path) + strlen("/");
	s = malloc(slen + 1);
	if (s == NULL) {
		listerror("malloc");
	}
	snprintf(s, slen + 1, "%s/%s", path, bannerfile);

	errno = 0;
	fd = open(s, O_RDONLY);
	free(s);
	if (fd == -1) {
		if (errno == ENOENT) {
			/* A banner does not exist for this directory; this is fine. */
			return;
		}

		/* A real error occured. */
		listerror("open");
	}

	if (fstat(fd, &sb) == -1) {
		listerror("fstat");
	}

	/* We're mapping read/write so we can conveniently tokenise in-place with \0s */
	errno = 0;
	mm = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mm == MAP_FAILED) {
		listerror("mmap");
	}

	showbanner(mm);

	munmap(mm, sb.st_size);
	close(fd);
}

