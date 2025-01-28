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
#include "de_leopard.h"

#define DEV_DBG(...) //printk("[DE][D]: " __VA_ARGS__);
#define DEV_INF(...) //printk("[DE][I]: " __VA_ARGS__);
#define DEV_WRN(...) printk("[DE][W]: " __VA_ARGS__);
#define DEV_ERR(...) printk("[DE][E]: " __VA_ARGS__);

#define CONFIG_DE_RESET_DEV_AFTER_WB 1
#define CONFIG_DE_USE_DEV_HALFULL 0 /* using halfull interrupt to post display */
#define CONFIG_DE_ALPHA_SSAA 0

#ifdef CONFIG_LCDC_Y_FLIP
#  define CONFIG_DE_DEV_Y_FLIP CONFIG_LCDC_Y_FLIP
#else
#  define CONFIG_DE_DEV_Y_FLIP 0
#endif
#define CONFIG_DE_WB_Y_FLIP  0

#define MAX_NUM_OVERLAYS  5
#define MAX_WIDTH  512
#define MAX_HEIGHT 512
#define MAX_PITCH  4095

#define MIN_WB_WIDTH 2

#define SSAA_PIXEL_FORMATS \
	(PIXEL_FORMAT_A2 | PIXEL_FORMAT_A1 | PIXEL_FORMAT_A1_LE)

#define ALPHA_PIXEL_FORMATS \
	(PIXEL_FORMAT_A8 | PIXEL_FORMAT_A4 | PIXEL_FORMAT_A4_LE | \
	 PIXEL_FORMAT_A2 | PIXEL_FORMAT_A1 | PIXEL_FORMAT_A1_LE)

#define ABMP_BITS_PIXEL_FORMATS \
	(PIXEL_FORMAT_A4 | PIXEL_FORMAT_A4_LE | PIXEL_FORMAT_A2 | \
	 PIXEL_FORMAT_A1 | PIXEL_FORMAT_A1_LE | PIXEL_FORMAT_I4 | \
	 PIXEL_FORMAT_I2 | PIXEL_FORMAT_I1)

#define ABMP_PIXEL_FORMATS \
	(ALPHA_PIXEL_FORMATS | PIXEL_FORMAT_I8 | \
	 PIXEL_FORMAT_I4 | PIXEL_FORMAT_I2 | PIXEL_FORMAT_I1)

#define SUPPORTED_SCALING_PIXEL_FORMATS \
	(SUPPORTED_OUTPUT_PIXEL_FORMATS | ABMP_PIXEL_FORMATS | PIXEL_FORMAT_BGR_888 | \
	 PIXEL_FORMAT_BGRA_5658 | PIXEL_FORMAT_BGRA_6666 | PIXEL_FORMAT_RGBA_6666 | PIXEL_FORMAT_BGR_565)

#define SUPPORTED_ROTATE_PIXEL_FORMATS (PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888 | PIXEL_FORMAT_BGR_565)

#define SUPPORTED_OUTPUT_PIXEL_FORMATS \
	(PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_XRGB_8888 | PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_BGR_565)

#define SUPPORTED_INPUT_PIXEL_FORMATS \
	(SUPPORTED_SCALING_PIXEL_FORMATS | PIXEL_FORMAT_BGRA_5551)

#define GUESS_PITCH(buffer, bits_per_pixel) \
	((buffer)->desc.pitch > 0 ? (buffer)->desc.pitch : ((buffer)->desc.width * (bits_per_pixel) / 8))

#define GUESS_PITCH_BYTES(buffer, bytes_per_pixel) \
	((buffer)->desc.pitch > 0 ? (buffer)->desc.pitch : ((buffer)->desc.width * (bytes_per_pixel)))

struct de_priv_data {
#if CONFIG_DE_RESET_DEV_AFTER_WB
	bool last_ovl_is_wb;
#endif
	int8_t clken_count;
	bool nor_locked;
};

#define DE ((DE_Type *)DE_REG_BASE)
static DE_LAYER_Type * const DE_LX[3] = {
	&DE->LX_CTL[0], &DE->LX_CTL[1], &DE->L2_CTL,
};

static struct de_priv_data de_priv_data __in_section_unique(ram.noinit.drv_de.data);

__de_func static void de_clk_set_en(bool en)
{
	unsigned int key = irq_lock();

	if (en) {
		if (de_priv_data.clken_count++ == 0)
			acts_clock_peripheral_enable(CLOCK_ID_DE);
	} else {
		if (--de_priv_data.clken_count == 0)
			acts_clock_peripheral_disable(CLOCK_ID_DE);
	}

	DEV_DBG("de: clk en %d, cnt %d\n", en, clken_count);

	irq_unlock(key);
}

__de_func static void de_set_nor_locked(bool locked)
{
	if (locked != de_priv_data.nor_locked) {
		nor_xip_set_locked(locked);
		de_priv_data.nor_locked = locked;
		DEV_DBG("de: nor locked %d\n", locked);
	}
}

static void de_reset(struct de_data *data)
{
	acts_reset_peripheral_assert(RESET_ID_DE);
	de_clk_set_en(true);
	acts_reset_peripheral_deassert(RESET_ID_DE);

#if CONFIG_DE_RESET_DEV_AFTER_WB
	de_priv_data.last_ovl_is_wb = false;
#endif

	DE->MEM_OPT = (DE->MEM_OPT & ~DE_MEM_BURST_LEN_MASK) | DE_MEM_BURST_32;
	DE->ALPHA_POS = DE_ALPHA_POS(0, 0);

#if CONFIG_DE_USE_DEV_HALFULL
	DE->IRQ_CTL = data->display_sync_en ?
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC | DE_IRQ_DEV_FIFO_HF | DE_IRQ_DEV_FIFO_UDF :
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC | DE_IRQ_DEV_FIFO_HF;
#else
	DE->IRQ_CTL = data->display_sync_en ?
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC | DE_IRQ_DEV_FIFO_UDF :
		DE_IRQ_WB_FTC | DE_IRQ_DEV_FTC;
#endif /* CONFIG_DE_USE_DEV_HALFULL */

	/* keep clock disabled */
	de_clk_set_en(false);
}

