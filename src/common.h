#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>    /* [u]int*_t */
#include <stdlib.h>    /* size_t */
#include <sys/types.h> /* ssize_t */

#define ERR(msg) err_(__FILE__, __LINE__, msg)
#define ERR1(fmt, a) err_(__FILE__, __LINE__, fmt, a)
#define ERR2(fmt, a, b) err_(__FILE__, __LINE__, fmt, a, b)
#define ERR3(fmt, a, b, c) err_(__FILE__, __LINE__, fmt, a, b, c)
#define ERR4(fmt, a, b, c, d) err_(__FILE__, __LINE__, fmt, a, b, c, d)
#define ERR5(fmt, a, b, c, d, e) err_(__FILE__, __LINE__, fmt, a, b, c, d, e)

typedef struct str {
	size_t size;
	const char *buf;
} str_t;

typedef struct str_m {
	size_t size;
	char *buf;
} str_m_t;

typedef ssize_t (*read_f)(void *, uint8_t *, size_t);
typedef ssize_t (*write_f)(void *, const uint8_t *, size_t);

/* https://lemire.me/blog/2019/04/17/parsing-short-hexadecimal-strings-efficiently/
 */
static inline uint8_t hex_to_byte(unsigned char c) {
	return (uint8_t)((uint8_t)(c & 0xFU) + 9 * (c >> 6U));
}

__attribute__((format(printf, 3, 4))) void
err_(const char *file, unsigned long line, const char *format, ...);

__attribute__((sentinel)) char *strcpy_v(str_m_t dst, ...);

char *strcpy_vv(str_m_t out, size_t ins_size, const str_t ins[]);

char *strdup_l(const char *str, size_t *len_p);

#endif /* COMMON_H */
