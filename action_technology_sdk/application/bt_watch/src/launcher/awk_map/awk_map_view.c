/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <assert.h>
#include <app_ui.h>
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#include "dvfs.h"
#endif
#include <ui_mem.h>
#include <view_stack.h>
#include <view_manager.h>
#include <awk_service.h>
#include <awk_render_adapter.h>
#include <awk_system_adapter.h>
#include <memory/mem_cache.h>

#ifdef CONFIG_UI_MANAGER
#define UI_MAP_MALLOC(size)        ui_mem_aligned_alloc(MEM_FB, 64, size, __func__)
#define UI_MAP_FREE(ptr)           ui_mem_free(MEM_FB, ptr)
#define UI_MAP_REALLOC(ptr,size)   ui_mem_realloc(MEM_FB,ptr,size,__func__)
#else
#define UI_MAP_MALLOC(size)        lv_malloc(size)
#define UI_MAP_FREE(ptr)           lv_free(ptr)
#define UI_MAP_REALLOC(ptr,size)   lv_realloc(ptr,size)
#endif

enum {
	AWK_VIEW_MSG_START = MSG_VIEW_USER_OFFSET,
	AWK_INIT_RESULT,
	AWK_MAP_CREATE_RESULT,
	AWK_RENDER_COMMIT_DRAWING,
	AWK_VIEW_MSG_END,
};

//#define OVERLAY_TEST
#ifdef OVERLAY_TEST
#define TEST_TEXTURE_WIDTH  30
#define TEST_TEXTURE_HEIGHT 30
enum {
	TEXTURE_POINT_NORMAL,
	TEXTURE_POINT_FOCUS,
	TEXTURE_NUM,
};
#endif

typedef struct map_view_data {
	lv_obj_t *cont;
	lv_obj_t *map;
	lv_obj_t *map_bg;
	lv_image_dsc_t img_map;
	lv_image_dsc_t img_bg;
	lv_obj_t *zoom_in_btn;
	lv_obj_t *zoom_out_btn;
	lv_obj_t *test_btn;
	/* resource */
	lv_font_t font;
	lv_timer_t *timer;
	int map_id;
	t_awk_view_buffer_info render_buffer;
} map_view_data_t;
static map_view_data_t *map_data = NULL;

static void awk_srv_map_init_callback(t_awk_init_result result);
static void awk_map_create_view_callback(t_awk_srv_map_create_result result);

void awk_map_view_enter(void)
{
	view_stack_push_view(AWK_MAP_VIEW, NULL);
}

static void map_btn_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	map_view_data_t *data = lv_event_get_user_data(e);
	lv_indev_t *indev = lv_indev_get_act();
	lv_point_t point = {0};
	if (data->map_id <= 0 || data->map == NULL) {
		SYS_LOG_ERR("map not exist");
		return;
	}

	lv_indev_get_point(indev, &point);

	switch (code) {
		case LV_EVENT_PRESSED:
			awk_srv_map_touch_data_input(data->map_id, AWK_MAP_TOUCH_START, point.x, point.y);
			break;
		case LV_EVENT_PRESSING:
			awk_srv_map_touch_data_input(data->map_id, AWK_MAP_TOUCH_UPDATE, point.x, point.y);
			break;
		case LV_EVENT_RELEASED:
			awk_srv_map_touch_data_input(data->map_id, AWK_MAP_TOUCH_END, point.x, point.y);
			break;
		case LV_EVENT_SHORT_CLICKED:
			awk_srv_map_touch_data_input(data->map_id, AWK_MAP_TOUCH_CLICKED, point.x, point.y);
			break;
		default:
			break;
	}

}

#ifdef OVERLAY_TEST
static void map_point_test_process(void *user_data, uint32_t data_size)
{
	awk_map_point_overlay_t pointOverlay;
	// 调用初始化接口（填充部分默认值）
	awk_map_init_point_overlay(&pointOverlay);

	// 经纬度坐标，需要传入GCJ02坐标系的坐标
	pointOverlay.position.lon = 116.473362;
	pointOverlay.position.lat = 39.995273;

	// 添加这个覆盖物到SDK中进行绘制
	awk_map_add_overlay((uint32_t)map_data->map_id, (awk_map_base_overlay_t *)&pointOverlay);
	// 设置地图中心方便观察测试结果
	awk_map_set_center(map_data->map_id, pointOverlay.position);
}

