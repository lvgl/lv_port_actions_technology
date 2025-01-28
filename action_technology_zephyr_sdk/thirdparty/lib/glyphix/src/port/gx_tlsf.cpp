/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include "gx_tlsf.h"

#include "gx_mutex.h"
#include "sys/printk.h"
#include "tlsf.h"
#include <stdint.h>
#include <string.h>
#include <new>

// #define DEBUG_GX_TLSF_MEMORY 

namespace gx {
struct tlsf_heap {
  size_t total; /**< memory size */
  size_t used;  /**< size used */
  size_t max;   /**< maximum usage */
  pool_t pool;
  tlsf_t tlsf_handle;
  Mutex mutex;
};

struct tlsf_heap *tlsf_heap_init(void *mem, size_t size) {
  GX_ASSERT(mem != nullptr);
//  GX_ASSERT(size >= 1024 * 1024);
  struct tlsf_heap *heap = new (mem) tlsf_heap;
  void *heap_start = heap + 1;
  size_t heap_size = size - sizeof(tlsf_heap);
  /* init tlsf memory object */
  heap->total = heap_size;
  heap->used = 0;
  heap->max = 0;
  /* init heap */
  heap->tlsf_handle = tlsf_create_with_pool(heap_start, heap_size);
  GX_ASSERT(heap->tlsf_handle != NULL);
  heap->pool = tlsf_get_pool(heap->tlsf_handle);
  printk(" init tlsf heap size:%d\n", heap_size);
  return heap;
}

void *tlsf_malloc_align(struct tlsf_heap *heap, size_t size, size_t align) {
  void *ptr = NULL;
  if (size) {
    LockGuard<Mutex> lock(heap->mutex);
    ptr = ::tlsf_memalign(heap->tlsf_handle, align, size);
    heap->used += ::tlsf_block_size(ptr);
    if (heap->used > heap->max) {
      heap->max = heap->used;
    }
    GX_ASSERT(intptr_t(ptr) % align == 0);
  }
  return ptr;
}

void tlsf_free_align(struct tlsf_heap *heap, void *ptr) {
  /* NULL check */
  if (ptr == NULL)
    return;
  tlsf_free(heap, ptr);
}

void *tlsf_malloc(struct tlsf_heap *heap, size_t size) {
  void *ptr = NULL;
  if (size) {
    LockGuard<Mutex> lock(heap->mutex);
    ptr = ::tlsf_malloc(heap->tlsf_handle, size);
    heap->used += ::tlsf_block_size(ptr);
    if (heap->used > heap->max) {
      heap->max = heap->used;
    }
  }
  return ptr;
}

void tlsf_free(struct tlsf_heap *heap, void *ptr) {
  if (ptr != NULL) {
    LockGuard<Mutex> lock(heap->mutex);
    heap->used -= ::tlsf_block_size(ptr);
    ::tlsf_free(heap->tlsf_handle, ptr);
  }
}

void *tlsf_realloc(struct tlsf_heap *heap, void *ptr, size_t new_size) {
  size_t old_size;

  if (new_size == 0) {
    tlsf_free(heap, ptr);
    return NULL;
  }
  if (ptr == NULL) {
    return tlsf_malloc(heap, new_size);
  }
  LockGuard<Mutex> lock(heap->mutex);
  old_size = ::tlsf_block_size(ptr);
  ptr = ::tlsf_realloc(heap->tlsf_handle, ptr, new_size);
  if (ptr != NULL) {
    heap->used -= old_size;
    heap->used += ::tlsf_block_size(ptr);
    if (heap->used > heap->max) {
      heap->max = heap->used;
    }
  }
  return ptr;
}

size_t tlsf_size(void *ptr) { return tlsf_block_size(ptr); }

struct tlsf_mem_info {
  size_t used_size;
  size_t free_size;
  int (*out)(const char *fmt, ...);
};
#if 0
static void mem_info(void *ptr, size_t size, int used, void *user) {
  struct tlsf_mem_info *info_ptr = (struct tlsf_mem_info *)user;

  info_ptr->out("[0x%08x - ", ptr);
  if (size < 1024)
    info_ptr->out("%5d", size);
  else if (size < 1024 * 1024)
    info_ptr->out("%4dK", size / 1024);
  else
    info_ptr->out("%4dM", size / (1024 * 1024));

  if (used) {
    info_ptr->used_size += size;
    info_ptr->out("    USED\n");
  } else {
    info_ptr->free_size += size;
    info_ptr->out("\n");
  }
}
#endif
typedef struct {
    size_t max_free_blocks[3];
} max_blocks_finder_t;

void init_max_blocks_finder(max_blocks_finder_t* finder) {
    finder->max_free_blocks[0] = 0;
    finder->max_free_blocks[1] = 0;
    finder->max_free_blocks[2] = 0;
}

void update_max_blocks(size_t size, max_blocks_finder_t* finder) {
    if (size > finder->max_free_blocks[0]) {
        finder->max_free_blocks[2] = finder->max_free_blocks[1];
        finder->max_free_blocks[1] = finder->max_free_blocks[0];
        finder->max_free_blocks[0] = size;
    } else if (size > finder->max_free_blocks[1]) {
        finder->max_free_blocks[2] = finder->max_free_blocks[1];
        finder->max_free_blocks[1] = size;
    } else if (size > finder->max_free_blocks[2]) {
        finder->max_free_blocks[2] = size;
    }
}

static void find_max_free_blocks(void* ptr, size_t size, int used, void* user) {
    if (!used) {
        max_blocks_finder_t* finder = (max_blocks_finder_t*)user;
        update_max_blocks(size, finder);
    }
}

void get_max_free_blocks(void* tlsf, size_t* max_blocks) {
    max_blocks_finder_t finder;
    init_max_blocks_finder(&finder);
    tlsf_walk_pool(tlsf, find_max_free_blocks, &finder);

    max_blocks[0] = finder.max_free_blocks[0];
    max_blocks[1] = finder.max_free_blocks[1];
    max_blocks[2] = finder.max_free_blocks[2];
}

void tlsf_trace(struct tlsf_heap *heap) {
  struct tlsf_mem_info info = {0};
  if (heap == NULL) {
    return;
  }
  info.out = (int (*)(const char *, ...))printk;
  printk("\ntlsf gx ui heap infomation:\n");
  printk("total   : %d\n", heap->total);
  printk("used    : %d\n", heap->used);
  printk("max_used: %d\n", heap->max);
#ifdef DEBUG_GX_TLSF_MEMORY
  printk("\n--memory item information --\n");
  tlsf_walk_pool(heap->pool, mem_info, &info);
  printk("\n-- used:%d  free:%d --\n", info.used_size, info.free_size);
#endif
  size_t max_blocks[3];
  get_max_free_blocks(heap->pool, max_blocks);
  printk("Top 3 largest free blocks: %zu, %zu, %zu bytes\n", max_blocks[0], max_blocks[1], max_blocks[2]);
}
} // namespace gx
