/* https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=287603#5 */

#include "z.h"
#include "common.h"
#include <assert.h> /* assert */
#include <string.h> /* memcmp */
#include <stdio.h> /* snprintf */
#include <limits.h> /* SSIZE_MAX */

int_fast8_t z_create(struct z *ctx, const uint8_t *buf, size_t len) {
	int ret;

	assert(ctx != NULL);

	if (len > UINT32_MAX) {
        ERR2("%zu is bigger than %u", len, UINT32_MAX);
        return -2; /* EOVERFLOW */
    }

	ctx->z.next_in = buf;
	ctx->z.avail_in = len;
	ctx->z.opaque = Z_NULL;
	ctx->z.zalloc = Z_NULL;
	ctx->z.zfree = Z_NULL;

	/* NOLINTNEXTLINE(hicpp-signed-bitwise) */
	ret = inflateInit2(&(ctx->z), MAX_WBITS | 16U);
	if (ret != Z_OK) {
		return -1;
	}

	return 0;
}

void z_destroy(struct z *ctx) {
	inflateEnd(&(ctx->z));
}

void z_err(struct z *ctx, char *buf, size_t len) {
	snprintf(buf, len, "Unable to inflate: %s\n", ctx->z.msg);
}


ssize_t z_read(void *_ctx, uint8_t *buf, size_t len) {
    struct z *ctx = _ctx;

    if (len > UINT32_MAX) {
        len = UINT32_MAX;
    }

    if (len > SSIZE_MAX) {
        len = SSIZE_MAX;
    }

	ctx->z.avail_out = len;
	ctx->z.next_out = buf;

	switch (inflate(&ctx->z, Z_NO_FLUSH)) {
    case Z_OK:
    case Z_STREAM_END:
		return len - ctx->z.avail_out;
	}

	return -1;
}

uint_fast8_t z_check(const uint8_t *buf, size_t len) {
	/* https://www.ietf.org/rfc/rfc1952.txt
     * 2.3.1. Member header and trailer
	 */
    return len >= 2 && !memcmp("\037\213", buf, 2);
}