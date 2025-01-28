/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <assert.h>
#include <string.h>
#include <soc.h>
#include <spicache.h>
#include "jpeg_hw.h"
#include <logging/log.h>

#include <flash/spi_flash.h>
#include <tracing/tracing.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define OUTPUT_TEMP_BUFFER_SIZE (4 * 1024)

//#define CONFIG_BLOCK_MODE 1
#ifdef CONFIG_BLOCK_MODE
#define CONFIG_BLOCK_SIZE  1024
#endif

LOG_MODULE_REGISTER(jpeg_hw_dev, LOG_LEVEL_INF);

#if CONFIG_JPEG_HW_INPUT_SDMA_CHAN < 1 || CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN > 3 \
	|| CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN < 1 || CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN > 3
#  error "CONFIG_JPEG_HW_SDMA_CHAN must in range [1, 3]."
#endif

// sdma config
#define SDMA              		((SDMA_Type *)SDMA_REG_BASE)
#define JPEG_INPUT_CHAN         (&(SDMA->CHAN_CTL[CONFIG_JPEG_HW_INPUT_SDMA_CHAN]))
#define JPEG_INPUT_LINE         (&(SDMA->LINE_CTL[CONFIG_JPEG_HW_INPUT_SDMA_CHAN]))
#define JPEG_OUTPUT_CHAN        (&(SDMA->CHAN_CTL[CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN]))
#define JPEG_OUTPUT_LINE        (&(SDMA->LINE_CTL[CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN]))
#define JPEG_COUPLE_CHAN		(&(SDMA->CHAN_CTL[CONFIG_JPEG_HW_COUPLE_SDMA_CHAN]))
#define JPEG_COUPLE_LINE 		(&(SDMA->LINE_CTL[CONFIG_JPEG_HW_COUPLE_SDMA_CHAN]))

#define __SDMA_IRQ_ID(n)  		(IRQ_ID_SDMA##n)
#define _SDMA_IRQ_ID(n)  		__SDMA_IRQ_ID(n)
#define JPEG_INPUT_IRQ_ID       _SDMA_IRQ_ID(CONFIG_JPEG_HW_INPUT_SDMA_CHAN)
#define JPEG_OUTPUT_IRQ_ID      _SDMA_IRQ_ID(CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN)
#define JPEG_OUTPUT_IRQ_HF_PENDDING  (1 << (CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN + 16))
#define JPEG_OUTPUT_IRQ_TC_PENDDING  (1 << CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN)

// jpeg hw config
#define JPEG_HW              	((JPEG_HW_Type *)JPEG_REG_BASE)

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct jpeg_hw_instance {
	jpeg_hw_instance_callback_t callback;
	void *user_data;
};

struct jpeg_data {
	uint8_t is_busy : 1;
	uint8_t is_locked : 1;
	uint8_t is_decoding : 1;
	struct jpeg_info_t *cfg_info;
	struct jpeg_hw_instance instance;

	struct k_sem wait_sem;
	struct k_mutex mutex;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int _jpeg_hw_poll_unlock(const struct device *dev, int timeout_ms);

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_SOC_NO_PSRAM
__in_section_unique(jpeg.bss.temp_buffer)
uint8_t jpeg_sram_temp_buffer[OUTPUT_TEMP_BUFFER_SIZE];
#endif

static struct jpeg_data jpeg_hw_drv_data;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void _jpeg_hw_dump(void)
{
	printk("CMU_DEVCLKEN0: 0x%x\n", sys_read32(CMU_DEVCLKEN0));
	printk("CMU_MEMCLKEN0: 0x%x\n", sys_read32(CMU_MEMCLKEN0));
	printk("CMU_MEMCLKEN1: 0x%x\n", sys_read32(CMU_MEMCLKEN1));
	printk("CMU_MEMCLKSRC0: 0x%x\n", sys_read32(CMU_MEMCLKSRC0));
	printk("CMU_MEMCLKSRC1: 0x%x\n", sys_read32(CMU_MEMCLKSRC1));
	printk("COREPLL_CTL: 0x%x\n", sys_read32(COREPLL_CTL));
	printk("CMU_SYSCLK: 0x%x\n", sys_read32(CMU_SYSCLK));
	printk("CMU_JPEGCLK: 0x%x\n", sys_read32(CMU_JPEGCLK));

