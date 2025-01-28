/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <app_ui.h>
#include <assert.h>
#include <msgbox_cache.h>
#include <os_common_api.h>

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	bool focused;
	uint16_t cur_ui;
	lv_style_t style;
} app_view_data_t;

/**********************
 *   GLOBAL PROTOTYPES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static app_view_data_t * _get_main_view_data(void)
{
	view_data_t *view_data = view_get_data(APP_MAIN_VIEW);
	if (view_data == NULL)
		return NULL;

	return view_data->user_data;
}

static inline lv_obj_t *_get_scr_act(lv_disp_t *disp)
{
	return disp->scr_to_load ? disp->scr_to_load : lv_disp_get_scr_act(disp);
}

static int  _main_view_layout(view_data_t * view_data)
{
	app_view_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->focused = true;

	lv_style_init(&data->style);
	lv_style_set_bg_color(&data->style, lv_color_black());
	lv_style_set_bg_opa(&data->style, LV_OPA_COVER);

	view_data->user_data = data;
	return 0;
}

static app_ui_scene_proc_t _get_ui_proc(const app_ui_info_t * info)
{
	uint16_t count = info->n_scenes[0] + info->n_scenes[1] +
					 info->n_scenes[2] + info->n_scenes[3];

	return (count > 0) ? app_ui_scene_tiles_proc : info->scenes[0].proc;
}

static app_ui_scene_proc_t _get_ui_proc_by_id(uint16_t id)
{
	const app_ui_info_t *info = app_ui_get_info(id);
	if (info == NULL)
		return NULL;

	return _get_ui_proc(info);
}

static void _scr_event_cb(lv_event_t * e)
{
	lv_obj_t *scr = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	uint16_t id = (uintptr_t)lv_event_get_user_data(e);

	if (code == LV_EVENT_SCREEN_LOAD_START) {
		/* reset all indev */
		lv_indev_t *indev = lv_indev_get_next(NULL);
		while (indev) {
			lv_indev_reset(indev, NULL);
			lv_indev_wait_release(indev);
			indev = lv_indev_get_next(indev);
		}

		SYS_LOG_INF("UI %u load started", id);
	}

	if (code == LV_EVENT_SCREEN_LOADED) {
		SYS_LOG_INF("UI %u load finished", id);

		/* update refresh rate for single scene */
		const app_ui_info_t *info = app_ui_get_info(id);
		if (info) {
			uint16_t scene_id = 0;
			app_ui_stack_current(&scene_id);

			const app_ui_scene_info_t *scene_info = app_ui_get_scene_info(info, scene_id);
			app_ui_set_fps(app_ui_get_scene_fps(scene_info));

			app_view_data_t *data = _get_main_view_data();
			if (data->focused) {
				app_ui_scene_proc_t proc = _get_ui_proc(info);
				if (proc != NULL)
					proc(scr, MSG_VIEW_FOCUS, NULL);
			}
		}
	}

	if (code == LV_EVENT_SCREEN_UNLOADED) {
		SYS_LOG_INF("UI %u unloaded", id);
	}

	if (code == LV_EVENT_DELETE) {
		app_ui_scene_proc_t proc = _get_ui_proc_by_id(id);
		if (proc != NULL)
			proc(scr, MSG_VIEW_DELETE, NULL);
	}
}

static int _main_view_load_ui(view_data_t * view_data, view_user_msg_data_t * msg_data)
{
	lv_disp_t *disp = view_data->display;
	app_view_data_t *data = view_data->user_data;
	uint16_t old_ui = data->cur_ui;
	uint16_t new_ui = msg_data->id;
	uint8_t anim_type = (uintptr_t)msg_data->data;
	const app_ui_info_t *old_info, *new_info;
	lv_obj_t *new_scr, *old_scr;

	if (data->cur_ui == new_ui)
		return 0;

	new_scr = lv_obj_create(NULL);
	if (new_scr == NULL)
		return -ENOMEM;

	new_info = app_ui_get_info(new_ui);
	if (new_info != NULL) {
		app_ui_scene_proc_t proc = _get_ui_proc(new_info);
		int res;

		if (proc != new_info->scenes[0].proc) {
			res = proc(new_scr, MSG_VIEW_CREATE, (void *)new_info);
		} else {
			res = proc(new_scr, MSG_VIEW_CREATE, (void *)new_info->scenes[0].presenter);
			if (res) {
				SYS_LOG_ERR("Scene %u:%u create failed", new_info->id, new_info->scenes[0].id);
			} else {
				SYS_LOG_INF("Scene %u:%u created", new_info->id, new_info->scenes[0].id);
			}
		}

		if (res) {
			lv_obj_del(new_scr);
			return res;
		}

		/* initialize current scene as the center scene */
		app_ui_stack_update_scene(new_info->scenes[new_info->n_scenes[0]].id);
	}

	lv_obj_add_style(new_scr, &data->style, 0);
	lv_obj_add_event_cb(new_scr, _scr_event_cb, LV_EVENT_ALL, (void *)(uintptr_t)new_ui);

	old_scr = _get_scr_act(disp);
	old_info = app_ui_get_info(old_ui);

	if (old_info != NULL && data->focused) {
		app_ui_scene_proc_t proc = _get_ui_proc(old_info);
		proc(old_scr, MSG_VIEW_DEFOCUS, NULL);
	}

	data->cur_ui = new_ui; /* corresponding the act_scr returned by _get_scr_act() */

	if (old_ui == APP_UI_INVALID || new_ui == APP_UI_INVALID ||
		anim_type == APP_UI_LOAD_ANIM_NONE || DEF_UI_LOAD_ANIM_DURATION_MS <= 0) {
		/* After this, act_scr changed */
		lv_scr_load_anim(new_scr, APP_UI_LOAD_ANIM_NONE, 0, 0, true);
	} else {
		/* update refresh rate for animation */
		uint8_t anim_fps = app_ui_get_transition_fps(new_info, old_info, anim_type);
		app_ui_set_fps(anim_fps);

		lv_scr_load_anim(new_scr, anim_type, DEF_UI_LOAD_ANIM_DURATION_MS, 0, true);

		/* start anim immediately which updates prev_scr and act_scr of the display */
		lv_anim_refr_now();
	}

	//if (new_ui == APP_UI_INVALID)
	//	lv_timer_pause(disp->refr_timer);

	return 0;
}

