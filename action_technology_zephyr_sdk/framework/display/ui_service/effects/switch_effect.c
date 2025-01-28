/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <math.h>
#include <os_common_api.h>
#include <display/sw_math.h>
#include "effects_inner.h"

/*********************
 *      DEFINES
 *********************/
#ifndef CONFIG_UI_EFFECT_TRANSFORM_BUFFER_COUNT
#  define CONFIG_UI_EFFECT_TRANSFORM_BUFFER_COUNT 2
#endif

#if CONFIG_UI_EFFECT_TRANSFORM_BUFFER_COUNT > 1
#  define DEF_MAX_FRAMES 16 /* near 267 ms for 60 Hz display */
#else
#  define DEF_MAX_FRAMES 8 /* the frame-rate will cut by almost half for only 1 trans buffer */
#endif

/* SWITCH_EFFECT_PAGE: (set 0.0f for orthogonal projection) */
#define CAMERA_DISTANCE  512.0f

/* SWITCH_EFFECT_FAN, range (0, 90] */
#define FAN_ANGLE  60

/* SWITCH_EFFECT_CUBE */
/* can see the cube outside or inside during rotation */
#define CUBE_OUT_ROTATE  1
/* angles between the 2 normal of buffer planes, range (0, 90] */
#if CUBE_OUT_ROTATE
#  define CUBE_ANGLE  60
#  define CUBE_CAMERA_VALUE 100
#  define CUBE_CAMERA_COORD 100
#  define CUBE_ZOOM (0.8)
#  define CUBE_DYNAMIC_ANGLE 16
#else
#  define CUBE_ANGLE  90
#endif
#define CUBE_ALPHA 0xFF

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _switch_effect_msg_cb(struct app_msg *msg, int result, void *unused);
static void _switch_effect_transform_handle(const ui_transform_param_t *param, int *trans_end);

#ifdef CONFIG_VG_LITE
static int _vglite_switch_effect_handle(const ui_transform_param_t *param);
#endif /* CONFIG_VG_LITE */

