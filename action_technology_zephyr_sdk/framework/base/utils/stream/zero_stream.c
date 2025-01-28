/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
 * @file zero stream interface
 */

#include <string.h>
#include <stdint.h>
#include "zero_stream.h"
#include "stream_internal.h"

static int _zero_stream_open(io_stream_t handle,stream_mode mode)
{
	handle->total_size = UINT32_MAX;
	return 0;
}

static int _zero_stream_read(io_stream_t handle, unsigned char *buf, int len)
{
	memset(buf, 0, len);
	handle->rofs += len;
	return 0;
}

static int _zero_stream_write(io_stream_t handle, unsigned char *buf, int len)
{
	handle->wofs += len;
	return len;
}

static int _zero_stream_get_length(io_stream_t handle)
{
	return INT32_MAX;
}

static int _zero_stream_get_space(io_stream_t handle)
{
	return INT32_MAX;
}

static int _zero_stream_close(io_stream_t handle)
{
	return 0;
}

static const stream_ops_t zero_stream_ops = {
	.open = _zero_stream_open,
	.read = _zero_stream_read,
	.write = _zero_stream_write,
	.close = _zero_stream_close,
	.get_length = _zero_stream_get_length,
	.get_space = _zero_stream_get_space,
};

io_stream_t zero_stream_create(void)
{
	return stream_create(&zero_stream_ops, NULL);
}
