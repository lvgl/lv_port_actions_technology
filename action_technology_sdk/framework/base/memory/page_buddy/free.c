#include "heap.h"

#ifdef CONFIG_ACTIONS_TRACE
#include <trace.h>
#endif

//#define CHECK_FREE_TASK_PRIO

typedef union{
    unsigned short info[2];
    unsigned long int_info;
}mem_node_info_t;

extern void alloc_err_print(char *who, unsigned int data, void *caller);
extern void check_mem_debug(void *where, struct mem_info *mem_info, void *caller, \
    struct buddy_debug_info *buddy_debug, uint32_t size);

static void del_buddy(struct mem_info *mem_info, int index, int nr)
{
	for(; index < BUDDYS_SIZE - nr; index++)
		mem_info->buddys[index] = mem_info->buddys[index+nr];
	for(nr--; nr >= 0; nr--)
		mem_info->buddys[index+nr] = (uint8_t)-1;
}


static void free_panic(void *where, void *caller, int err_no)
{
	os_printk("BUG:task ");
	os_printk(" free %p from ", where);
	os_printk(" errno %d\n", err_no);
	//panic("buddy free error");
}

void mem_buddy_free(void *where, struct mem_info *mem_info, void *caller)
{
#ifdef CONFIG_SYS_IRQ_LOCK
	SYS_IRQ_FLAGS flags;
#endif
	int i, size, page_num = -1;
	uint8_t buddy_no;
	void *page;
	//mem_node_info_t node_info;
    struct buddy_debug_info buddy_debug;

	//printk("-- free from %x caller %x ptr %x\n", mem_info, caller, where);
#ifdef CONFIG_ACTIONS_TRACE
	TRACE_FREE(where, caller);
#endif

	if(where == NULL || mem_info == NULL)
		free_panic(where, caller, -EINVAL);

	buddy_no = pagepool_convert_addr_to_pageindex(where);
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_lock(&flags);
#endif
	for(i = 0; i < BUDDYS_SIZE; i++)
	{
		if (mem_info->buddys[i] == (uint8_t) -1)
			break;

		if((mem_info->buddys[i] & 0x80) == 0)
		{
			if(mem_info->buddys[i] == buddy_no)
			{
				page_num = 0;
				break;
			}

			continue;
		}

		if((mem_info->buddys[i] & 0x7f) == buddy_no)
		{
			if (i + 1 < BUDDYS_SIZE && mem_info->buddys[i + 1] != (uint8_t) -1
					&& (mem_info->buddys[i + 1] & (uint8_t) 0xc0) == (uint8_t) 0xc0)
			{
				page_num = mem_info->buddys[i + 1] & (uint8_t) 0x3f;
			}
			else
			{
				page_num = 1;
			}
			break;
		}

		//if (i + 1 < BUDDYS_SIZE && mem_info->buddys[i + 1] != (uint8_t) -1
		//		&& (mem_info->buddys[i + 1] & (uint8_t) 0xc0) == (uint8_t) 0xc0)
		{
			i++;
		}
	}

	if(i == BUDDYS_SIZE || page_num == -1)
		free_panic(where, caller, -EACCES);

	if(page_num == 0)
	{
		size = buddy_free(buddy_no, where, NULL);
		if(size <= 0)
			free_panic(where, caller, -EIO);

		if(rom_is_buddy_idled(buddy_no, &g_rom_buddy_data))
		{
			for(i = 0; i < BUDDYS_SIZE; i++)
			{
				if(mem_info->buddys[i] == buddy_no)
					break;
			}
			del_buddy(mem_info, i, 1);
			page_num = 1;
		}

		mem_info->alloc_size -= size;

		memcpy(&buddy_debug, (void *)((char *)where + size - 8), sizeof(buddy_debug));

		check_mem_debug(where, mem_info, caller, &buddy_debug, size);
	}
	else
	{
		if((unsigned int)where & (PAGE_SIZE - 1))
			free_panic(where, caller, -EPERM);

		for(i = 0; i < BUDDYS_SIZE; i++)
		{
			if(mem_info->buddys[i] == (buddy_no | 0x80))
				break;
		}
		del_buddy(mem_info, i, 2/*1+(page_num>1)*/);
		size = PAGE_SIZE * page_num;

		mem_info->alloc_size -= size;
		memcpy(&buddy_debug, (void *)((char *)where + size - 8), sizeof(buddy_debug));

		check_mem_debug(where, mem_info, caller, &buddy_debug, size);
	}

	if(page_num > 0)
	{
	    int ret_val;

		page = pagepool_convert_index_to_addr(buddy_no);

		//printk("buddy_no %d page %x freelist next %x prev %x\n", buddy_no, page, freelist.next_index, freelist.prev_index);

		ret_val = pfree(page, page_num, caller);
		if(ret_val < 0){
			free_panic(where, caller, -EBUSY);
	    }

	    //printk("pagenum %d free pagenum %d \n", page_num, freepage_num[0]);
	}
#ifdef CONFIG_SYS_IRQ_LOCK
	sys_irq_unlock(&flags);
#endif
}

#if 0
void free(void *where)
{
	if(_is_in_isr())
	{
		free_panic(where, __builtin_return_address(0), -EPERM);
		return;
	}
	free_inner(where, current_mem_info(), __builtin_return_address(0));
}

void free_anon(void *where, k_tid_t tid)
{
	if(tid == NULL)
		tid = _main_thread;

	free_inner(where, get_mem_info(tid), __builtin_return_address(0));
}
#endif


