/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view cache interface
 */

#ifndef CONFIG_VIEW_SCROLL_MEM_DEFAULT

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <ui_manager.h>
#include <view_cache.h>
#include <string.h>
#include <assert.h>

LOG_MODULE_REGISTER(view_cache, LOG_LEVEL_INF);

#define MAX_VIEW_CACHE    (sizeof(uint32_t) * 8)

typedef struct {
	const view_cache_dsc_t *dsc;
	uint32_t stat;
	uint8_t rotate : 1;
	uint8_t rebound : 1;
	uint8_t transforming : 1;
	uint8_t serial_load_paused : 1;
	int8_t main_idx; /* focused main idx in vlist */
	int8_t focus_idx; /* focused idx also considering cross_vlist */
	int8_t load_idx; /* the index that is loading */

	/* save the initial param */
	int8_t init_main_idx;
	int8_t init_focus_idx;
	uint16_t last_focus_view;
} view_cache_ctx_t;

static int _view_cache_set_focus(uint16_t view_id);
static int _view_cache_set_focus_idx(int8_t main_idx);

static view_cache_ctx_t view_cache_ctx;
static view_cache_focus_cb_t last_focus_cb;
static uint16_t last_focus_view;
static OS_MUTEX_DEFINE(view_cache_mutex);

static uint16_t _view_cache_get_view_id(uint8_t idx)
{
	if (idx < view_cache_ctx.dsc->num) {
		return view_cache_ctx.dsc->vlist[idx];
	} else if (idx < view_cache_ctx.dsc->num + 2) {
		return view_cache_ctx.dsc->cross_vlist[idx - view_cache_ctx.dsc->num];
	} else {
		return VIEW_INVALID_ID;
	}
}

static const void * _view_cache_get_presenter(uint8_t idx)
{
	if (idx < view_cache_ctx.dsc->num) {
		return view_cache_ctx.dsc->plist ? view_cache_ctx.dsc->plist[idx] : NULL;
	} else if (idx < view_cache_ctx.dsc->num + 2) {
		return view_cache_ctx.dsc->cross_plist[idx - view_cache_ctx.dsc->num];
	} else {
		return NULL;
	}
}

static uint8_t _view_cache_get_create_flags(uint8_t idx)
{
	if (idx < view_cache_ctx.dsc->num && view_cache_ctx.dsc->vlist_create_flags) {
		return view_cache_ctx.dsc->vlist_create_flags[idx];
	} else {
		return 0;
	}
}

static int8_t _view_cache_get_main_idx(uint16_t view_id)
{
	int8_t idx;

	for (idx = view_cache_ctx.dsc->num - 1; idx >= 0; idx--) {
		if (view_cache_ctx.dsc->vlist[idx] == view_id) {
			return idx;
		}
	}

	return -1;
}

static int8_t _view_cache_get_idx(uint16_t view_id)
{
	if (view_id == view_cache_ctx.dsc->cross_vlist[0]) {
		return view_cache_ctx.dsc->num;
	} else if (view_id == view_cache_ctx.dsc->cross_vlist[1]) {
		return view_cache_ctx.dsc->num + 1;
	} else {
		return _view_cache_get_main_idx(view_id);
	}
}

static int _view_cache_load(uint8_t idx, uint8_t create_flags)
{
	uint16_t view_id;

	if (view_cache_ctx.stat & (1 << idx))
		return -EALREADY;

	view_id = _view_cache_get_view_id(idx);
	if (view_id != VIEW_INVALID_ID) {
		view_cache_ctx.stat |= (1 << idx);
		create_flags |= _view_cache_get_create_flags(idx);
		return ui_view_create(view_id, _view_cache_get_presenter(idx), create_flags);
	}

	return -EINVAL;
}

