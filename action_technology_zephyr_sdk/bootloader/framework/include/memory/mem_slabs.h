/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mem slabes interface 
 */

#ifndef __MEM_SLABS_H__
#define __MEM_SLABS_H__

#ifdef CONFIG_MEMORY_SLABS
struct slab_info
{
	struct k_mem_slab *slab;
	char * slab_base;
	uint16_t block_num;
	uint32_t block_size;
};

struct slabs_info
{
	uint16_t slab_num;
	uint16_t slab_flag;
	uint8_t * max_used;
	uint32_t * max_size;
	const struct slab_info *slabs;
};

void mem_slabs_init(struct slabs_info * slabs);
void mem_slabs_free(struct slabs_info * slabs, void *ptr);
void *mem_slabs_malloc(struct slabs_info * slabs, unsigned int num_bytes);
void mem_slabs_dump(struct slabs_info * slabs, int index);
#endif
#endif


