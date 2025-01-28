/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ringbuffer stream interface
 */

#ifndef __RINGBUFF_STREAM_H__
#define __RINGBUFF_STREAM_H__

#include <stream.h>
#include <acts_ringbuf.h>

/**
 * @defgroup buffer_stream_apis Buffer Stream APIs
 * @ingroup stream_apis
 * @{
 */

/**
 * @brief create ring buffer stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream parama
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t ringbuff_stream_create(struct acts_ringbuf *param);

/**
 * @brief create ring buffer stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param ring_buff ring buffer base addr
 * @param ring_buff_size ring buffer size
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t ringbuff_stream_create_ext(void *ring_buff, uint32_t ring_buff_size);

/**
 * @} end defgroup buffer_stream_apis
 */

#endif /* __RINGBUFF_STREAM_H__ */