static void _view_cache_unload(uint8_t idx)
{
	uint16_t view_id;

	if (!(view_cache_ctx.stat & (1 << idx)))
		return;

	view_id = _view_cache_get_view_id(idx);
	assert(view_id != VIEW_INVALID_ID);

	view_cache_ctx.stat &= ~(1 << idx);
	ui_view_delete(view_id);
}

static int _view_cache_set_attr(uint8_t idx, uint16_t attr, bool keep_pos, bool by_msg)
{
	uint16_t view_id;

	if (!(view_cache_ctx.stat & (1 << idx)))
		return -EINVAL;

	view_id = _view_cache_get_view_id(idx);
	assert(view_id != VIEW_INVALID_ID);

	if (by_msg) {
		return ui_view_set_drag_attribute(view_id, attr, keep_pos);
	} else {
		return view_set_drag_attribute(view_id, attr, keep_pos);
	}
}

static int8_t _view_cache_rotate_main_idx(int8_t idx)
{
	while (idx < 0)
		idx += view_cache_ctx.dsc->num;

	while (idx >= view_cache_ctx.dsc->num)
		idx -= view_cache_ctx.dsc->num;

	return idx;
}

static bool _view_cache_main_idx_is_in_range(int8_t idx)
{
	int8_t min_idx = view_cache_ctx.main_idx - 1;
	int8_t max_idx = view_cache_ctx.main_idx + 1;

	if (view_cache_ctx.rotate) {
		if (view_cache_ctx.dsc->num <= (1 * 2 + 1))
			return true;

		idx = _view_cache_rotate_main_idx(idx);
		if (min_idx < 0) {
			min_idx += view_cache_ctx.dsc->num;
			if (idx >= min_idx || idx <= max_idx)
				return true;
		} else if (max_idx >= view_cache_ctx.dsc->num) {
			max_idx -= view_cache_ctx.dsc->num;
			if (idx >= min_idx || idx <= max_idx)
				return true;
		} else if (idx >= min_idx && idx <= max_idx) {
			return true;
		}
	} else {
		if (idx >= UI_MAX(min_idx, 0) &&
			idx <= UI_MIN(max_idx, view_cache_ctx.dsc->num - 1))
			return true;
	}

	return false;
}

static int _view_cache_load_main(int8_t idx, uint8_t create_flags)
{
	if (view_cache_ctx.rotate) {
		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return -EINVAL;
	}

	return _view_cache_load(idx, create_flags);
}

static void _view_cache_unload_main(int8_t idx, bool forced)
{
	if (view_cache_ctx.rotate) {
		if (!forced && _view_cache_main_idx_is_in_range(idx))
			return;

		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return;
	}

	_view_cache_unload(idx);
}

static int _view_cache_set_attr_main(int8_t idx, uint16_t attr, bool by_msg)
{
	if (view_cache_ctx.rotate) {
		idx = _view_cache_rotate_main_idx(idx);
	} else if (idx < 0 || idx >= view_cache_ctx.dsc->num) {
		return -EINVAL;
	}

	return _view_cache_set_attr(idx, attr, false, by_msg);
}

static uint16_t _view_cache_rotate_drag_attr(uint16_t drag_attr)
{
	static const uint8_t s_attr_rotated[][4] = {
		{ UI_DRAG_DROPLEFT, UI_DRAG_DROPRIGHT, UI_DRAG_DROPUP, UI_DRAG_DROPDOWN },
		{ UI_DRAG_DROPUP, UI_DRAG_DROPDOWN, UI_DRAG_DROPRIGHT, UI_DRAG_DROPLEFT },
		{ UI_DRAG_DROPRIGHT, UI_DRAG_DROPLEFT, UI_DRAG_DROPDOWN, UI_DRAG_DROPUP },
	};

	uint16_t rotation = view_manager_get_disp_rotation();
	const uint8_t *rotated_attr;
	uint16_t new_attr = drag_attr & ~UI_DRAG_DIR_MASK;

	if (rotation == 0 || !(drag_attr & UI_DRAG_DIR_MASK))
		return drag_attr;

	if (rotation == 90) {
		rotated_attr = s_attr_rotated[0];
	} else if (rotation == 180) {
		rotated_attr = s_attr_rotated[1];
	} else {
		rotated_attr = s_attr_rotated[2];
	}

	if (drag_attr & 0x0f) {
		for (int i = 0; i < 4; i++) {
			if (drag_attr & (1 << i)) {
				new_attr |= (uint16_t)rotated_attr[i];
			}
		}
	}

	if (drag_attr & 0xf0) {
		for (int i = 4; i < 8; i++) {
			if (drag_attr & (1 << i)) {
				new_attr |= (uint16_t)rotated_attr[i - 4] << 4;
			}
		}
	}

	return new_attr;
}

