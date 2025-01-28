/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file view stack interface
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <ui_manager.h>
#include <string.h>
#include <assert.h>
#include <view_stack.h>
#ifdef CONFIG_UI_SWITCH_EFFECT
#include "ui_effects/switch_effect.h"
#endif

LOG_MODULE_REGISTER(view_stack, LOG_LEVEL_INF);

typedef struct {
	/* view cache descripter */
	const view_cache_dsc_t *cache;
	/* view group descripter */
	const view_group_dsc_t *group;
	/* presenter of the view, used only when cache == NULL */
	const void *presenter;
	/* init focused view id */
	uint16_t id;
	/* init focused main view id for view cache */
	uint16_t main_id;
} view_stack_data_t;

typedef struct {
	view_stack_data_t data[CONFIG_VIEW_STACK_LEVEL];
	uint8_t num;
	uint8_t inited : 1;
} view_stack_ctx_t;

static view_stack_ctx_t view_stack;
static OS_MUTEX_DEFINE(mutex);

static int _view_stack_update(const view_stack_data_t *data, bool push_stack);
static int _view_stack_jump(view_stack_data_t *old_data,
		const view_stack_data_t *new_data);

int view_stack_init(void)
{
	int res = 0;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.inited) {
		res = -EALREADY;
		goto out_unlock;
	}

	view_stack.inited = 1;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

void view_stack_deinit(void)
{
	os_mutex_lock(&mutex, OS_FOREVER);
	view_stack.inited = 0;
	os_mutex_unlock(&mutex);

	view_stack_clean();
}

uint8_t view_stack_get_num(void)
{
	return view_stack.num;
}

uint16_t view_stack_get_top(void)
{
	view_stack_data_t * top;
	uint16_t focused = VIEW_INVALID_ID;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num == 0) {
		goto out_unlock;
	}

	top = &view_stack.data[view_stack.num - 1];
	if (top->cache) {
		focused = view_cache_get_focus_view();
	} else {
		focused = top->id;
	}

out_unlock:
	os_mutex_unlock(&mutex);
	return focused;
}

const view_cache_dsc_t * view_stack_get_top_cache(void)
{
	view_stack_data_t * top;
	const view_cache_dsc_t *dsc = NULL;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num > 0) {
		top = &view_stack.data[view_stack.num - 1];
		dsc = top->cache;
	}

	os_mutex_unlock(&mutex);
	return dsc;
}

uint16_t view_stack_level_get_view_id(uint16_t level_id)
{
	uint16_t focused = VIEW_INVALID_ID;
	os_mutex_lock(&mutex, OS_FOREVER);
	if(level_id < view_stack.num)
		focused = view_stack.data[level_id].id;
	os_mutex_unlock(&mutex);
	return focused;
}

void view_stack_clean(void)
{
	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num > 0) {
		_view_stack_jump(&view_stack.data[view_stack.num - 1], NULL);
		view_stack.num = 0;
	}

	os_mutex_unlock(&mutex);
}

int view_stack_pop_home(void)
{
	int res = -ENODATA;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num <= 1) {
		goto out_unlock;
	}

	res = _view_stack_jump(&view_stack.data[view_stack.num - 1], &view_stack.data[0]);
	view_stack.num = 1;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

int view_stack_pop(void)
{
	int res = -ENODATA;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.num <= 1) {
		goto out_unlock;
	}
#ifdef CONFIG_UI_SWITCH_EFFECT
	ui_switch_effect_set_anim_dir(true);	//quit reverse
#endif
	res = _view_stack_jump(&view_stack.data[view_stack.num - 1],
					&view_stack.data[view_stack.num - 2]);
	view_stack.num--;

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

int view_stack_push_cache(const view_cache_dsc_t *dsc, uint16_t view_id)
{
	return view_stack_push_cache2(dsc, view_id, VIEW_INVALID_ID);
}

int view_stack_push_cache2(const view_cache_dsc_t *dsc,
		uint16_t focus_view_id, uint16_t main_view_id)
{
	view_stack_data_t data = {
		.cache = dsc,
		.group = NULL,
		.presenter = NULL,
		.id = focus_view_id,
		.main_id = main_view_id,
	};

	return _view_stack_update(&data, true);
}

