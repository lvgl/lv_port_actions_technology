/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call view
 */

#include "ui_manager.h"
#include <input_manager.h>
#include <ui_manager.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include <lvgl/lvgl_res_loader.h>
#include "launcher_app.h"
#include "app_ui.h"
#include "widgets/img_number.h"

/* stopwatch main view */
#define SW_STATE_RESET	0
#define SW_STATE_START	1
#define SW_STATE_STOP	2
#define SW_UPDATE_PERIOD	10

enum {
	BMP_SW_BG = 0,
#if DEF_UI_WIDTH < 454
	BMP_SW_ICON,
	BMP_SW_NAME,
#endif
	BMP_SW_COLON,
	BMP_SW_DOT,

	NUM_SW_BG_IMGS,
};
enum {
	BTN_RESET = 0,
	BTN_START,

	NUM_SW_BTNS,
};

enum {
	BTN_RESET_BMP = 0,
	BTN_RESET_BMP_P,
	BTN_START_BMP,
	BTN_START_BMP_P,
	BTN_STOP_BMP,
	BTN_STOP_BMP_P,

	NUM_SW_BTN_BMPS,
};

const static uint32_t sw_bg_bmp_ids[] = {
	PIC_SW_BG,
#if DEF_UI_WIDTH < 454
	PIC_SW_ICON,
	PIC_SW_NAME,
#endif
	PIC_SW_COLON,
	PIC_SW_DOT,
};

const static uint32_t sw_btn_ids[] = {
	PIC_BTN_RESET,
	PIC_BTN_RESET_P,
	PIC_BTN_START,
	PIC_BTN_START_P,
	PIC_BTN_STOP,
	PIC_BTN_STOP_P,
};

static const uint32_t pic_sw_min_ids[] = {
	PIC_M_0, PIC_M_1, PIC_M_2, PIC_M_3, PIC_M_4, PIC_M_5, PIC_M_6, PIC_M_7, PIC_M_8, PIC_M_9,
};
static const uint32_t pic_sw_second_ids[] = {
	PIC_S_0, PIC_S_1, PIC_S_2, PIC_S_3, PIC_S_4, PIC_S_5, PIC_S_6, PIC_S_7, PIC_S_8, PIC_S_9,
};
static const uint32_t pic_sw_ms_ids[] = {
	PIC_MS_0, PIC_MS_1, PIC_MS_2, PIC_MS_3, PIC_MS_4, PIC_MS_5, PIC_MS_6, PIC_MS_7, PIC_MS_8, PIC_MS_9,
};

typedef struct sw_view_tmp_res {
	lvgl_res_group_t grp_sw_min;
	lvgl_res_group_t grp_sw_sec;
	lvgl_res_group_t grp_sw_ms;
} sw_view_tmp_res_t;

typedef struct sw_view_data {
	lv_obj_t *obj_sw_bg[NUM_SW_BG_IMGS];
	lv_obj_t *obj_sw_min;
	lv_obj_t *obj_sw_sec;
	lv_obj_t *obj_sw_ms;
	lv_obj_t *obj_sw_btns[NUM_SW_BTNS];

	/* lvgl resource */
	lv_image_dsc_t img_dsc_bg[NUM_SW_BG_IMGS];
	lv_image_dsc_t img_dsc_btn[NUM_SW_BTN_BMPS];
	lv_image_dsc_t img_dsc_min_num[10];
	lv_image_dsc_t img_dsc_sec_num[10];
	lv_image_dsc_t img_dsc_ms_num[10];

	lv_point_t pt_sw_bg[NUM_SW_BG_IMGS];
	lv_point_t pt_sw_btn[NUM_SW_BTN_BMPS];

	lvgl_res_scene_t res_scene;
	/* user data */
	uint32_t cur_times;/* unit:s */
	uint32_t sw_time;/* unit:s */
	uint16_t cur_ms;/* unit:ms */
	uint16_t sw_ms : 14;/* unit:ms */
	uint16_t state : 2;/* 0--reset,1--start,2--stop*/
	lv_timer_t *timer;
} sw_view_data_t;

