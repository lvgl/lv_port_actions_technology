/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file file stream interface
 */

#ifndef __LOOP_FSTREAM_H__
#define __LOOP_FSTREAM_H__

#include <stream.h>

#ifdef CONFIG_FILE_SYSTEM
#include <fs/fs.h>
#endif

/**
 * @defgroup loop_fstream_apis Loop Fstream APIs
 * @ingroup stream_apis, shared structures
 * @{
 */
/**
 * loop fstream is a stream very similar to file stream, but read only.
 * It has a extremly big length by repeating the true file many many times.
 */
/**
 * @brief create loop fstream , return stream handle
 *
 * This routine provides create stream ,and return stream handle.
 * and stream state is  STATE_INIT
 *
 * @param param create stream param, loop fstream is file url
 *
 * @return stream handle if create stream success
 * @return NULL  if create stream failed
 */
 io_stream_t loop_fstream_create(const char *param);

/**
 * @brief get loop fstream info
 *
 * This routine allows app who uses this stream to set some file info or special ops,
 * which are alternative but can help to improve experience.
 */
void loop_fstream_get_info(void *info, uint8_t info_id);

/* loop fstream info_id to get info from app  */
#define LOOPFS_FILE_INFO		(1)
#define LOOPFS_CALLBACK			(2)

/* get file head&tail info */
struct loopfs_file_info {
	uint32_t head;
	uint32_t tail;
	int bitrate;
};

/* get callback func */
typedef void (*loopfs_energy_cb)(uint8_t energy_enable);


/**
 * @} end defgroup loop_stream_apis
 */

#endif /* __LOOP_STREAM_H__ */


