#ifndef COMPAT_H
#define COMPAT_H

#include <stdint.h>
#include <limits.h>
#include <sys/printk.h>
#include "freebsd-compat.h"
#include "queue.h"

#define howmany(x, y)   (((x)+((y)-1))/(y))

#define powerof2(x)  ((x & (x-1)) == 0)

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef LONG_BIT
#define LONG_BIT	32
#endif

typedef uint8_t u_char;
typedef uint32_t uint;
typedef uint32_t u_int;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

#define MAXPATHLEN	256

#define pfatal	printk
#define pwarn	printk
#define perr	printk
#define pinfo	printk

/*
 * Open file
 */
int ext_open(const char* fname);

/*
 * Close file
 */
int ext_close(int fd);

/*
 * read file by offset
 */
int ext_read(int fd, int off, void* ptr, int size);

/*
 * write file by offset
 */
int ext_write(int fd, int off, void* ptr, int size);

/*
 * malloc buffer
 */
void* ext_malloc(int size);

/*
 * calloc buffer
 */
void* ext_calloc(int nmemb, int size);

/*
 * free buffer
 */
void ext_free(void* ptr);

#endif
