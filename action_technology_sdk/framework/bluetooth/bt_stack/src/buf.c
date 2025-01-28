/* buf.c - Buffer management */

/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>

#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "net_buf"
#define LOG_LEVEL CONFIG_NET_BUF_LOG_LEVEL
#include <logging/log.h>
//LOG_MODULE_DECLARE(LOG_MODULE_NAME, LOG_LEVEL);

#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/byteorder.h>

#include <acts_net/buf.h>

/* Actions add start */
#define BUF_TEST_DEBUG		1

#if BUF_TEST_DEBUG
#define BUF_ASSERT(cond, str, ...)			__ASSERT(cond, "" __VA_ARGS__)
#define BUF_LOG(fmt, ...)				printk(fmt, ##__VA_ARGS__)
#else
#define BUF_ASSERT(cond, str, ...)
#define BUF_LOG(fmt, ...)
#endif
/* Actions add end */

#if defined(CONFIG_NET_BUF_LOG)
#define NET_BUF_DBG(fmt, ...) LOG_DBG("(%p) " fmt, k_current_get(), \
				      ##__VA_ARGS__)
#define NET_BUF_ERR(fmt, ...) LOG_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_WARN(fmt, ...) LOG_WRN(fmt, ##__VA_ARGS__)
#define NET_BUF_INFO(fmt, ...) LOG_INF(fmt, ##__VA_ARGS__)
#else

#define NET_BUF_DBG(fmt, ...)
#define NET_BUF_ERR(fmt, ...)
#define NET_BUF_WARN(fmt, ...)
#define NET_BUF_INFO(fmt, ...)
#endif /* CONFIG_NET_BUF_LOG */

#define NET_BUF_ASSERT(cond, ...) __ASSERT(cond, "" __VA_ARGS__)

#if CONFIG_NET_BUF_WARN_ALLOC_INTERVAL > 0
#define WARN_ALLOC_INTERVAL K_SECONDS(CONFIG_NET_BUF_WARN_ALLOC_INTERVAL)
#else
#define WARN_ALLOC_INTERVAL K_FOREVER
#endif

/* Linker-defined symbol bound to the static pool structs */
extern struct net_buf_pool _net_buf_pool_list[];

struct net_buf_pool *net_buf_pool_get(int id)
{
	return &_net_buf_pool_list[id];
}

static int pool_id(struct net_buf_pool *pool)
{
	return pool - _net_buf_pool_list;
}

int net_buf_id(struct net_buf *buf)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return buf - pool->__bufs;
}

static inline struct net_buf *pool_get_uninit(struct net_buf_pool *pool,
					      uint16_t uninit_count)
{
	struct net_buf *buf;

	buf = &pool->__bufs[pool->buf_count - uninit_count];

	buf->pool_id = pool_id(pool);

	return buf;
}

void net_buf_reset(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf->flags == 0U);
	__ASSERT_NO_MSG(buf->frags == NULL);

	net_buf_simple_reset(&buf->b);
}

static uint8_t *generic_data_ref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - 1;
	(*ref_count)++;

	return data;
}

static uint8_t *mem_pool_data_alloc(struct net_buf *buf, size_t *size,
				 k_timeout_t timeout)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;

	/* Reserve extra space for a ref-count (uint8_t) */
	void *b = k_heap_alloc(pool, 1 + *size, timeout);

	if (b == NULL) {
		return NULL;
	}

	ref_count = (uint8_t *)b;
	*ref_count = 1U;

	/* Return pointer to the byte following the ref count */
	return ref_count + 1;
}

static void mem_pool_data_unref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *buf_pool = net_buf_pool_get(buf->pool_id);
	struct k_heap *pool = buf_pool->alloc->alloc_data;
	uint8_t *ref_count;

	ref_count = data - 1;
	if (--(*ref_count)) {
		return;
	}

	/* Need to copy to local variable due to alignment */
	k_heap_free(pool, ref_count);
}

const struct net_buf_data_cb net_buf_var_cb = {
	.alloc = mem_pool_data_alloc,
	.ref   = generic_data_ref,
	.unref = mem_pool_data_unref,
};

static uint8_t *fixed_data_alloc(struct net_buf *buf, size_t *size,
			      k_timeout_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	*size = MIN(fixed->data_size, *size);

	return fixed->data_pool + fixed->data_size * net_buf_id(buf);
}

static void fixed_data_unref(struct net_buf *buf, uint8_t *data)
{
	/* Nothing needed for fixed-size data pools */
}

const struct net_buf_data_cb net_buf_fixed_cb = {
	.alloc = fixed_data_alloc,
	.unref = fixed_data_unref,
};

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

static uint8_t *heap_data_alloc(struct net_buf *buf, size_t *size,
			     k_timeout_t timeout)
{
	uint8_t *ref_count;

	ref_count = k_malloc(1 + *size);
	if (!ref_count) {
		return NULL;
	}

	*ref_count = 1U;

	return ref_count + 1;
}

static void heap_data_unref(struct net_buf *buf, uint8_t *data)
{
	uint8_t *ref_count;

	ref_count = data - 1;
	if (--(*ref_count)) {
		return;
	}

	k_free(ref_count);
}

static const struct net_buf_data_cb net_buf_heap_cb = {
	.alloc = heap_data_alloc,
	.ref   = generic_data_ref,
	.unref = heap_data_unref,
};