static uint16_t _view_cache_decide_attr_main(int8_t idx)
{
	int8_t left_idx = view_cache_ctx.main_idx - 1;
	int8_t right_idx = view_cache_ctx.main_idx + 1;
	uint16_t attr = 0;

	if (view_cache_ctx.rotate) {
		idx = _view_cache_rotate_main_idx(idx);
		left_idx = _view_cache_rotate_main_idx(left_idx);
		right_idx = _view_cache_rotate_main_idx(right_idx);
	}

	if (view_cache_ctx.dsc->type == LANDSCAPE) {
		if (view_cache_ctx.rotate && view_cache_ctx.dsc->num == 2) {
			attr = (idx == view_cache_ctx.main_idx) ? 0 :
						(UI_DRAG_MOVERIGHT | UI_DRAG_MOVELEFT);
		} else if (idx == left_idx) {
			attr = UI_DRAG_MOVERIGHT;
		} else if (idx == right_idx) {
			attr = UI_DRAG_MOVELEFT;
		}
	} else {
		if (view_cache_ctx.rotate && view_cache_ctx.dsc->num == 2) {
			attr = (idx == view_cache_ctx.main_idx) ? 0 :
						(UI_DRAG_MOVEDOWN | UI_DRAG_MOVEUP);
		} else if (idx == left_idx) {
			attr = UI_DRAG_MOVEDOWN;
		} else if (idx == right_idx) {
			attr = UI_DRAG_MOVEUP;
		}
	}

	if (view_cache_ctx.rebound && (idx == 0 || idx == view_cache_ctx.dsc->num - 1)) {
		attr |= UI_DRAG_SNAPEDGE;
	}

	return _view_cache_rotate_drag_attr(attr);
}

static uint16_t _view_cache_decide_attr_cross(int8_t idx)
{
	uint16_t attr = 0;

	if (view_cache_ctx.dsc->type == LANDSCAPE) {
		if (idx == view_cache_ctx.dsc->num)
			attr = UI_DRAG_DROPDOWN;
		else if (idx == view_cache_ctx.dsc->num + 1)
			attr =  UI_DRAG_DROPUP;
	} else {
		if (idx == view_cache_ctx.dsc->num)
			attr = UI_DRAG_DROPRIGHT;
		else if (idx == view_cache_ctx.dsc->num + 1)
			attr = UI_DRAG_DROPLEFT;
	}

	return _view_cache_rotate_drag_attr(attr);
}

