/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file third_party_app view
 */

#include <assert.h>
#include <view_stack.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "app_ui.h"
#include "app_defines.h"
#include "system_app.h"
#include "third_party_app_view.h"
#include "third_party_app_ui.h"

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#include "dvfs.h"
#endif

#ifdef CONFIG_GLYPHIX

#include "gx_lvgx.h"
#include "glyphix_ats.h"

lv_font_t lv_default_font;

static int _third_party_app_view_preload(view_data_t *view_data)
{
	ARG_UNUSED(view_data);
	ui_view_update(THIRD_PARTY_APP_VIEW);
	return 0;
}

static int _third_party_app_view_layout_update(view_data_t *view_data, bool first_layout)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&lv_default_font, DEF_FONT_SIZE_NORMAL) < 0) {
		LVGL_FONT_CLOSE(&lv_default_font);
		return -1;
	}

	lv_obj_t *label = lv_label_create(scr);
	lv_obj_set_style_text_font(label, &lv_default_font, 0);
	lv_label_set_text(label, "Glyphix Applet");
	lv_obj_set_pos(label, 150, 20);
	lv_obj_set_style_text_color(label, lv_color_hex(0xffA6A6A6),
	                          LV_PART_MAIN | LV_STATE_DEFAULT);

	lv_obj_t *label1 = lv_label_create(scr);
	lv_obj_set_style_text_font(label1, &lv_default_font, 0);
	lv_label_set_text(label1, "LVGL V8");
	lv_obj_set_pos(label1, 180, 420);
	lv_obj_set_style_text_color(label1, lv_color_hex(0xffA6A6A6),
	                          LV_PART_MAIN | LV_STATE_DEFAULT);
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "glyphix");
#endif
	lvgx_applet_list(scr);
	return 0;
}

static int _third_party_app_view_layout(view_data_t *view_data)
{
	int ret;

	ret = _third_party_app_view_layout_update(view_data, 1);
	if(ret < 0)
	{
		return ret;
	}

	lv_refr_now(view_data->display);
	SYS_LOG_INF("_third_party_app_view_layout");

	return 0;
}

static int _third_party_app_view_delete(view_data_t *view_data) {
	LVGL_FONT_CLOSE(&lv_default_font);
	printk("_glyphix_enter_view_delete closed lv_default_font \n");
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "glyphix");
#endif
	return 0;
}

int _third_party_app_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _third_party_app_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _third_party_app_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _third_party_app_view_delete(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(third_party_app_view, _third_party_app_view_handler, NULL, \
		NULL, THIRD_PARTY_APP_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_WIDTH, DEF_UI_HEIGHT);

#endif
