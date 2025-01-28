/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <app_ui.h>
#include <msgbox_cache.h>

/*********************
 *      DEFINES
 *********************/
#define NUM_UI_LEVELS  5
#define UI_LEVEL_TOP   (NUM_UI_LEVELS + 1)
#define UI_LEVEL_PREV  (NUM_UI_LEVELS + 2)

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	/* ui id */
	uint16_t id;
	uint16_t scene_id;
} app_ui_stack_data_t;

typedef struct {
	app_ui_stack_data_t data[NUM_UI_LEVELS];
	uint8_t num;
	uint8_t inited : 1;
} app_ui_stack_ctx_t;

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
extern uint32_t app_scene_tiles_ui_get_state(bool * scrollable, lv_dir_t * scroll_dir, lv_point_t * scroll_pos);

/**********************
 *  STATIC VARIABLES
 **********************/
static app_ui_stack_ctx_t ui_stack;
static OS_MUTEX_DEFINE(ui_stack_mutex);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int _app_ui_jump(uint16_t new_id, uint16_t old_id, lv_scr_load_anim_t anim_type)
{
	int err = ui_view_send_user_msg2(APP_MAIN_VIEW, MSG_VIEW_LOAD_UI,
	                                new_id, (void *)(uintptr_t)anim_type);

	SYS_LOG_INF("UI jump %u->%u (err=%d)", old_id, new_id, err);
	return err;
}

static uint16_t _app_ui_stack_get(int16_t level, uint16_t * p_scene_id)
{
	uint16_t id = APP_UI_INVALID;
	uint16_t scene_id = APP_SCENE_INVALID;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);
	if (level == UI_LEVEL_TOP)
		level = (int16_t)ui_stack.num - 1;
	else if (level == UI_LEVEL_PREV)
		level = (int16_t)ui_stack.num - 2;

	if (level >= 0 && level < ui_stack.num) {
		id = ui_stack.data[level].id;
		scene_id = ui_stack.data[level].scene_id;
	}
	os_mutex_unlock(&ui_stack_mutex);

	if (p_scene_id)
		*p_scene_id = scene_id;

	return id;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int app_ui_stack_init(uint16_t main_id)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (ui_stack.inited) {
		res = -EALREADY;
		goto out_unlock;
	}

	if (ui_view_create(APP_MAIN_VIEW, NULL, UI_CREATE_FLAG_NO_PRELOAD | UI_CREATE_FLAG_SHOW)) {
		res = -EINVAL;
		goto out_unlock;
	}

	ui_stack.inited = 1;

	if (main_id != APP_UI_INVALID)
		res = app_ui_stack_forward(main_id, APP_UI_LOAD_ANIM_NONE);

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_deinit(void)
{
	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited)
		goto out_unlock;

	ui_view_delete(APP_MAIN_VIEW);
	ui_stack.num = 0;
	ui_stack.inited = 0;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return 0;
}

int app_ui_stack_clean(bool blanking)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (ui_stack.num < 1) {
		res = -ENODATA;
		goto out_unlock;
	}

	if (blanking) {
		res = _app_ui_jump(APP_UI_INVALID, ui_stack.data[ui_stack.num - 1].id,
						APP_UI_LOAD_ANIM_NONE);
	}

	ui_stack.num = 0;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

uint16_t app_ui_stack_current(uint16_t * p_scene_id)
{
	return _app_ui_stack_get(UI_LEVEL_TOP, p_scene_id);
}

uint16_t app_ui_stack_prev(uint16_t * p_scene_id)
{
	return _app_ui_stack_get(UI_LEVEL_PREV, p_scene_id);
}

