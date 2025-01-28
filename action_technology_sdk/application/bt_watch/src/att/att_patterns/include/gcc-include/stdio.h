#ifndef __STDIO_H__
#define __STDIO_H__

#include <restrict.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__FILE_defined)
#define __FILE_defined
typedef int  FILE;
#endif

#if !defined(EOF)
#define EOF  (-1)
#endif

#define stdin  ((FILE *) 1)
#define stdout ((FILE *) 2)
#define stderr ((FILE *) 3)

#ifndef __printf_like
#define __printf_like(f, a)   __attribute__((format (printf, f, a)))
#endif

int __printf_like(1, 2) printf(const char *_MLIBC_RESTRICT format, ...);

__printf_like(1, 2) void printk(const char *fmt, ...);

int __printf_like(3, 4) snprintf(char *_MLIBC_RESTRICT str, size_t len,
				 const char *_MLIBC_RESTRICT format, ...);
int __printf_like(2, 3) sprintf(char *_MLIBC_RESTRICT str,
				const char *_MLIBC_RESTRICT format, ...);
int __printf_like(2, 3) fprintf(FILE * _MLIBC_RESTRICT stream,
				const char *_MLIBC_RESTRICT format, ...);


int __printf_like(1, 0) vprintf(const char *_MLIBC_RESTRICT format, va_list list);
int __printf_like(3, 0) vsnprintf(char *_MLIBC_RESTRICT str, size_t len,
				  const char *_MLIBC_RESTRICT format,
				  va_list list);
int __printf_like(2, 0) vsprintf(char *_MLIBC_RESTRICT str,
				 const char *_MLIBC_RESTRICT format, va_list list);
int __printf_like(2, 0) vfprintf(FILE * _MLIBC_RESTRICT stream,
				 const char *_MLIBC_RESTRICT format,
				 va_list ap);

int puts(const char *s);

int fputc(int c, FILE *stream);
int fputs(const char *_MLIBC_RESTRICT s, FILE *_MLIBC_RESTRICT stream);
size_t fwrite(const void *_MLIBC_RESTRICT ptr, size_t size, size_t nitems,
	      FILE *_MLIBC_RESTRICT stream);
static inline int putc(int c, FILE *stream)
{
	return fputc(c, stream);
}
static inline int putchar(int c)
{
	return putc(c, stdout);
}

void print_buffer(const void *buffer, int width, int count, int linelen, unsigned long disp_addr);

#ifdef __UVISION_VERSION
#define printf printk
#endif

#ifdef __cplusplus
}
#endif

#endif  /* __STDIO_H__ */
