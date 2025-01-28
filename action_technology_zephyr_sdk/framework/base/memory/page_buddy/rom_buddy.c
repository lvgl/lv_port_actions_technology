#include "heap.h"

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const struct buddy model =
{
	.page_no = 0,
	.max =
	{
		/*8bit i:0~14	index:0~14*/
		0x40,
		0x40, 0x20,
		0x20, 0x20, 0x20, 0x10,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08,
		/*4bit i:15~38	index:15~62*/
		0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		/*2bit i:39~54	index:63~126*/
		0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
		0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
		/*1bit i:55~70	index:127~254*/
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	},
	.type =
	{
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000,
	}
};

struct free_nodes
{
	unsigned char num;
	unsigned char index[9];
	unsigned char size[9];
};


static uint32_t fixsize(uint32_t size)
{
	size |= size >> 1;
	size |= size >> 2;
	size |= size >> 4;
	size |= size >> 8;
	size |= size >> 16;

	return (size + 1);
}

static void getMaxInfo(int *index, int *shift, int *mask)
{
	if(*index <= 14)
	{
		*shift = 0;
		*mask = 0xff;
	}
	else if(*index <= 62)
	{
		*index -= 15;
		*shift = (*index & 0x1) * 4;
		*index = (*index >> 1) + 15;
		*mask = 0x0f;
	}
	else if(*index <= 126)
	{
		*index -= 63;
		*shift = (*index & 0x3) * 2;
		*index = (*index >> 2) + 39;
		*mask = 0x03;
	}
	else
	{
		*index -= 127;
		*shift = (*index & 0x7);
		*index = (*index >> 3) + 55;
		*mask = 0x01;
	}
}

uint8_t getMax(struct buddy* self, int index)
{
	int shift, mask;

	getMaxInfo(&index, &shift, &mask);

	if(index <= MAX_INDEX)
	{
	    return ((self->max[index] >> shift) & mask);
    }
    else
    {
		return -1;
    }
}

//need check array out of bundary
void setMax(struct buddy* self, int index, uint8_t val)
{
	int shift, mask;

	getMaxInfo(&index, &shift, &mask);

	if(index <= MAX_INDEX)
	{
	    self->max[index] = (self->max[index] & (~(mask << shift))) | (val << shift);
    }
    else
    {
        return;
    }
}

uint8_t  rom_new_buddy_no(uint8_t page_no, rom_buddy_data_t *ctx)
{
	struct buddy* self;

	self = (struct buddy*)((char *)ctx->pagepool_convert_index_to_addr(page_no) + PAGE_SIZE - SELF_SIZE);
	memcpy((void*)self, (void*)&model, SELF_SIZE);
	self->page_no = page_no;
	self->freed = SELF_INDEX;

	return page_no;
}

static void update_alloc_parent(struct buddy* self, int index)
{
	while (index)
	{
		index = PARENT(index);
		setMax(self, index, max(getMax(self, LEFT_LEAF(index)), getMax(self, RIGHT_LEAF(index))));
	}
}

static void setType(struct buddy* self, int index)
{
	int shift;

	shift = index & 0x1f;	//index %= 32;
	index >>= 5;		//index /= 32;
	self->type[index] |= 1 << shift;
}

static int32_t get_contins_size(struct buddy* self, uint32_t index, uint32_t node_size)
{
	int32_t nsize;

	//for(nsize = 0; getMax(self, index) == node_size; node_size /= 2)
    for(nsize = 0; (node_size > 0 && getMax(self, index) == node_size); node_size /= 2)
	{
		nsize += node_size;
		index = RIGHT_SIBLING_LEFT_LEAF(index);
	}
	return nsize;
}


int get_buddy_max(void)
{
	return MAX_FREE_SIZE;
}

static void clearType(struct buddy* self, int index)
{
	int shift;

	shift = index & 0x1f;	//index %= 32;
	index >>= 5;		//index /= 32;
	self->type[index] &= ~(1 << shift);
}

int rom_buddy_getType(struct buddy* self, int index)
{
	int shift;

	shift = index & 0x1f;	//index %= 32;
	index >>= 5;		//index /= 32;

	return (self->type[index] & (1 << shift));
}

