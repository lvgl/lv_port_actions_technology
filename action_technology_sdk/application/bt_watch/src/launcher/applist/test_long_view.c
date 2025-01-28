/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <assert.h>
#include <app_ui.h>

static int _test_long_view_preload(view_data_t *view_data)
{
	return ui_view_layout(TEST_LONG_VIEW);
}

static void _test_long_view_drag_anim_cb(uint16_t view_id,
		const ui_point_t *drag_throw_vect, struct ui_view_anim_cfg *cfg)
{
	SYS_LOG_INF("pos %d %d, %d %d\n",
		cfg->start.x, cfg->start.y, drag_throw_vect->x, drag_throw_vect->y);

	if (LV_ABS(drag_throw_vect->y) >= 10) {
		cfg->stop.x = cfg->start.x;
		cfg->stop.y = cfg->start.y + drag_throw_vect->y * 3 / 4;
		cfg->duration = LV_ABS(drag_throw_vect->y) * 5;
	}
}

static int _test_long_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scr, lv_color_make(255, 0, 0), LV_PART_MAIN);
	lv_obj_set_style_bg_grad_color(scr, lv_color_make(0, 0, 255), LV_PART_MAIN);
	lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN);

	view_set_drag_attribute(TEST_LONG_VIEW, UI_DRAG_MOVEDOWN | UI_DRAG_MOVEUP, true);
	view_set_drag_anim_cb(TEST_LONG_VIEW, _test_long_view_drag_anim_cb);
	return 0;
}

static int _test_long_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == TEST_LONG_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _test_long_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _test_long_view_layout(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(test_longview, _test_long_view_handler, NULL,
		NULL, TEST_LONG_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT * 2);
