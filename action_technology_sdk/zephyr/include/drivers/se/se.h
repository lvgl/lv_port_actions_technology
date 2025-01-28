/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file security engine for Actions SoC
 */

#ifndef SE_H_
#define SE_H_

#include <kernel.h>

#define CPY_MEM_TO_FIFO             0
#define CPY_FIFO_TO_MEM             1
#define CPY_MEM_TO_MEM              2
#define CPY_MEMU8_TO_FIFO           3

/* AES Macro */
#define AES_MODE_ECB                0
#define AES_MODE_CTR                1
#define AES_MODE_CBC                2
#define AES_MODE_CBC_CTS            3
#define CYPHER_ENCRYPT              1
#define CYPHER_DECRYPT              0

extern struct k_mutex se_lock;
extern void se_memcpy(void *out, const void *in,
                      size_t size, uint8_t direct);
extern int aes_cypher(void *out, const void *in, size_t size,
                      const void *iv, const void *key, size_t keysize,
                      int mode, int encrypt);
extern uint32_t utils_crc32(uint32_t crc, const uint8_t *ptr, int buf_len);
extern uint32_t crc32_mpeg2(uint32_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint32_t crc32(uint32_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_xmodem(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_x5(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_ccitt_false(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_ccitt(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_modbus(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_usb(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_maxim(uint16_t inital_val, const unsigned char* ptr, unsigned int len);
extern uint16_t crc16_ibm(uint16_t inital_val, const unsigned char* ptr, unsigned int len);

#endif /* SE_H_ */
