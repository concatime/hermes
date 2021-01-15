#ifndef Z_H
#define Z_H

#include <stddef.h> /* size_t */
#include <stdint.h> /* [u]int*_t */
#include <sys/types.h> /* ssize_t */

#include <zlib.h>   /* z_stream */

#define CHUNK_LEN 65536UL

struct z {
    const uint8_t *buf;
	size_t len;
	z_stream z;
};

int_fast8_t z_create(struct z *ctx, const uint8_t *buf, size_t len);

void z_destroy(struct z *ctx);

void z_err(struct z *ctx, char *buf, size_t len);

/* implement read interface */
ssize_t z_read(void *_ctx, uint8_t *buf, size_t len);

uint_fast8_t z_check(const uint8_t *buf, size_t len);

#endif /* Z_H */