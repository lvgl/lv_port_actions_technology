/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <sdfs.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include <widgets/draw_util.h>
#include "launcher_ui.h"

/*********************
 *      DEFINES
 *********************/
#define USE_AUTO_ROTATE  0
#define USE_UPDATE_TIME  1

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
	lv_obj_t *cube;
	lv_img_dsc_t cube_imgs[6];

#if USE_AUTO_ROTATE
	lv_timer_t * timer_rot;
#endif

#if USE_UPDATE_TIME
	lv_timer_t * timer_1s;
	lv_img_dsc_t front_img_ori;
	lv_font_t font;
	lv_area_t tm_area;

	/* time */
	uint8_t tm_hour;
	uint8_t tm_min;
	uint8_t tm_sec;
#endif

	/* measure in 0.1 degree */
	int16_t angle_x;
	int16_t angle_y;
	int16_t angle_z;

	bool pressed;
} cube_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static int _cube_scene_load_resource(cube_scene_data_t *data)
{
	lvgl_res_scene_t res_scene;
	int ret;

	ret = lvgl_res_load_scene(SCENE_CUBE_ROTATE, &res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", SCENE_CUBE_ROTATE);
		return -ENOENT;
	}

	lvgl_res_picregion_t picreg;
	ret = lvgl_res_load_picregion_from_scene(&res_scene, PIC_CUBE, &picreg);
	if (ret || picreg.frames < ARRAY_SIZE(data->cube_imgs)) {
		SYS_LOG_ERR("cannot find picreg 0x%x\n", PIC_CUBE);
		ret = -ENOENT;
		goto fail_exit;
	}

	ret = lvgl_res_load_pictures_from_picregion(&picreg, 0, ARRAY_SIZE(data->cube_imgs) - 1, data->cube_imgs);
	lvgl_res_unload_picregion(&picreg);

	if (ret) {
		SYS_LOG_ERR("cannot load picreg 0x%x pictures\n", PIC_CUBE);
		goto fail_exit;
	}

fail_exit:
	lvgl_res_unload_scene(&res_scene);
	return ret;
}

#if USE_AUTO_ROTATE
static void _timer_rotate_cb(lv_timer_t *timer)
{
	cube_scene_data_t * data = timer->user_data;

	data->angle_y += 150;
	if (data->angle_y >= 3600) data->angle_y -= 3600;

	cube_map_set_angle(data->cube, data->angle_x, data->angle_y, data->angle_z);
}
#endif /* USE_AUTO_ROTATE */

#if USE_UPDATE_TIME
static void _compute_text_area(cube_scene_data_t * data)
{
	lv_point_t size_res = { 0, 0};
	lv_txt_get_size(&size_res, "00:00:00", &data->font, 0, 0, 0, LV_TEXT_FLAG_EXPAND);

	data->tm_area.x1 = ((data->front_img_ori.header.w - size_res.x) / 2) & ~0x1;
	data->tm_area.x2 = data->tm_area.x1 + size_res.x - 1;
	data->tm_area.y2 = data->front_img_ori.header.h - 10;
	data->tm_area.y1 = data->tm_area.y2 - size_res.y + 1;
}

static void _update_time(cube_scene_data_t * data)
{
	if (++data->tm_sec < 60)
		return;

	data->tm_sec = 0;
	if (++data->tm_min < 60)
		return;

	data->tm_min = 0;
	if (++data->tm_hour < 24)
		return;

	data->tm_hour = 0;
}

