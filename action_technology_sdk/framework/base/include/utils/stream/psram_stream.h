/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file psram stream interface
 */

#ifndef __PSRAM_STREAM_H__
#define __PSRAM_STREAM_H__

#include <stream.h>

#ifdef CONFIG_PSRAM
#include <psram.h>
#endif

/**
 * @defgroup psram_stream_apis Psram Stream APIs
 * @ingroup stream_apis
 * @{
 */

/** structure of psram stream info,
 * used to pass parama to psram stream
 */
struct psram_info_t {
	/** size of cache*/
	int size;
	/** name of cache*/
	char *name;
	/** wait size of cache*/
	int min_size;
	/** write must wait free space*/
	bool write_pending;
};

io_stream_t psram_stream_create(struct psram_info_t *psram_info);

/**
 * @} end defgroup psram_stream_apis
 */ 

#endif /* __PSRAM_STREAM_H__ */
