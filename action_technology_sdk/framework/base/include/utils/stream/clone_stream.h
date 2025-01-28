/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clone stream interface
 */

#ifndef __CLONE_STREAM_H__
#define __CLONE_STREAM_H__

#include <stream.h>

/**
 * @defgroup clone_stream_apis Clone Stream APIs
 * @ingroup stream_apis
 * @{

/** structure of clone stream info,
 * used to pass parama to clone stream
 */
struct clone_stream_info {
	/* stream to be cloned */
	io_stream_t origin;

	/* clone direction (MODE_IN, MODE_OUT) : origin stream data read or write */
	int clone_mode;
	/* stream array that will be cloned to */
	io_stream_t clones[1];
};

io_stream_t clone_stream_create(struct clone_stream_info *info);

/**
 * @} end defgroup clone_stream_apis
 */

#endif /* __CLONE_STREAM_H__ */
