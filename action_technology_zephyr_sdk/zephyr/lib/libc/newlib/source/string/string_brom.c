/* string.c - common string routines */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <brom_interface.h>

#ifdef __cplusplus
	#define _MLIBC_RESTRICT __restrict__
#else
	#define _MLIBC_RESTRICT restrict
#endif

/**
 *
 * @brief Copy a string
 *
 * @return pointer to destination buffer <d>
 */

char *strcpy(char *_MLIBC_RESTRICT d, const char *_MLIBC_RESTRICT s)
{
	return pbrom_libc_api->p_strcpy(d, s);
}

/**
 *
 * @brief Copy part of a string
 *
 * @return pointer to destination buffer <d>
 */

char *strncpy(char *_MLIBC_RESTRICT d, const char *_MLIBC_RESTRICT s, size_t n)
{
	return pbrom_libc_api->p_strncpy(d, s, n);
}

/**
 *
 * @brief String scanning operation
 *
 * @return pointer to 1st instance of found byte, or NULL if not found
 */

char *strchr(const char *s, int c)
{
	return pbrom_libc_api->p_strchr(s, c);
}

/**
 *
 * @brief String scanning operation
 *
 * @return pointer to last instance of found byte, or NULL if not found
 */

char *strrchr(const char *s, int c)
{
	return pbrom_libc_api->p_strrchr(s, c);
}

/**
 *
 * @brief Get string length
 *
 * @return number of bytes in string <s>
 */

size_t strlen(const char *s)
{
	return pbrom_libc_api->p_strlen(s);
}

/**
 *
 * @brief Get fixed-size string length
 *
 * @return number of bytes in fixed-size string <s>
 */

size_t strnlen(const char *s, size_t maxlen)
{
	return pbrom_libc_api->p_strnlen(s, maxlen);
}

/**
 *
 * @brief Compare two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strcmp(const char *s1, const char *s2)
{
	return pbrom_libc_api->p_strcmp(s1, s2);
}

/**
 *
 * @brief Compare part of two strings
 *
 * @return negative # if <s1> < <s2>, 0 if <s1> == <s2>, else positive #
 */

int strncmp(const char *s1, const char *s2, size_t n)
{
	return pbrom_libc_api->p_strncmp(s1, s2, n);
}

/**
 * @brief Separate `str` by any char in `sep` and return NULL terminated
 * sections. Consecutive `sep` chars in `str` are treated as a single
 * separator.
 *
 * @return pointer to NULL terminated string or NULL on errors.
 */
#if 0
char *strtok_r(char *str, const char *sep, char **state)
{
	return pbrom_libc_api->p_strtok_r(str, sep, state);
}
#endif

char *strcat(char *_MLIBC_RESTRICT dest, const char *_MLIBC_RESTRICT src)
{
	return pbrom_libc_api->p_strcat(dest, src);
}

char *strncat(char *_MLIBC_RESTRICT dest, const char *_MLIBC_RESTRICT src,
	      size_t n)
{
	return pbrom_libc_api->p_strncat(dest, src, n);
}

/**
 *
 * @brief Compare two memory areas
 *
 * @return negative # if <m1> < <m2>, 0 if <m1> == <m2>, else positive #
 */
int memcmp(const void *m1, const void *m2, size_t n)
{
	return pbrom_libc_api->p_memcmp(m1, m2, n);
}

/**
 *
 * @brief Copy bytes in memory with overlapping areas
 *
 * @return pointer to destination buffer <d>
 */

void *memmove(void *d, const void *s, size_t n)
{
	return pbrom_libc_api->p_memmove(d, s, n);
}

/**
 *
 * @brief Copy bytes in memory
 *
 * @return pointer to start of destination buffer
 */

void *memcpy(void *_MLIBC_RESTRICT d, const void *_MLIBC_RESTRICT s, size_t n)
{
	return pbrom_libc_api->p_memcpy(d, s, n);
}

/**
 *
 * @brief Set bytes in memory
 *
 * @return pointer to start of buffer
 */

void *memset(void *buf, int c, size_t n)
{
	return pbrom_libc_api->p_memset(buf, c, n);
}

/**
 *
 * @brief Scan byte in memory
 *
 * @return pointer to start of found byte
 */

void *memchr(const void *s, int c, size_t n)
{
	return pbrom_libc_api->p_memchr(s, c, n);
}
