/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file desktop_ui.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <widgets/watch_tileview.h>
#include <app_ui.h>

/*********************
 *      DEFINES
 *********************/
/* Optimize the number of inflated scenes for lower memory footprint  */
#define OPT_NUM_INFLATED_SCENES  1

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
	int16_t act_idx;
	bool focused;
	uint32_t inflated_flags;
	lv_obj_t *tileview;
	lv_obj_t **tiles;
	const app_ui_info_t *tileinfo;
} scene_tiles_ui_data_t;

/**********************
 *   GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static inline uint16_t _get_scene_count(scene_tiles_ui_data_t * data);
static int16_t _get_tile_idx_by_obj(scene_tiles_ui_data_t * data, lv_obj_t * tile);
static int16_t _get_tile_idx_by_scene(scene_tiles_ui_data_t * data, uint16_t scene_id);

static inline bool _scene_is_inflated(scene_tiles_ui_data_t * data, int16_t idx);
static void _scene_inflate(scene_tiles_ui_data_t * data, int16_t idx);
static void _scene_deflate(scene_tiles_ui_data_t * data, int16_t idx);

static bool _scene_is_in_range(scene_tiles_ui_data_t * data, int16_t idx, lv_dir_t scrl_dir);
static void _scene_tiles_do_inflate(scene_tiles_ui_data_t * data, lv_dir_t scrl_dir);

static void _scene_notify_focus(scene_tiles_ui_data_t * data, uint16_t idx, bool focused);

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static inline uint16_t _get_scene_count(scene_tiles_ui_data_t * data)
{
	return app_ui_get_scene_count(data->tileinfo);
}

static int16_t _get_tile_idx_by_obj(scene_tiles_ui_data_t * data, lv_obj_t * tile)
{
	for (int i = _get_scene_count(data) - 1; i >= 0; i--) {
		if (tile == data->tiles[i]) {
			return i;
		}
	}

	assert(0);
	return -1;
}

static int16_t _get_tile_idx_by_scene(scene_tiles_ui_data_t * data, uint16_t scene_id)
{
	for (int i = _get_scene_count(data) - 1; i >= 0; i--) {
		if (scene_id == data->tileinfo->scenes[i].id) {
			return i;
		}
	}

	return -1;
}

static inline bool _scene_is_inflated(scene_tiles_ui_data_t * data, int16_t idx)
{
	return (data->inflated_flags & BIT(idx)) ? true : false;
}

static void _scene_inflate(scene_tiles_ui_data_t * data, int16_t idx)
{
	const app_ui_scene_info_t *scene = &data->tileinfo->scenes[idx];

	if (!(data->inflated_flags & BIT(idx))) {
		/* For the convenience of managing the scene objects */
		lv_obj_t * cont = lv_obj_create(data->tiles[idx]);
		lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));

		SYS_LOG_INF("Scene %u:%u creating", data->tileinfo->id, scene->id);

		if (!scene->proc(cont, MSG_VIEW_CREATE, (void *)scene->presenter)) {
			data->inflated_flags |= BIT(idx);
			SYS_LOG_INF("Scene %u:%u created", data->tileinfo->id, scene->id);
		} else {
			SYS_LOG_ERR("Scene %u:%u create failed", data->tileinfo->id, scene->id);
			lv_obj_clean(data->tiles[idx]);
		}
	}
}

static void _scene_deflate(scene_tiles_ui_data_t * data, int16_t idx)
{
	const app_ui_scene_info_t *scene = &data->tileinfo->scenes[idx];

	if (data->inflated_flags & BIT(idx)) {
		data->inflated_flags &= ~BIT(idx);

		SYS_LOG_INF("Scene %u:%u deleting", data->tileinfo->id, scene->id);
		scene->proc(lv_obj_get_child(data->tiles[idx], 0), MSG_VIEW_DELETE, NULL);
		SYS_LOG_INF("Scene %u:%u deleted", data->tileinfo->id, scene->id);

		lv_obj_clean(data->tiles[idx]);
	}
}

static bool _main_idx_is_in_range(scene_tiles_ui_data_t * data, int16_t idx)
{
	int16_t main_idx_max = data->tileinfo->n_scenes[0] + data->tileinfo->n_scenes[1];

	if (idx >= data->act_idx - 1 && idx <= data->act_idx + 1)
		return true;

	/* require 3 main scenes to support rotation mode */
	if (data->tileinfo->main_scrl_rotated && main_idx_max >= 2) {
		if ((data->act_idx == 0 && idx == main_idx_max) ||
			(data->act_idx == main_idx_max && idx == 0)) {
			return true;
		}
	}

	return false;
}

