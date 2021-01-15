#include "../common.h"

char *strcpy_vv(str_m_t out, size_t ins_size, const str_t ins[]) {
	char *cur;
	size_t i, size;

	if (out.size == 0) {
		return out.buf;
	}

	cur = out.buf;

	for (i = 0, size = out.size - 1; i < ins_size; ++i) {
		const str_t in = ins[i];
		if (in.size > size) {
			size_t j;
			for (j = 0; j < in.size && size != 0; ++j, ++cur, --size) {
				if (in.buf[j] == '\0') {
					break;
				}
				*cur = in.buf[j];
			}
			if (size == 0) {
				break;
			}
		} else {
			size_t j;
			for (j = 0; j < in.size; ++j, ++cur) {
				if (in.buf[j] == '\0') {
					break;
				}
				*cur = in.buf[j];
			}
			size -= j;
		}
	}

	*cur = '\0';
	return cur;
}
