/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <string.h>
#include <soc.h>
#include <spicache.h>
#include <drivers/display/display_engine.h>
#ifdef CONFIG_SPI_FLASH_ACTS
#include <flash/spi_flash.h>
#endif
#include <sys/atomic.h>
#include <tracing/tracing.h>
#include <linker/linker-defs.h>
#include "dma2d_lite.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(dma2dlite_dev, LOG_LEVEL_INF);

#if CONFIG_DMA2D_LITE_SDMA_CHAN < 1 || CONFIG_DMA2D_LITE_SDMA_CHAN > 4
#  error "CONFIG_DMA2D_LITE_SDMA_CHAN must in range [1, 4]."
#endif

#define SDMA              ((SDMA_Type *)SDMA_REG_BASE)
#define SDMA_CHAN         (&(SDMA->CHAN_CTL[CONFIG_DMA2D_LITE_SDMA_CHAN]))
#define SDMA_LINE         (&(SDMA->LINE_CTL[CONFIG_DMA2D_LITE_SDMA_CHAN]))
#define __SDMA_IRQ_ID(n)  (IRQ_ID_SDMA##n)
#define _SDMA_IRQ_ID(n)   __SDMA_IRQ_ID(n)
#define SDMA_IRQ_ID       _SDMA_IRQ_ID(CONFIG_DMA2D_LITE_SDMA_CHAN)

#define MAX_WIDTH  (((1 << 16) - 1) / 4)
#define MAX_HEIGHT ((1 << 16) - 1)
#define MAX_PITCH  (((1 << 16) - 1) / 4)
#define MAX_BYTES  ((1 << 20) - 1)

#define SUPPORTED_PIXEL_FORMATS \
	(PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_ABGR_8888 | \
	 PIXEL_FORMAT_XRGB_8888 | PIXEL_FORMAT_XBGR_8888 | \
	 PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_BGR_888 | \
	 PIXEL_FORMAT_BGRA_6666 | PIXEL_FORMAT_RGBA_6666 | \
	 PIXEL_FORMAT_BGRA_5658 | PIXEL_FORMAT_RGBA_5658 | \
	 PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_BGR_565_SWAP | \
	 PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_BGRA_5551 | PIXEL_FORMAT_A8)

struct sdma_instance {
	/* user callback */
	display_engine_instance_callback_t callback;
	void *user_data;

	/* poll sem */
	bool polling;
	struct k_sem sem;
};

struct sdma_data {
	bool waiting;
	int8_t cur_inst;
	uint16_t cmd_seq;
	uint16_t cplt_seq;
	atomic_t open_mask;
	struct sdma_instance instance[CONFIG_DMA2D_LITE_INSTANCE_NUM];

	struct k_sem wait_sem;
	struct k_spinlock lock;

#ifdef CONFIG_SPI_FLASH_ACTS
	bool nor_locked;
#endif
};

static k_spinlock_key_t _wait_idle_locked(struct sdma_data *data);
static int sdma_poll(const struct device *dev, int inst, int timeout_ms);

__de_func static void _set_nor_locked(struct sdma_data *data, bool locked)
{
#ifdef CONFIG_SPI_FLASH_ACTS
	if (locked != data->nor_locked) {
		data->nor_locked = locked;
		LOG_DBG("nor locked %d\n", locked);

		if (locked) {
			spi0_nor_xip_lock();
		} else {
			spi0_nor_xip_unlock();
		}
	}
#endif
}

static bool _validate_inst(struct sdma_data *data, int inst)
{
	if (inst < 0 || inst >= ARRAY_SIZE(data->instance))
		return false;

	return atomic_test_bit(&data->open_mask, inst);
}

static int sdma_open(const struct device *dev, uint32_t flags)
{
	struct sdma_data *data = dev->data;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->instance); i++) {
		if (false == atomic_test_and_set_bit(&data->open_mask, i)) {
			return i;
		}
	}

	LOG_ERR("no free instance");
	return -EMFILE;
}

