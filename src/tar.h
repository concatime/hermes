#ifndef TAR_H
#define TAR_H

#include <stddef.h> /* size_t */
#include <stdint.h> /* [u]int*_t */
#include <time.h>   /* time_t */
#include <sys/types.h> /* ssize_t */

#include "fd.h"

#define TAR_REGULAR 0U
#define TAR_NORMAL (unsigned int)'0'
#define TAR_HARDLINK (unsigned int)'1'
#define TAR_SYMLINK (unsigned int)'2'
#define TAR_CHAR (unsigned int)'3'
#define TAR_BLOCK (unsigned int)'4'
#define TAR_DIRECTORY (unsigned int)'5'
#define TAR_FIFO (unsigned int)'6'
#define TAR_CONTIGUOUS (unsigned int)'7'
#define TAR_PAX_GLOBAL_HEADER (unsigned int)'g'
#define TAR_PAX_PERFILE_HEADER (unsigned int)'x'

#define TAR_BLOCKSIZE 512U
#define TAR_TEXT_LEN 100U
#define TAR_MODE_LEN 8U
#define TAR_UID_LEN 8U
#define TAR_GID_LEN 8U
#define TAR_SIZE_LEN 12U
#define TAR_TIME_LEN 12U
#define TAR_CSUM_LEN 8U
#define TAR_USTAR_LEN 8U
#define TAR_MAJOR_LEN 8U
#define TAR_MINOR_LEN 8U
#define TAR_PREFIX_LEN 155U

/* Worst case:
 * text = 100 (no NUL)
 * prefix = 155 (no NUL)
 * fullname = 155 + '/' + 100 + '\0' = 257
 */
#define TAR_FULLNAME_LEN (TAR_PREFIX_LEN + TAR_TEXT_LEN + 2)

/* No Pre-POSIX.1-1988 format
 * Only UStar format (POSIX IEEE P1003.1)
 */
struct tar_blk {
	char    name[TAR_TEXT_LEN];   /* file name */
	uint8_t mode[TAR_MODE_LEN];   /* permissions */
	uint8_t uid[TAR_UID_LEN];     /* user id (octal) */
	uint8_t gid[TAR_GID_LEN];     /* group id (octal) */
	uint8_t size[TAR_SIZE_LEN];   /* size (octal) */
	uint8_t time[TAR_TIME_LEN];   /* modification time (octal) */
	uint8_t csum[TAR_CSUM_LEN];
	uint8_t type;                    /* file type */
	char    link[TAR_TEXT_LEN];      /* link name */
	uint8_t ustar[TAR_USTAR_LEN];    /* magic+version (ustar\000) */
	uint8_t uname[32];               /* uname (string) */
	uint8_t gname[32];               /* gname (string) */
	uint8_t devmajor[TAR_MAJOR_LEN]; /* device major number */
	uint8_t devminor[TAR_MINOR_LEN]; /* device minor number */
	char    prefix[TAR_PREFIX_LEN];
	uint8_t pad[12];
};

#define TAR_INIT(var) var = {0, 0, FD_INIT(.fd), .ts[0].tv_nsec = 0, .ts[1].tv_nsec = 0}

struct tar {
	uint8_t par;
	size_t size;
	struct fd fd;
	struct timespec ts[2];
	struct tar_blk tar_blk;
};

int_fast8_t tar_create(struct tar *ctx);

void tar_destroy(struct tar *ctx);

/* implement write interface */
ssize_t tar_write(void *_ctx, const uint8_t *buf, size_t len);

/* UTILS */

uint_fast8_t tar_check(const uint8_t *buf, size_t len);

const char *tar_blk_name(const struct tar_blk *blk,
                      char fullname[TAR_FULLNAME_LEN]);

int_fast8_t tar_blk_magic(const struct tar_blk *blk);

mode_t tar_blk_mode(const struct tar_blk *blk);

time_t tar_blk_time(const struct tar_blk *blk);

size_t tar_blk_size(const struct tar_blk *blk);

uint_fast32_t tar_blk_csum(const struct tar_blk *blk);

#endif /* TAR_H */