static int de_open(const struct device *dev, uint32_t flags)
{
	struct de_data *data = dev->data;
	int inst = -EMFILE;

	if (data->display_sync_en && !(flags & DISPLAY_ENGINE_FLAG_POST)) {
		DEV_WRN("de busy in lcd-sync\n");
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
	printk("\t ctl1                 0x%08x\n", DE->CTL1);

	for (i = 0; i < ARRAY_SIZE(DE_LX); i++) {
		printk("\t l%d_pos               0x%08x\n", i, DE_LX[i]->POS);
		printk("\t l%d_size              0x%08x\n", i, DE_LX[i]->SIZE);
		printk("\t l%d_addr              0x%08x\n", i, DE_LX[i]->ADDR);
		printk("\t l%d_stride            0x%08x\n", i, DE_LX[i]->STRIDE);
		printk("\t l%d_length            0x%08x\n", i, DE_LX[i]->LENGTH);
		printk("\t l%d_def_color         0x%08x\n", i, DE_LX[i]->DEF_COLOR);
	}

	printk("\t alpha_ctl            0x%08x\n", DE->ALPHA_CTL);
	printk("\t alpha_pos            0x%08x\n", DE->ALPHA_POS);
	printk("\t alpha_size           0x%08x\n", DE->ALPHA_SIZE);
	printk("\t sta                  0x%08x\n", DE->STA);
	printk("\t wb_addr              0x%08x\n", DE->WB_MEM_ADR);
	printk("\t wb_stride            0x%08x\n", DE->WB_MEM_STRIDE);
	printk("\t color_fill_pos       0x%08x\n", DE->COLOR_FILL_POS);
	printk("\t color_fill_size      0x%08x\n", DE->COLOR_FILL_SIZE);
	printk("\t fill_color           0x%08x\n", DE->FILL_COLOR);
	printk("\t lx_abmp_ctl          0x%08x\n", DE->LX_ABMP_CTL);

	printk("\t sc_lx_src_img_size   0x%08x\n", DE->SC_LX_SRC_IMG_SIZE);
	printk("\t sc_lx_rate           0x%08x\n", DE->SC_LX_RATE);
	printk("\t sc_lx_pos            0x%08x\n", DE->SC_LX_POS);

#if 1
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
	printk("\t rt_src_img_size      0x%08x\n", DE->RT_SRC_IMG_SIZE);
#endif

	de_clk_set_en(false);
}

__de_func static void de_apply_overlay(struct de_data *data, struct de_command_entry *entry)
{
	de_overlay_cfg_t *cfg = (de_overlay_cfg_t *)entry->cfg;
	display_rect_t area = {
		.x = cfg->x, .y = cfg->y,
		.w = cfg->w, .h = cfg->h,
	};
	int8_t idx;

	sys_trace_u32x3(SYS_TRACE_ID_DE_DRAW, entry->cmd, cfg->w, cfg->h);

	if (cfg->wb_addr == 0) {
#if CONFIG_DE_RESET_DEV_AFTER_WB
		if (de_priv_data.last_ovl_is_wb) {
			de_priv_data.last_ovl_is_wb = false;
			de_reset(data);
		}
#endif

		if (data->prepare_fn)
			data->prepare_fn(data->prepare_fn_arg, &area);
	} else {
#if CONFIG_DE_RESET_DEV_AFTER_WB
		de_priv_data.last_ovl_is_wb = true;
#endif

		DE->WB_MEM_ADR = cfg->wb_addr;
		DE->WB_MEM_STRIDE = cfg->wb_stride;
	}

	if (cfg->ctl & DE_CTL_OUT_COLOR_FILL_EN) {
		DE->FILL_COLOR = cfg->fill_color;
		DE->COLOR_FILL_POS = cfg->color_fill_pos;
		DE->COLOR_FILL_SIZE = cfg->color_fill_size;
	}

	/* set default bg */
	DE->CTL = cfg->ctl;
	DE->CTL1 = cfg->ctl1;
	DE->GAT_CTL = DE_OVERLAY_GAT_EN;
	DE->BG_COLOR = cfg->bg_color;
	DE->BG_SIZE = DE_BG_SIZE(cfg->w, cfg->h);
	DE->ALPHA_CTL = cfg->alpha_ctl;

	if (cfg->n_layers == 0) {
		goto out_exit;
	}

#if 0
	if (cfg->blend_idx > 0) {
		/* keep the same as blend layer area */
		DE->ALPHA_POS = cfg->layers[cfg->blend_idx].pos;
		DE->ALPHA_SIZE = cfg->layers[cfg->blend_idx].size;
	}
#else
	DE->ALPHA_SIZE = DE_BG_SIZE(cfg->w, cfg->h);
#endif
	DE->LX_ABMP_CTL = cfg->lx_abmp_ctl;

	for (idx = 2; cfg->n_layers > 0; cfg->n_layers--, idx--) {
		DE_LX[idx]->POS = cfg->layers[idx].pos;
		DE_LX[idx]->SIZE = cfg->layers[idx].size;
		DE_LX[idx]->ADDR = cfg->layers[idx].addr;
		DE_LX[idx]->STRIDE = cfg->layers[idx].stride;
		DE_LX[idx]->LENGTH = cfg->layers[idx].length;
		DE_LX[idx]->DEF_COLOR = cfg->layers[idx].def_color;
	}

	/* only one rotation regs mapping */
	if (cfg->has_rotation) {
		DE_LX[2]->RT_SRC_SIZE = cfg->rotate.src_size;
		DE_LX[2]->RT_SW_X_XY = cfg->rotate.sw_x_xy;
		DE_LX[2]->RT_SW_Y_XY = cfg->rotate.sw_y_xy;
		DE_LX[2]->RT_SW_X0 = cfg->rotate.sw_x0;
		DE_LX[2]->RT_SW_Y0 = cfg->rotate.sw_y0;
	}

	if (cfg->has_scaling) {
		DE->SC_LX_SRC_IMG_SIZE = cfg->scaling.sc_lx_src_size;
		DE->SC_LX_RATE = cfg->scaling.sc_lx_rate;
		DE->SC_LX_POS = cfg->scaling.sc_lx_pos;
	}

	if (cfg->has_nor_access)
		de_set_nor_locked(true);

out_exit:
	/*
	 * sequence:
	 * 1) modify configuration registers
	 * 2) modify REG_UD=1
	 * 3) modify EN=1
	 * 4) make sure EN is really 1 (read and compare with 1)
	 * 5) REG_UD becomes 0
	 * 6) hw START
	 **/
	DE->REG_UD = 1;

	if (DE->EN == 0) {
		if (data->display_sync_en == 0) {
			k_work_schedule(&data->timeout_work,
					K_MSEC(CONFIG_DISPLAY_ENGINE_COMMAND_TIMEOUT_MS));
		}

		DE->EN = 1;
		while (DE->EN == 0);

		/* REG_UD (active after EN) and CTL2 written must be close enough */
		DE->CTL2 = DE_CTL2_RGB_CVT_LOW | DE_CTL2_START;

#if CONFIG_DE_USE_DEV_HALFULL == 0
		if (cfg->wb_addr == 0 && data->start_fn) {
			data->start_fn(data->start_fn_arg);
		}
#endif /* CONFIG_DE_USE_DEV_HALFULL */
	}
}

__de_func static void de_apply_transform(struct de_data *data, struct de_command_entry *entry)
{
	de_transform_cfg_t *cfg = (de_transform_cfg_t *)entry->cfg;

	sys_trace_u32x3(SYS_TRACE_ID_DE_DRAW, DE_CMD_ROTATE_CIRCLE,
			cfg->img_size & 0x1FF, (cfg->img_size >> 16) - cfg->start_height);

	DE->GAT_CTL = DE_ROTATE_GAT_EN;
	DE->EN = 1;

	DE->RT_DST_ADDR = cfg->dst_addr;
	DE->RT_DST_STRIDE = cfg->dst_stride;
	DE->RT_FILL_COLOR = cfg->fill_color;
	DE->RT_IMG_SIZE = cfg->img_size;

	if (entry->cmd != DE_CMD_ROTATE_FILL) {
		if (entry->cmd == DE_CMD_ROTATE_RECT) {
			DE->RT_SRC_IMG_SIZE = cfg->src_img_size;
			DE->RT_START_HEIGHT = 0;
		} else {
			DE->RT_R1M2 = cfg->r1m2;
			DE->RT_R0M2 = cfg->r0m2;
			DE->RT_SW_FIRST_DIST = cfg->sw_first_dist;
			DE->RT_START_HEIGHT = cfg->start_height;
		}

		DE->RT_SRC_ADDR = cfg->src_addr;
		DE->RT_SRC_STRIDE = cfg->src_stride;
		DE->RT_SW_X_XY = cfg->sw_x_xy;
		DE->RT_SW_Y_XY = cfg->sw_y_xy;
		DE->RT_SW_X0 = cfg->sw_x0;
		DE->RT_SW_Y0 = cfg->sw_y0;
	}

	k_work_schedule(&data->timeout_work,
			K_MSEC(CONFIG_DISPLAY_ENGINE_COMMAND_TIMEOUT_MS));

	DE->RT_CTL = cfg->ctl;
}

__de_func static void de_apply_clut(struct de_data *data, struct de_command_entry *entry)
{
	de_clut_cfg_t *cfg = (de_clut_cfg_t *)entry->cfg;

	//DE->GAT_CTL = DE_OVERLAY_GAT_EN;
	//DE->LX_ABMP_CTL = BIT(3) | BIT(11) | BIT(19); /* all layer clut enabled */
	DE->CLUT_CTL = DE_CLUT_TAB_SEL(cfg->layer_idx) | DE_CLUT_TAB_IDX(0);

	for (; cfg->size > 0; cfg->size--) {
		DE->CLUT_DATA = *cfg->clut++;
	}

	//DE->GAT_CTL = 0;
}

__de_func static void de_process_next_cmd(struct de_data *data, bool skipped)
{
	struct de_command_entry *entry;
	sys_snode_t *node = NULL;
	bool is_sw_cmd;

	do {
		is_sw_cmd = skipped;

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

		/* unlock nor access */
		de_set_nor_locked(false);

		if (node) {
			if (data->cmd_node == NULL) /* new command from idle */
				de_clk_set_en(true);

			entry = CONTAINER_OF(node, struct de_command_entry, node);
			if (skipped == false) {
				switch (entry->cmd) {
				case DE_CMD_ROTATE_FILL:
				case DE_CMD_ROTATE_RECT:
				case DE_CMD_ROTATE_CIRCLE:
					de_apply_transform(data, entry);
					break;
				case DE_CMD_SET_CLUT:
					de_apply_clut(data, entry);
					data->cmd_status = 0;
					is_sw_cmd = true;
					break;
				default:
					de_apply_overlay(data, entry);
					break;
				}
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
	} while (is_sw_cmd);
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

	if (++data->cmd_num == 1) {
		de_process_next_cmd(data, false);
	}

	irq_unlock(key);
}

__de_func static int _validate_layers(const display_layer_t *layers, uint8_t num_ovls, bool *allow_ssaa)
{
	bool has_rotation = false;
	bool has_scaling = false;
	uint8_t bpp;

	/* validate layer parameters */
	for (int i = 0; i < num_ovls; i++) {
		if (!layers[i].buffer) {
			continue;
		}

		if ((layers[i].buffer->desc.pixel_format & SUPPORTED_INPUT_PIXEL_FORMATS) == 0) {
			DEV_WRN("L%d format 0x%x unsupported\n", i, layers[i].buffer->desc.pixel_format);
			return -EINVAL;
		}

		if (layers[i].buffer->desc.width > MAX_WIDTH ||
			layers[i].buffer->desc.height > MAX_HEIGHT ||
			layers[i].buffer->desc.pitch > MAX_PITCH) {
			DEV_WRN("L%d size %dx%d pitch %d too large\n", i, layers[i].buffer->desc.width,
					layers[i].buffer->desc.height, layers[i].buffer->desc.pitch);
			return -EINVAL;
		}

		bpp = display_format_get_bits_per_pixel(layers[i].buffer->desc.pixel_format);
		if ((bpp == 16 && ((layers[i].buffer->addr & 0x1) || (layers[i].buffer->desc.pitch & 0x1))) ||
			(bpp == 32 && ((layers[i].buffer->addr & 0x3) || (layers[i].buffer->desc.pitch & 0x3)))) {
			DEV_WRN("L%d format 0x%x addr 0x%x pitch %d unaligned\n", i,
					layers[i].buffer->desc.pixel_format, layers[i].buffer->addr,
					layers[i].buffer->desc.pitch);
			return -EINVAL;
		}

		if (layers[i].frame.w <= 0 || layers[i].frame.w > MAX_WIDTH ||
			layers[i].frame.h <= 0 || layers[i].frame.h > MAX_HEIGHT) {
			DEV_WRN("L%d size %u x %u invalid\n", i, layers[i].frame.w, layers[i].frame.h);
			return -EINVAL;
		}

		if (layers[i].matrix) {
			if (has_rotation || (i == 0 && num_ovls >= 3)) {
				DEV_WRN("L%d already has or unsupport rotation\n", i);
				return -EINVAL;
			}

			if ((layers[i].buffer->desc.pixel_format & SUPPORTED_ROTATE_PIXEL_FORMATS) == 0) {
				DEV_WRN("L%d format 0x%x not support rotation\n", i, layers[i].buffer->desc.pixel_format);
				return -EINVAL;
			}

			has_rotation = true;

			/* no need to check scaling */
			continue;
		}

		if (layers[i].frame.w != layers[i].buffer->desc.width ||
			layers[i].frame.h != layers[i].buffer->desc.height) {
			if (has_scaling || (i == 0 && num_ovls >= 3)) {
				DEV_WRN("L%d already has or unsupport scaling\n", i);
				return -EINVAL;
			}

			if ((layers[i].buffer->desc.pixel_format & SUPPORTED_SCALING_PIXEL_FORMATS) == 0) {
				DEV_WRN("L%d format 0x%x not support scaling\n", i, layers[i].buffer->desc.pixel_format);
				return -EINVAL;
			}

			/* only need to check scaling down factor */
			if (layers[i].frame.w * 15 < layers[i].buffer->desc.width ||
				layers[i].frame.h * 15 < layers[i].buffer->desc.height) {
				DEV_WRN("L%d scaling exceed [1/15, 4096]: (%u, %u) -> (%u, %u)\n", i,
						layers[i].buffer->desc.width, layers[i].buffer->desc.height,
						layers[i].frame.w, layers[i].frame.h);
				return -EINVAL;
			}

			if (layers[i].buffer->desc.pixel_format & ABMP_BITS_PIXEL_FORMATS) {
				uint16_t width_bits = layers[i].buffer->desc.width * bpp;
				if (layers[i].buffer->desc.pitch == 0 && (width_bits & 0x7)) {
					DEV_WRN("L%d format %x not support both scaling and no-stride\n", i,
							layers[i].buffer->desc.pixel_format);
					return -EINVAL;
				}
			}

			has_scaling = true;
		}

		if (layers[i].buffer->desc.pixel_format & ABMP_BITS_PIXEL_FORMATS) {
			if (layers[i].buffer->px_ofs * bpp >= 8 ||
			    (layers[i].buffer->px_ofs > 0 && layers[i].buffer->desc.pitch == 0)) {
				DEV_WRN("L%d format %x px_ofs %u invalid\n", i,
					layers[i].buffer->desc.pixel_format, layers[i].buffer->px_ofs);
				return -EINVAL;
			}
		}
	}

#if CONFIG_DE_ALPHA_SSAA
	*allow_ssaa = !has_scaling;
#endif

	return 0;
}

__de_func static void _config_layer_scaling(de_overlay_cfg_t *cfg, const display_layer_t *layer, uint8_t idx)
{
	uint16_t x_off = 0;
	uint16_t y_off = 0;
	uint16_t h_rate = 4096, v_rate = 4096;

	if (layer->buffer->desc.width == layer->frame.w && layer->buffer->desc.height == layer->frame.h) {
		x_off = 1024; /* SSAA: offset 1/4 pixel */
		y_off = 1024;
	} else {
		if (layer->frame.w > 1)
			h_rate = ((layer->buffer->desc.width - 1) * 4096 - x_off) / (layer->frame.w - 1);
		if (layer->frame.h > 1)
			v_rate = ((layer->buffer->desc.height - 1) * 4096 - y_off) / (layer->frame.h - 1);
	}

	cfg->scaling.sc_lx_rate = SC_LX_RATE(h_rate, v_rate);
	cfg->scaling.sc_lx_src_size = SC_LX_SRC_IMG_SIZE(layer->buffer->desc.width, layer->buffer->desc.height);
	cfg->scaling.sc_lx_pos = SC_LX_POS(x_off, y_off);
	cfg->has_scaling = 1;
}

__de_func static void _config_layer_rotation(de_overlay_cfg_t *cfg, const display_layer_t *layer,
					     uint8_t idx, uint8_t bpp, bool y_flip)
{
	const display_matrix_t *matrix = layer->matrix;
	const display_buffer_t *buffer = layer->buffer;
	int32_t pitch = GUESS_PITCH(buffer, bpp);

	if (y_flip) {
		cfg->rotate.sw_x0 = matrix->tx + (layer->frame.h - 1) * matrix->shx;
		cfg->rotate.sw_y0 = matrix->ty + (layer->frame.h - 1) * matrix->sy;
		cfg->rotate.sw_y_xy = DE_LX_RT_DELTA_XY(-matrix->shx, -matrix->sy);
	} else {
		cfg->rotate.sw_x0 = matrix->tx;
		cfg->rotate.sw_y0 = matrix->ty;
		cfg->rotate.sw_y_xy = DE_LX_RT_DELTA_XY(matrix->shx, matrix->sy);
	}

	cfg->rotate.sw_x_xy = DE_LX_RT_DELTA_XY(matrix->sx, matrix->shy);
	cfg->layers[idx].addr += (cfg->rotate.sw_y0 >> 12) * pitch +
			(cfg->rotate.sw_x0 >> 12) * bpp / 8;
	cfg->rotate.src_size = DE_LX_RT_SRC_SIZE(buffer->desc.width, buffer->desc.height);

	cfg->has_rotation = 1;
}

__de_func static void _config_layers(de_overlay_cfg_t *cfg, const display_layer_t *layer,
				     uint8_t idx, bool y_flip, bool allow_ssaa)
{
	static const struct {
		uint32_t pixel_format;
		uint32_t lx_format[3];
		uint32_t abmp_format[3];
		uint8_t bpp;
	} layer_formats[] = {
		{
			PIXEL_FORMAT_ARGB_8888,
			{ DE_L0_FORMAT_ARGB_8888,    DE_L1_FORMAT_ARGB_8888,    DE_L2_FORMAT_ARGB_8888 },
			{ 0, 0, 0 }, 32,
		},
		{
			PIXEL_FORMAT_XRGB_8888,
			{ DE_L0_FORMAT_XRGB_8888,    DE_L1_FORMAT_XRGB_8888,    DE_L2_FORMAT_XRGB_8888 },
			{ 0, 0, 0 }, 32,
		},
		{
			PIXEL_FORMAT_BGRA_5658,
			{ DE_L0_FORMAT_ARGB_8565,    DE_L1_FORMAT_ARGB_8565,    DE_L2_FORMAT_ARGB_8565 },
			{ 0, 0, 0 }, 24,
		},
		{
			PIXEL_FORMAT_BGRA_6666,
			{ DE_L0_FORMAT_ARGB_6666,    DE_L1_FORMAT_ARGB_6666,    DE_L2_FORMAT_ARGB_6666 },
			{ 0, 0, 0 }, 24,
		},
		{
			PIXEL_FORMAT_RGBA_6666,
			{ DE_L0_FORMAT_ABGR_6666,    DE_L1_FORMAT_ABGR_6666,    DE_L2_FORMAT_ABGR_6666 },
			{ 0, 0, 0 }, 24,
		},
		{
			PIXEL_FORMAT_BGR_565,
			{ DE_L0_FORMAT_RGB_565,      DE_L1_FORMAT_RGB_565,      DE_L2_FORMAT_RGB_565 },
			{ 0, 0, 0 }, 16,
		},
		{
			PIXEL_FORMAT_RGB_565,
			{ DE_L0_FORMAT_RGB_565_SWAP, DE_L1_FORMAT_RGB_565_SWAP, DE_L2_FORMAT_RGB_565_SWAP },
			{ 0, 0, 0 }, 16,
		},
		{
			PIXEL_FORMAT_RGB_888,
			{ DE_L0_FORMAT_RGB_888,      DE_L1_FORMAT_RGB_888,      DE_L2_FORMAT_RGB_888 },
			{ 0, 0, 0 }, 24,
		},
		{
			PIXEL_FORMAT_BGR_888,
			{ DE_L0_FORMAT_BGR_888,      DE_L1_FORMAT_BGR_888,      DE_L2_FORMAT_BGR_888 },
			{ 0, 0, 0 }, 24,
		},
		{
			PIXEL_FORMAT_BGRA_5551,
			{ DE_L0_FORMAT_ARGB_1555, DE_L1_FORMAT_ARGB_1555, DE_L2_FORMAT_ARGB_1555 },
			{ 0, 0, 0 }, 16,
		},
		{
			PIXEL_FORMAT_I8,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_INDEX_8,             DE_L1_INDEX_8,             DE_L2_INDEX_8 },
			8,
		},
		{
			PIXEL_FORMAT_I4,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_INDEX_4,             DE_L1_INDEX_4,             DE_L2_INDEX_4 },
			4,
		},
		{
			PIXEL_FORMAT_I2,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_INDEX_2,             DE_L1_INDEX_2,             DE_L2_INDEX_2 },
			2,
		},
		{
			PIXEL_FORMAT_I1,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_INDEX_1,             DE_L1_INDEX_1,             DE_L2_INDEX_1 },
			1,
		},
		{
			PIXEL_FORMAT_A8,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_8,             DE_L1_ALPHA_8,             DE_L2_ALPHA_8 },
			8,
		},
		{
			PIXEL_FORMAT_A4,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_4,             DE_L1_ALPHA_4,             DE_L2_ALPHA_4 },
			4,
		},
		{
			PIXEL_FORMAT_A2,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_2,             DE_L1_ALPHA_2,             DE_L2_ALPHA_2 },
			2,
		},
		{
			PIXEL_FORMAT_A1,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_1,             DE_L1_ALPHA_1,             DE_L2_ALPHA_1 },
			1,
		},
		{
			PIXEL_FORMAT_A4_LE,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_4_LE,          DE_L1_ALPHA_4_LE,          DE_L2_ALPHA_4_LE },
			4,
		},
		{
			PIXEL_FORMAT_A1_LE,
			{ DE_L0_FORMAT_ABMP_INDEX,   DE_L1_FORMAT_ABMP_INDEX,   DE_L2_FORMAT_ABMP_INDEX },
			{ DE_L0_ALPHA_1_LE,          DE_L1_ALPHA_1_LE,          DE_L2_ALPHA_1_LE },
			1,
		},
	};

	const display_buffer_t *buffer = layer->buffer;
	bool has_transform = false;
	uint32_t lx_format = 0;
	uint32_t abmp_format = 0;
	uint8_t fill_en = 0;
	uint8_t no_stride_en = 0;
	uint8_t abmp_bitofs = 0;
	uint8_t i;

	if (buffer) {
		uint16_t pitch_bits, width_bits;

		for (i = 0; i < ARRAY_SIZE(layer_formats); i++) {
			if (buffer->desc.pixel_format == layer_formats[i].pixel_format) {
				break;
			}
		}

		__ASSERT(i < ARRAY_SIZE(layer_formats), "format 0x%x support not added", buffer->desc.pixel_format);
		if (i >= ARRAY_SIZE(layer_formats)) { /* Make Klocwork Static Code check happpy */
			k_panic();
			return;
		}

		lx_format = layer_formats[i].lx_format[idx];
		abmp_format = layer_formats[i].abmp_format[idx];
		has_transform = layer->matrix || (buffer->desc.width != layer->frame.w) ||
				(buffer->desc.height != layer->frame.h);

		if ((buffer->desc.pixel_format & ABMP_BITS_PIXEL_FORMATS))
			abmp_bitofs = buffer->px_ofs * layer_formats[i].bpp;

		width_bits = buffer->desc.width * layer_formats[i].bpp;
		pitch_bits = (buffer->desc.pitch > 0) ? (buffer->desc.pitch * 8) : width_bits;

#if CONFIG_DE_ALPHA_SSAA
		if (!has_transform && (buffer->desc.pixel_format & SSAA_PIXEL_FORMATS)) {
			if (allow_ssaa && !cfg->has_scaling && idx > 0 && !(pitch_bits & 0x7)) {
				has_transform = true;
			}
		}
#endif

		no_stride_en = (!y_flip && !has_transform && pitch_bits == width_bits && abmp_bitofs == 0);

		cfg->lx_abmp_ctl |= abmp_format;

		if (buf_is_nor((void *)buffer->addr)) {
			cfg->has_nor_access = 1;
			cfg->layers[idx].addr = (uint32_t)cache_to_uncache_by_master(
				(void *)buffer->addr, SPI0_CACHE_MASTER_DE);
		} else {
			cfg->layers[idx].addr = (uint32_t)cache_to_uncache((void *)buffer->addr);
		}

		cfg->layers[idx].stride = pitch_bits / 8; /* only used by stride mode */
		cfg->layers[idx].length = ((uint32_t)pitch_bits * buffer->desc.height + 7) / 8; /* only used by no-stride mode */
		cfg->layers[idx].def_color = (layer->color.full & 0xFFFFFF); /* donot double use the alpha for the global alpha */
	} else {
		fill_en = 1;
		cfg->layers[idx].addr = 0;
		cfg->layers[idx].def_color = layer->color.full;
	}

	if (idx == 0) {
		cfg->lx_abmp_ctl |= DE_L0_ABMP_BITOFS(abmp_bitofs);
		cfg->ctl |= DE_L0_EN | lx_format | DE_L0_NO_STRIDE_EN(no_stride_en) | DE_L0_COLOR_FILL_EN(fill_en);
	} else {
		if (idx == 1) {
			cfg->lx_abmp_ctl |= DE_L1_ABMP_BITOFS(abmp_bitofs);
			cfg->ctl |= DE_L1_EN | lx_format | DE_L1_NO_STRIDE_EN(no_stride_en) | DE_L1_COLOR_FILL_EN(fill_en);
			if (has_transform)
				cfg->ctl |= layer->matrix ? DE_L1_RT_EN : DE_L1_SC_EN;
		} else {
			cfg->lx_abmp_ctl |= DE_L2_ABMP_BITOFS(abmp_bitofs);
			cfg->ctl1 = DE_L2_EN | lx_format | DE_L2_NO_STRIDE_EN(no_stride_en) | DE_L2_COLOR_FILL_EN(fill_en);
			if (has_transform)
				cfg->ctl1 |= layer->matrix ? DE_L2_RT_EN : DE_L2_SC_EN;
		}

		/* configure blending */
		if (layer->blending != DISPLAY_BLENDING_NONE) {
			if (cfg->blend_idx == 0) {
				uint32_t alpha_mode = (layer->blending == DISPLAY_BLENDING_PREMULT) ?
						DE_ALPHA_PREMULTIPLIED : DE_ALPHA_COVERAGE;
				/* don't double use the color alpha */
				uint8_t alpha = buffer ? layer->color.a : 255;

				/* DE will only blend the pixels both inside ALPHA_AREA and L0 & L1 & L2 */
				cfg->alpha_ctl = DE_ALPHA_EN | alpha_mode | DE_ALPHA_PLANE_ALPHA(alpha);

				cfg->blend_idx = idx;
			}
		}
	}

	if (y_flip) {
		cfg->layers[idx].pos = DE_LX_POS(layer->frame.x - cfg->x,
				cfg->h - (layer->frame.y - cfg->y) - layer->frame.h);
		if (buffer && layer->matrix == NULL) {
			cfg->layers[idx].addr += cfg->layers[idx].stride * (buffer->desc.height - 1);
		}
	} else {
		cfg->layers[idx].pos = DE_LX_POS(layer->frame.x - cfg->x, layer->frame.y - cfg->y);
	}

	cfg->layers[idx].size = DE_LX_SIZE(layer->frame.w, layer->frame.h);

	if (has_transform) {
		if (layer->matrix) {
			_config_layer_rotation(cfg, layer, idx, layer_formats[i].bpp, y_flip);
		} else {
			_config_layer_scaling(cfg, layer, idx);
		}
	}
}