int app_ui_stack_forward(uint16_t id, uint8_t anim_type)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (ui_stack.num >= ARRAY_SIZE(ui_stack.data)) {
		res = -ENOSPC;
		goto out_unlock;
	}

	uint16_t old_id = (ui_stack.num == 0) ?
			APP_UI_INVALID : ui_stack.data[ui_stack.num - 1].id;

	res = _app_ui_jump(id, old_id, anim_type);
	if (res == 0)
		ui_stack.data[ui_stack.num++].id = id;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_back(uint8_t anim_type)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (ui_stack.num <= 1) {
		res = -EALREADY;
		goto out_unlock;
	}

	uint16_t old_id = ui_stack.data[ui_stack.num - 1].id;
	uint16_t new_id = ui_stack.data[ui_stack.num - 2].id;

	res = _app_ui_jump(new_id, old_id, anim_type);
	if (res == 0)
		ui_stack.num--;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_back_home(uint8_t anim_type)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (ui_stack.num <= 1) {
		res = -EALREADY;
		goto out_unlock;
	}

	uint16_t old_id = ui_stack.data[ui_stack.num - 1].id;
	uint16_t new_id = ui_stack.data[0].id;

	res = _app_ui_jump(new_id, old_id, anim_type);
	if (res == 0)
		ui_stack.num = 1;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_back_gestured(uint8_t anim_type)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	if (ui_stack.num <= 1) {
		res = -EALREADY;
		goto out_unlock;
	}

	uint16_t old_id = ui_stack.data[ui_stack.num - 1].id;
	uint16_t new_id = ui_stack.data[ui_stack.num - 2].id;

	const app_ui_info_t *info = app_ui_get_info(old_id);
	if (info->back_gesture_disabled) {
		res = -EPERM;
		goto out_unlock;
	}

	res = _app_ui_jump(new_id, old_id, anim_type);
	if (res == 0)
		ui_stack.num--;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_update(uint16_t id, uint8_t anim_type)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited) {
		res = -ESRCH;
		goto out_unlock;
	}

	uint16_t old_id = (ui_stack.num == 0) ?
			APP_UI_INVALID : ui_stack.data[ui_stack.num - 1].id;

	res = _app_ui_jump(id, old_id, anim_type);
	if (res == 0) {
		if (ui_stack.num == 0)
			ui_stack.num = 1;

		ui_stack.data[ui_stack.num - 1].id = id;
	}

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

int app_ui_stack_update_scene(uint16_t scene_id)
{
	int res = 0;

	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	if (!ui_stack.inited || ui_stack.num <= 0) {
		res = -ESRCH;
		goto out_unlock;
	}

	ui_stack.data[ui_stack.num - 1].scene_id = scene_id;

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
	return res;
}

void app_ui_stack_dump(void)
{
	os_mutex_lock(&ui_stack_mutex, OS_FOREVER);

	view_manager_dump();
	msgbox_cache_dump();

	if (!ui_stack.inited || ui_stack.num <= 0)
		goto out_unlock;

	os_printk("UI stack:\n", ui_stack.num);

	for (int idx = ui_stack.num - 1; idx >= 0; idx--) {
		const app_ui_info_t * info = app_ui_get_info(ui_stack.data[idx].id);
		uint16_t n_scenes = app_ui_get_scene_count(info);
		int i;

		if (n_scenes == 1) {
			os_printk("[%d] UI: %u-%u\n", idx, ui_stack.data[idx].id, info->scenes[0].id);
			continue;
		} else {
			uint32_t inflated = 0;
			bool scrollable = false;
			lv_dir_t scroll_dir = LV_DIR_NONE;
			lv_point_t scroll_pos = { 0, 0 };

			if (idx == ui_stack.num - 1)
				inflated = app_scene_tiles_ui_get_state(&scrollable, &scroll_dir, &scroll_pos);

			os_printk("[%d] UI: %u-%u (inflated %x, scrollable %d, scroll_dir %x, scroll_pos %d %d, act_scene %u)\n",
					idx, ui_stack.data[idx].id, ui_stack.data[idx].scene_id,
					inflated, scrollable, scroll_dir, scroll_pos.x, scroll_pos.y, ui_stack.data[idx].scene_id);

			os_printk("\t main(%u-1-%u):", info->n_scenes[0], info->n_scenes[1]);
			for (i = 0; i < info->n_scenes[0] + info->n_scenes[1] + 1; i++)
				os_printk(" %c%u", (i == info->n_scenes[0]) ? '*' : ' ', info->scenes[i].id);

			os_printk("\n\t cross(%u-%u): ", info->n_scenes[2], info->n_scenes[3]);
			for (; i < n_scenes; i++)
				os_printk("  %u", info->scenes[i].id);

			os_printk("\n");
		}
	}

	os_printk("\n");

out_unlock:
	os_mutex_unlock(&ui_stack_mutex);
}
