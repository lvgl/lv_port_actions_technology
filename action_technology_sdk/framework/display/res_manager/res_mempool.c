#include <os_common_api.h>
#include <mem_manager.h>
#include <ui_mem.h>
#include <string.h>
#include "res_manager_api.h"
#include "res_mempool.h"

#define MAX_INNER_BITMAP_SIZE		64
#define MAX_ARRAY_INNER_SIZE		32*1024
#define MAX_ARRAY_INNER_BLOCK		32

#define MAX_PRELOAD_BITMAP_SIZE		32
#define MAX_ARRAY_PRELOAD_SIZE		16*1024
#define MAX_ARRAY_PRELOAD_BLOCK		16

#define MAX_COMPACT_BITMAP_SIZE		16
#define MAX_ARRAY_COMPACT_SIZE		8*1024
#define MAX_ARRAY_COMPACT_BLOCK		8

#define MAX_ARRAY_BUFFER_BLOCK		(MAX_ARRAY_INNER_BLOCK+MAX_ARRAY_PRELOAD_BLOCK+MAX_ARRAY_COMPACT_BLOCK)
#define MAX_ARRAY_BUFFER_SIZE		(MAX_ARRAY_INNER_SIZE+MAX_ARRAY_PRELOAD_SIZE+MAX_ARRAY_COMPACT_SIZE)

#define RES_MEM_MAX_RESOURCE_SETS			6
#define RES_DEBUG_LOAD_BMP					0
#define RES_PRELOAD_MAX_BLOCK_SIZE		100*1024

typedef struct _mem_info
{
	void* ptr;
	size_t size;
	struct _mem_info* next;
}mem_info_t;

static uint32_t res_mem_peak = 0;

static uint8_t* block_mem = NULL;
static uint32_t block_size = 0;
static uint32_t block_num = 0;
static uint32_t block_stat = 0;

#ifdef RES_MEM_PEAK_STATISTIC
static mem_info_t* mem_info_list = NULL;
static uint32_t res_mem_total = 0;


void _add_mem_info(void* ptr, size_t size)
{
	mem_info_t* prev = NULL;
	mem_info_t* listp = NULL;
	mem_info_t* item;

	//item = mem_malloc(sizeof(mem_info_t));
	item = (mem_info_t*)res_array_alloc(RES_MEM_SIMPLE_INNER, sizeof(mem_info_t));
	if(item == NULL)
	{
		SYS_LOG_ERR("no ram for res mem info\n");
		return;
	}
	item->ptr = ptr;
	item->size = size;
	item->next = NULL;

	listp = mem_info_list;
	if(mem_info_list == NULL)
	{
		mem_info_list = item;
	}
	else
	{
		while(listp != NULL)
		{
			prev = listp;
			listp = listp->next;
		}
		prev->next = item;
	}

	res_mem_total += size;
	if(res_mem_total > res_mem_peak)
	{
		res_mem_peak = res_mem_total;
	}
	//SYS_LOG_INF("testalloc alloc == ptr 0x%x, size %d, total %d, peak %d\n", ptr, size, res_mem_total, res_mem_peak);

}

void _remove_mem_info(void* ptr)
{
	mem_info_t* prev;
	mem_info_t* listp;
	mem_info_t* item;

	listp = mem_info_list;
	prev = NULL;
	while(listp != NULL)
	{
		if(listp->ptr == ptr)
		{
			break;
		}
		prev = listp;
		listp = listp->next;
	}

	if(listp == NULL)
	{
		SYS_LOG_ERR("testalloc remove mem info not found %p\n", ptr);
		return;
	}

	item = listp;


	if(prev == NULL)
	{
		mem_info_list = listp->next;
		res_mem_total -= item->size;
		//SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d, total %d, peak %d\n", item, item->ptr, item->size, res_mem_total, res_mem_peak);
		res_array_free(item);
	}
	else
	{
		prev->next = listp->next;
		res_mem_total -= item->size;
		//SYS_LOG_INF("testalloc free == item 0x%x, ptr 0x%x size %d, total %d, peak %d\n", item, item->ptr, item->size, res_mem_total, res_mem_peak);
		res_array_free(item);
	}

}
#endif

void res_mem_init(void)
{
	if(CONFIG_RES_MANAGER_BLOCK_SIZE > 0 && CONFIG_RES_MANAGER_BLOCK_NUM > 0)
	{
		block_mem = ui_mem_aligned_alloc(MEM_RES, res_mem_get_align(),
				CONFIG_RES_MANAGER_BLOCK_SIZE*CONFIG_RES_MANAGER_BLOCK_NUM, __func__);
		if(!block_mem)
		{
			return;
		}
		block_size = CONFIG_RES_MANAGER_BLOCK_SIZE;
		block_num = CONFIG_RES_MANAGER_BLOCK_NUM;
		block_stat = 0;
	}
}

void res_mem_deinit(void)
{

}

uint32_t _get_simple_size(int32_t type)
{
	switch (type) {
	case RES_MEM_SIMPLE_INNER:
		return MAX_ARRAY_INNER_SIZE;
	case RES_MEM_SIMPLE_PRELOAD:
		return MAX_ARRAY_PRELOAD_SIZE;
	case RES_MEM_SIMPLE_COMPACT:
		return MAX_ARRAY_COMPACT_SIZE;
	default:
		return 0;
	}
}

