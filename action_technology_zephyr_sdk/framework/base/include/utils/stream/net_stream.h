/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file net stream interface
 */

#ifndef __NET_STREAM_H__
#define __NET_STREAM_H__

#include <stream.h>
#include "http_client.h"
#include "tcp_client.h"

/**
 * @defgroup net_stream_apis Net Stream APIs
 * @ingroup stream_apis
 * @{
 */

/**
 * @brief create net stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream parama
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t net_stream_create(struct request_info *param);

/**
 * @} end defgroup net_stream_apis
 */

#endif /* __NET_STREAM_H__ */
