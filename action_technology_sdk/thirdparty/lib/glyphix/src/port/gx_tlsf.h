#ifndef GX_TLSF_H_
#define GX_TLSF_H_

#include <cstddef>

namespace gx {
struct tlsf_heap *tlsf_heap_init(void *mem, size_t size);
void *tlsf_malloc_align(struct tlsf_heap *heap, size_t size, size_t align);
void tlsf_free_align(struct tlsf_heap *heap, void *ptr);
void *tlsf_malloc(struct tlsf_heap *heap, size_t size);
void tlsf_free(struct tlsf_heap *heap, void *ptr);
void *tlsf_realloc(struct tlsf_heap *heap, void *ptr, size_t new_size);
void tlsf_trace(struct tlsf_heap *heap);
size_t tlsf_size(void *ptr);
} // namespace gx

#endif
