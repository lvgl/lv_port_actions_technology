/*
 * Copyright (c) 2020 Actions Corporation.
 * Copy from Linux
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <nls.h>

/*
 * Sample implementation from Unicode home page.
 * http://www.stonehand.com/unicode/standard/fss-utf.html
 */
struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	long    lmask;
	long    lval;
};

static const struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,						       /* end of table    */}
};

#define UNICODE_MAX	0x0010ffff
#define PLANE_SIZE	0x00010000

#define SURROGATE_MASK	0xfffff800
#define SURROGATE_PAIR	0x0000d800
#define SURROGATE_LOW	0x00000400
#define SURROGATE_BITS	0x000003ff

int utf8_to_utf32(const u8_t *s, int inlen, unicode_t *pu)
{
	unsigned long l;
	int c0, c, nc;
	const struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval || l > UNICODE_MAX ||
					(l & SURROGATE_MASK) == SURROGATE_PAIR)
				return -1;
			*pu = (unicode_t) l;
			return nc;
		}
		if (inlen <= nc)
			return -1;
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -1;
		l = (l << 6) | c;
	}
	return -1;
}

int utf32_to_utf8(unicode_t u, u8_t *s, int maxout)
{
	unsigned long l;
	int c, nc;
	const struct utf8_table *t;

	if (!s)
		return 0;

	l = u;
	if (l > UNICODE_MAX || (l & SURROGATE_MASK) == SURROGATE_PAIR)
		return -1;

	nc = 0;
	for (t = utf8_table; t->cmask && maxout; t++, maxout--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = (u8_t) (t->cval | (l >> c));
			while (c > 0) {
				c -= 6;
				s++;
				*s = (u8_t) (0x80 | ((l >> c) & 0x3F));
			}
			return nc;
		}
	}
	return -1;
}