const struct net_buf_data_alloc net_buf_heap_alloc = {
	.cb = &net_buf_heap_cb,
};

#endif /* CONFIG_HEAP_MEM_POOL_SIZE > 0 */

static uint8_t *data_alloc(struct net_buf *buf, size_t *size, k_timeout_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->alloc(buf, size, timeout);
}

static uint8_t *data_ref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	return pool->alloc->cb->ref(buf, data);
}

static void data_unref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

	if (buf->flags & NET_BUF_EXTERNAL_DATA) {
		return;
	}

	pool->alloc->cb->unref(buf, data);
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_len_debug(struct net_buf_pool *pool, size_t size,
					k_timeout_t timeout, const char *func,
					int line)
#else
struct net_buf *net_buf_alloc_len(struct net_buf_pool *pool, size_t size,
				  k_timeout_t timeout)
#endif
{
	uint64_t end = os_clock_timeout_end_calc(timeout);
	struct net_buf *buf;
	unsigned int key;

	__ASSERT_NO_MSG(pool);

	NET_BUF_DBG("%s():%d: pool %p size %zu", func, line, pool, size);

	/* We need to lock interrupts temporarily to prevent race conditions
	 * when accessing pool->uninit_count.
	 */
	key = irq_lock();

	/* If there are uninitialized buffers we're guaranteed to succeed
	 * with the allocation one way or another.
	 */
	if (pool->uninit_count) {
		uint16_t uninit_count;

		/* If this is not the first access to the pool, we can
		 * be opportunistic and try to fetch a previously used
		 * buffer from the LIFO with K_NO_WAIT.
		 */
		if (pool->uninit_count < pool->buf_count) {
			buf = k_lifo_get(&pool->free, K_NO_WAIT);
			if (buf) {
				irq_unlock(key);
				goto success;
			}
		}

		uninit_count = pool->uninit_count--;
		irq_unlock(key);

		buf = pool_get_uninit(pool, uninit_count);
		goto success;
	}

	irq_unlock(key);

#if defined(CONFIG_NET_BUF_LOG) && (CONFIG_NET_BUF_LOG_LEVEL >= LOG_LEVEL_WRN)
	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		uint32_t ref = k_uptime_get_32();
		buf = k_lifo_get(&pool->free, K_NO_WAIT);
		while (!buf) {
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s low on buffers.",
				     func, line, pool->name);
#else
			NET_BUF_WARN("%s():%d: Pool %p low on buffers.",
				     func, line, pool);
#endif
			buf = k_lifo_get(&pool->free, WARN_ALLOC_INTERVAL);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
			NET_BUF_WARN("%s():%d: Pool %s blocked for %u secs",
				     func, line, pool->name,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#else
			NET_BUF_WARN("%s():%d: Pool %p blocked for %u secs",
				     func, line, pool,
				     (k_uptime_get_32() - ref) / MSEC_PER_SEC);
#endif
		}
	} else {
		buf = k_lifo_get(&pool->free, timeout);
	}
#else
	buf = k_lifo_get(&pool->free, timeout);
#endif
	if (!buf) {
		NET_BUF_ERR("%s():%d: Failed to get free buffer", func, line);
		return NULL;
	}

success:
	NET_BUF_DBG("allocated buf %p", buf);

	if (size) {
#if __ASSERT_ON
		size_t req_size = size;
#endif
		if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
		    !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			int64_t remaining = end - os_clock_tick_get();

			if (remaining <= 0) {
				timeout = K_NO_WAIT;
			} else {
				timeout = Z_TIMEOUT_TICKS(remaining);
			}
		}

		buf->__buf = data_alloc(buf, &size, timeout);
		if (!buf->__buf) {
			NET_BUF_ERR("%s():%d: Failed to allocate data",
				    func, line);
			net_buf_destroy(buf);
			return NULL;
		}

		NET_BUF_ASSERT(req_size <= size);
	} else {
		buf->__buf = NULL;
	}

	buf->ref   = 1U;
	buf->flags = 0U;
	buf->frags = NULL;
	buf->size  = size;
	net_buf_reset(buf);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	atomic_dec(&pool->avail_count);
/* Actions add start */
	if ((pool->buf_count - pool->avail_count) > pool->max_used) {
		pool->max_used = pool->buf_count - pool->avail_count;
		BUF_LOG("pool(%s), max_used: %d\n", pool->name, pool->max_used);
	}
/* Actions add end */
	__ASSERT_NO_MSG(atomic_get(&pool->avail_count) >= 0);
#endif
	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_fixed_debug(struct net_buf_pool *pool,
					  k_timeout_t timeout, const char *func,
					  int line)
{
/* Actions modify start */
	//const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	return net_buf_alloc_len_debug(pool, pool->alloc->data_size, timeout, func,
				       line);
/* Actions modify end */
}
#else
struct net_buf *net_buf_alloc_fixed(struct net_buf_pool *pool,
				    k_timeout_t timeout)
{
/* Actions modify start */
	//const struct net_buf_pool_fixed *fixed = pool->alloc->alloc_data;

	return net_buf_alloc_len(pool, pool->alloc->data_size, timeout);
/* Actions modify end */
}
#endif

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_alloc_with_data_debug(struct net_buf_pool *pool,
					      void *data, size_t size,
					      k_timeout_t timeout,
					      const char *func, int line)
