#include <sys/sys_heap.h>

/* kernel synchronized heap struct */

struct k_heap {
	struct sys_heap heap;
    void* mutex;
};

/**
 * @brief Initialize a k_heap
 *
 * This constructs a synchronized k_heap object over a memory region
 * specified by the user.  Note that while any alignment and size can
 * be passed as valid parameters, internal alignment restrictions
 * inside the inner sys_heap mean that not all bytes may be usable as
 * allocated memory.
 *
 * @param h Heap struct to initialize
 * @param mem Pointer to memory.
 * @param bytes Size of memory region, in bytes
 */
void k_heap_init(struct k_heap *h, void *mem, size_t bytes);

/** @brief Allocate aligned memory from a k_heap
 *
 * Behaves in all ways like k_heap_alloc(), except that the returned
 * memory (if available) will have a starting address in memory which
 * is a multiple of the specified power-of-two alignment value in
 * bytes.  The resulting memory can be returned to the heap using
 * k_heap_free().
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 * @note When CONFIG_MULTITHREADING=n any @a timeout is treated as K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param h Heap from which to allocate
 * @param align Alignment in bytes, must be a power of two
 * @param bytes Number of bytes requested
 * @param timeout How long to wait, or K_NO_WAIT
 * @return Pointer to memory the caller can now use
 */
void *k_heap_aligned_alloc(struct k_heap *h, size_t align, size_t bytes,
			int timeout);

/**
 * @brief Allocate memory from a k_heap
 *
 * Allocates and returns a memory buffer from the memory region owned
 * by the heap.  If no memory is available immediately, the call will
 * block for the specified timeout (constructed via the standard
 * timeout API, or K_NO_WAIT or K_FOREVER) waiting for memory to be
 * freed.  If the allocation cannot be performed by the expiration of
 * the timeout, NULL will be returned.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 * @note When CONFIG_MULTITHREADING=n any @a timeout is treated as K_NO_WAIT.
 *
 * @funcprops \isr_ok
 *
 * @param h Heap from which to allocate
 * @param bytes Desired size of block to allocate
 * @param timeout How long to wait, or K_NO_WAIT
 * @return A pointer to valid heap memory, or NULL
 */
void *k_heap_alloc(struct k_heap *h, size_t bytes,
				 int timeout);

/**
 * @brief Free memory allocated by k_heap_alloc()
 *
 * Returns the specified memory block, which must have been returned
 * from k_heap_alloc(), to the heap for use by other callers.  Passing
 * a NULL block is legal, and has no effect.
 *
 * @param h Heap to which to return the memory
 * @param mem A valid memory block, or NULL
 */
void k_heap_free(struct k_heap *h, void *mem);

/**
 * @defgroup heap_apis Heap APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Allocate memory from the heap with a specified alignment.
 *
 * This routine provides semantics similar to aligned_alloc(); memory is
 * allocated from the heap with a specified alignment. However, one minor
 * difference is that k_aligned_alloc() accepts any non-zero @p size,
 * wherase aligned_alloc() only accepts a @p size that is an integral
 * multiple of @p align.
 *
 * Above, aligned_alloc() refers to:
 * C11 standard (ISO/IEC 9899:2011): 7.22.3.1
 * The aligned_alloc function (p: 347-348)
 *
 * @param align Alignment of memory requested (in bytes).
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_aligned_alloc(size_t align, size_t size);

/**
 * @brief Allocate memory from the heap.
 *
 * This routine provides traditional malloc() semantics. Memory is
 * allocated from the heap memory pool.
 *
 * @param size Amount of memory requested (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_malloc(size_t size);

/**
 * @brief Free memory allocated from heap.
 *
 * This routine provides traditional free() semantics. The memory being
 * returned must have been allocated from the heap memory pool or
 * k_mem_pool_malloc().
 *
 * If @a ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to previously allocated memory.
 *
 * @return N/A
 */
extern void k_free(void *ptr);

/**
 * @brief Allocate memory from heap, array style
 *
 * This routine provides traditional calloc() semantics. Memory is
 * allocated from the heap memory pool and zeroed.
 *
 * @param nmemb Number of elements in the requested array
 * @param size Size of each array element (in bytes).
 *
 * @return Address of the allocated memory if successful; otherwise NULL.
 */
extern void *k_calloc(size_t nmemb, size_t size);
