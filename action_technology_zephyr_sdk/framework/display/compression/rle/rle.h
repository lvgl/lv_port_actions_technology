/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMPRESSION_RLE_H__
#define __COMPRESSION_RLE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RLE encode
 *
 * @param out_buf pointer to encoded output buffer
 * @param out_size size of output buffer in bytes
 * @param in_buf pointer to input buffer
 * @param in_count size of input buffer in elements
 * @param size size of each element in bytes
 *
 * @retval number of bytes actually encoded if out_buf is not NULL
 * @retval total encoded bytes if out_buf is NULL
 */

int rle_compress(const uint8_t* in_buf, uint8_t * out_buf,  size_t in_count, size_t out_size, size_t size);

/**
 * @brief RLE decode
 *
 * @param out_buf pointer to output buffer
 * @param out_count size of output buffer in elements
 * @param in_buf pointer to encoded input buffer
 * @param in_size size of input buffer in bytes
 * @param size size of each element in bytes
 *
 * @retval number of bytes actually decoded if out_buf is not NULL
 * @retval total decoded bytes if out_buf is NULL
 */

int rle_decompress(const uint8_t * in_buf, uint8_t * out_buf, int in_size, int out_count, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __COMPRESSION_RLE_H__ */
