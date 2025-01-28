#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdarg.h>

#ifndef _ASSEMBLER_

#ifndef u8
typedef unsigned char u8;
#endif

#ifndef u16
typedef unsigned short u16;
#endif

#ifndef u32
typedef unsigned int u32;
#endif

#ifndef uchar
#define  uchar unsigned char
#endif

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef int8
typedef signed char int8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef int16
typedef signed short int16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#ifndef int32
typedef signed int int32;
#endif

#ifndef ulong32
typedef unsigned long ulong32;
#endif

#ifndef long32
typedef signed long long32;
#endif

#ifndef HWORD
#define  HWORD unsigned short
#endif

#ifndef hword
#define  hword unsigned short
#endif

/* #define  WORD unsigned int */
/* #define  WORD unsigned int */
#ifndef word
#define  word unsigned int
#endif

#ifndef bool
#define  bool int
#endif

#ifndef BOOL
#define  BOOL int
#endif

#ifndef dword
#define  dword unsigned long long
#endif

#endif

#ifndef HANDLE
#define  HANDLE unsigned int
#endif

#ifndef ulong
#define  ulong unsigned long
#endif

#ifndef INT8
typedef char INT8;
#endif

#ifndef UINT8
typedef unsigned char UINT8;
#endif

#ifndef INT16
typedef short INT16;
#endif

#ifndef UINT16
typedef unsigned short UINT16;
#endif

#ifndef INT32
typedef int INT32;
#endif

#ifndef UINT32
typedef unsigned int UINT32;
#endif

#ifndef PC
#ifndef INT64
typedef long long INT64;
#endif

#ifndef UINT64
typedef unsigned long long UINT64;
#endif

#ifndef NULL
#define  NULL ((void *)0)
#endif

#ifndef TRUE
#define  TRUE   1
#endif

#ifndef FALSE
#define  FALSE  0
#endif

#ifndef FOREVER
#define  FOREVER 1
#endif

#ifndef NEVER
#define  NEVER 0
#endif

#ifndef size_t
#define size_t unsigned int
#endif

enum {
	false = 0,
	true = 1
};

/* general data types
 */
typedef unsigned char uchar_t;
typedef unsigned short ushort_t;
typedef unsigned int uint_t;
typedef unsigned long ulong_t;

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;

typedef signed char s8_t;
typedef signed short s16_t;
typedef signed int s32_t;
typedef signed long long s64_t;

typedef u8_t byte_t;

/* volatile data types
 */
typedef volatile u8_t vu8_t;
typedef volatile u16_t vu16_t;
typedef volatile u32_t vu32_t;

/* register data types
 */
#define __reg8(_reg)   (*(vu8_t *)(_reg))
#define __reg16(_reg)  (*(vu16_t *)(_reg))
#define __reg32(_reg)  (*(vu32_t *)(_reg))

/* stdbool data types
 */
#define __defined_stdbool  1

/* stdint data types
 */
#define __defined_stdint  1


/* stddef data types
 */
typedef u16_t wchar_t;
typedef s32_t ptrdiff_t;

/* stdio data types
 */
typedef u32_t fpos_t;
typedef s32_t off_t;
typedef s32_t ssize_t;

typedef long blksize_t;

typedef long blkcnt_t;

/* time data types
 */
typedef u32_t useconds_t;

struct timespec {
	u32_t tv_sec;   /*  second */
	u32_t tv_nsec;  /*  nano second */
};

struct timeval {
	u32_t tv_sec;   /*  second */
	u32_t tv_usec;  /*  micro second */
};

/* sys data types
 */
typedef s32_t pid_t;
typedef s32_t tid_t;
typedef u32_t id_t;

typedef u32_t mode_t;
typedef u32_t key_t;

typedef int atomic_t;
typedef atomic_t atomic_val_t;

/* number of array elements
 */
#define ARRAY_SIZE(_array)  (sizeof(_array) / sizeof(_array[0]))

/*!
 * \brief parameter data types of sys_irq_lock & sys_irq_unlock
 */
typedef struct sys_irq_flags {
	unsigned int keys[2];
} SYS_IRQ_FLAGS;

#define _STRINGIFY(x) #x
#define STRINGIFY(s) _STRINGIFY(s)

#define ___in_section(a, b, c) \
	__attribute__((section("." _STRINGIFY(a)			\
				"." _STRINGIFY(b)			\
				"." _STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define __in_section_unique(seg) ___in_section(seg, __FILE__, __COUNTER__)

#define ARG_UNUSED(x) (void)(x)

#endif /* _ASSEMBLER_ */

#endif /* _TYPES_H_ */