	for (int i = 0; i < 23; i++) {
		printk("reg[%d] : 0x%x \n", i + 1, *(uint32_t *)(JPEG_REG_BASE + i * 4));
	}

	printk("SDMA->IP: 0x%x \n", SDMA->IP);
	printk("SDMA->IE: 0x%x \n", SDMA->IE);
	printk("SDMA->COUPLE_CONFIG: 0x%x \n", SDMA->COUPLE_CONFIG);
	printk("SDMA->COUPLE_BUF_ADDR: 0x%x \n", SDMA->COUPLE_BUF_ADDR);
	printk("SDMA->COUPLE_BUF_SIZE: 0x%x \n", SDMA->COUPLE_BUF_SIZE);
	printk("SDMA->COUPLE_WRITER_POINTER: 0x%x \n", SDMA->COUPLE_WRITER_POINTER);
	printk("SDMA->COUPLE_READ_POINTER: 0x%x \n", SDMA->COUPLE_READ_POINTER);

	printk("JPEG_INPUT_CHAN->CTL: 0x%x \n", JPEG_INPUT_CHAN->CTL);
	printk("JPEG_INPUT_CHAN->SADDR: 0x%x \n", JPEG_INPUT_CHAN->SADDR);
	printk("JPEG_INPUT_CHAN->DADDR: 0x%x \n", JPEG_INPUT_CHAN->DADDR);
	printk("JPEG_INPUT_CHAN->BC: 0x%x \n", JPEG_INPUT_CHAN->BC);
	printk("JPEG_INPUT_CHAN->RC: 0x%x \n", JPEG_INPUT_CHAN->RC);

	printk("JPEG_OUTPUT_CHAN->CTL: 0x%x \n", JPEG_OUTPUT_CHAN->CTL);
	printk("JPEG_OUTPUT_CHAN->SADDR: 0x%x \n", JPEG_OUTPUT_CHAN->SADDR);
	printk("JPEG_OUTPUT_CHAN->DADDR: 0x%x \n", JPEG_OUTPUT_CHAN->DADDR);
	printk("JPEG_OUTPUT_CHAN->BC: 0x%x \n", JPEG_OUTPUT_CHAN->BC);
	printk("JPEG_OUTPUT_CHAN->RC: 0x%x \n", JPEG_OUTPUT_CHAN->RC);

	printk("JPEG_OUTPUT_LINE->LENGTH: 0x%x \n", JPEG_OUTPUT_LINE->LENGTH);
	printk("JPEG_OUTPUT_LINE->COUNT: 0x%x \n", JPEG_OUTPUT_LINE->COUNT);
	printk("JPEG_OUTPUT_LINE->SSTRIDE: 0x%x \n", JPEG_OUTPUT_LINE->SSTRIDE);
	printk("JPEG_OUTPUT_LINE->DSTRIDE: 0x%x \n", JPEG_OUTPUT_LINE->DSTRIDE);

	printk("JPEG_COUPLE_CHAN->CTL: 0x%x \n", JPEG_COUPLE_CHAN->CTL);
	printk("JPEG_COUPLE_CHAN->SADDR: 0x%x \n", JPEG_COUPLE_CHAN->SADDR);
	printk("JPEG_COUPLE_CHAN->DADDR: 0x%x \n", JPEG_COUPLE_CHAN->DADDR);
	printk("JPEG_COUPLE_CHAN->BC: 0x%x \n", JPEG_COUPLE_CHAN->BC);
	printk("JPEG_COUPLE_CHAN->RC: 0x%x \n", JPEG_COUPLE_CHAN->RC);
	printk("JPEG_COUPLE_LINE->LENGTH: 0x%x \n", JPEG_COUPLE_LINE->LENGTH);
	printk("JPEG_COUPLE_LINE->COUNT: 0x%x \n", JPEG_COUPLE_LINE->COUNT);
	printk("JPEG_COUPLE_LINE->SSTRIDE: 0x%x \n", JPEG_COUPLE_LINE->SSTRIDE);
	printk("JPEG_COUPLE_LINE->DSTRIDE: 0x%x \n", JPEG_COUPLE_LINE->DSTRIDE);
}

static int _jpeg_hw_open(const struct device *dev)
{
	struct jpeg_data *data = dev->data;
	int ret = -EBUSY;
	k_mutex_lock(&data->mutex, K_FOREVER);

	if (!data->is_busy) {
		data->is_busy = 1;
		ret = 0;
	}

	k_mutex_unlock(&data->mutex);

	k_sem_init(&data->wait_sem, 0, 1);

	acts_reset_peripheral_assert(RESET_ID_JPEG);

	acts_clock_peripheral_enable(CLOCK_ID_JPEG);

	acts_reset_peripheral_deassert(RESET_ID_JPEG);

	return ret;
}
//config JPEG AC VLC code part1 (reg11)
static void _jpeg_set_ac_vlc_part1(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;
	for (uint8_t i = 0; i < 6; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB0[i] << shiftval);
		shiftval += (i + 2);
	}
	JPEG_HW->JPEG_AC_VLC[0] = val;
}

