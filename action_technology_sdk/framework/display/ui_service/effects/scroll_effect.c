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
#include "effects_inner.h"

/*********************
 *      DEFINES
 *********************/
/**
 * Camera distance from screen
 *
 * set 0.0f for orthogonal projection)
 */
#define CAMERA_DISTANCE  512.0f

/**
 * SCROLL_EFFECT_SCALE.
 */
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
#  define CUBE_ZOOM (0.5)
#  define CUBE_DYNAMIC_ANGLE 20
#endif


#define CUBE_ALPHA 0xFF

/**
 * SCROLL_EFFECT_FAN.
 */
/* angles between the 2 normal of buffers, range (0, 90] */
#define FAN_ANGLE  30

/**
 * SCROLL_EFFECT_SCALE.
 */
/* minimum scale value, range (0.0, 1.0) */
#define SCALE_MIN  0.5f

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
#ifdef CONFIG_VG_LITE
static void _vglite_scroll_effect_handle(const ui_transform_param_t *param, int *trans_end);
static void _vglite_vscroll_effect_handle(const ui_transform_param_t *param, int *trans_end);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
__unused static scroll_effect_ctx_t scroll_ctx = {
	.type = UI_SCROLL_EFFECT_NONE,

#ifdef CONFIG_VG_LITE
	.opt_round_screen_overlapped = false,
	.camera_distance = CAMERA_DISTANCE,
#endif
};

