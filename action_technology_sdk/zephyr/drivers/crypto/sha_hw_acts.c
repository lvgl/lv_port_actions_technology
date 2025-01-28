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
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"

LOG_MODULE_REGISTER(sha, CONFIG_LOG_SHA_LEVEL);

/* SHA Register Addresses ***************************************************/

#define SHA_CTRL                    (SE_REG_BASE+0x0200)
#define SHA_MODE                    (SE_REG_BASE+0x0204)
#define SHA_LEN                     (SE_REG_BASE+0x0208)
#define SHA_INFIFOCTL               (SE_REG_BASE+0x020c)
#define SHA_INFIFO                  (SE_REG_BASE+0x0210)
#define SHA_DATAOUT                 (SE_REG_BASE+0x0214)
#define SHA_TOTAL_LEN               (SE_REG_BASE+0x0218)

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

#define SHA_MODE_MOD_SHIFT          (0)
#define SHA_MODE_MOD_MASK           (0x3 << SHA_MODE_MOD_SHIFT)
#define SHA_MODE_MOD_SHA1           (0x0 << SHA_MODE_MOD_SHIFT)
#define SHA_MODE_MOD_SHA256         (0x1 << SHA_MODE_MOD_SHIFT)
#define SHA_MODE_MOD_SHA224         (0x2 << SHA_MODE_MOD_SHIFT)
#define SHA_MODE_MOD_SHA512         (0x3 << SHA_MODE_MOD_SHIFT)
#define SHA_MODE_PADDING_SHIFT      (4)
#define SHA_MODE_PADDING_MASK       (0x1 << SHA_MODE_PADDING_SHIFT)
#define SHA_MODE_PADDING_HW         (0x1 << SHA_MODE_PADDING_SHIFT)
#define SHA_MODE_FIRST_SHIFT        (5)
#define SHA_MODE_FIRST_MASK         (0x1 << SHA_MODE_FIRST_SHIFT)
#define SHA_MODE_FIRST              (0x1 << SHA_MODE_FIRST_SHIFT)

/* InFIFO Control Register */

#define SHA_INFIFOCTL_IE_SHIFT      (0)
#define SHA_INFIFOCTL_IE_MASK       (0x1 << SHA_INFIFOCTL_IE_SHIFT)
#define SHA_INFIFOCTL_IE            (0x1 << SHA_INFIFOCTL_IE_SHIFT)
#define SHA_INFIFOCTL_DE_SHIFT      (1)
#define SHA_INFIFOCTL_DE_MASK       (0x1 << SHA_INFIFOCTL_DE_SHIFT)
#define SHA_INFIFOCTL_DE            (0x1 << SHA_INFIFOCTL_DE_SHIFT)
#define SHA_INFIFOCTL_FULL_SHIFT    (2)
#define SHA_INFIFOCTL_FULL_MASK     (0x1 << SHA_INFIFOCTL_FULL_SHIFT)
#define SHA_INFIFOCTL_FULL          (0x1 << SHA_INFIFOCTL_FULL_SHIFT)
#define SHA_INFIFOCTL_ERR_SHIFT     (3)
#define SHA_INFIFOCTL_ERR_MASK      (0x1 << SHA_INFIFOCTL_ERR_SHIFT)
#define SHA_INFIFOCTL_ERR           (0x1 << SHA_INFIFOCTL_ERR_SHIFT)
#define SHA_INFIFOCTL_IP_SHIFT      (8)
#define SHA_INFIFOCTL_IP_MASK       (0x1 << SHA_INFIFOCTL_IP_SHIFT)
#define SHA_INFIFOCTL_IP            (0x1 << SHA_INFIFOCTL_IP_SHIFT)

#define SE_DMA_ID                   17
#define SHA_MODE_SHA1               0
#define SHA_MODE_SHA256             1
#define SHA_MODE_SHA224             2
#define SHA_MODE_SHA512             3

#define SHA_SEG_LEN                 64
#define SHA_CACHE_SIZE              (CONFIG_SHA_CACHE_NSEG * SHA_SEG_LEN)

