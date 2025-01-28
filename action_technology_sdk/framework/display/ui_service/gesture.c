/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ui service interface
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <view_manager.h>
#include <input_manager.h>
#include <ui_service.h>
#include <ui_math.h>

LOG_MODULE_REGISTER(view_scrl, LOG_LEVEL_INF);

#ifdef CONFIG_VIEW_SCROLL_TRACKING_FINGER
#define GESTURE_DROP_START_THRESHOLD  100
#else
#define GESTURE_DROP_START_THRESHOLD  400
#endif

#define GESTURE_DROP_THRESHOLD        100
#define GESTURE_MOVE_THRESHOLD        10
#define GESTURE_ANIM_SPEED            32 /* pixels per 16 ms */

/* gesture long view snap to edge anim enable flag */
#define USE_GESTURE_LONGVIEW_SNAP_ANIM 0

/* gesture animation config */
#define USE_GESTURE_ANIM_PATH_COS     1
#define USE_GESTURE_ANIM_PATH_BEZIER  0

#if USE_GESTURE_ANIM_PATH_COS
static int32_t _animation_path_cos(int32_t elaps);
#elif USE_GESTURE_ANIM_PATH_BEZIER
static int32_t _animation_update_bezier(int32_t elaps);
#endif

static const uint8_t opposite_gestures[] = {
	0, GESTURE_DROP_UP, GESTURE_DROP_DOWN, GESTURE_DROP_RIGHT, GESTURE_DROP_LEFT,
};

static void _reposition_region_in_display(ui_region_t *region)
{
	ui_region_t cont = {
		.x1 = 0,
		.y1 = 0,
		.x2 = view_manager_get_disp_xres() - 1,
		.y2 = view_manager_get_disp_yres() - 1,
	};

	ui_region_fit_in(region, &cont);
}

static int gesture_scroll_begin(input_dev_t * pointer_dev)
{
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	point_t *act_point = &runtime->types.pointer.act_point;
	int16_t x_res = view_manager_get_disp_xres();
	int16_t y_res = view_manager_get_disp_yres();
	uint16_t focus_id;
	uint16_t focus_attr;
	bool towards_screen = false;

	runtime->view_id = view_manager_get_draggable_view(runtime->scroll_dir, &towards_screen);

	focus_id = view_manager_get_focused_view();
	if (focus_id == VIEW_INVALID_ID) {
		return -1;
	}

	focus_attr = view_get_drag_attribute(focus_id);
	if (focus_id != runtime->view_id && (focus_attr & UI_DRAG_DIR_MASK) > 0) {
		return -1;
	}

	runtime->types.pointer.scroll_to_screen = towards_screen;
	runtime->types.pointer.scroll_start = *act_point;
	runtime->types.pointer.last_scroll_off = 0;
	runtime->types.pointer.scroll_outscreen_en = (focus_attr & UI_DRAG_SNAPEDGE) ? 1 : 0;

	if ((runtime->view_id == VIEW_INVALID_ID && (focus_attr & UI_DRAG_SNAPEDGE)) ||
		(runtime->view_id != VIEW_INVALID_ID && view_has_move_attribute(runtime->view_id))) {	/* move */
		runtime->last_view_id = focus_id;
		runtime->pre_view_id = view_manager_get_draggable_view(opposite_gestures[runtime->scroll_dir], NULL);
		if (runtime->view_id == VIEW_INVALID_ID && runtime->pre_view_id == VIEW_INVALID_ID) {
			return -2;
		}

	} else if (runtime->view_id != VIEW_INVALID_ID) { /* drop */
		int16_t offset = 0;

		switch (runtime->scroll_dir) {
		case GESTURE_DROP_DOWN:
			offset = act_point->y;
#ifdef CONFIG_VIEW_SCROLL_TRACKING_FINGER
			runtime->types.pointer.scroll_start.y = 0;
#endif
			break;
		case GESTURE_DROP_UP:
			offset = y_res - 1 - act_point->y;
#ifdef CONFIG_VIEW_SCROLL_TRACKING_FINGER
			runtime->types.pointer.scroll_start.y = y_res - 1;
#endif
			break;
		case GESTURE_DROP_RIGHT:
			offset = act_point->x;
#ifdef CONFIG_VIEW_SCROLL_TRACKING_FINGER
			runtime->types.pointer.scroll_start.x = 0;
#endif
			break;
		case GESTURE_DROP_LEFT:
			offset = x_res - 1 - act_point->x;
#ifdef CONFIG_VIEW_SCROLL_TRACKING_FINGER
			runtime->types.pointer.scroll_start.x = x_res - 1;
#endif
			break;
		default:
			break;
		}

		if (offset >= GESTURE_DROP_START_THRESHOLD)
			return -1;

		runtime->last_view_id = VIEW_INVALID_ID;
		runtime->pre_view_id = VIEW_INVALID_ID;
	} else {
		return -1;
	}

	runtime->current_view_id = runtime->view_id;
	runtime->related_view_id = runtime->last_view_id;

	SYS_LOG_INF("gesture begin %d, start (%d %d), view %u, last_view %u, pre_view %u\n",
		runtime->scroll_dir, act_point->x, act_point->y, runtime->view_id,
		runtime->last_view_id, runtime->pre_view_id);

	return 0;
}

