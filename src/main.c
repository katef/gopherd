/*
 * A gopher server. Menus are generated for directory listings.
 * This is intended to be launched from inetd. Something like:
 * 
 *  gopher stream tcp nowait nobody /path/to/gopherd gopherd
 */

#include <arpa/inet.h>
#include <sys/stat.h>

#include <unistd.h>
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "gopherd.h"

/*
 * Default server address and port. The port is set below.
 */
static char *server = "localhost";
static unsigned short port = 70;

/*
 * Lookup the default port of a given service.
 */
unsigned short
getservport(const char *service)
{
	unsigned short port;
	struct servent *se;

	errno = 0;
	se = getservbyname(service, NULL);
	if (se == NULL) {
		listerror("getservbyname");
	}

	port = ntohs(se->s_port);
	endservent();

	return port;
}

int
main(int argc, char *argv[])
{
	struct stat sb;
	char *user = NULL;
	char *path;

	/*
	 * Set the default port for responses. This may be overridden below.
	 */
	port = getservport("gopher");

	/* TODO some option to specify a banner file for "welcome to such-and-such server" */
	/* TODO option to syslog requests */
	/* TODO does inetd provide those as environment variables? */
	{
		int c;

		while (c = getopt(argc, argv, "viahr:u:b:s:p:"), c != -1) {
			switch(c) {
			case 'p':
				port = atoi(optarg);
				break;

			case 's': server     = optarg; break;
			case 'r': root       = optarg; break;
			case 'u': user       = optarg; break;
			case 'b': bannerfile = optarg; break;
			case 'i': hidesize   = false;  break;
			case 'a': showhidden = true;   break;

			case 'v':
				/* TODO from last / for argv[0] */
				printf("%s %s, %s\n", argv[0], VERSION, AUTHOR);
				return EXIT_SUCCESS;

			case 'h':
			case '?':
			default:
				/* TODO document usage in a manpage or somesuch */
				printf("usage: %s [ -h | -a | -b <bannerfile> | -r <root> | -u <user> | -s <server> | -p <port> ]\n", argv[0]);
				return EXIT_SUCCESS;
			}
		}

		argc -= optind;
		argv += optind;
	}

	/*
	 * Initialise and read input selector.
	 */
	path = readandchroot(user);
	if (path == NULL) {
		listerror("initialise");
	}

	errno = 0;
	if (stat(path, &sb) == -1) {
		listerror("stat");
	}

	if (S_ISDIR(sb.st_mode)) {
		dirmenu(path, server, port);
		printf(".\r\n");
	} else {
		char *ext;

		mapfile(path, sb.st_size);

		/* If it's not binary, end with a '.' */
		/* TODO do pictures need this? */
		ext = strrchr(path, '.');
		if (findtype(ext, path) != FT_BINARY) {
			printf(".\r\n");
		}
	}

	return EXIT_SUCCESS;
}