enum sha_state_t {
	SHA_START,
	SHA_UPDATE,
	SHA_FINISH,
};

struct sha_dev_s
{
	enum sha_state_t hw_state;     /* SHA HARDWARE CONTROLLOR STATE */
	const struct device *dma;      /* SHA DMA */
	struct k_sem waitsem;          /* SHA DMA crypto and trans complete sem */
	int    chan;                   /* SHA DMA input channel */
	uint8_t  cach[SHA_CACHE_SIZE];  /* SHA cache date */
	uint32_t offset;               /* SHA cache date offset */
	bool     first_seg;            /* SHA First segment flag */
	uint8_t  mode;                 /* SHA mode */
	bool     out_dump;             /* SHA print output */
};

static struct sha_dev_s g_shadev =
{
	.dma        = NULL,
	.waitsem    = Z_SEM_INITIALIZER(g_shadev.waitsem, 0, 1),
#if (CONFIG_LOG_SHA_LEVEL > LOG_LEVEL_INF)
	.out_dump   = 1,
#endif
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void sha_reset(void)
{
	uint32_t ctlval;

	sys_write32(sys_read32(SHA_INFIFOCTL) & ~SHA_INFIFOCTL_DE_MASK, SHA_INFIFOCTL);

	/* enable SHA reset */

	ctlval = sys_read32(SHA_CTRL);
	ctlval |= SHA_CTRL_RESET;
	sys_write32(ctlval, SHA_CTRL);

	/* Wait finished */

	while (sys_read32(SHA_CTRL) & SHA_CTRL_RESET);
}

static int sha_init(void)
{
	uint32_t clken0_val;

	clken0_val = sys_read32(CMU_DEVCLKEN0);

	/* check whether the CLOCK_ID_SE has been enabled or not */
	if (!(clken0_val & (0x1 << CLOCK_ID_SE))) {

		/* if not initialized, we should init it */
		/* SE CLK = HOSC/1 */
		sys_write32((0x0 << 8) | (0 << 0), CMU_SECCLK);

		/* enable SE controller clock */
		acts_clock_peripheral_enable(CLOCK_ID_SE);

		/* reset SE controller */
		acts_reset_peripheral(RESET_ID_SE);
	}

	/* enable SHA controller clock */
	sys_write32(SHA_CTRL_CLK_EN, SHA_CTRL);

	return 0;
}

static void sha_deinit(void)
{
	/* disable sha clk */

	sys_write32(0, SHA_CTRL);
}

static void sha_dmacallback(const struct device *dev,
                                void *user_data, uint32_t chan, int status)
{
	k_sem_give(&g_shadev.waitsem);
}

static int sha_by_dma(const unsigned char *in, size_t size)
{
	uint32_t sha_infifoctl_val;
	uint32_t sha_ctrl_val;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};
	uint8_t *sha_in;
	int ret = 0;

	/* if not 4 bytes aligned(or in rom), malloc 4-aligned for dma trans */

	if ((((uint32_t)in & 0x3) != 0) ||
        (((uint32_t)in >= 0x10000000) && ((uint32_t)in < 0x18000000))) {

		sha_in = (uint8_t *)malloc(size);
		if (sha_in == NULL)
			return -EINVAL;

		memcpy(sha_in, (uint8_t *)in, size);
	} else {
		sha_in = (uint8_t *)in;
	}

	/* set SE FIFO to SHA */

	sys_write32(0x2, SE_FIFOCTRL);

	/* enable DRQ */

	sha_infifoctl_val  = sys_read32(SHA_INFIFOCTL);
	sha_infifoctl_val |= SHA_INFIFOCTL_DE;
	sys_write32(sha_infifoctl_val, SHA_INFIFOCTL);

	/* set sha len */

	sys_write32(size, SHA_LEN);

	/* Configure DMA */

