/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* poriting sscanf from musl-1.2.3 */

#ifndef __UVISION_VERSION

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>

typedef struct _SBUF {
	unsigned char *rpos, *rend;
	unsigned char *buf;
	void *cookie;
	unsigned char *shend;
	off_t shlim, shcnt;
} _BUF;

#define F_PERM 1
#define F_NORD 4
#define F_NOWR 8
#define F_EOF 16
#define F_ERR 32
#define F_SVB 64
#define F_APP 128

#define SIZE_hh -2
#define SIZE_h  -1
#define SIZE_def 0
#define SIZE_l   1
#define SIZE_L   2
#define SIZE_ll  3

#define shcnt(f) ((f)->shcnt + ((f)->rpos - (f)->buf))
#define shlim(f, lim) __shlim((f), (lim))
#define shgetc(f) (((f)->rpos != (f)->shend) ? *(f)->rpos++ : __shgetc(f))
#define shunget(f) ((f)->shlim>=0 ? (void)(f)->rpos-- : (void)0)

/* Lookup table for digit values. -1==255>=36 -> invalid */
static const unsigned char table[] = { -1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
-1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
-1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

static int __toread(_BUF *f)
{
	f->rpos = f->rend = f->buf;
	return 0;
}

static size_t __read(_BUF *f, unsigned char *buf, size_t len)
{
	char *src = f->cookie;
	size_t k = len+256;
	char *end = memchr(src, 0, k);
	if (end) k = end-src;
	if (k < len) len = k;
	memcpy(buf, src, len);
	f->rpos = (void *)(src+len);
	f->rend = (void *)(src+k);
	f->cookie = src+k;
	return len;
}

static int __uflow(_BUF *f)
{
	unsigned char c;
	if (!__toread(f) && __read(f, &c, 1)==1) return c;
	return EOF;
}

static void __shlim(_BUF *f, off_t lim)
{
	f->shlim = lim;
	f->shcnt = f->buf - f->rpos;
	/* If lim is nonzero, rend must be a valid pointer. */
	if (lim && f->rend - f->rpos > lim)
		f->shend = f->rpos + lim;
	else
		f->shend = f->rend;
}

static int __shgetc(_BUF *f)
{
	int c;
	off_t cnt = shcnt(f);
	if ((f->shlim && cnt >= f->shlim) || (c=__uflow(f)) < 0) {
		f->shcnt = f->buf - f->rpos + cnt;
		f->shend = f->rpos;
		f->shlim = -1;
		return EOF;
	}
	cnt++;
	if (f->shlim && f->rend - f->rpos > f->shlim - cnt)
		f->shend = f->rpos + (f->shlim - cnt);
	else
		f->shend = f->rend;
	f->shcnt = f->buf - f->rpos + cnt;
	if (f->rpos <= f->buf) f->rpos[-1] = c;
	return c;
}

static unsigned long long __intscan(_BUF *f, unsigned base, int pok, unsigned long long lim)
{
	const unsigned char *val = table+1;
	int c, neg=0;
	unsigned x;
	unsigned long long y;
	if (base > 36 || base == 1) {
		errno = EINVAL;
		return 0;
	}
	while (isspace((c=shgetc(f))));
	if (c=='+' || c=='-') {
		neg = -(c=='-');
		c = shgetc(f);
	}
	if ((base == 0 || base == 16) && c=='0') {
		c = shgetc(f);
		if ((c|32)=='x') {
			c = shgetc(f);
			if (val[c]>=16) {
				shunget(f);
				if (pok) shunget(f);
				else shlim(f, 0);
				return 0;
			}
			base = 16;
		} else if (base == 0) {
			base = 8;
		}
	} else {
		if (base == 0) base = 10;
		if (val[c] >= base) {
			shunget(f);
			shlim(f, 0);
			errno = EINVAL;
			return 0;
		}
	}
	if (base == 10) {
		for (x=0; c-'0'<10U && x<=UINT_MAX/10-1; c=shgetc(f))
			x = x*10 + (c-'0');
		for (y=x; c-'0'<10U && y<=ULLONG_MAX/10 && 10*y<=ULLONG_MAX-(c-'0'); c=shgetc(f))
			y = y*10 + (c-'0');
		if (c-'0'>=10U) goto done;
	} else if (!(base & (base-1))) {
		int bs = "\0\1\2\4\7\3\6\5"[(0x17*base)>>5&7];
		for (x=0; val[c]<base && x<=UINT_MAX/32; c=shgetc(f))
			x = x<<bs | val[c];
		for (y=x; val[c]<base && y<=ULLONG_MAX>>bs; c=shgetc(f))
			y = y<<bs | val[c];
	} else {
		for (x=0; val[c]<base && x<=UINT_MAX/36-1; c=shgetc(f))
			x = x*base + val[c];
		for (y=x; val[c]<base && y<=ULLONG_MAX/base && base*y<=ULLONG_MAX-val[c]; c=shgetc(f))
			y = y*base + val[c];
	}
	if (val[c]<base) {
		for (; val[c]<base; c=shgetc(f));
		errno = ERANGE;
		y = lim;
		if (lim&1) neg = 0;
	}
done:
	shunget(f);
	if (y>=lim) {
		if (!(lim&1) && !neg) {
			errno = ERANGE;
			return lim-1;
		} else if (y>lim) {
			errno = ERANGE;
			return lim;
		}
	}
	return (y^neg)-neg;
}

static void store_int(void *dest, int size, unsigned long long i)
{
	if (!dest) return;
	switch (size) {
	case SIZE_hh:
		*(char *)dest = i;
		break;
	case SIZE_h:
		*(short *)dest = i;
		break;
	case SIZE_def:
		*(int *)dest = i;
		break;
	case SIZE_l:
		*(long *)dest = i;
		break;
	case SIZE_ll:
		*(long long *)dest = i;
		break;
	}
}

static void *arg_n(va_list ap, unsigned int n)
{
	void *p;
	unsigned int i;
	va_list ap2;
	va_copy(ap2, ap);
	for (i=n; i>1; i--) va_arg(ap2, void *);
	p = va_arg(ap2, void *);
	va_end(ap2);
	return p;
}

static int _vfscanf(_BUF *f, const char *fmt, va_list ap)
{
	int width;
	int size;
	int base;
	const unsigned char *p;
	int c, t;
	char *s;
	void *dest=NULL;
	int invert;
	int matches=0;
	unsigned long long x;
	off_t pos = 0;
	unsigned char scanset[257];
	size_t i;

	if (!f->rpos) __toread(f);
	if (!f->rpos) goto input_fail;

	for (p=(const unsigned char *)fmt; *p; p++) {
		if (isspace(*p)) {
			while (isspace(p[1])) p++;
			shlim(f, 0);
			while (isspace(shgetc(f)));
			shunget(f);
			pos += shcnt(f);
			continue;
		}
		if (*p != '%' || p[1] == '%') {
			shlim(f, 0);
			if (*p == '%') {
				p++;
				while (isspace((c=shgetc(f))));
			} else {
				c = shgetc(f);
			}
			if (c!=*p) {
				shunget(f);
				if (c<0) goto input_fail;
				goto match_fail;
			}
			pos += shcnt(f);
			continue;
		}

		p++;
		if (*p=='*') {
			dest = 0; p++;
		} else if (isdigit(*p) && p[1]=='$') {
			dest = arg_n(ap, *p-'0'); p+=2;
		} else {
			dest = va_arg(ap, void *);
		}

		for (width=0; isdigit(*p); p++) {
			width = 10*width + *p - '0';
		}

		size = SIZE_def;
		switch (*p++) {
		case 'h':
			if (*p == 'h') p++, size = SIZE_hh;
			else size = SIZE_h;
			break;
		case 'l':
			if (*p == 'l') p++, size = SIZE_ll;
			else size = SIZE_l;
			break;
		case 'j':
			size = SIZE_ll;
			break;
		case 'z':
		case 't':
			size = SIZE_l;
			break;
		case 'L':
			size = SIZE_L;
			break;
		case 'd': case 'i': case 'o': case 'u': case 'x':
		case 'a': case 'e': case 'f': case 'g':
		case 'A': case 'E': case 'F': case 'G': case 'X':
		case 's': case 'c': case '[':
		case 'S': case 'C':
		case 'p': case 'n':
			p--;
			break;
		default:
			goto fmt_fail;
		}

		t = *p;

		/* C or S */
		if ((t&0x2f) == 3) {
			t |= 32;
			size = SIZE_l;
		}

		switch (t) {
		case 'c':
			if (width < 1) width = 1;
		case '[':
			break;
		case 'n':
			store_int(dest, size, pos);
			/* do not increment match count, etc! */
			continue;
		default:
			shlim(f, 0);
			while (isspace(shgetc(f)));
			shunget(f);
			pos += shcnt(f);
		}

		shlim(f, width);
		if (shgetc(f) < 0) goto input_fail;
		shunget(f);

		switch (t) {
		case 's':
		case 'c':
		case '[':
			if (t == 'c' || t == 's') {
				memset(scanset, -1, sizeof scanset);
				scanset[0] = 0;
				if (t == 's') {
					scanset[1+'\t'] = 0;
					scanset[1+'\n'] = 0;
					scanset[1+'\v'] = 0;
					scanset[1+'\f'] = 0;
					scanset[1+'\r'] = 0;
					scanset[1+' '] = 0;
				}
			} else {
				if (*++p == '^') p++, invert = 1;
				else invert = 0;
				memset(scanset, invert, sizeof scanset);
				scanset[0] = 0;
				if (*p == '-') p++, scanset[1+'-'] = 1-invert;
				else if (*p == ']') p++, scanset[1+']'] = 1-invert;
				for (; *p != ']'; p++) {
					if (!*p) goto fmt_fail;
					if (*p=='-' && p[1] && p[1] != ']')
						for (c=p++[-1]; c<*p; c++)
							scanset[1+c] = 1-invert;
					scanset[1+*p] = 1-invert;
				}
			}
			s = 0;
			i = 0;
			if ((s = dest)) {
				while (scanset[(c=shgetc(f))+1])
					s[i++] = c;
			} else {
				while (scanset[(c=shgetc(f))+1]);
			}
			shunget(f);
			if (!shcnt(f)) goto match_fail;
			if (t == 'c' && shcnt(f) != width) goto match_fail;
			if (t != 'c') {
				if (s) s[i] = 0;
			}
			break;
		case 'p':
		case 'X':
		case 'x':
			base = 16;
			goto int_common;
		case 'o':
			base = 8;
			goto int_common;
		case 'd':
		case 'u':
			base = 10;
			goto int_common;
		case 'i':
			base = 0;
		int_common:
			x = __intscan(f, base, 0, ULLONG_MAX);
			if (!shcnt(f)) goto match_fail;
			if (t=='p' && dest) *(void **)dest = (void *)(uintptr_t)x;
			else store_int(dest, size, x);
			break;
		}
		pos += shcnt(f);
		if (dest) matches++;
	}
	if (0) {
fmt_fail:
input_fail:
		if (!matches) matches--;
	}
match_fail:
	return matches;
}

int vsscanf(const char *s, const char *fmt, va_list ap)
{
	_BUF f = {
		.buf = (void *)s, .cookie = (void *)s,
	};
	return _vfscanf(&f, fmt, ap);
}

int sscanf(const char *s, const char *fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vsscanf(s, fmt, ap);
	va_end(ap);
	return ret;
}

#endif