static void update_free_parent(struct buddy* self, int index, int node_size)
{
	uint32_t left_max, right_max;

	while (index)
	{
		index = PARENT(index);
		node_size *= 2;

		left_max = getMax(self, LEFT_LEAF(index));
		right_max = getMax(self, RIGHT_LEAF(index));

		if (left_max + right_max == node_size)
			setMax(self, index, node_size);
		else
			setMax(self, index, max(left_max, right_max));
	}
}

static int get_nsize_and_nodes(struct buddy* self, int index, int node_size, struct free_nodes *free_nodes)
{
	int nsize;

	free_nodes->num = 0;
	nsize = node_size;
	free_nodes->index[free_nodes->num] = index;
	free_nodes->size[free_nodes->num] = node_size;
	free_nodes->num++;

	for(index = RIGHT_SIBLING_LEFT_LEAF(index), node_size /= 2; node_size; node_size /= 2)
	{

		if(getMax(self, index) == 0)
		{

			if(rom_buddy_getType(self, index))
				break;

			else if(index > MAX_INDEX)
			{
				nsize += node_size;
				free_nodes->index[free_nodes->num] = index;
				free_nodes->size[free_nodes->num] = node_size;
				free_nodes->num++;
				break;
			}

			else if(getMax(self, LEFT_LEAF(index)) && getMax(self, RIGHT_LEAF(index)))
			{
				nsize += node_size;
				free_nodes->index[free_nodes->num] = index;
				free_nodes->size[free_nodes->num] = node_size;
				free_nodes->num++;

				index = RIGHT_SIBLING_LEFT_LEAF(index);
			}
			else
			{
				index = LEFT_LEAF(index);
			}
		}

		else if(getMax(self, index) == node_size)
			break;

		else
		{
			index = LEFT_LEAF(index);
		}
	}

	return nsize;
}

bool rom_is_buddy_idled(uint8_t buddy_no, rom_buddy_data_t *ctx)
{
	void *page;
	struct buddy* self;

	page = ctx->pagepool_convert_index_to_addr(buddy_no);
	self = (struct buddy*)((char *)page + PAGE_SIZE - SELF_SIZE);

	if(self->freed == SELF_INDEX)
		return true;
	else
		return false;
}

void * rom_buddy_alloc(uint8_t buddy_no, int32_t *size, rom_buddy_data_t *ctx)
{
	void *page;
	struct buddy* self;
	uint32_t index;
	uint32_t node_size;
	uint32_t offset = 0;
	int32_t bsize = *size, nsize;

	if(bsize <= 0)
	{
		ctx->printf("buddy_alloc size %d ?\n", bsize);
		return NULL;
	}

	if(bsize > MAX_FREE_SIZE)
	{
		return NULL;
    }
	page = ctx->pagepool_convert_index_to_addr(buddy_no);
	self = (struct buddy*)((char *)page + PAGE_SIZE - SELF_SIZE);
	if(self->page_no != buddy_no) {
		ctx->printf("wrong buddy_no %d,%d\n", self->page_no, buddy_no);
		return NULL;
	}

	bsize = (bsize + UNIT_SIZE - 1) / UNIT_SIZE;

	if(bsize > MAX_INDEX / 4)
	{
		if(bsize <= (MAX_INDEX / 2) - BUDDY_SELF_SIZE
			&& get_contins_size(self, 5,  MAX_INDEX / 4) >= bsize)
		{
			index = 2;
			offset = MAX_INDEX / 2;
			node_size = MAX_INDEX / 2;
		}
		else
		{
			if(get_contins_size(self, 1,  MAX_INDEX / 2) < bsize)
				return NULL;

			offset = 0;

			if(bsize > MAX_INDEX / 2)
			{
				index = 0;
				node_size = MAX_INDEX;
			}
			else
			{
				index = 1;
				node_size = MAX_INDEX / 2;
			}
		}
	}
	else
	{
		index = 0;
		if(bsize == 0)
			bsize = 1;

		if(!IS_POWER_OF_2(bsize))
			nsize = fixsize(bsize);
		else
			nsize = bsize;

		if(getMax(self, index) < nsize)
			return NULL;

/*
		 for(node_size = MAX_INDEX; node_size != nsize; node_size /= 2 )
		{
			if (getMax(self, LEFT_LEAF(index)) >= nsize)
				index = LEFT_LEAF(index);
			else
				index = RIGHT_LEAF(index);
		}

*/

		for(node_size = MAX_INDEX; node_size != nsize; node_size /= 2 )
		{
			if (getMax(self, RIGHT_LEAF(index)) >= nsize)
				index = RIGHT_LEAF(index);
			else
				index = LEFT_LEAF(index);
		}
		offset = ((index + 1) * node_size) - MAX_INDEX;
	}

	if(node_size > bsize)
	{
		nsize = bsize;
		setType(self, LEFT_LEAF(index));

		for(node_size /= 2;  node_size > 0; node_size /= 2)
		{
			if(nsize >= node_size)
			{
				setMax(self, LEFT_LEAF(index), 0);
				update_alloc_parent(self, LEFT_LEAF(index));
				nsize -= node_size;
				index = RIGHT_LEAF(index);
			}
			else
			{
				index = LEFT_LEAF(index);
			}
		}
	}
	else
	{
		setType(self, index);
		setMax(self, index, 0);
		update_alloc_parent(self, index);
	}

	*size = bsize * UNIT_SIZE;
	self->freed -= bsize;
	return ((char *)page + (offset * UNIT_SIZE));
}