typedef struct sw_bp_data {
	uint32_t cur_time;/* unit:s */
	uint32_t sw_bp_time;/* unit:s */
	uint16_t sw_bp_ms : 14;/* unit:ms */
	uint16_t state : 2;/* 0--reset,1--start,2--stop*/
	uint16_t cur_ms;/* unit:ms */
} sw_bp_data_t;
static sw_view_data_t *p_sw_data = NULL;
static sw_bp_data_t bp_data;

static void btn_state_toggle(lv_obj_t * btn)
{
    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
}

static void _stopwatch_delete_obj_array(lv_obj_t **pobj, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		if (pobj[i]) {
			lv_obj_delete(pobj[i]);
			pobj[i] = NULL;
		}
	}
}
static void _stopwatch_view_update(sw_view_data_t *data)
{
	if (data) {
		img_number_set_value(data->obj_sw_min, data->sw_time / 60, 2);
		img_number_set_value(data->obj_sw_sec, data->sw_time % 60, 2);
		img_number_set_value(data->obj_sw_ms, data->sw_ms / 10, 2);
	}
}
static void _stopwatch_get_cur_time(sw_view_data_t *data, uint32_t *times, uint16_t *cur_ms)
{
	launcher_app_t *app = launcher_app_get();

	if (!app || !data)
		return;

#ifdef CONFIG_RTC_ACTS
	struct rtc_time rtc_time;

	if (app->rtc_dev) {
		rtc_get_time(app->rtc_dev, &rtc_time);
		if (times)
			rtc_tm_to_time(&rtc_time, times);
		*cur_ms = rtc_time.tm_ms;
	}
#endif
}
static void _stopwatch_view_update_process_task_cb(lv_timer_t *task)
{
	uint16_t cur_ms = 0;
	if (!p_sw_data || p_sw_data->state != SW_STATE_START)
		return;

	_stopwatch_get_cur_time(p_sw_data, &p_sw_data->cur_times, &cur_ms);

	if (cur_ms < p_sw_data->cur_ms) {
		cur_ms += 1000;
	}
	p_sw_data->sw_ms += (cur_ms - p_sw_data->cur_ms);

	/* full 1000ms carry 1s */
	if (p_sw_data->sw_ms >= 1000) {
		p_sw_data->sw_time++;
		p_sw_data->sw_ms %= 1000;
	}
	p_sw_data->cur_ms = cur_ms % 1000;
#if 0
	/* calculate the milliseconds */
	p_sw_data->sw_ms += SW_UPDATE_PERIOD;
	if (p_sw_data->sw_ms >= 1000)
		p_sw_data->sw_ms = 0;
#endif
	_stopwatch_view_update(p_sw_data);
}
static void _stopwatch_store_bp_data(sw_view_data_t *data)
{
	if (!data)
		return;

	bp_data.cur_time = data->cur_times;
	bp_data.cur_ms = data->cur_ms;
	bp_data.state = data->state;
	bp_data.sw_bp_time = data->sw_time;
	bp_data.sw_bp_ms = data->sw_ms;
}
static void _stopwatch_restore_bp_view(sw_view_data_t *data)
{
	lv_state_t btn_state;
	uint32_t cur_times = 0;
	uint16_t cur_ms = 0;

	if (!data)
		return;
	if (bp_data.state == SW_STATE_STOP || bp_data.state == SW_STATE_START) {
		data->timer = lv_timer_create(_stopwatch_view_update_process_task_cb, SW_UPDATE_PERIOD, data);
		if (data->timer)
			lv_timer_ready(data->timer);
	}
	/*restore btn state*/
	if (bp_data.state == SW_STATE_START) {
		btn_state = lv_obj_get_state(p_sw_data->obj_sw_btns[BTN_START]);
		SYS_LOG_DBG("btn_state %d\n", btn_state);
		btn_state_toggle(p_sw_data->obj_sw_btns[BTN_START]);
		lv_obj_invalidate(p_sw_data->obj_sw_btns[BTN_START]);

		_stopwatch_get_cur_time(data, &data->cur_times, &data->cur_ms);
		cur_times = data->cur_times;
		cur_ms = data->cur_ms;
		if (cur_ms < bp_data.cur_ms) {/*make sure cur ms >= bp ms*/
			cur_times--;
			cur_ms += 1000;
		}
		if (cur_times >= bp_data.cur_time) {
			bp_data.sw_bp_time += (cur_times - bp_data.cur_time);
			bp_data.sw_bp_ms += (cur_ms - bp_data.cur_ms);
			if (bp_data.sw_bp_ms >= 1000)
				bp_data.sw_bp_time++;
		} else {
			SYS_LOG_WRN("tims error bp_times %d cur_times %d\n", bp_data.cur_time, cur_times);
		}
	}
	data->state = bp_data.state;
	data->sw_time = bp_data.sw_bp_time;
	data->sw_ms = bp_data.sw_bp_ms;
	_stopwatch_view_update(data);
}
static void _stopwatch_btn_evt_handler(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_current_target(e);
	lv_state_t btn_state;

	if (!p_sw_data)
		return;

	if (event == LV_EVENT_CLICKED) {
		if (p_sw_data->obj_sw_btns[BTN_RESET] == obj && p_sw_data->state == SW_STATE_STOP) {
			p_sw_data->sw_time = 0;
			p_sw_data->sw_ms = 0;
			/*reset pp data*/
			memset(&bp_data, 0, sizeof(sw_bp_data_t));
			if (p_sw_data->timer) {
				p_sw_data->state = SW_STATE_RESET;
				lv_timer_delete(p_sw_data->timer);
				p_sw_data->timer = NULL;
				_stopwatch_view_update(p_sw_data);
			}

			btn_state = lv_obj_get_state(p_sw_data->obj_sw_btns[BTN_START]);
			if (btn_state == LV_STATE_PRESSED || ((btn_state & (LV_STATE_CHECKED | LV_STATE_PRESSED)) == LV_STATE_CHECKED)) {
				btn_state_toggle(p_sw_data->obj_sw_btns[BTN_START]);
				lv_obj_invalidate(p_sw_data->obj_sw_btns[BTN_START]);
			}

		} else if (p_sw_data->obj_sw_btns[BTN_START] == obj) {
			btn_state = lv_obj_get_state(p_sw_data->obj_sw_btns[BTN_START]);
			if (p_sw_data->state == SW_STATE_RESET) {
				p_sw_data->state = SW_STATE_START;
				_stopwatch_get_cur_time(p_sw_data, &p_sw_data->cur_times, &p_sw_data->cur_ms);
				p_sw_data->timer = lv_timer_create(_stopwatch_view_update_process_task_cb, SW_UPDATE_PERIOD, p_sw_data);
				if (p_sw_data->timer)
					lv_timer_ready(p_sw_data->timer);
			} else if (p_sw_data->state == SW_STATE_START) {
				_stopwatch_view_update_process_task_cb(p_sw_data->timer);
				p_sw_data->state = SW_STATE_STOP;
			} else if (p_sw_data->state == SW_STATE_STOP) {
				p_sw_data->state = SW_STATE_START;
				_stopwatch_get_cur_time(p_sw_data, &p_sw_data->cur_times, &p_sw_data->cur_ms);
			}
		}
		SYS_LOG_INF("Clicked\n");
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		SYS_LOG_INF("Toggled\n");
	}
}