static bool _scene_is_in_range(scene_tiles_ui_data_t * data, int16_t idx, lv_dir_t scrl_dir)
{
	int16_t main_idx_max = data->tileinfo->n_scenes[0] + data->tileinfo->n_scenes[1];
	int16_t act_idx = data->act_idx;

#if OPT_NUM_INFLATED_SCENES
	lv_dir_t main_dir = data->tileinfo->vertical ? LV_DIR_VER : LV_DIR_HOR;
	lv_dir_t cross_dir = data->tileinfo->vertical ? LV_DIR_HOR : LV_DIR_VER;

	if (idx == act_idx)
		return true;

	if (scrl_dir == LV_DIR_NONE)
		return false;

	if (act_idx <= main_idx_max) {
		/* main direction case */
		if ((scrl_dir & main_dir) && (idx <= main_idx_max))
			return _main_idx_is_in_range(data, idx);

		/* cross direction center case */
		if ((scrl_dir & cross_dir) && (idx > main_idx_max) &&
			(act_idx == data->tileinfo->n_scenes[0])) {
			if (idx == main_idx_max + data->tileinfo->n_scenes[2] ||
				idx == main_idx_max + data->tileinfo->n_scenes[2] + 1) {
				return true;
			}
		}

		return false;
	}

	/* require cross direction */
	if (!(scrl_dir & cross_dir))
		return false;

	/* cross direction top edge case */
	if (act_idx == main_idx_max + data->tileinfo->n_scenes[2]) {
		return ((idx == data->tileinfo->n_scenes[0]) ||
				(idx == act_idx - 1 && idx > main_idx_max)) ? true : false;
	}

	/* cross direction bottom edge case */
	if (act_idx == main_idx_max + data->tileinfo->n_scenes[2] + 1) {
		return ((idx == data->tileinfo->n_scenes[0]) ||
				(idx == act_idx + 1)) ? true : false;
	}

	/* cross direction normal case */
	return (idx > main_idx_max && (idx == act_idx - 1 || idx == act_idx + 1)) ? true : false;

#else
	/* always inflate cross direction */
	if (idx >= main_idx_max + 1)
		return true;

	if (act_idx >= main_idx_max + 1)
		act_idx = data->tileinfo->n_scenes[0];

	return _main_idx_is_in_range(data, idx);
#endif /* OPT_NUM_INFLATED_SCENES */
}

static void _scene_tiles_do_inflate(scene_tiles_ui_data_t * data, lv_dir_t scrl_dir)
{
	uint32_t inflated_flags = data->inflated_flags;

	/* delete first, then inflate */
	for (int i = _get_scene_count(data) - 1; i >= 0; i--) {
		if (!_scene_is_in_range(data, i, scrl_dir))
			_scene_deflate(data, i);
	}

	for (int i = _get_scene_count(data) - 1; i >= 0; i--) {
		if (_scene_is_in_range(data, i, scrl_dir))
			_scene_inflate(data, i);
	}

	if (inflated_flags != data->inflated_flags) {
		const app_ui_scene_info_t *scene = &data->tileinfo->scenes[data->act_idx];

		SYS_LOG_INF("Scene %u:* act %d, dir %x, inflated %08x",
				scene->id, data->tileinfo->id, scrl_dir, data->inflated_flags);
	}
}

static void _scene_notify_focus(scene_tiles_ui_data_t * data, uint16_t idx, bool focused)
{
	const app_ui_scene_info_t *scene = &data->tileinfo->scenes[idx];

	if (!_scene_is_inflated(data, idx)) {
		SYS_LOG_ERR("Scene %u:%u must be inflated before focuse(%d) changed",
				data->tileinfo->id, scene->id, focused);
		return;
	}

	if (focused)
		app_ui_stack_update_scene(scene->id);

	/* only apply scene focus/defocus when the whole UI is focused */
	if (data->focused) {
		SYS_LOG_INF("Scene %u:%u focused %d", data->tileinfo->id, scene->id, focused);
		scene->proc(lv_obj_get_child(data->tiles[idx], 0),
					focused ? MSG_VIEW_FOCUS : MSG_VIEW_DEFOCUS, NULL);
	}
}

static void _scene_tiles_scroll_begin_cb(lv_event_t * e)
{
	static lv_dir_t s_dir = LV_DIR_NONE;

	scene_tiles_ui_data_t *data = lv_event_get_user_data(e);
	lv_dir_t dir = lv_indev_get_scroll_dir(lv_indev_get_act());

	/* FIXME: animation scrolling in the case of param not NULL */
	if (dir != LV_DIR_NONE && lv_event_get_param(e) == NULL) {
		s_dir = dir;

#if OPT_NUM_INFLATED_SCENES
		_scene_tiles_do_inflate(data, dir);
#endif
	}

	/* update fps for scrolling */
	app_ui_set_fps(app_ui_get_scroll_fps(data->tileinfo, s_dir != LV_DIR_HOR));
}

