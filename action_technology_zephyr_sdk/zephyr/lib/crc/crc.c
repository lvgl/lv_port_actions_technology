/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file crc interface
 */

#include <stdint.h>
#include <crc.h>

uint16_t utils_crc16(const uint8_t *src, int len, uint16_t polynomial,
		     uint16_t initial_value, int pad)
{
    uint16_t crc = initial_value;
    int padding = (pad != 0) ? sizeof(crc) : 0;
    int i, b;

    /* src length + padding (if required) */
    for (i = 0; i < len + padding; i++) {

        for (b = 0; b < 8; b++) {
            uint16_t divide = crc & 0x8000;

            crc = (crc << 1);

            /* choose input bytes or implicit trailing zeros */
            if (i < len) {
                crc |= !!(src[i] & (0x80 >> b));
            }

            if (divide) {
                crc = crc ^ polynomial;
            }
        }
    }

    return crc;
}

static const uint32_t s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158,
        0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };

// Karl Malbrain's compact CRC-32. See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": http://www.geocities.com/malbrain/
uint32_t utils_crc32(uint32_t crc, const uint8_t *ptr, int buf_len)
{
    uint32_t crcu32 = crc;
    crcu32 = ~crcu32;
    while (buf_len--) {
        uint8_t b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }
    return ~crcu32;
}