int view_stack_push_group(const view_group_dsc_t *dsc)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = dsc,
		.presenter = NULL,
		.id = dsc->vlist[dsc->idx],
	};

	return _view_stack_update(&data, true);
}

int view_stack_push_view(uint16_t view_id, const void *presenter)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = NULL,
		.presenter = presenter,
		.id = view_id,
	};
#ifdef CONFIG_UI_SWITCH_EFFECT
	ui_switch_effect_set_anim_dir(false);
#endif
	return _view_stack_update(&data, true);
}

int view_stack_jump_cache(const view_cache_dsc_t *dsc, uint16_t view_id)
{
	return view_stack_jump_cache2(dsc, view_id, VIEW_INVALID_ID);
}

int view_stack_jump_cache2(const view_cache_dsc_t *dsc,
		uint16_t focus_view_id, uint16_t main_view_id)
{
	view_stack_data_t data = {
		.cache = dsc,
		.group = NULL,
		.presenter = NULL,
		.id = focus_view_id,
		.main_id = main_view_id,
	};
#ifdef CONFIG_UI_SWITCH_EFFECT
	ui_switch_effect_set_anim_dir(false);
#endif
	return _view_stack_update(&data, false);
}

int view_stack_jump_group(const view_group_dsc_t *dsc)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = dsc,
		.presenter = NULL,
		.id = dsc->vlist[dsc->idx],
	};

	return _view_stack_update(&data, false);
}

int view_stack_jump_view(uint16_t view_id, const void *presenter)
{
	view_stack_data_t data = {
		.cache = NULL,
		.group = NULL,
		.presenter = presenter,
		.id = view_id,
	};

	return _view_stack_update(&data, false);
}

void view_stack_dump(void)
{
	uint8_t i;
	int8_t idx;

	os_mutex_lock(&mutex, OS_FOREVER);

	os_printk("view stack:\n", view_stack.num);

	for (idx = view_stack.num - 1; idx >= 0; idx--) {
		if (view_stack.data[idx].cache) {
			os_printk("[%d] cache:\n", idx);

			os_printk("\t main(%u):", view_stack.data[idx].cache->num);
			for (i = 0; i < view_stack.data[idx].cache->num; i++) {
				os_printk(" %u", view_stack.data[idx].cache->vlist[i]);
			}

			os_printk("\n\t cross: %u %u\n", view_stack.data[idx].cache->cross_vlist[0],
					view_stack.data[idx].cache->cross_vlist[1]);
		} else if (view_stack.data[idx].group) {
			os_printk("[%d] group:", idx, view_stack.data[idx].group->num);
			for (i = 0; i < view_stack.data[idx].group->num; i++) {
				os_printk(" %u", i, view_stack.data[idx].group->vlist[i]);
			}
			os_printk("\n");
		} else {
			os_printk("[%d] view: %u\n", idx, view_stack.data[idx].id);
		}
	}

	os_printk("\n");

	os_mutex_unlock(&mutex);
}

static int _view_stack_update(const view_stack_data_t *data, bool push_stack)
{
	view_stack_data_t *old_data = NULL;
	int res = 0;
	int new_idx;

	os_mutex_lock(&mutex, OS_FOREVER);

	if (view_stack.inited == 0) {
		res = -ESRCH;
		goto out_unlock;
	}

	if ((view_stack.num >= ARRAY_SIZE(view_stack.data)) && push_stack) {
		res = -ENOSPC;
		goto out_unlock;
	}

	if (view_stack.num > 0) {
		old_data = &view_stack.data[view_stack.num - 1];
	} else {
		push_stack = true;
	}

	res = _view_stack_jump(old_data, data);
	if (!res) {
		if (push_stack) {
			new_idx = view_stack.num ++;
		} else {
			new_idx = view_stack.num - 1;
		}
		memcpy(&view_stack.data[new_idx], data, sizeof(*data));
	}

out_unlock:
	os_mutex_unlock(&mutex);
	return res;
}

