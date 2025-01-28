/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/display/display_mmu.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_mmu, CONFIG_DISPLAY_LOG_LEVEL);

#define MMU_BLOCK_SIZE 16
#define MMU_X_RES_MAX 512
#define MMU_Y_RES_MAX 512
#define MMU_FIXED_PITCH (MMU_X_RES_MAX * 4)

#define MMU_START_VADDR 0x20000000
#define MMU_END_VADDR   0x21000000

#define SUPPORTED_PIXEL_FORMAT (PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_RGB_888 | \
		PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888)

struct mmu_lut_entry {
	uint32_t enable : 1;
	uint32_t offset : 17;
	uint32_t start : 7;
	uint32_t end : 7;
};

struct mmu_lut {
	struct mmu_lut_entry entries[MMU_Y_RES_MAX];
};

struct mmu_mapping_config {
	uint32_t reg;
	uint32_t vaddr;
};

struct mmu_mapping_data {
	struct display_buffer_descriptor fb_desc;
};

static struct mmu_lut * const mmu_lut[NUM_DISPLAY_MMU_SLOTS] = {
	(struct mmu_lut *)0x21000000,
	(struct mmu_lut *)0x21000800,
	(struct mmu_lut *)0x21001000,
	(struct mmu_lut *)0x21001800,
};

static struct mmu_mapping_config const mmu_config[NUM_DISPLAY_MMU_SLOTS] = {
	{ DISPLAY_MMU_BASEADDR0, 0x20000000, },
	{ DISPLAY_MMU_BASEADDR1, 0x20400000, },
	{ DISPLAY_MMU_BASEADDR2, 0x20800000, },
	{ DISPLAY_MMU_BASEADDR3, 0x20c00000, },
};

static struct mmu_mapping_data mmu_data;

/**
 * Get the square root of a number using binary search
 *
 * Several integer sqrt found here:
 * https://cloud.tencent.com/developer/ask/sof/83082
 *
 * @param num integer which square root should be calculated
 * @param mask optional to skip some iterations if the magnitude of the root is known.
 * Set to 0x8000 by default (set the minimum mask that meet: root < mask or mask * mask >= num).
 * If root < 16: mask = 0x80
 * If root < 256: mask = 0x800
 * Else: mask = 0x8000
 *
 * @retval the square root
 */
static uint16_t sqrti(uint32_t num, uint16_t mask)
{
	uint32_t root = 0;
	uint32_t trial;

	do {
		trial = root + mask; /* or trial = root | mask */
		if (trial * trial <= num) root = trial;
		mask = mask >> 1;
	} while (mask);

	return root;
}

int display_mmu_set_enabled(bool en)
{
	if (en) {
		sys_write32(BIT(16) | BIT(0), DISPLAY_MMU_CTL);
	} else {
		sys_write32(0, DISPLAY_MMU_CTL);
	}

	LOG_INF("display mmu en %d\n", en);
	return 0;
}

int display_mmu_config_lut(uint16_t x_res, uint16_t y_res, uint32_t pixel_format, display_mmu_get_range_t range_cb)
{
	struct mmu_lut_entry *lut_ent = &mmu_lut[0]->entries[0];
	uint8_t px_bytes = display_format_get_bits_per_pixel(pixel_format) / 8;
	int32_t blk_cnt = 0;
	uint16_t y;

	if (x_res > MMU_X_RES_MAX || y_res > MMU_Y_RES_MAX) {
		LOG_ERR("size exceed %d x %d\n", MMU_X_RES_MAX, MMU_Y_RES_MAX);
		return -EINVAL;
	}

	if ((pixel_format & SUPPORTED_PIXEL_FORMAT) == 0) {
		LOG_ERR("unsupported pixel format 0x%x\n", pixel_format);
		return -EINVAL;
	}

	if (range_cb == NULL) {
		LOG_ERR("range_cb is NULL");
		return -EINVAL;
	}

	if (sys_read32(DISPLAY_MMU_CTL) & BIT(0)) {
		LOG_ERR("try again after disable display MMU\n");
		return -EPERM;
	}

	for (y = 0; y < y_res; y++, lut_ent++) {
		int16_t blk_x1, blk_x2;
		uint16_t x1 = 0, x2 = 0;

		range_cb(&x1, &x2, y, x_res, y_res);
		if (x2 >= x_res) {
			x2 = x_res - 1;
		}

		if (x1 > x2) {
			*lut_ent = (struct mmu_lut_entry) { 0 };
			continue;
		}

		blk_x1 = x1 * px_bytes / MMU_BLOCK_SIZE;
		blk_x2 = (x2 * px_bytes + px_bytes - 1) / MMU_BLOCK_SIZE;

		lut_ent->enable = 1;
		lut_ent->start = blk_x1;
		lut_ent->end = blk_x2;
		lut_ent->offset = (blk_cnt - blk_x1) & 0x1ffff;

		blk_cnt += blk_x2 - blk_x1 + 1;

		LOG_DBG("[%d] entry %p 0x%08x, ofs 0x%05x, blk %d %d %d", y, lut_ent,
				*(uint32_t *)lut_ent, lut_ent->offset, blk_cnt, blk_x1, blk_x2);
	}

	for (; y < MMU_Y_RES_MAX; y++, lut_ent++) {
		*lut_ent = (struct mmu_lut_entry) { 0 };
	}

	memcpy(mmu_lut[1], mmu_lut[0], sizeof(*mmu_lut[0]));
	memcpy(mmu_lut[2], mmu_lut[0], sizeof(*mmu_lut[0]));
	memcpy(mmu_lut[3], mmu_lut[0], sizeof(*mmu_lut[0]));

	mmu_data.fb_desc.buf_size = blk_cnt * MMU_BLOCK_SIZE;
	mmu_data.fb_desc.pixel_format = pixel_format;
	mmu_data.fb_desc.width = x_res;
	mmu_data.fb_desc.height = y_res;
	mmu_data.fb_desc.pitch = MMU_FIXED_PITCH;

	LOG_INF("mapping fb desc: buf_size 0x%x, pixel_format 0x%x, size %u x %u, pitch %u\n",
			mmu_data.fb_desc.buf_size, mmu_data.fb_desc.pixel_format,
			mmu_data.fb_desc.width, mmu_data.fb_desc.height, mmu_data.fb_desc.pitch);

	return (int)mmu_data.fb_desc.buf_size;
}