/* config JPEG AC VLC code part2  (reg12) */
static void _jpeg_set_ac_vlc_part2(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;
	for(uint8_t i = 6; i < 10; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB0[i] << shiftval);
		shiftval += 8;
	}
	JPEG_HW->JPEG_AC_VLC[1] = val;
}

/* config JPEG AC VLC code part3 (reg13) */
static void _jpeg_set_ac_vlc_part3(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;
	for(uint8_t i = 10; i < 14; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB0[i] << shiftval);
		shiftval += 8;
	}
	JPEG_HW->JPEG_AC_VLC[2] = val;
}

/* config JPEG AC VLC code part4 (reg14) */
static void _jpeg_set_ac_vlc_part4(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;
	uint8_t i;

	for(i = 14; i < 16; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB0[i] << shiftval);
		shiftval += 8;
	}

	for(i = 0; i < 4; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB1[i] << shiftval);
		shiftval += (i + 2);
	}


	JPEG_HW->JPEG_AC_VLC[3] = val;

}
/*  config JPEG AC VLC code part5 (reg15) */
static void _jpeg_set_ac_vlc_part5(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 4; i < 8; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB1[i] << shiftval);
		if (i == 6)
			shiftval += 8;
		else
			shiftval += (i + 2);
	}

	JPEG_HW->JPEG_AC_VLC[4] = val;
}

/* config JPEG AC VLC code part6 (reg16) */
static void _jpeg_set_ac_vlc_part6(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 8; i < 12; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB1[i] << shiftval);
		shiftval += 8;
	}

	JPEG_HW->JPEG_AC_VLC[5] = val;
}

/* config JPEG AC VLC code part7 (reg17) */
static void _jpeg_set_ac_vlc_part7(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 12; i < 16; i++) {
		val |= (uint32_t)(cfg_info->AC_TAB1[i] << shiftval);
		shiftval += 8;
	}

	JPEG_HW->JPEG_AC_VLC[6] = val;
}

/* config JPEG DC VLC code part1 (reg18) */
static void _jpeg_set_dc_vlc_part1(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 0; i < 8; i++) {
		val |= (uint32_t)(cfg_info->DC_TAB0[i] << shiftval);
		if (i < 2)
			shiftval += (i + 2);
		else
			shiftval += 4;
	}

	JPEG_HW->JPEG_DC_VLC[0] = val;
}

/* config JPEG DC VLC code part2 (reg19) */
static void _jpeg_set_dc_vlc_part2(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 8; i < 16; i++) {
		val |= (uint32_t)(cfg_info->DC_TAB0[i] << shiftval);
		shiftval += 4;
	}

	JPEG_HW->JPEG_DC_VLC[1] = val;
}

/* config JPEG DC VLC code part3 (reg20) */
static void _jpeg_set_dc_vlc_part3(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 0; i < 8; i++) {
		val |= (uint32_t)(cfg_info->DC_TAB1[i] << shiftval);
		if (i < 2)
			shiftval += (i + 2);
		else
			shiftval += 4;
	}

	JPEG_HW->JPEG_DC_VLC[2] = val;
}

/* config JPEG DC VLC code part4 (reg21) */
static void _jpeg_set_dc_vlc_part4(struct jpeg_info_t *cfg_info)
{
	uint32_t val = 0;
	uint32_t shiftval = 0;

	for(uint8_t i = 8; i < 16; i++) {
		val |= (uint32_t)(cfg_info->DC_TAB1[i] << shiftval);
		shiftval += 4;
	}

	JPEG_HW->JPEG_DC_VLC[3] = val;

}

