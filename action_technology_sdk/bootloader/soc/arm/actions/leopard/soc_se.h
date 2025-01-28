/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file security engine for Actions SoC
 */

#ifndef SOC_SE_H_
#define SOC_SE_H_

//#include <device.h>
//#include <drivers/dma.h>

#define AES_CTRL          (SE_REG_BASE+0x0000)
#define AES_MODE          (SE_REG_BASE+0x0004)
#define AES_LEN           (SE_REG_BASE+0x0008)
#define AES_KEY0          (SE_REG_BASE+0x0010)
#define AES_KEY1          (SE_REG_BASE+0x0014)
#define AES_KEY2          (SE_REG_BASE+0x0018)
#define AES_KEY3          (SE_REG_BASE+0x001c)
#define AES_KEY4          (SE_REG_BASE+0x0020)
#define AES_KEY5          (SE_REG_BASE+0x0024)
#define AES_KEY6          (SE_REG_BASE+0x0028)
#define AES_KEY7          (SE_REG_BASE+0x002c)
#define AES_IV0           (SE_REG_BASE+0x0040)
#define AES_IV1           (SE_REG_BASE+0x0044)
#define AES_IV2           (SE_REG_BASE+0x0048)
#define AES_IV3           (SE_REG_BASE+0x004c)
#define AES_INOUTFIFOCTL  (SE_REG_BASE+0x0080)
#define AES_INFIFO        (SE_REG_BASE+0x0084)
#define AES_OUTFIFO       (SE_REG_BASE+0x0088)

#define TRNG_CTRL         (SE_REG_BASE+0x0400)
#define TRNG_LR           (SE_REG_BASE+0x0404)
#define TRNG_MR           (SE_REG_BASE+0x0408)
#define TRNG_ILR          (SE_REG_BASE+0x040c)
#define TRNG_IMR          (SE_REG_BASE+0x0410)

#define CRC_CTRL          (SE_REG_BASE+0x0600)
#define CRC_MODE          (SE_REG_BASE+0x0604)
#define CRC_LEN           (SE_REG_BASE+0x0608)
#define CRC_INFIFOCTL     (SE_REG_BASE+0x060c)
#define CRC_INFIFO        (SE_REG_BASE+0x0610)
#define CRC_DATAOUT       (SE_REG_BASE+0x0614)
#define CRC_DATAINIT      (SE_REG_BASE+0x0618)
#define CRC_DATAOUTXOR    (SE_REG_BASE+0x061c)
#define CRC_DEBUGOUT      (SE_REG_BASE+0x0630)

#define SE_FIFOCTRL       (SE_REG_BASE+0x0800)

struct acts_se_data {
	void *dma_dev;
	uint32_t dma_chan;
	uint32_t dma_chan_out; /*only aes need*/
};

#if 0
int se_aes_init(void);
int se_aes_deinit(void);
uint32_t se_aes_process(int mode, uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf);
#endif

int se_trng_init(void);
int se_trng_deinit(void);
uint32_t se_trng_process(uint32_t *trng_low, uint32_t *trng_high);

#if 0
int se_crc_init(void);
int se_crc_deinit(void);

#define CRC32_MODE_CRC32         0
#define CRC32_MODE_MPEG2         1
#define CONFIG_CRC32_MODE        CRC32_MODE_CRC32
uint32_t se_crc32_process(uint8_t *buffer, uint32_t buf_len, uint32_t crc_initial, bool last);
uint32_t se_crc32(uint8_t *buffer, uint32_t buf_len);

#define CRC16_MODE_CCITT         0
#define CRC16_MODE_CCITT_FALSE   1
#define CRC16_MODE_XMODEM        2
#define CRC16_MODE_X25           3
#define CRC16_MODE_MODBUS        4
#define CRC16_MODE_IBM           5
#define CRC16_MODE_MAXIM         6
#define CRC16_MODE_USB           7
#define CONFIG_CRC16_MODE        CRC16_MODE_CCITT
uint16_t se_crc16_process(uint8_t *buffer, uint32_t buf_len, uint16_t crc_initial, bool last);
uint16_t se_crc16(uint8_t *buffer, uint32_t buf_len);
#endif

#endif /* SOC_SE_H_ */