__unused static vscroll_effect_ctx_t vscroll_ctx = {
	.type = UI_VSCROLL_EFFECT_NONE,

#ifdef CONFIG_VG_LITE
	.opt_round_screen_overlapped = false,
	.camera_distance = CAMERA_DISTANCE,
#endif
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int ui_scroll_effect_set_type(uint8_t type)
{
	if (type >= NUM_UI_SCROLL_EFFECTS)
		return -EINVAL;
	if (type == scroll_ctx.type)
		return 0;

#ifdef CONFIG_VG_LITE
	scroll_ctx.type = type;
	switch (type) {
	case UI_SCROLL_EFFECT_CUBE:
		memset(&scroll_ctx.cube, 0, sizeof(scroll_ctx.cube));
		scroll_ctx.cube.out = CUBE_OUT_ROTATE;
		scroll_ctx.cube.angle = CUBE_ANGLE;
		scroll_ctx.cube.alpha = CUBE_ALPHA;
#ifdef CUBE_ZOOM
		scroll_ctx.cube.cube_zoom = CUBE_ZOOM;
#if (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE
		scroll_ctx.cube.dynamic_angle = CUBE_DYNAMIC_ANGLE;
#endif /* (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE */
#endif /* CUBE_ZOOM */
#ifdef CUBE_CAMERA_VALUE
		scroll_ctx.cube.look_down = CUBE_CAMERA_VALUE;
#endif /* CUBE_CAMERA_VALUE */
#ifdef CUBE_CAMERA_COORD
		scroll_ctx.cube.look_down_coord = CUBE_CAMERA_COORD;
#endif /* CUBE_CAMERA_VALUE */
		break;
	case UI_SCROLL_EFFECT_FAN:
		memset(&scroll_ctx.fan, 0, sizeof(scroll_ctx.fan));
		scroll_ctx.fan.angle = FAN_ANGLE;
		scroll_ctx.fan.out_in = true;
		break;
	case UI_SCROLL_EFFECT_SCALE:
		memset(&scroll_ctx.scale, 0, sizeof(scroll_ctx.scale));
		scroll_ctx.scale.min_ratio = SCALE_MIN;
		break;
	case UI_SCROLL_EFFECT_BOOK:
		memset(&scroll_ctx.book, 0, sizeof(scroll_ctx.book));
		scroll_ctx.book.alpha = 0x8f;
		scroll_ctx.book.once_angles = 15;
		scroll_ctx.book.radius = 50;
		break;
	case UI_SCROLL_EFFECT_PAGE:
		break;
	default:
		ui_manager_set_transform_scroll_callback(NULL);
		return 0;
	}
	scroll_ctx.param_inited = false;
	ui_manager_set_transform_scroll_callback(_vglite_scroll_effect_handle);

	return 0;
#else
	return -ENOSYS;
#endif /* CONFIG_VG_LITE */
}

int ui_vscroll_effect_set_type(uint8_t type)
{
	if (type >= NUM_UI_VSCROLL_EFFECTS)
		return -EINVAL;
	if (type == vscroll_ctx.type)
		return 0;
	vscroll_ctx.type = type;
#ifdef CONFIG_VG_LITE
	switch (type) {
	case UI_VSCROLL_EFFECT_ALPHA:
		memset(&vscroll_ctx.cube, 0, sizeof(scroll_ctx.cube));
		vscroll_ctx.alpha.min_zoom = 0.9;
		vscroll_ctx.alpha.global_alpha = CONFIG_UI_VIEW_OVERLAY_OPA;
		vscroll_ctx.alpha.fade = false;
		break;
	case UI_VSCROLL_EFFECT_CUBE:
		memset(&vscroll_ctx.cube, 0, sizeof(vscroll_ctx.cube));
		vscroll_ctx.cube.out = CUBE_OUT_ROTATE;
		vscroll_ctx.cube.angle = CUBE_ANGLE;
		vscroll_ctx.cube.alpha = CUBE_ALPHA;
#ifdef CUBE_ZOOM
		vscroll_ctx.cube.cube_zoom = CUBE_ZOOM;
#if (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE
		vscroll_ctx.cube.dynamic_angle = CUBE_DYNAMIC_ANGLE;
#endif /* (CUBE_ANGLE/2) > CUBE_DYNAMIC_ANGLE */
#endif /* CUBE_ZOOM */
		break;
	case UI_VSCROLL_EFFECT_FAN:
		memset(&vscroll_ctx.fan, 0, sizeof(vscroll_ctx.fan));
		vscroll_ctx.fan.angle = FAN_ANGLE;
		vscroll_ctx.fan.out_in = true;
		break;
	case UI_VSCROLL_EFFECT_PAGE:
		break;
	case UI_VSCROLL_EFFECT_SCALE:
		memset(&vscroll_ctx.scale, 0, sizeof(vscroll_ctx.scale));
		vscroll_ctx.scale.min_ratio = SCALE_MIN;
		break;
	default:
		ui_manager_set_transform_vscroll_callback(NULL);
		return 0;
	}
	vscroll_ctx.param_inited = false;
	ui_manager_set_transform_vscroll_callback(_vglite_vscroll_effect_handle);
	return 0;
#else
	return -ENOSYS;
#endif /* CONFIG_VG_LITE */
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
#ifdef CONFIG_VG_LITE
static void _vglite_scroll_init_effect_param(uint8_t type, const ui_transform_param_t *param)
{
	if (!scroll_ctx.param_inited) {
		scroll_ctx.param_inited = true;

		switch (type) {
		case UI_SCROLL_EFFECT_CUBE:
			if (param->rotation == 0 || param->rotation == 180) {
				scroll_ctx.cube.radius = (param->dst->width / 2.0f) / tan(RAD(scroll_ctx.cube.angle/2.0f));
			} else {
				scroll_ctx.cube.radius = (param->dst->height / 2.0f) / tan(RAD(scroll_ctx.cube.angle/2.0f));
			}
			break;
		case UI_SCROLL_EFFECT_FAN:
			if (param->rotation == 0 || param->rotation == 180) {
				scroll_ctx.fan.radius = (param->dst->height / 2.0f) +
						((param->dst->width / 2.0f) / tan(RAD(scroll_ctx.fan.angle/2.0f)));
			} else {
				scroll_ctx.fan.radius = (param->dst->width / 2.0f) +
						((param->dst->height / 2.0f) / tan(RAD(scroll_ctx.fan.angle/2.0f)));
			}
			if(scroll_ctx.fan.out_in)
				scroll_ctx.fan.radius = -scroll_ctx.fan.radius;
			scroll_ctx.fan.pivot_x = param->dst->width / 2.0f;
			scroll_ctx.fan.pivot_y = param->dst->height / 2.0f;
			if (param->rotation == 0) {
				scroll_ctx.fan.pivot_y += scroll_ctx.fan.radius;
			} else if (param->rotation == 90) {
				scroll_ctx.fan.pivot_x -= scroll_ctx.fan.radius;
			} else if (param->rotation == 180) {
				scroll_ctx.fan.pivot_y -= scroll_ctx.fan.radius;
			} else /* (param->rotation == 270) */ {
				scroll_ctx.fan.pivot_x += scroll_ctx.fan.radius;
			}
			break;
		default:
			break;
		}
	}
}

static void _vglite_scroll_effect_handle(const ui_transform_param_t *param, int *trans_end)
{
	os_strace_u32x2(SYS_TRACE_ID_VIEW_SCROLL_EFFECT, scroll_ctx.type, ui_region_get_width(&param->region_old));

	_vglite_scroll_init_effect_param(scroll_ctx.type, param);

	switch (scroll_ctx.type) {
	case UI_SCROLL_EFFECT_CUBE:
		vglite_scroll_proc_cube_effect(&scroll_ctx, param);
		break;
	case UI_SCROLL_EFFECT_FAN:
		vglite_scroll_proc_fan_effect(&scroll_ctx, param);
		break;
	case UI_SCROLL_EFFECT_PAGE:
		vglite_scroll_proc_page_effect(&scroll_ctx, param);
		break;
	case UI_SCROLL_EFFECT_SCALE:
		vglite_scroll_proc_scale_effect(&scroll_ctx, param);
		break;
	default:
		break;
	}

	os_strace_end_call(SYS_TRACE_ID_VIEW_SCROLL_EFFECT);
}

static void _vglite_vscroll_init_effect_param(uint8_t type, const ui_transform_param_t *param)
{
	if (!vscroll_ctx.param_inited) {
		vscroll_ctx.param_inited = true;

		switch (type) {
		case UI_VSCROLL_EFFECT_CUBE:
			if (param->rotation == 0 || param->rotation == 180) {
				vscroll_ctx.cube.radius = (param->dst->height / 2.0f) / tan(RAD(vscroll_ctx.cube.angle/2.0f));
			} else {
				vscroll_ctx.cube.radius = (param->dst->width / 2.0f) / tan(RAD(vscroll_ctx.cube.angle/2.0f));
			}
			break;
		case UI_VSCROLL_EFFECT_FAN:
			if (param->rotation == 0 || param->rotation == 180) {
				vscroll_ctx.fan.radius = (param->dst->height / 2.0f) +
						((param->dst->width / 2.0f) / tan(RAD(vscroll_ctx.fan.angle/2.0f)));
			} else {
				vscroll_ctx.fan.radius = (param->dst->width / 2.0f) +
						((param->dst->height / 2.0f) / tan(RAD(vscroll_ctx.fan.angle/2.0f)));
			}

			vscroll_ctx.fan.pivot_x = param->dst->width / 2.0f;
			vscroll_ctx.fan.pivot_y = param->dst->height / 2.0f;
			if(vscroll_ctx.fan.out_in)
				vscroll_ctx.fan.radius = -vscroll_ctx.fan.radius;
			if (param->rotation == 0) {
				vscroll_ctx.fan.pivot_x += vscroll_ctx.fan.radius;
			} else if (param->rotation == 90) {
				vscroll_ctx.fan.pivot_y -= vscroll_ctx.fan.radius;
			} else if (param->rotation == 180) {
				vscroll_ctx.fan.pivot_x -= vscroll_ctx.fan.radius;
			} else /* (param->rotation == 270) */ {
				vscroll_ctx.fan.pivot_y += vscroll_ctx.fan.radius;
			}
			break;
		default:
			break;
		}
	}
}

static void _vglite_vscroll_effect_handle(const ui_transform_param_t *param, int *trans_end)
{
	os_strace_u32x2(SYS_TRACE_ID_VIEW_SCROLL_EFFECT, vscroll_ctx.type, ui_region_get_height(&param->region_new));

	_vglite_vscroll_init_effect_param(vscroll_ctx.type, param);

	switch (vscroll_ctx.type) {
	case UI_VSCROLL_EFFECT_ALPHA:
		vglite_vscroll_proc_alpha_effect(&vscroll_ctx, param);
		break;
	case UI_VSCROLL_EFFECT_CUBE:
		vglite_vscroll_proc_cube_effect(&vscroll_ctx, param);
		break;
	case UI_VSCROLL_EFFECT_FAN:
		vglite_vscroll_proc_fan_effect(&vscroll_ctx, param);
		break;
	case UI_VSCROLL_EFFECT_PAGE:
		vglite_vscroll_proc_page_effect(&vscroll_ctx, param);
		break;
	case UI_VSCROLL_EFFECT_SCALE:
		vglite_vscroll_proc_scale_effect(&vscroll_ctx, param);
		break;
	default:
		break;
	}

	os_strace_end_call(SYS_TRACE_ID_VIEW_SCROLL_EFFECT);
}

#endif /* CONFIG_VG_LITE */
