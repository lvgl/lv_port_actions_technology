/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zero stream interface
 */

#ifndef __ZERO_STREAM_H__
#define __ZERO_STREAM_H__

#include <stream.h>

/**
 * @defgroup zero_stream_apis Buffer Stream APIs
 * @ingroup stream_apis
 * @{
 */

/**
 * @brief create zero stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is STATE_INIT.
 *
 * This stream always return zero data for stream_read, and always ignore
 * data for stream_write.
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t zero_stream_create(void);

/**
 * @} end defgroup zero_stream_apis
 */

#endif /* __ZERO_STREAM_H__ */
