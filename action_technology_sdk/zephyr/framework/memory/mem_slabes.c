/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief mem slab manager.
*/

#include <os_common_api.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <memory/mem_slabs.h>

static int find_slab_by_addr(struct slabs_info * slabs, void * addr)
{
	int i = 0;
	int target_slab_index = slabs->slab_num;
	for (i = 0 ; i < slabs->slab_num; i++) {
		if ((uint32_t)slabs->slabs[i].slab_base <= (uint32_t)addr
			&& (uint32_t)((uint32_t)slabs->slabs[i].slab_base +
				slabs->slabs[i].block_size *
				slabs->slabs[i].block_num) > (uint32_t)addr) {
			target_slab_index = i;
			break;
		}
	}
	return target_slab_index;
}
static void dump_mem_hex(struct slabs_info *slabs, int slab_index);
static void* malloc_from_stable_slab(struct slabs_info * slabs,int slab_index)
{
	void * block_ptr = NULL;
	if (slab_index >= 0 && slab_index < slabs->slab_num)	{
		if(!k_mem_slab_alloc(
					slabs->slabs[slab_index].slab,
					&block_ptr, K_NO_WAIT)) {
			if (slabs->max_used[slab_index] <
				k_mem_slab_num_used_get(slabs->slabs[slab_index].slab))	{
				slabs->max_used[slab_index] =
				k_mem_slab_num_used_get(slabs->slabs[slab_index].slab);
			}
		} else {
		    SYS_LOG_ERR("Memory allocation time-out"
				"slab_index %d, free_list %p next freelist %p ",
				slab_index, slabs->slabs[slab_index].slab->free_list,
				*(char **)(slabs->slabs[slab_index].slab->free_list));
		#if DEBUG
			dump_stack();
		#endif
			mem_slabs_dump(slabs, -1);
		}
	} else {
		SYS_LOG_INF("slab_index %d overflow  max is %d",
						slab_index, slabs->slab_num);
	}
#ifdef DEBUG
	SYS_LOG_DBG("mem_malloc from stable_slab %d : ptr %p",
						slab_index, block_ptr);
#endif
	return block_ptr;
}

static bool free_to_stable_slab(struct slabs_info *slabs, void * ptr)
{
	int slab_index = find_slab_by_addr(slabs, ptr);
	if (slab_index >= 0 && slab_index < slabs->slab_num) {
		k_mem_slab_free(slabs->slabs[slab_index].slab,
						&ptr);
	#ifdef DEBUG
		SYS_LOG_DBG("mem_free to stable slab %d "
						": ptr %p ",slab_index, ptr);
	#endif
		return true;
	}
	return false;
}

static int find_slab_index(struct slabs_info *slabs, unsigned int num_bytes)
{
	uint8_t i = 0;
	uint8_t first_fit_slab = slabs->slab_num;
	uint8_t target_slab_index = slabs->slab_num;

	uint8_t flag=1;

	for(i = 0; i < slabs->slab_num; i++) {
		if (slabs->slabs[i].block_size >= num_bytes) {
			target_slab_index = i;
			if (first_fit_slab == slabs->slab_num) {
				first_fit_slab = i;
			}
			if(k_mem_slab_num_free_get(
					slabs->slabs[target_slab_index].slab) != 0) {
				break;
			} else {
				if (flag) {
					SYS_LOG_DBG("%d ne",slabs->slabs[i].block_size);
					flag = 0;
				}
			}
		}
	}

	return target_slab_index;
}

