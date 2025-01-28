/*
 * Copyright (c) 2020 Actions Corporation.
 * Copy from Linux
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NLS_H_
#define _NLS_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Unicode has changed over the years.  Unicode code points no longer
 * fit into 16 bits; as of Unicode 5 valid code points range from 0
 * to 0x10ffff (17 planes, where each plane holds 65536 code points).
 *
 * The original decision to represent Unicode characters as 16-bit
 * wchar_t values is now outdated.  But plane 0 still includes the
 * most commonly used characters, so we will retain it.  The newer
 * 32-bit unicode_t type can be used when it is necessary to
 * represent the full Unicode character set.
 */

/* Plane-0 Unicode character */
#define MAX_WCHAR_T	0xffff

/* Arbitrary Unicode character */
typedef unsigned int unicode_t;

/* nls_base.c */
int utf8_to_utf32(const u8_t *s, int inlen, unicode_t *pu);
int utf32_to_utf8(unicode_t u, u8_t *s, int maxout);

/* nls_utf8.c */
#ifdef CONFIG_NLS_UTF8
int nls_utf8_uni2char(u16_t uni, u8_t *out, int boundlen);
int nls_utf8_char2uni(const u8_t *rawstring, int boundlen, u16_t *uni);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NLS_H_ */