__de_func static void _config_overlay_output(struct de_data *data, de_overlay_cfg_t *cfg,
					     const display_buffer_t *dest,
					     struct display_rect *area, bool y_flip)
{
	uint32_t out_format;

	if (dest) {
		uint8_t bytes_per_pixel = display_format_get_bits_per_pixel(dest->desc.pixel_format) / 8;
		uint16_t pitch = GUESS_PITCH_BYTES(dest, bytes_per_pixel);
		uint8_t no_stride_en = 0;

		cfg->wb_stride = pitch;
		cfg->wb_addr = dest->addr + area->y * cfg->wb_stride + area->x * bytes_per_pixel;
		cfg->wb_addr = (uint32_t)cache_to_uncache((void *)cfg->wb_addr);
		if (y_flip) {
			cfg->wb_addr += cfg->wb_stride * (area->h - 1);
		} else {
			/* no-stride requires both the address and stride 4-byte aligned */
			no_stride_en = !(cfg->wb_addr & 0x3) && !(cfg->wb_stride & 0x3) &&
					(cfg->wb_stride == area->w * bytes_per_pixel);
		}

		cfg->ctl = DE_CTL_TRANSFER_MODE_TRIGGER | DE_CTL_OUT_MODE_WB |
				DE_CTL_WB_NO_STRIDE_EN(no_stride_en) | DE_CTL_OUT_WB_YFLIP_EN(y_flip);

		out_format = dest->desc.pixel_format;
	} else {
		cfg->wb_addr = 0; /* flag */
		cfg->ctl = DE_CTL_OUT_MODE_DISPLAY | (data->display_sync_en ?
				DE_CTL_TRANSFER_MODE_CONTINUE : DE_CTL_TRANSFER_MODE_TRIGGER) |
				DE_CTL_OUT_DISPLY_YFLIP_EN(y_flip);

		out_format = data->display_format;
	}