int display_mmu_map_buf(uint8_t slot, display_buffer_t *buffer)
{
	if (slot >= NUM_DISPLAY_MMU_SLOTS) {
		LOG_ERR("invalid slot %d\n", slot);
		return -EINVAL;
	}

	if (buffer->desc.pixel_format != mmu_data.fb_desc.pixel_format ||
		buffer->desc.width != mmu_data.fb_desc.width ||
		buffer->desc.height != mmu_data.fb_desc.height) {
		LOG_ERR("[%d] buffer(%p) geometry changed\n", slot, buffer);
		return -EINVAL;
	}

	if (buffer->desc.buf_size == 0) {
		buffer->desc.buf_size = buffer->desc.pitch * buffer->desc.height;
	}

	if (buffer->desc.buf_size < mmu_data.fb_desc.buf_size) {
		LOG_ERR("[%d] buffer(%p) size %u too small\n", slot, buffer, buffer->desc.buf_size);
		return -EINVAL;
	}

	if ((buffer->addr & (MMU_BLOCK_SIZE - 1)) || (buffer->addr < CONFIG_SRAM_BASE_ADDRESS) ||
		(buffer->addr + mmu_data.fb_desc.buf_size > CONFIG_SRAM_BASE_ADDRESS + CONFIG_SRAM_SIZE * 1024)) {
		LOG_ERR("[%d] buffer(%p) addr 0x%x must be in SRAM\n", slot, buffer, buffer->addr);
		return -EINVAL;
	}

	LOG_INF("[%d] mapping size 0x%x, paddr 0x%x -> vaddr 0x%x\n", slot,
			mmu_data.fb_desc.buf_size, buffer->addr, mmu_config[slot].vaddr);

	sys_write32(buffer->addr, mmu_config[slot].reg);

	buffer->addr = mmu_config[slot].vaddr;
	buffer->desc.pitch = mmu_data.fb_desc.pitch;
	buffer->desc.buf_size = mmu_data.fb_desc.buf_size;

	return 0;
}

bool display_mmu_is_buf_mapped(const display_buffer_t *buffer)
{
	return (buffer->addr >= MMU_START_VADDR && buffer->addr < MMU_END_VADDR) ? true : false;
}

const struct display_buffer_descriptor * display_mmu_get_buf_desc(void)
{
	return &mmu_data.fb_desc;
}

void display_mmu_get_round_range(uint16_t * x1, uint16_t * x2, uint16_t y, uint16_t x_res, uint16_t y_res)
{
	const int16_t cx = x_res - 1; // .1 fixed point
	const int16_t cy = y_res - 1; // .1 fixed point
	const int32_t y1 = y << 1; // .1 fixed point
	const int32_t radius = (cx <= cy) ? cx : cy;
	uint16_t root;

	if (y1 - cy < -radius || y1 - cy > radius) {
		*x1 = 1;
		*x2 = 0;
		return;
	}

	root = sqrti(radius * radius - (y1 - cy) * (y1 - cy), 0x800);
	*x1 = (cx - root) >> 1;
	*x2 = (cx + root) >> 1;
}

void display_mmu_get_rectangle_range(uint16_t * x1, uint16_t * x2, uint16_t y, uint16_t x_res, uint16_t y_res)
{
	*x1 = 0;
	*x2 = x_res - 1;
}