static void _view_cache_serial_load(void)
{
	int8_t main_idx = view_cache_ctx.main_idx;
	int8_t load_idx;

	/* load the "cross_attached_view" main view as soon as possible */
	if (!_view_cache_load(main_idx, UI_CREATE_FLAG_SHOW)) {
		return;
	}

	/* load cross views */
	for(; view_cache_ctx.load_idx >= view_cache_ctx.dsc->num; view_cache_ctx.load_idx--) {
		if (!_view_cache_load(view_cache_ctx.load_idx, UI_CREATE_FLAG_NO_FB)) {
			return;
		}
	}

	/* load right main view */
	if (view_cache_ctx.rotate) {
		load_idx = _view_cache_rotate_main_idx(main_idx + 1);
	} else {
		load_idx = main_idx + 1;
	}

	if (_view_cache_main_idx_is_in_range(load_idx) &&
		!_view_cache_load(load_idx, 0)) {
		return;
	}

	/* load left main view */
	if (view_cache_ctx.init_focus_idx == main_idx) {
		if (view_cache_ctx.rotate) {
			load_idx = _view_cache_rotate_main_idx(main_idx - 1);
		} else {
			load_idx = main_idx - 1;
		}

		if (_view_cache_main_idx_is_in_range(load_idx) &&
			!_view_cache_load(load_idx, 0)) {
			return;
		}
	}

	/* load end */
	view_cache_ctx.load_idx = -1;

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
	ui_manager_set_max_buffer_count(CONFIG_SURFACE_MAX_BUFFER_COUNT);
#endif

	/* set cross view attribute */
	if (view_cache_ctx.dsc->cross_attached_view == VIEW_INVALID_ID ||
		view_cache_ctx.dsc->cross_attached_view == view_cache_ctx.dsc->vlist[main_idx]) {
		if (view_cache_ctx.dsc->num != view_cache_ctx.init_focus_idx) {
			_view_cache_set_attr(view_cache_ctx.dsc->num,
					_view_cache_decide_attr_cross(view_cache_ctx.dsc->num), false, false);
		} else {
			_view_cache_set_attr(view_cache_ctx.dsc->num,
					_view_cache_decide_attr_cross(view_cache_ctx.dsc->num), true, false);
		}

		if (view_cache_ctx.dsc->num + 1 != view_cache_ctx.init_focus_idx) {
			_view_cache_set_attr(view_cache_ctx.dsc->num + 1,
					_view_cache_decide_attr_cross(view_cache_ctx.dsc->num + 1), false, false);
		} else {
			_view_cache_set_attr(view_cache_ctx.dsc->num + 1,
					_view_cache_decide_attr_cross(view_cache_ctx.dsc->num + 1), true, false);
		}
	}

	/* set right-left view attribute */
	_view_cache_set_attr_main(main_idx + 1, _view_cache_decide_attr_main(main_idx + 1), false);
	if (!view_cache_ctx.rotate || view_cache_ctx.dsc->num != 2)
		_view_cache_set_attr_main(main_idx - 1, _view_cache_decide_attr_main(main_idx - 1), false);

	if (view_cache_ctx.dsc->event_cb) {
		view_cache_ctx.dsc->event_cb(VIEW_CACHE_EVT_LOAD_END);
	}
}

static void _view_cache_scroll_cb(uint16_t view_id, uint8_t msg_id)
{
	const view_cache_dsc_t *dsc;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	dsc = view_cache_ctx.dsc;
	if (dsc == NULL) {
		goto out_unlock;
	}

	if (view_cache_ctx.load_idx >= 0) {
		goto out_unlock;
	}

	if (msg_id == MSG_VIEW_SCROLL_BEGIN) {
		if (view_id == dsc->cross_vlist[0] || view_id == dsc->cross_vlist[1]) {
			_view_cache_unload_main(view_cache_ctx.main_idx - 1, true);
		}

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
		ui_manager_set_max_buffer_count(1);
#endif
	} else {
		if (view_id != VIEW_INVALID_ID &&
			view_id != dsc->cross_vlist[0] &&
			view_id != dsc->cross_vlist[1]) {
			_view_cache_set_focus(view_id);
		}

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
		ui_manager_set_max_buffer_count(CONFIG_SURFACE_MAX_BUFFER_COUNT);
#endif
	}

out_unlock:
	os_mutex_unlock(&view_cache_mutex);
}