#else
struct net_buf *net_buf_alloc_with_data(struct net_buf_pool *pool,
					void *data, size_t size,
					k_timeout_t timeout)
#endif
{
	struct net_buf *buf;

#if defined(CONFIG_NET_BUF_LOG)
	buf = net_buf_alloc_len_debug(pool, 0, timeout, func, line);
#else
	buf = net_buf_alloc_len(pool, 0, timeout);
#endif
	if (!buf) {
		return NULL;
	}

	net_buf_simple_init_with_data(&buf->b, data, size);
	buf->flags = NET_BUF_EXTERNAL_DATA;

	return buf;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_get_debug(struct k_fifo *fifo, k_timeout_t timeout,
				  const char *func, int line)
#else
struct net_buf *net_buf_get(struct k_fifo *fifo, k_timeout_t timeout)
#endif
{
	struct net_buf *buf, *frag;

	NET_BUF_DBG("%s():%d: fifo %p", func, line, fifo);

	buf = k_fifo_get(fifo, timeout);
	if (!buf) {
		return NULL;
	}

	NET_BUF_DBG("%s():%d: buf %p fifo %p", func, line, buf, fifo);

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		frag->frags = k_fifo_get(fifo, K_NO_WAIT);
		__ASSERT_NO_MSG(frag->frags);

		/* The fragments flag is only for FIFO-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_simple_init_with_data(struct net_buf_simple *buf,
				   void *data, size_t size)
{
	buf->__buf = data;
	buf->data  = data;
	buf->size  = size;
	buf->len   = size;
}

void net_buf_simple_reserve(struct net_buf_simple *buf, size_t reserve)
{
	__ASSERT_NO_MSG(buf);
	__ASSERT_NO_MSG(buf->len == 0U);
	NET_BUF_DBG("buf %p reserve %zu", buf, reserve);

	buf->data = buf->__buf + reserve;
}

void net_buf_slist_put(sys_slist_t *list, struct net_buf *buf)
{
	struct net_buf *tail;
	unsigned int key;

	__ASSERT_NO_MSG(list);
	__ASSERT_NO_MSG(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

	key = irq_lock();
	sys_slist_append_list(list, &buf->node, &tail->node);
	irq_unlock(key);
}

struct net_buf *net_buf_slist_get(sys_slist_t *list)
{
	struct net_buf *buf, *frag;
	unsigned int key;

	__ASSERT_NO_MSG(list);

	key = irq_lock();
	buf = (void *)sys_slist_get(list);
	irq_unlock(key);

	if (!buf) {
		return NULL;
	}

	/* Get any fragments belonging to this buffer */
	for (frag = buf; (frag->flags & NET_BUF_FRAGS); frag = frag->frags) {
		key = irq_lock();
		frag->frags = (void *)sys_slist_get(list);
		irq_unlock(key);

		__ASSERT_NO_MSG(frag->frags);

		/* The fragments flag is only for list-internal usage */
		frag->flags &= ~NET_BUF_FRAGS;
	}

	/* Mark the end of the fragment list */
	frag->frags = NULL;

	return buf;
}

void net_buf_put(struct k_fifo *fifo, struct net_buf *buf)
{
	struct net_buf *tail;

	__ASSERT_NO_MSG(fifo);
	__ASSERT_NO_MSG(buf);

	for (tail = buf; tail->frags; tail = tail->frags) {
		tail->flags |= NET_BUF_FRAGS;
	}

/* Actions add start */
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	buf->timestamp = k_uptime_get_32();
#endif
/* Actions add end */

	k_fifo_put_list(fifo, buf, tail);
}

#if defined(CONFIG_NET_BUF_LOG)
void net_buf_unref_debug(struct net_buf *buf, const char *func, int line)
#else
void net_buf_unref(struct net_buf *buf)
#endif
{
	__ASSERT_NO_MSG(buf);

	while (buf) {
		struct net_buf *frags = buf->frags;
		struct net_buf_pool *pool;

#if defined(CONFIG_NET_BUF_LOG)
		if (!buf->ref) {
			NET_BUF_ERR("%s():%d: buf %p double free", func, line,
				    buf);
			return;
		}
#endif
		NET_BUF_DBG("buf %p ref %u pool_id %u frags %p", buf, buf->ref,
			    buf->pool_id, buf->frags);

		if (--buf->ref > 0) {
			return;
		}

		if (buf->__buf) {
			data_unref(buf, buf->__buf);
			buf->__buf = NULL;
		}

		buf->data = NULL;
		buf->frags = NULL;

		pool = net_buf_pool_get(buf->pool_id);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		atomic_inc(&pool->avail_count);
		__ASSERT_NO_MSG(atomic_get(&pool->avail_count) <= pool->buf_count);
#endif

		if (pool->destroy) {
			pool->destroy(buf);
		} else {
			net_buf_destroy(buf);
		}

		buf = frags;
	}
}

struct net_buf *net_buf_ref(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf);

	NET_BUF_DBG("buf %p (old) ref %u pool_id %u",
		    buf, buf->ref, buf->pool_id);
	buf->ref++;
	return buf;
}

