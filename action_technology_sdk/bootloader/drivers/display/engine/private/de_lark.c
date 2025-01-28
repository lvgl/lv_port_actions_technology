/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <spicache.h>
#include <drivers/display.h>
#include <drivers/display/display_controller.h>
#include <drivers/display/display_engine.h>
#include <assert.h>
#include <string.h>
#include <sys/byteorder.h>
#include <tracing/tracing.h>

#include "../de_common.h"
#include "../de_device.h"
#include "de_lark.h"

/* TODO: unstable feature */
#define CONFIG_DE_USE_POST_PRECONFIG 0

#define DEV_DBG(...)
#define DEV_ERR(...) printk("[DE][E]: " __VA_ARGS__);

#define MAX_NUM_OVERLAYS  2
#define MIN_WB_WIDTH   3
#define MIN_DEV_WIDTH  2

#define SUPPORTED_OUTPUT_PIXEL_FORMATS \
	(PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_BGR_565 | PIXEL_FORMAT_RGB_888)
#define SUPPORTED_INPUT_PIXEL_FORMATS                                          \
	(SUPPORTED_OUTPUT_PIXEL_FORMATS | PIXEL_FORMAT_BGR_888 |PIXEL_FORMAT_BGRA_6666 | \
	 PIXEL_FORMAT_RGBA_6666 | PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_BGR_565 | \
	 PIXEL_FORMAT_A8 | PIXEL_FORMAT_A4_LE | PIXEL_FORMAT_A1_LE)
#define SUPPORTED_ROTATE_PIXEL_FORMATS (PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_BGR_565)

#define GUESS_PITCH(buffer, bits_per_pixel) \
	((buffer)->desc.pitch > 0 ? (buffer)->desc.pitch : ((buffer)->desc.width * (bits_per_pixel) / 8))

#define GUESS_PITCH_BYTES(buffer, bytes_per_pixel) \
	((buffer)->desc.pitch > 0 ? (buffer)->desc.pitch : ((buffer)->desc.width * (bytes_per_pixel)))

#define DE  ((DE_Type *)DE_REG_BASE)
#define DE_INVALID_ADDR  0x02000000

#define A1_FONT_STRIDE(size) ROUND_UP(size, 8)
#define A4_FONT_STRIDE(size) ROUND_UP(size, 2)
#define A8_FONT_STRIDE(size) (size)

#define IS_WB_REQUIRE_WORD_ALIGN(addr) ((uint32_t)(addr) >= 0x0102C000)

#ifdef CONFIG_DISPLAY_ENGINE_GAMMA_LUT
static int de_config_gamma(const uint8_t *gamma_lut, int len);

static const uint8_t dts_gamma_lut[] = CONFIG_DISPLAY_ENGINE_GAMMA_LUT;
#endif

static bool s_last_ovl_is_wb = false;

__de_func static void de_clk_set_en(bool en)
{
	static int8_t clken_count;

	unsigned int key = irq_lock();

	if (en) {
		if (clken_count++ == 0)
			acts_clock_peripheral_enable(CLOCK_ID_DE);
	} else {
		if (--clken_count == 0)
			acts_clock_peripheral_disable(CLOCK_ID_DE);
	}

	DEV_DBG("de: clk en %d, cnt %d\n", en, clken_count);

	irq_unlock(key);
}

static void de_reset(struct de_data *data)
{
	acts_reset_peripheral_assert(RESET_ID_DE);
	de_clk_set_en(true);
	acts_reset_peripheral_deassert(RESET_ID_DE);

#ifdef CONFIG_DISPLAY_ENGINE_GAMMA_LUT
	de_config_gamma(dts_gamma_lut, ARRAY_SIZE(dts_gamma_lut));
#endif

	DE->MEM_OPT = (DE->MEM_OPT & ~(DE_MEM_BURST_LEN_MASK | DE_SW_FRAME_RST_MASK)) |
		(DE_MEM_BURST_32 | DE_SW_FRAME_RST_LEN(2));
	DE->IRQ_CTL = data->display_sync_en ?
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC | DE_IRQ_DEV_FIFO_HF | DE_IRQ_DEV_FIFO_UDF :
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC;

	data->preconfiged = false;

	/* keep clock disabled */
	de_clk_set_en(false);
}

