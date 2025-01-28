/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file buffer stream interface
 */

#ifndef __BUFFER_STREAM_H__
#define __BUFFER_STREAM_H__

#include <stream.h>

/**
 * @defgroup buffer_stream_apis Buffer Stream APIs
 * @ingroup stream_apis
 * @{
 */
/** structure of buffer,
  * used to pass parama to buffer stream
  */
struct buffer_t
{
	/** lenght of buffer*/
	int length;
	/** pointer to base of buffer*/
	char * base;
	/** cache size */
	int cache_size;
};

/**
 * @brief create buffer stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream parama
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t buffer_stream_create(struct buffer_t *param);

/**
 * @} end defgroup buffer_stream_apis
 */

#endif /* __BUFFER_STREAM_H__ */
