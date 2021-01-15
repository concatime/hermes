#include "../common.h"
#include <stdarg.h> /* va_* */

char *strcpy_v(char *dst, size_t dst_len, ...) {
	va_list argp;
	const char *src, *end;
	uint_fast8_t b;

	if (dst_len == 0) {
		return dst;
	}

	/* -1 for NUL */
	end = dst + dst_len - 1;

	va_start(argp, dst_len);

	b = 0;
	while (!b) {
		src = va_arg(argp, const char *);
		/* sentinel */
		if (src == NULL) {
			break;
		}

		for (;; ++src, ++dst) {
			if (dst == end) {
				b = 1;
				break;
			}

			if (*src == '\0') {
				break;
			}

			*dst = *src;
		}
	}

	va_end(argp);
	*dst = '\0';
	return dst;
}
