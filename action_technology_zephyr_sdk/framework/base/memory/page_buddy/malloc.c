#include "heap.h"
#ifdef CONFIG_ACTIONS_TRACE
#include <trace.h>
#endif
#include <os_common_api.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <mem_manager_version.h>

#define enableFuncSect  0

extern void trace_set_panic(void);

void malloc_err_print(int who, unsigned int data, void *caller, int result)
{
	const char *whos_name[2] = {"malloc", "malloc_anon"};

#ifdef CONFIG_ACTIONS_TRACE
    trace_set_panic();
#endif

	mem_buddy_dump_info(DUMP_DETAIL_TYPE, NULL);
	os_printk("%s 0x%x failed, caller 0x%p error ret %d\n", whos_name[who], data, caller, result);
	//panic("buddy malloc error");
}

void * mem_buddy_malloc(int size, int need_size, struct mem_info *mem_info, void *caller)
{
#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS flags;
#endif
	int i, real, result;
	uint8_t page_no, new_buddy;
	void *page, *addr;
	struct buddy_debug_info buddy_debug;

	if(size == 0 || mem_info == NULL)
		return NULL;

	real = size;
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_lock(&flags);
#endif

	if(size > get_buddy_max())
	{
		int page_num = SIZE2PAGE(size);

		result = (mem_info->buddys[BUDDYS_SIZE - 2] != (uint8_t)-1);
		if(result){
			result = 1;
			goto err_ret;
        }

		page = pmalloc(size, caller);
		if(page == NULL){
			result = 2;
			goto err_ret;
	    }

		page_no = pagepool_convert_addr_to_pageindex(page);

		result = (mem_info->buddys[BUDDYS_SIZE - 2] != (uint8_t)-1);
		if(result)
		{
			pfree(page, page_num, caller);
			result = 3;
			goto err_ret;

		}
		for(i = 0; i < BUDDYS_SIZE; i++)
		{
			if(mem_info->buddys[i] == (uint8_t)-1)
				break;
		}
		mem_info->buddys[i] = page_no | 0x80;
//		if(page_num > 1)
			mem_info->buddys[i + 1] = page_num | 0xc0;

		addr = page;
		real = PAGE2SIZE(page_num);
		goto success;
	}

	for(i = 0; i < BUDDYS_SIZE; i++)
	{
		if(mem_info->buddys[i] == (uint8_t)-1)
			break;
		if(mem_info->buddys[i] & 0x80)
			continue;

		addr = buddy_alloc(mem_info->buddys[i], &real);
		if(addr != NULL)
		{
			goto success;
		}
	}

	result = (mem_info->buddys[BUDDYS_SIZE - 1] != (uint8_t)-1);
	if(result){
        result = 4;
        goto err_ret;
    }

	page = pmalloc(PAGE_SIZE, caller);
	if(page == NULL){
        result = 5;
        goto err_ret;
    }
	page_no = pagepool_convert_addr_to_pageindex(page);
	new_buddy = rom_new_buddy_no(page_no, &g_rom_buddy_data);
	addr = buddy_alloc(new_buddy, &real);

	result = (mem_info->buddys[BUDDYS_SIZE - 1] != (uint8_t)-1);
	if(result)
	{
		pfree(page, 1, caller);
        result = 6;
        goto err_ret;

	}
	for(i = 0; i < BUDDYS_SIZE; i++)
	{
		if(mem_info->buddys[i] == (uint8_t)-1)
			break;
	}
	mem_info->buddys[i] = new_buddy;

success:
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#endif
    size = ((size + 3) / 4) * 4;
	buddy_debug.caller = PTR_DEFLATE(caller);
	if(os_is_in_isr()){
        buddy_debug.prio = INVALID_THREAD_PRIO;
	}else{
        buddy_debug.prio = k_thread_priority_get(k_current_get());
	}
	buddy_debug.size = size;
	buddy_debug.size_mask = (~((unsigned short)size + (unsigned short)0x1234));

	memcpy((void **)((char *)addr + real - 8), &buddy_debug, sizeof(buddy_debug));

    memset(addr, 0, need_size);

#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_lock(&flags);
#endif

	mem_info->alloc_size += real;
	mem_info->original_size += size;
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#endif
#ifdef CONFIG_ACTIONS_TRACE
    TRACE_MALLOC(addr, size, caller);
#endif
	return addr;

err_ret:
	malloc_err_print(1, size, caller, result);
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#endif
    return NULL;
}

/**
 *
 * @brief Return the libmem manager version of the present build
 *
 * The libmem manager version is a four-byte value, whose format is described in the
 * file "mem_manager_version.h".
 *
 * @return mem manager version
 */
uint32_t libmemmgr_version_get(void)
{
    return LIBMEMMGR_VERSION_NUMBER;
}
