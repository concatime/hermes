#ifndef MM_H
#define MM_H

#include <assert.h> /* assert */
#include <stdint.h> /* [u]int*_t */
#include <stdlib.h> /* size_t */
#include <sys/mman.h> /* MAP_FAILED, mmap, munmap */

#include "fd.h"

/* usage:
 * struct mm MM_INIT(mm);
 */
#define MM_INIT(var) var = {0}

struct mm {
    uint8_t par;
    const uint8_t *ptr;
    size_t len;
};


int_fast8_t mm_create(struct mm *ctx, const struct fd *fd) {
    struct stat stat;

    assert(ctx != NULL && ctx->par == 0);
    assert(fd != NULL && fd->par != 0);

    if (fd_stat(fd, &stat)) {
        return -1;
    }

    ctx->len = (size_t)stat.st_size;

    ctx->ptr = mmap(NULL, ctx->len, PROT_READ, MAP_PRIVATE, fd->fd, 0);
    if (ctx->ptr == MAP_FAILED) {
        return -1;
    }

    ctx->par = 1;
    return 0;
}

void mm_destroy(struct mm *ctx) {
    assert(ctx != NULL && ctx->par != 0);

    munmap((uint8_t *)ctx->ptr, ctx->len);

    ctx->par = 0;
}

#endif /* MM_H */