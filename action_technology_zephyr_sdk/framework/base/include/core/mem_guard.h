/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem leak check.
*/

#ifndef MEM_LEAK_CHECK_H_
#define MEM_LEAK_CHECK_H_
#if defined(__linux__) || defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define MUTEX_LOCK_DECLARE
#define MUTEX_LOCK()
#define MUTEX_UNLOCK()
#define USE_UNIT_TEST

#elif defined(__ZEPHYR__)
#include <zephyr.h>
#include <init.h>
#include <sys/printk.h>
#ifdef CONFIG_UI_MANAGER
#include <view_manager.h>
#endif
#define PRINTF printk
#define MUTEX_LOCK_DECLARE unsigned int __key;
#define MUTEX_LOCK()       __key = irq_lock()
#define MUTEX_UNLOCK()     irq_unlock(__key)
#else
#error "Unknown operation system"
#endif

#include <stdint.h>
#include <stdlib.h>
#if defined(__ZEPHYR__)
#include <sys/__assert.h>
#else
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MM_DEFINE
#define __mem_guard_obj
#else
#define __mem_guard_obj extern
#endif

#define MAX_MOBJ_NR     1000
#define MAX_MOBJ_LOGN   8
#define MAX_MOBJ_HASH    (0x1ul << MAX_MOBJ_LOGN)
#define MM_HASH(v) \
    (((uintptr_t)(v) ^ ((uintptr_t)(v) >> MAX_MOBJ_LOGN)) & (MAX_MOBJ_HASH - 1))

struct mem_guard_obj {
    struct mem_guard_obj* next;
    const void* ptr;
    size_t len;
    union {
        const char* caller;
        const void* caller_addr;
    };
	int user_info;
};
struct mem_guard_head {
    int free_blocks;
    char* free_list;
    void* free_ptr;
    int max_used_size;
    int current_used_size;
    int total_size;
	int free_index;
    struct mem_guard_obj* heads[MAX_MOBJ_HASH];
    struct mem_guard_obj nodes[MAX_MOBJ_NR];
	struct mem_guard_obj free_nodes[MAX_MOBJ_NR];
};

#define MM_OBJHEAD_DECLARE(_name) \
    __mem_guard_obj struct mem_guard_head _name

#if defined(__linux__) || defined(_WIN32)
#define MM_ASSERT(n) assert(n)
#elif defined(__ZEPHYR__)
#define MM_ASSERT(n) __ASSERT_NO_MSG(n)
#else
#error "Unknown OS"
#endif

#define MEM_STAKE_SIZE (sizeof(int))
#define MEM_STAKE      0x55AA55AA
void mem_guard_init(struct mem_guard_head* head, int total_size);
int mem_guard_add(struct mem_guard_head* head, const char* name, const void* obj, size_t len);
struct mem_guard_obj* mem_guard_find(struct mem_guard_head* head, const void* obj);
int mem_guard_del_locked(struct mem_guard_head* head, const void* obj);
int mem_guard_del(struct mem_guard_head* head, const void* obj, void (*mfree)(void* obj));
void mem_guard_iterate(struct mem_guard_head* head, int (*iterator)(struct mem_guard_obj*, void*),
    void* context);

static inline void mem_guard_free(struct mem_guard_head* head,
    void (*free)(void*), void* ptr) {
	mem_guard_del(head, ptr, free);
}

static inline void* mem_guard_malloc(struct mem_guard_head* head,
    void* (*malloc)(size_t), size_t size, const char* func) {
    void* ptr = NULL;
	size = (size + 3) / 4 * 4;
	ptr = malloc(size + MEM_STAKE_SIZE);
	if(ptr) {
		*(int*)((int)ptr + size) = MEM_STAKE;
		mem_guard_add(head, (const char*)func, ptr, size + MEM_STAKE_SIZE);
	} else {
		MM_ASSERT(ptr != NULL);
	}
    return ptr;
}

static inline void* mem_guard_calloc(struct mem_guard_head* head,
    void* (*calloc)(size_t, size_t), size_t n, size_t size,
    const char* func) {
	void* ptr = NULL;
    size = (size + 3) / 4 * 4;
	ptr = calloc(n, size + MEM_STAKE_SIZE);
	if(ptr) {
		*(int*)((int)ptr + size) = MEM_STAKE;
		mem_guard_add(head, (const char*)func, ptr, size + MEM_STAKE_SIZE);
	}else {
		MM_ASSERT(ptr != NULL);
	}
    return ptr;
}

static inline void* mem_guard_realloc(struct mem_guard_head* head,
    void* (*realloc)(void*, size_t), void* old_ptr, size_t size,
    const char* func) {
	void* ptr = NULL;
    size = (size + 3) / 4 * 4;
	mem_guard_del(head, old_ptr, NULL);
    ptr = realloc(old_ptr, size + MEM_STAKE_SIZE);
	if (ptr) {
		*(int*)((int)ptr + size) = MEM_STAKE;
		mem_guard_add(head, (const char*)func, ptr, size + MEM_STAKE_SIZE);
	} else {
		MM_ASSERT(ptr != NULL);
	}
    return ptr;
}

static inline void* mem_guard_aligned_alloc(struct mem_guard_head* head,
    void* (*aligned_alloc)(size_t, size_t), size_t align, size_t size,
    const char* func) {
    void* ptr = NULL;
	size = (size + 3) / 4 * 4;
	ptr = aligned_alloc(align, size + MEM_STAKE_SIZE);
	if (ptr) {
		*(int*)((int)ptr + size) = MEM_STAKE;
		mem_guard_add(head, (const char*)func, ptr, size + MEM_STAKE_SIZE);
	}else{
		MM_ASSERT(ptr != NULL);
	}
    return ptr;
}
static inline void mem_guard_dump(struct mem_guard_head* head) {
	int total_malloc = 0;

	PRINTF("total size: %d \n",head->total_size);

	PRINTF("max used : %d bytes max used rate %d %% \n",
					head->max_used_size,
					head->max_used_size * 100 / head->total_size);
	PRINTF("current used : %d bytes current used rate %d %% \n",
					head->current_used_size,
					head->current_used_size * 100 / head->total_size);

    mem_guard_iterate(head, NULL, &total_malloc);

	PRINTF("memory total used %d \n", total_malloc);
}

void mem_guard_set_user_info(int user_info);
void mem_guard_leak_check(struct mem_guard_head* head, int user_info);
void mem_guard_check_overwrite(struct mem_guard_head* head);

#ifdef __cplusplus
}
#endif
#endif /* MEM_LEAK_CHECK_H_ */