	dma_cfg.dma_slot = SE_DMA_ID;
	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 4U;
	dma_cfg.dest_data_size = 4U;
	dma_cfg.dma_callback = sha_dmacallback;
	dma_cfg.complete_callback_en = 1U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;

	dma_block_cfg.source_address = (uint32_t)sha_in;
	dma_block_cfg.dest_address = SHA_INFIFO;
	dma_block_cfg.block_size = size;
	dma_block_cfg.source_reload_en = 0;

	ret = dma_config(g_shadev.dma, g_shadev.chan, &dma_cfg);
	if (ret < 0)
		return -EBUSY;

	dma_start(g_shadev.dma, g_shadev.chan);

	/* enable sha */

	sha_ctrl_val = sys_read32(SHA_CTRL);
	sys_write32(sha_ctrl_val | SHA_CTRL_EN, SHA_CTRL);

	ret = k_sem_take(&g_shadev.waitsem, K_MSEC(500));
	if (ret < 0)
		goto wait_errout;

	/* wait sha end */

	while (sys_read32(SHA_CTRL) & SHA_CTRL_EN);

	/* Clear SHA Ready */

	sha_ctrl_val |= SHA_CTRL_END;
	sys_write32(sha_ctrl_val, SHA_CTRL);

	if ((((uint32_t)in & 0x3) != 0) ||
        (((uint32_t)in >= 0x10000000) && ((uint32_t)in < 0x18000000)))
		free(sha_in);

wait_errout:
	dma_stop(g_shadev.dma, g_shadev.chan);
	return ret;
}

static int sha_setup_mr(enum sha_state_t state, int mode, size_t size)
{
	uint32_t regval = mode;

	switch(state) {
	case SHA_START:
		sha_init();
		sha_reset();
		g_shadev.hw_state = SHA_START;
		break;

	case SHA_UPDATE:
		if (g_shadev.hw_state == SHA_START) {
			regval |= SHA_MODE_FIRST;
			g_shadev.hw_state = SHA_UPDATE;
			g_shadev.first_seg = true;
			LOG_DBG("PAD FIRST");
		} else {
			g_shadev.first_seg = false;
		}
		break;

	case SHA_FINISH:
		if (g_shadev.hw_state == SHA_START) {
			regval |= SHA_MODE_FIRST;
			g_shadev.hw_state  = SHA_UPDATE;
			g_shadev.first_seg = true;
			LOG_DBG("PAD FIRST");
		} else {
			g_shadev.first_seg = false;
		}

		if ((g_shadev.hw_state == SHA_UPDATE) && (size <= 64)) {
			regval |= SHA_MODE_PADDING;
			g_shadev.hw_state = SHA_FINISH;
			LOG_DBG("PAD FINISH");
		}
		break;

	default:
		return -1;
	}

	sys_write32(regval, SHA_MODE);

	return 0;
}

static int sha_cypher_data(const unsigned char *in,
                               size_t size,
                               enum sha_state_t state,
                               int mode)
{
	int ret = 0;
	uint32_t nseg = 0;
	uint32_t remain_size = size;

	LOG_DBG("%s, digested cypher size=%d", __func__ , size);

	do {
		sha_setup_mr(state, mode, remain_size);
		if (remain_size <= SHA_SEG_LEN) {
			ret = sha_by_dma(in, remain_size);
			in += remain_size;
			remain_size = 0;
		} else if ((remain_size > SHA_SEG_LEN) && (g_shadev.first_seg)) {
			ret = sha_by_dma(in, SHA_SEG_LEN);
			in += SHA_SEG_LEN;
			remain_size -= SHA_SEG_LEN;
		} else {
			nseg = (remain_size - 1)/ SHA_SEG_LEN;
			ret = sha_by_dma(in, nseg * SHA_SEG_LEN);
			in += nseg * SHA_SEG_LEN;
			remain_size -= nseg * SHA_SEG_LEN;
		}
		LOG_DBG("staged remain=%d", remain_size);
	} while (remain_size && !ret);

	return ret;
}

