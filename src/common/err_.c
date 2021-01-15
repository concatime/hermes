#include "../common.h"
#include <errno.h>  /* errno */
#include <stdarg.h> /* va_start, va_list */
#include <stdio.h>  /* fprintf, vfprintf, fputc, fputs */
#include <string.h> /* strerror_r(XSI-compliant) */

/* Longuest error is “Attempting to link in too many shared libraries” (47) */
#define ERRNO_LEN 64U
#define fputc(ctx, x) fputc(x, ctx)
#define fputs(ctx, x) fputs(x, ctx)

void err_(const char *const file, const unsigned long line,
		  const char *const format, ...) {
	char err[ERRNO_LEN];
	va_list argp;

	fprintf(stderr, "[%s:%lu] ", file, line);

	va_start(argp, format);
	vfprintf(stderr, format, argp);
	va_end(argp);

	if (errno != 0) {
		int ret;

		fputs(stderr, ": ");

		ret = strerror_r(errno, err, sizeof err);
		if (ret) {
			fprintf(stderr, "error number %d", ret);
		} else {
			fputs(stderr, err);
		}
	}

	fputc(stderr, '\n');
}
