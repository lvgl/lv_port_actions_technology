/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_UI_SERVICE_EFFECTS_INNER_H_
#define FRAMEWORK_DISPLAY_UI_SERVICE_EFFECTS_INNER_H_

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <ui_manager.h>

#ifdef CONFIG_DMA2D_HAL
#  include <dma2d_hal.h>
#endif
#ifdef CONFIG_VG_LITE
#  include <vg_lite/vglite_util.h>
#endif
#ifdef CONFIG_UI_SCROLL_EFFECT
#  include <ui_effects/scroll_effect.h>
#endif
#ifdef CONFIG_UI_SWITCH_EFFECT
#  include <ui_effects/switch_effect.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define FRAME_DIGS 10
#define FRAME_FIXP(x) ((x) << FRAME_DIGS)
#define FRAME_STEP    FRAME_FIXP(1)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	float angle; /* range (0, 90] */
	float radius;
	float pivot_x;
	float pivot_y;
	uint8_t out_in	:	1;
} effects_fan;

typedef struct {
	float cube_zoom;			/*zoom value >1.0 magnify , <1.0 lessen*/
	float angle;
	float radius;
	int16_t look_down;			/*look down value*/
	int16_t look_down_coord;	/*look down center compensation coord*/
	uint16_t dynamic_angle;		/*dynamic execute angle scope:0-(angle/2)*/
	uint8_t alpha;
	uint8_t out :	1;
} effects_cube;

typedef struct {
	float min_ratio;
} effects_scale;

typedef struct {
	float min_zoom;
	uint8_t global_alpha;
	uint8_t fade :	1;
} effects_alpha;

typedef struct {
	uint16_t radius;	/*curve radius*/
	uint16_t once_angles;	/*segmentation angles*/
	uint8_t alpha;	/*above alpha*/
}effects_book;

typedef struct {
	uint8_t type; /* effect type */

#ifdef CONFIG_VG_LITE
	float camera_distance;
	union {
		effects_cube cube;
		effects_fan fan;
		effects_scale scale;
		effects_book book;
		uint32_t user_data[10];		/* use for user extension space */
	};
	uint8_t param_inited : 1;
	/* set true on setting effect type if one buffer's foreground content
	 * overlapped with another buffer's corner background garbage.
	 */
	uint8_t opt_round_screen_overlapped	: 1;
#endif
} scroll_effect_ctx_t;

typedef struct {
	uint8_t type; /* effect type */

#ifdef CONFIG_VG_LITE
	float camera_distance;
	union {
		effects_cube cube;
		effects_fan fan;
		effects_alpha alpha;
		effects_scale scale;
		uint32_t user_data[10];		/* use for user extension space */
	};
	/* internal flags */
	uint8_t blend : 1;
	uint8_t dither : 1;
	uint8_t param_inited : 1;
	uint8_t opt_round_screen_overlapped	: 1;
#endif
} vscroll_effect_ctx_t;

typedef struct {
	float max_zoom;			/*zoom value >1.0 magnify , <1.0 lessen*/
	uint8_t min_alpha;
	uint8_t direction :	1;
} effects_zoom_alpha;

typedef struct {
	float min_zoom;			/*zoom value >1.0 magnify , <1.0 lessen*/
	uint8_t min_alpha;
} effects_translation;

typedef struct {
	int32_t frame; /* current frame count in fixed-point for interpolation convenience */
	int32_t max_frames; /* maximum frames in fixed-point */
	uint8_t type; /* effect type */
	uint8_t new_type; /* new effect type to applied later */
	/* indicate old UI leave anim direction: move right (negative rotate around Y-axis) or opposite direction */
	uint8_t trans_end; /* 1: end */
	uint8_t tp_pressed; /* previous frame has touch pressed or not */
	uint8_t anim_right : 1;
	uint8_t new_anim_right : 1; 
#ifdef CONFIG_DMA2D_HAL
	hal_dma2d_handle_t dma2d;
	bool dma2d_inited;
#endif

#ifdef CONFIG_VG_LITE
	/* set true on setting effect type if one buffer's foreground content
	 * overlapped with another buffer's corner background garbage.
	 */
	uint8_t opt_round_screen_overlapped : 1; 

	float camera_distance;
	union {
		effects_cube cube;
		effects_fan fan;
		effects_zoom_alpha zoom_alpha;
		effects_translation	translation;
		effects_book book;
		uint32_t user_data[10];		/* use for user extension space */
	};
#endif
} switch_effect_ctx_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int32_t switch_effects_path_cos(int32_t start, int32_t end);

#ifdef CONFIG_VG_LITE
void vglite_scroll_proc_cube_effect(scroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_scroll_proc_fan_effect(scroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_scroll_proc_page_effect(scroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_scroll_proc_scale_effect(scroll_effect_ctx_t *ctx, const ui_transform_param_t *param);

void vglite_vscroll_proc_alpha_effect(vscroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_vscroll_proc_cube_effect(vscroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_vscroll_proc_fan_effect(vscroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_vscroll_proc_page_effect(vscroll_effect_ctx_t *ctx, const ui_transform_param_t *param);
void vglite_vscroll_proc_scale_effect(vscroll_effect_ctx_t *ctx, const ui_transform_param_t *param);

int vglite_switch_proc_fan_effect(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
int vglite_switch_proc_scale_effect(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
int vglite_switch_proc_page_effect(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
int vglite_switch_proc_zoom_alpha(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
int vglite_switch_proc_translation(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
int vglite_switch_proc_cube_effect(switch_effect_ctx_t *ctx, const ui_transform_param_t *param);
#endif /* CONFIG_VG_LITE */

#ifdef __cplusplus
}
#endif
#endif /*FRAMEWORK_DISPLAY_UI_SERVICE_EFFECTS_INNER_H_*/
