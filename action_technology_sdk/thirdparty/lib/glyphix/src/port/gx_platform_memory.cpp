/*
 * This file is part of PersimUI
 * Copyright (c) 2006-2023, RT-Thread Development Team
 */
#include "gx_application.h"
#include "gx_lvgx.h"
#include "gx_memory.h"
#include "gx_tlsf.h"
#include "kernel.h"
#include <cstdlib>

#define GX_LOG_TAG "heap"
#include "gx_logger.h"

namespace gx {
static tlsf_heap *uiHeap;

__in_section_unique(lvgl.noinit.malloc)
    __aligned(4) static uint8_t heap_mem[CONFIG_GX_UI_HEAP_MEM_POOL_SIZE];

extern "C" void lvgx_heap_init(void) {
  uiHeap = tlsf_heap_init(heap_mem, CONFIG_GX_UI_HEAP_MEM_POOL_SIZE);
}

void *alloc_align(size_t size, size_t align) {
  if (!size)
    return nullptr;
  void *ptr = tlsf_malloc_align(uiHeap, size, align);
  for (int i = 0; !ptr && i < 10; ++i) {
    LogInfo() << "flush ui cache:" << App()->flushCache(size);
    ptr = tlsf_malloc_align(uiHeap, size, align);
  }
  if (!ptr) {
    LogError() << "alloc_align fail" << align << size << ptr;
    tlsf_trace(uiHeap);
  }
  return ptr;
}

void free_align(void *ptr) {
  if (ptr == NULL)
    return;
  tlsf_free(uiHeap, ptr);
}

void *malloc(std::size_t size, int) {
  if (!size)
    return nullptr;
  void *ptr = tlsf_malloc(uiHeap, size);
  for (int i = 0; !ptr && i < 10; ++i) {
    LogInfo() << "flush ui cache:" << App()->flushCache(size);
    ptr = tlsf_malloc(uiHeap, size);
  }
  if (!ptr) {
    LogError() << "malloc fail" << size << ptr;
    tlsf_trace(uiHeap);
  }
  return ptr;
}

void *realloc(void *ptr, std::size_t size, int) {
  void *reptr = tlsf_realloc(uiHeap, ptr, size);
  if (size) {
    for (int i = 0; !reptr && i < 3; ++i) {
      LogInfo() << "flush ui cache:" << App()->flushCache(size);
      reptr = tlsf_realloc(uiHeap, ptr, size);
    }
    if (!reptr) {
      LogError() << "realloc fail" << size << reptr;
      tlsf_trace(uiHeap);
    }
  }
  return reptr;
}

void free(void *ptr, int) { tlsf_free(uiHeap, ptr); }

std::size_t malloc_usable_size(const void *ptr, int) {
  return tlsf_size(const_cast<void *>(ptr));
}

namespace texture_allocator {
uint8_t *alloc(std::size_t size) {
  return static_cast<uint8_t *>(alloc_align(size, 64));
}

Block alloc(int width, int height, int pixfmt) {
  int pitch = (width * PixelFormat::depth(pixfmt) + 7) / 8;
  std::size_t size = pitch * height;
  uint8_t *ptr = static_cast<uint8_t *>(alloc_align(size, 64));
  return Block(ptr, size, pitch);
}

void free(uint8_t *ptr) { free_align(ptr); }
} // namespace texture_allocator
} // namespace gx

void *operator new(size_t size) { return gx::malloc(size, gx::AllocChunk); }

void *operator new[](size_t size) { return gx::malloc(size, gx::AllocChunk); }

void operator delete(void *ptr) GX_NOEXCEPT { gx::free(ptr, gx::AllocChunk); }

void operator delete[](void *ptr) GX_NOEXCEPT { gx::free(ptr, gx::AllocChunk); }

#if (__cplusplus > 201103L)
void operator delete(void *ptr, size_t) GX_NOEXCEPT {
  gx::free(ptr, gx::AllocChunk);
}

void operator delete[](void *ptr, size_t) GX_NOEXCEPT {
  gx::free(ptr, gx::AllocChunk);
}
#endif // __cplusplus > 201103L
