#include "../common.h"
#include <stdarg.h> /* va_* */

char *strcpy_v(str_m_t dst, ...) {
	va_list argp;
	const char *src, *end;
	char *cur;
	uint_fast8_t b;

	if (dst.size == 0) {
		return dst.buf;
	}

	cur = dst.buf;
	/* -1 for NUL */
	end = cur + dst.size - 1;

	va_start(argp, dst);

	b = 0;
	while (!b) {
		src = va_arg(argp, const char *);
		/* sentinel */
		if (src == NULL) {
			break;
		}

		for (;; ++src, ++cur) {
			if (cur == end) {
				b = 1;
				break;
			}

			if (*src == '\0') {
				break;
			}

			*cur = *src;
		}
	}

	va_end(argp);
	*cur = '\0';
	return cur;
}