struct net_buf *net_buf_clone(struct net_buf *buf, k_timeout_t timeout)
{
	int64_t end = os_clock_timeout_end_calc(timeout);
	struct net_buf_pool *pool;
	struct net_buf *clone;

	__ASSERT_NO_MSG(buf);

	pool = net_buf_pool_get(buf->pool_id);

	clone = net_buf_alloc_len(pool, 0, timeout);
	if (!clone) {
		return NULL;
	}

	/* If the pool supports data referencing use that. Otherwise
	 * we need to allocate new data and make a copy.
	 */
	if (pool->alloc->cb->ref && !(buf->flags & NET_BUF_EXTERNAL_DATA)) {
		clone->__buf = data_ref(buf, buf->__buf);
		clone->data = buf->data;
		clone->len = buf->len;
		clone->size = buf->size;
	} else {
		size_t size = buf->size;

		if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
		    !K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			int64_t remaining = end - os_clock_tick_get();

			if (remaining <= 0) {
				timeout = K_NO_WAIT;
			} else {
				timeout = Z_TIMEOUT_TICKS(remaining);
			}
		}

		clone->__buf = data_alloc(clone, &size, timeout);
		if (!clone->__buf || size < buf->size) {
			net_buf_destroy(clone);
			return NULL;
		}

		clone->size = size;
		clone->data = clone->__buf + net_buf_headroom(buf);
		net_buf_add_mem(clone, buf->data, buf->len);
	}

	return clone;
}

struct net_buf *net_buf_frag_last(struct net_buf *buf)
{
	__ASSERT_NO_MSG(buf);

	while (buf->frags) {
		buf = buf->frags;
	}

	return buf;
}

void net_buf_frag_insert(struct net_buf *parent, struct net_buf *frag)
{
	__ASSERT_NO_MSG(parent);
	__ASSERT_NO_MSG(frag);

	if (parent->frags) {
		net_buf_frag_last(frag)->frags = parent->frags;
	}
	/* Take ownership of the fragment reference */
	parent->frags = frag;
}

struct net_buf *net_buf_frag_add(struct net_buf *head, struct net_buf *frag)
{
	__ASSERT_NO_MSG(frag);

	if (!head) {
		return net_buf_ref(frag);
	}

	net_buf_frag_insert(net_buf_frag_last(head), frag);

	return head;
}

#if defined(CONFIG_NET_BUF_LOG)
struct net_buf *net_buf_frag_del_debug(struct net_buf *parent,
				       struct net_buf *frag,
				       const char *func, int line)
#else
struct net_buf *net_buf_frag_del(struct net_buf *parent, struct net_buf *frag)
#endif
{
	struct net_buf *next_frag;

	__ASSERT_NO_MSG(frag);

	if (parent) {
		__ASSERT_NO_MSG(parent->frags);
		__ASSERT_NO_MSG(parent->frags == frag);
		parent->frags = frag->frags;
	}

	next_frag = frag->frags;

	frag->frags = NULL;

#if defined(CONFIG_NET_BUF_LOG)
	net_buf_unref_debug(frag, func, line);
#else
	net_buf_unref(frag);
#endif

	return next_frag;
}

size_t net_buf_linearize(void *dst, size_t dst_len, struct net_buf *src,
			 size_t offset, size_t len)
{
	struct net_buf *frag;
	size_t to_copy;
	size_t copied;

	len = MIN(len, dst_len);

	frag = src;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	copied = 0;
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		memcpy((uint8_t *)dst + copied, frag->data + offset, to_copy);

		copied += to_copy;

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	return copied;
}

/* This helper routine will append multiple bytes, if there is no place for
 * the data in current fragment then create new fragment and add it to
 * the buffer. It assumes that the buffer has at least one fragment.
 */
size_t net_buf_append_bytes(struct net_buf *buf, size_t len,
			    const void *value, k_timeout_t timeout,
			    net_buf_allocator_cb allocate_cb, void *user_data)
{
	struct net_buf *frag = net_buf_frag_last(buf);
	size_t added_len = 0;
	const uint8_t *value8 = value;

	do {
		uint16_t count = MIN(len, net_buf_tailroom(frag));

		net_buf_add_mem(frag, value8, count);
		len -= count;
		added_len += count;
		value8 += count;

		if (len == 0) {
			return added_len;
		}

		if (allocate_cb) {
			frag = allocate_cb(timeout, user_data);
		} else {
			struct net_buf_pool *pool;

			/* Allocate from the original pool if no callback has
			 * been provided.
			 */
			pool = net_buf_pool_get(buf->pool_id);
			frag = net_buf_alloc_len(pool, len, timeout);
		}

		if (!frag) {
			return added_len;
		}

		net_buf_frag_add(buf, frag);
	} while (1);

	/* Unreachable */
	return 0;
}

#if defined(CONFIG_NET_BUF_SIMPLE_LOG)
#define NET_BUF_SIMPLE_DBG(fmt, ...) NET_BUF_DBG(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_ERR(fmt, ...) NET_BUF_ERR(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_WARN(fmt, ...) NET_BUF_WARN(fmt, ##__VA_ARGS__)
#define NET_BUF_SIMPLE_INFO(fmt, ...) NET_BUF_INFO(fmt, ##__VA_ARGS__)
#else
#define NET_BUF_SIMPLE_DBG(fmt, ...)
#define NET_BUF_SIMPLE_ERR(fmt, ...)
#define NET_BUF_SIMPLE_WARN(fmt, ...)
#define NET_BUF_SIMPLE_INFO(fmt, ...)
#endif /* CONFIG_NET_BUF_SIMPLE_LOG */