static void gesture_scroll(input_dev_t * pointer_dev)
{
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	point_t *act_point = &runtime->types.pointer.act_point;
	int16_t x_res = view_manager_get_disp_xres();
	int16_t y_res = view_manager_get_disp_yres();
	bool is_vscroll = (runtime->scroll_dir <= GESTURE_DROP_UP);
	ui_point_t drag_pos = { 0, 0 };
	ui_region_t rel_region = { 0, 0, x_res - 1, y_res - 1 };

	if (runtime->current_view_id == VIEW_INVALID_ID || view_has_move_attribute(runtime->current_view_id)) {
		if (runtime->view_id == runtime->related_view_id) { /* long view  */
			view_get_region(runtime->view_id, &rel_region);
			if (is_vscroll) {
				ui_region_set_y(&rel_region, rel_region.y1 + runtime->types.pointer.vect.y);
			} else {
				ui_region_set_x(&rel_region, rel_region.x1 + runtime->types.pointer.vect.x);
			}

#if USE_GESTURE_LONGVIEW_SNAP_ANIM == 0
			_reposition_region_in_display(&rel_region);
#endif

			view_set_drag_pos(runtime->view_id, rel_region.x1, rel_region.y1);
		} else {
			int16_t drop_sign;
			int16_t scroll_off;

			if (is_vscroll) {
				scroll_off = act_point->y - runtime->types.pointer.scroll_start.y;
				drag_pos.y = (runtime->scroll_dir == GESTURE_DROP_DOWN) ? -y_res : y_res;
				drop_sign = (runtime->scroll_dir == GESTURE_DROP_DOWN) ? 1 : -1;
			} else {
				scroll_off = act_point->x - runtime->types.pointer.scroll_start.x;
				drag_pos.x = (runtime->scroll_dir == GESTURE_DROP_RIGHT) ? -x_res : x_res;
				drop_sign = (runtime->scroll_dir == GESTURE_DROP_RIGHT) ? 1 : -1;
			}

			/* check move direction compared to the origin */
			if (scroll_off * drop_sign > 0) {
				if (runtime->view_id != runtime->current_view_id) {
					view_set_drag_pos(runtime->view_id, -drag_pos.x, -drag_pos.y);
					runtime->view_id = runtime->current_view_id;
				}
			} else if (scroll_off * drop_sign < 0) {
				drag_pos.x = -drag_pos.x;
				drag_pos.y = -drag_pos.y;

				if (runtime->view_id == runtime->current_view_id) {
					view_set_drag_pos(runtime->view_id, drag_pos.x, drag_pos.y);
					runtime->view_id = runtime->pre_view_id;
				}
			}

			runtime->types.pointer.last_scroll_off = scroll_off;

			if (runtime->view_id != VIEW_INVALID_ID) {
				if (is_vscroll) {
					drag_pos.y += runtime->types.pointer.scroll_sum.y;
				} else {
					drag_pos.x += runtime->types.pointer.scroll_sum.x;
				}

				view_set_drag_pos(runtime->view_id, drag_pos.x, drag_pos.y);
			}

			if (runtime->related_view_id != VIEW_INVALID_ID) {
				if (is_vscroll) {
					ui_region_set_y(&rel_region, runtime->types.pointer.scroll_sum.y);
				} else {
					ui_region_set_x(&rel_region, runtime->types.pointer.scroll_sum.x);
				}

				if (runtime->view_id == VIEW_INVALID_ID && !runtime->types.pointer.scroll_outscreen_en) {
					_reposition_region_in_display(&rel_region);
				}

				view_set_drag_pos(runtime->related_view_id, rel_region.x1, rel_region.y1);
			}

			SYS_LOG_DBG("scroll_off %d, view %u pos (%d,%d), related_view %u (%d,%d)\n",
				scroll_off, runtime->view_id, drag_pos.x, drag_pos.y,
				runtime->related_view_id, rel_region.x1, rel_region.y1);
		}
	} else {
		int16_t scroll_off;

		if (is_vscroll) {
			scroll_off = act_point->y - runtime->types.pointer.scroll_start.y;
		} else {
			scroll_off = act_point->x - runtime->types.pointer.scroll_start.x;
		}

		switch (runtime->scroll_dir) {
		case GESTURE_DROP_DOWN:
			scroll_off = UI_MAX(scroll_off, 0);
			drag_pos.y = runtime->types.pointer.scroll_to_screen ?
					(scroll_off - y_res) : scroll_off;
			break;
		case GESTURE_DROP_UP:
			scroll_off = UI_MIN(scroll_off, 0);
			drag_pos.y = runtime->types.pointer.scroll_to_screen ?
					(scroll_off + y_res) : scroll_off;
			break;
		case GESTURE_DROP_RIGHT:
			scroll_off = UI_MAX(scroll_off, 0);
			drag_pos.x = runtime->types.pointer.scroll_to_screen ?
					(scroll_off - x_res) : scroll_off;
			break;
		case GESTURE_DROP_LEFT:
		default:
			scroll_off = UI_MIN(scroll_off, 0);
			drag_pos.x = runtime->types.pointer.scroll_to_screen ?
					(scroll_off + x_res) : scroll_off;
			break;
		}

		runtime->types.pointer.last_scroll_off = scroll_off;
		view_set_drag_pos(runtime->view_id, drag_pos.x, drag_pos.y);
	}
}