static bool _view_group_has_view(const view_group_dsc_t *group, uint8_t view_id)
{
	int i;

	for (i = group->num - 1; i >= 0; i--) {
		if (view_id == group->vlist[i])
			return true;
	}

	return false;
}

static void _view_group_scroll_cb(uint16_t view_id, uint8_t msg_id)
{
	view_stack_data_t * data;

	if (msg_id != MSG_VIEW_SCROLL_END) {
		return;
	}

	os_mutex_lock(&mutex, OS_FOREVER);

	data = &view_stack.data[view_stack.num - 1];
	if (view_stack.num < 1 || data->group == NULL) {
		goto out_unlock;
	}

	if (data->group->scroll_cb)
		data->group->scroll_cb(view_id);

out_unlock:
	os_mutex_unlock(&mutex);
}

static void _view_group_monitor_cb(uint16_t view_id, uint8_t msg_id, void *msg_data)
{
	view_stack_data_t * data;

	os_mutex_lock(&mutex, OS_FOREVER);

	data = &view_stack.data[view_stack.num - 1];
	if (view_stack.num < 1 || data->group == NULL) {
		goto out_unlock;
	}

	if (_view_group_has_view(data->group, view_id) == false) {
		goto out_unlock;
	}

	if (msg_id == MSG_VIEW_FOCUS || msg_id == MSG_VIEW_DEFOCUS) {
		bool focused = (msg_id == MSG_VIEW_FOCUS);

		if (data->group->focus_cb)
			data->group->focus_cb(view_id, focused);

		if (focused)
			data->id = view_id;
	}

out_unlock:
	os_mutex_unlock(&mutex);
}

static int _view_stack_jump(view_stack_data_t *old_data,
		const view_stack_data_t *new_data)
{
	uint16_t old_view = VIEW_INVALID_ID;
	uint16_t new_view = VIEW_INVALID_ID;
	int8_t i;
	int res = 0;

	if (old_data) {
		if (old_data->cache) {
			old_view = view_cache_get_focus_view();
			old_data->id = old_view;
			old_data->main_id = view_cache_get_focus_main_view();
			view_cache_deinit();
		} else if (old_data->group) {
			ui_manager_set_monitor_callback(NULL);
			if (old_data->group->scroll_cb)
				ui_manager_set_scroll_callback(NULL);

			old_view = old_data->id;
			for (i = old_data->group->num - 1; i >= 0; i--) {
				if (old_data->group->vlist[i] != old_view)
					ui_view_delete(old_data->group->vlist[i]);
			}

			/* delete the focused view at last to avoid unexpected focus changes */
			ui_view_delete(old_view);
		} else {
			old_view = old_data->id;
			ui_view_delete(old_data->id);
		}
	}

	if (new_data) {
		if (new_data->cache) {
			res = view_cache_init2(new_data->cache, new_data->id, new_data->main_id);
		} else if (new_data->group) {
			uint8_t idx = new_data->group->idx;

			ui_manager_set_monitor_callback(_view_group_monitor_cb);
			if (new_data->group->scroll_cb)
				ui_manager_set_scroll_callback(_view_group_scroll_cb);

			for (i = new_data->group->num - 1; i >= 0; i--) {
				if (new_data->group->vlist[i] == new_data->id) {
					idx = i;
					break;
				}
			}

			ui_view_create(new_data->group->vlist[idx],
					new_data->group->plist ? new_data->group->plist[idx] : NULL,
					UI_CREATE_FLAG_SHOW);

			for (i = new_data->group->num - 1; i >= 0; i--) {
				if (i != idx) {
					ui_view_create(new_data->group->vlist[i],
							new_data->group->plist ? new_data->group->plist[i] : NULL,
							0);
				}
			}

			/* trigger first scroll_cb to notify app */
			if (new_data->group->scroll_cb)
				new_data->group->scroll_cb(new_data->group->vlist[idx]);
		} else {
			res = ui_view_create(new_data->id, new_data->presenter, UI_CREATE_FLAG_SHOW);
		}

		new_view = new_data->id;
	}

	SYS_LOG_INF("view_stack: jump %d -> %d (res=%d)", old_view, new_view, res);
	return res;
}
