/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <sdfs.h>
#include "launcher_ui.h"

/*********************
 *      DEFINES
 *********************/
#define DMEO_INIT_FONT_SIZE		32
#define DEMO_FONT_PATH			CONFIG_APP_FONT_DISK"/vfont.ttf"


/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t *text;
	lv_timer_t * timer;
	lv_font_t font;
	int currrent_size;
} text_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static char demo_text[64] = "文字演示DEMO123";

/**********************
 *  STATIC FUNCTIONS
 **********************/

static int _text_scene_load_resource(text_scene_data_t *data)
{
	int ret;

	printf("before font open \n");
	ret = lvgl_freetype_font_open(&data->font, DEMO_FONT_PATH, DMEO_INIT_FONT_SIZE);
	if (ret != 0)
	{
		printf("open font failed\n");
		return -1;
	}
	printf("after font open, font %p, get_dsc_cb %p\n", &data->font, data->font.get_glyph_dsc);
	data->currrent_size = DMEO_INIT_FONT_SIZE;
	return 0;
}

static void _text_scale(text_scene_data_t* data, int font_size)
{
	/*
	if(data->currrent_size == font_size)
	{
		return;
	}
	*/

	//if(data->currrent_size != DMEO_INIT_FONT_SIZE)
	{
		lvgl_freetype_font_close(&data->font);
		SYS_LOG_INF("zoom font to size %d\n", font_size);
		lvgl_freetype_font_open(&data->font, DEMO_FONT_PATH, font_size);
		data->currrent_size = font_size;
	}
	
}

static void _timer_cb(lv_timer_t *timer)
{
	static int font_size = 32;
	static int zoom_out = 1;
	text_scene_data_t * data = timer->user_data;

	SYS_LOG_INF("zoom timer cb data %p\n", data);
	if(data)
	{
		_text_scale(data, font_size);

		if(zoom_out)
		{
			font_size += 8;
			if(font_size > 96)
			{
				zoom_out = 0;
				font_size = 64;
			}
		}
		else
		{
			font_size -= 8;
			if(font_size < 16)
			{
				zoom_out = 1;
				font_size = 24;
			}
		}

		lv_label_set_text(data->text, demo_text);
	}
}

static int _font_scene_create(lv_obj_t * scr, const void * presenter)
{
	text_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	if (_text_scene_load_resource(data)) {
		SYS_LOG_ERR("load res failed");
		app_mem_free(data);
		return -ENOENT;
	}

	data->text = lv_label_create(scr);
	lv_obj_set_pos(data->text, 0, 200);
	lv_obj_set_style_text_font(data->text, &data->font, LV_PART_MAIN);
	lv_obj_set_style_text_color(data->text, lv_color_white(), LV_PART_MAIN);
	lv_label_set_text(data->text, demo_text);

	data->timer = lv_timer_create(_timer_cb, 32, data);

	lv_obj_set_user_data(scr, data);
	return 0;
}

static int _font_scene_delete(lv_obj_t * scr)
{
	text_scene_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	lv_obj_del(data->text);
	if (data->timer)
		lv_timer_del(data->timer);

	lvgl_freetype_font_close(&data->font);
	app_mem_free(data);
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int font_scene_proc(void * scr, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		return _font_scene_create(scr, msg_data);
	case MSG_VIEW_DELETE:
		return _font_scene_delete(scr);
	default:
		return 0;
	}
}