	switch (out_format) {
	case PIXEL_FORMAT_BGR_565:
		cfg->ctl |= DE_CTL_OUT_FORMAT_RGB565;
		break;
	case PIXEL_FORMAT_ARGB_8888:
		cfg->ctl |= DE_CTL_OUT_FORMAT_RGB888_WB_ARGB8888;
		break;
	case PIXEL_FORMAT_XRGB_8888:
		cfg->ctl |= DE_CTL_OUT_FORMAT_RGB888_WB_XRGB8888;
		break;
	case PIXEL_FORMAT_RGB_888:
	default:
		cfg->ctl |= DE_CTL_OUT_FORMAT_RGB888;
		break;
	}

	/* set default bg */
	cfg->w = area->w;
	cfg->h = area->h;
}

__de_func static int de_insert_overlay_cmd(const struct device *dev, int inst,
					   const display_buffer_t *dest,
					   const display_layer_t *layers, uint8_t num_ovls,
					   uint8_t cmd)
{
	struct de_data *data = dev->data;
	struct de_command_entry *entry;
	de_overlay_cfg_t *cfg;
	struct display_rect dest_rect;
	int8_t top_idx = num_ovls - 1;
	/* FIXME: writeback never uses y-flip ? */
	bool wb_y_flip = dest ? CONFIG_DE_WB_Y_FLIP : false;
	bool dev_y_flip = dest ? false : CONFIG_DE_DEV_Y_FLIP;
	bool allow_ssaa = false; /* using bilinear interpolation */
	int i;

	if (sizeof(de_overlay_cfg_t) > sizeof(((struct de_command_entry *)0)->cfg)) {
		DEV_ERR("must increase command cfg size to %u\n", sizeof(de_overlay_cfg_t));
		return -ENOMEM;
	}

	if (dest != NULL && data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_WRN("de display-only\n");
		return -EBUSY;
	}

	if (_validate_layers(layers, num_ovls, &allow_ssaa)) {
		return -EINVAL;
	}

	/* validate dest area */
	memcpy(&dest_rect, &layers[0].frame, sizeof(dest_rect));
	for (i = 1; i < num_ovls; i++) {
		display_rect_merge(&dest_rect, &layers[i].frame);
	}

	if (display_rect_get_width(&dest_rect) < MIN_WB_WIDTH) {
		DEV_INF("bg width less than 2\n");
		return -EINVAL;
	}

	/* validate dest parameters */
	if (dest) {
		if ((dest->desc.pixel_format & SUPPORTED_OUTPUT_PIXEL_FORMATS) == 0) {
			DEV_WRN("dest format 0x%x unsupported\n", dest->desc.pixel_format);
			return -EINVAL;
		}

		if (dest->desc.width > MAX_WIDTH || dest->desc.height > MAX_HEIGHT || dest->desc.pitch > MAX_PITCH) {
			DEV_WRN("dest size %dx%d pitch %d too large\n", dest->desc.width,
					dest->desc.height, dest->desc.pitch);
			return -EINVAL;
		}
	} else {
		if (data->display_format == 0) {
			DEV_WRN("display mode not configured\n");
			return -EINVAL;
		}
	}

	entry = de_instance_alloc_entry(inst);
	if (!entry)
		return -EBUSY;

	cfg = (de_overlay_cfg_t *)entry->cfg;
	cfg->x = dest_rect.x;
	cfg->y = dest_rect.y;
	cfg->n_layers = 0;
	cfg->blend_idx = 0;
	cfg->has_rotation = 0;
	cfg->has_scaling = 0;
	cfg->has_nor_access = 0;
	cfg->bg_color = 0;
	cfg->ctl1 = 0;
	cfg->alpha_ctl = 0;
	cfg->lx_abmp_ctl = DE_LX_AA_P0_COFF_3_4;

	_config_overlay_output(data, cfg, dest, &dest_rect, wb_y_flip || dev_y_flip);

	/* 1 fill color layer */
	if (layers[top_idx].buffer == NULL &&
		(top_idx == 0 || layers[top_idx].blending == DISPLAY_BLENDING_NONE)) {
		cfg->ctl |= DE_CTL_OUT_COLOR_FILL_EN;
		cfg->fill_color = layers[top_idx].color.full;

		if (dev_y_flip) {
			cfg->color_fill_pos = DE_COLOR_FILL_POS(layers[top_idx].frame.x - dest_rect.x,
					cfg->h - (layers[top_idx].frame.y - dest_rect.y) - layers[top_idx].frame.h);
		} else {
			cfg->color_fill_pos = DE_COLOR_FILL_POS(layers[top_idx].frame.x - dest_rect.x,
					layers[top_idx].frame.y - dest_rect.y);
		}

		cfg->color_fill_size = DE_COLOR_FILL_SIZE(
				layers[top_idx].frame.w, layers[top_idx].frame.h);

		if (--top_idx < 0)
			goto out_exit;
	}

	/* 1 background layer */
	if (layers[0].buffer == NULL &&
		(top_idx == 0 || layers[1].blending == DISPLAY_BLENDING_NONE)) {
		cfg->bg_color = layers[0].color.full;
		layers++;
		if (--top_idx < 0)
			goto out_exit;
	}

	for (int8_t layer_idx = 2; layer_idx >= 0 && top_idx >= 0; top_idx--, layer_idx--) {
		_config_layers(cfg, &layers[top_idx], layer_idx, dev_y_flip, allow_ssaa);
		cfg->n_layers++;
	}

	if (cfg->blend_idx > 0 && dest == NULL)
		cfg->alpha_ctl |= DE_ALPHA_FCV_EN;

out_exit:
	entry->cmd = cmd;
	de_append_cmd(data, entry, de_instance_has_flag(inst, DISPLAY_ENGINE_FLAG_HIGH_PRIO));
	return entry->seq;
}