int rom_buddy_free(uint8_t buddy_no, void *where, unsigned long *info, rom_buddy_data_t *ctx)
{
	//unsigned int flags;
#ifdef CONFIG_SYS_IRQ_LOCK 
	SYS_IRQ_FLAGS flags;
#endif
	void *page;
	struct buddy* self;
	uint32_t node_size, size, index = 0;
	uint32_t offset;
	struct free_nodes free_nodes;

	page = ctx->pagepool_convert_index_to_addr(buddy_no);
	self = (struct buddy*)((char *)page + PAGE_SIZE - SELF_SIZE);
	if(self->page_no != buddy_no) {
		ctx->printf("wrong buddy_no %d,%d, %p\n", buddy_no, self->page_no, where);
		return -EINVAL;
	}

	offset = ((uintptr_t)where - (uintptr_t)page) / UNIT_SIZE;

	if(offset >= SELF_INDEX)
	{
		ctx->printf("%p not belong buddy_no %d\n", where, buddy_no);
		return -EINVAL;
	}

	node_size = 1;
	index = offset + MAX_INDEX - 1;

	for (; getMax(self, index); index = PARENT(index))
	{
		node_size *= 2;
		if (index == 0)
		{
			ctx->printf("%p freed already\n", where);

#if 1
			{
	volatile int ii = 1;
	os_printk("%s: buddy_no %d, where 0x%p, info 0x%p, offset 0x%x, node_size %d\n", __func__, buddy_no, where, info, offset, node_size);
	//dump_stack();
	while(ii == 1);
			}
#endif
			return -EINVAL;
		}
	}

	if(!rom_buddy_getType(self, index) || where != ((char *)page + (offset * UNIT_SIZE)))
	{
	    ctx->printf("BUG:free 0x%x but need 0x%x\n", where, ((char *)page + (offset * UNIT_SIZE)));
		ctx->halt();
	}

	node_size = get_nsize_and_nodes(self, index, node_size, &free_nodes);
	size = node_size * UNIT_SIZE;
	if(info)
	{
		*info = *(unsigned long *)((char *)where + size - 4);
	}

#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_lock(&flags);
#endif

	for(index = 0; index < free_nodes.num; index++)
	{
		if(index == 0)
			clearType(self, free_nodes.index[index]);
		setMax(self, free_nodes.index[index], free_nodes.size[index]);
		update_free_parent(self, free_nodes.index[index], free_nodes.size[index]);
	}
	self->freed += node_size;

#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#endif

	return size;
}