void net_buf_simple_clone(const struct net_buf_simple *original,
			  struct net_buf_simple *clone)
{
	memcpy(clone, original, sizeof(struct net_buf_simple));
}

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len)
{
	uint8_t *tail = net_buf_simple_tail(buf);

	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	if (net_buf_simple_tailroom(buf) < len) {
		printk("Tailroom %d len %d\n", net_buf_simple_tailroom(buf), len);
	}

	__ASSERT_NO_MSG(net_buf_simple_tailroom(buf) >= len);

	buf->len += len;
	return tail;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem,
			     size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	return memcpy(net_buf_simple_add(buf, len), mem, len);
}

uint8_t *net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *u8;

	NET_BUF_SIMPLE_DBG("buf %p val 0x%02x", buf, val);

	u8 = net_buf_simple_add(buf, 1);
	*u8 = val;

	return u8;
}

void net_buf_simple_add_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le16(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be16(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_le24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le24(val, net_buf_simple_add(buf, 3));
}

void net_buf_simple_add_be24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be24(val, net_buf_simple_add(buf, 3));
}

void net_buf_simple_add_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le32(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be32(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_le48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le48(val, net_buf_simple_add(buf, 6));
}

void net_buf_simple_add_be48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be48(val, net_buf_simple_add(buf, 6));
}

void net_buf_simple_add_le64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le64(val, net_buf_simple_add(buf, sizeof(val)));
}

void net_buf_simple_add_be64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be64(val, net_buf_simple_add(buf, sizeof(val)));
}

void *net_buf_simple_remove_mem(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	return buf->data + buf->len;
}

uint8_t net_buf_simple_remove_u8(struct net_buf_simple *buf)
{
	uint8_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = *(uint8_t *)ptr;

	return val;
}

uint16_t net_buf_simple_remove_le16(struct net_buf_simple *buf)
{
	uint16_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint16_t *)ptr);

	return sys_le16_to_cpu(val);
}

uint16_t net_buf_simple_remove_be16(struct net_buf_simple *buf)
{
	uint16_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint16_t *)ptr);

	return sys_be16_to_cpu(val);
}

uint32_t net_buf_simple_remove_le24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24 : 24;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint24 *)ptr);

	return sys_le24_to_cpu(val.u24);
}

uint32_t net_buf_simple_remove_be24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24 : 24;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint24 *)ptr);

	return sys_be24_to_cpu(val.u24);
}

uint32_t net_buf_simple_remove_le32(struct net_buf_simple *buf)
{
	uint32_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint32_t *)ptr);

	return sys_le32_to_cpu(val);
}

uint32_t net_buf_simple_remove_be32(struct net_buf_simple *buf)
{
	uint32_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint32_t *)ptr);

	return sys_be32_to_cpu(val);
}

uint64_t net_buf_simple_remove_le48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48 : 48;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint48 *)ptr);

	return sys_le48_to_cpu(val.u48);
}

uint64_t net_buf_simple_remove_be48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48 : 48;
	} __packed val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((struct uint48 *)ptr);

	return sys_be48_to_cpu(val.u48);
}

uint64_t net_buf_simple_remove_le64(struct net_buf_simple *buf)
{
	uint64_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint64_t *)ptr);

	return sys_le64_to_cpu(val);
}

uint64_t net_buf_simple_remove_be64(struct net_buf_simple *buf)
{
	uint64_t val;
	void *ptr;

	ptr = net_buf_simple_remove_mem(buf, sizeof(val));
	val = UNALIGNED_GET((uint64_t *)ptr);

	return sys_be64_to_cpu(val);
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	__ASSERT_NO_MSG(net_buf_simple_headroom(buf) >= len);

	buf->data -= len;
	buf->len += len;
	return buf->data;
}

void *net_buf_simple_push_mem(struct net_buf_simple *buf, const void *mem,
			      size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	return memcpy(net_buf_simple_push(buf, len), mem, len);
}

