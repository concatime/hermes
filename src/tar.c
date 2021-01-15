/* futimes */
#define _GNU_SOURCE

#include "tar.h"
#include "common.h"
#include <stdlib.h> /* size_t */
#include <string.h> /* memcmp, memcpy */
#include <sys/stat.h> /* mkdir */
#include <errno.h> /* EEXIST, errno */
#include <unistd.h> /* ftruncate, symlink */
#include <fcntl.h> /* O_*, open */
#include <assert.h> /* assert */
#include <sys/time.h> /* struct timeval, futimes */
#include <limits.h> /* SSIZE_MAX */

static uint64_t parse_octal(const uint8_t *oct, size_t size) {
	uint64_t dec = 0;
    size_t i;
	for (i = 0; i != size; ++i) {
        uint64_t c = oct[i];
        if (c == '\0' || c == ' ') {
            break;
        }
		dec = dec << 3U | (c - '0');
	}
	return dec;
}

/* FROM SPECS
 * Header checksum, stored as an octal number in ASCII.  To compute
 * the checksum, set the checksum field to all spaces, then sum all
 * bytes in the header using unsigned arithmetic.  This field should
 * be stored as six octal digits followed by a null and a space
 * character.  Note that many early implementations of tar used
 * signed arithmetic for the checksum field, which can cause inter-
 * operability problems when transferring archives between systems.
 * Modern robust readers compute the checksum both ways and accept
 * the header if either computation matches.
 * It can fit in u32 (6x07 = 0777777 = 0x3FFFF < uint32_t).
 */
static uint_fast32_t calc_chksum(const uint8_t *buf) {
	size_t i;
	uint_fast32_t chksum = ' ' * 8;
	for (i = 0; i != 148; ++i) {
		chksum += buf[i];
	}
	for (i = 148 + 8; i != 512; ++i) {
		chksum += buf[i];
	}
	return chksum;
}

static int_fast8_t interpret_header(struct tar *ctx,
                          const struct tar_blk *blk) {
	mode_t mode;
	uint32_t flags;
	const char *name;
	char fullname[TAR_FULLNAME_LEN];

	if (!tar_check((const uint8_t *)blk, TAR_BLOCKSIZE)) {
		return -1;
	}

	name = tar_blk_name(blk, fullname);
	mode = tar_blk_mode(blk);

	switch (blk->type) {
	case TAR_DIRECTORY:
		if (mkdir(name, mode) < 0) {
			if (errno != EEXIST) {
				ERR1("Unable to create directory %s", name);
				return -1;
			}
		}
		/* KEEP ctx->size TO ZERO */
		break;
	case TAR_PAX_PERFILE_HEADER:
		name = ".pax_ignore";
	case TAR_PAX_GLOBAL_HEADER:
	case TAR_REGULAR:
	case TAR_NORMAL:
		/* NOLINTNEXTLINE(hicpp-signed-bitwise) */
		flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
		ctx->ts[1].tv_sec = tar_blk_time(blk);
		ctx->size = tar_blk_size(blk);

		if (fd_create(&ctx->fd, name, flags, mode)) {
			ERR("fd_create failed");
			return -1;
		}

		if (ctx->size == 0) {
			fd_time(&ctx->fd, ctx->ts);
			fd_destroy(&ctx->fd);
			return 0;
		}

		if (fd_truncate(&ctx->fd, ctx->size)) {
			ERR("fd_truncate failed");
			return -1;
		}

		break;
	case TAR_SYMLINK:
		if (symlink(blk->link, name) < 0) {
			ERR1("Unable to create symlink %s", name);
			return -1;
		}

		/* KEEP ctx->size TO ZERO */
		break;
	default:
		ERR2("Unsupported type %d for %s", blk->type, name);
		return -1;
	}

	return 0;
}

static int is_zero(const uint8_t *const buf) {
	static const uint8_t zeroBlock[TAR_BLOCKSIZE];
	return !memcmp(buf, zeroBlock, TAR_BLOCKSIZE);
}

/******************************************************************************/

int_fast8_t tar_create(struct tar *ctx) {
	time_t atime;

	/* assert(ctx != NULL && ctx->par == 0); */

	atime = time(NULL);
	if (atime == ((time_t)-1)) {
		return -1;
	}

	ctx->ts[0].tv_sec = atime;

	return 0;
}

