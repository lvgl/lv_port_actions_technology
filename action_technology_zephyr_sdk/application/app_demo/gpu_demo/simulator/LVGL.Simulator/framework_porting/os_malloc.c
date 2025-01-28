/*
********************************************************************************
*                       noya131---upgrade.bin
*                (c) Copyright 2002-2007, Actions Co,Ld. 
*                        All Right Reserved 
*
* FileName: upgrade.h     Author: huang he        Date:2007/12/24
* Description: defines macros for upgrading
* Others:      
* History:         
* <author>    <time>       <version >    <desc>
*   huang he    2007/12/24       1.0         build this file
********************************************************************************
*/ 
#include <string.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define MY_NBYTE_ALIGN(x, y) ((((x) + y-1)/y)*y)             /* alloc based on 16 byte */
#define MY_BYTE_ALIGN(x) MY_NBYTE_ALIGN(x,32)             /* alloc based on 16 byte */
#define MALLOC_LARGE_BUF 0x400000

struct alloc_struct 
{
    unsigned long address;
    unsigned long size;
    struct alloc_struct *next;
    unsigned long reserve[5];     /*modify by yujing 20111219*/
};

struct alloc_struct head,tail;

#ifdef DEBUG_MEM_USED
#define DEBUG_MAX_MEM_CNT 100
#define MEM_ALIGN_LEN		32

static unsigned int current_mem_used = 0;
static unsigned int max_mem_used = 0;

static unsigned short mem_len_cnt[DEBUG_MAX_MEM_CNT];
static unsigned short max_len_cnt[DEBUG_MAX_MEM_CNT];

static void debug_malloc_size(unsigned int size)
{
	unsigned int index;

	current_mem_used += size;
	if (current_mem_used > max_mem_used) {
		max_mem_used = current_mem_used;
	}

	index = size/MEM_ALIGN_LEN;
	if (index >= DEBUG_MAX_MEM_CNT) {
		index = 0;
	}

	mem_len_cnt[index]++;
	if (mem_len_cnt[index] > max_len_cnt[index]) {
		max_len_cnt[index] = mem_len_cnt[index];
	}
}

static void debug_free_size(unsigned int size)
{
	unsigned int index;

	current_mem_used -= size;

	index = size/MEM_ALIGN_LEN;
	if (index >= DEBUG_MAX_MEM_CNT) {
		index = 0;
	}
	mem_len_cnt[index]--;
}
#endif

void debug_printk_used(void)
{
#ifdef DEBUG_MEM_USED
	unsigned int i;
	printk("max_mem_used:%d\n", max_mem_used);
	for (i=0; i<DEBUG_MAX_MEM_CNT; i++) {
		if (max_len_cnt[i] != 0) {
			printk("Mem size %s %d, max used cnt:%d\n", (i==0)? ">=" : ":",
					(i==0)? MEM_ALIGN_LEN*100 : MEM_ALIGN_LEN*i, max_len_cnt[i]);
		}
	}
#endif
}



int owl_init_heap(unsigned long startaddr, unsigned int heap_buf_size)
{
#ifdef DEBUG_MEM_USED
	unsigned int i;

	for (i=0; i<DEBUG_MAX_MEM_CNT; i++) {
		mem_len_cnt[i] = 0;
		max_len_cnt[i] = 0;
	}
#endif

    head.size = tail.size = 0;
    head.address = startaddr;
    tail.address = startaddr + heap_buf_size;
    head.next = &tail;
    tail.next = (struct alloc_struct *)0;

    return 0;
}

void *owl_malloc(unsigned int num_bytes)
{
    struct alloc_struct *ptr, *newptr;
	unsigned int new_bytes;
	
    if (num_bytes == 0) 
    {
    	return 0;
	}

    new_bytes = MY_BYTE_ALIGN(num_bytes);       /* translate the byte count to size of long type       */
    ptr = &head;                                /* scan from the head of the heap                      */
    while (ptr && ptr->next)                    /* look for enough memory for alloc                    */
    {
        if (ptr->next->address >= (ptr->address + ptr->size + \
                2*sizeof(struct alloc_struct) + new_bytes)) 
        {
        	break;
        }
                                                /* find enough memory to alloc                         */
        ptr = ptr->next;
    }

    if (ptr->next == 0)
    {
        return 0;                   /* it has reached the tail of the heap now             */
    }
  
    if(new_bytes < MALLOC_LARGE_BUF)
    {
        newptr = (struct alloc_struct *)(ptr->address + ptr->size);
    }
    else
    {
        newptr = (struct alloc_struct *)(ptr->next->address - (ptr->size + 2*sizeof(struct alloc_struct) + new_bytes) );
    }
                                                /* create a new node for the memory block              */
    if (newptr == 0)
    {
        return 0;                   /* create the node failed, can't manage the block      */
    }

    /* set the memory block chain, insert the node to the chain */
    newptr->address = ptr->address + ptr->size + sizeof(struct alloc_struct);
    newptr->size = new_bytes;
    newptr->next = ptr->next;
    ptr->next = newptr;
	
//	adfu_printf("0x%8x,malloc:0x%x,num_bytes:0x%x, new_bytes:0x%x\n", calladdr,newptr->address, num_bytes, new_bytes);
#ifdef DEBUG_MEM_USED
	debug_malloc_size(new_bytes);
#endif

    return (void *)(newptr->address);
}

void owl_free(void *p)
{
	int hit = 0;
    struct alloc_struct *ptr, *prev;
	
    ptr = &head;                /* look for the node which point this memory block                     */
    while (ptr && ptr->next)
    {
        if (ptr->next->address == (unsigned long)p)
        {
            hit = 1;
            break;              /* find the node which need to be release                              */
        }
        ptr = ptr->next;
    }

    if(!hit)
    {
        //dump_stack();
        os_printk("owl_free_error %p \n",p);
    }
#ifdef DEBUG_MEM_USED
	debug_free_size(ptr->next->size);
#endif

	prev = ptr;
    ptr = ptr->next;

    if (ptr == 0) 
    {
    	return; /* the node is heap tail                                               */
	}

    prev->next = ptr->next;     /* delete the node which need be released from the memory block chain  */

    return ;
}

void* owl_calloc(unsigned int n_elements, unsigned int elem_size)
{
	void *buf;
	
	buf = owl_malloc(n_elements*elem_size);
	memset(buf, 0, n_elements*elem_size);
	return buf;
}

static unsigned long get_buf_size(void *mem)
{
	struct alloc_struct *p_malloc_header;
	
	if(mem != NULL)
	{
		p_malloc_header = (struct alloc_struct *)((unsigned long)mem - sizeof(struct alloc_struct));
		return p_malloc_header->size;
	}
	else
	{
		return 0;
	}
}

void* owl_realloc(void* oldMem, unsigned int bytes)
{
	void *newMem;
	unsigned long oldMemSize;
	
	if(oldMem == NULL)
	{
		return owl_malloc(bytes);
	}
	
	oldMemSize = get_buf_size(oldMem);
	if(oldMemSize >= bytes)
	{
		return oldMem;
	}
	newMem = owl_malloc(bytes);
	if(newMem != NULL)
	{
		memcpy(newMem, oldMem, oldMemSize);
		owl_free(oldMem);
	}
	return newMem;
}