static void _stopwatch_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}
static void _stopwatch_unload_resource(sw_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_bg, NUM_SW_BG_IMGS);
	lvgl_res_unload_pictures(data->img_dsc_btn, NUM_SW_BTN_BMPS);
	lvgl_res_unload_pictures(data->img_dsc_min_num, ARRAY_SIZE(data->img_dsc_min_num));
	lvgl_res_unload_pictures(data->img_dsc_sec_num, ARRAY_SIZE(data->img_dsc_sec_num));
	lvgl_res_unload_pictures(data->img_dsc_ms_num, ARRAY_SIZE(data->img_dsc_ms_num));
	lvgl_res_unload_scene(&data->res_scene);
	if (data->timer) {
		lv_timer_delete(data->timer);
		data->timer = NULL;
	}
}

static int _stopwatch_load_resource(sw_view_data_t *data, sw_view_tmp_res_t *tmp_res)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_STOPWATCH_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_STOPWATCH_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, sw_bg_bmp_ids, data->img_dsc_bg, data->pt_sw_bg, NUM_SW_BG_IMGS);
	if (ret < 0) {
		goto out_exit;
	}
	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, sw_btn_ids, data->img_dsc_btn, data->pt_sw_btn, NUM_SW_BTN_BMPS);
	if (ret < 0) {
		goto out_exit;
	}

	/* sw minute */
	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_STOPWATCH_MINUTE, &tmp_res->grp_sw_min);
	if (ret < 0) {
		goto out_exit;
	}
	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_sw_min, pic_sw_min_ids,
			data->img_dsc_min_num, NULL, ARRAY_SIZE(pic_sw_min_ids));

	lvgl_res_unload_group(&tmp_res->grp_sw_min);

	/* sw second */
	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_STOPWATCH_SECOND, &tmp_res->grp_sw_sec);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_sw_sec, pic_sw_second_ids,
			data->img_dsc_sec_num, NULL, ARRAY_SIZE(pic_sw_second_ids));

	lvgl_res_unload_group(&tmp_res->grp_sw_sec);

	/* sw milli second */
	ret = lvgl_res_load_group_from_scene(&data->res_scene, RES_STOPWATCH_MS, &tmp_res->grp_sw_ms);
	if (ret < 0) {
		goto out_exit;
	}

	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_sw_ms, pic_sw_ms_ids,
			data->img_dsc_ms_num, NULL, ARRAY_SIZE(pic_sw_ms_ids));

	lvgl_res_unload_group(&tmp_res->grp_sw_ms);

