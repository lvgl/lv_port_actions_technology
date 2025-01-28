#include "heap.h"

struct page pagepool0[RAM_MPOOL0_MAX_NUM];
struct page *pagepool[1] = {pagepool0,};
const int pagepool_size[1] = {POOL0_NUM,};
unsigned char freepage_num[1];

struct list_index freelist = { (unsigned char)FREE_INDEX_FREE_FLAG, (unsigned char)FREE_INDEX_FREE_FLAG };

void *pagepool_convert_index_to_addr(unsigned char index)
{
	void *start_addr = POOL0_ADDR;

#if 0
	if(INDEX_ZONE(index) != zone_ram_cache)
	{
#if 1
		os_printk("%s: invalid index %d\n", __func__, index);
#else	
		if(index >= POOL1_DSP_NUM)
		{
			start_addr = HW_RAM_ADDR;
			index -= POOL1_DSP_NUM;
		}
		else
		{
			start_addr = DSP_ADDR;
		}

		index = INDEX2PAGE(index);	
#endif
	}
#endif /* #if 0 */

	return ((char *)start_addr + index * PAGE_SIZE);
}

static void __freelist_add(unsigned char new_index, unsigned char prev_index, unsigned char next_index)
{
//    os_printk("new index %d prev %d next %d\n",new_index, prev_index, next_index);

	NODE(next_index)->prev_index = new_index;
	NODE(new_index)->next_index = next_index;
	NODE(new_index)->prev_index = prev_index;
	NODE(prev_index)->next_index = new_index;
}

static void __freelist_del(unsigned char prev_index, unsigned char next_index)
{
	NODE(next_index)->prev_index = prev_index;
	NODE(prev_index)->next_index = next_index;
}

void pagepool_freelist_add(unsigned char new_index)
{
//    os_printk("++ page index add %d\n",new_index);

	__freelist_add(new_index, (unsigned char)FREE_INDEX_FREE_FLAG, freelist.next_index);
}

//void pagepool_freelist_add_tail(unsigned char new_index)
//{
//	__freelist_add(new_index, freelist.prev_index, (unsigned char)FREE_INDEX_FLAG);
//}

void pagepool_freelist_del(unsigned char index)
{
//    os_printk("-- page index del %d\n",index);

	__freelist_del(NODE(index)->prev_index, NODE(index)->next_index);
	NODE(index)->prev_index = FREE_INDEX_USED_FLAG;
	NODE(index)->next_index = FREE_INDEX_USED_FLAG;
}

int pagepool_is_page_in_freelist(struct page *page)
{
	return PAGE_IN_FREELIST(page);
}

void * pmalloc(unsigned int size, void *caller)
{
    void *ptr;

    page_alloc_ctx_t ctx;

    int page_num = SIZE2PAGE(size);

    ctx.freelist = &freelist;
    ctx.continus_start_alloc_page_num = POOL0_NUM;

    ctx.pagepool_cache_ibank_into_freepage_cb = NULL;
    ctx.pagepool_freelist_del_cb = pagepool_freelist_del;
    ctx.pagepool_is_page_in_freelist = pagepool_is_page_in_freelist;
    ctx.pagepool_convert_index_to_addr = pagepool_convert_index_to_addr;

    ptr =  rom_pagepool_alloc_pages(page_num, zone_ram_cache, pagepool_size, freepage_num, pagepool, &ctx);

    //os_printk("%s: ptr 0x%x, size 0x%x, page_num %d, freepage_num %d %p\n",
    //	__func__, ptr, size, page_num, freepage_num[0], caller);

    return ptr;
}

struct page *pagepool_get_page_by_index(unsigned char index)
{
    return PAGE(index);
}

unsigned char pagepool_convert_addr_to_pageindex(void *addr)
{
    return ((uintptr_t)addr - (uintptr_t)POOL0_ADDR) / PAGE_SIZE;
}

int pfree(void *addr, unsigned int page_num, void *caller)
{
    page_free_ctx_t ctx;

    ctx.freelist = &freelist;
    ctx.pagepool_convert_addr_to_pageindex = pagepool_convert_addr_to_pageindex;
    ctx.pagepool_freelist_add = pagepool_freelist_add;
    ctx.pagepool_is_page_in_freelist = pagepool_is_page_in_freelist;
    ctx.pagepool_get_page_by_index = pagepool_get_page_by_index;

    //printk("%s: ptr 0x%x, page_num %d %p\n",
    //	__func__, addr, page_num, caller);

    //dump_stack();	

    return rom_pagepool_free_pages(addr, page_num, zone_ram_cache, pagepool_size, freepage_num, &ctx);
}


void pagepool_use_dump(uint32_t use_size)
{
    uint32_t total_mem = POOL0_NUM * PAGE_SIZE; 

	os_printk("mpool: %d(used)/%d(total)\n\n"
			,freepage_num[0], POOL0_NUM);   

    os_printk("total mem size %u\n", total_mem);

    os_printk("use mem size %u (%u %%)\n", use_size, \
        use_size * 100 / total_mem);
    
    os_printk("free mem size %u (%u %%)\n", total_mem - use_size, \
        (total_mem - use_size) * 100 / total_mem);
}