static void gesture_fixed(input_dev_runtime_t *runtime)
{
	uint16_t temp_view_id;

	/* must both are vertical or horizontal direction */
	if (runtime->last_scroll_dir == 0 || runtime->last_scroll_dir == runtime->scroll_dir ||
		(runtime->last_scroll_dir < GESTURE_DROP_LEFT) != (runtime->scroll_dir < GESTURE_DROP_LEFT)) {
		return;
	}

	runtime->scroll_dir = runtime->last_scroll_dir;

	temp_view_id = runtime->related_view_id;
	runtime->related_view_id = runtime->view_id;
	runtime->view_id = temp_view_id;
}

static void gesture_scroll_end(input_dev_t * pointer_dev)
{
	static const uint8_t slidein_anim[] = {
		 0, UI_ANIM_SLIDE_IN_DOWN, UI_ANIM_SLIDE_IN_UP,
		UI_ANIM_SLIDE_IN_LEFT, UI_ANIM_SLIDE_IN_RIGHT,
	};
	static const uint8_t slideout_anim[] = {
		0, UI_ANIM_SLIDE_OUT_UP, UI_ANIM_SLIDE_OUT_DOWN,
		UI_ANIM_SLIDE_OUT_RIGHT, UI_ANIM_SLIDE_OUT_LEFT,
	};
	input_dev_runtime_t *runtime = &pointer_dev->proc;
	int16_t animation_type = -1;
	int16_t scroll_off = UI_ABS(runtime->types.pointer.last_scroll_off);
	bool is_longview = false;

	if (runtime->current_view_id == VIEW_INVALID_ID || view_has_move_attribute(runtime->current_view_id)) {
		if (runtime->view_id == runtime->related_view_id) { /* long view  */
			is_longview = true;
		} else if (runtime->view_id == runtime->current_view_id &&
				runtime->current_view_id != VIEW_INVALID_ID) {
			if (scroll_off >= GESTURE_MOVE_THRESHOLD) {
				gesture_fixed(runtime);
				animation_type = slidein_anim[runtime->scroll_dir];
			} else {
				animation_type = slideout_anim[runtime->scroll_dir];
			}
		} else {
			if (runtime->view_id != VIEW_INVALID_ID) {
				if (scroll_off >= GESTURE_MOVE_THRESHOLD) {
					gesture_fixed(runtime);
					animation_type = slidein_anim[opposite_gestures[runtime->scroll_dir]];
				} else {
					animation_type = slideout_anim[opposite_gestures[runtime->scroll_dir]];
				}
			} else {
				runtime->view_id = runtime->related_view_id;
				runtime->related_view_id = VIEW_INVALID_ID;
				animation_type = slidein_anim[opposite_gestures[runtime->scroll_dir]];
			}
		}
	} else {
		if (runtime->types.pointer.scroll_to_screen) {
			if (scroll_off >= GESTURE_DROP_THRESHOLD) {
				animation_type = slidein_anim[runtime->scroll_dir];
			} else {
				animation_type = slideout_anim[runtime->scroll_dir];
			}
		} else {
			if (scroll_off >= GESTURE_DROP_THRESHOLD) {
				animation_type = slideout_anim[opposite_gestures[runtime->scroll_dir]];
			} else {
				animation_type = slidein_anim[opposite_gestures[runtime->scroll_dir]];
			}
		}
	}

	SYS_LOG_INF("gesture end %d, view %u, related_view %u, scroll_off %d, point (%d %d), anim %d\n",
		runtime->scroll_dir, runtime->view_id, runtime->related_view_id,
		runtime->types.pointer.last_scroll_off, runtime->types.pointer.last_point.x,
		runtime->types.pointer.last_point.y, animation_type);

	if (runtime->view_id != VIEW_INVALID_ID) {
		ui_view_anim_cfg_t cfg;
		memset(&cfg, 0, sizeof(cfg));

		if (is_longview) {
			view_manager_get_drag_animation_config(runtime->view_id, &cfg, runtime);
		} else {
			view_manager_get_slide_animation_config(runtime->view_id, &cfg, animation_type);
		}

		if (cfg.duration == 0) {
			uint16_t step_x = UI_ABS(cfg.stop.x - cfg.start.x);
			uint16_t step_y = UI_ABS(cfg.stop.y - cfg.start.y);

			cfg.duration = (UI_MAX(step_x, step_y) * 16 + GESTURE_ANIM_SPEED - 1) / GESTURE_ANIM_SPEED;
		}

		if (cfg.path_cb == NULL) { /* default is linear */
#if USE_GESTURE_ANIM_PATH_COS
			cfg.path_cb = _animation_path_cos;
#elif USE_GESTURE_ANIM_PATH_BEZIER
			cfg.path_cb = _animation_path_bezier;
#endif
		}

		if (is_longview) {
			view_manager_drag_animation_start(runtime->view_id, &cfg);
		} else {
			view_manager_slide_animation_start(runtime->view_id, runtime->related_view_id, animation_type, &cfg);
		}
	}
}

#if USE_GESTURE_ANIM_PATH_COS
static int32_t _animation_path_cos(int32_t elaps)
{
	uint32_t angle = ui_map(elaps, 0, UI_VIEW_ANIM_RANGE, 900, 1800);
	int32_t cos_value = -sw_cos30(angle) / UI_VIEW_ANIM_RANGE;

	return ui_map(cos_value, 0, -sw_cos30(1800) / UI_VIEW_ANIM_RANGE, 0, UI_VIEW_ANIM_RANGE);
}
#elif USE_GESTURE_ANIM_PATH_BEZIER
static int32_t _animation_path_bezier(int32_t elaps)
{
	return ui_bezier3(elaps, 0, 900, 950, UI_VIEW_ANIM_RANGE);
}
#endif /* USE_GESTURE_ANIM_PATH_COS */

static const ui_gesture_callback_t gesture_callback = {
	.scroll_begin = gesture_scroll_begin,
	.scroll = gesture_scroll,
	.scroll_end = gesture_scroll_end,
};

int ui_service_register_gesture_default_callback(void)
{
	return ui_service_register_gesture_callback(&gesture_callback);
}