void net_buf_simple_push_le16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le16(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be16(struct net_buf_simple *buf, uint16_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be16(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_u8(struct net_buf_simple *buf, uint8_t val)
{
	uint8_t *data = net_buf_simple_push(buf, 1);

	*data = val;
}

void net_buf_simple_push_le24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le24(val, net_buf_simple_push(buf, 3));
}

void net_buf_simple_push_be24(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be24(val, net_buf_simple_push(buf, 3));
}

void net_buf_simple_push_le32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_le32(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be32(struct net_buf_simple *buf, uint32_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %u", buf, val);

	sys_put_be32(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_le48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le48(val, net_buf_simple_push(buf, 6));
}

void net_buf_simple_push_be48(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be48(val, net_buf_simple_push(buf, 6));
}

void net_buf_simple_push_le64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_le64(val, net_buf_simple_push(buf, sizeof(val)));
}

void net_buf_simple_push_be64(struct net_buf_simple *buf, uint64_t val)
{
	NET_BUF_SIMPLE_DBG("buf %p val %" PRIu64, buf, val);

	sys_put_be64(val, net_buf_simple_push(buf, sizeof(val)));
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len)
{
	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	return buf->data += len;
}

void *net_buf_simple_pull_mem(struct net_buf_simple *buf, size_t len)
{
	void *data = buf->data;

	NET_BUF_SIMPLE_DBG("buf %p len %zu", buf, len);

	__ASSERT_NO_MSG(buf->len >= len);

	buf->len -= len;
	buf->data += len;

	return data;
}

uint8_t net_buf_simple_pull_u8(struct net_buf_simple *buf)
{
	uint8_t val;

	val = buf->data[0];
	net_buf_simple_pull(buf, 1);

	return val;
}

uint16_t net_buf_simple_pull_le16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le16_to_cpu(val);
}

uint16_t net_buf_simple_pull_be16(struct net_buf_simple *buf)
{
	uint16_t val;

	val = UNALIGNED_GET((uint16_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be16_to_cpu(val);
}

uint32_t net_buf_simple_pull_le24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24:24;
	} __packed val;

	val = UNALIGNED_GET((struct uint24 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le24_to_cpu(val.u24);
}

uint32_t net_buf_simple_pull_be24(struct net_buf_simple *buf)
{
	struct uint24 {
		uint32_t u24:24;
	} __packed val;

	val = UNALIGNED_GET((struct uint24 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be24_to_cpu(val.u24);
}

uint32_t net_buf_simple_pull_le32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le32_to_cpu(val);
}

uint32_t net_buf_simple_pull_be32(struct net_buf_simple *buf)
{
	uint32_t val;

	val = UNALIGNED_GET((uint32_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be32_to_cpu(val);
}

uint64_t net_buf_simple_pull_le48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48:48;
	} __packed val;

	val = UNALIGNED_GET((struct uint48 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le48_to_cpu(val.u48);
}

uint64_t net_buf_simple_pull_be48(struct net_buf_simple *buf)
{
	struct uint48 {
		uint64_t u48:48;
	} __packed val;

	val = UNALIGNED_GET((struct uint48 *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be48_to_cpu(val.u48);
}

uint64_t net_buf_simple_pull_le64(struct net_buf_simple *buf)
{
	uint64_t val;

	val = UNALIGNED_GET((uint64_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_le64_to_cpu(val);
}

uint64_t net_buf_simple_pull_be64(struct net_buf_simple *buf)
{
	uint64_t val;

	val = UNALIGNED_GET((uint64_t *)buf->data);
	net_buf_simple_pull(buf, sizeof(val));

	return sys_be64_to_cpu(val);
}

size_t net_buf_simple_headroom(struct net_buf_simple *buf)
{
	return buf->data - buf->__buf;
}

size_t net_buf_simple_tailroom(struct net_buf_simple *buf)
{
	return buf->size - net_buf_simple_headroom(buf) - buf->len;
}


/* Actions add start */

/** Option for continue memory manager */
#ifndef CONFIG_LEGACY_TIMEOUT_API
#define K_TIMEOUT_GREATER(a, b)		((a).ticks > (b).ticks)
#define K_TIMEOUT_SUB(a, b)			K_TICKS(((a).ticks - (b).ticks))
#else
#define K_TIMEOUT_GREATER(a, b)		((a) > (b))
#define K_TIMEOUT_SUB(a, b)			((a) - (b))
#endif

#define DATA_MEM_POOL_ALIGN		(4)
#define DATA_MEM_POOL_USED		BIT(0)
#define MEM_INIT_CHKSUM			(0x5A)

#define DATA_MEM_USED(x)		((x)&DATA_MEM_POOL_USED)
#define DATA_MEM_FREE(x)		(!DATA_MEM_USED(x))

#define DATA_MEM_SET_USED(x) 	do{ \
			(x) = (x) | DATA_MEM_POOL_USED; \
		} while(0)
#define DATA_MEM_SET_FREE(x)	do{ \
			(x) = (x)&(~DATA_MEM_POOL_USED); \
		} while(0)

#define DATA_MEM_SET_CHKSUM(x)	(((uint8_t *)(x))[3] = \
		(((uint8_t *)(x))[0])^(((uint8_t *)(x))[1])^(((uint8_t *)(x))[2])^MEM_INIT_CHKSUM)
#define DATA_MEM_CHKSUM_CORRECT(x) ((((uint8_t *)(x))[3]) == \
		((((uint8_t *)(x))[0])^(((uint8_t *)(x))[1])^(((uint8_t *)(x))[2])^MEM_INIT_CHKSUM))

#define MEM_MALLOC_CHECK_INTERVAL	(K_MSEC(5))		/* 5ms */

/* Continue memory manager struct */
struct pool_data_info {
	uint16_t len;				/* len include data info */
	uint8_t flag;
	uint8_t chksum;
};

#define DATA_INFO_SIZE		sizeof(struct pool_data_info)

static void net_data_continue_pool_init(void *data_pool)
{
	struct net_buf_pool_continue *pool = data_pool;
	struct pool_data_info *info;

	pool->alloc_pos = pool->data_pool;
	info = (struct pool_data_info *)pool->alloc_pos;

	memset(info, 0, DATA_INFO_SIZE);
	info->len = pool->data_size;
	DATA_MEM_SET_FREE(info->flag);
	DATA_MEM_SET_CHKSUM(info);

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	pool->curr_used = 0;
	pool->max_used = 0;
#endif
}

#define POOL_DEBUG_TEST		1

static uint8_t *continue_pool_malloc(struct net_buf_pool_continue *data_pool, uint16_t malloc_size)
{
	unsigned int key;
	uint8_t *sPos, *nPos, *mallocPos = NULL;
	struct pool_data_info *sInfo, *nInfo;
	bool merge;

	key = irq_lock();
	sPos = data_pool->alloc_pos;

#if POOL_DEBUG_TEST
	uint16_t debug_run_times = 0;
	uint8_t *debug_start_pos = sPos;
#endif

	do {
		merge = false;
		sInfo = (struct pool_data_info *)sPos;
		BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(sInfo), "chksum error");

#if POOL_DEBUG_TEST
		debug_run_times++;
		if (debug_run_times > 100) {
			printk("pool_malloc len %d %p %d\n", malloc_size, data_pool->data_pool, data_pool->data_size);
			printk("Start pos %p curr %p flag 0x%x len %d\n", debug_start_pos, sInfo, sInfo->flag, sInfo->len);
		}
#endif

		if (DATA_MEM_USED(sInfo->flag)) {
			sPos += sInfo->len;
			BUF_ASSERT(sPos <= (data_pool->data_pool + data_pool->data_size), "address error");

			if (sPos == (data_pool->data_pool + data_pool->data_size)) {
				sPos = data_pool->data_pool;
			}
#if POOL_DEBUG_TEST
			if (debug_run_times > 100) {
				printk("debug1 %p %p %d\n", sPos, data_pool->alloc_pos, merge);
			}
#endif
			continue;
		}

		if (sInfo->len >= malloc_size) {
			mallocPos = sPos;
			break;
		} else {
			nPos = sPos + sInfo->len;
			BUF_ASSERT(nPos <= (data_pool->data_pool + data_pool->data_size), "address error");

			if (nPos == (data_pool->data_pool + data_pool->data_size)) {
				sPos = data_pool->data_pool;
#if POOL_DEBUG_TEST
				if (debug_run_times > 100) {
					printk("debug2 %p %p %d\n", sPos, data_pool->alloc_pos, merge);
				}
#endif
				continue;
			} else {
				nInfo = (struct pool_data_info *)nPos;
				BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(nInfo), "chksum error");

				if (DATA_MEM_USED(nInfo->flag)) {
					sPos = nPos;
#if POOL_DEBUG_TEST
					if (debug_run_times > 100) {
						printk("debug3 %p %p %d\n", sPos, data_pool->alloc_pos, merge);
					}
#endif
					continue;
				} else {
					sInfo->len += nInfo->len;
					DATA_MEM_SET_CHKSUM(sInfo);
					memset(nInfo, 0, DATA_INFO_SIZE);
					merge = true;

					if (data_pool->alloc_pos == (uint8_t *)nInfo) {
						data_pool->alloc_pos = (uint8_t *)sInfo;
					}
#if POOL_DEBUG_TEST
					if (debug_run_times > 100) {
						printk("debug4 %p %p %d\n", sPos, data_pool->alloc_pos, merge);
					}
#endif
					continue;
				}
			}
		}
	} while ((sPos != data_pool->alloc_pos) || merge);

	if (mallocPos) {
		sInfo = (struct pool_data_info *)mallocPos;
		if (sInfo->len > malloc_size) {
			nInfo = (struct pool_data_info *)(mallocPos + malloc_size);
			memset(nInfo, 0, DATA_INFO_SIZE);
			nInfo->len = sInfo->len - malloc_size;
			DATA_MEM_SET_FREE(nInfo->flag);
			DATA_MEM_SET_CHKSUM(nInfo);
		}

		if ((mallocPos + malloc_size) == (data_pool->data_pool + data_pool->data_size)) {
			data_pool->alloc_pos = data_pool->data_pool;
		} else {
			data_pool->alloc_pos = mallocPos + malloc_size;
		}

		sInfo->len = malloc_size;
		DATA_MEM_SET_USED(sInfo->flag);
		DATA_MEM_SET_CHKSUM(sInfo);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
		data_pool->curr_used += malloc_size;
		if (data_pool->curr_used > data_pool->max_used) {
			data_pool->max_used = data_pool->curr_used;
			BUF_LOG("data pool(%p), max_used: %d\n", data_pool, data_pool->max_used);
			BUF_ASSERT((data_pool->max_used <= data_pool->data_size), "Error max used");
		}
#endif
	}

	irq_unlock(key);

	if (mallocPos) {
		return (mallocPos + DATA_INFO_SIZE);
	} else {
		return NULL;
	}
}

static void continue_pool_free(struct net_buf_pool_continue *data_pool, uint8_t *pData)
{
	unsigned int key;
	struct pool_data_info *info = (struct pool_data_info *)(pData - DATA_INFO_SIZE);

	BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(info), "chksum error");

	key = irq_lock();

	DATA_MEM_SET_FREE(info->flag);
	DATA_MEM_SET_CHKSUM(info);
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	data_pool->curr_used -= info->len;
#endif

	irq_unlock(key);
}

#if BUF_TEST_DEBUG
static void dump_continue_pool(struct net_buf_pool_continue *pool, uint16_t malloc_size)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	uint8_t *pos;
	uint16_t cal_used_len = 0;
	struct pool_data_info *info;

	k_sched_lock();

	BUF_LOG("%s\n", __func__);
	BUF_LOG("data_pool: %p~%p, malloc_size: %d\n", pool->data_pool,
			(pool->data_pool + pool->data_size), malloc_size);
	BUF_LOG("size: %d, curr_used: %d, max_used: %d, alloc_pos: %p\n",
			pool->data_size, pool->curr_used, pool->max_used, pool->alloc_pos);

	pos = pool->data_pool;
	BUF_LOG("address\t len\t flag\n");
	while (pos < (pool->data_pool + pool->data_size)) {
		info = (struct pool_data_info *)pos;
		BUF_ASSERT(DATA_MEM_CHKSUM_CORRECT(info), "chksum error");
		BUF_LOG("%p\t %d\t %d\n", info, info->len, info->flag);
		if (DATA_MEM_USED(info->flag)) {
			cal_used_len += info->len;
		}

		pos += info->len;
	}

	BUF_LOG("cal_used_len: %d\n", cal_used_len);

	k_sched_unlock();
#endif
}
#endif

static uint8_t *net_data_continue_pool_malloc(struct net_buf_pool_continue *data_pool, uint16_t size, k_timeout_t timeout)
{
	uint8_t *pData;
	uint16_t malloc_size;
	k_timeout_t sleep_time, remain_time = timeout;

	BUF_ASSERT(data_pool, "data pool NULL");

	malloc_size = size + DATA_INFO_SIZE;
	malloc_size = ROUND_UP(malloc_size, DATA_MEM_POOL_ALIGN);

try_again:
	pData = continue_pool_malloc(data_pool, malloc_size);
	if (pData) {
		return pData;
	}

#if BUF_TEST_DEBUG
	dump_continue_pool(data_pool, malloc_size);
#endif
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return pData;
	} else if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		k_sleep(MEM_MALLOC_CHECK_INTERVAL);
		goto try_again;
	} else if (!K_TIMEOUT_EQ(remain_time, K_NO_WAIT)) {
		sleep_time = K_TIMEOUT_GREATER(remain_time, MEM_MALLOC_CHECK_INTERVAL)? MEM_MALLOC_CHECK_INTERVAL : remain_time;
		k_sleep(sleep_time);
		remain_time = K_TIMEOUT_SUB(remain_time, sleep_time);
		goto try_again;
	} else {
		return pData;
	}
}

static void net_data_continue_pool_free(struct net_buf_pool_continue *data_pool, uint8_t *data)
{
	BUF_ASSERT(data, "buf->__buf NULL");
	BUF_ASSERT(data_pool, "data pool NULL");

	continue_pool_free(data_pool, data);
}

static uint8_t *continue_pool_data_alloc(struct net_buf *buf, size_t *size,
				 k_timeout_t timeout)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	struct net_buf_pool_continue *data_pool = pool->alloc->alloc_data;
	uint8_t *ref_count;

	if (!data_pool->init_flag) {
		net_data_continue_pool_init(data_pool);
		data_pool->init_flag = 1;
	}

	ref_count = net_data_continue_pool_malloc(data_pool, (*size + 1), timeout);
	if (ref_count == NULL) {
		return NULL;
	}

	*ref_count = 1U;

	/* Return pointer to the byte following the ref count */
	return ref_count + 1;
}

