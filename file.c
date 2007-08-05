/*
 * Single file handling.
 *
 * $Id$
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <magic.h>

#include "gopherd.h"

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
 * Attempt to find a file's type. If the extension is not recognised,
 * an attempt is made to guess the file's contents via libmagic.
 */
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

