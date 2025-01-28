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
#include <soc.h>

/* AES Register Addresses ***************************************************/

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

/* SHA Register Addresses ***************************************************/

#define SHA_CTRL          (SE_REG_BASE+0x0200)
#define SHA_MODE          (SE_REG_BASE+0x0204)
#define SHA_LEN           (SE_REG_BASE+0x0208)
#define SHA_INFIFOCTL     (SE_REG_BASE+0x020c)
#define SHA_INFIFO        (SE_REG_BASE+0x0210)
#define SHA_DATAOUT       (SE_REG_BASE+0x0214)
#define SHA_TOTAL_LEN     (SE_REG_BASE+0x0218)

/* TRNG Register Addresses **************************************************/

#define TRNG_CTRL         (SE_REG_BASE+0x0400)
#define TRNG_LR           (SE_REG_BASE+0x0404)
#define TRNG_MR           (SE_REG_BASE+0x0408)
#define TRNG_ILR          (SE_REG_BASE+0x040c)
#define TRNG_IMR          (SE_REG_BASE+0x0410)

/* CRC Register Addresses ***************************************************/

#define CRC_CTRL          (SE_REG_BASE+0x0600)
#define CRC_MODE          (SE_REG_BASE+0x0604)
#define CRC_LEN           (SE_REG_BASE+0x0608)
#define CRC_INFIFOCTL     (SE_REG_BASE+0x060c)
#define CRC_INFIFO        (SE_REG_BASE+0x0610)
#define CRC_DATAOUT       (SE_REG_BASE+0x0614)
#define CRC_DATAINIT      (SE_REG_BASE+0x0618)
#define CRC_DATAOUTXOR    (SE_REG_BASE+0x061c)
#define CRC_DEBUGOUT      (SE_REG_BASE+0x0630)

/* SE FIFO Register *********************************************************/

#define SE_FIFOCTRL       (SE_REG_BASE+0x0800)
#define SE_FIFOCTRL_AES   (0)
#define SE_FIFOCTRL_CRC   (0x1)
#define SE_FIFOCTRL_SHA   (0x2)

/* AES Register Bit Definitions *********************************************/

/* Control Register */


#define AES_CTRL_EN_SHIFT           (0)
#define AES_CTRL_EN_MASK            (0x1 << AES_CTRL_EN_SHIFT)
#define AES_CTRL_EN                 (0x1 << AES_CTRL_EN_SHIFT)
#define AES_CTRL_RESET_SHIFT        (1)
#define AES_CTRL_RESET_MASK         (0x1 << AES_CTRL_RESET_SHIFT)
#define AES_CTRL_RESET              (0x1 << AES_CTRL_RESET_SHIFT)
#define AES_CTRL_INT_EN_SHIFT       (2)
#define AES_CTRL_INT_EN_MASK        (0x1 << AES_CTRL_INT_EN_SHIFT)
#define AES_CTRL_INT_EN             (0x1 << AES_CTRL_INT_EN_SHIFT)
#define AES_CTRL_CLK_EN_SHIFT       (3)
#define AES_CTRL_CLK_EN_MASK        (0x1 << AES_CTRL_CLK_EN_SHIFT)
#define AES_CTRL_CLK_EN             (0x1 << AES_CTRL_CLK_EN_SHIFT)
#define AES_CTRL_CNT_OVF_SHIFT      (30)
#define AES_CTRL_CNT_OVF_MASK       (0x1 << AES_CTRL_CNT_OVF_SHIFT)
#define AES_CTRL_CNT_OVF            (0x1 << AES_CTRL_CNT_OVF_SHIFT)
#define AES_CTRL_END_SHIFT          (31)
#define AES_CTRL_END_MASK           (0x1 << AES_CTRL_END_SHIFT)
#define AES_CTRL_END                (0x1 << AES_CTRL_END_SHIFT)

/* Mode Register */

