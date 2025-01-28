/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CRC utils
 */

#ifndef __UTILS_CRC_H__
#define __UTILS_CRC_H__

uint16_t utils_crc16(const uint8_t *src, int len, uint16_t polynomial,
		     uint16_t initial_value, int pad);

uint32_t utils_crc32(uint32_t crc, const uint8_t *buf, int len);

#endif /* __UTILS_CRC_H__ */
