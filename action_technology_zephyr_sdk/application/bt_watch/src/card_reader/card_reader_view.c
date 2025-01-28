/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief card reader app view
*/

#include "card_reader.h"
#include "hotplug_manager.h"
#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>
#include <lvgl/lvgl_res_loader.h>
#include <lvgl/lvgl_bitmap_font.h>

enum {
	BMP_USB_BG = 0,

	NUM_CARD_READER_IMGS,
};

const static uint32_t bmp_ids[] = {
	PIC_BG_USB,
};

typedef struct card_reader_view_data {
	/* lvgl object */
	lv_obj_t *bmp[NUM_CARD_READER_IMGS];

	/* ui-editor resource */
	lvgl_res_scene_t res_scene;
	lv_image_dsc_t img_bmp[NUM_CARD_READER_IMGS];

	/* lvgl resource */
	lv_point_t pt_bmp[NUM_CARD_READER_IMGS];
	/* user data */
} card_reader_view_data_t;

static card_reader_view_data_t *p_card_reader_view_data = NULL;

static void _delete_obj_array(lv_obj_t **pobj, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		if (pobj[i]) {
			lv_obj_delete(pobj[i]);
			pobj[i] = NULL;
		}
	}
}

static void _bmp_event_handler(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_current_target(e);
	lv_event_code_t event = lv_event_get_code(e);

	if (event == LV_EVENT_CLICKED) {
		LOG_INF("%p even match\n", obj);
		if (p_card_reader_view_data && p_card_reader_view_data->bmp[BMP_USB_BG] == obj) {
			struct app_msg	msg = {0};
			msg.type = MSG_HOTPLUG_EVENT;
			msg.cmd = HOTPLUG_USB_DEVICE;
			msg.value = HOTPLUG_OUT;
			send_async_msg("main", &msg);
		}
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		LOG_INF("%p Toggled\n", obj);
	}
}

static void _create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

void card_reader_show_storage_working(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_set_flash(FLASH_ITEM(SLED_SD), 400, FLASH_FOREVER, NULL);
#endif
#ifdef CONFIG_LED_MANAGER
	led_manager_set_blink(0, 200, 100, 500, LED_START_STATE_ON, NULL);
	led_manager_set_blink(1, 200, 100, 500, LED_START_STATE_OFF, NULL);
#endif
}

void card_reader_show_card_plugin(void)
{
#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_display_icon(SLED_SD, true);
#endif
}

static int _load_resource(card_reader_view_data_t *data)
{
	int32_t ret;

	/* load scene */
	ret = lvgl_res_load_scene(SCENE_CARD_READER_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		LOG_ERR("SCENE_HEART_VIEW not found");
		return -ENOENT;
	}

	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, bmp_ids, data->img_bmp, data->pt_bmp, NUM_CARD_READER_IMGS);

	LOG_INF("load resource succeed");

	return 0;
}

static void _unload_resource(card_reader_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_bmp, NUM_CARD_READER_IMGS);
	lvgl_res_unload_scene(&data->res_scene);
}

void card_reader_view_init(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_create(CARD_READER_VIEW, NULL, UI_CREATE_FLAG_SHOW);
#endif
	SYS_LOG_INF("ok\n");
}

void card_reader_view_deinit(void)
{
#ifdef CONFIG_UI_MANAGER
	ui_view_delete(CARD_READER_VIEW);
#endif

	SYS_LOG_INF("ok\n");
}
static int _card_reader_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_CARD_READER_VIEW, NULL, 0,
				lvgl_res_scene_preload_default_cb_for_view, (void *)CARD_READER_VIEW,
				DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _card_reader_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	card_reader_view_data_t *data;

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}

	view_data->user_data = data;
	memset(data, 0, sizeof(*data));

	if (_load_resource(data)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}

	p_card_reader_view_data = data;

	/* create image */
	_create_img_array(scr, data->bmp, data->pt_bmp, data->img_bmp, NUM_CARD_READER_IMGS);
	lv_obj_add_flag(p_card_reader_view_data->bmp[BMP_USB_BG], LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(p_card_reader_view_data->bmp[BMP_USB_BG], _bmp_event_handler, LV_EVENT_ALL, NULL);

	SYS_LOG_INF("create\n");
	return 0;
}

static int _card_reader_view_paint(view_data_t *view_data)
{
	return 0;
}

static int _card_reader_view_delete(view_data_t *view_data)
{
	card_reader_view_data_t *data = view_data->user_data;

	if (data) {
		_delete_obj_array(data->bmp, NUM_CARD_READER_IMGS);
		_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
	} else {
		lvgl_res_preload_cancel_scene(SCENE_CARD_READER_VIEW);
	}

	p_card_reader_view_data = NULL;

	lvgl_res_unload_scene_compact(SCENE_CARD_READER_VIEW);
	lvgl_res_cache_clear(0);

	return 0;
}

static int _card_reader_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _card_reader_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _card_reader_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _card_reader_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _card_reader_view_paint(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(card_reader_view, _card_reader_view_handler, NULL, \
		NULL, CARD_READER_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