static void _jpeg_input_config(struct jpeg_info_t *cfg_info)
{
	uint8_t * stream = (uint8_t *)cfg_info->stream_addr + cfg_info->stream_offset;

	SDMA->IE |= (1<<SDMAIE_SDMA2TCIE);

	JPEG_INPUT_CHAN->CTL = (0 << SDMA2CTL_YDAM) | (1 << SDMA2CTL_DSTSL)
						 | (0 << SDMA2CTL_YSAM) | (0 << SDMA2CTL_SRCSL);

	JPEG_INPUT_CHAN->SADDR = buf_is_nor(stream) ?
				(uint32_t)cache_to_uncache_by_master(stream, SPI0_CACHE_MASTER_SDMA) :
				(uint32_t)cache_to_uncache(stream);

	JPEG_INPUT_CHAN->DADDR = 0;//JPG

#ifdef CONFIG_BLOCK_MODE
	JPEG_INPUT_CHAN->BC = CONFIG_BLOCK_SIZE;
#else
	JPEG_INPUT_CHAN->BC = cfg_info->stream_size + 512;
#endif

	cfg_info->stream_offset += JPEG_INPUT_CHAN->BC;
}

static void _jpeg_output_config(struct jpeg_info_t *cfg_info)
{
	uint8_t bytes_per_pix = 0;
	uint16_t crop_width = cfg_info->window_w;
	uint16_t crop_height = cfg_info->window_h;
	uint32_t out_bmp_addr = (uint32_t)cache_to_uncache(cfg_info->output_bmp);

	// RGB888
	if (cfg_info->output_format == 0) {
		bytes_per_pix = 3;
	} else {
		bytes_per_pix = 2;
	}

	SDMA->IE |= (1 << SDMAIE_SDMA3TCIE);

	JPEG_OUTPUT_CHAN->CTL = (1 << SDMA3CTL_STRME)
						| (cfg_info->output_format << SDMA3CTL_JPEG_OUT_FORMAT)
						| (0 << SDMA3CTL_YDAM) | (0 << SDMA3CTL_DSTSL)
						| (0 << SDMA3CTL_YSAM) | (1 << SDMA3CTL_SRCSL);

	JPEG_OUTPUT_CHAN->SADDR = 0;//JPG
	JPEG_OUTPUT_CHAN->DADDR = out_bmp_addr;

	//config output window	(swreg6)(swreg7)(swreg8)
	JPEG_HW->JPEG_WINDOW_START = cfg_info->window_y << 16 | cfg_info->window_x;
	JPEG_HW->JPEG_WINDOW_SIZE = cfg_info->window_h << 16 | cfg_info->window_w;

	JPEG_OUTPUT_CHAN->BC = crop_width * crop_height * bytes_per_pix;
	JPEG_OUTPUT_LINE->LENGTH = crop_width * bytes_per_pix;
	JPEG_OUTPUT_LINE->COUNT = crop_height;
	JPEG_OUTPUT_LINE->SSTRIDE = 0;
	JPEG_OUTPUT_LINE->DSTRIDE = cfg_info->output_stride * bytes_per_pix;

#ifndef CONFIG_SOC_NO_PSRAM
	//psram addr
	if (buf_is_psram_un(out_bmp_addr)) {
		JPEG_OUTPUT_CHAN->DADDR = (uint32_t)&jpeg_sram_temp_buffer;

		JPEG_COUPLE_CHAN->CTL = (1 << SDMA3CTL_STRME);
		JPEG_COUPLE_CHAN->SADDR = (uint32_t)&jpeg_sram_temp_buffer;
		JPEG_COUPLE_CHAN->DADDR = out_bmp_addr;
		JPEG_COUPLE_CHAN->BC = crop_width * crop_height * bytes_per_pix;

		JPEG_COUPLE_LINE->LENGTH = crop_width * bytes_per_pix;
		JPEG_COUPLE_LINE->COUNT = crop_height;
		JPEG_COUPLE_LINE->SSTRIDE = crop_width * bytes_per_pix;
		JPEG_COUPLE_LINE->DSTRIDE = cfg_info->output_stride * bytes_per_pix;

		SDMA->COUPLE_CONFIG = (0x1f << 2) | CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN;
		SDMA->COUPLE_BUF_ADDR = (uint32_t)&jpeg_sram_temp_buffer;
		SDMA->COUPLE_BUF_SIZE = sizeof(jpeg_sram_temp_buffer);
	}
#else
	SDMA->COUPLE_CONFIG = 0;
#endif
}