out_exit:
	if (ret < 0) {
		_stopwatch_unload_resource(data);
	}

	return ret;
}

static int _stopwatch_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_STOPWATCH_VIEW, NULL, 0, lvgl_res_scene_preload_default_cb_for_view, (void *)STOPWATCH_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _stopwatch_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sw_view_data_t *data = NULL;
	sw_view_tmp_res_t resource;

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}

	memset(data, 0, sizeof(*data));

	if (_stopwatch_load_resource(data, &resource)) {
		app_mem_free(data);
		return -ENOENT;
	}
	/* create sw bg image */
	_stopwatch_create_img_array(scr, data->obj_sw_bg, data->pt_sw_bg, data->img_dsc_bg, NUM_SW_BG_IMGS);

	/* create reset button */
	data->obj_sw_btns[BTN_RESET] = lv_imagebutton_create(data->obj_sw_bg[BMP_SW_BG]);
	lv_obj_set_pos(data->obj_sw_btns[BTN_RESET], data->pt_sw_btn[BTN_RESET_BMP].x, data->pt_sw_btn[BTN_RESET_BMP].y);
	lv_obj_set_size(data->obj_sw_btns[BTN_RESET], data->img_dsc_btn[BTN_RESET].header.w, data->img_dsc_btn[BTN_RESET].header.h);
	lv_obj_add_flag(data->obj_sw_btns[BTN_RESET], LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_event_cb(data->obj_sw_btns[BTN_RESET], _stopwatch_btn_evt_handler, LV_EVENT_ALL, NULL);

	lv_imagebutton_set_src(data->obj_sw_btns[BTN_RESET], LV_IMAGEBUTTON_STATE_RELEASED, NULL, &data->img_dsc_btn[BTN_RESET_BMP], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_RESET], LV_IMAGEBUTTON_STATE_PRESSED, NULL, &data->img_dsc_btn[BTN_RESET_BMP_P], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_RESET], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &data->img_dsc_btn[BTN_RESET_BMP], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_RESET], LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &data->img_dsc_btn[BTN_RESET_BMP_P], NULL);

	/* create start button */
	data->obj_sw_btns[BTN_START] = lv_imagebutton_create(data->obj_sw_bg[BMP_SW_BG]);
	lv_obj_set_pos(data->obj_sw_btns[BTN_START], data->pt_sw_btn[BTN_START_BMP].x, data->pt_sw_btn[BTN_START_BMP].y);
	lv_obj_set_size(data->obj_sw_btns[BTN_START], data->img_dsc_btn[BTN_START].header.w, data->img_dsc_btn[BTN_START].header.h);
	lv_obj_add_flag(data->obj_sw_btns[BTN_START], LV_OBJ_FLAG_CHECKABLE);
	lv_obj_add_event_cb(data->obj_sw_btns[BTN_START], _stopwatch_btn_evt_handler, LV_EVENT_ALL, NULL);

	lv_imagebutton_set_src(data->obj_sw_btns[BTN_START], LV_IMAGEBUTTON_STATE_RELEASED, NULL, &data->img_dsc_btn[BTN_START_BMP], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_START], LV_IMAGEBUTTON_STATE_PRESSED, NULL, &data->img_dsc_btn[BTN_START_BMP_P], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_START], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &data->img_dsc_btn[BTN_STOP_BMP], NULL);
	lv_imagebutton_set_src(data->obj_sw_btns[BTN_START], LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &data->img_dsc_btn[BTN_STOP_BMP_P], NULL);

	data->obj_sw_min = img_number_create(data->obj_sw_bg[BMP_SW_BG]);
	data->obj_sw_sec = img_number_create(data->obj_sw_bg[BMP_SW_BG]);
	data->obj_sw_ms = img_number_create(data->obj_sw_bg[BMP_SW_BG]);
	if (!data->obj_sw_min || !data->obj_sw_sec || !data->obj_sw_ms) {
		goto fail_exit;
	}
	lv_obj_set_pos(data->obj_sw_min, resource.grp_sw_min.x, resource.grp_sw_min.y);
	lv_obj_set_size(data->obj_sw_min, resource.grp_sw_min.width, resource.grp_sw_min.height);
	img_number_set_src(data->obj_sw_min, data->img_dsc_min_num, 10);
	img_number_set_align(data->obj_sw_min, LV_ALIGN_RIGHT_MID);

	lv_obj_set_pos(data->obj_sw_sec, resource.grp_sw_sec.x, resource.grp_sw_sec.y);
	lv_obj_set_size(data->obj_sw_sec, resource.grp_sw_sec.width, resource.grp_sw_sec.height);
	img_number_set_src(data->obj_sw_sec, data->img_dsc_sec_num, 10);
	img_number_set_align(data->obj_sw_sec, LV_ALIGN_LEFT_MID);

	lv_obj_set_pos(data->obj_sw_ms, resource.grp_sw_ms.x, resource.grp_sw_ms.y);
	lv_obj_set_size(data->obj_sw_ms, resource.grp_sw_ms.width, resource.grp_sw_ms.height);
	img_number_set_src(data->obj_sw_ms, data->img_dsc_ms_num, 10);
	img_number_set_align(data->obj_sw_ms, LV_ALIGN_LEFT_MID);

	p_sw_data = data;
	view_data->user_data = data;

	_stopwatch_restore_bp_view(data);

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif
	return 0;
