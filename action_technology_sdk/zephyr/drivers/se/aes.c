/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <drivers/dma.h>
#include <drivers/se/se.h>
#include <soc.h>
#include <board_cfg.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(aes, CONFIG_LOG_DEFAULT_LEVEL);

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
#define AES_MODE_KEY_EFUSE_SHIFT    (19)
#define AES_MODE_KEY_EFUSE_MASK     (0x1 << AES_MODE_KEY_EFUSE_SHIFT)
#define AES_MODE_IV0_SRC_SHIFT      (20)
#define AES_MODE_IV0_SRC_MASK       (0x1 << AES_MODE_IV0_SRC_SHIFT)
#define AES_MODE_REG_IV0            (0x1 << AES_MODE_IV0_SRC_SHIFT)

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

#define SE_DMA_ID                   17

#define AES_BLOCK_SIZE              16
#define AES_FIFO_DEPTH              (4 * AES_BLOCK_SIZE)
#define IN_DMA_TRANS_OK             0x01
#define OUT_DMA_TRANS_OK            0x02
#define ALL_DMA_TRANS_OK            (IN_DMA_TRANS_OK | OUT_DMA_TRANS_OK)

#ifdef CONFIG_AES_DMA
struct aes_dev_s
{
	const struct device *dma;      /* AES DMA */
	struct k_sem waitsem;          /* AES DMA crypto and trans complete sem */
	int     in_chan;               /* AES DMA input channel */
	int     out_chan;              /* AES DMA output channel */
	uint8_t trans_done;            /* AES DMA input/output trans ok flag */
};

