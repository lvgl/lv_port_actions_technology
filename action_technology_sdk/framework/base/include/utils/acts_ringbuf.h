/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ACTS_RINGBUF_H__
#define __ACTS_RINGBUF_H__

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/util.h>
#include <mem_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIZE32_OF
# define SIZE32_OF(x) (sizeof((x)) / sizeof(uint32_t))
#endif
#ifndef SIZE16_OF
# define SIZE16_OF(x) (sizeof((x)) / sizeof(uint16_t))
#endif
#ifndef SIZE8_OF
# define SIZE8_OF(x) (sizeof((x)))
#endif
#ifndef IS_POWER_OF_TWO
# define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))
#endif

 /* get #of elements in a static array */
 #ifndef NELEM
 # define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
 #endif

/* actions ring buffer element size in bytes */
#define ACTS_RINGBUF_ELEMSZ (1)
#define ACTS_RINGBUF_NELEM(size8)	((size8) / ACTS_RINGBUF_ELEMSZ)
#define ACTS_RINGBUF_SIZE8(n)		((n) * ACTS_RINGBUF_ELEMSZ)

#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1 : __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1 : __max2; })

typedef int (*acts_ringbuf_read_fn)(void *, void *, unsigned int);
typedef int (*acts_ringbuf_write_fn)(void *, const void *, unsigned int);

struct acts_ringbuf {
	/* Index in buf for the head element */
	uint32_t head;
	/* Index in buf for the tail element */
	uint32_t tail;
	/* Size of buffer in elements */
	uint32_t size;
	/* Modulo mask if size is a power of 2 */
	uint32_t mask;
	/* cpu/dsp address of buffer  */
	uint32_t cpu_ptr;   /* in bytes */
	uint32_t dsp_ptr;	/* in 16-bit words */

	/* Index in actual buf for the head element */
	uint32_t head_offset;
	/* Index in actual buf for the tail element */
	uint32_t tail_offset;
};

/**
 * @brief Statically define and initialize a high performance ring buffer.
 *
 * This macro establishes a ring buffer whose size must be a power of 2;
 * that is, the ring buffer contains 2^pow 32-bit words, where @a pow is
 * the specified ring buffer size exponent. A high performance ring buffer
 * doesn't require the use of modulo arithmetic operations to maintain itself.
 *
 * This ring buffer defined in this way can only be accessed on cpu side.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct acts_ringbuf <name>; @endcode
 *
 * @param name Name of the ring buffer.
 * @param buf Ring buffer data area.
 * @param pow Ring buffer size exponent in elements.
 */
#define ACTS_RINGBUF_DEFINE_POW2(name, buf, pow) \
	struct acts_ringbuf name = {	\
		.head = 0,	\
		.head_offset = 0,  \
		.tail = 0,	\
		.tail_offset = 0,  \
		.mask = (1u << (pow)) - 1,	\
		.size = 1u << (pow),	\
		.cpu_ptr = (uint32_t)buf,	\
		.dsp_ptr = UINT32_MAX, \
	}

/**
 * @brief Statically define and initialize a standard ring buffer.
 *
 * This macro establishes a ring buffer of an arbitrary size. A standard
 * ring buffer uses modulo arithmetic operations to maintain itself.
 *
 * This ring buffer defined in this way can only be accessed on cpu side.
 *
 * The ring buffer can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct acts_ringbuf <name>; @endcode
 *
 * @param name Name of the ring buffer.
 * @param buf Ring buffer data area.
 * @param size_e Ring buffer size in elements.
 */
#define ACTS_RINGBUF_DEFINE(name, buf, size_e)	\
	struct acts_ringbuf name = {	\
		.head = 0,	\
		.head_offset = 0,  \
		.tail = 0,	\
		.tail_offset = 0,  \
		.mask = IS_POWER_OF_TWO(size_e) ? (size_e - 1) : 0,	\
		.size = size_e,	\
		.cpu_ptr = (uint32_t)buf,	\
		.dsp_ptr = UINT32_MAX, \
	}

/**
 * @brief Initialize a ring buffer.
 *
 * This routine initializes a ring buffer, prior to its first use.
 *
 * Setting @a size to a power of 2 establishes a high performance ring buffer
 * that doesn't require the use of modulo arithmetic operations to maintain
 * itself.
 *
 * @param buf Address of ring buffer.
 * @param data Ring buffer data area.
 * @param size Ring buffer size in elements.
 *
 * @return 0 if succeed, or 0 if not.
 */
int acts_ringbuf_init(struct acts_ringbuf *buf, void *data, uint32_t size);

/**
 * @brief ALLocate a ringbuf structure and initialize it.
 *
 * This routine allocate a ring buffer structure and initialize it,
 * prior to its first use.
 *
 * Setting @a size to a power of 2 establishes a high performance ring buffer
 * that doesn't require the use of modulo arithmetic operations to maintain
 * itself.
 *
 * @param data Ring buffer data area.
 * @param size Ring buffer size in elements.
 *
 * @return Ring buffer.
 */
struct acts_ringbuf *acts_ringbuf_init_ext(void *data, uint32_t size);