fail_exit:
	_stopwatch_unload_resource(data);
	app_mem_free(data);
	return -ENOMEM;
}

static int _stopwatch_view_paint(view_data_t *view_data)
{
	return 0;
}

static int _stopwatch_view_delete(view_data_t *view_data)
{
	sw_view_data_t *data = view_data->user_data;

	if (data) {
		_stopwatch_store_bp_data(data);
		lv_obj_delete(data->obj_sw_min);
		lv_obj_delete(data->obj_sw_sec);
		lv_obj_delete(data->obj_sw_ms);
		_stopwatch_delete_obj_array(data->obj_sw_btns, NUM_SW_BTNS);
		_stopwatch_delete_obj_array(data->obj_sw_bg, NUM_SW_BG_IMGS);

		_stopwatch_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif

		SYS_LOG_INF("ok\n");
	} else {
		lvgl_res_preload_cancel_scene(SCENE_STOPWATCH_VIEW);
	}

	p_sw_data = NULL;

	lvgl_res_unload_scene_compact(SCENE_STOPWATCH_VIEW);
	return 0;
}

int _stopwatch_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _stopwatch_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _stopwatch_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _stopwatch_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _stopwatch_view_paint(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(stopwatch_view, _stopwatch_view_handler, NULL, \
		NULL, STOPWATCH_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);


