/*
 * Single file handling.
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <magic.h>

#include "gopherd.h"

/*
 * Output the given file.
 */
void
mapfile(const char *path, size_t len)
{
	char *mm;
	int fd;

	assert(path != NULL);

	if (len == 0) {
		return;
	}

	errno = 0;
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		listerror("open");
	}

	errno = 0;
	mm = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mm == MAP_FAILED) {
		listerror("mmap");
	}

	fwrite(mm, len, 1, stdout);

	munmap(mm, len);
	close(fd);
}


/*
 * Attempt to find a file's type. If the extension is not recognised,
 * an attempt is made to guess the file's contents via libmagic.
 */
enum filetype
findtype(const char *ext, const char *path)
{
	magic_t mt;
	const char *ms;
	enum filetype ft;

	/* TODO: replace with binary-search table. Or maybe a list of regexps */
	if (ext == NULL) {
		goto magic;
	} else if (!strcasecmp(ext, "txt")) {
		/* plain text */
		return FT_TEXT;
	} else if (!strcasecmp(ext, "png")
		|| !strcasecmp(ext, "jpg")
		|| !strcasecmp(ext, "jpeg")
		|| !strcasecmp(ext, "bmp")) {
		/* image */
		return FT_IMAGE;
	} else if (!strcasecmp(ext, "gif")) {
		return FT_GIF;
	} else if (!strcasecmp(ext, "wav")
		|| !strcasecmp(ext, "ogg")
		|| !strcasecmp(ext, "mp3")) {
		/* audio */
		return FT_AUDIO;
	} else if (!strcasecmp(ext, "hqx")
		|| !strcasecmp(ext, "hcx")) {
		/* BinHex - Also .hex, but we let libmagic determine that */
		return FT_BINHEX;
	}

magic:

	/* unrecognised extension: attempt to guess by contents */
	mt = magic_open(MAGIC_SYMLINK | MAGIC_ERROR | MAGIC_MIME);
	if (!mt) {
		return FT_BINARY;
	}

	if (magic_load(mt, NULL) == -1) {
		return FT_BINARY;
	}

	ms = magic_file(mt, path);
	if (!ms) {
		return FT_BINARY;
	}

	/* TODO map in more mime types here, including UUE (6) and BinHex (4) */
	if (!strncmp(ms, "text/html", strlen("text/html"))) {
		ft = FT_HTML;
	} else if (!strncmp(ms, "text/", strlen("text/"))) {
		ft = FT_TEXT;
	} else {
		/* still unrecognised: binary */
		ft = FT_BINARY;
	}

	magic_close(mt);

	return ft;
}