static int _jpeg_hw_config(const struct device *dev, struct jpeg_info_t *cfg_info)
{
	struct jpeg_data *data = dev->data;
	int ret = 0;
    int bitstreamoffset =0;
    int stream_start_bit = 0;
	int image_block_w = 0;
	int image_block_h = 0;

	if (!cfg_info || cfg_info->yuv_mode != YUV420) {
		LOG_ERR("yuv_mode %d not support\n",cfg_info ? cfg_info->yuv_mode : 0);
		return -EINVAL;
	}

	data->cfg_info = cfg_info;

	// size must align to 16 bytes block
	image_block_w= (cfg_info->image_w + 15) / 16 * 16;
	image_block_h = (cfg_info->image_h + 15) / 16 * 16;

	//config image w & h (swreg2)
	JPEG_HW->JPEG_CONFIG = ((image_block_w >> 4) << 23)
							 | ((image_block_h >> 4) << 14);

	//config yuv to rgb mode bt709
	JPEG_HW->JPEG_CONFIG |= (1 << 8);

	if (cfg_info->output_format == 0) {
		//RB SWAP , BGR888
		//JPEG_HW->JPEG_CONFIG |= (1 << 9);
	} else {
		// RGB 565
		JPEG_HW->JPEG_CONFIG &= (~(1 << 9));
	}

	//config RGB_FIFO_DRQ_MODE
	JPEG_HW->JPEG_CONFIG |= (cfg_info->ds_mode << 7);

	//config Amount of qtabs
	JPEG_HW->JPEG_CONFIG |= (cfg_info->amountOfQTables << 5);

	//config yuv mode ,only support yuv 420 ,no need config
	//JPEG_HW->JPEG_CONFIG |= (cfg_info->yuv_mode << 2);

	// need check (swreg3)
	JPEG_HW->JPEG_BITSTREAM = (bitstreamoffset << 14)
								| (511 << 5) | (stream_start_bit << 0);

	//config stream size (swreg5)
#ifdef CONFIG_BLOCK_MODE
	JPEG_HW->JPEG_STREAM_SIZE = CONFIG_BLOCK_SIZE - 1;
#else
	JPEG_HW->JPEG_STREAM_SIZE = cfg_info->stream_size + 1024 - 1;
#endif

	//not support scale ,no need config
	//JPEG_HW->JPEG_OUTPUT

	//config image resulation (swreg9)
	JPEG_HW->JPEG_RESOLUTION = ((cfg_info->image_h % 16) % 2 ?
		cfg_info->image_h + 1 : cfg_info->image_h)  << 13 | cfg_info->image_w;

	//config  Huffman table select (swreg10)
	JPEG_HW->JPEG_HUFF_TABLE = (cfg_info->scan.Ta[2] << 3)
								| (cfg_info->scan.Ta[1] << 2)
								| (cfg_info->scan.Td[2] << 1)
								| (cfg_info->scan.Td[1] << 0);

	_jpeg_set_ac_vlc_part1(cfg_info);
	_jpeg_set_ac_vlc_part2(cfg_info);
	_jpeg_set_ac_vlc_part3(cfg_info);
	_jpeg_set_ac_vlc_part4(cfg_info);
	_jpeg_set_ac_vlc_part5(cfg_info);
	_jpeg_set_ac_vlc_part6(cfg_info);
	_jpeg_set_ac_vlc_part7(cfg_info);
	_jpeg_set_dc_vlc_part1(cfg_info);
	_jpeg_set_dc_vlc_part2(cfg_info);
	_jpeg_set_dc_vlc_part3(cfg_info);
	_jpeg_set_dc_vlc_part4(cfg_info);

	//switch jpeg mem clk to jpeg
	sys_write32(sys_read32(CMU_MEMCLKSRC1) | (0x1f << 24),CMU_MEMCLKSRC1);

	_jpeg_input_config(cfg_info);
	_jpeg_output_config(cfg_info);

	//_jpeg_hw_dump();

	return ret;
}

static int _jpeg_hw_decode(const struct device *dev)
{
	struct jpeg_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);