uint32_t _get_simple_bitmap_size(int32_t type)
{
	switch (type) {
	case RES_MEM_SIMPLE_INNER:
		return MAX_INNER_BITMAP_SIZE;
	case RES_MEM_SIMPLE_PRELOAD:
		return MAX_PRELOAD_BITMAP_SIZE;
	case RES_MEM_SIMPLE_COMPACT:
		return MAX_COMPACT_BITMAP_SIZE;
	default:
		return 0;
	}
}


void* res_mem_alloc_block(size_t size, const char* func)
{
	int i;
	void* ptr = NULL;
	size_t align = 4;

	if(block_num > 0 && block_size > 0 && size <= block_size)
	{
		for(i=0;i<block_num;i++)
		{
			if((block_stat & (1<<i)) == 0)
			{
				ptr = (void*)((uint32_t)block_mem + block_size*i);
				block_stat = block_stat | (1<<i);
				SYS_LOG_DBG("block alloc : %d, %p, stat 0x%x\n", i, ptr, block_stat);
				return ptr;
			}
		}
	}

	align = res_mem_get_align();
	ptr =  res_mem_aligned_alloc_debug(RES_MEM_POOL_BMP, align, size, func);
	if(ptr == NULL)
	{
		SYS_LOG_ERR("block alloc: no res block or mem available, stat %d\n", block_stat);
		res_manager_dump_info();
		return NULL;
	}

	SYS_LOG_DBG("block alloc: alloc from heap, stat 0x%x\n", block_stat);
	return ptr;
}

void res_mem_free_block(void* ptr)
{
	int off = -1;

	if(block_mem == NULL || (uint32_t)ptr < (uint32_t)block_mem || (uint32_t)ptr >= (uint32_t)(block_mem+block_size*block_num))
	{
		SYS_LOG_DBG("block free: ptr %p not in block mem, stat 0x%x \n", ptr, block_stat);
		res_mem_free(RES_MEM_POOL_BMP, ptr);
		return;
	}

	off = ((uint32_t)ptr - (uint32_t)block_mem)/block_size;
	if(off < 0)
	{
		SYS_LOG_DBG("block free: ptr %p invalid\n", ptr);
		return;		
	}

	if((block_stat & (1<<off)) != 0)
	{
		block_stat = block_stat & ~(1<<off);
		SYS_LOG_DBG("block free: %d, %p, stat 0x%x\n", off, ptr, block_stat);
	}
	else
	{
		SYS_LOG_INF("block freed: %d, %p, stat 0x%x\n", off, ptr, block_stat);
	}
}

void* res_array_alloc_debug(int32_t type, size_t size, const char *func)
{
	return ui_mem_alloc(MEM_RES, size, func);
}

uint32_t res_array_free(void* ptr)
{
	ui_mem_free(MEM_RES, ptr);

	os_strace_u32(SYS_TRACE_ID_RES_PRELOAD_FREE, (uint32_t)ptr);
	return 0;
}

void *res_mem_alloc_debug(uint32_t type, size_t size, const char *func)
{
	void* ptr;

	ptr = ui_mem_alloc(MEM_RES, size, func);

#ifdef RES_MEM_PEAK_STATISTIC
	if(ptr != NULL)
	{
		_add_mem_info(ptr, size);
	
}
#endif	
	return ptr;
}

void res_mem_free(uint32_t type, void *ptr)
{
	ui_mem_free(MEM_RES, ptr);
#ifdef RES_MEM_PEAK_STATISTIC
	_remove_mem_info(ptr);
#endif
}

void *res_mem_realloc_debug(uint32_t type, void *ptr, size_t requested_size, const char* func)
{
	return ui_mem_realloc(MEM_RES, ptr, requested_size, func);
}

void * res_mem_aligned_alloc_debug(uint8_t type, size_t align, size_t size, const void* caller)
{
	void* ptr;
	ptr =  ui_mem_aligned_alloc(MEM_RES, align, size, caller);
#ifdef RES_MEM_PEAK_STATISTIC
	if(ptr != NULL)
	{
		_add_mem_info(ptr, size);
	}
#endif	
	return ptr;
}

size_t res_mem_get_align(void)
{
#ifdef CONFIG_RES_MANAGER_ALIGN
	return CONFIG_RES_MANAGER_ALIGN;
#else
	return 4;
#endif
}

void res_mem_dump(void)
{
	ui_mem_dump(MEM_RES);
}

int res_is_auto_search_files(void)
{
#ifndef CONFIG_RES_MANAGER_DISABLE_AUTO_SEARCH_FILES
	return 1; 
#else
	return 0;
#endif
}

int res_debug_load_bitmap_is_on(void)
{
#if RES_DEBUG_LOAD_BMP == 1
	return 1;
#else
	return 0;
#endif
}

int res_mem_get_max_resource_sets(void)
{
	return RES_MEM_MAX_RESOURCE_SETS;
}

uint32_t res_mem_get_max_compact_block_size(void)
{
	return RES_PRELOAD_MAX_BLOCK_SIZE;
}

uint32_t res_mem_get_mem_peak(void)
{
	return res_mem_peak;
}
