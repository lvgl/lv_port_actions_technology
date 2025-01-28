#ifndef __STDLIB_STDARG_H__
#define __STDLIB_STDARG_H__

#ifndef _VA_LIST_DECLARED
#define	_VA_LIST_DECLARED
typedef __builtin_va_list va_list;
#endif

#define	va_start(ap, last) \
	__builtin_va_start((ap), (last))

#define	va_arg(ap, type) \
	__builtin_va_arg((ap), type)

#define	va_end(ap) \
	__builtin_va_end(ap)

#endif	/* __STDLIB_STDARG_H__ */