#ifdef CONFIG_SPI_FLASH_ACTS
	struct jpeg_info_t *cfg_info = data->cfg_info;

	if(buf_is_nor(cfg_info->stream_addr) && !data->is_locked) {
		spi0_nor_xip_lock();
		data->is_locked = 1;
	}
#endif
	data->is_decoding = 1;
	k_mutex_unlock(&data->mutex);

	k_sem_reset(&data->wait_sem);

	sys_trace_u32(SYS_TRACE_ID_IMG_HW_DECODE, 0);

	// start output sdma
	JPEG_OUTPUT_CHAN->START = 1;

	// start input sdma
	JPEG_INPUT_CHAN->START = 1;

	SDMA->COUPLE_START = 1;

	//start jpeg hw, enable timout and decoder interrupt (swreg0)
	JPEG_HW->JPEG_ENABLE |= 0x05;

	return 0;
}

static int _jpeg_hw_close(const struct device *dev)
{
	struct jpeg_data *data = dev->data;
	int ret = -EINVAL;

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->instance.callback = NULL;
	data->instance.user_data = NULL;
	data->cfg_info = NULL;
	data->is_busy = 0;

	k_mutex_unlock(&data->mutex);

	// reset sdma when jpeg close
	JPEG_OUTPUT_CHAN->START = 0;
	JPEG_INPUT_CHAN->START = 0;
	SDMA->COUPLE_START = 0;

	JPEG_HW->JPEG_ENABLE = 0x00;

	//switch jpeg mem clk to mcu
	sys_write32(sys_read32(CMU_MEMCLKSRC1) & (~(0x1f << 24)),CMU_MEMCLKSRC1);

	acts_clock_peripheral_disable(CLOCK_ID_JPEG);

	return ret;
}


static int _jpeg_hw_poll_unlock(const struct device *dev, int timeout_ms)
{
	struct jpeg_data *data = dev->data;

	if (data->is_decoding) {
		k_timeout_t timeout = (timeout_ms >= 0) ? K_MSEC(timeout_ms) : K_FOREVER;
		return k_sem_take(&data->wait_sem, timeout);
	} else {
		data->is_decoding = 0;
		return 0;
	}
}

