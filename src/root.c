/*
 * Root constraints. This is either chrooted, or a transparently-
 * prepended path.
 */

/* for chroot(2) */
#define _BSD_SOURCE
#include <unistd.h>
#undef _BSD_SOURCE

#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pwd.h>
#include <stdio.h>

#include "gopherd.h"

char *root;
bool chrooted;

/*
 * Strip off the prepended root path if neccessary. This is used to santize the
 * output when the server is unable to chroot and so has prepended the root.
 */
const char *
striproot(const char *path)
{
	if (root != NULL && !chrooted) {
		return path + strlen(root);
	}

	return path;
}

/*
 * Returns the path based from the selector read. The two operations are
 * combined here because the selector is prepended with the root path if a
 * chroot cannot be performed. Caller frees.
 *
 * The path returned always represents the path on the filesystem. It it was
 * prepended with the root (that is, if root is set, but chrooted is false)
 * then the root should be skipped before output, for consistency to the client.
 * This way the client is returned links which look like their origional
 * selector.
 */
char *
readandchroot(const char *user)
{
	char selector[MAXPATHLEN];
	char path[MAXPATHLEN];
	struct passwd *pw;

	/*
	 * Find the user to switch to. This must be done before chroot, because
	 * getpwnam() needs to read /etc/passwd.
	 */
	if (user != NULL) {
		errno = 0;
		pw = getpwnam(user);
		if (!pw) {
			listerror("unknown user");
		}
	}

	/*
	 * Perform the chroot. This must be done before changing user as chroot may only
	 * be done by the root user. If the user is not root, we will prepend the root
	 * path to our selector further on, as a substitute.
	 */
	chrooted = false;
	if (root != NULL && getuid() == 0) {
		if (chroot(root) == -1) {
			listerror("chroot");
		}
		chrooted = true;
	}

	/*
	 * Switch user.
	 */
	if (user != NULL && pw) {
		if (setgid(pw->pw_gid) == -1) {
			listerror("setgid");
		}

		if (setuid(pw->pw_uid) == -1) {
			listerror("setuid");
		}

		endpwent();
	}

	/*
	 * Find and simplify the given selector into a selection path.
	 */
	fgets(selector, MAXPATHLEN, stdin);
	selector[strcspn(selector, "\r\n")] = '\0';
	if (strlen(selector) == 0) {
		strcpy(selector, "/");
	}
	if (root != NULL && !chrooted) {
		char s[MAXPATHLEN];

		strncpy(s, selector, sizeof(s) - 1);
		s[sizeof(s) - 1] = '\0';
		snprintf(selector, MAXPATHLEN, "%s/%s", root, s);
	}
	if (!realpath(selector, path)) {
		listerror("realpath");
	}

	/*
	 * Check that the selection path is inside the given root.
	 */
	if (root != NULL && !chrooted) {
		if (strncmp(path, root, strlen(root))) {
			errno = EACCES;
			listerror("root");
		}
	}

	return strdup(path);
}