static void _show_time_cb(lv_timer_t *timer)
{
	cube_scene_data_t * data = timer->user_data;
	const lv_img_dsc_t *img_dsc = cube_map_get_src(data->cube, CUBE_MAP_FRONT);

	lv_area_t buf_area;
	lv_area_set(&buf_area, 0, 0, img_dsc->header.w - 1, img_dsc->header.h - 1);

	/*Create a dummy display to fool the lv_draw function.
	 *It will think it draws to real screen.*/
	lv_disp_drv_t driver;
	lv_disp_drv_init(&driver);
	driver.hor_res = img_dsc->header.w;
	driver.ver_res = img_dsc->header.h;
	driver.draw_ctx = lv_mem_alloc(driver.draw_ctx_size);
	if (driver.draw_ctx == NULL)
		return;

	driver.draw_ctx_init(&driver, driver.draw_ctx);
	driver.draw_ctx->clip_area = &buf_area;
	driver.draw_ctx->buf_area = &buf_area;
	driver.draw_ctx->buf = (void *)img_dsc->data;

	lv_disp_t fake_disp;
	memset(&fake_disp, 0, sizeof(lv_disp_t));
	fake_disp.driver = &driver;

	/* fake refresh disp */
	lv_disp_t * refr_ori = _lv_refr_get_disp_refreshing();
	_lv_refr_set_disp_refreshing(&fake_disp);

	/* Start draw */
	/* Step 1: copy image data */
	const lv_color_t * src = (lv_color_t *)data->front_img_ori.data +
			data->tm_area.y1 * img_dsc->header.w + data->tm_area.x1;
	lv_color_t * dst_ori = (lv_color_t *)img_dsc->data +
			data->tm_area.y1 * img_dsc->header.w + data->tm_area.x1;
	lv_color_t * dst = dst_ori;
	int copy_len = lv_area_get_width(&data->tm_area) * sizeof(lv_color_t);
	for (int i = lv_area_get_height(&data->tm_area); i > 0; i--) {
		memcpy(dst, src, copy_len);
		src += img_dsc->header.w;
		dst += img_dsc->header.w;
	}

	/* Step 2: draw text */
	char buffer[16];
	_update_time(data);
	snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", data->tm_hour, data->tm_min, data->tm_sec);

	lv_draw_label_dsc_t label_dsc;
	lv_draw_label_dsc_init(&label_dsc);
	label_dsc.color = lv_color_black();
	label_dsc.font = &data->font;
	lvgl_draw_label_with_emoji(driver.draw_ctx, &label_dsc, &data->tm_area, buffer, NULL, NULL, NULL);

	/* restore refresh disp */
	_lv_refr_set_disp_refreshing(refr_ori);

	/* deinit draw context*/
	driver.draw_ctx_deinit(&driver, driver.draw_ctx);
	lv_mem_free(driver.draw_ctx);

	/* clean dcache */
	mem_dcache_clean(dst_ori, lv_area_get_height(&data->tm_area) * img_dsc->header.w * sizeof(lv_color_t));

	lv_obj_invalidate(data->cube);
}
#endif /* USE_UPDATE_TIME */

static void _cube_touch_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t * indev = lv_event_get_param(e);
	cube_scene_data_t * data = lv_event_get_user_data(e);

	if (code == LV_EVENT_PRESSING) {
		lv_point_t vect = { 0, 0 };
		lv_indev_get_vect(indev, &vect);

		data->pressed = true;
		data->angle_x += vect.y * 900 / lv_obj_get_height(data->cube);
		data->angle_y += -vect.x * 900 / lv_obj_get_width(data->cube);

		if (data->angle_x < 0)     data->angle_x += 3600;
		if (data->angle_x >= 3600) data->angle_x -= 3600;
		if (data->angle_y < 0)     data->angle_y += 3600;
		if (data->angle_y >= 3600) data->angle_y -= 3600;

		SYS_LOG_INF("angle: %d-%d-%d", data->angle_x, data->angle_y, data->angle_z);
		cube_map_set_angle(data->cube, data->angle_x, data->angle_y, data->angle_z);
	}

	if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
		data->pressed = false;
	}
}

static int _cube_scene_create(lv_obj_t * scr, const void * presenter)
{
	cube_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));
	data->angle_x = 200;
	data->angle_y = 200;
	data->angle_z = 200;

	if (_cube_scene_load_resource(data)) {
		SYS_LOG_ERR("load res failed");
		app_mem_free(data);
		return -ENOENT;
	}

