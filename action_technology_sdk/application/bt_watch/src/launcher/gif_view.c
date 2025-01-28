/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <app_ui.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_mem.h>
#endif

/**********************
 *      TYPEDEFS
 **********************/
#define GIF_PATH DEF_UI_DISK"/test.gif"

typedef struct {
	lv_image_dsc_t gif_img;
	lv_obj_t *git_icon;
	lv_font_t font_1;
	lv_font_t font_2;
} gif_scene_data_t;

#ifdef CONFIG_UI_MANAGER
#  define GIF_MALLOC(size)        ui_mem_alloc(MEM_RES, size, __func__)
#  define GIF_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#else
#  define GIF_MALLOC(size)        lv_malloc(size)
#  define GIF_FREE(ptr)           lv_free(ptr)
#endif

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
static int _gif_view_preload(view_data_t *view_data)
{
	return ui_view_layout(GIF_VIEW);
}

static void _gif_btn_evt_handler(lv_event_t *e)
{
	lv_obj_t *gif_icon = (lv_obj_t *)lv_event_get_user_data(e);
	if(gif_img_get_paused(gif_icon))
		gif_img_resume(gif_icon);
	else
		gif_img_pause(gif_icon);
}

static int _gif_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	gif_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	if (data->git_icon == NULL) {
		data->git_icon = gif_img_create(scr);
		gif_img_set_src(data->git_icon, (const void * )GIF_PATH, false);
		lv_obj_align(data->git_icon,LV_ALIGN_CENTER,0,0);
		lv_obj_add_flag(data->git_icon,LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(data->git_icon, _gif_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data->git_icon);
	}

	view_data->user_data = data;
	return 0;
}

static int _gif_view_delete(view_data_t *view_data)
{
	gif_scene_data_t * data = view_data->user_data;

	if (data) {
		lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
		lv_obj_clean(scr);
		app_mem_free(data);
	}
	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int gif_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == GIF_VIEW);
	SYS_LOG_ERR("gif_view_handler %d",msg_id);
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _gif_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _gif_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _gif_view_delete(view_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(gif, gif_view_handler, NULL, NULL, GIF_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