static int de_open(const struct device *dev, uint32_t flags)
{
	struct de_data *data = dev->data;
	int inst = -EMFILE;

	if (data->display_sync_en && !(flags & DISPLAY_ENGINE_FLAG_POST)) {
		DEV_DBG("de busy in lcd-sync\n");
		return -1;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	inst = de_alloc_instance(flags);
	if (inst < 0)
		goto out_unlock;

	if (data->open_count++ == 0) {
		de_reset(data);
	}

out_unlock:
	k_mutex_unlock(&data->mutex);
	return inst;
}

static int de_close(const struct device *dev, int inst)
{
	struct de_data *data = dev->data;
	int ret = -EBUSY;

	ret = de_instance_poll(inst, -1);
	if (ret < 0)
		return ret;

	k_mutex_lock(&data->mutex, K_FOREVER);

	de_free_instance(inst);

	k_mutex_unlock(&data->mutex);

	return 0;
}

void de_dump(void)
{
	int i;

	de_clk_set_en(true);

	printk("de regs:\n");
	printk("\t ctl                  0x%08x\n", DE->CTL);
	printk("\t gate_ctl             0x%08x\n", DE->GAT_CTL);
	printk("\t reg_ud               0x%08x\n", DE->REG_UD);
	printk("\t irq_ctl              0x%08x\n", DE->IRQ_CTL);
	printk("\t bg_size              0x%08x\n", DE->BG_SIZE);
	printk("\t bg_color             0x%08x\n", DE->BG_COLOR);
	printk("\t mem_opt              0x%08x\n", DE->MEM_OPT);
	printk("\t en                   0x%08x\n", DE->EN);
	printk("\t ctl2                 0x%08x\n", DE->CTL2);

	for (i = 0; i < ARRAY_SIZE(DE->LAYER_CTL); i++) {
		printk("\t l%d_pos               0x%08x\n", i, DE->LAYER_CTL[i].POS);
		printk("\t l%d_size              0x%08x\n", i, DE->LAYER_CTL[i].SIZE);
		printk("\t l%d_addr              0x%08x\n", i, DE->LAYER_CTL[i].ADDR);
		printk("\t l%d_stride            0x%08x\n", i, DE->LAYER_CTL[i].STRIDE);
		printk("\t l%d_length            0x%08x\n", i, DE->LAYER_CTL[i].LENGTH);
		printk("\t l%d_color_gain        0x%08x\n", i, DE->LAYER_CTL[i].COLOR_GAIN);
		printk("\t l%d_color_offset      0x%08x\n", i, DE->LAYER_CTL[i].COLOR_OFFSET);
		printk("\t l%d_def_color         0x%08x\n", i, DE->LAYER_CTL[i].DEF_COLOR);
	}

	printk("\t alpha_ctl            0x%08x\n", DE->ALPHA_CTL);
	printk("\t alpha_pos            0x%08x\n", DE->ALPHA_POS);
	printk("\t alpha_size           0x%08x\n", DE->ALPHA_SIZE);
	printk("\t sta                  0x%08x\n", DE->STA);
	printk("\t gamma_ctl            0x%08x\n", DE->GAMMA_CTL);
	printk("\t dither_ctl           0x%08x\n", DE->DITHER_CTL);
	printk("\t wb_addr              0x%08x\n", DE->WB_MEM_ADR);
	printk("\t wb_stride            0x%08x\n", DE->WB_MEM_STRIDE);
	printk("\t color_fill_pos       0x%08x\n", DE->COLOR_FILL_POS);
	printk("\t color_fill_size      0x%08x\n", DE->COLOR_FILL_SIZE);
	printk("\t fill_color           0x%08x\n", DE->FILL_COLOR);
	printk("\t a148_color           0x%08x\n", DE->A148_COLOR);
	printk("\t a14_ctl              0x%08x\n", DE->A14_CTL);

#if 0
	printk("\t rt_ctl               0x%08x\n", DE->RT_CTL);
	printk("\t rt_img_size          0x%08x\n", DE->RT_IMG_SIZE);
	printk("\t rt_src_addr          0x%08x\n", DE->RT_SRC_ADDR);
	printk("\t rt_src_stride        0x%08x\n", DE->RT_SRC_STRIDE);
	printk("\t rt_dst_addr          0x%08x\n", DE->RT_DST_ADDR);
	printk("\t rt_dst_stride        0x%08x\n", DE->RT_DST_STRIDE);
	printk("\t rt_start_height      0x%08x\n", DE->RT_START_HEIGHT);
	printk("\t rt_sw_x_xy           0x%08x\n", DE->RT_SW_X_XY);
	printk("\t rt_sw_y_xy           0x%08x\n", DE->RT_SW_Y_XY);
	printk("\t rt_sw_x0             0x%08x\n", DE->RT_SW_X0);
	printk("\t rt_sw_y0             0x%08x\n", DE->RT_SW_Y0);
	printk("\t rt_sw_first_dist     0x%08x\n", DE->RT_SW_FIRST_DIST);
	printk("\t rt_r1m2              0x%08x\n", DE->RT_R1M2);
	printk("\t rt_r0m2              0x%08x\n", DE->RT_R0M2);
	printk("\t rt_fill_color        0x%08x\n", DE->RT_FILL_COLOR);
	printk("\t rt_result_x0         0x%08x\n", DE->RT_RESULT_X0);
	printk("\t rt_result_y0         0x%08x\n", DE->RT_RESULT_Y0);
	printk("\t rt_result_first_dist 0x%08x\n", DE->RT_RESULT_FIRST_DIST);
	printk("\t rt_result_src_addr   0x%08x\n", DE->RT_RESULT_SRC_ADDR);
#endif

	de_clk_set_en(false);
}

#ifdef CONFIG_DISPLAY_ENGINE_GAMMA_LUT
/* gamma correction look-up table: First R0-255, then G0-255, last B0-255 */
static int de_config_gamma(const uint8_t *gamma_lut, int len)
{
	int i;

	if (len != 0x300) { /* 256 * 3 */
		return -EINVAL;
	}

	DE->GAT_CTL |= DE_GAMMA_AHB_GATING_EN;

	for (i = 0; i < len; i += 4) {
		DE->GAMMA_CTL = DE_GAMMA_RAM_INDEX(i / 4);
		DE->PATH_GAMMA_RAM = sys_get_le32(&gamma_lut[i]);
	}

	DE->GAT_CTL &= ~DE_GAMMA_AHB_GATING_EN;
	return 0;
}
#endif /* CONFIG_DISPLAY_ENGINE_GAMMA_LUT */

__de_func static int de_config_layer(int layer_idx, display_layer_t *ovl)
{
	static const uint32_t ly_argb_8888[] = { DE_CTL_L0_FORMAT_ARGB8888, DE_CTL_L1_FORMAT_ARGB8888 };
	static const uint32_t ly_rgb_565[]   = { DE_CTL_L0_FORMAT_RGB565,   DE_CTL_L1_FORMAT_RGB565 };
	static const uint32_t ly_rgb_565_le[] = { DE_CTL_L0_FORMAT_RGB565_SWAP, DE_CTL_L1_FORMAT_RGB565_SWAP };
	static const uint32_t ly_argb_6666[] = { DE_CTL_L0_FORMAT_ARGB6666, DE_CTL_L1_FORMAT_ARGB6666 };
	static const uint32_t ly_abgr_6666[] = { DE_CTL_L0_FORMAT_ABGR6666, DE_CTL_L1_FORMAT_ABGR6666 };
	static const uint32_t ly_rgb_888[]   = { DE_CTL_L0_FORMAT_RGB888,   DE_CTL_L1_FORMAT_RGB888 };
	static const uint32_t ly_bgr_888[]   = { DE_CTL_L0_FORMAT_BGR888,   DE_CTL_L1_FORMAT_BGR888 };
	static const uint32_t ly_ax[]        = { DE_CTL_L0_FORMAT_AX,       DE_CTL_L1_FORMAT_AX };

	const display_buffer_t *buffer = ovl->buffer;
	uint32_t format, addr;
	uint16_t bytes_per_line;
	uint8_t no_stride_en, halfword_en, bpp;

	if (buffer == NULL) {
		DE->LAYER_CTL[layer_idx].DEF_COLOR = ovl->color.full;

		/* HW Bug:
		 * de still reads the memory though not used, so just give it
		 * an invalid address which do not cause the bus halt.
		 */
		addr = DE_INVALID_ADDR;
		no_stride_en = 1;
		halfword_en = 0;
		format = DE_CTL_L1_FORMAT_RGB565 | DE_CTL_L1_COLOR_FILL_EN;
		bytes_per_line = ovl->frame.w * 2;
	} else {
		switch (buffer->desc.pixel_format) {
		case PIXEL_FORMAT_ARGB_8888:
			bpp = 32;
			format = ly_argb_8888[layer_idx];
			break;
		case PIXEL_FORMAT_BGR_565:
			bpp = 16;
			format = ly_rgb_565[layer_idx];
			break;
		case PIXEL_FORMAT_RGB_565:
			bpp = 16;
			format = ly_rgb_565_le[layer_idx];
			break;
		case PIXEL_FORMAT_BGRA_6666:
			bpp = 24;
			format = ly_argb_6666[layer_idx];
			break;
		case PIXEL_FORMAT_RGBA_6666:
			bpp = 24;
			format = ly_abgr_6666[layer_idx];
			break;
		case PIXEL_FORMAT_RGB_888:
			bpp = 24;
			format = ly_rgb_888[layer_idx];
			break;
		case PIXEL_FORMAT_BGR_888:
			bpp = 24;
			format = ly_bgr_888[layer_idx];
			break;
		case PIXEL_FORMAT_A8:
			bpp = 8;
			format = ly_ax[layer_idx];
			DE->A148_COLOR = DE_TYPE_A8 | (ovl->color.full & 0xffffff);
			break;
		case PIXEL_FORMAT_A4_LE:
			bpp = 4;
			format = ly_ax[layer_idx];
			DE->A148_COLOR = DE_TYPE_A4 | (ovl->color.full & 0xffffff);
			DE->A14_CTL = DE_A14_FONT(2, 2, ovl->frame.w);
			break;
		case PIXEL_FORMAT_A1_LE:
			bpp = 1;
			format = ly_ax[layer_idx];
			DE->A148_COLOR = DE_TYPE_A1 | (ovl->color.full & 0xffffff);
			DE->A14_CTL = DE_A14_FONT(8, 8, ovl->frame.w);
			break;
		default:
			DEV_ERR("unsupported format %d\n", buffer->desc.pixel_format);
			return -EINVAL;
		}

		addr = buffer->addr;
		bytes_per_line = buffer->desc.pitch;
		halfword_en = (addr & 0x3) ? 1 : 0;
		no_stride_en = !halfword_en && (buffer->desc.pitch == ovl->frame.w * bpp / 8);

		/* Keep default value.
		 *
		 * DE->LAYER_CTL[layer_idx].COLOR_GAIN = DE_L_COLOR_GAIN(0x80, 0x80, 0x80);
		 * DE->LAYER_CTL[layer_idx].COLOR_OFFSET = DE_L_COLOR_GAIN(0, 0, 0);
		 */
	}

	if (layer_idx == 0) {
		DE->CTL |= DE_CTL_L0_EN | format | DE_CTL_L0_NO_STRIDE_EN(no_stride_en) | DE_CTL_L0_HALFWORD_EN(halfword_en);
	} else {
		DE->CTL |= DE_CTL_L1_EN | format | DE_CTL_L1_NO_STRIDE_EN(no_stride_en) | DE_CTL_L1_HALFWORD_EN(halfword_en);
	}

	DE->LAYER_CTL[layer_idx].ADDR = addr & ~0x3;
	DE->LAYER_CTL[layer_idx].POS = DE_L_POS(ovl->frame.x, ovl->frame.y);
	DE->LAYER_CTL[layer_idx].SIZE = DE_L_SIZE(ovl->frame.w, ovl->frame.h);
	DE->LAYER_CTL[layer_idx].STRIDE = bytes_per_line; /* only used by stride mode */
	DE->LAYER_CTL[layer_idx].LENGTH = bytes_per_line * ovl->frame.h; /* only used by no-stride mode */

	return 0;
}

__de_func static int de_apply_overlay_cfg(struct de_data *data, struct de_command_entry *entry, bool preconfig)
{
	de_overlay_cfg_t *cfg = (de_overlay_cfg_t *)entry->cfg;
	display_buffer_t *target = &cfg->target;
	display_layer_t *ovls = cfg->ovls;
	int top_idx = cfg->num_ovls - 1;
	uint32_t target_format;
	uint16_t target_length_per_line;
	uint8_t target_bytes_per_pixel;

	if (target->addr == 0) {
		if (s_last_ovl_is_wb) {
			s_last_ovl_is_wb = false;
			preconfig = false;
			de_reset(data);
		}
	} else {
		s_last_ovl_is_wb = true;
	}

#if CONFIG_DE_USE_POST_PRECONFIG == 0
	preconfig = false;
#endif /* CONFIG_DE_USE_POST_PRECONFIG */

	if (preconfig == false) {
		sys_trace_u32x3(SYS_TRACE_ID_DE_DRAW, entry->cmd, cfg->target_rect.w, cfg->target_rect.h);

		DE->GAT_CTL = DE_LAYER_GATING_EN | DE_OUTPUT_GATING_EN | DE_PATH_GATING_EN;

		if (target->addr == 0 && data->prepare_fn)
			data->prepare_fn(data->prepare_fn_arg, &cfg->target_rect);

		if (data->preconfiged) {
			DEV_DBG("preconfiged\n");
			goto out;
		}
	}

	if (target->addr > 0) {
		target_bytes_per_pixel = display_format_get_bits_per_pixel(target->desc.pixel_format) / 8;
		uint8_t no_stride_en = (target->desc.pitch == target->desc.width * target_bytes_per_pixel);

#ifdef CONFIG_DISPLAY_ENGINE_GAMMA_LUT
		DE->GAMMA_CTL = 0;
#endif
		DE->CTL = DE_CTL_TRANSFER_MODE_TRIGGER | DE_CTL_OUT_MODE_WB |
				DE_CTL_WB_NO_STRIDE_EN(no_stride_en);
		DE->WB_MEM_ADR = target->addr;
		DE->WB_MEM_STRIDE = target->desc.pitch;

		target_format = target->desc.pixel_format;
	} else {
		DE->CTL = DE_CTL_OUT_MODE_DISPLAY | (data->display_sync_en ?
				DE_CTL_TRANSFER_MODE_CONTINUE : DE_CTL_TRANSFER_MODE_TRIGGER);
#ifdef CONFIG_DISPLAY_ENGINE_GAMMA_LUT
		DE->GAMMA_CTL = DE_GAMMA_EN;
#endif

		target_format = data->display_format;
		target_bytes_per_pixel = data->display_bytes_per_pixel;
	}

	switch (target_format) {
	case PIXEL_FORMAT_ARGB_8888:
		DE->CTL |= DE_CTL_OUT_FORMAT_RGB888_WB_ARGB8888;
		break;
	case PIXEL_FORMAT_RGB_888:
		DE->CTL |= DE_CTL_OUT_FORMAT_RGB888;
		break;
	case PIXEL_FORMAT_BGR_565:
	default:
		DE->CTL |= DE_CTL_OUT_FORMAT_RGB565;
		break;
	}

	/* set default bg */
	DE->BG_SIZE = DE_BG_SIZE(cfg->target_rect.w, cfg->target_rect.h);
	DE->BG_COLOR = 0;

	/* Hardware demand */
	target_length_per_line = target_bytes_per_pixel * cfg->target_rect.w;
	if (target_length_per_line >= 32 * 4) {
		DE->MEM_OPT = (DE->MEM_OPT & ~DE_MEM_BURST_LEN_MASK) | DE_MEM_BURST_32;
	} else if (target_length_per_line >= 16 * 4) {
		DE->MEM_OPT = (DE->MEM_OPT & ~DE_MEM_BURST_LEN_MASK) | DE_MEM_BURST_16;
	} else {
		DE->MEM_OPT = (DE->MEM_OPT & ~DE_MEM_BURST_LEN_MASK) | DE_MEM_BURST_8;
	}

	/* 1 ovl at least */
	assert(top_idx >= 0);

	/* 1 fill color layer */
	if (ovls[top_idx].buffer == NULL &&
		(top_idx == 0 || ovls[top_idx].blending == DISPLAY_BLENDING_NONE)) {
		DE->CTL |= DE_CTL_OUT_COLOR_FILL_EN;
		DE->FILL_COLOR = ovls[top_idx].color.full;
		DE->COLOR_FILL_POS = DE_COLOR_FILL_POS(
				ovls[top_idx].frame.x, ovls[top_idx].frame.y);
		DE->COLOR_FILL_SIZE = DE_COLOR_FILL_SIZE(
				ovls[top_idx].frame.w, ovls[top_idx].frame.h);

		if (--top_idx < 0)
			goto out;
	}

	/* 1 background layer */
	if (ovls[0].buffer == NULL) {
		DE->BG_COLOR = ovls[0].color.full;
		ovls++;
		if (--top_idx < 0)
			goto out;
	}

	/* 2 normal ovls */
	assert(top_idx < 2);

	DE->ALPHA_CTL = 0;

	if (top_idx > 0 && ovls[top_idx].blending != DISPLAY_BLENDING_NONE) {
		uint32_t alpha_mode = (ovls[top_idx].blending == DISPLAY_BLENDING_PREMULT) ?
				DE_ALPHA_PREMULTIPLIED : DE_ALPHA_COVERAGE;
		/* don't double use the color alpha */
		uint8_t alpha = ovls[top_idx].buffer ? ovls[top_idx].color.a : 255;

		/* DE will only blend the pixels both inside ALPHA_AREA and L0 & L1 */
		DE->ALPHA_CTL = DE_ALPHA_EN | alpha_mode | DE_ALPHA_PLANE_ALPHA(alpha);
		DE->ALPHA_POS = DE_ALPHA_POS(0, 0);
		DE->ALPHA_SIZE = DE_ALPHA_SIZE(cfg->target_rect.w, cfg->target_rect.h);
	}

	for (; top_idx >= 0; top_idx--) {
		if (de_config_layer(top_idx, &ovls[top_idx]))
			break;
	}

	assert(top_idx == -1);

out:
#if CONFIG_DE_USE_POST_PRECONFIG
	data->preconfiged = preconfig;
	if (preconfig == true) {
		return 0;
	}
#endif

	/* sequence:
	 * 1) modify configuration registers
	 * 2) modify REG_UD=1
	 * 3) modify EN=1
	 * 4) make sure EN is really 1 (read and compare with 1)
	 * 5) REG_UD becomes 0
	 * 6) hw START
	 **/
	DE->REG_UD = 1;

	if (DE->EN == 0) {
		DE->EN = 1;
		while (DE->EN == 0);

		if (data->display_sync_en == 0) {
			k_work_schedule(&data->timeout_work,
					K_MSEC(CONFIG_DISPLAY_ENGINE_COMMAND_TIMEOUT_MS));
		}

		/* however, set write back start */
		DE->CTL2 = DE_CTL2_WB_START;

		/* FIXME: better to start display at DEV FIFO HALFULL (DE_STAT_DEV_FIFO_HF) */
		if (data->display_sync_en == 0 && target->addr == 0 && data->start_fn) {
			data->start_fn(data->start_fn_arg);
		}
	}

	return 0;
}

static int de_apply_rotate_cfg(struct de_data *data, struct de_command_entry *entry)
{
	de_rotate_cfg_t *cfg = (de_rotate_cfg_t *)entry->cfg;

	sys_trace_u32x3(SYS_TRACE_ID_DE_DRAW, DE_CMD_ROTATE_CIRCLE,
			cfg->img_size & 0x1FF, (cfg->img_size >> 16) - cfg->start_height);

	DE->GAT_CTL = DE_ROTATE_GATING_EN;
	DE->EN = 1;

	DE->RT_DST_ADDR = cfg->dst_addr;
	DE->RT_DST_STRIDE = cfg->dst_stride;
	DE->RT_FILL_COLOR = cfg->fill_color;
	DE->RT_IMG_SIZE = cfg->img_size;
	DE->RT_R1M2 = cfg->r1m2;
	DE->RT_R0M2 = cfg->r0m2;
	DE->RT_SW_FIRST_DIST = cfg->sw_first_dist;
	DE->RT_START_HEIGHT = cfg->start_height;
	DE->RT_SRC_ADDR = cfg->src_addr;
	DE->RT_SRC_STRIDE = cfg->src_stride;
	DE->RT_SW_X_XY = cfg->sw_x_xy;
	DE->RT_SW_Y_XY = cfg->sw_y_xy;
	DE->RT_SW_X0 = cfg->sw_x0;
	DE->RT_SW_Y0 = cfg->sw_y0;

	k_work_schedule(&data->timeout_work,
			K_MSEC(CONFIG_DISPLAY_ENGINE_COMMAND_TIMEOUT_MS));

	DE->RT_CTL = cfg->ctl;

	return 0;
}

__de_func static void de_process_next_cmd(struct de_data *data, bool skipped)
{
	struct de_command_entry *entry;
	sys_snode_t *node = NULL;

#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
	if (data->cmd_node) {
		node = sys_slist_peek_head(&data->high_cmd_list);
		if (node == data->cmd_node) {
			sys_slist_remove(&data->high_cmd_list, NULL, data->cmd_node);
		} else {
			assert(data->cmd_node == sys_slist_peek_head(&data->cmd_list));
			sys_slist_remove(&data->cmd_list, NULL, data->cmd_node);
		}

		data->cmd_num--;
	}
#else /* CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE */
	if (data->cmd_node) {
		assert(data->cmd_node == sys_slist_peek_head(&data->cmd_list));
		sys_slist_remove(&data->cmd_list, NULL, data->cmd_node);

		data->cmd_num--;
	}
#endif /* CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE */

	/* find and execute next command */
#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
	node = sys_slist_peek_head(&data->high_cmd_list);
	if (!node)
		node = sys_slist_peek_head(&data->cmd_list);
#else /* CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE */
	node = sys_slist_peek_head(&data->cmd_list);
#endif /* CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE */

	if (node) {
		if (data->cmd_node == NULL) /* new command from idle */
			de_clk_set_en(true);

		entry = CONTAINER_OF(node, struct de_command_entry, node);

		if (skipped == false) {
			switch (entry->cmd) {
			case DE_CMD_ROTATE_CIRCLE:
				de_apply_rotate_cfg(data, entry);
				break;
			default:
				de_apply_overlay_cfg(data, entry, false);
				break;
			}

#if CONFIG_DE_USE_POST_PRECONFIG
			if (s_last_ovl_is_wb == false) {
				sys_snode_t *next_node = node->next;
#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
				if (next_node == NULL && node == sys_slist_peek_head(&data->high_cmd_list)) {
					next_node = sys_slist_peek_head(&data->cmd_list);
				}
#endif
				if (next_node) {
					entry = CONTAINER_OF(next_node, struct de_command_entry, node);
					if (entry->cmd == DE_CMD_COMPOSE) {
						de_apply_overlay_cfg(data, &entry->ovl_cfg, entry->cmd, true);
					}
				}
			}
#endif /* CONFIG_DE_USE_POST_PRECONFIG */
		}
	} else { /* no more commands, become idle */
		de_clk_set_en(false);

		if (data->waiting) {
			data->waiting = 0;
			k_sem_give(&data->wait_sem);
		}
	}

	/* free previous command entry */
	if (data->cmd_node) {
		entry = CONTAINER_OF(data->cmd_node, struct de_command_entry, node);
		de_instance_notify(entry, data->cmd_status);
		de_instance_free_entry(entry);
	}

	/* point to the new command */
	data->cmd_node = node;
}

__de_func static void de_complete_cmd(struct de_data *data, int status)
{
	sys_trace_end_call(SYS_TRACE_ID_DE_DRAW);

	if (data->display_sync_en == 0) {
		DE->EN = 0;
		DE->RT_CTL = RT_STAT_COMPLETE;
		DE->GAT_CTL = 0;
	}

	data->cmd_status = status;
	de_process_next_cmd(data, false);
}

static void de_cleanup_all_cmd(struct de_data *data)
{
	unsigned int key = irq_lock();

	while (data->cmd_num > 0) {
		de_process_next_cmd(data, true);
	}

	irq_unlock(key);
}

__de_func static void de_append_cmd(struct de_data *data, struct de_command_entry *entry, bool high_prio)
{
	unsigned int key = irq_lock();

#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
	if (high_prio) {
		sys_slist_append(&data->high_cmd_list, &entry->node);
	} else {
		sys_slist_append(&data->cmd_list, &entry->node);
	}
#else
	sys_slist_append(&data->cmd_list, &entry->node);
#endif

	++data->cmd_num;

	if (data->cmd_num == 1) {
		de_process_next_cmd(data, false);
	} else if (data->cmd_num == 2) {
		if (entry->cmd == DE_CMD_COMPOSE) {
			de_apply_overlay_cfg(data, entry, true);
		}
	}

	irq_unlock(key);
}

__de_func static int de_insert_overlay_cmd(const struct device *dev, int inst,
		const display_buffer_t *target, const display_layer_t *ovls,
		uint8_t num_ovls, uint8_t cmd)
{
	struct de_data *data = dev->data;
	struct de_command_entry *entry;
	de_overlay_cfg_t *cfg;
	struct display_rect dst_rect;
	uint32_t dst_addr = 0;
	uint8_t bpp, halfword_en;
	uint16_t dst_pitch = 0, ovl_pitch[MAX_NUM_OVERLAYS] = { 0 };
	uint8_t has_ax_format = 0;
	int i;

	if (sizeof(de_overlay_cfg_t) < sizeof(((struct de_command_entry *)0)->cfg)) {
		printk("DE: must increase command cfg size to %u\n", sizeof(de_overlay_cfg_t));
		return -ENOMEM;
	}

	if (target != NULL && data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_DBG("de display-only\n");
		return -EBUSY;
	}

	/* validate layer parameters */
	for (int i = 0; i < num_ovls; i++) {
		if (!ovls[i].buffer) {
			continue;
		}

		if ((ovls[i].buffer->desc.pixel_format & SUPPORTED_INPUT_PIXEL_FORMATS) == 0) {
			DEV_DBG("L%d format %d unsupported\n", i, ovls[i].buffer->desc.pixel_format);
			return -EINVAL;
		}

		if (ovls[i].buffer->desc.pixel_format & (PIXEL_FORMAT_A8 | PIXEL_FORMAT_A4_LE | PIXEL_FORMAT_A1_LE)) {
			if (has_ax_format) {
				DEV_DBG("L%d only one layer can be Ax\n", i);
				return -EINVAL;
			}

			if (ovls[i].buffer->desc.pixel_format == PIXEL_FORMAT_A4_LE && (ovls[i].frame.w & 0x1)) {
				DEV_DBG("L%d frame w of A4 must be mutiple of 2\n", i);
				return -EINVAL;
			}

			if (ovls[i].buffer->desc.pixel_format == PIXEL_FORMAT_A1_LE && (ovls[i].frame.w & 0x7)) {
				DEV_DBG("L%d frame w of A1 must be mutiple of 8\n", i);
				return -EINVAL;
			}

			has_ax_format = 1;
		}

		if (ovls[i].matrix != NULL ||
			ovls[i].frame.w == 0 || ovls[i].frame.w != ovls[i].buffer->desc.width ||
			ovls[i].frame.h == 0 || ovls[i].frame.h != ovls[i].buffer->desc.height) {
			DEV_DBG("L%d transform (%dx%d->%dx%d, %p) unsupported\n",
					i, ovls[i].buffer->desc.width, ovls[i].buffer->desc.height,
					ovls[i].frame.w, ovls[i].frame.h, ovls[i].matrix);
			return -EINVAL;
		}

		bpp = display_format_get_bits_per_pixel(ovls[i].buffer->desc.pixel_format);
		ovl_pitch[i] = GUESS_PITCH(ovls[i].buffer, bpp);

		/* Only rgb565 support half word aligned.
		 * In this case, hardware implemention assumes the origin image is 4-byte aligned,
		 * and then cropped (1, 0, xx, xx) which lead to the half-word aligned address.
		 */
		halfword_en = (ovls[i].buffer->addr & 0x3) ? 1 : 0;
		if (halfword_en > 0) {
			if ((ovls[i].buffer->desc.pixel_format != PIXEL_FORMAT_BGR_565 &&
				 ovls[i].buffer->desc.pixel_format != PIXEL_FORMAT_RGB_565)) {
				DEV_DBG("L%d address 0x%x unaligned\n", i, ovls[i].buffer->addr);
				return -EINVAL;
			}

			if (ovl_pitch[i] < ovls[i].buffer->desc.width * 2 + 2) {
				DEV_DBG("L%d width %u should not greater than pitch %u (hwa=%u)\n", i,
						ovls[i].buffer->desc.width, ovl_pitch[i], halfword_en);
				return -EINVAL;
			}
		}

		if ((ovl_pitch[i] & 0x3) && (halfword_en ||
				ovl_pitch[i] != ovls[i].buffer->desc.width * bpp / 8)) {
			DEV_DBG("L%d pitch %u unaligned (hwa=%u)\n", i, ovl_pitch[i], halfword_en);
			return -EINVAL;
		}
	}

	/* compute target area */
	memcpy(&dst_rect, &ovls[0].frame, sizeof(dst_rect));
	for (i = 1; i < num_ovls; i++) {
		display_rect_merge(&dst_rect, &ovls[i].frame);
	}

	/* validate target parameters */
	if (target) {
		uint8_t bytes_per_pixel;
		uint8_t min_w = MIN_WB_WIDTH;

		if ((target->desc.pixel_format & SUPPORTED_OUTPUT_PIXEL_FORMATS) == 0) {
			DEV_DBG("target format 0x%x unsupported\n", target->desc.pixel_format);
			return -EINVAL;
		}

		bytes_per_pixel = display_format_get_bits_per_pixel(target->desc.pixel_format) / 8;
		dst_pitch = GUESS_PITCH_BYTES(target, bytes_per_pixel);
		dst_addr = target->addr + dst_rect.y * dst_pitch + dst_rect.x * bytes_per_pixel;
		if (buf_is_psram(dst_addr)) {
			dst_addr = (uint32_t)cache_to_uncache((void *)dst_addr);
			min_w = (target->desc.pixel_format != PIXEL_FORMAT_BGR_565) ? 4 : 8;
		}

		if (display_rect_get_width(&dst_rect) < min_w) {
			DEV_DBG("bg width less than %d\n", min_w);
			return -EINVAL;
		}

		/* Only rgb565 support half word aligned.
		 * In this case, hardware implemention assumes the origin target image is 4-byte aligned,
		 * and writing to the area (1, 0, xx, xx) which lead to the half-word aligned address.
		 */
		halfword_en = (dst_addr & 0x3) ? 1 : 0;
		if (target->desc.pixel_format != PIXEL_FORMAT_BGR_565) {
			if (halfword_en > 0) {
				 DEV_DBG("target address 0x%x unaligned\n", dst_addr);
				return -EINVAL;
			}

			if ((dst_pitch & 0x3) && dst_pitch != target->desc.width * bytes_per_pixel) {
				DEV_DBG("target pitch %u unaligned (hwa=%u)\n", dst_pitch, halfword_en);
				return -EINVAL;
			}
		} else if (!(target->desc.width & 0x1)) {
			if (halfword_en || (dst_pitch != target->desc.width  * bytes_per_pixel && (dst_pitch & 0x3))) {
				DEV_DBG("target address 0x%x unaligned (pitch %u, width %u)\n",
						dst_addr, dst_pitch, target->desc.width);
				return -EINVAL;
			}
		}

		/* HW Bug: SPI-Controller only support word aligned access from DE */
		if (IS_WB_REQUIRE_WORD_ALIGN(dst_addr)) {
			uint16_t bytes_to_copy = (target->desc.width * bytes_per_pixel);

			if (halfword_en || (bytes_to_copy & 0x3) || (dst_pitch & 0x3)) {
				DEV_DBG("target width %u & pitch %u not aligned for psram (hwa=%u)\n",
						target->desc.width, dst_pitch, halfword_en);
				return -EINVAL;
			}
		}
	} else {
		if (data->display_format == 0) {
			DEV_DBG("display mode not configured\n");
			return -EINVAL;
		}

		if (display_rect_get_width(&dst_rect) < MIN_DEV_WIDTH) {
			DEV_DBG("bg width less than 2\n");
			return -EINVAL;
		}
	}

	entry = de_instance_alloc_entry(inst);
	if (!entry)
		return -EBUSY;

	entry->cmd = cmd;

	cfg = (de_overlay_cfg_t *)entry->cfg;
	cfg->num_ovls = num_ovls;
	memcpy(cfg->ovls, ovls, num_ovls * sizeof(*ovls));
	memcpy(&cfg->target_rect, &dst_rect, sizeof(dst_rect));

	for (i = 0; i < num_ovls; i++) {
		if (ovls[i].buffer) {
			cfg->ovls[i].buffer = &cfg->bufs[i];
			memcpy(&cfg->bufs[i], ovls[i].buffer, sizeof(*target));
			cfg->bufs[i].desc.pitch = ovl_pitch[i];

			if (buf_is_psram(ovls[i].buffer->addr)) {
				cfg->bufs[i].addr =
					(uint32_t)cache_to_uncache((void *)ovls[i].buffer->addr);
			}
		}

		display_rect_move(&cfg->ovls[i].frame, -dst_rect.x, -dst_rect.y);
	}

	if (target) {
		cfg->target.desc.pixel_format = target->desc.pixel_format;
		cfg->target.desc.pitch = dst_pitch;
		cfg->target.desc.width = dst_rect.w;
		cfg->target.desc.height = dst_rect.h;
		cfg->target.addr = dst_addr;
	} else {
		cfg->target.addr = 0;
	}

	de_append_cmd(data, entry, de_instance_has_flag(inst, DISPLAY_ENGINE_FLAG_HIGH_PRIO));
	return entry->seq;
}

__de_func static int de_fill(const struct device *dev, int inst,
		const display_buffer_t *dest, display_color_t color)
{
	display_buffer_t dest_fake;
	display_layer_t layer;

	/* fake as ARGB8888, min_w >= 3 */
	if (dest->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		uint16_t pitch = GUESS_PITCH_BYTES(dest, 2);

		/* min_w >= 3 */
		if (!(pitch & 0x3) && !(dest->addr & 0x3) &&
		    !(dest->desc.width & 0x1) && (dest->desc.width >= MIN_WB_WIDTH * 2)) {
			dest_fake = (display_buffer_t) {
				.addr = dest->addr,
				.desc = {
					.pixel_format = PIXEL_FORMAT_ARGB_8888,
					.width = dest->desc.width / 2,
					.height = dest->desc.height,
					.pitch = pitch,
				},
			};

			dest = &dest_fake;
			color.full = ((color.r & 0xf8) << 8) | ((color.g & 0xfc) << 3) | (color.b >> 3);
			color.full = (color.full << 16) | color.full;
		}
	}

	layer.buffer = NULL;
	layer.color = color;
	layer.frame = (display_rect_t) { 0, 0, dest->desc.width, dest->desc.height };

	return de_insert_overlay_cmd(dev, inst, dest, &layer, 1, DE_CMD_FILL);
}

__de_func static int de_blit(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src)
{
	display_layer_t layer = {
		.buffer = src,
		.color = { .a = 0xff, .r = 0, .g = 0, .b = 0, },
		.frame = { 0, 0, dest->desc.width, dest->desc.height },
	};

	if (dest->desc.width != src->desc.width || dest->desc.height != src->desc.height)
		return -EINVAL;

	if (buf_is_nor(src->addr))
		return -EACCES;

	return de_insert_overlay_cmd(dev, inst, dest, &layer, 1, DE_CMD_BLIT);
}

__de_func static int de_blend(const struct device *dev,
		int inst, const display_buffer_t *dest,
		const display_buffer_t *fg, display_color_t fg_color,
		const display_buffer_t *bg, display_color_t bg_color)
{
	display_layer_t ovls[2] = {
		{
			.buffer = bg,
			.color = bg_color,
			.frame = { 0, 0, dest->desc.width, dest->desc.height },
		},
		{
			.buffer = fg,
			.color = fg_color,
			.blending = DISPLAY_BLENDING_COVERAGE,
			.frame = { 0, 0, dest->desc.width, dest->desc.height },
		},
	};

	if (bg == NULL || dest->desc.width != bg->desc.width || dest->desc.height != bg->desc.height) {
		return -EINVAL;
	}

	if (fg != NULL && (dest->desc.width != fg->desc.width || dest->desc.height != fg->desc.height)) {
		return -EINVAL;
	}

	if ((fg != NULL && buf_is_nor(fg->addr)) || buf_is_nor(bg->addr)) {
		return -EACCES;
	}

	if (fg != NULL && display_format_is_opaque(fg->desc.pixel_format) && fg_color.a == 255) {
		ovls[1].blending = DISPLAY_BLENDING_NONE;
	}

	return de_insert_overlay_cmd(dev, inst, dest, ovls, 2, fg ? DE_CMD_BLEND : DE_CMD_BLEND_FG);
}

__de_func static int de_compose(const struct device *dev, int inst,
		const display_buffer_t *target, const display_layer_t *ovls, int num_ovls)
{
	uint8_t cmd = target ? DE_CMD_COMPOSE_WB :
			DE_CMD_COMPOSE;

	if (num_ovls > MAX_NUM_OVERLAYS || num_ovls <= 0) {
		DEV_DBG("unsupported ovl num %d\n", num_ovls);
		return -EINVAL;
	}

	return de_insert_overlay_cmd(dev, inst, target, ovls, num_ovls, cmd);
}

static int de_transform(const struct device *dev,
		int inst, const display_buffer_t *dest, const display_buffer_t *src,
		const display_engine_transform_param_t *param)
{
	struct de_data *data = dev->data;
	struct de_command_entry *entry;
	de_rotate_cfg_t *cfg;
	uint16_t outer_diameter = src->desc.width - 1;
	uint8_t bytes_per_pixel;

	if (data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_DBG("de display-only\n");
		return -EBUSY;
	}

	if (!param->is_circle) {
		return -EINVAL;
	}

	if (src->desc.pixel_format != dest->desc.pixel_format ||
		src->desc.width != dest->desc.width ||
		src->desc.width != src->desc.height) {
		DEV_DBG("src and dest must meet circle rotation demand\n");
		return -EINVAL;
	}

	if (param->circle.line_start + dest->desc.height > src->desc.width) {
		DEV_DBG("circle rotation line range exceed\n");
		return -EINVAL;
	}

	entry = de_instance_alloc_entry(inst);
	if (!entry)
		return -EBUSY;

	cfg = (de_rotate_cfg_t *)entry->cfg;

	cfg->start_height = param->circle.line_start;
	cfg->img_size = RT_IMG_WIDTH(dest->desc.width) |
			RT_END_HEIGHT(param->circle.line_start + dest->desc.height);
	cfg->r1m2 = param->circle.outer_radius_sq;
	cfg->r0m2 = param->circle.inner_radius_sq;
	cfg->sw_first_dist = outer_diameter * outer_diameter +
			(outer_diameter - 2 * param->circle.line_start) *
				(outer_diameter - 2 * param->circle.line_start);

	cfg->sw_x0 = param->matrix.tx;
	cfg->sw_y0 = param->matrix.ty;
	cfg->sw_x_xy = RT_SW_DELTA_XY(param->matrix.sx, param->matrix.shy);
	cfg->sw_y_xy = RT_SW_DELTA_XY(param->matrix.shx, param->matrix.sy);

	bytes_per_pixel = display_format_get_bits_per_pixel(src->desc.pixel_format) / 8;
	cfg->src_stride = GUESS_PITCH_BYTES(src, bytes_per_pixel);
	cfg->src_addr = buf_is_psram(src->addr) ?
			(uint32_t)cache_to_uncache((void *)src->addr) : src->addr;
	cfg->src_addr += (cfg->sw_y0 >> 12) * (int32_t)src->desc.pitch + (cfg->sw_x0 >> 12) * bytes_per_pixel;

	cfg->dst_stride = GUESS_PITCH_BYTES(dest, bytes_per_pixel);
	cfg->dst_addr = buf_is_psram(dest->addr) ?
			(uint32_t)cache_to_uncache((void *)dest->addr) : dest->addr;
	cfg->ctl = RT_EN | RT_IRQ_EN | RT_FILTER_BILINEAR | RT_COLOR_FILL_EN;

	if (src->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		cfg->ctl |= RT_FORMAT_RGB565;
		cfg->fill_color = RT_COLOR_RGB_565(param->color.r,
				param->color.g, param->color.b);
	} else {
		cfg->ctl |= RT_FORMAT_ARGB8888;
		cfg->fill_color = param->color.full & 0xFFFFFF;
	}

	entry->cmd = DE_CMD_ROTATE_CIRCLE;
	de_append_cmd(data, entry, de_instance_has_flag(inst, DISPLAY_ENGINE_FLAG_HIGH_PRIO));

	return entry->seq;
}

static int de_poll(const struct device *dev, int inst, int timeout_ms)
{
	if (inst >= 0) {
		return de_instance_poll(inst, timeout_ms);
	}

	return -EINVAL;
}

static int de_register_callback(const struct device *dev,
		int inst, display_engine_instance_callback_t callback, void *user_data)
{
	struct de_data *data = dev->data;
	int res;

	k_mutex_lock(&data->mutex, K_FOREVER);
	res = de_instance_register_callback(inst, callback, user_data);
	k_mutex_unlock(&data->mutex);

	return res;
}

static void de_get_capabilities(const struct device *dev,
		struct display_engine_capabilities *capabilities)
{
	capabilities->num_overlays = MAX_NUM_OVERLAYS;
	capabilities->max_width = 512;
	capabilities->max_height = 512;
	capabilities->max_pitch = 4095;
	capabilities->support_fill = 1;
	capabilities->support_blend = 1;
	capabilities->support_blend_fg = 1;
	capabilities->support_blend_bg = 0;
	capabilities->supported_output_pixel_formats = SUPPORTED_OUTPUT_PIXEL_FORMATS;
	capabilities->supported_input_pixel_formats = SUPPORTED_INPUT_PIXEL_FORMATS;
	capabilities->supported_rotate_pixel_formats = SUPPORTED_ROTATE_PIXEL_FORMATS;
}

static int de_control(const struct device *dev, int cmd, void *arg1, void *arg2)
{
	struct de_data *data = dev->data;
	int ret = 0;

	switch (cmd) {
	case DISPLAY_ENGINE_CTRL_DISPLAY_PREPARE_CB:
		data->prepare_fn_arg = arg2;
		data->prepare_fn = arg1;
		break;
	case  DISPLAY_ENGINE_CTRL_DISPLAY_START_CB:
		data->start_fn_arg = arg2;
		data->start_fn = arg1;
		break;
	case DISPLAY_ENGINE_CTRL_DISPLAY_MODE:
		data->display_format = ((struct display_videomode *)arg1)->pixel_format;
		data->display_bytes_per_pixel = display_format_get_bits_per_pixel(data->display_format) / 8;
		break;
	case DISPLAY_ENGINE_CTRL_DISPLAY_PORT:
		data->display_sync_en =
			(((struct display_videoport *)arg1)->type == DISPLAY_PORT_QSPI_SYNC);
		if (data->display_sync_en) {
			data->op_mode = DISPLAY_ENGINE_MODE_DISPLAY_ONLY;
		}
		break;
	case DISPLAY_ENGINE_CTRL_DISPLAY_SYNC_STOP:
		if (data->display_sync_en) {
			int32_t wait_ms = (int32_t)arg1;

			/* wait command queue empty */
			data->display_sync_en = 0;

			while (data->cmd_num > 0 && wait_ms > 0) {
				k_msleep(2);
				wait_ms -= 2;
			}

			if (data->cmd_num > 0) {
				DEV_ERR("de sync stop timeout (stat=0x%x)\n", DE->STA);
				de_cleanup_all_cmd(data);
				ret = -ETIME;
			}

			data->display_sync_en = 1;

			/* FIXME: any better way to do this ? */
			de_reset(data);
		}
		break;
	case DISPLAY_ENGINE_CTRL_WORK_MODE:
		if (!data->display_sync_en) {
			uint8_t mode = (uint8_t)((intptr_t)arg1);

			if (mode != data->op_mode) {
				data->op_mode = mode;

				if (mode == DISPLAY_ENGINE_MODE_DISPLAY_ONLY) {
					unsigned int key = irq_lock();
					data->waiting = (data->cmd_num > 0);
					irq_unlock(key);

					if (data->waiting) {
						k_sem_take(&data->wait_sem, K_MSEC(5000));
					}
				}
			}
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

__de_func void de_isr(const void *arg)
{
	const struct device *dev = arg;
	struct de_data *data = dev->data;
	uint32_t status = DE->STA;
	uint32_t rt_stat = DE->RT_CTL;
	bool completed = false;

	DE->STA = status;

	if (rt_stat & RT_STAT_COMPLETE) {
		DEV_DBG("de rt complete 0x%08x\n", rt_stat);
		completed = true;
	}

	if (status & (DE_STAT_WB_FTC | DE_STAT_DEV_FTC)) {
		DEV_DBG("de ovl complete 0x%08x\n", status);
		completed = true;
	}

	if (completed) {
		if (data->display_sync_en == 0)
			k_work_cancel_delayable(&data->timeout_work);

		if (data->display_sync_en == 0 || data->cmd_num > 1)
			de_complete_cmd(data, 0);
	}

	if (status & DE_STAT_DEV_FIFO_HF) {
		DEV_DBG("de dev halfull\n");
		if (data->display_sync_en && data->start_fn)
			data->start_fn(data->start_fn_arg);
	}

	if (status & DE_STAT_DEV_FIFO_UDF) {
		if (data->display_sync_en) {
			DEV_ERR("de dev underflow 0x%08x\n", status);
			de_reset(data);
			de_complete_cmd(data, DE_STAT_DEV_FIFO_UDF);
		}
	}

	/* RGB and SPI_QUAD_SYNC have vsync signal*/
	if (status & DE_STAT_DEV_VSYNC) {
		/* TODO: refresh frames */
		DEV_DBG("vsync arrived\n");
	}

	/* update frames for those do not have vsync signal */
	if (status & DE_STAT_PRELINE) {
		/* TODO: refresh frames */
		DEV_DBG("preline arrived\n");
	}
}

static void de_timeout_work_handler(struct k_work *work)
{
	struct de_data *data = CONTAINER_OF(work, struct de_data, timeout_work);

	printk("de timeout\n");
	de_dump();

	de_complete_cmd(data, -ETIME);
}

extern uint32_t drv_de_version_dump(void);

int de_init(const struct device *dev)
{
	struct de_data *data = dev->data;

	drv_de_version_dump();

	/* set invalid value */
	data->display_format = 0;

	k_sem_init(&data->wait_sem, 0, 1);
	k_mutex_init(&data->mutex);
	k_work_init_delayable(&data->timeout_work, de_timeout_work_handler);

	sys_slist_init(&data->cmd_list);
#ifdef CONFIG_DISPLAY_ENGINE_HIHG_PRIO_INSTANCE
	sys_slist_init(&data->high_cmd_list);
#endif

	de_command_pools_init();
	return 0;
}

#ifdef CONFIG_PM_DEVICE
int de_pm_control(const struct device *dev, enum pm_device_action action)
{
	struct de_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_FORCE_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = (data->cmd_num > 0) ? -EBUSY : 0;
		break;
	case PM_DEVICE_ACTION_RESUME:
		de_reset(data);
		break;
	default:
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

const struct display_engine_driver_api de_drv_api = {
	.control = de_control,
	.open = de_open,
	.close = de_close,
	.get_capabilities = de_get_capabilities,
	.register_callback = de_register_callback,
	.fill = de_fill,
	.blit = de_blit,
	.blend = de_blend,
	.compose = de_compose,
	.transform = de_transform,
	.poll = de_poll,
};

struct de_data de_drv_data;
