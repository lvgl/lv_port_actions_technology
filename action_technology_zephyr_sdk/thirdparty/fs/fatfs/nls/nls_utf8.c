/*
 * Copyright (c) 2020 Actions Corporation.
 * Copy from Linux
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "nls.h"

int nls_utf8_uni2char(u16_t uni, u8_t *out, int boundlen)
{
	int n;

	if (boundlen <= 0)
		return -ENAMETOOLONG;

	n = utf32_to_utf8(uni, out, boundlen);
	if (n < 0) {
		*out = '?';
		return -EINVAL;
	}
	return n;
}

int nls_utf8_char2uni(const u8_t *rawstring, int boundlen, u16_t *uni)
{
	int n;
	unicode_t u;

	n = utf8_to_utf32(rawstring, boundlen, &u);
	if (n < 0 || u > MAX_WCHAR_T) {
		*uni = 0x003f;	/* ? */
		return -EINVAL;
	}
	*uni = (u16_t) u;
	return n;
}
