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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "gopherd.h"

/*
 * Default server address and port.
 */
char *server = "localhost";
unsigned short port = 70;

int main(int argc, char *argv[]) {
	struct stat sb;
	int c;
	char *user = NULL;
	char *path;

	/* TODO some option to specify a banner file for "welcome to such-and-such server" */
	/* TODO option to syslog requests */
	/* TODO does inetd provide those as environment variables? */
	while((c = getopt(argc, argv, "ahr:u:")) != -1) {
		switch(c) {
		case 'p':
			port = atoi(optarg);
			break;

		case 's':
			server = optarg;
			break;

		case 'r':
			root = optarg;
			break;

		case 'u':
			user = optarg;
			break;

		case 'a':
			showhidden = true;
			break;

		case 'h':
		case '?':
		default:
			/* TODO document usage in a manpage or somesuch */
			printf("usage: %s [ -h | -a | -r <root> | -u <user> | -s <server> | -p <port> ]\n", argv[0]);
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
		dirmenu(path, server, port);
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