void map_polyline_test_process(void *user_data, uint32_t data_size)
{
	awk_map_polyline_overlay_t lineOverlay;
	// 调用初始化接口（填充部分默认值）
	awk_map_init_line_overlay(&lineOverlay);

	// 线覆盖物的点序列，需要传入GCJ02坐标系的坐标
	lineOverlay.point_size = 5;
	lineOverlay.points = (awk_map_coord2d_t *)awk_mem_malloc_adapter(sizeof(awk_map_coord2d_t) * lineOverlay.point_size);
	lineOverlay.points[0] = (awk_map_coord2d_t){116.473362, 39.996273};
	lineOverlay.points[1] = (awk_map_coord2d_t){116.474362, 39.996273};
	lineOverlay.points[2] = (awk_map_coord2d_t){116.474362, 39.998273};
	lineOverlay.points[3] = (awk_map_coord2d_t){116.473362, 39.998273};
	lineOverlay.points[4] = (awk_map_coord2d_t){116.473362, 39.996273};
	// 线的属性
	lineOverlay.normal_marker.line_width = 5;               // 线宽，单位：像素
	lineOverlay.normal_marker.line_color = 0xff00caf8;      // 线的颜色，ARGB格式，alpha在第一个
	lineOverlay.normal_marker.border_width = 2;             // 边框宽度，单位：像素
	lineOverlay.normal_marker.border_color = 0xff047BC8;    // 边框颜色，ARGB格式，alpha在第一个

	// 画虚线
	lineOverlay.normal_marker.use_dash = true;              // 如果需要画虚线，use_dash需要设置为true，否则画的是实线
	lineOverlay.normal_marker.dash_offset = 0;              // 虚线开始绘制的偏移值，单位：像素
	lineOverlay.normal_marker.dash_painted_length = 5;      // 虚线实部的长度比例
	lineOverlay.normal_marker.dash_unpainted_length = 10;   // 虚线虚部的长度比例

	// 添加这个覆盖物到SDK中进行绘制
	awk_map_add_overlay((uint32_t)map_data->map_id,(awk_map_base_overlay_t *)&lineOverlay);
	// 设置地图中心方便观察测试结果
	awk_map_set_center(map_data->map_id, lineOverlay.points[0]);

	awk_mem_free_adapter(lineOverlay.points);
}

void map_polygon_test_process(void *user_data, uint32_t data_size)
{
	awk_map_polygon_overlay_t polygonOverlay;
	// 调用初始化接口（填充部分默认值）
	awk_map_init_polygon_overlay(&polygonOverlay);

	// 面覆盖物的点序列，需要传入GCJ02坐标系的坐标
	polygonOverlay.point_size = 3;
	polygonOverlay.points = (awk_map_coord2d_t *)awk_mem_malloc_adapter(sizeof(awk_map_coord2d_t) * polygonOverlay.point_size);
	polygonOverlay.points[0] = (awk_map_coord2d_t){116.476362, 39.996273};
	polygonOverlay.points[1] = (awk_map_coord2d_t){116.478362, 39.995273};
	polygonOverlay.points[2] = (awk_map_coord2d_t){116.478862, 39.996273};

	// 面的颜色，ARGB格式，alpha在第一个
	polygonOverlay.color = 0xffffffcd;

	// 添加这个覆盖物到SDK中进行绘制
	awk_map_add_overlay((uint32_t)map_data->map_id, (awk_map_base_overlay_t *)&polygonOverlay);
	// 设置地图中心方便观察测试结果
	awk_map_set_center(map_data->map_id, polygonOverlay.points[0]);
	awk_mem_free_adapter(polygonOverlay.points);
}

static void map_test_btn_cb(lv_event_t * e)
{
	awk_service_user_work(map_point_test_process, NULL, 0);
	awk_service_user_work(map_polyline_test_process, NULL, 0);
	awk_service_user_work(map_polygon_test_process, NULL, 0);
	return;
}
#endif

enum {
	AWK_MAP_ZOOM_NONE,
	AWK_MAP_ZOOM_IN,
	AWK_MAP_ZOOM_OUT,
};