static void _view_cache_monitor_cb(uint16_t view_id, uint8_t msg_id, void *msg_data)
{
	const view_cache_dsc_t *dsc = NULL;
	uint16_t recreate_main_view = VIEW_INVALID_ID;
	uint16_t recreate_focus_view = VIEW_INVALID_ID;
	int8_t idx;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (last_focus_cb && view_id == last_focus_view && msg_id == MSG_VIEW_DEFOCUS) {
		SYS_LOG_DBG("last focus view %u handled", view_id);
		last_focus_cb(view_id, false);
		last_focus_cb = NULL;
		goto out_unlock;
	}

	dsc = view_cache_ctx.dsc;
	if (dsc == NULL) {
		goto out_unlock;
	}

	idx = _view_cache_get_idx(view_id);
	if (idx < 0) {
		goto out_unlock;
	}

	if (msg_id == MSG_VIEW_FOCUS || msg_id == MSG_VIEW_DEFOCUS) {
		bool focused = (msg_id == MSG_VIEW_FOCUS);

		if (focused) {
			last_focus_cb = NULL;
			view_cache_ctx.focus_idx = idx;
			view_cache_ctx.last_focus_view = view_id;
		} else if (view_id == view_cache_ctx.last_focus_view) {
			view_cache_ctx.last_focus_view = VIEW_INVALID_ID;
		}

		if (dsc->focus_cb)
			dsc->focus_cb(view_id, focused);
	} else if (msg_id == MSG_VIEW_SCROLL_BEGIN || msg_id == MSG_VIEW_SCROLL_END) {
		if (dsc->monitor_cb)
			dsc->monitor_cb(view_id, msg_id);
	} else if (msg_id == MSG_VIEW_LAYOUT) {
		if (view_cache_ctx.load_idx >= 0) {
			if (view_cache_ctx.transforming) {
				SYS_LOG_INF("view serial load paused after %d\n", view_id);
				view_cache_ctx.serial_load_paused = 1;
			} else {
				_view_cache_serial_load();
			}
		} else {
			int8_t idx = _view_cache_get_main_idx(view_id);
			if (idx >= 0) { /* in case that previous set_attr() failed */
				_view_cache_set_attr_main(idx, _view_cache_decide_attr_main(idx), false);
			}
		}
	} else if(msg_id == MSG_VIEW_TRANSFORM_START) {
		view_cache_ctx.transforming = 1;
	} else if (msg_id == MSG_VIEW_TRANSFORM_END) {
		SYS_LOG_INF("view serial load resume after %d\n", view_id);
		view_cache_ctx.transforming = 0;
		if (view_cache_ctx.serial_load_paused == 1) {
			view_cache_ctx.serial_load_paused = 0;
			_view_cache_serial_load();
		}
	} else if (msg_id == MSG_DISPLAY_ROTATE) {
		recreate_main_view = _view_cache_get_view_id(
				(view_cache_ctx.main_idx >= 0) ? view_cache_ctx.main_idx : view_cache_ctx.init_main_idx);
		recreate_focus_view = _view_cache_get_view_id(
				(view_cache_ctx.focus_idx >= 0) ? view_cache_ctx.focus_idx : view_cache_ctx.init_focus_idx);
	}

out_unlock:
	os_mutex_unlock(&view_cache_mutex);

	if (recreate_main_view != VIEW_INVALID_ID) {
		view_cache_deinit();
		view_cache_init2(dsc, recreate_focus_view, recreate_main_view);
	}
}

int view_cache_init(const view_cache_dsc_t *dsc, uint16_t init_view)
{
	return view_cache_init2(dsc, init_view, VIEW_INVALID_ID);
}

