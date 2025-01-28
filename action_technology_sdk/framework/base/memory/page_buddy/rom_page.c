#include "heap.h"

void * rom_pagepool_alloc_pages(unsigned int page_num, unsigned int zone_index, \
            const int *pagepool_size,
            unsigned char *freepage_num,
	        struct page **pagepool,
	        page_alloc_ctx_t * ctx)
{
	unsigned int flags;
 	int i, size, continus_index;
	unsigned char index, recache;
	struct page *pool, *page;
    void *addr;

	if(page_num == 0)
	{
		return NULL;
	}

	flags = irq_lock();

	//check page pool free page num is enough
	if(freepage_num[zone_index] < page_num)
	{
		irq_unlock(flags);
		return NULL;
	}

	pool = pagepool[zone_index];
	size = pagepool_size[zone_index];

	// check alloc one page or continue page mode
	if(page_num == 1)
	{
	
		for(i = 0; i < size; i++)
		{
			page = &pool[i];
			if(ctx->pagepool_is_page_in_freelist(page))
			{
				index = (unsigned char)i | (zone_index << 7);
                recache = (index != ctx->freelist->next_index);
				ctx->pagepool_freelist_del_cb(index);

				freepage_num[zone_index] -= page_num;

                addr = ctx->pagepool_convert_index_to_addr(index);

                if(recache && page->ibank_no != (unsigned short)-1)
                {
                    ctx->pagepool_cache_ibank_into_freepage_cb(page->ibank_no, addr);
                }

				irq_unlock(flags);
				return addr;
			}
		}
		irq_unlock(flags);
		return NULL;
	}

	//size = (int)&__ram_mpool0_num;
	size = ctx->continus_start_alloc_page_num;

	//continue page alloc from end
	for(continus_index = 0, i = size - 1; i >= 0; i--)
	{
		page = &pool[i];
		if(ctx->pagepool_is_page_in_freelist(page))
		{
			continus_index++;
			if(continus_index == page_num)
			{
				for(continus_index = i + page_num; i < continus_index; i++)
				{
					page = &pool[i];

					index = (unsigned char)i | (zone_index << 7);
					ctx->pagepool_freelist_del_cb(index);
				}

				freepage_num[zone_index] -= page_num;
				irq_unlock(flags);

				i -= page_num;
				return ctx->pagepool_convert_index_to_addr(i);
			}
		}
		else
		{
			continus_index = 0;
		}
	}

	irq_unlock(flags);

	return NULL;
}


int rom_pagepool_free_pages(void *addr, unsigned int page_num, unsigned int zone_index,\
            const int *pagepool_size,
            unsigned char *freepage_num,
            page_free_ctx_t *ctx)
{
	unsigned int flags, i;
	unsigned char index;
	struct page *page;

	i = ctx->pagepool_convert_addr_to_pageindex(addr);

	if(i >= pagepool_size[zone_index])	//out of pool addr ring
	{
		return -EINVAL;
    }

	flags = irq_lock();
	for(index = i; index < i + page_num; index++)
	{
		page = ctx->pagepool_get_page_by_index(index | (zone_index << 7));

		//if page in free list, means not alloced, addr is bad
		if(ctx->pagepool_is_page_in_freelist(page))
		{
			irq_unlock(flags);
			return -EINVAL;
		}
	}

	for(index = i; index < i + page_num; index++)
	{
		ctx->pagepool_freelist_add(index | (zone_index << 7));
		page = ctx->pagepool_get_page_by_index(index | (zone_index << 7));
		page->ibank_no = -1;
	}
	freepage_num[zone_index] += page_num;
	irq_unlock(flags);

	return 0;
}