void map_zoom_process(void *user_data, uint32_t data_size)
{
	uint8_t zoom_type = *((uint8_t *)user_data);

	if (map_data == NULL || user_data == NULL) {
		return;
	}
	awk_map_posture_t posture;
	awk_map_get_posture(map_data->map_id, &posture);

	if (zoom_type == AWK_MAP_ZOOM_IN) {
		if (posture.level > 15) {
			awk_map_set_level(map_data->map_id, posture.level - 1);
		}
	} else if (zoom_type == AWK_MAP_ZOOM_OUT) {
		if (posture.level < 16) {
			awk_map_set_level(map_data->map_id, posture.level + 1);
		}
	}
}

static void map_zoom_btn_cb(lv_event_t * e)
{
	map_view_data_t *data = lv_event_get_user_data(e);
	lv_obj_t *obj = lv_event_get_target(e);
	uint8_t zoom_type = AWK_MAP_ZOOM_NONE;

	if (obj == data->zoom_in_btn) {
		zoom_type = AWK_MAP_ZOOM_IN;
	} else if (obj == data->zoom_out_btn) {
		zoom_type = AWK_MAP_ZOOM_OUT;
	}

	awk_service_user_work(map_zoom_process, &zoom_type, sizeof(zoom_type));

	return;
}

//run in awk thread
static void awk_srv_map_init_callback(t_awk_init_result result)
{
	SYS_LOG_INF("awk_srv_map_init_cbk, ret:%d\n", result.ret);
	ui_message_send_async2(AWK_MAP_VIEW, AWK_INIT_RESULT, (uint32_t) result.ret, NULL);
}

//run in ui thread
void awk_srv_map_init_result_handler(struct view_data *view_data, void *user_data)
{
	int ret = (int) user_data;
	SYS_LOG_INF("awk_srv_map_init_result_handler, ret:%d", ret);
	if (ret == 0) {
		t_awk_srv_map_create_param param;
		memset(&param, 0, sizeof(t_awk_srv_map_create_param));
		param.view_param.port.height = DEF_UI_HEIGHT;
		param.view_param.port.width = DEF_UI_WIDTH;
		param.map_center.lat = 40.002713;
		param.map_center.lon = 116.489486;
		param.zoom = 16;
		awk_map_create_view_async(param, awk_map_create_view_callback);
	}
}

//run in awk thread
static void awk_map_create_view_callback(t_awk_srv_map_create_result result)
{
	ui_message_send_async2(AWK_MAP_VIEW, AWK_MAP_CREATE_RESULT, (uint32_t) result.map_id, NULL);
}

static void awk_render_commit_drawing_callback(struct view_data *view_data, void *user_data)
{
	map_view_data_t *data = view_data->user_data;
	if (data == NULL || data->map == NULL) {
		return;
	}
	if (data->map_bg) {
		lv_obj_remove_flag(data->map_bg, LV_OBJ_FLAG_HIDDEN);
		lv_obj_invalidate(data->map_bg);
	}

	lv_obj_remove_flag(data->map, LV_OBJ_FLAG_HIDDEN);
	lv_obj_invalidate(data->map);
	lv_refr_now(view_data->display);
}

static void user_render_commit_cbk(uint32_t map_id)
{
	ui_message_send_sync2(AWK_MAP_VIEW, AWK_RENDER_COMMIT_DRAWING, (uint32_t) map_id, NULL);
}

