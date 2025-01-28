/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mem_manager.h>
#include "dsp_inner.h"

#define MAX_SESSION_BUFF_NUM 20
static struct dsp_session_buf buff_pool[MAX_SESSION_BUFF_NUM] __in_section_unique(DSP_SHARE_RAM);
static int bit_mask = 0;

static struct dsp_session_buf * malloc_session_buf(int size)
{
	for (int i = 0 ; i < MAX_SESSION_BUFF_NUM; i++) {
		if (!(bit_mask & (1 << i))) {
			bit_mask |= (1 << i);
			memset(&buff_pool[i], 0, sizeof(struct dsp_session_buf));
			return &buff_pool[i];
		}
	}
	printk("session malloc failed\n");
	return NULL;
}

static void free_session_buf(struct dsp_session_buf * session_buf)
{
	for (int i = 0 ; i < MAX_SESSION_BUFF_NUM; i++) {
		if (&buff_pool[i] == session_buf) {
			bit_mask &= (~(1 << i));
			return;
		}
	}
}

struct dsp_session_buf *dsp_session_buf_init(struct dsp_session *session,
					     void *data, unsigned int size)
{
	struct dsp_session_buf *buf = malloc_session_buf(sizeof(*buf));
	if (buf == NULL)
		return NULL;

	acts_ringbuf_init(&buf->buf, data, ACTS_RINGBUF_NELEM(size));
#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	buf->session = session;
#endif
	return buf;
}

void dsp_session_buf_destroy(struct dsp_session_buf *buf)
{
	if (buf)
		free_session_buf(buf);
}

struct dsp_session_buf *dsp_session_buf_alloc(struct dsp_session *session, unsigned int size)
{
	struct dsp_session_buf *buf = malloc_session_buf(sizeof(*buf));
	if (buf == NULL)
		return NULL;

	void *data = mem_malloc(size);
	if (data == NULL) {
		mem_free(buf);
		return NULL;
	}

	acts_ringbuf_init(&buf->buf, data, ACTS_RINGBUF_NELEM(size));
#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	buf->session = session;
#endif
	return buf;
}

void dsp_session_buf_free(struct dsp_session_buf *buf)
{
	if (buf) {
		mem_free((void *)(buf->buf.cpu_ptr));
		free_session_buf(buf);
	}
}

void dsp_session_buf_reset(struct dsp_session_buf *buf)
{
	acts_ringbuf_reset(&buf->buf);
}

unsigned int dsp_session_buf_size(struct dsp_session_buf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_size(&buf->buf));
}

unsigned int dsp_session_buf_space(struct dsp_session_buf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_space(&buf->buf));
}

unsigned int dsp_session_buf_length(struct dsp_session_buf *buf)
{
	return ACTS_RINGBUF_SIZE8(acts_ringbuf_length(&buf->buf));
}

int dsp_session_buf_read(struct dsp_session_buf *buf, void *data, unsigned int size)
{
	int len = ACTS_RINGBUF_SIZE8(acts_ringbuf_get(&buf->buf, data, ACTS_RINGBUF_NELEM(size)));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next read the same size */
	//if (!k_is_in_isr() && dsp_session_buf_length(buf) < size) {
	//	if (!dsp_session_kick(buf->session))
	//		SYS_LOG_DBG("kick %u (%u ms)", dsp_session_buf_length(buf), k_uptime_get_32());
	//}
#endif

	return len;
}

int dsp_session_buf_write(struct dsp_session_buf *buf, const void *data, unsigned int size)
{
#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	bool empty = acts_ringbuf_is_empty(&buf->buf);
#endif

	int len = ACTS_RINGBUF_SIZE8(acts_ringbuf_put(&buf->buf, data, ACTS_RINGBUF_NELEM(size)));

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next write the same size */
	if (!k_is_in_isr() && (empty || dsp_session_buf_space(buf) < size)) {
		if (!dsp_session_kick(buf->session))
			SYS_LOG_DBG("kick %u (%u ms)", dsp_session_buf_space(buf), k_uptime_get_32());
	}
#endif

	return len;
}

int dsp_session_buf_read_to_buffer(struct dsp_session_buf *buf, void *buffer, unsigned int size)
{
	int len = ACTS_RINGBUF_SIZE8(
			acts_ringbuf_read(&buf->buf, buffer, ACTS_RINGBUF_NELEM(size),
					NULL));

    return len;
}

int dsp_session_buf_read_to_stream(struct dsp_session_buf *buf,
		void *stream, unsigned int size,
		dsp_session_buf_write_fn stream_write)
{
	int len = ACTS_RINGBUF_SIZE8(
			acts_ringbuf_read(&buf->buf, stream, ACTS_RINGBUF_NELEM(size),
					(acts_ringbuf_write_fn)stream_write));

	//printk("%s, read len 0x%x from dsp outbuf to outstream!\n\n", __func__, len);

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	/* Supposed next read the same size */
	//if (!k_is_in_isr() && dsp_session_buf_length(buf) < size) {
	//	if (!dsp_session_kick(buf->session))
	//		SYS_LOG_DBG("kick %u (%u ms)", dsp_session_buf_length(buf), k_uptime_get_32());
	//}
#endif

	return len;
}

int dsp_session_buf_write_from_buffer(struct dsp_session_buf *buf, void *buffer, unsigned int size)
{
    int len = ACTS_RINGBUF_SIZE8(
            acts_ringbuf_write(&buf->buf, buffer, ACTS_RINGBUF_NELEM(size),
                    NULL));

	//printk("%s: %d: write to dsp session in_buf: %p,  len: 0x%x\n\n", __func__, __LINE__, buffer, len);

    return len;
}

int dsp_session_buf_write_from_stream(struct dsp_session_buf *buf,
		void *stream, unsigned int size,
		dsp_session_buf_read_fn stream_read)
{
#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0
	bool empty = acts_ringbuf_is_empty(&buf->buf);
#endif

	int len = ACTS_RINGBUF_SIZE8(
			acts_ringbuf_write(&buf->buf, stream, ACTS_RINGBUF_NELEM(size),
					(acts_ringbuf_read_fn)stream_read));

	//printk("%s, dsp session inbuf write 0x%x finish!\n\n", __func__, len);

#if CONFIG_DSP_ACTIVE_POWER_LATENCY_MS >= 0

	/* Supposed next write the same size */
	if (!k_is_in_isr() && (empty || dsp_session_buf_space(buf) < size)) {
		if (!dsp_session_kick(buf->session))
			SYS_LOG_DBG("kick %u (%u ms)", dsp_session_buf_space(buf), k_uptime_get_32());
	}
#endif

	return len;
}

size_t dsp_session_buf_drop_all(struct dsp_session_buf *buf)
{
	return acts_ringbuf_drop_all(&buf->buf);
}