static int sdma_close(const struct device *dev, int inst)
{
	struct sdma_data *data = dev->data;
	int ret;

	ret = sdma_poll(dev, inst, -1);
	if (ret != -EINVAL) {
		data->instance[inst].callback = NULL;
		data->instance[inst].user_data = NULL;
		atomic_clear_bit(&data->open_mask, inst);
	}

	return ret;
}

__unused static void sdma_dump(void)
{
	printk("SDMA-%d:\n", CONFIG_DMA2D_LITE_SDMA_CHAN);
	printk("                CTL 0x%08x\n", SDMA_CHAN->CTL);
	printk("              SADDR 0x%08x\n", SDMA_CHAN->SADDR);
	printk("              DADDR 0x%08x\n", SDMA_CHAN->DADDR);
	printk("                 BC 0x%08x\n", SDMA_CHAN->BC);
	printk("                 RC 0x%08x\n", SDMA_CHAN->RC);
	printk("        LINE_LENGTH 0x%08x\n", SDMA_LINE->LENGTH);
	printk("         LINE_COUNT 0x%08x\n", SDMA_LINE->COUNT);
	printk("       LINE_SSTRIDE 0x%08x\n", SDMA_LINE->SSTRIDE);
	printk("       LINE_DSTRIDE 0x%08x\n", SDMA_LINE->DSTRIDE);
	printk("        LINE_REMAIN 0x%08x\n", SDMA_LINE->REMAIN);
	printk("BYTE_REMAIN_IN_LINE 0x%08x\n", SDMA_LINE->BYTE_REMAIN_IN_LINE);

#if CONFIG_DMA2D_LITE_SDMA_CHAN < 4
	printk("   COLOR_FILL_DATA0 0x%08x\n", SDMA->COLOR_FILL_DATA[0]);
	printk("   COLOR_FILL_DATA1 0x%08x\n", SDMA->COLOR_FILL_DATA[1]);
	printk("   COLOR_FILL_DATA2 0x%08x\n", SDMA->COLOR_FILL_DATA[2]);
#endif
}

#if CONFIG_DMA2D_LITE_SDMA_CHAN < 4

static uint8_t _convert_color(display_color_t *result, display_color_t color, uint32_t pixel_format)
{
	uint8_t px_bytes = 0;

	switch (pixel_format) {
	case PIXEL_FORMAT_XRGB_8888:
		color.a = 0xFF;
		/* fall through */
	case PIXEL_FORMAT_ARGB_8888:
		result->full = color.full;
		px_bytes = 4;
		break;

	case PIXEL_FORMAT_XBGR_8888:
		color.a = 0xFF;
		/* fall through */
	case PIXEL_FORMAT_ABGR_8888:
		result->full = (color.full & 0xff00ff00) | (color.b << 16) | color.r;
		px_bytes = 4;
		break;

	case PIXEL_FORMAT_BGR_565:
		result->c16[0] = ((color.full & 0x00f80000) >> 8) |
				((color.full & 0x00fc00) >> 5) | ((color.full & 0xf8) >> 3);
		result->c16[1] = result->c16[0];
		px_bytes = 2;
		break;

	case PIXEL_FORMAT_BGR_565_SWAP:
		result->c16[0] = ((color.full & 0x00f80000) >> 19) |
				((color.full & 0x00fc00) >> 5) | ((color.full & 0xf8) << 8);
		result->c16[1] = result->c16[0];
		px_bytes = 2;
		break;

	case PIXEL_FORMAT_RGB_565:
		result->c8[0] = (color.r & 0xF8) | (color.g >> 5);
		result->c8[1] = (color.g << 5) | (color.b >> 3);
		result->c16[1] = result->c16[0];
		px_bytes = 2;
		break;

	case PIXEL_FORMAT_BGRA_5551:
		result->c16[0] = ((color.full & 0x80000000) >> 16) | ((color.full & 0x00f80000) >> 9) |
				((color.full & 0x00f800) >> 6) | ((color.full & 0xf8) >> 3);
		result->c16[1] = result->c16[0];
		px_bytes = 2;
		break;

	case PIXEL_FORMAT_RGB_888:
		result->c8[0] = color.r;
		result->c8[1] = color.g;
		result->c8[2] = color.b;
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_BGR_888:
		result->c8[0] = color.b;
		result->c8[1] = color.g;
		result->c8[2] = color.r;
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_BGRA_5658:
		result->c16[0] = ((color.full & 0x00f80000) >> 8) |
				((color.full & 0x00fc00) >> 5) | ((color.full & 0xf8) >> 3);
		result->c8[2] = color.a;
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_RGBA_5658:
		result->c16[0] = ((color.full & 0x00f80000) >> 19) |
				((color.full & 0x00fc00) >> 5) | ((color.full & 0xf8) << 8);
		result->c8[2] = color.a;
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_BGRA_6666:
		result->c8[0] = ((color.g & 0x0c) << 4) | ((color.b & 0xfc) >> 2);
		result->c8[1] = ((color.r & 0x3c) << 2) | ((color.g & 0xf0) >> 4);
		result->c8[2] = (color.a & 0xfc) | ((color.r & 0xc0) >> 6);
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_RGBA_6666:
		result->c8[0] = ((color.g & 0x0c) << 4) | ((color.r & 0xfc) >> 2);
		result->c8[1] = ((color.b & 0x3c) << 2) | ((color.g & 0xf0) >> 4);
		result->c8[2] = (color.a & 0xfc) | ((color.b & 0xc0) >> 6);
		result->c8[3] = 0;
		px_bytes = 3;
		break;

	case PIXEL_FORMAT_A8:
	default:
		result->c8[0] = result->c8[1] = color.a;
		result->c16[1] = result->c16[0];
		px_bytes = 1;
		break;
	}

	return px_bytes;
}