/**
 * @brief Destroy a ringbuf.
 *
 * This routine destroy a ring buffer initialized by acts_ringbuf_init_ext.
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A.
 */
void acts_ringbuf_destroy_ext(struct acts_ringbuf *buf);

/**
 * @brief Allocate a ring buffer.
 *
 * This routine allocate a ring buffer.
 *
 * Setting @a size to a power of 2 establishes a high performance ring buffer
 * that doesn't require the use of modulo arithmetic operations to maintain
 * itself.
 *
 * @param size Ring buffer size in elements.
 *
 * @return Ring buffer.
 */
struct acts_ringbuf *acts_ringbuf_alloc(uint32_t size);

/**
 * @brief Free a ring buffer.
 *
 * This routine free a ring buffer allocated by acts_ringbuf_alloc.
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A.
 */
void acts_ringbuf_free(struct acts_ringbuf *buf);

/**
 * @brief Peek a ring buffer.
 *
 * This routine peek a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data.
 * @param size Size of data in elements.
 *
 * @return number of elements successfully peek.
 */
uint32_t acts_ringbuf_peek(struct acts_ringbuf *buf, void *data, uint32_t size);

/**
 * @brief Read a ring buffer.
 *
 * This routine read a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data.
 * @param size Size of data in elements.
 *
 * @return number of elements successfully read.
 */
uint32_t acts_ringbuf_get(struct acts_ringbuf *buf, void *data, uint32_t size);

/**
 * @brief Get address of a valid data in a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is processed it can be freed
 * using @ref acts_ringbuf_get_finish.
 *
 * @param[in]  buf Address of ring buffer.
 * @param[out] data Pointer to the address. It is set to a location within
 *		    ring buffer.
 * @param[in] size Requested size in elements.
 *
 * @return Number of valid elements in the provided buffer which can be smaller
 *	   than requested if there is not enough free space or buffer wraps.
 */
uint32_t acts_ringbuf_get_claim(struct acts_ringbuf *buf, void **data, uint32_t size);

/**
 * @brief Indicate number of elements read from claimed buffer.
 *
 * @param  buf Address of ring buffer.
 * @param  size Number of elements that can be freed.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds valid elements in the ring buffer.
 */
int acts_ringbuf_get_finish(struct acts_ringbuf *buf, uint32_t size);

/**
 * @brief Write a ring buffer.
 *
 * This routine write a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param data Address of data.
 * @param size Size of data in elements.
 *
 * @return number of elements successfully written.
 */
uint32_t acts_ringbuf_put(struct acts_ringbuf *buf, const void *data, uint32_t size);

/**
 * @brief Allocate buffer for writing data to a ring buffer.
 *
 * With this routine, memory copying can be reduced since internal ring buffer
 * can be used directly by the user. Once data is written to allocated area
 * number of bytes written can be confirmed (see @ref acts_ringbuf_put_finish).
 *
 * @param[in]  buf Address of ring buffer.
 * @param[out] data Pointer to the address. It is set to a location within
 *		    ring buffer.
 * @param[in]  size Requested allocation size in elements.
 *
 * @return Size of allocated buffer which can be smaller than requested if
 *	   there is not enough free space or buffer wraps.
 */
uint32_t acts_ringbuf_put_claim(struct acts_ringbuf *buf, void **data, uint32_t size);

/**
 * @brief Indicate number of elements written to allocated buffers.
 *
 * @warning
 * Use cases involving multiple writers to the ring buffer must prevent
 * concurrent write operations, either by preventing all writers from
 * being preempted or by using a mutex to govern writes to the ring buffer.
 *
 * @warning
 * Ring buffer instance should not mix byte access and item access
 * (calls prefixed with ring_buf_item_).
 *
 * @param  buf Address of ring buffer.
 * @param  size Number of valid elements in the allocated buffers.
 *
 * @retval 0 Successful operation.
 * @retval -EINVAL Provided @a size exceeds free space in the ring buffer.
 */
int acts_ringbuf_put_finish(struct acts_ringbuf *buf, uint32_t size);

/**
 * @brief Copy a ring buffer.
 *
 * This routine copy a ring buffer.
 *
 * @param dst_buf Address of destination ring buffer.
 * @param src_buf Address of source ring buffer.
 * @param size Size of data in elements.
 *
 * @return number of elements successfully copied.
 */
uint32_t acts_ringbuf_copy(struct acts_ringbuf *dst_buf, struct acts_ringbuf *src_buf, uint32_t size);

/**
 * @brief Read a ring buffer to stream.
 *
 * This routine read a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param stream Handle of stream.
 * @param size Size of data in elements.
 * @param stream_write Stream write ops.
 *
 * @return number of elements successfully read.
 */
uint32_t acts_ringbuf_read(struct acts_ringbuf *buf,
		void *stream, uint32_t size, acts_ringbuf_write_fn stream_write);

/**
 * @brief Read a ring buffer to stream.
 *
 * This routine read a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param stream Handle of stream.
 * @param size Size of data in elements.
 * @param stream_read Stream read ops.
 *
 * @return number of elements successfully read.
 */