int view_cache_init2(const view_cache_dsc_t *dsc,
		uint16_t init_focus_view, uint16_t init_main_view)
{
	int8_t init_main_idx = -1; /* index of init_main_view */
	int8_t main_idx = -1;
	int8_t cross_idx = -1;
	int8_t cross_attached_idx = -1;
	int8_t idx;
	int res = -EINVAL;

	/* Also consider the cross views */
	if (dsc == NULL || dsc->num <= 0 || dsc->num + 2 > MAX_VIEW_CACHE) {
		goto out_fail;
	}

	if (init_focus_view == VIEW_INVALID_ID) {
		goto out_fail;
	}

	for (idx = dsc->num - 1; idx >= 0; idx--) {
		if (dsc->vlist[idx] == VIEW_INVALID_ID)
			goto out_fail;

		if (dsc->vlist[idx] == init_focus_view)
			main_idx = idx;

		if (dsc->vlist[idx] == init_main_view)
			init_main_idx = idx;

		if (dsc->vlist[idx] == dsc->cross_attached_view)
			cross_attached_idx = idx;
	}

	if (main_idx < 0) {
		if (init_focus_view == dsc->cross_vlist[0]) {
			cross_idx = dsc->num;
		} else if (init_focus_view == dsc->cross_vlist[1]) {
			cross_idx = dsc->num + 1;
		} else {
			goto out_fail;
		}

		if (init_main_idx >= 0) {
			main_idx = init_main_idx;
		} else if (cross_attached_idx >= 0) {
			main_idx = cross_attached_idx;
		} else { /* FIXME: just select the middle view ? */
			main_idx = (dsc->num / 2);
		}
	}

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc) {
		SYS_LOG_WRN("view_cache: already init");
		res = -EALREADY;
		goto out_unlock;
	}

	view_cache_ctx.dsc = dsc;
	view_cache_ctx.main_idx = -1;
	view_cache_ctx.focus_idx = -1;
	view_cache_ctx.rotate = (view_cache_ctx.dsc->rotate && view_cache_ctx.dsc->num >= 2);
	view_cache_ctx.rebound = (view_cache_ctx.dsc->rebound && !view_cache_ctx.dsc->rotate && view_cache_ctx.dsc->num >= 2);
	view_cache_ctx.init_main_idx = main_idx;
	view_cache_ctx.init_focus_idx = (cross_idx >= 0) ? cross_idx : main_idx;
	view_cache_ctx.last_focus_view = VIEW_INVALID_ID;

	ui_manager_set_scroll_callback(_view_cache_scroll_cb);
	ui_manager_set_monitor_callback(_view_cache_monitor_cb);

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
	ui_manager_set_max_buffer_count(1);
#endif

	view_cache_ctx.load_idx = dsc->num + 1;
	view_cache_ctx.main_idx = main_idx;

	/* load focused view */
	if (cross_idx < 0) {
		_view_cache_load(main_idx, UI_CREATE_FLAG_SHOW);
		if (view_cache_ctx.rebound && (main_idx == 0 || main_idx == view_cache_ctx.dsc->num - 1))
			_view_cache_set_attr_main(main_idx, UI_DRAG_SNAPEDGE, true);
	} else if (!_view_cache_load(cross_idx, UI_CREATE_FLAG_SHOW)) {
		//_view_cache_set_attr(cross_idx, _view_cache_decide_attr_cross(cross_idx), true, true);
	}

	res = 0;
out_unlock:
	SYS_LOG_INF("view_cache: init %u, main %u (res=%d)", init_focus_view, init_main_view, res);
	os_mutex_unlock(&view_cache_mutex);
	return res;
out_fail:
	os_mutex_lock(&view_cache_mutex, OS_FOREVER);
	goto out_unlock;
}

