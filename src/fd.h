#ifndef FD_H
#define FD_H

#include <assert.h> /* assert */
#include <stdint.h> /* [u]int*_t */
#include <stdlib.h> /* size_t */
#include <sys/types.h> /* off_t, ssize_t */
#include <fcntl.h> /* open */
#include <unistd.h> /* close, ftruncate, lseek */
#include <errno.h> /* EINTR, errno */
#include <limits.h> /* SSIZE_MAX */
#include <sys/stat.h> /* fstat, futimens, mode_t, struct stat */
#include <time.h> /* struct timespec */


/* usage:
 * struct fd FD_INIT(fd);
 * ... fd
 * struct meta meta = {FD_INIT(.fd)};
 * ... meta.fd
 */
#define FD_INIT(var) var = {0}

struct fd {
    uint8_t par;
    int fd;
};

static int_fast8_t fd_create(struct fd *ctx, const char *path, uint32_t flags, mode_t mode) {
    assert(ctx != NULL && ctx->par == 0);

    ctx->fd = open(path, flags, mode);
    if (ctx->fd < 0) {
        return -1;
    }

    ctx->par = 1;
    return 0;
}

static void fd_destroy(struct fd *ctx) {
    assert(ctx != NULL && ctx->par != 0);

    close(ctx->fd);
    ctx->par = 0;
}

static int_fast8_t fd_stat(const struct fd *ctx, struct stat *stat) {
    return fstat(ctx->fd, stat);
}

/* off_t fd_seek(const struct fd *ctx, off_t off, uint32_t flags)
 * return lseek(ctx->fd, off, flags);
 */

static int_fast8_t fd_truncate(const struct fd *ctx, size_t len) {
    /* OFF_MAX does not exist
     * so we cross our figers...
     */
    return ftruncate(ctx->fd, (off_t)len);
}

/* TODO(iemaghni): futimens + struct timespec */
static int_fast8_t fd_time(const struct fd *ctx, const struct timespec ts[2]) {
    return futimens(ctx->fd, ts);
}


/* implements read interface */
static ssize_t fd_read(void *_ctx, uint8_t *buf, size_t len) {
    const struct fd *ctx = _ctx;
	return read(ctx->fd, buf, len);
}

/* implements write interface */
static ssize_t fd_write(void *_ctx, const uint8_t *buf, size_t len) {
    ssize_t len2;
    const struct fd *ctx = _ctx;

    if (len > SSIZE_MAX) {
        len = SSIZE_MAX;
    }
    len2 = (ssize_t)len;

    do {
		const ssize_t wc = write(ctx->fd, buf, len);
		if (wc >= 0) {
			buf += wc;
			len -= (size_t)wc;
		} else if (errno != EINTR) {
			return -1;
		}
	} while (len != 0);

	return len2;
}

#endif /* FD_H */
