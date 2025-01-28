/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include "launcher_ui.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_font_t font;
} main_scene_data_t;

typedef struct {
	const char * text;
	uint32_t id; /* UI id */
} main_item_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static const main_item_data_t s_main_idata[] = {
	{ "SVG Render", SVG_SCENE },
	{ "Cube Rotate", CUBE_SCENE },
	{ "Text Scale", FONT_SCENE },
};

/**********************
 *  STATIC FUNCTIONS
 **********************/
static void _click_event_handler(lv_event_t * e)
{
	lv_obj_t *label = lv_event_get_target(e);
	const main_item_data_t *idata = lv_obj_get_user_data(label);
	if (idata == NULL) return;

	LV_LOG_WARN("click %s", idata->text);

	if (idata->id != APP_UI_INVALID)
		app_ui_stack_forward(idata->id, DEF_UI_LOAD_ANIM_TYPE);
}

static int _main_scene_create(lv_obj_t * scr, const void * presenter)
{
	main_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_LARGE) < 0) {
		app_mem_free(data);
		return -ENOMEM;
	}

	lv_obj_t *cont = lv_obj_create(scr);
	lv_obj_set_size(cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_center(cont);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_row(cont, 20, LV_PART_MAIN);
	lv_obj_set_style_text_font(cont, &data->font, LV_PART_MAIN);
	lv_obj_set_style_text_color(cont, lv_color_white(), LV_PART_MAIN);

	for (int i = 0; i < ARRAY_SIZE(s_main_idata); i++) {
		lv_obj_t *label = lv_label_create(cont);
		lv_label_set_text_static(label, s_main_idata[i].text);
		lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_user_data(label, (void *)&s_main_idata[i]);
		lv_obj_add_event_cb(label, _click_event_handler, LV_EVENT_SHORT_CLICKED, data);
	}

	lv_obj_set_user_data(scr, data);
	return 0;
}

static int _main_scene_delete(lv_obj_t * scr)
{
	main_scene_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	//LVGL_FONT_CLOSE(&data->font);
	app_mem_free(data);
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int main_scene_proc(void * scr, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		return _main_scene_create(scr, msg_data);
	case MSG_VIEW_DELETE:
		return _main_scene_delete(scr);
	default:
		return 0;
	}
}
