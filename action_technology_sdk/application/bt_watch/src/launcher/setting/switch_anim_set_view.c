/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <app_ui.h>
#include "main_view.h"
#include <widgets/simple_img.h>
#include "ui_coder/ui_coder.h"

extern const main_view_presenter_t main_view_presenter;

static const uint32_t pic_on_off_ids[] = {
	PIC_ENABLED,PIC_DISABILITY
};

static const uint32_t switch_anim_set_text_ids[] = {
	ANIM_SET_TEXT_1,
	ANIM_SET_TEXT_2,
	ANIM_SET_TEXT_3,
	ANIM_SET_TEXT_4,
	ANIM_SET_TEXT_5,
	ANIM_SET_TEXT_6,
	ANIM_SET_TEXT_7,
	ANIM_SET_TEXT_8,
};

typedef struct switch_anim_set_view_data {
	lv_obj_t *tv;
	lv_obj_t *on_off_icon[ARRAY_SIZE(switch_anim_set_text_ids)];
	lv_image_dsc_t img_on_off[ARRAY_SIZE(pic_on_off_ids)];
	lv_font_t font;

	lvgl_res_group_t layout_1;
	lvgl_res_group_t layout_2;

	lvgl_res_group_t select_group;
	lvgl_res_scene_t res_scene;
	lvgl_res_string_t res_txt[ARRAY_SIZE(switch_anim_set_text_ids)];
} switch_anim_set_data_t;


static void _switch_anim_set_mode(uint8_t id)
{
	main_view_presenter.set_switch_mode(id);
}

static void _switch_anim_set_up(switch_anim_set_data_t *data)
{
	uint8_t num = main_view_presenter.get_switch_mode();

	for(uint8_t i = 0 ; i < ARRAY_SIZE(switch_anim_set_text_ids) ; i++)
	{
		if(num == i)
			simple_img_set_src(data->on_off_icon[i], &data->img_on_off[0]);
		else
			simple_img_set_src(data->on_off_icon[i], &data->img_on_off[1]);
	}
}

static void _switch_anim_set_btn_evt_handler(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	switch_anim_set_data_t *data = lv_event_get_user_data(e);
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
	if (event == LV_EVENT_CLICKED) {
		_switch_anim_set_mode(btn_id);
		_switch_anim_set_up(data);
	}
}

static int switch_switch_anim_set_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_SWITCH_ANIM_SET_VIEW, NULL, 0, lvgl_res_scene_preload_default_cb_for_view, (void *)SWITCH_ANIM_SET_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static void _switch_anim_set_unload_resource(switch_anim_set_data_t *data)
{
	lvgl_res_unload_group(&data->layout_1);
	lvgl_res_unload_group(&data->layout_2);
	lvgl_res_unload_group(&data->select_group);
	lvgl_res_unload_strings(data->res_txt, ARRAY_SIZE(data->res_txt));
	lvgl_res_unload_pictures(data->img_on_off, ARRAY_SIZE(pic_on_off_ids));
	LVGL_FONT_CLOSE(&data->font);
}

static int _switch_anim_set_load_resource(switch_anim_set_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_SWITCH_ANIM_SET_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_SWITCH_ANIM_SET_VIEW not found");
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

	ret = lvgl_res_load_group_from_scene(&data->res_scene,ANIM_SET_SELECT, &data->select_group);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_group(&data->select_group, pic_on_off_ids, data->img_on_off,NULL, ARRAY_SIZE(data->img_on_off));
	if (ret < 0) {
		goto out_exit;
	}
	SYS_LOG_ERR("_switch_anim_set_load_resource %d",data->img_on_off[0].header.cf);

	lvgl_res_group_t text_string;
	ret = lvgl_res_load_group_from_scene(&data->res_scene,ANIM_SET_LIST_STRING, &text_string);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_strings_from_group(&text_string, switch_anim_set_text_ids, data->res_txt, ARRAY_SIZE(data->res_txt));
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
		_switch_anim_set_unload_resource(data);
	}
	lvgl_res_unload_scene(&data->res_scene);

	return ret;
}

static int switch_switch_anim_set_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	switch_anim_set_data_t *data = NULL;

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}

	memset(data, 0, sizeof(*data));

	if (_switch_anim_set_load_resource(data)) {
		app_mem_free(data);
		return -ENOENT;
	}
	lv_coord_t layout_space = data->layout_2.y - data->layout_1.y;
	coder_simulation_tp_register(scr, SWITCH_ANIM_SET_VIEW, LV_DIR_VER, 50, NULL);
	data->tv = lv_obj_create(scr);
	lv_obj_set_size(data->tv,0,0);
	lv_obj_set_size(data->tv,DEF_UI_VIEW_WIDTH,data->layout_1.y * 2 + layout_space * ARRAY_SIZE(data->res_txt));
	lv_obj_set_style_pad_all(data->tv,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->tv,0,LV_PART_MAIN);
	lv_coord_t text_x = data->layout_1.width >> 2;
	for(uint32_t i = 0 ; i < ARRAY_SIZE(data->res_txt) ; i++)
	{
		lv_obj_t *switch_anim_set_icon = lv_obj_create(data->tv);
		lv_obj_set_pos(switch_anim_set_icon,data->layout_1.x,data->layout_1.y + layout_space * i);
		lv_obj_set_size(switch_anim_set_icon,data->layout_1.width,data->layout_1.height);
		lv_obj_set_style_pad_all(switch_anim_set_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(switch_anim_set_icon,0,LV_PART_MAIN);
		lv_obj_set_style_bg_color(switch_anim_set_icon,lv_color_hex(data->layout_1.group_data->sty_data->backgroud),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(switch_anim_set_icon,data->layout_1.group_data->sty_data->opaque,LV_PART_MAIN);
		lv_obj_add_event_cb(switch_anim_set_icon, _switch_anim_set_btn_evt_handler, LV_EVENT_CLICKED, data);
		lv_obj_set_user_data(switch_anim_set_icon,(void *)i);

		lv_obj_t *text_icon = lv_label_create(switch_anim_set_icon);
		lv_obj_align(text_icon,LV_ALIGN_LEFT_MID,text_x,0);
		lv_obj_set_width(text_icon,DEF_UI_WIDTH - text_x * 2);
		lv_label_set_long_mode(text_icon,LV_LABEL_LONG_SCROLL);
		lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_LEFT,LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,data->res_txt[0].color,LV_PART_MAIN);
		lv_label_set_text(text_icon, data->res_txt[i].txt);

		data->on_off_icon[i] = simple_img_create(switch_anim_set_icon);
		lv_obj_align(data->on_off_icon[i],LV_ALIGN_RIGHT_MID,data->select_group.x,0);
	}

	view_data->user_data = data;
	_switch_anim_set_up(data);
	return 0;
}

static int switch_switch_anim_set_delete(view_data_t *view_data)
{
	switch_anim_set_data_t *data = view_data->user_data;

	if (data) {
		lv_obj_delete(data->tv);
		_switch_anim_set_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_SWITCH_ANIM_SET_VIEW);
	}
	lvgl_res_unload_scene_compact(SCENE_SWITCH_ANIM_SET_VIEW);
	return 0;
}

int _switch_switch_anim_set_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return switch_switch_anim_set_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return switch_switch_anim_set_layout(view_data);
	case MSG_VIEW_DELETE:
		return switch_switch_anim_set_delete(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(switch_switch_anim_set_ui, _switch_switch_anim_set_handler, NULL, \
		NULL, SWITCH_ANIM_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
