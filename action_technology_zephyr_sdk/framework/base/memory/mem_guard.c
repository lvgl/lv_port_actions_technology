/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief mem leak check.
*/
#include <errno.h>
#ifdef CONFIG_ACTIONS_PRINTK_DMA
#  include <sys/printk.h>
#endif
#include "mem_guard.h"

static int g_user_info = 0;

static int mem_guard_iterator(struct mem_guard_obj* node, void* context) {
    int * total = (int *)context;
    PRINTF("$Memory <0x%p %6u> (%s) %d \n", node->ptr, node->len - MEM_STAKE_SIZE, node->caller, node->user_info);
    *total += node->len;
    return 0;
}

static int mem_guard_check_overwirte(struct mem_guard_obj* node, void* context) {
    int *check_memory = (int *)((uint8_t *)node->ptr + node->len - 4);
    struct mem_guard_head* head = context;
    if (*check_memory != MEM_STAKE) {
        PRINTF("$Memory overwrite <0x%p %6u> (%s) view id %d \n", node->ptr, node->len - MEM_STAKE_SIZE, node->caller, node->user_info);
        for (int i = 0; i < MAX_MOBJ_NR; i++) {
            struct mem_guard_obj* free_node = &head->free_nodes[i];
            if((free_node->ptr <= node->ptr && (uint32_t)free_node->ptr + free_node->len > (uint32_t)node->ptr)
                || (free_node->ptr > node->ptr && (uint32_t)free_node->ptr < (uint32_t)node->ptr + node->len)) {
                PRINTF("$old used overwrite <0x%p %6u> (%s) view id %d \n", free_node->ptr, free_node->len - MEM_STAKE_SIZE, free_node->caller, free_node->user_info);
            }
        }
        return 1;
    }
    return 0;
}

static struct mem_guard_obj* mem_guard_obj_alloc(struct mem_guard_head* head) {
    char* free_node = head->free_list;
    if (free_node) {
        head->free_list = *(char**)free_node;
        head->free_blocks--;
        return (struct mem_guard_obj*)free_node;
    } else {
        PRINTF("mem guard node not enough free %d max %d \n", head->free_blocks, MAX_MOBJ_NR);
        mem_guard_dump(head);
        MM_ASSERT(0);
    }
    return NULL;
}

static void mem_guard_obj_free(struct mem_guard_head* head, void* node) {
    MM_ASSERT(node != NULL);
    if (node) {
        *(char**)node = head->free_list;
        head->free_list = node;
        head->free_blocks++;
    }
}

int mem_guard_add(struct mem_guard_head* head, const char* name,
    const void* obj, size_t len) {
    MUTEX_LOCK_DECLARE
    MUTEX_LOCK();
    mem_guard_iterate(head, mem_guard_check_overwirte, head);
    struct mem_guard_obj* node = mem_guard_obj_alloc(head);
    int index = MM_HASH(obj);
    node->caller = name;
    node->ptr = obj;
    node->len = len;
    node->user_info = g_user_info;
    node->next = head->heads[index];
    head->heads[index] = node;
    head->current_used_size += len;
    if (head->current_used_size > head->max_used_size) {
        head->max_used_size = head->current_used_size;
        //mem_guard_dump(head);
    }
    MUTEX_UNLOCK();
    return 0;
}

struct mem_guard_obj* mem_guard_find(struct mem_guard_head* head, const void* obj) {
    MUTEX_LOCK_DECLARE
        MUTEX_LOCK();
    int index = MM_HASH(obj);
    for (struct mem_guard_obj** node = &head->heads[index];
        *node != NULL;
        node = &(*node)->next) {
        if ((*node)->ptr == obj) {
            MUTEX_UNLOCK();
            return *node;
        }
    }
    MUTEX_UNLOCK();
    return NULL;
}

int mem_guard_del_locked(struct mem_guard_head* head, const void* obj) {
    struct mem_guard_obj** node, * tmp;
    int index = MM_HASH(obj);
    for (node = &head->heads[index]; *node != NULL;
        node = &(*node)->next) {
        if ((*node)->ptr == obj) {
            head->current_used_size -= (*node)->len;
            //memset((void *)(*node)->ptr, 0, (*node)->len);
            struct mem_guard_obj* free_node = &head->free_nodes[head->free_index++];
            free_node->ptr = (*node)->ptr;
            free_node->len = (*node)->len;
            free_node->caller = (*node)->caller;
            if (head->free_index >= MAX_MOBJ_NR) {
                head->free_index = 0;
            }
            tmp = *node;
            *node = tmp->next;
            tmp->next = NULL;
            mem_guard_obj_free(head, tmp);
            return 0;
        }
    }
    return -EINVAL;
}

int mem_guard_del(struct mem_guard_head* head, const void* obj,
    void (*mfree)(void* obj)) {
    //MM_ASSERT(mfree != NULL);
    MUTEX_LOCK_DECLARE
        if (obj == NULL)
            return -EINVAL;
    MUTEX_LOCK();
    head->free_ptr = (void*)obj;
    if (obj) {
        mem_guard_del_locked(head, obj);
    }
    if (mfree) {
        mem_guard_iterate(head, mem_guard_check_overwirte, (void*)obj);
        mfree((void*)obj);
    }
    MUTEX_UNLOCK();
    return 0;
}

void mem_guard_iterate(struct mem_guard_head* head,
    int (*iterator)(struct mem_guard_obj*, void*),
    void* context) {
    MUTEX_LOCK_DECLARE
    if (iterator == NULL)
        iterator = mem_guard_iterator;
    MUTEX_LOCK();
    for (int i = 0; i < (int)MAX_MOBJ_HASH; i++) {
        for (struct mem_guard_obj* node = head->heads[i];
            node != NULL; node = node->next) {
            MUTEX_UNLOCK();
            if(iterator(node, context)) {
                MUTEX_LOCK();
                break;
            }
            MUTEX_LOCK();
        }
    }
    MUTEX_UNLOCK();
}

void mem_guard_set_user_info(int user_info)
{
    g_user_info = user_info;
}

void mem_guard_leak_check(struct mem_guard_head* head, int user_info)
{
    MUTEX_LOCK_DECLARE
    MUTEX_LOCK();

#ifdef CONFIG_ACTIONS_PRINTK_DMA
    printk_dma_switch(0);
#endif

    for (int i = 0; i < (int)MAX_MOBJ_HASH; i++) {
        for (struct mem_guard_obj* node = head->heads[i];
            node != NULL; node = node->next) {
            MUTEX_UNLOCK();
            if (node->user_info == user_info) {
                  PRINTF("$Memory leak <0x%p %6u> (%s)\n", node->ptr, node->len, node->caller);
            }
            MUTEX_LOCK();
        }
    }

#ifdef CONFIG_ACTIONS_PRINTK_DMA
    printk_dma_switch(1);
#endif

    MUTEX_UNLOCK();
}

void mem_guard_check_overwrite(struct mem_guard_head* head)
{
    mem_guard_iterate(head, mem_guard_check_overwirte, head);
}

void mem_guard_init(struct mem_guard_head* head, int total_size) {
    char* p = (char*)head->nodes;
    head->free_blocks = MAX_MOBJ_NR;
    head->free_list = NULL;
    head->current_used_size = 0;
    head->max_used_size = 0;
    head->total_size = total_size;
    head->free_index = 0;
    for (int i = 0; i < head->free_blocks; i++) {
        *(char**)p = head->free_list;
        head->free_list = p;
        p += sizeof(struct mem_guard_obj);
    }
}