static int sdma_fill(const struct device *dev, int inst,
		const display_buffer_t *dest, display_color_t color)
{
	struct sdma_data *data = dev->data;
	k_spinlock_key_t key;
	display_color_t value;
	uint32_t dst_addr;
	uint32_t total_bytes;
	uint32_t row_bytes;
	uint8_t px_bytes;
	bool stride_mode;
	int ret = -EINVAL;

	if (false == _validate_inst(data, inst)) {
		return -EINVAL;
	}

	if ((dest->desc.pixel_format & SUPPORTED_PIXEL_FORMATS) == 0) {
		return -EINVAL;
	}

	px_bytes = _convert_color(&value, color, dest->desc.pixel_format);
	row_bytes = (uint32_t)px_bytes * dest->desc.width;
	total_bytes = row_bytes * dest->desc.height;
	if (total_bytes > MAX_BYTES) {
		return -EINVAL;
	}

	stride_mode = (dest->desc.pitch > 0 && dest->desc.pitch != row_bytes);
	if (stride_mode && (dest->desc.pitch > MAX_PITCH || dest->desc.height > MAX_HEIGHT)) {
		return -EINVAL;
	}

	dst_addr = (uint32_t)cache_to_uncache((void *)dest->addr);

	key = _wait_idle_locked(data);

	data->cur_inst = (int8_t)inst;
	ret = data->cmd_seq++;

	/* config registers */
	if (px_bytes == 3) {
		SDMA->COLOR_FILL_DATA[0] = value.full | (value.full << 24);
		SDMA->COLOR_FILL_DATA[1] = (value.full >> 8) | (value.full << 16);
		SDMA->COLOR_FILL_DATA[2] = (value.full >> 16) | (value.full << 8);
	} else {
		SDMA->COLOR_FILL_DATA[0] = value.full;
		SDMA->COLOR_FILL_DATA[1] = value.full;
		SDMA->COLOR_FILL_DATA[2] = value.full;
	}

	if (stride_mode) {
		SDMA_LINE->DSTRIDE = dest->desc.pitch;
		SDMA_LINE->COUNT = dest->desc.height;
		SDMA_LINE->LENGTH = row_bytes;
		SDMA_CHAN->CTL = BIT(24) | 2; /* stride mode; color_fill */
	} else {
		SDMA_CHAN->CTL = 2; /* stride mode; color_fill */
	}

	SDMA_CHAN->BC = total_bytes;
	SDMA_CHAN->DADDR = dst_addr;
	SDMA_CHAN->START = 1;

	k_spin_unlock(&data->lock, key);

	return ret;
}