#define AES_MODE_TYP_SHIFT          (0)
#define AES_MODE_TYP_MASK           (0x1 << AES_MODE_TYP_SHIFT)
#define AES_MODE_DECRYPT            (0x0 << AES_MODE_TYP_SHIFT)
#define AES_MODE_ENCRYPT            (0x1 << AES_MODE_TYP_SHIFT)
#define AES_MODE_MOD_SHIFT          (2)
#define AES_MODE_MOD_MASK           (0x3 << AES_MODE_MOD_SHIFT)
#define AES_MODE_MOD_ECB            (0x0 << AES_MODE_MOD_SHIFT)
#define AES_MODE_MOD_CTR            (0x1 << AES_MODE_MOD_SHIFT)
#define AES_MODE_MOD_CBC            (0x2 << AES_MODE_MOD_SHIFT)
#define AES_MODE_MOD_CBC_CTS        (0x3 << AES_MODE_MOD_SHIFT)
#define AES_MODE_BYPASS_SHIFT       (4)
#define AES_MODE_BYPASS_MASK        (0x1 << AES_MODE_BYPASS_SHIFT)
#define AES_MODE_BYPASS_DECRYPT     (0x1 << AES_MODE_BYPASS_SHIFT)
#define AES_MODE_IV_UPDAT_SHIFT     (8)
#define AES_MODE_IV_UPDAT_MASK      (0x1 << AES_MODE_IV_UPDAT_SHIFT)
#define AES_MODE_IV_UPDAT           (0x1 << AES_MODE_IV_UPDAT_SHIFT)
#define AES_MODE_KEY_SRC_SHIFT      (16)
#define AES_MODE_KEY_SRC_MASK       (0x1 << AES_MODE_KEY_SRC_SHIFT)
#define AES_MODE_REG_KEY            (0x1 << AES_MODE_KEY_SRC_SHIFT)
#define AES_MODE_KEY_SIZE_SHIFT     (17)
#define AES_MODE_KEY_SIZE_MASK      (0x1 << AES_MODE_KEY_SIZE_SHIFT)
#define AES_MODE_KEY_128BIT         (0x0 << AES_MODE_KEY_SIZE_SHIFT)
#define AES_MODE_KEY_192BIT         (0x1 << AES_MODE_KEY_SIZE_SHIFT)
#define AES_MODE_KEY_256BIT         (0x2 << AES_MODE_KEY_SIZE_SHIFT)

/* InOutFIFO Control Register */

#define AES_FIFOCTL_INFIFODE_SHIFT  (1)
#define AES_FIFOCTL_INFIFODE_MASK   (0x1 << AES_FIFOCTL_INFIFODE_SHIFT)
#define AES_FIFOCTL_INFIFODE        (0x1 << AES_FIFOCTL_INFIFODE_SHIFT)
#define AES_FIFOCTL_OUTFIFODE_SHIFT (5)
#define AES_FIFOCTL_OUTFIFODE_MASK  (0x1 << AES_FIFOCTL_OUTFIFODE_SHIFT)
#define AES_FIFOCTL_OUTFIFODE       (0x1 << AES_FIFOCTL_OUTFIFODE_SHIFT)
#define AES_FIFOCTL_OUTFIFOFP_SHIFT (9)
#define AES_FIFOCTL_OUTFIFOFP_MASK  (0x1 << AES_FIFOCTL_OUTFIFOFP_SHIFT)
#define AES_FIFOCTL_OUTFIFOFP       (0x1 << AES_FIFOCTL_OUTFIFOFP_SHIFT)
#define AES_FIFOCTL_OUTFIFOLP_SHIFT (10)
#define AES_FIFOCTL_OUTFIFOLP_MASK  (0x1 << AES_FIFOCTL_OUTFIFOLP_SHIFT)
#define AES_FIFOCTL_OUTFIFOLP       (0x1 << AES_FIFOCTL_OUTFIFOLP_SHIFT)

/* TRNG Register Bit Definitions ********************************************/

/* Control Register */

