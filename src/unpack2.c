#include "mm.h"
#include "tar.h"
#include "z.h"
#include <assert.h>
#include <stdio.h>
#include <string.h> /* strdup */

#include <janet.h>

#define CHUNK_LEN 65536UL

static int_fast8_t unpack(const uint8_t *data, size_t len, char **dir) {
    static uint8_t chunk[CHUNK_LEN];

    uint_fast8_t type;

    if (z_check(data, len)) {
        ssize_t rc;
        struct z z;

        if (z_create(&z, data, len)) {
            return -1;
        }

        rc = z_read(&z, chunk, sizeof chunk);
        if (rc < 0) {
            return -1;
        }

        if (tar_check(chunk, (size_t)rc)) {
            struct tar TAR_INIT(tar);
            struct tar_blk *tar_blk = (struct tar_blk *)chunk;

            if (tar_create(&tar)) {
                return -1;
            }

            if (tar_blk_magic(tar_blk) != 0) {
                fputs("WARNING: not POSIX ustar\n", stderr);
            }

            if (dir != NULL) {
                if (tar_blk->type == TAR_PAX_GLOBAL_HEADER ||
                    tar_blk->type == TAR_PAX_PERFILE_HEADER) {
                    /* skip header and content */
                    /* suppose that content is <= 512 */
                    tar_blk += 2;
                }
                if (tar_blk->type == TAR_DIRECTORY) {
                    char fullname[TAR_FULLNAME_LEN];
                    const char *name = tar_blk_name(tar_blk, fullname);
                    *dir = strdup(name);
                }
            }

            while (rc > 0) {
                if (tar_write(&tar, chunk, (size_t)rc) < 0) {
                    return -1;
                }
                rc = z_read(&z, chunk, sizeof chunk);
                if (rc < 0) {
                    return -1;
                }
            }

            tar_destroy(&tar);
        }
        z_destroy(&z);
    }

    return 0;
}

Janet primitive_unpack2(int argc, Janet *argv) {
    int_fast8_t err;
    const char *fname;
    char *dir = NULL;
    struct fd FD_INIT(fd);
    struct mm MM_INIT(mm);
    JanetString jdir;

    janet_fixarity(argc, 1);
    fname = janet_getcstring(argv, 0);

    /* NOLINTNEXTLINE(hicpp-signed-bitwise) */
    if (fd_create(&fd, fname, O_RDONLY | O_CLOEXEC, 0)) {
        janet_panic("fd_create failed");
    }

	if (mm_create(&mm, &fd)) {
        janet_panic("mm_create failed");
	}

    err = unpack(mm.ptr, mm.len, &dir);

    mm_destroy(&mm);
    fd_destroy(&fd);

    if (err) {
        janet_panic("unpack failed");
    }

    if (dir == NULL) {
        janet_panic("dir is NULL");
    }

    jdir = janet_string(dir, strlen(dir));
    return janet_wrap_string(jdir);
}