int view_cache_deinit(void)
{
	int8_t idx;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc == NULL) {
		SYS_LOG_WRN("view_cache: not init yet");
		os_mutex_unlock(&view_cache_mutex);
		return EALREADY;
	}

	ui_manager_set_scroll_callback(NULL);
	ui_gesture_stop_scroll();

	/**
	 * If view_cache_ctx.focus_idx >= 0, the current view cache must have
	 * handled the at least 1 view's MSG_VIEW_FOCUS, so the last_focus_cb set
	 * by the old view cache must have been handled when receiving the
	 * old MSG_VIEW_DEFOCUS.
	 */
	if (view_cache_ctx.last_focus_view != VIEW_INVALID_ID && view_cache_ctx.dsc->focus_cb) {
		assert(last_focus_cb == NULL);
		last_focus_cb = view_cache_ctx.dsc->focus_cb;
		last_focus_view = view_cache_ctx.last_focus_view;
		SYS_LOG_DBG("last focus view %u", last_focus_view);
	}

	/* delete the main views first to avoid unexpected focus changes */
	for (idx = 0; idx < view_cache_ctx.dsc->num + 2; idx++) {
		_view_cache_unload(idx);
	}

	/* Place this after view unload to handle the last focus view's MSG_VIEW_DEFOCUS */
	ui_manager_set_monitor_callback(NULL);

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
	ui_manager_set_max_buffer_count(CONFIG_SURFACE_MAX_BUFFER_COUNT);
#endif

	memset(&view_cache_ctx, 0, sizeof(view_cache_ctx));

	SYS_LOG_INF("view_cache: deinit");

	os_mutex_unlock(&view_cache_mutex);
	return 0;
}

uint16_t view_cache_get_focus_view(void)
{
	uint16_t view_id = VIEW_INVALID_ID;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc) {
		int8_t focus_idx = (view_cache_ctx.focus_idx >= 0) ?
				view_cache_ctx.focus_idx : view_cache_ctx.init_focus_idx;

		view_id = _view_cache_get_view_id(focus_idx);
	}

	os_mutex_unlock(&view_cache_mutex);
	return view_id;
}

uint16_t view_cache_get_focus_main_view(void)
{
	uint16_t view_id = VIEW_INVALID_ID;

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	if (view_cache_ctx.dsc) {
		int8_t main_idx = (view_cache_ctx.main_idx >= 0) ?
				view_cache_ctx.main_idx : view_cache_ctx.init_main_idx;

		view_id = _view_cache_get_view_id(main_idx);
	}

	os_mutex_unlock(&view_cache_mutex);
	return view_id;
}

int view_cache_set_focus_view(uint16_t view_id)
{
	const view_cache_dsc_t *dsc;
	uint16_t main_view_id;
	int8_t idx;

	SYS_LOG_INF("view_cache: set focus view %u", view_id);

	os_mutex_lock(&view_cache_mutex, OS_FOREVER);

	dsc = view_cache_ctx.dsc;
	if (dsc == NULL) {
		os_mutex_unlock(&view_cache_mutex);
		return -ESRCH;
	}

	idx = _view_cache_get_idx(view_id);
	if (idx < 0) {
		os_mutex_unlock(&view_cache_mutex);
		return -EINVAL;
	}

	int8_t focus_idx = (view_cache_ctx.focus_idx >= 0) ?
			view_cache_ctx.focus_idx : view_cache_ctx.init_focus_idx;
	if (idx == focus_idx) {
		os_mutex_unlock(&view_cache_mutex);
		return 0;
	}

	main_view_id = _view_cache_get_view_id(view_cache_ctx.main_idx);

	os_mutex_unlock(&view_cache_mutex);

	view_cache_deinit();
	return view_cache_init2(dsc, view_id, main_view_id);
}

int view_cache_shrink(void)
{
	return 0;
}

void view_cache_dump(void)
{
	uint8_t i;

	if (view_cache_ctx.dsc == NULL)
		return;

	os_printk("view cache:\n\t main(%u):", view_cache_ctx.dsc->num);
	for (i = 0; i < view_cache_ctx.dsc->num; i++) {
		os_printk(" %c%u", (i == view_cache_ctx.main_idx) ? '*' : ' ',
			view_cache_ctx.dsc->vlist[i]);
	}

	os_printk("\n\t cross:");
	for (; i < view_cache_ctx.dsc->num + 2; i++) {
		os_printk(" %c%u", (i == view_cache_ctx.focus_idx) ? '*' : ' ',
			view_cache_ctx.dsc->cross_vlist[i - view_cache_ctx.dsc->num]);
	}

	os_printk("\n\n");
}