#if USE_UPDATE_TIME
	/* Only support LV_IMG_CF_TRUE_COLOR */
	if (data->cube_imgs[CUBE_MAP_FRONT].header.cf == LV_IMG_CF_TRUE_COLOR) {
		memcpy(&data->front_img_ori, &data->cube_imgs[CUBE_MAP_FRONT], sizeof(lv_img_dsc_t));
		data->cube_imgs[CUBE_MAP_FRONT].data = ui_mem_aligned_alloc(
				MEM_RES, 64, data->front_img_ori.data_size, __func__);
		if (data->cube_imgs[CUBE_MAP_FRONT].data == NULL) {
			SYS_LOG_ERR("res mem alloc failed");
			data->cube_imgs[CUBE_MAP_FRONT].data = data->front_img_ori.data;
			data->front_img_ori.data = NULL;
		}
	}

	if (data->front_img_ori.data) {
		if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
			SYS_LOG_ERR("font open failed");
			ui_mem_free(MEM_RES, (void *)data->cube_imgs[CUBE_MAP_FRONT].data);
			app_mem_free(data);
			return -ENOMEM;
		}

		_compute_text_area(data);

		memcpy((void *)data->cube_imgs[CUBE_MAP_FRONT].data, data->front_img_ori.data,
				data->front_img_ori.data_size);
		mem_dcache_clean(data->cube_imgs[CUBE_MAP_FRONT].data, data->cube_imgs[CUBE_MAP_FRONT].data_size);

		data->timer_1s = lv_timer_create(_show_time_cb, 1000, data);
		if (data->timer_1s)
			lv_timer_ready(data->timer_1s);
	}
#endif /* USE_UPDATE_TIME */

	data->cube = cube_map_create(scr);
	cube_map_set_src(data->cube, data->cube_imgs);
	cube_map_set_angle(data->cube, data->angle_x, data->angle_y, data->angle_z);
	cube_map_set_zoom(data->cube, LV_IMG_ZOOM_NONE);
	lv_obj_center(data->cube);
	lv_obj_add_flag(data->cube, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(data->cube, _cube_touch_event_cb, LV_EVENT_ALL, data);

#if USE_AUTO_ROTATE
	data->timer_rot = lv_timer_create(_timer_rotate_cb, 10, data);
	if (data->timer_rot)
		lv_timer_ready(data->timer_rot);
#endif /* USE_AUTO_ROTATE */

	lv_obj_set_user_data(scr, data);
	return 0;
}

static int _cube_scene_delete(lv_obj_t * scr)
{
	cube_scene_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

#if USE_AUTO_ROTATE
	if (data->timer_rot)
		lv_timer_del(data->timer_rot);
#endif /* USE_AUTO_ROTATE */

#if USE_UPDATE_TIME
	if (data->timer_1s)
		lv_timer_del(data->timer_1s);

	if (data->front_img_ori.data) {
		ui_mem_free(MEM_RES, (void *)data->cube_imgs[CUBE_MAP_FRONT].data);
		data->cube_imgs[CUBE_MAP_FRONT].data = data->front_img_ori.data;
		LVGL_FONT_CLOSE(&data->font);
	}
#endif /* USE_UPDATE_TIME */

	lvgl_res_unload_pictures(data->cube_imgs, ARRAY_SIZE(data->cube_imgs));

	app_mem_free(data);
	return 0;
}

static int _cube_scene_proc_key(lv_obj_t * scr, ui_key_msg_data_t * key_data)
{
	cube_scene_data_t * data = lv_obj_get_user_data(scr);
	if (data == NULL)
		return 0;

	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT)
		key_data->done = data->pressed; /* key done, and stop next processing */

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int cube_scene_proc(void * scr, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_CREATE:
		return _cube_scene_create(scr, msg_data);
	case MSG_VIEW_DELETE:
		return _cube_scene_delete(scr);
	case MSG_VIEW_KEY:
		return _cube_scene_proc_key(scr, msg_data);
	default:
		return 0;
	}
}