__de_func static int de_fill(const struct device *dev, int inst, const display_buffer_t *dest,
			     display_color_t color)
{
	struct de_data *data = dev->data;
	display_layer_t layer = {
		.buffer = NULL,
		.color = color,
		.frame = (display_rect_t) { 0, 0, dest->desc.width, dest->desc.height },
	};

	if (data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_WRN("de display-only\n");
		return -EBUSY;
	}

	if (dest->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		uint16_t pitch;

		if (dest->desc.width < 2 || (dest->desc.pitch & 0x1)) {
			DEV_INF("fill width less than 2\n");
			return -EINVAL;
		}

		pitch = GUESS_PITCH_BYTES(dest, 2);

		if (!(pitch & 0x3) && !(dest->addr & 0x3) &&
		    !(dest->desc.width & 0x1) && (dest->desc.width >= MIN_WB_WIDTH * 2)) {
			display_buffer_t dest_argb8888 = {
				.addr = dest->addr,
				.desc = {
					.pixel_format = PIXEL_FORMAT_ARGB_8888,
					.width = dest->desc.width / 2,
					.height = dest->desc.height,
					.pitch = pitch,
				},
			};

			layer.frame.w = dest_argb8888.desc.width;
			layer.color.c16[0] = ((color.r & 0xf8) << 8) | ((color.g & 0xfc) << 3) | (color.b >> 3);
			layer.color.c16[1] = layer.color.c16[0];
			return de_insert_overlay_cmd(dev, inst, &dest_argb8888, &layer, 1, DE_CMD_FILL);
		}

		/* BUGFIX: cannot access crossing RAM9 and RAM10 (0x30000000 - ?) */
		uint32_t dst_end = dest->addr + (dest->desc.height - 1) * pitch + dest->desc.width * 2;
		if (dest->addr >= 0x30000000 || dst_end <= 0x30000000) {
			struct de_command_entry *entry = de_instance_alloc_entry(inst);
			if (!entry)
				return -EBUSY;

			de_transform_cfg_t *cfg = (de_transform_cfg_t *)entry->cfg;
			cfg->img_size = RT_IMG_WIDTH(dest->desc.width) | RT_END_HEIGHT(dest->desc.height);
			cfg->dst_addr = (uint32_t)cache_to_uncache((void *)dest->addr);
			cfg->dst_stride = pitch;
			cfg->fill_color = RT_COLOR_RGB_565(color.r, color.g, color.b);
			cfg->ctl = RT_EN | RT_IRQ_EN | RT_COLOR_FILL_START | RT_COLOR_FILL_BURST_16;

			entry->cmd = DE_CMD_ROTATE_FILL;
			de_append_cmd(data, entry, de_instance_has_flag(inst, DISPLAY_ENGINE_FLAG_HIGH_PRIO));

			return entry->seq;
		}
	}

	return de_insert_overlay_cmd(dev, inst, dest, &layer, 1, DE_CMD_FILL);
}