static void _scene_tiles_scroll_end_cb(lv_event_t * e)
{
	scene_tiles_ui_data_t *data = lv_event_get_user_data(e);
	lv_indev_t *indev = lv_event_get_param(e);

	if (!indev && lv_obj_is_scrolling(data->tileview))
		return;

	lv_point_t scroll_end;
	lv_obj_get_scroll_end(data->tileview, &scroll_end);

	if (scroll_end.x == lv_obj_get_scroll_x(data->tileview) &&
		scroll_end.y == lv_obj_get_scroll_y(data->tileview)) {
		/* update fps after scrolling */
		app_ui_set_fps(data->tileinfo->scenes[data->act_idx].fps);
	}
}

static void _scene_tiles_focus_change_cb(lv_event_t * e)
{
	scene_tiles_ui_data_t *data = lv_event_get_user_data(e);
	lv_obj_t *tile = watch_tileview_get_tile_act(data->tileview);
	int16_t new_idx = _get_tile_idx_by_obj(data, tile);

	if (new_idx != data->act_idx) {
		SYS_LOG_INF("Act scene %u:%u->%u", data->tileinfo->id,
				data->tileinfo->scenes[data->act_idx].id,
				data->tileinfo->scenes[new_idx].id);

		/* defocus previous scene */
		_scene_notify_focus(data, data->act_idx, false);

#if OPT_NUM_INFLATED_SCENES
		if (!_scene_is_inflated(data, new_idx)) {
			_scene_deflate(data, data->act_idx);
			_scene_inflate(data, new_idx);

			SYS_LOG_INF("Scene %u:* inflated %08x", data->tileinfo->id, data->inflated_flags);
		}

		data->act_idx = new_idx;
#else
		/* make sure new focused scene inflated */
		data->act_idx = new_idx;
		_scene_tiles_do_inflate(data, LV_DIR_NONE);
#endif /* OPT_NUM_INFLATED_SCENES */

		/* focus new scene */
		_scene_notify_focus(data, data->act_idx, true);
	}
}

static int _scene_tiles_create(lv_obj_t * scr, const app_ui_info_t * tileinfo)
{
	scene_tiles_ui_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->tileinfo = tileinfo;

	data->tiles = app_mem_malloc(sizeof(lv_obj_t *) * _get_scene_count(data));
	if (data->tiles == NULL) {
		app_mem_free(data);
		return -ENOMEM;
	}

	memset(data->tiles, 0, sizeof(lv_obj_t *) * _get_scene_count(data));

	lv_obj_set_user_data(scr, data);

	data->tileview = watch_tileview_create(scr);
	lv_obj_set_size(data->tileview, LV_PCT(100), LV_PCT(100));
	lv_obj_set_user_data(data->tileview, data);

	watch_tileview_add_tiles(data->tileview, data->tiles,
			tileinfo->n_scenes, tileinfo->vertical ? LV_DIR_VER : LV_DIR_HOR,
			tileinfo->main_scrl_rotated, tileinfo->cross_scrl_overlapped);

	lv_obj_update_layout(data->tileview);
	watch_tileview_set_tile(data->tileview, data->tiles[tileinfo->n_scenes[0]], LV_ANIM_OFF);
	lv_obj_add_event_cb(data->tileview, _scene_tiles_focus_change_cb, LV_EVENT_VALUE_CHANGED, data);
	lv_obj_add_event_cb(data->tileview, _scene_tiles_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, data);
	lv_obj_add_event_cb(data->tileview, _scene_tiles_scroll_end_cb, LV_EVENT_SCROLL_END, data);

	data->act_idx = _get_tile_idx_by_obj(data, watch_tileview_get_tile_act(data->tileview));
	SYS_LOG_INF("Initial scene %u:%u", data->tileinfo->id, data->tileinfo->scenes[data->act_idx].id);

	/* inflate scenes */
	_scene_tiles_do_inflate(data, LV_DIR_NONE);
	/* notify focuse */
	_scene_notify_focus(data, data->act_idx, true);

	return 0;
}

static int _scene_tiles_focus_change(lv_obj_t * scr, bool focused)
{
	scene_tiles_ui_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (data->focused == focused)
		return 0;

	if (focused) {
		data->focused = true;
		_scene_notify_focus(data, data->act_idx, true);
	} else {
		lv_obj_update_snap(data->tileview, LV_ANIM_OFF);

		_scene_notify_focus(data, data->act_idx, false);
		data->focused = false;
	}

	return 0;
}