//run in ui thread
void awk_map_create_view_result_handler(struct view_data *view_data, void *user_data)
{
	map_view_data_t *data = view_data->user_data;
	int map_id = (int) user_data;
	SYS_LOG_INF("awk_map_create_view_result_handler, ret:%d", map_id);
	if (map_id > 0) {
		//map create successed, load resource
		data->map_id = map_id;
		lv_color_format_t cf = LV_COLOR_FORMAT_RGB565;
		int32_t width = DEF_UI_WIDTH;
		int32_t height = DEF_UI_HEIGHT;
		uint32_t stride = width * lv_color_format_get_size(cf);
		uint32_t buffer_size = stride * height;

		uint8_t *img_data_bg = (uint8_t *)UI_MAP_MALLOC(buffer_size);
		if(img_data_bg == NULL) {
			SYS_LOG_ERR("malloc %d failed", buffer_size);
			return;
		}
		memset(img_data_bg, 0, buffer_size);
		data->img_bg.header.magic = LV_IMAGE_HEADER_MAGIC;
		data->img_bg.header.cf = cf;
		data->img_bg.header.stride = stride;
		data->img_bg.header.w = width;
		data->img_bg.header.h = height;
		data->img_bg.data = img_data_bg;
		data->img_bg.data_size = buffer_size;
		lv_obj_t *map_bg = lv_image_create(data->cont);
		lv_image_set_src(map_bg, &data->img_bg);
		lv_obj_center(map_bg);
		data->map_bg = map_bg;

		uint8_t *img_data = (uint8_t *)UI_MAP_MALLOC(buffer_size);
		if(img_data == NULL) {
			SYS_LOG_ERR("malloc %d failed", buffer_size);
			return;
		}
		memset(img_data, 0, buffer_size);
		data->img_map.header.magic = LV_IMAGE_HEADER_MAGIC;
		data->img_map.header.cf = cf;
		data->img_map.header.stride = stride;
		data->img_map.header.w = width;
		data->img_map.header.h = height;
		data->img_map.data = img_data;
		data->img_map.data_size = buffer_size;
		lv_obj_t *map = lv_image_create(data->cont);
		lv_image_set_src(map, &data->img_map);
		lv_obj_center(map);
		data->map = map;

		lv_obj_add_flag(map_bg, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(map, LV_OBJ_FLAG_HIDDEN);
		lv_refr_now(view_data->display);

		t_awk_view_buffer_info render_buffer_info;
		render_buffer_info.cf = AWK_PIXEL_MODE_RGB_565;
		render_buffer_info.width = width;
		render_buffer_info.height = height;
		render_buffer_info.stride = stride;
		render_buffer_info.data = img_data;
		render_buffer_info.data_size = buffer_size;

		t_awk_view_buffer_info background_buffer_info;
		background_buffer_info.cf = AWK_PIXEL_MODE_RGB_565;
		background_buffer_info.width = width;
		background_buffer_info.height = height;
		background_buffer_info.stride = stride;
		background_buffer_info.data = img_data_bg;
		background_buffer_info.data_size = buffer_size;
		awk_render_init(&render_buffer_info, &background_buffer_info, user_render_commit_cbk);

		awk_service_render_start();
	}
}

static int _map_view_layout(view_data_t *view_data)
{
	map_view_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
		app_mem_free(data);
		return -EIO;
	}

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "map");
#endif
	printk("_map_view_layout\n");

	data->cont = lv_obj_create(lv_display_get_screen_active(view_data->display));
	lv_obj_set_size(data->cont, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
	lv_obj_set_style_bg_color(data->cont, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(data->cont, LV_OPA_100, 0);
	lv_obj_center(data->cont);
	lv_obj_add_flag(data->cont, LV_OBJ_FLAG_CLICKABLE);
	//lv_obj_add_event_cb(data->cont, map_btn_cb, LV_EVENT_PRESSED, data);
	//lv_obj_add_event_cb(data->cont, map_btn_cb, LV_EVENT_PRESSING, data);
	//lv_obj_add_event_cb(data->cont, map_btn_cb, LV_EVENT_RELEASED, data);
	lv_obj_add_event_cb(data->cont, map_btn_cb, LV_EVENT_ALL, data);

	data->zoom_in_btn = lv_btn_create(lv_display_get_screen_active(view_data->display));
	lv_obj_set_size(data->zoom_in_btn, 60, 60);
	lv_obj_set_style_bg_color(data->zoom_in_btn, lv_color_make(0xc0, 0xc0, 0xc0), 0);
	lv_obj_set_style_bg_opa(data->zoom_in_btn, LV_OPA_100, 0);
	lv_obj_set_style_radius(data->zoom_in_btn, 20, 0);
	lv_obj_align(data->zoom_in_btn, LV_ALIGN_BOTTOM_MID, -40, -30);
	lv_obj_add_event_cb(data->zoom_in_btn, map_zoom_btn_cb, LV_EVENT_CLICKED, data);
	lv_obj_t *zoom_in_label = lv_label_create(data->zoom_in_btn);
	lv_obj_set_style_text_font(zoom_in_label, &data->font, 0);
	lv_obj_set_style_text_color(zoom_in_label, lv_color_make(0xff, 0xff, 0xff), 0);
	lv_label_set_text(zoom_in_label, "-");
	lv_obj_center(zoom_in_label);

	data->zoom_out_btn = lv_btn_create(lv_disp_get_scr_act(view_data->display));
	lv_obj_set_size(data->zoom_out_btn, 60, 60);
	lv_obj_set_style_bg_color(data->zoom_out_btn, lv_color_make(0xc0, 0xc0, 0xc0), 0);
	lv_obj_set_style_bg_opa(data->zoom_out_btn, LV_OPA_100, 0);
	lv_obj_set_style_radius(data->zoom_out_btn, 20, 0);
	lv_obj_align(data->zoom_out_btn, LV_ALIGN_BOTTOM_MID, 40, -30);
	lv_obj_add_event_cb(data->zoom_out_btn, map_zoom_btn_cb, LV_EVENT_CLICKED, data);
	lv_obj_t *zoom_out_label = lv_label_create(data->zoom_out_btn);
	lv_obj_set_style_text_font(zoom_out_label, &data->font, 0);
	lv_obj_set_style_text_color(zoom_out_label, lv_color_make(0xff, 0xff, 0xff), 0);
	lv_label_set_text(zoom_out_label, "+");
	lv_obj_center(zoom_out_label);
#ifdef OVERLAY_TEST
	data->test_btn = lv_btn_create(lv_disp_get_scr_act(view_data->display));
	lv_obj_set_style_bg_color(data->test_btn, lv_color_white(), 0);
	lv_obj_set_style_bg_opa(data->test_btn, LV_OPA_100, 0);
	lv_obj_align(data->test_btn, LV_ALIGN_RIGHT_MID, -60, -30);
	lv_obj_add_event_cb(data->test_btn, map_test_btn_cb, LV_EVENT_CLICKED, data);
	lv_obj_t *test_btn_label = lv_label_create(data->test_btn);
	lv_label_set_text(test_btn_label, "test");
	lv_obj_set_style_text_font(test_btn_label, &data->font, 0);
	lv_obj_center(test_btn_label);
#endif

	lv_obj_t *init_label = lv_label_create(data->cont);
	lv_label_set_text(init_label, "地图初始化中...");
	lv_obj_set_style_text_font(init_label, &data->font, 0);
	lv_obj_set_style_text_color(init_label, lv_color_make(0xff, 0xff, 0xff), 0);
	lv_obj_center(init_label);

	view_data->user_data = data;
	map_data = data;

	awk_init_async(awk_srv_map_init_callback);

	return 0;
}

static int _map_view_delete(view_data_t *view_data)
{
	map_view_data_t *data = view_data->user_data;

	if (data) {
		if (data->timer) {
			lv_timer_del(data->timer);
		}
		awk_service_render_stop();
		awk_render_deinit();
		t_awk_map_destroy_param param;
		param.map_id = data->map_id;
		awk_map_destroy_view_async(param, NULL);
		awk_uninit_async(NULL);
		if (data->img_map.data) {
			UI_MAP_FREE((void *)data->img_map.data);
		}
		if(data->img_bg.data){
			UI_MAP_FREE((void *)data->img_bg.data);
		}
		LVGL_FONT_CLOSE(&data->font);
		lv_obj_delete(data->cont);
		app_mem_free(data);
		view_data->user_data = NULL;
		map_data = NULL;
	}
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "map");
#endif
	return 0;
}

static int awk_map_view_proc_key(view_data_t *view_data, ui_key_msg_data_t *key_data)
{
	switch (KEY_VALUE(key_data->event)) {
	case KEY_GESTURE_RIGHT:
		key_data->done = true;
		break;
	default:
		break;
	}

	return 0;
}

static int _awk_map_view_handler(uint16_t view_id, struct view_data *view_data, uint8_t msg_id, void *msg_data)
{
	view_user_msg_data_t *user_data = (view_user_msg_data_t *)msg_data;
	assert(view_id == AWK_MAP_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return ui_view_layout(view_id);
	case MSG_VIEW_LAYOUT:
		return _map_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _map_view_delete(view_data);
	case AWK_INIT_RESULT:
		awk_srv_map_init_result_handler(view_data, user_data->data);
		return 0;
	case AWK_MAP_CREATE_RESULT:
		awk_map_create_view_result_handler(view_data, user_data->data);
		return 0;
	case AWK_RENDER_COMMIT_DRAWING:
		awk_render_commit_drawing_callback(view_data, user_data->data);
		return 0;
	case MSG_VIEW_KEY:
		return awk_map_view_proc_key(view_data, msg_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(awk_map_view, _awk_map_view_handler, NULL, NULL, AWK_MAP_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