void tar_destroy(struct tar *ctx) {
	(void)ctx;
}

/*
 * We suppose that the directory has its own header block
 * Also, when extracting all, the order of extraction
 * aloud us to not carry about recursion.
 */
ssize_t tar_write(void *_ctx, const uint8_t *buf, size_t len) {
	struct tar *ctx = _ctx;

	if (len > SSIZE_MAX) {
		len = SSIZE_MAX;
	}

	/* assert(!(len & (BLOCKSIZE - 1))); */
	assert(len != 0 && len % TAR_BLOCKSIZE == 0);

	while (len != 0) {
		if (ctx->size == 0) {
			if (is_zero(buf)) {
				break; /* TODO(iemaghni): check two zero blocks */
			}

			if (interpret_header(ctx, (struct tar_blk *)buf)) {
				return -1;
			}

			buf += TAR_BLOCKSIZE;
			len -= TAR_BLOCKSIZE;
		} else if (ctx->size <= len) {
			size_t padding, skip;
			ssize_t wc;

			wc = fd_write(&ctx->fd, buf, ctx->size);
			if (wc < 0) {
				return -1;
			}

			fd_time(&ctx->fd, ctx->ts);
			fd_destroy(&ctx->fd);

			padding = ctx->size % TAR_BLOCKSIZE;
			skip = ctx->size + (padding != 0 ? TAR_BLOCKSIZE - padding : 0);
			buf += skip;
			len -= skip;
			ctx->size = 0;
		} else { /* ctx->size > len */
			ssize_t wc;

			wc = fd_write(&ctx->fd, buf, len);
			if (wc < 0) {
				return -1;
			}

			ctx->size -= len;
			break;
		}
	}

	return 0;
}

uint_fast8_t tar_check(const uint8_t *buf, size_t len) {
	struct tar_blk *blk;

	if (len < TAR_BLOCKSIZE) {
		return 0;
	}

	blk = (struct tar_blk *)buf;

	int_fast8_t t = tar_blk_magic(blk);

	return t >= 0 && tar_blk_csum(blk) == calc_chksum(buf);
}

/* FROM SPEC
 * If the pathname is too long to fit in the 100 bytes provided by the standard
 * format, it can be split at any / character with the first portion going into
 * the prefix field. If the prefix field is not empty, the reader will prepend
 * the prefix value and a / character to the regular name field to obtain the
 * full pathname. The standard does not require a trailing / character on
 * directory names, though most implementations still include this for
 * compatibility reasons.
 */
const char *tar_blk_name(const struct tar_blk *blk,
                      char fullname[TAR_FULLNAME_LEN]) {
	if (blk->prefix[0] != '\0') {
		strcpy_vv((str_m_t){TAR_FULLNAME_LEN, fullname}, 3,
			(str_t[]){
				{sizeof blk->prefix, blk->prefix},
				{2, "/"},
				{sizeof blk->name, blk->name}});
		return fullname;
	}

	if (blk->name[(sizeof blk->name) - 1] != '\0') {
		/* (sizeof fullname) > (sizeof blk->name) */
		memcpy(fullname, blk->name, sizeof blk->name);
		fullname[sizeof blk->name] = '\0';
		return fullname;
	}

	return blk->name;
}

int_fast8_t tar_blk_magic(const struct tar_blk *blk) {
	/* POSIX ustar */
	if (!memcmp(blk->ustar, "ustar\00000", sizeof blk->ustar)) {
		return 0;
	}

	/* GNU tar */
	if (!memcmp(blk->ustar, "ustar  \0", sizeof blk->ustar)) {
		return 1;
	}

	if (!memcmp(blk->ustar, "\0\0\0\0\0\0\0\0", sizeof blk->ustar)) {
		return 2;
	}

	return -1;
}

mode_t tar_blk_mode(const struct tar_blk *blk) {
	return parse_octal(blk->mode, sizeof blk->mode);
}

time_t tar_blk_time(const struct tar_blk *blk) {
	return parse_octal(blk->time, sizeof blk->time);
}

size_t tar_blk_size(const struct tar_blk *blk) {
	return parse_octal(blk->size, sizeof blk->size);
}

uint_fast32_t tar_blk_csum(const struct tar_blk *blk) {
	return parse_octal(blk->csum, sizeof blk->csum);
}