#else

static int sdma_fill(const struct device *dev, int inst,
		const display_buffer_t *dest, display_color_t color)
{
	return -ENOTSUP;
}

#endif /* CONFIG_DMA2D_LITE_SDMA_CHAN < 4 */

static int sdma_blit(const struct device *dev, int inst,
		const display_buffer_t *dest, const display_buffer_t *src)
{
	struct sdma_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t dst_addr;
	uint32_t src_addr;
	uint32_t total_bytes;
	uint32_t row_bytes;
	uint8_t px_bytes;
	bool stride_mode;
	bool access_nor;
	int ret = -EINVAL;

	if (false == _validate_inst(data, inst)) {
		return -EINVAL;
	}

	if ((dest->desc.pixel_format & SUPPORTED_PIXEL_FORMATS) == 0) {
		return -EINVAL;
	}

	if (dest->desc.pixel_format != src->desc.pixel_format ||
		dest->desc.width != src->desc.width ||
		dest->desc.height != src->desc.height) {
		return -EINVAL;
	}

	px_bytes = display_format_get_bits_per_pixel(dest->desc.pixel_format) / 8;
	row_bytes = (uint32_t)px_bytes * dest->desc.width;
	total_bytes = row_bytes * dest->desc.height;
	if (total_bytes > MAX_BYTES) {
		return -EINVAL;
	}

	stride_mode = (dest->desc.pitch > 0 && dest->desc.pitch != row_bytes) ||
				(src->desc.pitch > 0 && src->desc.pitch != row_bytes);
	if (stride_mode && (src->desc.pitch > MAX_PITCH ||
		dest->desc.pitch > MAX_PITCH || dest->desc.height > MAX_HEIGHT)) {
		return -EINVAL;
	}

	dst_addr = (uint32_t)cache_to_uncache((void *)dest->addr);

	access_nor = buf_is_nor((void *)src->addr);
	if (access_nor) {
		src_addr = (uint32_t)cache_to_uncache_by_master((void *)src->addr, SPI0_CACHE_MASTER_SDMA);
	} else {
		src_addr = (uint32_t)cache_to_uncache((void *)src->addr);
	}

	key = _wait_idle_locked(data);

	if (access_nor) {
		_set_nor_locked(data, true);
	}

	data->cur_inst = (int8_t)inst;
	ret = data->cmd_seq++;

	/* config registers */
	if (stride_mode) {
		SDMA_LINE->COUNT = src->desc.height;
		SDMA_LINE->LENGTH = row_bytes;
		SDMA_LINE->SSTRIDE = (src->desc.pitch > 0) ? src->desc.pitch : row_bytes;
		SDMA_LINE->DSTRIDE = (dest->desc.pitch > 0) ? dest->desc.pitch : row_bytes;
		SDMA_CHAN->CTL = BIT(24); /* stride mode */
	} else {
		SDMA_CHAN->CTL = 0;
	}

	SDMA_CHAN->BC = total_bytes;
	SDMA_CHAN->SADDR = src_addr;
	SDMA_CHAN->DADDR = dst_addr;
	SDMA_CHAN->START = 1;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static inline int sdma_blend(const struct device *dev,
		int inst, const display_buffer_t *dest,
		const display_buffer_t *fg, display_color_t fg_color,
		const display_buffer_t *bg, display_color_t bg_color)
{
	return -ENOTSUP;
}

static int sdma_compose(const struct device *dev, int inst,
		const display_buffer_t *target, const display_layer_t *ovls, int num_ovls)
{
	return -ENOTSUP;
}

static k_spinlock_key_t _wait_idle_locked(struct sdma_data *data)
{
	k_spinlock_key_t key;

	do {
		key = k_spin_lock(&data->lock);
		data->waiting = (data->cur_inst >= 0);
		if (!data->waiting) {
			return key;
		}

		k_spin_unlock(&data->lock, key);
		k_sem_take(&data->wait_sem, K_FOREVER);
	} while (1);
}

static int sdma_poll(const struct device *dev, int inst, int timeout_ms)
{
	struct sdma_data *data = dev->data;
	struct sdma_instance *instance = &data->instance[inst];
	k_spinlock_key_t key;
	int ret = 0;

	if (false == _validate_inst(data, inst)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	instance->polling = (data->cur_inst == inst);
	k_spin_unlock(&data->lock, key);

	if (instance->polling) {
		ret = k_sem_take(&data->instance[inst].sem,
					(timeout_ms >= 0) ? K_MSEC(timeout_ms) : K_FOREVER);
		instance->polling = false;
	}

	return ret;
}

static int sdma_register_callback(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data)
{
	struct sdma_data *data = dev->data;

	if (false == _validate_inst(data, inst)) {
		return -EINVAL;
	}

	data->instance[inst].user_data = user_data;
	data->instance[inst].callback = callback;

	return 0;
}

static void sdma_get_capabilities(const struct device *dev,
		struct display_engine_capabilities *capabilities)
{
	/* considering 32bit pixel format */
	capabilities->max_width = MAX_WIDTH;
	capabilities->max_height = MAX_HEIGHT;
	capabilities->max_pitch = MAX_PITCH;
	capabilities->num_overlays = 1;
	capabilities->support_fill = (CONFIG_DMA2D_LITE_SDMA_CHAN < 4) ? 1 : 0;
	capabilities->support_blend = 0;
	capabilities->support_blend_fg = 0;
	capabilities->support_blend_bg = 0;
	capabilities->supported_output_pixel_formats = SUPPORTED_PIXEL_FORMATS;
	capabilities->supported_input_pixel_formats = SUPPORTED_PIXEL_FORMATS;
	capabilities->supported_rotate_pixel_formats = 0;
}

__de_func static void sdma_isr(const void *arg)
{
	const struct device *dev = arg;
	struct sdma_data *data = dev->data;
	struct sdma_instance *instance = &data->instance[data->cur_inst];

	SDMA->IP = BIT(CONFIG_DMA2D_LITE_SDMA_CHAN);

	_set_nor_locked(data, false);

	if (instance->callback) {
		instance->callback(0, data->cplt_seq, instance->user_data);
	}

	data->cplt_seq++;
	data->cur_inst = -1;

	if (data->waiting) {
		k_sem_give(&data->wait_sem);
	}

	if (instance->polling) {
		k_sem_give(&instance->sem);
	}
}

DEVICE_DECLARE(dma2d_lite);

static int sdma_init(const struct device *dev)
{
	struct sdma_data *data = dev->data;
	int i;

	memset(data, 0, sizeof(*data));
	data->cur_inst = -1;
	k_sem_init(&data->wait_sem, 0, 1);
	for (i = 0; i < ARRAY_SIZE(data->instance); i++) {
		k_sem_init(&data->instance[i].sem, 0, 1);
	}

	SDMA->IE |= BIT(CONFIG_DMA2D_LITE_SDMA_CHAN);

	IRQ_CONNECT(SDMA_IRQ_ID, 0, sdma_isr, DEVICE_GET(dma2d_lite), 0);
	irq_enable(SDMA_IRQ_ID);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int sdma_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct sdma_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
		ret = (data->cur_inst >= 0) ? -EBUSY : 0;
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* Make sure interrupt enabled */
		SDMA->IE |= BIT(CONFIG_DMA2D_LITE_SDMA_CHAN);
		break;
	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_engine_driver_api sdma_drv_api = {
	.open = sdma_open,
	.close = sdma_close,
	.get_capabilities = sdma_get_capabilities,
	.register_callback = sdma_register_callback,
	.fill = sdma_fill,
	.blit = sdma_blit,
	.blend = sdma_blend,
	.compose = sdma_compose,
	.poll = sdma_poll,
};

static struct sdma_data sdma_drv_data __in_section_unique(ram.noinit.drv_sdma);

#if IS_ENABLED(CONFIG_DMA2D_LITE_DEV)
DEVICE_DEFINE(dma2d_lite, CONFIG_DMA2D_LITE_DEV_NAME, &sdma_init,
		sdma_pm_control, &sdma_drv_data, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &sdma_drv_api);
#endif