#define TRNG_CTRL_EN_SHIFT          (0)
#define TRNG_CTRL_EN_MASK           (0x1 << TRNG_CTRL_EN_SHIFT)
#define TRNG_CTRL_EN                (0x1 << TRNG_CTRL_EN_SHIFT)
#define TRNG_CTRL_MODE_SHIFT        (1)
#define TRNG_CTRL_MODE_MASK         (0x3 << TRNG_CTRL_MODE_SHIFT)
#define TRNG_CTRL_MODE_OSC_LFSR     (0x0 << TRNG_CTRL_MODE_SHIFT)
#define TRNG_CTRL_MODE_OSC          (0x1 << TRNG_CTRL_MODE_SHIFT)
#define TRNG_CTRL_MODE_LFSR         (0x2 << TRNG_CTRL_MODE_SHIFT)
#define TRNG_CTRL_CLK_EN_SHIFT      (3)
#define TRNG_CTRL_CLK_EN_MASK       (0x1 << TRNG_CTRL_CLK_EN_SHIFT)
#define TRNG_CTRL_CLK_EN            (0x1 << TRNG_CTRL_CLK_EN_SHIFT)
#define TRNG_CTRL_OSAMPLE_MOD_SHIFT (6)
#define TRNG_CTRL_OSAMPLE_MOD_MASK  (0x3 << TRNG_CTRL_OSAMPLE_MOD_SHIFT)
#define TRNG_CTRL_OSAMPLE_HIG_LOW   (0x0 << TRNG_CTRL_OSAMPLE_MOD_SHIFT)
#define TRNG_CTRL_OSAMPLE_HIG       (0x1 << TRNG_CTRL_OSAMPLE_MOD_SHIFT)
#define TRNG_CTRL_OSAMPLE_LOW       (0x2 << TRNG_CTRL_OSAMPLE_MOD_SHIFT)
#define TRNG_CTRL_OPOST_EN_SHIFT    (8)
#define TRNG_CTRL_OPOST_EN_MASK     (0x1 << TRNG_CTRL_OPOST_EN_SHIFT)
#define TRNG_CTRL_OPOST_EN          (0x1 << TRNG_CTRL_OPOST_EN_SHIFT)
#define TRNG_CTRL_OXOR_MODE_SHIFT   (9)
#define TRNG_CTRL_OXOR_MODE_MASK    (0x1 << TRNG_CTRL_OXOR_MODE_SHIFT)
#define TRNG_CTRL_OXOR_MODE         (0x1 << TRNG_CTRL_OXOR_MODE_SHIFT)
#define TRNG_CTRL_SMPCLK_MODE_SHIFT (10)
#define TRNG_CTRL_SMPCLK_MODE_MASK  (0x1 << TRNG_CTRL_SMPCLK_MODE_SHIFT)
#define TRNG_CTRL_SMP_SE_CLK        (0x1 << TRNG_CTRL_SMPCLK_MODE_SHIFT)
#define TRNG_CTRL_READY_SHIFT       (31)
#define TRNG_CTRL_READY_MASK        (0x1 << TRNG_CTRL_READY_SHIFT)
#define TRNG_CTRL_READY             (0x1 << TRNG_CTRL_READY_SHIFT)

/* CRC Register Bit Definitions *********************************************/

/* Control Register */

#define CRC_CTRL_EN_SHIFT           (0)
#define CRC_CTRL_EN_MASK            (0x1 << CRC_CTRL_EN_SHIFT)
#define CRC_CTRL_EN                 (0x1 << CRC_CTRL_EN_SHIFT)
#define CRC_CTRL_RESET_SHIFT        (1)
#define CRC_CTRL_RESET_MASK         (0x1 << CRC_CTRL_RESET_SHIFT)
#define CRC_CTRL_RESET              (0x1 << CRC_CTRL_RESET_SHIFT)
#define CRC_CTRL_INT_EN_SHIFT       (2)
#define CRC_CTRL_INT_EN_MASK        (0x1 << CRC_CTRL_INT_EN_SHIFT)
#define CRC_CTRL_INT_EN             (0x1 << CRC_CTRL_INT_EN_SHIFT)
#define CRC_CTRL_CLK_EN_SHIFT       (3)
#define CRC_CTRL_CLK_EN_MASK        (0x1 << CRC_CTRL_CLK_EN_SHIFT)
#define CRC_CTRL_CLK_EN             (0x1 << CRC_CTRL_CLK_EN_SHIFT)
#define CRC_CTRL_END_SHIFT          (31)
#define CRC_CTRL_END_MASK           (0x1 << CRC_CTRL_END_SHIFT)
#define CRC_CTRL_END                (0x1 << CRC_CTRL_END_SHIFT)

/* Mode Register */

#define CRC_MODE_TYPE_SHIFT         (0)
#define CRC_MODE_TYPE_MASK          (0x1 << CRC_MODE_TYPE_SHIFT)
#define CRC_MODE_TYPE_CRC32         (0x1 << CRC_MODE_TYPE_SHIFT)
#define CRC_MODE_CRC16_POLY_SHIFT   (4)
#define CRC_MODE_CRC16_POLY_8005    (0x1 << CRC_MODE_CRC16_POLY_SHIFT)
#define CRC_MODE_IN_INV_SHIFT       (12)
#define CRC_MODE_IN_INV_TRUE        (0x1 << CRC_MODE_IN_INV_SHIFT)
#define CRC_MODE_OUT_INV_SHIFT      (13)
#define CRC_MODE_OUT_INV_TRUE       (0x1 << CRC_MODE_OUT_INV_SHIFT)
#define CRC_MODE_INITDATA_UPD_SHIFT (17)
#define CRC_MODE_INITDATA_UPD       (0x1 << CRC_MODE_INITDATA_UPD_SHIFT)

