#ifndef __ALLOC_INNER_H
#define __ALLOC_INNER_H

#define BUDDYS_SIZE (sizeof(((struct mem_info *)0)->buddys) / sizeof(((struct mem_info *)0)->buddys[0]))

#if 0

#include <kernel_structs.h>

#define BUDDYS_SIZE (sizeof(((struct mem_info *)0)->buddys) / sizeof(((struct mem_info *)0)->buddys[0]))

extern k_tid_t const _main_thread;

static  inline struct mem_info *current_mem_info(void)
{
    return _current->arch.p_memory;
}

static  inline struct mem_info *get_mem_info(k_tid_t tid)
{
    return tid->arch.p_memory;
}

static  inline int is_father_thread(k_tid_t tid)
{
    return (tid->arch.p_memory == &tid->arch.memory);
}

#endif

#endif