static int _view_cache_set_focus(uint16_t view_id)
{
	int8_t main_idx = _view_cache_get_main_idx(view_id);

	if (main_idx < 0)
		return -EINVAL;

	if (main_idx == view_cache_ctx.main_idx) {
		uint8_t main_create_flag = 0;

		if (view_cache_ctx.dsc->cross_vlist[0] != VIEW_INVALID_ID)
			ui_view_pause(view_cache_ctx.dsc->cross_vlist[0]);
		if (view_cache_ctx.dsc->cross_vlist[1] != VIEW_INVALID_ID)
			ui_view_pause(view_cache_ctx.dsc->cross_vlist[1]);

#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
		main_create_flag = UI_CREATE_FLAG_NO_PRELOAD;
#endif
		if (!_view_cache_load_main(view_cache_ctx.main_idx - 1, main_create_flag)) {
			_view_cache_set_attr_main(main_idx - 1, _view_cache_decide_attr_main(main_idx - 1), false);
		}

		return 0;
	}

	return _view_cache_set_focus_idx(main_idx);
}

static int _view_cache_set_focus_idx(int8_t main_idx)
{
	uint16_t view_id = view_cache_ctx.dsc->vlist[main_idx];
	int8_t prefocus_idx = view_cache_ctx.main_idx;
	bool rotate2 = (view_cache_ctx.rotate && view_cache_ctx.dsc->num == 2);
	uint8_t main_create_flag = 0;

	// save cur view, _view_cache_decide_attr() depends on the main_idx.
	view_cache_ctx.main_idx = main_idx;

	if (view_cache_ctx.dsc->cross_attached_view != VIEW_INVALID_ID) {
		uint16_t cross_attr[2] = { 0, 0 };

		if (view_id == view_cache_ctx.dsc->cross_attached_view) {
			cross_attr[0] = _view_cache_decide_attr_cross(view_cache_ctx.dsc->num);
			cross_attr[1] = _view_cache_decide_attr_cross(view_cache_ctx.dsc->num + 1);
		}

		_view_cache_set_attr(view_cache_ctx.dsc->num, cross_attr[0], false, false);
		_view_cache_set_attr(view_cache_ctx.dsc->num + 1, cross_attr[1], false, false);
	}

	// clear attr for old prev/next view
	_view_cache_set_attr_main(prefocus_idx - 1, 0, false);
	if (!rotate2)
		_view_cache_set_attr_main(prefocus_idx + 1, 0, false);

	if (view_cache_ctx.rebound && (main_idx == 0 || main_idx == view_cache_ctx.dsc->num - 1))
		_view_cache_set_attr_main(main_idx, UI_DRAG_SNAPEDGE, false);

	// unload unused view
	_view_cache_unload_main(main_idx - 2, false);
	_view_cache_unload_main(main_idx + 2, false);

	// preload and show new cur view
	_view_cache_load_main(main_idx, UI_CREATE_FLAG_SHOW);

	// preload and set attr for new prev/next view
#ifdef CONFIG_VIEW_SCROLL_MEM_LOWEST
	main_create_flag = UI_CREATE_FLAG_NO_PRELOAD;
#endif

	_view_cache_load_main(main_idx - 1, main_create_flag);
	_view_cache_set_attr_main(main_idx - 1, _view_cache_decide_attr_main(main_idx - 1), false);

	if (!rotate2) {
		_view_cache_load_main(main_idx + 1, main_create_flag);
		_view_cache_set_attr_main(main_idx + 1, _view_cache_decide_attr_main(main_idx + 1), false);
	}

	return 0;
}

#endif /* CONFIG_VIEW_SCROLL_MEM_DEFAULT */
