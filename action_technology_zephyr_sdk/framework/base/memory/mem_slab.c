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
#include <mem_manager.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>

struct k_mem_slab mem_slab[CONFIG_SLAB_TOTAL_NUM];
uint8_t system_max_used[CONFIG_SLAB_TOTAL_NUM];
uint16_t system_max_size[CONFIG_SLAB_TOTAL_NUM];

#ifdef CONFIG_USED_DYNAMIC_SLAB
sys_slist_t dynamic_slab_list[CONFIG_SLAB_TOTAL_NUM];
#endif

#define CONFIG_SLAB_TOTAL_NUM 9

#define SLAB_TOTAL_SIZE (CONFIG_SLAB0_BLOCK_SIZE * CONFIG_SLAB0_NUM_BLOCKS \
				+ CONFIG_SLAB1_BLOCK_SIZE * CONFIG_SLAB1_NUM_BLOCKS \
				+ CONFIG_SLAB2_BLOCK_SIZE * CONFIG_SLAB2_NUM_BLOCKS \
				+ CONFIG_SLAB3_BLOCK_SIZE * CONFIG_SLAB3_NUM_BLOCKS \
				+ CONFIG_SLAB4_BLOCK_SIZE * CONFIG_SLAB4_NUM_BLOCKS \
				+ CONFIG_SLAB5_BLOCK_SIZE * CONFIG_SLAB5_NUM_BLOCKS \
				+ CONFIG_SLAB6_BLOCK_SIZE * CONFIG_SLAB6_NUM_BLOCKS \
				+ CONFIG_SLAB7_BLOCK_SIZE * CONFIG_SLAB7_NUM_BLOCKS \
				+ CONFIG_SLAB8_BLOCK_SIZE * CONFIG_SLAB8_NUM_BLOCKS )

char __aligned(4) mem_slab_buffer[SLAB_TOTAL_SIZE];


#define SLAB0_BLOCK_OFF 0

#define SLAB1_BLOCK_OFF \
		SLAB0_BLOCK_OFF + \
		CONFIG_SLAB0_BLOCK_SIZE * CONFIG_SLAB0_NUM_BLOCKS

#define SLAB2_BLOCK_OFF \
		SLAB1_BLOCK_OFF + \
		CONFIG_SLAB1_BLOCK_SIZE * CONFIG_SLAB1_NUM_BLOCKS

#define SLAB3_BLOCK_OFF \
		SLAB2_BLOCK_OFF + \
		CONFIG_SLAB2_BLOCK_SIZE * CONFIG_SLAB2_NUM_BLOCKS

#define SLAB4_BLOCK_OFF \
		SLAB3_BLOCK_OFF + \
		CONFIG_SLAB3_BLOCK_SIZE * CONFIG_SLAB3_NUM_BLOCKS

#define SLAB5_BLOCK_OFF \
		SLAB4_BLOCK_OFF + \
		CONFIG_SLAB4_BLOCK_SIZE * CONFIG_SLAB4_NUM_BLOCKS

#define SLAB6_BLOCK_OFF \
		SLAB5_BLOCK_OFF + \
		CONFIG_SLAB5_BLOCK_SIZE * CONFIG_SLAB5_NUM_BLOCKS

#define SLAB7_BLOCK_OFF \
		SLAB6_BLOCK_OFF + \
		CONFIG_SLAB6_BLOCK_SIZE * CONFIG_SLAB6_NUM_BLOCKS

#define SLAB8_BLOCK_OFF \
		SLAB7_BLOCK_OFF + \
		CONFIG_SLAB7_BLOCK_SIZE * CONFIG_SLAB7_NUM_BLOCKS