#ifdef CONFIG_DMA2D_HAL
static int _dma2d_init(graphic_buffer_t *dst);
static int _dma2d_switch_effect_handle(const ui_transform_param_t *param);
static int _dma2d_switch_proc_alpha_effect(const ui_transform_param_t *param);
static int _dma2d_switch_proc_scale_effect(const ui_transform_param_t *param);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static switch_effect_ctx_t switch_ctx = {
	.type = UI_SWITCH_EFFECT_NONE,
	.frame = FRAME_FIXP(1),
	.max_frames = FRAME_FIXP(DEF_MAX_FRAMES),
	.anim_right = 1,
	.trans_end = 1,

#ifdef CONFIG_VG_LITE
	.opt_round_screen_overlapped = false,
	.camera_distance = CAMERA_DISTANCE,
#endif
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int ui_switch_effect_set_type(uint8_t type)
{
	if (type >= NUM_UI_SWITCH_EFFECTS)
		return -EINVAL;

	return ui_message_send_async2(VIEW_INVALID_ID, MSG_VIEW_USER_OFFSET, type, _switch_effect_msg_cb);
}

void ui_switch_effect_set_total_frames(uint16_t frame)
{
	if (frame > 0)
		switch_ctx.max_frames = FRAME_FIXP(frame);
}

void ui_switch_effect_set_anim_dir(bool out_right)
{
	ui_message_send_async2(VIEW_INVALID_ID, MSG_VIEW_USER_OFFSET + 1, out_right, _switch_effect_msg_cb);
}

int32_t switch_effects_path_cos(int32_t start, int32_t end)
{
	/* FIXME: both start & end less than （1 << 16）? */
	const int cos_shift = 16;
	const int32_t cos_max = sw_cos30(0) >> cos_shift;
	uint32_t angle = ui_map(switch_ctx.frame, 0, switch_ctx.max_frames, 900, 0);

	return ui_map(sw_cos30(angle) >> cos_shift, 0, cos_max, start, end);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _switch_effect_change_type(void)
{
	if (switch_ctx.type == switch_ctx.new_type)
		return;

	switch_ctx.type = switch_ctx.new_type;

	switch (switch_ctx.type) {
	case UI_SWITCH_EFFECT_FAN:
		switch_ctx.fan.angle = FAN_ANGLE;
		break;
	case UI_SWITCH_EFFECT_CUBE:
		switch_ctx.cube.out = CUBE_OUT_ROTATE;
		switch_ctx.cube.angle = CUBE_ANGLE;
		switch_ctx.cube.alpha = CUBE_ALPHA;
#ifdef CUBE_ZOOM
		switch_ctx.cube.cube_zoom = CUBE_ZOOM;
		switch_ctx.cube.look_down = CUBE_CAMERA_VALUE;
		switch_ctx.cube.look_down_coord = CUBE_CAMERA_COORD;
#if (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE
		switch_ctx.cube.dynamic_angle = CUBE_DYNAMIC_ANGLE;
#else
		switch_ctx.cube.dynamic_angle = 0;
#endif	/* (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE */
#endif	/* CUBE_ZOOM */
		break;
	case UI_SWITCH_EFFECT_ZOOM_ALPHA:
		switch_ctx.zoom_alpha.max_zoom = 2.0;
		switch_ctx.zoom_alpha.min_alpha = 0x7F;
		switch_ctx.zoom_alpha.direction = false;
		break;
	case UI_SWITCH_EFFECT_TRANSLATION:
		switch_ctx.translation.min_alpha = 0xFF;
		switch_ctx.translation.min_zoom = 0.8;
		break;
	case UI_SWITCH_EFFECT_BOOK:
		switch_ctx.book.alpha = 0x8f;
		switch_ctx.book.once_angles = 15;
		switch_ctx.book.radius = 50;
		break;
	case UI_SWITCH_EFFECT_ALPHA:
	case UI_SWITCH_EFFECT_PAGE:
	case UI_SWITCH_EFFECT_SCALE:
		break;
	default:
		view_manager_set_callback(UI_CB_TRANSFORM_SWITCH, NULL);
		return;
	}
	view_manager_set_callback(UI_CB_TRANSFORM_SWITCH, _switch_effect_transform_handle);
}

static void _switch_effect_msg_cb(struct app_msg *msg, int result, void *unused)
{
	if (msg->cmd == MSG_VIEW_USER_OFFSET) {
		switch_ctx.new_type = msg->value;
		if (switch_ctx.trans_end)
			_switch_effect_change_type();
	} else {
		switch_ctx.new_anim_right = msg->value ? 1 : 0;
		if (switch_ctx.trans_end)
			switch_ctx.anim_right = switch_ctx.new_anim_right;
	}
}

#ifdef CONFIG_UI_SWITCH_EFFECT_TRACKING_TOUCH
static void _compute_frame_by_touch(const ui_transform_param_t *param)
{
	int16_t dist = 0;
	switch (param->rotation) {
	case 0:
		dist = param->tp_vect.x;
		break;
	case 180:
		dist = -param->tp_vect.x;
		break;
	case 90:
		dist = param->tp_vect.y;
		break;
	case 270:
		dist = -param->tp_vect.y;
		break;
	default:
		break;
	}

	switch_ctx.frame = UI_MAX(dist, 0) * switch_ctx.max_frames / param->dst->width;
}
#endif /* CONFIG_UI_SWITCH_EFFECT_TRACKING_TOUCH */

static void _switch_effect_transform_handle(const ui_transform_param_t *param, int *trans_end)
{
	int ret = -1;

	if (param->first_frame) {
		switch_ctx.frame = FRAME_FIXP(1);
		switch_ctx.trans_end = 0;
		switch_ctx.tp_pressed = 0;
	}

	if (switch_ctx.trans_end || *trans_end != 0)
		goto end_exit;

	os_strace_u32x2(SYS_TRACE_ID_VIEW_SWITCH_EFFECT, switch_ctx.type, switch_ctx.frame);

#ifdef CONFIG_UI_SWITCH_EFFECT_TRACKING_TOUCH
	if (param->tp_pressing) {
		_compute_frame_by_touch(param);
		switch_ctx.tp_pressed = 1;
	} else if (switch_ctx.tp_pressed) {
		switch_ctx.tp_pressed = 0;
		switch_ctx.frame = (switch_ctx.frame & ~(FRAME_STEP - 1)) + FRAME_STEP;
		if (switch_ctx.frame > switch_ctx.max_frames)
			switch_ctx.frame = switch_ctx.max_frames;
	}
#endif

#ifdef CONFIG_VG_LITE
	ret = _vglite_switch_effect_handle(param);
#endif

#ifdef CONFIG_DMA2D_HAL
	if (ret < 0) /* fallback to DMA2D */
		ret = _dma2d_switch_effect_handle(param);
#endif

	os_strace_end_call(SYS_TRACE_ID_VIEW_SWITCH_EFFECT);

	if (!ret) {
		if (switch_ctx.tp_pressed == 0) {
			switch_ctx.frame += FRAME_STEP;
			if (switch_ctx.frame > switch_ctx.max_frames)
				switch_ctx.trans_end = 1;
		}

		return;
	}

end_exit:
	*trans_end = 1;
	switch_ctx.trans_end = 1;
	switch_ctx.anim_right = switch_ctx.new_anim_right;
	_switch_effect_change_type();
}

#ifdef CONFIG_VG_LITE
static int _vglite_switch_effect_handle(const ui_transform_param_t *param)
{
	switch (switch_ctx.type) {
	case UI_SWITCH_EFFECT_FAN:
		return vglite_switch_proc_fan_effect(&switch_ctx, param);
	case UI_SWITCH_EFFECT_PAGE:
		return vglite_switch_proc_page_effect(&switch_ctx, param);
	case UI_SWITCH_EFFECT_SCALE:
		return vglite_switch_proc_scale_effect(&switch_ctx, param);
	case UI_SWITCH_EFFECT_ZOOM_ALPHA:
		return vglite_switch_proc_zoom_alpha(&switch_ctx, param);
	case UI_SWITCH_EFFECT_TRANSLATION:
		return vglite_switch_proc_translation(&switch_ctx, param);
	case UI_SWITCH_EFFECT_CUBE:
		return vglite_switch_proc_cube_effect(&switch_ctx, param);
	default:
		return -ENOSYS;
	}
}
#endif /* CONFIG_VG_LITE */

#ifdef CONFIG_DMA2D_HAL

static int _dma2d_switch_effect_handle(const ui_transform_param_t *param)
{
	if (_dma2d_init(param->dst))
		return -ENODEV;

	switch (switch_ctx.type) {
	case UI_SWITCH_EFFECT_ALPHA:
		return _dma2d_switch_proc_alpha_effect(param);
	case UI_SWITCH_EFFECT_SCALE:
		return _dma2d_switch_proc_scale_effect(param);
	default:
		return -ENOSYS;
	}
}

static int _dma2d_init(graphic_buffer_t *dst)
{
	hal_dma2d_handle_t *hdma2d = &switch_ctx.dma2d;
	uint16_t dst_pitch = dst->stride * dst->bits_per_pixel / 8;

	if (switch_ctx.dma2d_inited)
		return 0;

	if (hal_dma2d_init(hdma2d, HAL_DMA2D_FULL_MODES)) {
		SYS_LOG_ERR("transxxx failed to init dma2d %d\n");
		return -ENODEV;
	}

	switch_ctx.dma2d_inited = true;

	/* background layer */
	hdma2d->layer_cfg[0].input_pitch = dst_pitch;
	hdma2d->layer_cfg[0].input_width = dst->width;
	hdma2d->layer_cfg[0].input_height = dst->height;
	hdma2d->layer_cfg[0].color_format = dst->pixel_format;
	hdma2d->layer_cfg[0].alpha_mode = HAL_DMA2D_NO_MODIF_ALPHA;
	hdma2d->layer_cfg[0].input_alpha = 0xFF000000;
	hal_dma2d_config_layer(hdma2d, 0);

	/*  foreground layer */
	hdma2d->layer_cfg[1].input_pitch = dst_pitch;
	hdma2d->layer_cfg[1].input_width = dst->width;
	hdma2d->layer_cfg[1].input_height = dst->height;
	hdma2d->layer_cfg[1].color_format = dst->pixel_format;
	hdma2d->layer_cfg[1].alpha_mode = HAL_DMA2D_COMBINE_ALPHA;
	hdma2d->layer_cfg[1].input_alpha = 0xFF000000;
	hal_dma2d_config_layer(hdma2d, 1);

	hdma2d->output_cfg.mode = HAL_DMA2D_M2M_BLEND;
	hdma2d->output_cfg.output_pitch = dst_pitch;
	hdma2d->output_cfg.color_format = dst->pixel_format;
	hal_dma2d_config_output(hdma2d);

	return 0;
}

static int _dma2d_switch_proc_alpha_effect(const ui_transform_param_t *param)
{
	hal_dma2d_handle_t *hdma2d = &switch_ctx.dma2d;
	uint32_t old_alpha = 255 * (switch_ctx.max_frames - switch_ctx.frame) / switch_ctx.max_frames;

	hdma2d->output_cfg.mode = HAL_DMA2D_M2M_BLEND;
	hal_dma2d_config_output(hdma2d);

	hdma2d->layer_cfg[1].input_alpha = (old_alpha << 24);
	hal_dma2d_config_layer(hdma2d, 1);

	hal_dma2d_blending_start(hdma2d, (uint32_t)param->src_old->data, (uint32_t)param->src_new->data,
			(uint32_t)param->dst->data, param->dst->width, param->dst->height);
	hal_dma2d_poll_transfer(hdma2d, -1);

	return 0;
}

static int _dma2d_switch_proc_scale_effect(const ui_transform_param_t *param)
{
	hal_dma2d_handle_t *hdma2d = &switch_ctx.dma2d;
	int32_t mid_frame = switch_ctx.max_frames / 2;
	int32_t scale_frame;
	int16_t scale_xres;
	int16_t scale_yres;
	uint32_t src_addr;
	bool zoom_out;

	/* fill black color */
	hdma2d->output_cfg.mode = HAL_DMA2D_R2M;
	hal_dma2d_config_output(hdma2d);
	hal_dma2d_start(hdma2d, 0, (uint32_t)param->dst->data, param->dst->width, param->dst->height);

	/* copy */
	hdma2d->output_cfg.mode = HAL_DMA2D_M2M;
	hal_dma2d_config_output(hdma2d);

	zoom_out = (switch_ctx.frame < mid_frame);
	if (zoom_out) {
		src_addr = (uint32_t)param->src_old->data;
		scale_frame = (mid_frame - switch_ctx.frame);
	} else {
		src_addr = (uint32_t)param->src_new->data;
		scale_frame = (switch_ctx.frame - mid_frame);
	}

	if (scale_frame > mid_frame)
		scale_frame = mid_frame;

	if (scale_frame > 0) {
		scale_xres = param->dst->width * scale_frame / mid_frame;
		scale_yres = param->dst->height * scale_frame / mid_frame;

		uint32_t dst_addr = (uint32_t)param->dst->data +
				(param->dst->stride * ((param->dst->height - scale_yres) / 2) +
					((param->dst->width - scale_xres) / 2)) * param->dst->bits_per_pixel / 8;

		hal_dma2d_start(hdma2d, (uint32_t)src_addr, dst_addr, scale_xres, scale_yres);
	}

	hal_dma2d_poll_transfer(hdma2d, -1);
	return 0;
}

#endif /* CONFIG_DMA2D_HAL */
