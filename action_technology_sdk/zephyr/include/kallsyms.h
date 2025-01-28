/* 
 * from linux/include/kallsyms.h
 *
 * Rewritten and vastly simplified by Rusty Russell for in-kernel
 * module loader:
 *   Copyright 2002 Rusty Russell <rusty@rustcorp.com.au> IBM Corporation
 */

#ifndef __INCLUDE_KALLSYMS_H__
#define __INCLUDE_KALLSYMS_H__

#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KSYM_NAME_LEN 32
#define KSYM_SYMBOL_LEN (sizeof("%s+%#x/%#x [%s]") + (KSYM_NAME_LEN - 1) + \
			 2*(32*3/10) + 1)

#ifdef CONFIG_KALLSYMS

int kallsyms_lookup_size_offset(unsigned long addr,
				  unsigned long *symbolsize,
				  unsigned long *offset);

int is_ksym_addr(unsigned long addr);

/* Look up a kernel symbol and print it to the kernel messages. */
void print_symbol(const char *fmt, unsigned long address);

int sprint_backtrace(char *buffer, unsigned long address);

#else /* !CONFIG_KALLSYMS */

static inline int kallsyms_lookup_size_offset(unsigned long addr,
					      unsigned long *symbolsize,
					      unsigned long *offset)
{
	return 0;
}

static inline int is_ksym_addr(unsigned long addr)
{
	return 0;
}

/* Stupid that this does nothing, but I didn't create this mess. */
#define print_symbol(fmt, addr)

#endif /* !CONFIG_KALLSYMS */

static inline void print_ip_sym(unsigned long ip)
{
	printk("[<%p>] ", (void *) ip);
#ifdef CONFIG_KALLSYMS_NO_NAME
	printk("\n");
#else
	print_symbol("%s\n", (unsigned long) ip);
#endif
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_KALLSYMS_H__ */
