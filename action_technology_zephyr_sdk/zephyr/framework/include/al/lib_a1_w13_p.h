/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __K1_WMA_P_H__
#define __K1_WMA_P_H__

#ifdef __cplusplus
extern "C" {
#endif

/*decoder need inital package*/
typedef struct
{
	void *inbuf; //input ringbuf
	void *cbbuf; //seek ringbuf
	int32_t time_offset; //break point (ms)
	int32_t file_len; //file len of this wma file
} parser_wma_t;

#ifdef __cplusplus
}
#endif

#endif /* __K1_WMA_P_H__ */
