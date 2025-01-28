#include <os_common_api.h>
#include <buddy_inner.h>
#include <page_inner.h>
#include <mem_buddy.h>

extern void pagepool_use_dump(uint32_t use_size);

void dump_mem_register_callback(void (*cb)(int32_t prio, uint32_t alloc_size, uint32_t in_isr));

extern struct mem_info sys_meminfo;

#ifdef CONFIG_MEMORY_ANALYSIS

#define MAX_THREAD_NUM 16

uint32_t alloc_num[6];

uint32_t isr_alloc_size;

struct heap_info
{
    uint32_t prio:8;
    uint32_t size:24;
};


struct heap_info thread_heap_info[MAX_THREAD_NUM];

static const char alloc_type[][5] =
{
    "<=16",
    "<=32",
    "<=48",
    "<=64",
    "< 2k",
    ">=2k",
};

static void _mem_buddy_debug_info_reset(void)
{
    memset(alloc_num, 0, sizeof(alloc_num));
}

static void _mem_buddy_debug_info_dump(void)
{
    uint32_t i;
    uint32_t cur_size;
    uint32_t total_size = 0;

    for(i = 0; i < ARRAY_SIZE(alloc_num); i++){
        k_busy_wait(5000);

        if(i < (ARRAY_SIZE(alloc_num) - 1)){
            cur_size = (alloc_num[i] * ((i + 1) << 4));
            printk("alloc type(%s):\t%5u (%2u %%)\n", &alloc_type[i][0], alloc_num[i], \
                 cur_size * 100 / sys_meminfo.alloc_size);
            total_size += cur_size;
        }else{
            printk("alloc type(%s):\t%5u (%2u %%)\n", &alloc_type[i][0], alloc_num[i], \
                 (sys_meminfo.alloc_size - total_size) * 100 / sys_meminfo.alloc_size);
        }
    }
}

static void _mem_buddy_thread_heap_reset(void)
{
    uint32_t i;

    for(i = 0; i < MAX_THREAD_NUM; i++){
        thread_heap_info[i].size = 0;
        thread_heap_info[i].prio = 0xff;
    }

    isr_alloc_size = 0;

}

static void _mem_buddy_thread_heap_use(int prio, uint32_t size, uint32_t in_isr)
{
    uint32_t i;

    if(in_isr){
        isr_alloc_size += size;
        return;
    }

    for(i = 0; i < MAX_THREAD_NUM; i++){
        if(thread_heap_info[i].prio == prio){
            thread_heap_info[i].size += size;
            return;
        }
    }

    for(i = 0; i < MAX_THREAD_NUM; i++){
        if(thread_heap_info[i].prio == 0xff){
            thread_heap_info[i].prio = prio;
            thread_heap_info[i].size += size;
            return;
        }
    }

    return;
}

static int _mem_buddy_thread_heap_dump(void)
{
    uint32_t i;
    uint32_t alloc_size = 0;

    for(i = 0; i < MAX_THREAD_NUM; i++){
        if(thread_heap_info[i].prio != 0xff){
            printk("alloc task(%4d):\t%5u (%2u %%) prio (%d)\n", thread_heap_info[i].prio, thread_heap_info[i].size, \
                thread_heap_info[i].size * 100 / sys_meminfo.alloc_size, \
                thread_heap_info[i].prio);
            alloc_size += thread_heap_info[i].size;
        }
    }

    printk("alloc isr:     \t%5u (%2u %%)\n", isr_alloc_size, \
         isr_alloc_size * 100 / sys_meminfo.alloc_size);

    alloc_size += isr_alloc_size;

    printk("others   :     \t%5u (%2u %%)\n", (sys_meminfo.alloc_size - alloc_size), \
         (sys_meminfo.alloc_size - alloc_size) * 100 / sys_meminfo.alloc_size);

    return 0;
}

static void _mem_buddy_dump_callback(int32_t prio, uint32_t alloc_size, uint32_t in_isr)
{
    if(alloc_size >= 80 && alloc_size < 2048){
        alloc_size = 80;
    }

    if(alloc_size != 2048){
        alloc_num[(alloc_size >> 4) - 1]++;
    }else{
        alloc_num[5]++;
    }

    _mem_buddy_thread_heap_use(prio, alloc_size, in_isr);
}
#endif


static void _mem_buddy_dump_memory(struct mem_info *mem_info, uint32_t dump_detail, const char *match_str)
{
	int i, page_num;
	int8_t buddy_no;
	void *page;
#ifdef CONFIG_SYS_IRQ_LOCK  
	SYS_IRQ_FLAGS flags;

    if(dump_detail){
        sys_irq_lock(&flags);
    }
#endif
	for(i = 0; i < sizeof(mem_info->buddys)/sizeof(mem_info->buddys[0]); i++)
	{
		if(mem_info->buddys[i] == (uint8_t)-1)
			break;

		buddy_no = mem_info->buddys[i] & 0x7f;
		if((mem_info->buddys[i] & 0x80) == 0)
		{
			if(buddy_dump(buddy_no, dump_detail, match_str) == false)
				os_printk("BUG:buddy %d broken, maybe overflow happened!\n", buddy_no);
			continue;
		}

		if(i == 0)
			continue;

		page = pagepool_convert_index_to_addr(buddy_no);
		if(i + 1 < sizeof(mem_info->buddys)/sizeof(mem_info->buddys[0])
			&& mem_info->buddys[i + 1] != 0xff
			&& (mem_info->buddys[i + 1] & (int8_t)0xc0) == (int8_t)0xc0)
		{
			page_num = mem_info->buddys[i + 1] & (int8_t)0x3f;
			i++;
		}
		else
		{
			page_num = 1;
		}
		dump_show_info(page, page_num * PAGE_SIZE,
			(void *)((char *)page + (page_num * PAGE_SIZE) - 8),
			dump_detail,match_str);
	}
#ifdef CONFIG_SYS_IRQ_LOCK  
    if(dump_detail){
	    sys_irq_unlock(&flags);
    }
#endif
}

void mem_buddy_dump_info(uint32_t dump_detail, const char *match_str)
{
    pagepool_use_dump(sys_meminfo.alloc_size);

#ifdef CONFIG_MEMORY_ANALYSIS
    dump_mem_register_callback(_mem_buddy_dump_callback);
    _mem_buddy_debug_info_reset();
    _mem_buddy_thread_heap_reset();
#endif

    printk("\nsys memory used:\n");
    printk("\ttotal size: %d\n", sys_meminfo.alloc_size);
    printk("\toriginal size: %d\n", sys_meminfo.original_size);

    if(dump_detail){
        printk("\tmalloc detail:\n");
		printk("%4s       (%4s %4s) %12s %12s\n","addr","asize","rsize", "thread", "tag");
        _mem_buddy_dump_memory(&sys_meminfo, dump_detail, match_str);

#ifdef CONFIG_MEMORY_ANALYSIS
        _mem_buddy_debug_info_dump();
        _mem_buddy_thread_heap_dump();
#endif
    }
}