const struct  slabs_info sys_slab =
{
	.slab_num = CONFIG_SLAB_TOTAL_NUM,
	.max_used = system_max_used,
	.max_size = system_max_size,
	.slab_flag = SYSTEM_MEM_SLAB,
	.slabs = {
			 {
				.slab = &mem_slab[0],
				.slab_base = &mem_slab_buffer[SLAB0_BLOCK_OFF],
			    .block_num = CONFIG_SLAB0_NUM_BLOCKS,
				.block_size = CONFIG_SLAB0_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[0],
			#endif
			 },

			 {
				.slab = &mem_slab[1],
				.slab_base = &mem_slab_buffer[SLAB1_BLOCK_OFF],
			    .block_num = CONFIG_SLAB1_NUM_BLOCKS,
				.block_size = CONFIG_SLAB1_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[1],
			#endif
			 },

			 {
				.slab = &mem_slab[2],
				.slab_base = &mem_slab_buffer[SLAB2_BLOCK_OFF],
			    .block_num = CONFIG_SLAB2_NUM_BLOCKS,
				.block_size = CONFIG_SLAB2_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[2],
			#endif
			 },

			 {
				.slab = &mem_slab[3],
				.slab_base = &mem_slab_buffer[SLAB3_BLOCK_OFF],
			    .block_num = CONFIG_SLAB3_NUM_BLOCKS,
				.block_size = CONFIG_SLAB3_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[3],
			#endif
			 },

			 {
				.slab = &mem_slab[4],
				.slab_base = &mem_slab_buffer[SLAB4_BLOCK_OFF],
			    .block_num = CONFIG_SLAB4_NUM_BLOCKS,
				.block_size = CONFIG_SLAB4_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[4],
			#endif
			 },

			 {
				.slab = &mem_slab[5],
				.slab_base = &mem_slab_buffer[SLAB5_BLOCK_OFF],
			    .block_num = CONFIG_SLAB5_NUM_BLOCKS,
				.block_size = CONFIG_SLAB5_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[5],
			#endif
			 },

			 {
				.slab = &mem_slab[6],
				.slab_base = &mem_slab_buffer[SLAB6_BLOCK_OFF],
			    .block_num = CONFIG_SLAB6_NUM_BLOCKS,
				.block_size = CONFIG_SLAB6_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[6],
			#endif
			 },

			 {
				.slab = &mem_slab[7],
				.slab_base = &mem_slab_buffer[SLAB7_BLOCK_OFF],
			    .block_num = CONFIG_SLAB7_NUM_BLOCKS,
				.block_size = CONFIG_SLAB7_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[7],
			#endif
			 },

			 {
				.slab = &mem_slab[8],
				.slab_base = &mem_slab_buffer[SLAB8_BLOCK_OFF],
			    .block_num = CONFIG_SLAB8_NUM_BLOCKS,
				.block_size = CONFIG_SLAB8_BLOCK_SIZE,
			#ifdef CONFIG_USED_DYNAMIC_SLAB
				.dynamic_slab_list = &dynamic_slab_list[8],
			#endif
			 }
			}
};

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
			memset(block_ptr, 0, slabs->slabs[slab_index].block_size);
			if (slabs->max_used[slab_index] <
				k_mem_slab_num_used_get(slabs->slabs[slab_index].slab))	{
				slabs->max_used[slab_index] =
				k_mem_slab_num_used_get(slabs->slabs[slab_index].slab);
			}
		} else {
//		    SYS_LOG_ERR("Memory allocation time-out"
//				" free_list %p next freelist %p ",
//				slabs->slabs[slab_index].slab->free_list,
//				*(char **)(slabs->slabs[slab_index].slab->free_list));
		#if DEBUG
			dump_stack();
			mem_dump(slabs,slab_index);
		#endif
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
		memset(ptr,
				0, slabs->slabs[slab_index].block_size);

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

#ifdef CONFIG_USED_DYNAMIC_SLAB
static void * malloc_from_dynamic_slab(struct slabs_info *slabs, int slab_index)
{
	sys_snode_t *node, *tmp;
	void * alloc_block = NULL;
	sys_slist_t	* dynamic_list =
			slabs->slabs[slab_index].dynamic_slab_list;

	SYS_SLIST_FOR_EACH_NODE_SAFE(dynamic_list, node, tmp) {

		struct dynamic_slab_info * slab = DYNAMIC_SLAB_INFO(node);

		if (((uint32_t)slab->base_addr & ALL_SLAB_BUSY) != ALL_SLAB_BUSY) {
			if (((uint32_t)slab->base_addr & SLAB0_BUSY) != SLAB0_BUSY) {
				alloc_block = slab->base_addr;
				slab->base_addr =
					(void *)((uint32_t)slab->base_addr | SLAB0_BUSY);
				#ifdef DEBUG
				SYS_LOG_DBG("mem_malloc from dynamic slab %d  slab 0 : ptr %p ",
						slab_index,
						(void *)((uint32_t)alloc_block & (~ALL_SLAB_BUSY)));
				#endif
				break;
			} else if(((uint32_t)slab->base_addr & SLAB1_BUSY) != SLAB1_BUSY) {
				alloc_block = slab->base_addr
							+ slabs->slabs[slab_index].block_size;
				slab->base_addr =
					(void *)((uint32_t)slab->base_addr | SLAB1_BUSY);
			#ifdef DEBUG
				SYS_LOG_DBG("mem_malloc from dynamic slab %d  slab 1 : ptr %p ",
						slab_index,
						(void *)((uint32_t)alloc_block & (~ALL_SLAB_BUSY)));
			#endif
				break;
			}
		}
	}

	return  (void *)((uint32_t)alloc_block & (~ALL_SLAB_BUSY));
}

static bool free_to_dynamic_slab(struct slabs_info *slabs, void * ptr)
{
	bool found = false;
	bool free_to_parent = false;
	int slab_index = 0;
	struct dynamic_slab_info * slab = NULL;
	sys_slist_t	* dynamic_list = NULL;
	void * base_addr = NULL;
	uint16_t block_size = 0;

found_begin:
	for (slab_index = 0 ; slab_index < slabs->slab_num; slab_index++) {
		sys_snode_t *node, *tmp;
		dynamic_list =
				 slabs->slabs[slab_index].dynamic_slab_list;

		SYS_SLIST_FOR_EACH_NODE_SAFE(dynamic_list, node, tmp) {
			slab = DYNAMIC_SLAB_INFO(node);
			base_addr = (void *)((uint32_t)slab->base_addr
								& (~ALL_SLAB_BUSY));
			block_size = slabs->slabs[slab_index].block_size * 2;

			if (base_addr <= ptr
				&& (base_addr +  block_size) > ptr) {
				found = true;
				goto found_end;
			} else {
				slab = NULL;
			}
		}
	}

found_end:
	if (slab == NULL) {
		if (!free_to_parent) {
			return false;
		} else {
			/*target to stable slab ,mem_free to parent*/
		#ifdef DEBUG
			SYS_LOG_DBG("target to stable slab ,mem_free to parent %d ,ptr %p ",
					slab_index, ptr);
		#endif
			if (!free_to_stable_slab(slabs, ptr)) {
				SYS_LOG_ERR("mem_free to stable slab"
								": ptr %p failed ",ptr);
			}
			return true;
		}
	}

	/*mem_free to slab 0 , remove busy flag for slab0*/
	if(base_addr == ptr
		&& ((uint32_t)slab->base_addr & SLAB0_BUSY) == SLAB0_BUSY)
	{
		slab->base_addr =
			(void *)((uint32_t)slab->base_addr & (~SLAB0_BUSY));
	#ifdef DEBUG
		SYS_LOG_DBG("mem_free to dynamic slab %d slab 0: ptr %p ",slab_index,ptr);
	#endif
	}

	/*mem_free to slab 1 , remove busy flag for slab1*/
	if(base_addr == (ptr + block_size / 2)
		&& ((uint32_t)slab->base_addr & SLAB1_BUSY) == SLAB1_BUSY)
	{
		slab->base_addr =
			(void *)((uint32_t)slab->base_addr & (~SLAB1_BUSY));
	#ifdef DEBUG
		SYS_LOG_DBG("mem_free to dynamic slab %d slab 1: ptr %p ",slab_index,ptr);
	#endif
	}

	/*
	 * if all block mem_free , we need remove this node
	 * ,and mem_free mem to Superior slab
	 */
	if (((uint32_t)slab->base_addr & ALL_SLAB_BUSY) == 0) {
		free_to_parent = true;
		ptr = slab->base_addr;
		sys_slist_find_and_remove(dynamic_list,(sys_snode_t *)slab);

		if (!free_to_stable_slab(slabs, slab)) {
			SYS_LOG_ERR("mem_free to stable slab "
							": ptr %p failed ",ptr);
		}
	#ifdef DEBUG
		SYS_LOG_DBG("all block mem_free,we need remove this node"
					" %d slab all : ptr %p ",slab_index,ptr);
	#endif
		goto found_begin;
	} else {
		return true;
	}
}

static bool generate_dynamic_slab(struct slabs_info *slabs,
					uint16_t parent_slab_index, uint16_t son_slab_index)
{
	struct dynamic_slab_info * new_slab = NULL;
	void * base_addr = NULL;
	uint16_t iteration_index = parent_slab_index;

#ifdef DEBUG
	SYS_LOG_DBG("slab %d is small , get memory form slab %d ",
				son_slab_index, parent_slab_index);
#endif

	base_addr = malloc_from_stable_slab(slabs, parent_slab_index);
	if (base_addr == NULL) {
		SYS_LOG_ERR("generate dynamic slab failed  because mem_malloc"
					" from parent slab %d failed",parent_slab_index);
		return false;
	}

begin_new_daynamic_slab:

	iteration_index --;

	new_slab = (struct dynamic_slab_info *)malloc_from_stable_slab(slabs, 0);

	if (new_slab != NULL) {
		new_slab->base_addr = base_addr;
	} else {
		SYS_LOG_ERR("slab 0 is small, can't mem_malloc dynamic_slab_node");
		goto err_end;
	}

#ifdef DEBUG
	SYS_LOG_DBG("new daynamic slab add to slab %d base_addr %p",
					iteration_index, base_addr);
#endif

	sys_slist_append(slabs->slabs[iteration_index].dynamic_slab_list,
					(sys_snode_t *)new_slab);

	if (iteration_index > son_slab_index) {
		base_addr = malloc_from_dynamic_slab(slabs, iteration_index);
		if (base_addr != NULL) {
			memset(base_addr, 0, slabs->slabs[iteration_index].block_size);
		}
		goto begin_new_daynamic_slab;
	}
	return  true;

err_end:

	if ((iteration_index + 1) == parent_slab_index) {
		free_to_stable_slab(slabs, base_addr);
	} else {
		free_to_dynamic_slab(slabs, base_addr);
	}
	return  false;
}

static int find_from_dynamic_slab(struct slabs_info *slabs, int slab_index)
{
	sys_snode_t *node, *tmp;
	struct dynamic_slab_info * slab = NULL;
	sys_slist_t	* dynamic_list = NULL;

	if (slab_index >= 0 && slab_index < slabs->slab_num) {

		dynamic_list = slabs->slabs[slab_index].dynamic_slab_list;

		SYS_SLIST_FOR_EACH_NODE_SAFE(dynamic_list, node, tmp) {
			slab = DYNAMIC_SLAB_INFO(node);
			if (((uint32_t)slab->base_addr & ALL_SLAB_BUSY) != ALL_SLAB_BUSY) {
				return slab_index;
			}
		}
	}
	return slabs->slab_num;
}
#endif

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

#ifdef CONFIG_USED_DYNAMIC_SLAB
	if(first_fit_slab >= 0
		&& target_slab_index < slabs->slab_num
		&& first_fit_slab != target_slab_index)
	{
#ifdef DEBUG
		SYS_LOG_DBG("first_fit_slab %d ,target_slab_index %d ",
							first_fit_slab, target_slab_index);
#endif
		if(slabs->slabs[target_slab_index].block_size
			<= slabs->slabs[first_fit_slab].block_size * 2)	{
			return target_slab_index;
		}
		if (find_from_dynamic_slab(slabs, first_fit_slab) != first_fit_slab)	{
			if(!generate_dynamic_slab(slabs,
						target_slab_index,first_fit_slab)) {
				target_slab_index = slabs->slab_num;
			}
		}

		if(target_slab_index != slabs->slab_num) {
			target_slab_index = first_fit_slab;
		}
	}
#endif

	return target_slab_index;
}