__de_func static int de_blit(const struct device *dev, int inst, const display_buffer_t *dest,
			     const display_buffer_t *src)
{
	display_layer_t layer = {
		.buffer = src,
		.color = { .a = 0xff, .r = 0, .g = 0, .b = 0, },
		.frame = { 0, 0, dest->desc.width, dest->desc.height },
	};

	return de_insert_overlay_cmd(dev, inst, dest, &layer, 1, DE_CMD_BLIT);
}

__de_func static int de_blend(const struct device *dev, int inst, const display_buffer_t *dest,
			      const display_buffer_t *fg, display_color_t fg_color,
			      const display_buffer_t *bg, display_color_t bg_color)
{
	display_layer_t layers[2] = {
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

	return de_insert_overlay_cmd(dev, inst, dest, layers, 2, fg ? DE_CMD_BLEND : DE_CMD_BLEND_FG);
}

__de_func static int de_compose(const struct device *dev, int inst, const display_buffer_t *dest,
				const display_layer_t *layers, int num_ovls)
{
	uint8_t cmd = dest ? DE_CMD_COMPOSE_WB : DE_CMD_COMPOSE;

	if (num_ovls > MAX_NUM_OVERLAYS || num_ovls <= 0) {
		DEV_WRN("unsupported ovl num %d\n", num_ovls);
		return -EINVAL;
	}

	return de_insert_overlay_cmd(dev, inst, dest, layers, num_ovls, cmd);
}

__de_func static int de_transform(const struct device *dev, int inst, const display_buffer_t *dest,
				  const display_buffer_t *src,
				  const display_engine_transform_param_t *param)
{
	struct de_data *data = dev->data;
	struct de_command_entry *entry;
	de_transform_cfg_t *cfg;
	uint8_t bytes_per_pixel;

	if (data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_WRN("de display-only\n");
		return -EBUSY;
	}

	if ((src->desc.pixel_format & SUPPORTED_ROTATE_PIXEL_FORMATS) == 0) {
		DEV_WRN("unsupported rotation format %u\n", src->desc.pixel_format);
		return -EINVAL;
	}

	if (param->is_circle) {
		if (src->desc.pixel_format != PIXEL_FORMAT_BGR_565 || (src->desc.width & 0x1)) {
			DEV_WRN("src width %d and format 0x%x must meet circle rotation\n",
					src->desc.width, src->desc.pixel_format);
			return -EINVAL;
		}

		if (src->desc.pixel_format != dest->desc.pixel_format ||
			src->desc.width != dest->desc.width ||
			src->desc.width != src->desc.height) {
			DEV_WRN("src and dest must meet circle rotation\n");
			return -EINVAL;
		}

		if (param->circle.line_start + dest->desc.height > src->desc.width) {
			DEV_WRN("circle rotation line range exceed\n");
			return -EINVAL;
		}
	} else {
		bool use_layer = param->blend_en || (src->desc.pixel_format != dest->desc.pixel_format) ||
				(src->desc.pixel_format != PIXEL_FORMAT_BGR_565) || (dest->desc.width & 0x1);

		if (use_layer) {
			display_layer_t layers[2] = {
				{
					.buffer = dest,
					.frame = { 0, 0, dest->desc.width, dest->desc.height },
				},
				{
					.buffer = src,
					.blending = param->blend_en ? DISPLAY_BLENDING_COVERAGE : DISPLAY_BLENDING_NONE,
					.frame = { 0, 0, dest->desc.width, dest->desc.height },
					.matrix = &param->matrix,
					.color =  param->color,
				},
			};

			return de_insert_overlay_cmd(dev, inst, dest, layers, 2, DE_CMD_BLEND);
		}
	}

	entry = de_instance_alloc_entry(inst);
	if (!entry)
		return -EBUSY;

	cfg = (de_transform_cfg_t *)entry->cfg;
	if (param->is_circle) {
		uint16_t outer_diameter = src->desc.width - 1;

		cfg->start_height = param->circle.line_start;
		cfg->r1m2 = param->circle.outer_radius_sq;
		cfg->r0m2 = param->circle.inner_radius_sq;
		cfg->sw_first_dist = outer_diameter * outer_diameter +
				(outer_diameter - 2 * param->circle.line_start) *
					(outer_diameter - 2 * param->circle.line_start);
		cfg->ctl = RT_MODE_CIRCLE | RT_EN | RT_IRQ_EN | RT_FILTER_BILINEAR | RT_COLOR_FILL_BURST_16;
	} else {
		cfg->start_height = 0;
		cfg->src_img_size = RT_SRC_IMG_SIZE(src->desc.width, src->desc.height);
		cfg->ctl = RT_MODE_RECT | RT_EN | RT_IRQ_EN | RT_FILTER_BILINEAR | RT_COLOR_FILL_BURST_16;
	}

	cfg->img_size = RT_IMG_WIDTH(dest->desc.width) |
			RT_END_HEIGHT(cfg->start_height + dest->desc.height);
	cfg->sw_x0 = param->matrix.tx;
	cfg->sw_y0 = param->matrix.ty;
	cfg->sw_x_xy = RT_SW_DELTA_XY(param->matrix.sx, param->matrix.shy);
	cfg->sw_y_xy = RT_SW_DELTA_XY(param->matrix.shx, param->matrix.sy);

	bytes_per_pixel = display_format_get_bits_per_pixel(src->desc.pixel_format) / 8;
	cfg->src_stride = GUESS_PITCH_BYTES(src, bytes_per_pixel);
	cfg->src_addr = buf_is_nor((void *)src->addr) ?
			(uint32_t)cache_to_uncache_by_master((void *)src->addr, SPI0_CACHE_MASTER_DE) :
			(uint32_t)cache_to_uncache((void *)src->addr);
	cfg->src_addr += (cfg->sw_y0 >> 12) * (int32_t)cfg->src_stride + (cfg->sw_x0 >> 12) * bytes_per_pixel;
	cfg->dst_stride = GUESS_PITCH_BYTES(dest, bytes_per_pixel);
	cfg->dst_addr = (uint32_t)cache_to_uncache((void *)dest->addr);

	cfg->fill_color = param->color.full & 0xFFFFFF;
	if (src->desc.pixel_format == PIXEL_FORMAT_BGR_565) {
		cfg->ctl |= RT_FORMAT_RGB565 | RT_COLOR_FILL_EN;
	} else {
		cfg->ctl |= RT_FORMAT_ARGB8888 | RT_COLOR_FILL_EN;
	}

	entry->cmd = param->is_circle ? DE_CMD_ROTATE_CIRCLE : DE_CMD_ROTATE_RECT;
	de_append_cmd(data, entry, de_instance_has_flag(inst, DISPLAY_ENGINE_FLAG_HIGH_PRIO));

	return entry->seq;
}

__de_func static int de_set_clut(const struct device *dev, int inst, uint16_t layer_idx,
				 uint16_t size, const uint32_t *clut)
{
	struct de_data *data = dev->data;
	struct de_command_entry *entry;
	de_clut_cfg_t *cfg;

	if (data->op_mode != DISPLAY_ENGINE_MODE_DEFAULT) {
		DEV_WRN("de display-only\n");
		return -EBUSY;
	}

	if (clut == NULL || layer_idx >= 3/*MAX_NUM_OVERLAYS*/ || size > 256) {
		DEV_WRN("invalid clut %p, idx %d, size %u\n", clut, layer_idx, size);
		return -EINVAL;
	}

	entry = de_instance_alloc_entry(inst);
	if (!entry)
		return -EBUSY;

	cfg = (de_clut_cfg_t *)entry->cfg;
	cfg->layer_idx = layer_idx;
	cfg->size = size;
	cfg->clut = clut;

	entry->cmd = DE_CMD_SET_CLUT;

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
	capabilities->max_width = MAX_WIDTH;
	capabilities->max_height = MAX_HEIGHT;
	capabilities->max_pitch = MAX_PITCH;
	capabilities->num_overlays = 3;/*MAX_NUM_OVERLAYS*/;
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
	DEV_DBG("de sta 0x%08x\n", status);

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

#if CONFIG_DE_USE_DEV_HALFULL
		if (data->start_fn) {
			data->start_fn(data->start_fn_arg);
		}
#endif /* CONFIG_DE_USE_DEV_HALFULL */
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

int de_init(const struct device *dev)
{
	struct de_data *data = dev->data;

	memset(data, 0, sizeof(*data));
	memset(&de_priv_data, 0, sizeof(de_priv_data));

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
	.set_clut = de_set_clut,
	.poll = de_poll,
};

struct de_data de_drv_data __in_section_unique(ram.noinit.drv_de.data);