static void continue_pool_data_unref(struct net_buf *buf, uint8_t *data)
{
	struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);
	struct net_buf_pool_continue *data_pool = pool->alloc->alloc_data;
	uint8_t *ref_count;

	ref_count = data - 1;
	if (--(*ref_count)) {
		return;
	}

	net_data_continue_pool_free(data_pool, ref_count);
}

const struct net_buf_data_cb net_buf_continue_cb = {
	.alloc = continue_pool_data_alloc,
	.ref   = generic_data_ref,
	.unref = continue_pool_data_unref,
};

void net_buf_dumpinfo(void)
{
#if defined(CONFIG_NET_BUF_POOL_USAGE)
	/* Need add _net_buf_pool_list_end to zephyr\include\linker\common-ram.ld */
	extern struct net_buf_pool _net_buf_pool_list_end;
	struct net_buf_pool *pool;
	struct net_buf_pool_continue *data_pool;

	printk("Net pool info\n");
	printk("\t addr\t count\t avail\t max_used\t name\t use_data_pool\n");
	for (pool = &_net_buf_pool_list[0];
		pool < &_net_buf_pool_list_end; pool++) {
		printk("pool: %p\t %d\t %d\t %d\t %s\n", pool, pool->buf_count,
			pool->avail_count, pool->max_used, pool->name);
		if (pool->alloc->cb == &net_buf_continue_cb) {
			data_pool = pool->alloc->alloc_data;
			printk("data_pool: %p\t %d\t %d\t\t %d\n", data_pool,
				data_pool->data_size, data_pool->curr_used, data_pool->max_used);
		}
	}
#else
	printk("CONFIG_NET_BUF_POOL_USAGE not set\n");
#endif
}

/* Actions add end */