static void dump_mem_hex(struct slabs_info *slabs, int slab_index)
{
	int length= slabs->slabs[slab_index].block_size *
					slabs->slabs[slab_index].block_num;
	unsigned char * addr = slabs->slabs[slab_index].slab_base;
	int num = 0;

	printk("slab id : %d base addr: %p , lenght %d \n",
			slab_index,	addr,	length);

	printk ("free_list %p next freelist %p \n",
			slabs->slabs[slab_index].slab->free_list,
			*(char **)(slabs->slabs[slab_index].slab->free_list));

	for (int i = 0 ; i < length; i++) {
		if ((i % 16) == 0) {
			printk("\n");
		}
		printk(" %2x ",addr[i]);
	}
	printk("\n");
	void * free_node = slabs->slabs[slab_index].slab->free_list;

	while (free_node != NULL) {
		printk("node[%d] %p \n",num++,free_node);
		free_node = *(char **)free_node;
	}

	if (k_mem_slab_num_free_get(slabs->slabs[slab_index].slab) != num) {
		printk("mem lost num %d , mem_free %d \n",
				num,
				k_mem_slab_num_free_get(slabs->slabs[slab_index].slab));
	}

}

void * mem_slabs_malloc(struct slabs_info * slabs, unsigned int num_bytes)
{
	void * block_ptr = NULL;
	unsigned int key = irq_lock();
	int slab_index = find_slab_index(slabs, num_bytes);

#ifdef DEBUG
	SYS_LOG_DBG("Memory mem_malloc  num_bytes %d bytes begin",num_bytes);
#endif
	if (slab_index >= slabs->slab_num) {
		SYS_LOG_ERR("Memory allocation failed ,block too big num_bytes %d ",
						num_bytes);
	#ifdef DEBUG
		dump_stack();
	#endif
		goto END;
	}

	block_ptr = malloc_from_stable_slab(slabs, slab_index);
	if (block_ptr != NULL) {
		if (slabs->max_size[slab_index] < num_bytes)
			slabs->max_size[slab_index] = num_bytes;
		goto END;
	}
END:
#ifdef DEBUG
	SYS_LOG_INF("Memory allocation num_bytes %d : block_ptr %p slab_index %d",
				num_bytes, block_ptr, slab_index);
#endif
	if (block_ptr == NULL) {
//		dump_stack();
		SYS_LOG_ERR("Memory allocation failed , num_bytes %d ", num_bytes);
	}
	irq_unlock(key);
	return block_ptr;
}

void mem_slabs_free(struct slabs_info * slabs, void *ptr)
{
	unsigned int key = irq_lock();

#ifdef DEBUG
	SYS_LOG_DBG("Memory Free  ptr %p begin",ptr);
#endif
	if (ptr != NULL) {
		if (!free_to_stable_slab(slabs, ptr)) {
		#ifdef DEBUG
			dump_stack();
		#endif
			SYS_LOG_ERR("Memory Free ERR ptr %p ", ptr);
			goto exit;
		}

#ifdef DEBUG
		SYS_LOG_INF("Memory Free ptr %p ",ptr);
#endif
	}

exit:
	irq_unlock(key);
}

void mem_slabs_init(struct slabs_info * slabs)
{
	for(int i = 0 ; i < slabs->slab_num; i++) {
		k_mem_slab_init(slabs->slabs[i].slab,
							slabs->slabs[i].slab_base,
							slabs->slabs[i].block_size,
							slabs->slabs[i].block_num);
	}
}

void mem_slabs_dump(struct slabs_info * slabs, int index)
{
	int total_used = 0;
	int total_size = 0;

	for (int i = 0 ; i < slabs->slab_num; i++) {
		total_used +=
			slabs->slabs[i].block_size *
			k_mem_slab_num_used_get(slabs->slabs[i].slab);
		total_size += slabs->slabs[i].block_size *
			slabs->slabs[i].block_num;
	}

	printk("slabs total mem : %d bytes ,used mem %d bytes\n",
			total_size, total_used);

	for (int i = 0 ; i < slabs->slab_num; i++) {
		printk(
			" mem slab %d :block size %4u : used %4u , mem_free %4u,"
			" max used %4u, max size %6u\n",
			i, slabs->slabs[i].block_size,
			k_mem_slab_num_used_get(slabs->slabs[i].slab),
			k_mem_slab_num_free_get(slabs->slabs[i].slab),
			slabs->max_used[i], slabs->max_size[i]);
	}

	if (index >= 0 && index < slabs->slab_num) {
		dump_mem_hex(slabs, index);
	}
}