/* InFIFO Control Register */

#define CRC_INFIFOCTL_DE_SHIFT      (1)
#define CRC_INFIFOCTL_DE_MASK       (0x1 << CRC_INFIFOCTL_DE_SHIFT)
#define CRC_INFIFOCTL_DE            (0x1 << CRC_INFIFOCTL_DE_SHIFT)

/* SHA Register Bit Definitions *********************************************/

/* Control Register */

#define SHA_CTRL_EN_SHIFT           (0)
#define SHA_CTRL_EN_MASK            (0x1 << SHA_CTRL_EN_SHIFT)
#define SHA_CTRL_EN                 (0x1 << SHA_CTRL_EN_SHIFT)
#define SHA_CTRL_RESET_SHIFT        (1)
#define SHA_CTRL_RESET_MASK         (0x1 << SHA_CTRL_RESET_SHIFT)
#define SHA_CTRL_RESET              (0x1 << SHA_CTRL_RESET_SHIFT)
#define SHA_CTRL_INT_EN_SHIFT       (2)
#define SHA_CTRL_INT_EN_MASK        (0x1 << SHA_CTRL_INT_EN_SHIFT)
#define SHA_CTRL_INT_EN             (0x1 << SHA_CTRL_INT_EN_SHIFT)
#define SHA_CTRL_CLK_EN_SHIFT       (3)
#define SHA_CTRL_CLK_EN_MASK        (0x1 << SHA_CTRL_CLK_EN_SHIFT)
#define SHA_CTRL_CLK_EN             (0x1 << SHA_CTRL_CLK_EN_SHIFT)
#define SHA_CTRL_GAT_EN_SHIFT       (4)
#define SHA_CTRL_GAT_EN_MASK        (0x1 << SHA_CTRL_GAT_EN_SHIFT)
#define SHA_CTRL_GAT_EN             (0x1 << SHA_CTRL_GAT_EN_SHIFT)
#define SHA_CTRL_LEN_ERR_SHIFT      (30)
#define SHA_CTRL_LEN_ERR_MASK       (0x1 << SHA_CTRL_LEN_ERR_SHIFT)
#define SHA_CTRL_LEN_ERR            (0x1 << SHA_CTRL_LEN_ERR_SHIFT)
#define SHA_CTRL_END_SHIFT          (31)
#define SHA_CTRL_END_MASK           (0x1 << SHA_CTRL_END_SHIFT)
#define SHA_CTRL_END                (0x1 << SHA_CTRL_END_SHIFT)

/* Mode Register */

#define SHA_MODE_TYPE_SHIFT         (0)
#define SHA_MODE_TYPE_MASK          (0x3 << SHA_MODE_TYPE_SHIFT)
#define SHA_MODE_TYPE_1             (0x0 << SHA_MODE_TYPE_SHIFT)
#define SHA_MODE_TYPE_256           (0x1 << SHA_MODE_TYPE_SHIFT)
#define SHA_MODE_TYPE_224           (0x2 << SHA_MODE_TYPE_SHIFT)
#define SHA_MODE_PADDING_SHIFT      (4)
#define SHA_MODE_PADDING_MASK       (0x1 << SHA_MODE_PADDING_SHIFT)
#define SHA_MODE_PADDING            (0x1 << SHA_MODE_PADDING_SHIFT)
#define SHA_MODE_FIRST_SHIFT        (5)
#define SHA_MODE_FIRST_MASK         (0x1 << SHA_MODE_FIRST_SHIFT)
#define SHA_MODE_FIRST              (0x1 << SHA_MODE_FIRST_SHIFT)

/* InFIFO Control Register */

#define SHA_INFIFOCTL_DE_SHIFT      (1)
#define SHA_INFIFOCTL_DE_MASK       (0x1 << SHA_INFIFOCTL_DE_SHIFT)
#define SHA_INFIFOCTL_DE            (0x1 << SHA_INFIFOCTL_DE_SHIFT)


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