static int _main_view_focus(view_data_t * view_data, bool focused)
{
	app_view_data_t *data = view_data->user_data;
	app_ui_scene_proc_t proc = _get_ui_proc_by_id(data->cur_ui);

	data->focused = focused;

	if (proc != NULL) {
		lv_obj_t *scr = _get_scr_act(view_data->display);
		return proc(scr, focused ? MSG_VIEW_FOCUS : MSG_VIEW_DEFOCUS, NULL);
	}

	return 0;
}

static int _main_view_focus_scene(view_data_t * view_data, view_user_msg_data_t * msg_data)
{
	app_view_data_t *data = view_data->user_data;
	app_ui_scene_proc_t proc = _get_ui_proc_by_id(data->cur_ui);

	if (proc != NULL) {
		lv_obj_t *scr = _get_scr_act(view_data->display);
		return proc(scr, MSG_VIEW_FOCUS_SCENE, msg_data);
	}

	return -ESRCH;
}

static int _main_view_dispatch(view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	app_view_data_t *data = view_data->user_data;
	app_ui_scene_proc_t proc = _get_ui_proc_by_id(data->cur_ui);

	if (proc != NULL) {
		lv_obj_t *scr = _get_scr_act(view_data->display);
		return proc(scr, msg_id, msg_data);
	}

	return -EINVAL;
}

static int _main_view_delete(view_data_t * view_data)
{
	app_view_data_t *data = view_data->user_data;

	if (data != NULL) {
		lv_style_reset(&data->style);
		app_mem_free(data);
	}

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int app_ui_set_scene_scrollable(bool scrollable)
{
	if (!is_in_ui_thread())
		return -EPERM;

	view_data_t *view_data = view_get_data(APP_MAIN_VIEW);
	if (view_data == NULL)
		return -ESRCH;

	app_view_data_t *data = view_data->user_data;
	if (data == NULL)
		return -ESRCH;

	app_ui_scene_proc_t proc = _get_ui_proc_by_id(data->cur_ui);
	if (proc) {
		lv_obj_t *scr = _get_scr_act(view_data->display);
		view_user_msg_data_t msg_data = { .data = (void *)(uintptr_t)scrollable, };
		proc(scr, MSG_VIEW_CHANGE_SCROLLABLE, &msg_data);
	}

	return 0;
}

int app_main_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_LAYOUT:
		return _main_view_layout(view_data);
	case MSG_VIEW_LOAD_UI:
		return _main_view_load_ui(view_data, msg_data);
	case MSG_VIEW_FOCUS_SCENE:
		return _main_view_focus_scene(view_data, msg_data);
	case MSG_VIEW_KEY:
	case MSG_VIEW_PAINT:
		return _main_view_dispatch(view_data, msg_id, msg_data);
	case MSG_VIEW_DELETE:
		return _main_view_delete(view_data);
	case MSG_MSGBOX_POPUP:
		return (msgbox_cache_num_popup_get() == 1) ? _main_view_focus(view_data, false) : 0;
	case MSG_MSGBOX_CLOSE:
		return (msgbox_cache_num_popup_get() == 0) ? _main_view_focus(view_data, true) : 0;
	default:
		return 0;
	}
}

VIEW_DEFINE2(app_main_view, app_main_view_handler, NULL, NULL, APP_MAIN_VIEW, \
		0, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
