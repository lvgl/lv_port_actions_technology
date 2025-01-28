/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARITHMETIC_STORAGE_IO_H__
#define __ARITHMETIC_STORAGE_IO_H__

#include "al_storage_io.h"

storage_io_t *storage_io_block_stream(void *stream);

/**
 * @brief wrap storage io from an opened io stream
 *
 * This routine provides wrapping storage io from an opened io stream
 *
 * @param stream handle of io stream
 *
 * @return address of storage io
 */
storage_io_t *storage_io_wrap_stream(void *stream);

/**
 * @brief unwrap storage io from a stream
 *
 * This routine provides unwrap storage io
 *
 * @param io address of storage io
 *
 * @return N/A
 */
void storage_io_unwrap_stream(storage_io_t *io);

#endif /* __ARITHMETIC_STORAGE_IO_H__ */
