/**
 * @file random.h
 *
 * @brief Public APIs for the random driver.
 */

/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MEM_BUDDY_H__
#define __MEM_BUDDY_H__

#include "page_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_THREAD_PRIO     (-16)

typedef enum mem_dump_type {
	DUMP_DEFAULT_TYPE,
	DUMP_DETAIL_TYPE,
	DUMP_BY_TAG_TYPE,
	DUMP_BY_THREAD_TYPE,
} mem_dump_type_e;

struct buddy_debug_info{
    uint16_t size;
    uint16_t size_mask;
    int32_t caller :26;
    int32_t prio : 6;
};

struct mem_info {
       unsigned int alloc_size;
       unsigned int original_size;
       unsigned char buddys[RAM_MPOOL0_MAX_NUM * 2];
};
extern void * mem_buddy_malloc(int size, int need_size, struct mem_info *mem_info, void *caller);
extern void mem_buddy_free(void *where, struct mem_info *mem_info, void *caller);
extern void mem_buddy_dump_info(u32_t dump_detail, const char *match_str);
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __MEM_BUDDY_H__ */
