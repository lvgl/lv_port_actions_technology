/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file file stream interface
 */

#ifndef __FILE_STREAM_H__
#define __FILE_STREAM_H__

#include <stream.h>

#ifdef CONFIG_FILE_SYSTEM
#include <fs/fs.h>
#endif

/**
 * @defgroup file_stream_apis File Stream APIs
 * @ingroup stream_apis
 * @{
 */
/**
 * @brief create file stream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream param, file stream is file url
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
 io_stream_t file_stream_create(const char *param);


typedef struct {
    int file_count;
    void *usr_data;
    void (* get_file_name)(void *usr_data, int index, char *file_name, int file_name_buffer_size);
}multi_file_info_t;

/**
 * @brief create multi file stream, return stream handle
 *
 * This routine provides create a multi file stream that pretend multi files as one file, and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream param
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
io_stream_t multi_file_stream_create(multi_file_info_t *param);

/**
 * @} end defgroup file_stream_apis
 */

#endif /* __FILE_STREAM_H__ */