static struct aes_dev_s g_aesdev =
{
	.dma        = NULL,
	.waitsem    = Z_SEM_INITIALIZER(g_aesdev.waitsem, 0, 1),
	.trans_done = 0,
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifndef CONFIG_AES_DMA

/****************************************************************************
 * Name: aes_encrypt_by_cpu
 *
 * Description:
 *   aes encrypt/decrypt by cpu.
 *
 ****************************************************************************/

static int aes_encrypt_by_cpu(void *out,
                              const void *in, size_t size)
{
	size_t size_remain;
	size_t aes_len;
	uint32_t aes_ctrl_val;
	uint32_t aes_inoutfifictl_val;
	uint8_t *aes_in  = (uint8_t *)in;
	uint8_t *aes_out = (uint8_t *)out;

	/* set SE FIFO to AES */

	sys_write32(0, SE_FIFOCTRL);

	/* disable DRQ */

	aes_inoutfifictl_val  = sys_read32(AES_INOUTFIFOCTL);
	aes_inoutfifictl_val &= ~AES_FIFOCTL_INFIFODE_MASK;
	aes_inoutfifictl_val &= ~AES_FIFOCTL_OUTFIFODE_MASK;
	sys_write32(aes_inoutfifictl_val, AES_INOUTFIFOCTL);

	size_remain = size;

	while (size_remain > 0) {

		if (size_remain >= AES_FIFO_DEPTH) {
			aes_len      = AES_FIFO_DEPTH;
			size_remain -= AES_FIFO_DEPTH;
		} else {
			aes_len     = size_remain;
			size_remain = 0;
		}

		sys_write32(aes_len, AES_LEN);

		se_memcpy((void *)AES_INFIFO,
                  aes_in, aes_len, CPY_MEM_TO_FIFO);

		/* enable aes crypt */

		aes_ctrl_val = sys_read32(AES_CTRL);
		sys_write32(aes_ctrl_val | AES_CTRL_EN, AES_CTRL);

		while ((sys_read32(AES_INOUTFIFOCTL)
               & (AES_FIFOCTL_OUTFIFOFP | AES_FIFOCTL_OUTFIFOLP)) == 0);

		aes_ctrl_val = sys_read32(AES_CTRL);

		/* clear fifo last data irq pending or fifo full irq pending */

		sys_write32(aes_ctrl_val, AES_CTRL);

		aes_ctrl_val = sys_read32(AES_CTRL);

		/* Clear AES Ready */

		aes_ctrl_val |= AES_CTRL_END;
		sys_write32(aes_ctrl_val, AES_CTRL);

		if (out) {
			se_memcpy(aes_out, (void *)AES_OUTFIFO,
                      aes_len, CPY_FIFO_TO_MEM);

			aes_out += aes_len;
		}

		aes_in += aes_len;

		/* enable AES reset */

		aes_ctrl_val = sys_read32(AES_CTRL);
		aes_ctrl_val |= AES_CTRL_RESET;
		sys_write32(aes_ctrl_val, AES_CTRL);

		/* Wait finished */

		while (sys_read32(AES_CTRL) & AES_CTRL_RESET);
	}

	return 0;
}
#endif

#ifdef CONFIG_AES_DMA

/****************************************************************************
 * Name: aes_in_dmacallback
 *
 * Description:
 *   aes input data transfer done callback by dma.
 *
 ****************************************************************************/

static void aes_in_dmacallback(const struct device *dev,
                               void *user_data, uint32_t chan, int status)
{
	g_aesdev.trans_done |= IN_DMA_TRANS_OK;

	if (g_aesdev.trans_done == ALL_DMA_TRANS_OK)
		k_sem_give(&g_aesdev.waitsem);
}

/****************************************************************************
 * Name: aes_out_dmacallback
 *
 * Description:
 *   aes output data transfer done callback by dma.
 *
 ****************************************************************************/

static void aes_out_dmacallback(const struct device *dev,
                                void *user_data, uint32_t chan, int status)
{
	g_aesdev.trans_done |= OUT_DMA_TRANS_OK;

	if (g_aesdev.trans_done == ALL_DMA_TRANS_OK)
		k_sem_give(&g_aesdev.waitsem);
}

/****************************************************************************
 * Name: aes_encrypt_by_dma
 *
 * Description:
 *   aes encrypt/decrypt by dma.
 *
 ****************************************************************************/

static int aes_encrypt_by_dma(void *out,
                              const void *in, size_t size)
{
	uint32_t aes_inoutfifictl_val;
	uint32_t aes_ctrl_val;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	uint8_t *aes_in;
	uint8_t *aes_out;
	int ret = 0;

	/* if not 4 bytes aligned(or in rom), malloc 4-aligned for dma trans */

	if ((((uint32_t)in & 0x3) != 0) ||
        (((uint32_t)in >= 0x10000000) && ((uint32_t)in < 0x18000000))) {

		aes_in = (uint8_t *)malloc(size);
		if (aes_in == NULL)
			return -EINVAL;

		memcpy(aes_in, (uint8_t *)in, size);
	} else {
		aes_in = (uint8_t *)in;
	}

	/* if not 4 bytes aligned, need malloc 4-aligned data for dma trans */

	if ((((uint32_t)out & 0x3) != 0) ||
        (((uint32_t)out >= 0x10000000) && ((uint32_t)out < 0x18000000))) {
		aes_out = (uint8_t *)malloc(size);

		if (aes_out == NULL)
			return -EINVAL;
	} else {
		aes_out = (uint8_t *)out;
	}

	/* set SE FIFO to AES */

	sys_write32(0, SE_FIFOCTRL);

	/* enable DRQ */

	aes_inoutfifictl_val  = sys_read32(AES_INOUTFIFOCTL);
	aes_inoutfifictl_val |= AES_FIFOCTL_INFIFODE;
	aes_inoutfifictl_val |= AES_FIFOCTL_OUTFIFODE;
	sys_write32(aes_inoutfifictl_val, AES_INOUTFIFOCTL);

	/* set aes len */

	sys_write32(size, AES_LEN);

	/* clear transfer flag */

	g_aesdev.trans_done = 0;

	/* Configure the TX DMA */

	dma_cfg.dma_slot = SE_DMA_ID;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 4U;
	dma_cfg.dest_data_size = 4U;
	dma_cfg.dma_callback = aes_in_dmacallback;
	dma_cfg.complete_callback_en = 1U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	dma_block_cfg.source_address = (uint32_t)aes_in;
	dma_block_cfg.dest_address = AES_INFIFO;
	dma_block_cfg.block_size = size;
	dma_block_cfg.source_reload_en = 0;

	ret = dma_config(g_aesdev.dma, g_aesdev.in_chan, &dma_cfg);
	if (ret < 0)
		return -EBUSY;

	/* Configure the RX DMA */

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = aes_out_dmacallback;
	dma_block_cfg.source_address = AES_OUTFIFO;
	dma_block_cfg.dest_address = (uint32_t)aes_out;
	dma_block_cfg.block_size = size;

	ret = dma_config(g_aesdev.dma, g_aesdev.out_chan, &dma_cfg);
	if (ret < 0)
		return -EBUSY;

	dma_start(g_aesdev.dma, g_aesdev.in_chan);
	dma_start(g_aesdev.dma, g_aesdev.out_chan);

	/* enable aes crypt */

	aes_ctrl_val = sys_read32(AES_CTRL);
	sys_write32(aes_ctrl_val | AES_CTRL_EN, AES_CTRL);

	/* wait aes end */

	ret = k_sem_take(&g_aesdev.waitsem, K_MSEC(50));
	if (ret < 0)
		goto wait_errout;

	/* Clear AES Ready */

	aes_ctrl_val |= AES_CTRL_END;
	sys_write32(aes_ctrl_val, AES_CTRL);

	/* enable AES reset */

	aes_ctrl_val = sys_read32(AES_CTRL);
	aes_ctrl_val |= AES_CTRL_RESET;
	sys_write32(aes_ctrl_val, AES_CTRL);

	/* Wait finished */

	while (sys_read32(AES_CTRL) & AES_CTRL_RESET);

	/* if out is not 4 aligned, we should copy aes_out to out */

	if ((((uint32_t)out & 0x3) != 0) ||
        (((uint32_t)out >= 0x10000000) && ((uint32_t)out < 0x18000000))) {
		memcpy((uint8_t *)out, aes_out, size);
		free(aes_out);
	}

	if ((((uint32_t)in & 0x3) != 0) ||
        (((uint32_t)in >= 0x10000000) && ((uint32_t)in < 0x18000000)))
		free(aes_in);

wait_errout:
	dma_stop(g_aesdev.dma, g_aesdev.in_chan);
	dma_stop(g_aesdev.dma, g_aesdev.out_chan);
	return ret;
}
#endif

/****************************************************************************
 * Name: aes_setup_mr
 *
 * Description:
 *   set aes encrypt/decrypt mode and parameter.
 *
 ****************************************************************************/

static int aes_setup_mr(uint32_t keysize,
                        int mode, int encrypt)
{
	uint32_t regval = AES_MODE_REG_KEY | AES_MODE_REG_IV0;

	if (encrypt)
		regval |= AES_MODE_ENCRYPT;
	else
		regval |= AES_MODE_DECRYPT;

	switch (keysize){
    case 16:
		regval |= AES_MODE_KEY_128BIT;
		break;

    case 24:
		regval |= AES_MODE_KEY_192BIT;
		break;

    case 32:
		regval |= AES_MODE_KEY_256BIT;
		break;

    default:
		return -EINVAL;
	}

	switch (mode){
	case AES_MODE_ECB:
		regval |= AES_MODE_MOD_ECB;
		break;

    case AES_MODE_CTR:
		regval |= AES_MODE_MOD_CTR;
		break;

    case AES_MODE_CBC:
		regval |= AES_MODE_MOD_CBC;
		break;

    case AES_MODE_CBC_CTS:
		regval |= AES_MODE_MOD_CBC_CTS;
		break;

    default:
		return -EINVAL;
	}

	sys_write32(regval, AES_MODE);

	return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: aes_init
 *
 * Description:
 *   before using aes, we should init aes clock and related things.
 *
 ****************************************************************************/

static int aes_init(void)
{
	uint32_t clken0_val;

	clken0_val = sys_read32(CMU_DEVCLKEN0);

	/* check whether the CLOCK_ID_SE has been enabled or not */
	if (!(clken0_val & (0x1 << CLOCK_ID_SE))){

		/* if not initialized, we should init it */
		/* SE CLK = HOSC/1 */
		sys_write32((0 << 8) | (0 << 0), CMU_SECCLK);

		/* enable SE controller clock */
		acts_clock_peripheral_enable(CLOCK_ID_SE);

		/* reset SE controller */
		acts_reset_peripheral(RESET_ID_SE);
	}

	/* enable AES controller clock */
	sys_write32(AES_CTRL_CLK_EN, AES_CTRL);

	return 0;
}

/****************************************************************************
 * Name: aes_deinit
 *
 * Description:
 *   Disable AES for energy saving purpose.
 *
 ****************************************************************************/
static void aes_deinit(void)
{
	/* disable aes clk */
	sys_write32(0, AES_CTRL);
}

/****************************************************************************
 * Name: aes_cypher
 *
 * Description:
 *   aes encypher/decrypter realization.
 *
 ****************************************************************************/

int aes_cypher(void *out, const void *in, size_t size,
               const void *iv, const void *key, size_t keysize,
               int mode, int encrypt)
{
	int ret = 0;

#ifdef CONFIG_AES_DMA

	/* Need request aes dma chan in the first init */
	if (g_aesdev.dma == NULL){

		g_aesdev.dma = device_get_binding(CONFIG_DMA_0_NAME);
		if (!g_aesdev.dma) {
			LOG_ERR("Bind DMA device %s error", CONFIG_DMA_0_NAME);
			return -ENOENT;
		}

		g_aesdev.in_chan  = dma_request(g_aesdev.dma, 0xff);
		g_aesdev.out_chan = dma_request(g_aesdev.dma, 0xff);
		if ((g_aesdev.in_chan < 0) || (g_aesdev.out_chan < 0)) {
			LOG_ERR("dma-dev rxchan config err chan\n");
			return -ENODEV;
		}
	}
#endif

	if (size % 16){
		return -EINVAL;
	}

	k_mutex_lock(&se_lock, K_FOREVER);

	aes_init();

	ret = aes_setup_mr(keysize, mode, encrypt);
	if (ret < 0){
		k_mutex_unlock(&se_lock);
		LOG_ERR("unsupported AES mode");
		return ret;
	}

	se_memcpy((void *)AES_KEY0, key, keysize, CPY_MEM_TO_MEM);

	if (iv != NULL)
		se_memcpy((void *)AES_IV0, iv, AES_BLOCK_SIZE, CPY_MEM_TO_MEM);

#ifndef CONFIG_AES_DMA
	aes_encrypt_by_cpu(out, in, size);
#else
	ret = aes_encrypt_by_dma(out, in, size);
#endif

	aes_deinit();

	k_mutex_unlock(&se_lock);

	return ret;
}