static int _scene_tiles_focus_scene(lv_obj_t * scr, view_user_msg_data_t * msg_data)
{
	scene_tiles_ui_data_t * data = lv_obj_get_user_data(scr);
	int16_t idx = _get_tile_idx_by_scene(data, msg_data->id);
	int16_t act_idx = data->act_idx;

	if (idx < 0)
		return -EINVAL;

	if (idx == data->act_idx)
		return 0;

	SYS_LOG_INF("Focus new scene %u:%u, idx %d->%d",
			data->tileinfo->id, msg_data->id, data->act_idx, idx);

	watch_tileview_set_tile(data->tileview, data->tiles[idx], LV_ANIM_OFF);

	if (act_idx == data->act_idx) {
		SYS_LOG_WRN("Fail to focus scene %u:%u", data->tileinfo->id, msg_data->id);
		return -EPERM;
	}

	if (!lv_obj_has_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE)) {
		_scene_deflate(data, act_idx); /* deflate old scene to clear scrollable */

		if (!lv_obj_has_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE)) {
			SYS_LOG_WRN("Scene %u:%u should reset scrollable %d",
					data->tileinfo->id, data->tileinfo->scenes[act_idx].id,
					lv_obj_has_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE));

			lv_obj_add_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE);
		}
	}

	return 0;
}

static int _scene_tiles_change_scrollable(lv_obj_t * scr, view_user_msg_data_t * msg_data)
{
	scene_tiles_ui_data_t * data = lv_obj_get_user_data(scr);
	bool scrollable = (msg_data->data != NULL);

	SYS_LOG_INF("Scene %u:%u scrollable %d",
			data->tileinfo->id, data->tileinfo->scenes[data->act_idx].id, scrollable);

	if (scrollable && !lv_obj_has_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE)) {
		lv_obj_add_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE);
	} else if (!scrollable && lv_obj_has_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE)) {
		lv_obj_clear_flag(data->tileview, LV_OBJ_FLAG_SCROLLABLE);
	}

	return 0;
}

static int _scene_tiles_key(lv_obj_t * scr, void * msg_data)
{
	scene_tiles_ui_data_t * data = lv_obj_get_user_data(scr);
	const app_ui_scene_proc_t proc = data->tileinfo->scenes[data->act_idx].proc;

	return proc(lv_obj_get_child(data->tiles[data->act_idx], 0), MSG_VIEW_KEY, msg_data);
}

static int _scene_tiles_paint(lv_obj_t * scr, view_user_msg_data_t * msg_data)
{
	scene_tiles_ui_data_t * data = lv_obj_get_user_data(scr);
	int16_t idx = _get_tile_idx_by_scene(data, msg_data->id);

	if (idx < 0)
		idx = data->act_idx;

	if (_scene_is_inflated(data, idx)) {
		const app_ui_scene_proc_t proc = data->tileinfo->scenes[idx].proc;
		return proc(lv_obj_get_child(data->tiles[idx], 0), MSG_VIEW_PAINT, msg_data);
	}

	return -EINVAL;
}

static int _scene_tiles_delete(lv_obj_t * scr)
{
	scene_tiles_ui_data_t *data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (data->tileview) {
		_scene_notify_focus(data, data->act_idx, false);

		for (int i = _get_scene_count(data) - 1; i >= 0; i--)
			_scene_deflate(data, i);

		lv_obj_del(data->tileview);
	}

	app_mem_free(data->tiles);
	app_mem_free(data);

	lv_obj_set_user_data(scr, NULL);
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int app_ui_scene_tiles_proc(void * scr, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		return _scene_tiles_create(scr, msg_data);
	case MSG_VIEW_FOCUS:
		return _scene_tiles_focus_change(scr, true);
	case MSG_VIEW_DEFOCUS:
		return _scene_tiles_focus_change(scr, false);
	case MSG_VIEW_FOCUS_SCENE:
		return _scene_tiles_focus_scene(scr, msg_data);
	case MSG_VIEW_CHANGE_SCROLLABLE:
		return _scene_tiles_change_scrollable(scr, msg_data);
	case MSG_VIEW_KEY:
		return _scene_tiles_key(scr, msg_data);
	case MSG_VIEW_PAINT:
		return _scene_tiles_paint(scr, msg_data);
	case MSG_VIEW_DELETE:
		return _scene_tiles_delete(scr);
	default:
		return 0;
	}
}

uint32_t app_scene_tiles_ui_get_state(bool * scrollable, lv_dir_t * scroll_dir, lv_point_t * scroll_pos)
{
	lv_obj_t * scr = lv_scr_act();
	lv_obj_t *tv = lv_obj_get_child(scr, 0);

	if (!lv_obj_check_type(tv, &watch_tileview_class))
		return 0;

	scene_tiles_ui_data_t *data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	*scrollable = lv_obj_has_flag(tv, LV_OBJ_FLAG_SCROLLABLE);
	*scroll_dir = lv_obj_get_scroll_dir(tv);
	scroll_pos->x = lv_obj_get_scroll_x(tv);
	scroll_pos->y = lv_obj_get_scroll_y(tv);
	return data->inflated_flags;
}
