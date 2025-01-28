/* fprintf.c */

/*
 * Copyright (c) 1997-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdio.h>
#include <sys/cbprintf.h>

#define DESC(d) ((void *)d)

#ifdef __UVISION_VERSION
int $Sub$$fprintf(FILE *_MLIBC_RESTRICT F, const char *_MLIBC_RESTRICT format, ...)
#else
int fprintf(FILE *_MLIBC_RESTRICT F, const char *_MLIBC_RESTRICT format, ...)
#endif
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = cbvprintf(fputc, DESC(F), format, vargs);
	va_end(vargs);

	return r;
}

#ifdef __UVISION_VERSION
int $Sub$$vfprintf(FILE *_MLIBC_RESTRICT F, const char *_MLIBC_RESTRICT format,
	     va_list vargs)
#else
int vfprintf(FILE *_MLIBC_RESTRICT F, const char *_MLIBC_RESTRICT format,
	     va_list vargs)
#endif
{
	int r;

	r = cbvprintf(fputc, DESC(F), format, vargs);

	return r;
}

#ifdef __UVISION_VERSION
int $Sub$$printf(const char *_MLIBC_RESTRICT format, ...)
#else
int printf(const char *_MLIBC_RESTRICT format, ...)
#endif
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = cbvprintf(fputc, DESC(stdout), format, vargs);
	va_end(vargs);

	return r;
}

#ifdef __UVISION_VERSION
int $Sub$$vprintf(const char *_MLIBC_RESTRICT format, va_list vargs)
#else
int vprintf(const char *_MLIBC_RESTRICT format, va_list vargs)
#endif
{
	int r;

	r = cbvprintf(fputc, DESC(stdout), format, vargs);

	return r;
}
