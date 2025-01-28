/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <app_ui.h>
#include <view_stack.h>

static const uint32_t text_list_ids[] = {
	SETTING_TEXT_1, SETTING_TEXT_2, SETTING_TEXT_3};

typedef struct setting_view_data {
	lv_obj_t *tv;
	lv_font_t font;
	lvgl_res_scene_t res_scene;

	lvgl_res_group_t layout_1;
	lvgl_res_group_t layout_2;

	lvgl_res_string_t res_txt[ARRAY_SIZE(text_list_ids)];
} setting_data_t;

static void _setting_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
	if(btn_id == 0)
		view_stack_push_view(APPLIST_ANIM_SET_VIEW, NULL);
	else if(btn_id == 1)
		view_stack_push_view(SCROLL_ANIM_SET_VIEW, NULL);
	else if(btn_id == 2)
		view_stack_push_view(SWITCH_ANIM_SET_VIEW, NULL);
}

static void _setting_ui_unload_resource(setting_data_t *data)
{
	lvgl_res_unload_group(&data->layout_1);
	lvgl_res_unload_group(&data->layout_2);
	lvgl_res_unload_strings(data->res_txt, ARRAY_SIZE(data->res_txt));
	LVGL_FONT_CLOSE(&data->font);
}

static int _setting_ui_load_resource(setting_data_t *data)
{
	int ret;

	ret = lvgl_res_load_scene(SCENE_SETTING_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_SETTING_VIEW not found");
		return -ENOENT;
	}

	ret = lvgl_res_load_group_from_scene(&data->res_scene,LAYOUT_RECT_1, &data->layout_1);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_group_from_scene(&data->res_scene,LAYOUT_RECT_2, &data->layout_2);
	if (ret < 0) {
		goto out_exit;
	}

	lvgl_res_group_t text_string;
	ret = lvgl_res_load_group_from_scene(&data->res_scene,SETTING_LIST_STRING, &text_string);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_strings_from_group(&text_string, text_list_ids, data->res_txt, ARRAY_SIZE(data->res_txt));
	if (ret < 0) {
		goto out_exit;
	}
	lvgl_res_unload_group(&text_string);

	ret = LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);
	if (ret < 0) {
		goto out_exit;
	}

out_exit:
	if (ret < 0) {
		_setting_ui_unload_resource(data);
	}
	lvgl_res_unload_scene(&data->res_scene);

	return ret;
}

static int _setting_ui_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_SETTING_VIEW, NULL, 0, lvgl_res_scene_preload_default_cb_for_view, (void *)SETTING_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _setting_ui_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	setting_data_t *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

	if (_setting_ui_load_resource(data)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}
	lv_coord_t layout_space = data->layout_2.y - data->layout_1.y;
	data->tv = lv_obj_create(scr);
	lv_obj_set_size(data->tv,0,0);
	lv_obj_set_size(data->tv,DEF_UI_VIEW_WIDTH,data->layout_1.y * 2 + layout_space * ARRAY_SIZE(data->res_txt));
	lv_obj_set_style_pad_all(data->tv,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->tv,0,LV_PART_MAIN);
	for(uint32_t i = 0 ; i < ARRAY_SIZE(data->res_txt) ; i++)
	{
		lv_obj_t *setting_icon = lv_obj_create(data->tv);
		lv_obj_set_pos(setting_icon,data->layout_1.x,data->layout_1.y + layout_space * i);
		lv_obj_set_size(setting_icon,data->layout_1.width,data->layout_1.height);
		lv_obj_set_style_pad_all(setting_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(setting_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(setting_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(setting_icon,lv_color_hex(data->layout_1.group_data->sty_data->backgroud),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(setting_icon,data->layout_1.group_data->sty_data->opaque,LV_PART_MAIN);
		lv_obj_add_event_cb(setting_icon, _setting_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(setting_icon,(void *)i);

		lv_obj_t *set_text_icon = lv_label_create(setting_icon);
		lv_obj_align(set_text_icon,LV_ALIGN_CENTER,0,0);
		lv_obj_set_width(set_text_icon,data->layout_1.width);
		lv_label_set_long_mode(set_text_icon,LV_LABEL_LONG_SCROLL);
		lv_obj_set_style_text_align(set_text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_font(set_text_icon,&data->font,LV_PART_MAIN);
		lv_obj_set_style_text_color(set_text_icon,data->res_txt[i].color,LV_PART_MAIN);
		lv_label_set_text(set_text_icon,data->res_txt[i].txt);
	}

	return 0;
}

static int _setting_ui_delete(view_data_t *view_data)
{
	setting_data_t *data = view_data->user_data;

	if (data) {
		lv_obj_delete(data->tv);
		_setting_ui_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_SETTING_VIEW);
	}
	lvgl_res_unload_scene_compact(SCENE_SETTING_VIEW);
	return 0;
}

int _setting_ui_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _setting_ui_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _setting_ui_layout(view_data);
	case MSG_VIEW_DELETE:
		return _setting_ui_delete(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(setting_ui, _setting_ui_handler, NULL, \
		NULL, SETTING_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