static void sha_print_output(unsigned char *out, uint32_t out_size, bool dump)
{
	if (dump) {
		printk("out:\n");
		for (int i = 0; i < out_size; i++) {
			printk("%02x ", out[i]);
		}
		printk("\n");
	}
}

static int sha_cypher_get_output(unsigned char *out, int mode)
{
	uint32_t out_size = 0;

	if (out != NULL) {
		if (mode == SHA_MODE_SHA1)
			out_size = 20;
		else if (mode == SHA_MODE_SHA256)
			out_size = 32;
		else if (mode == SHA_MODE_SHA224)
			out_size = 28;
		else if (mode == SHA_MODE_SHA512)
			out_size = 64;

		se_memcpy(out, (void *)SHA_DATAOUT, out_size, CPY_FIFO_TO_MEM);

		sha_print_output(out, out_size, g_shadev.out_dump);
	}

	return 0;
}

static int sha_cypher(unsigned char *out,
                      const unsigned char *in,
                      size_t size,
                      enum sha_state_t state,
                      int mode)
{
	int ret = 0;

	/* Need request sha dma chan in the first init */
	if (g_shadev.dma == NULL){

		g_shadev.dma = device_get_binding(CONFIG_DMA_0_NAME);
		if (!g_shadev.dma) {
			LOG_ERR("Bind DMA device %s error", CONFIG_DMA_0_NAME);
			return -ENOENT;
		}

		g_shadev.chan  = dma_request(g_shadev.dma, 0xff);
		if (g_shadev.chan < 0) {
			LOG_ERR("dma-dev chan cannot be allocated\n");
			return -ENODEV;
		}
	}

	k_mutex_lock(&se_lock, K_FOREVER);

	if (state == SHA_START) {
		sha_setup_mr(state, mode, size);
	} else if (state == SHA_UPDATE) {
		ret = sha_cypher_data(in, size, state, mode);
		if (ret < 0)
			goto trans_quit;
	} else if (state == SHA_FINISH) {
		ret = sha_cypher_data(in, size, state, mode);
		if (ret < 0)
			goto trans_quit;
		sha_cypher_get_output(out, mode);
		sha_deinit();
	}

	k_mutex_unlock(&se_lock);

	return 0;

trans_quit:
	sha_deinit();
	k_mutex_unlock(&se_lock);
	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*
 * SHA-1 context setup
 */
int mbedtls_sha1_starts_ret(mbedtls_sha1_context *ctx)
{
	LOG_DBG("\n%s", __func__);

	g_shadev.offset = 0;
	g_shadev.mode = SHA_MODE_SHA1;

	return sha_cypher(NULL, NULL, 0, SHA_START, SHA_MODE_SHA1);
}

/*
 * SHA-1 process buffer
 */
int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx,
                              const unsigned char *input,
                              size_t ilen)
{
	int ret;

	LOG_DBG("\n%s", __func__);

	do {
		if (g_shadev.offset + ilen < SHA_CACHE_SIZE) {
			LOG_DBG("sha1 update0, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, ilen);
			memcpy(&g_shadev.cach[g_shadev.offset], input, ilen);
			g_shadev.offset += ilen;
			ilen = 0;
			ret = 0;
		} else if (g_shadev.offset + ilen == SHA_CACHE_SIZE) {
			LOG_DBG("sha1 update1, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, SHA_CACHE_SIZE);
			memcpy(&g_shadev.cach[g_shadev.offset], input, ilen - 64);
			ret = sha_cypher(NULL, g_shadev.cach, SHA_CACHE_SIZE - 64, SHA_UPDATE, SHA_MODE_SHA1);
			input += ilen - 64;
			memcpy(&g_shadev.cach[0], input, 64);
			ilen = 0;
			g_shadev.offset = 64;
		} else {
			LOG_DBG("sha1 update2, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, SHA_CACHE_SIZE - g_shadev.offset);
			memcpy(&g_shadev.cach[g_shadev.offset], input, SHA_CACHE_SIZE - g_shadev.offset);
			ilen -= (SHA_CACHE_SIZE - g_shadev.offset);
			ret = sha_cypher(NULL, g_shadev.cach, SHA_CACHE_SIZE, SHA_UPDATE, SHA_MODE_SHA1);
			g_shadev.offset = 0;
		}
	} while (ilen && !ret);

	return ret;
}

/*
 * SHA-1 final digest
 */
int mbedtls_sha1_finish_ret(mbedtls_sha1_context *ctx,
                        unsigned char output[20])
{
	LOG_DBG("\n%s", __func__);

	return sha_cypher(output, g_shadev.cach, g_shadev.offset, SHA_FINISH, SHA_MODE_SHA1);
}

/*
 * SHA-256 context setup
 */
int mbedtls_sha256_starts_ret(mbedtls_sha256_context *ctx, int is224)
{
	if (is224)
		g_shadev.mode = SHA_MODE_SHA224;
	else
		g_shadev.mode = SHA_MODE_SHA256;

	LOG_DBG("\n%s", __func__);

	g_shadev.offset = 0;

	return sha_cypher(NULL, NULL, 0, SHA_START, g_shadev.mode);
}

/*
 * SHA-256 process buffer
 */
int mbedtls_sha256_update_ret(mbedtls_sha256_context *ctx,
                          const unsigned char *input,
                          size_t ilen)
{
	int ret;

	LOG_DBG("\n%s", __func__);

	do {
		if (g_shadev.offset + ilen < SHA_CACHE_SIZE) {
			LOG_DBG("sha256 update0, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, ilen);
			memcpy(&g_shadev.cach[g_shadev.offset], input, ilen);
			g_shadev.offset += ilen;
			ilen = 0;
			ret = 0;
		} else if (g_shadev.offset + ilen == SHA_CACHE_SIZE) {
			LOG_DBG("sha256 update1, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, SHA_CACHE_SIZE);
			memcpy(&g_shadev.cach[g_shadev.offset], input, ilen - 64);
			ret = sha_cypher(NULL, g_shadev.cach, SHA_CACHE_SIZE - 64, SHA_UPDATE, g_shadev.mode);
			input += ilen - 64;
			memcpy(&g_shadev.cach[0], input, 64);
			ilen = 0;
			g_shadev.offset = 64;
		} else {
			LOG_DBG("sha256 update2, off=%d, remains=%d, be handled size=%d",
				    g_shadev.offset, ilen, SHA_CACHE_SIZE - g_shadev.offset);
			memcpy(&g_shadev.cach[g_shadev.offset], input, SHA_CACHE_SIZE - g_shadev.offset);
			ilen -= (SHA_CACHE_SIZE - g_shadev.offset);
			ret = sha_cypher(NULL, g_shadev.cach, SHA_CACHE_SIZE, SHA_UPDATE, g_shadev.mode);
			g_shadev.offset = 0;
		}
	} while (ilen && !ret);

	return ret;
}

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish_ret(mbedtls_sha256_context *ctx,
                              unsigned char *output)
{
	LOG_DBG("\n%s", __func__);

	return sha_cypher(output, g_shadev.cach, g_shadev.offset, SHA_FINISH, g_shadev.mode);
}

void mbedtls_sha1_starts(mbedtls_sha1_context *ctx)
{
	mbedtls_sha1_starts_ret(ctx);
}

void mbedtls_sha1_update(mbedtls_sha1_context *ctx,
                         const unsigned char *input,
                         size_t ilen)
{
	mbedtls_sha1_update_ret(ctx, input, ilen);
}

void mbedtls_sha1_finish(mbedtls_sha1_context *ctx,
                         unsigned char output[20])
{
	mbedtls_sha1_finish_ret(ctx, output);
}

void mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
	mbedtls_sha256_starts_ret(ctx, is224);
}

void mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                           const unsigned char *input,
                           size_t ilen)
{
	mbedtls_sha256_update_ret(ctx, input, ilen);
}

void mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                           unsigned char *output)
{
	mbedtls_sha256_finish_ret(ctx, output);
}