static void dump_mem_hex(struct slabs_info *slabs, int slab_index)
{
	int length= slabs->slabs[slab_index].block_size *
					slabs->slabs[slab_index].block_num;
	unsigned char * addr = slabs->slabs[slab_index].slab_base;
	int num = 0;

	os_printk("slab id : %d base addr: %p , lenght %d \n",
			slab_index,	addr,	length);

	os_printk("free_list %p next freelist %p \n",
			slabs->slabs[slab_index].slab->free_list,
			*(char **)(slabs->slabs[slab_index].slab->free_list));

	for(int i = 0 ; i < length; i++)
	{
		if((i % 16) == 0)
		{
			os_printk("\n");
		}
		os_printk(" %2x ",addr[i]);
	}
	os_printk("\n");
	void * free_node = slabs->slabs[slab_index].slab->free_list;

	while(free_node != NULL)
	{
		os_printk("node[%d] %p \n",num++,free_node);
		free_node = *(char **)free_node;
	}

	if(k_mem_slab_num_free_get(slabs->slabs[slab_index].slab) != num)
	{
		os_printk("mem lost num %d , mem_free %d \n",
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
	if(slab_index >= slabs->slab_num)
	{
		SYS_LOG_ERR("Memory allocation failed ,block too big num_bytes %d ",
						num_bytes);
	#ifdef DEBUG
		dump_stack();
	#endif
		goto END;
	}

#ifdef CONFIG_USED_DYNAMIC_SLAB
	block_ptr = malloc_from_dynamic_slab(slabs, slab_index);
	if(block_ptr != NULL)
	{
		memset(block_ptr, 0, num_bytes);
		goto END;
	}
#endif

	block_ptr = malloc_from_stable_slab(slabs, slab_index);
	if(block_ptr != NULL)
	{
		if (slabs->max_size[slab_index] < num_bytes)
			slabs->max_size[slab_index] = num_bytes;
		goto END;
	}
END:
#ifdef DEBUG
	SYS_LOG_INF("Memory allocation num_bytes %d : block_ptr %p slab_index %d",
				num_bytes, block_ptr, slab_index);
#endif
	if(block_ptr == NULL)
	{
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

	if (ptr != NULL)
	{
#ifdef CONFIG_USED_DYNAMIC_SLAB
		if(free_to_dynamic_slab(slabs, ptr))
		{
		#ifdef DEBUG
			SYS_LOG_INF("Memory Free to dynamic slab ptr %p ",ptr);
		#endif
			goto exit;
		}
#endif
		if(!free_to_stable_slab(slabs, ptr))
		{
		#ifdef DEBUG
			dump_stack();
		#endif
			SYS_LOG_ERR("Memory Free ERR ptr %p ", ptr);
			goto exit;
		}

#ifdef DEBUG
		SYS_LOG_INF("Memory Free ptr %p ",ptr);
#endif
	}else
	{
		SYS_LOG_ERR("Memory Free ERR NULL ");
	}
exit:
	irq_unlock(key);
}

void slabs_mem_init(struct slabs_info * slabs)
{
	for(int i = 0 ; i < slabs->slab_num; i++)
	{
		k_mem_slab_init(slabs->slabs[i].slab,
							slabs->slabs[i].slab_base,
							slabs->slabs[i].block_size,
							slabs->slabs[i].block_num);
#ifdef CONFIG_USED_DYNAMIC_SLAB
		sys_slist_init(slabs->slabs[i].dynamic_slab_list);
#endif
	}
}

void mem_slabs_dump(struct slabs_info * slabs, int index)
{
	int total_used = 0;
	int total_size = 0;

	for(int i = 0 ; i < slabs->slab_num; i++)
	{
		total_used += 
			slabs->slabs[i].block_size *
			k_mem_slab_num_used_get(slabs->slabs[i].slab);
		total_size += slabs->slabs[i].block_size *
			slabs->slabs[i].block_num;
	}
	if(slabs->slab_flag == SYSTEM_MEM_SLAB)
	{
		os_printk("system total mem : %d bytes ,used mem %d bytes\n",
				total_size,
				total_used);
	}
	else
	{
		os_printk("app total mem : %d bytes ,used mem %d bytes \n",
					total_size,
					total_used );
	}
	for(int i = 0 ; i < slabs->slab_num; i++)
	{
		os_printk(
			" mem slab %d :block size %4d : used %4d , mem_free %4d,"
			" max used %4d, max size %4d\n",
		 	i ,
			slabs->slabs[i].block_size,
			k_mem_slab_num_used_get(slabs->slabs[i].slab),
			k_mem_slab_num_free_get(slabs->slabs[i].slab),
			slabs->max_used[i], slabs->max_size[i]);
	}
#ifdef CONFIG_USED_DYNAMIC_SLAB
	for(int i = 0 ; i < slabs->slab_num; i++)
	{
		sys_snode_t *node, *tmp;
		sys_slist_t	* dynamic_list = 
				 slabs->slabs[i].dynamic_slab_list;

		os_printk("slab %d dynamic slab list: \n", i);

		SYS_SLIST_FOR_EACH_NODE_SAFE(dynamic_list, node, tmp) {
			struct dynamic_slab_info * slab = DYNAMIC_SLAB_INFO(node);
			os_printk("%8x : %8x %8x \n",
					((uint32_t)slab->base_addr & (~ALL_SLAB_BUSY)),
					((uint32_t)slab->base_addr & (SLAB0_BUSY)),
					((uint32_t)slab->base_addr & (SLAB1_BUSY)));
		}
	}
#endif

	if(index >= 0 && index < slabs->slab_num)
	{
		dump_mem_hex(slabs, index);
	}
}


int mem_slab_init(void)
{
	slabs_mem_init((struct slabs_info *)&sys_slab);
#ifndef APP_USED_SYSTEM_SLAB
	slabs_mem_init((struct slabs_info *)&app_slab);
#endif
	return 0;
}
