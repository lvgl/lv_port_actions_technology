/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mem inner interface 
 */

#ifndef __MEM_INNER_H__
#define __MEM_INNER_H__

#ifdef CONFIG_APP_USED_MEM_POOL
void *mem_pool_malloc(unsigned int num_bytes);
void mem_pool_free(void *ptr);
void mem_pool_dump(void);
void mem_pool_init(void);
#endif

#ifdef APP_USED_MEM_SLAB
#define SLAB0_BUSY 		0x01
#define SLAB1_BUSY 		0x02
#define ALL_SLAB_BUSY 	0x03

#define SYSTEM_MEM_SLAB 0
#define APP_MEM_SLAB    1

struct dynamic_slab_info
{
	sys_snode_t node;
	void * base_addr;
	uint32_t alloc_mask;
	uint16_t block_size;
	uint16_t block_num;
};

struct slab_info
{
	struct k_mem_slab *slab;
	char * slab_base;
	uint16_t block_num;
	uint16_t block_size;
#ifdef CONFIG_USED_DYNAMIC_SLAB
	sys_slist_t	* dynamic_slab_list;
#endif
};

struct slabs_info
{
	uint16_t slab_num;
	uint16_t slab_flag;
	uint8_t * max_used;
	uint16_t * max_size;
	struct slab_info slabs[CONFIG_SLAB_TOTAL_NUM];
};

#define DYNAMIC_SLAB_INFO(_node) CONTAINER_OF(_node, struct dynamic_slab_info, node)

void mem_slabs_init(struct slabs_info * slabs);
void mem_slabs_free(struct slabs_info * slabs, void *ptr);
void *mem_slabs_malloc(struct slabs_info * slabs, unsigned int num_bytes);
void mem_slabs_dump(struct slabs_info * slabs,int index);
#endif

#ifdef CONFIG_APP_USED_MEM_PAGE
void mem_page_init(void);
void *mem_page_malloc(unsigned int num_bytes, void *caller);
void mem_page_free(void *ptr, void *caller);
void mem_page_dump(uint32_t dump_detail,const char * match_str);
#endif

#endif