static int _jpeg_hw_poll(const struct device *dev, int timeout_ms)
{
	struct jpeg_data *data = dev->data;
#ifdef CONFIG_SPI_FLASH_ACTS
	struct jpeg_info_t *cfg_info = data->cfg_info;
#endif
	int ret;

	k_mutex_lock(&data->mutex, K_FOREVER);
	ret = _jpeg_hw_poll_unlock(dev, timeout_ms);
#ifdef CONFIG_SPI_FLASH_ACTS
	if(buf_is_nor(cfg_info->stream_addr) && data->is_locked) {
		spi0_nor_xip_unlock();
		data->is_locked = 0;
	}
#endif
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int _jpeg_hw_register_callback(const struct device *dev,
									 jpeg_hw_instance_callback_t callback,
									 void *user_data)
{
	struct jpeg_data *data = dev->data;
	int ret = -EINVAL;

	k_mutex_lock(&data->mutex, K_FOREVER);

	if (data->is_busy) {
		data->instance.user_data = user_data;
		data->instance.callback = callback;
		ret = 0;
	}

	k_mutex_unlock(&data->mutex);

	return ret;
}

static void _jpeg_hw_get_capabilities(const struct device *dev,
		struct jpeg_hw_capabilities *capabilities)
{
	capabilities->min_width = 48;
	capabilities->min_height = 48;
	capabilities->max_width = 480;
	capabilities->max_height = 480;
}

static void _jpeg_hw_isr(const void *arg)
{
	const struct device *dev = arg;
	struct jpeg_data *data = dev->data;
	int dec_status = 0;

#ifdef CONFIG_BLOCK_MODE
	int block_len = CONFIG_BLOCK_SIZE;
	struct jpeg_info_t *cfg_info = data->cfg_info;
#endif
	/*first stop jpeg when error avoid interrupt can't clear*/
	JPEG_HW->JPEG_ENABLE = 0;
	if((JPEG_HW->JPEG_INTERRUPT & 0x02) != 0){
		sys_trace_end_call(SYS_TRACE_ID_IMG_HW_DECODE);

		data->is_decoding = 0;
		k_sem_give(&data->wait_sem);
		dec_status = DECODE_FRAME_FINISHED;
	}
#ifdef CONFIG_BLOCK_MODE
	else if((JPEG_HW->JPEG_INTERRUPT & 0x4) != 0) {
		if(cfg_info->stream_offset < cfg_info->stream_size) {
			if(cfg_info->stream_offset + block_len > cfg_info->stream_size) {
				block_len = cfg_info->stream_size - cfg_info->stream_offset;
			}
			JPEG_INPUT_CHAN->SADDR += JPEG_INPUT_CHAN->BC;
			JPEG_INPUT_CHAN->BC = block_len;
			JPEG_HW->JPEG_STREAM_SIZE = block_len - 1;
			cfg_info->stream_offset += block_len;
			dec_status = DECODE_BLOCK_FINISHED;
		} else {
			LOG_ERR("error: %x\n",JPEG_HW->JPEG_INTERRUPT);
		}
	}
#endif
	else if((JPEG_HW->JPEG_INTERRUPT & 0xEC) != 0) {
		sys_trace_end_call(SYS_TRACE_ID_IMG_HW_DECODE);
		LOG_ERR("error: %x\n",JPEG_HW->JPEG_INTERRUPT);
		_jpeg_hw_dump();
		data->is_decoding = 0;
		k_sem_give(&data->wait_sem);
		dec_status = DECODE_DRROR;
	}

	struct jpeg_hw_instance *instance = &data->instance;
	if (instance->callback) {
		instance->callback(dec_status, instance->user_data);
	}
	JPEG_HW->JPEG_INTERRUPT = 0;
}
#if 0
static void jpeg_output_isr(const void *arg)
{
	int dec_status = 0;
	const struct device *dev = arg;
	struct jpeg_data *data = dev->data;
	if(SDMA->IP & JPEG_OUTPUT_IRQ_TC_PENDDING) {
		// reset sdma when jpeg close
		JPEG_OUTPUT_CHAN->START = 0;
		JPEG_INPUT_CHAN->START = 0;
		SDMA->COUPLE_START = 0;
		SDMA->IP |= JPEG_OUTPUT_IRQ_TC_PENDDING;
		dec_status = DECODE_BLOCK_FINISHED;
		k_sem_give(&data->wait_sem);
		struct jpeg_hw_instance *instance = &data->instance;
		if (instance->callback) {
			instance->callback(dec_status, instance->user_data);
		}
	}
}
#endif
DEVICE_DECLARE(jpeg_hw);

static int _jpeg_hw_init(const struct device *dev)
{
	struct jpeg_data *data = dev->data;

	k_mutex_init(&data->mutex);

	SDMA->IE |= BIT(CONFIG_JPEG_HW_INPUT_SDMA_CHAN)
				| BIT(CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN);

	IRQ_CONNECT(IRQ_ID_JPEG, 0, _jpeg_hw_isr, DEVICE_GET(jpeg_hw), 0);

	irq_enable(IRQ_ID_JPEG);
	sys_write32(sys_read32(CMU_MEMCLKEN1) | (0x1f << 24), CMU_MEMCLKEN1);

	//IRQ_CONNECT(JPEG_OUTPUT_IRQ_ID, 0, jpeg_output_isr, DEVICE_GET(jpeg_hw), 0);
	//irq_enable(JPEG_OUTPUT_IRQ_ID);

	clk_set_rate(CLOCK_ID_JPEG, KHZ(CONFIG_JPEG_CLOCK_KHZ));
	data->is_locked = 0;
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int _jpeg_hw_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	struct jpeg_data *data = dev->data;
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		if (data->is_busy) {
			LOG_WRN("jpeg busy (action=%d)", action);
			ret = -EBUSY;
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct jpeg_hw_driver_api jpeg_hw_drv_api = {
	.open = _jpeg_hw_open,
	.close = _jpeg_hw_close,
	.config = _jpeg_hw_config,
	.get_capabilities = _jpeg_hw_get_capabilities,
	.register_callback = _jpeg_hw_register_callback,
	.decode = _jpeg_hw_decode,
	.poll = _jpeg_hw_poll,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#if IS_ENABLED(CONFIG_JPEG_HW_DEV)
DEVICE_DEFINE(jpeg_hw, CONFIG_JPEG_HW_DEV_NAME, &_jpeg_hw_init,
		_jpeg_hw_pm_control, &jpeg_hw_drv_data, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &jpeg_hw_drv_api);
#endif