uint32_t acts_ringbuf_write(struct acts_ringbuf *buf,
		void *stream, uint32_t size, acts_ringbuf_read_fn stream_read);

/**
 * @brief Determine size of a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer size in elements.
 */
static inline uint32_t acts_ringbuf_size(struct acts_ringbuf *buf)
{
	return buf->size;
}

/**
 * @brief Determine data length in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer data length in elements.
 */
static inline uint32_t acts_ringbuf_length(struct acts_ringbuf *buf)
{
	return buf->tail - buf->head;
}

/**
 * @brief Determine free space in a ring buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return Ring buffer free space in elements.
 */
static inline uint32_t acts_ringbuf_space(struct acts_ringbuf *buf)
{
	return buf->size - (buf->tail - buf->head);
}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is empty, or 0 if not.
 */
static inline int acts_ringbuf_is_empty(struct acts_ringbuf *buf)
{
	return buf->head == buf->tail;
}

/**
 * @brief Determine if a ring buffer is full.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is full, or 0 if not.
 */
static inline int acts_ringbuf_is_full(struct acts_ringbuf *buf)
{
	return buf->size == (buf->tail - buf->head);
}

/**
 * @brief Determine if a ring buffer is empty.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is more than half empty, or 0 if not.
 */
static inline int acts_ringbuf_is_half_empty(struct acts_ringbuf *buf)
{
	return (buf->tail - buf->head) <= (buf->size >> 1);
}

/**
 * @brief Determine if a ring buffer is full.
 *
 * @param buf Address of ring buffer.
 *
 * @return 1 if the ring buffer is more than half full, or 0 if not.
 */
static inline int acts_ringbuf_is_half_full(struct acts_ringbuf *buf)
{
	return (buf->tail - buf->head) >= (buf->size >> 1);
}

/**
 * @brief Reset a ring buffer
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A
 */
static inline void acts_ringbuf_reset(struct acts_ringbuf *buf)
{
	buf->head = buf->head_offset = buf->tail = buf->tail_offset = 0;
}

/**
 * @brief Drop data of a ring buffer
 *
 * @param buf Address of ring buffer.
 * @param size Size of data in elements.
 *
 * @return number of elements dropped in elements.
 */
uint32_t acts_ringbuf_drop(struct acts_ringbuf *buf, uint32_t size);

/**
 * @brief Drop all data of a ring buffer
 *
 * @param buf Address of ring buffer.
 *
 * @return number of elements dropped in elements.
 */
uint32_t acts_ringbuf_drop_all(struct acts_ringbuf *buf);

/**
 * @brief Fill constant data of a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param size Size of data in elements.
 *
 * @return number of elements filled.
 */
uint32_t acts_ringbuf_fill(struct acts_ringbuf *buf, uint8_t c, uint32_t size);

/**
 * @brief Fill no data of a ring buffer.
 *
 * @param buf Address of ring buffer.
 * @param size Size of data in elements.
 *
 * @return number of elements filled.
 */
uint32_t acts_ringbuf_fill_none(struct acts_ringbuf *buf, uint32_t size);

/**
 * @brief Determine the internal buffer head offset of a ring buffer
 *
 * @param buf Address of ring buffer.
 *
 * @return the head offset of ring buffer.
 */
static inline int acts_ringbuf_head_offset(struct acts_ringbuf *buf)
{
    return buf->head_offset;
}

/**
 * @brief Determine the internal buffer head pointer of a ring buffer
 *
 * @param[in] buf Address of ring buffer.
 * @param[out] len store the number of contiguous accessable elements
 *             started from head
 *
 * @return the head pointer of ring buffer.
 */
void *acts_ringbuf_head_ptr(struct acts_ringbuf *buf, uint32_t *len);

/**
 * @brief Determine the internal buffer tail pointer of a ring buffer
 *
 * @param[in] buf Address of ring buffer.
 * @param[out] len store number of contiguous accessable elements
 *             started from tail
 *
 * @return the tail pointer of ring buffer.
 */
void *acts_ringbuf_tail_ptr(struct acts_ringbuf *buf, uint32_t *len);

/**
 * @brief Determine the internal buffer tail offset of a ring buffer
 *
 * @param buf Address of ring buffer.
 *
 * @return the tail offset of ring buffer.
 */
static inline int acts_ringbuf_tail_offset(struct acts_ringbuf *buf)
{
    return buf->tail_offset;
}

/**
 * @brief Defrag a ring buffer.
 *
 * This can help ring buffer to simulate a line buffer.
 *
 * @param buf Address of ring buffer.
 *
 * @return N/A
 */
void acts_ringbuf_defrag(struct acts_ringbuf *buf);

/**
 * @brief Dump information of a ring buffer
 *
 * @param buf Address of ring buffer.
 * @param name Name of ring buffer.
 * @param line_prefix Prefix of each line.
 *
 * @return N/A
 */
void acts_ringbuf_dump(struct acts_ringbuf *buf, const char *name, const char *line_prefix);

#ifdef __cplusplus
}
#endif

#endif /* __ACTS_RINGBUF_H__ */
