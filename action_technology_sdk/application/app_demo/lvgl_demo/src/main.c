/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <board_cfg.h>
#include <os_common_api.h>
#include <lvgl/lvgl.h>
#include <lvgl/lvgl_ext.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl/lvgl_freetype_font.h>
#include <drivers/input/input_dev.h>
#include <mem_manager.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <thread_timer.h>
//#include <thread_wdg.h>
#include <sys_wakelock.h>
#include <sensor_manager.h>
#include <partition/partition.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include "app_msg.h"
#include "ota_app.h"

/*********************
 *      DEFINES
 *********************/

// NOR:A ~ NOR:C --> file_id: 10 ~ 12 (firmware.xml)
#define DEF_FONT22_FILE			"/NOR:C/sans22.font"
#define DEF_VFONT_FILE      	"/NOR:C/vfont.ttf"

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _disp_vsync_cb(const lvx_display_observer_t * observer, uint32_t timestamp);
static void _disp_state_cb(const lvx_display_observer_t * observer, uint32_t state);

/**********************
 *  STATIC VARIABLES
 **********************/

static const lvx_display_observer_t g_disp_observer = {
	.vsync_cb = _disp_vsync_cb,
	.state_cb = _disp_state_cb,
};

static uint8_t g_disp_active = 1;

static K_SEM_DEFINE(g_disp_vsync_sem, 0, 1);

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void _disp_vsync_cb(const lvx_display_observer_t *observer, uint32_t timestamp)
{
	k_sem_give(&g_disp_vsync_sem);
}

static void _disp_state_cb(const lvx_display_observer_t *observer, uint32_t state)
{
	switch (state) {
	case LVX_DISPLAY_STATE_OFF:
		app_msg_send(APP_NAME, MSG_UI, CMD_SCREEN_OFF);
		break;
	case LVX_DISPLAY_STATE_ON:
		app_msg_send(APP_NAME, MSG_UI, CMD_SCREEN_ON);
		break;
	default:
		break;
	}
}

static void _keypad_callback(struct device *dev, struct input_value *val)
{
	static uint32_t last_value = 0;

	SYS_LOG_INF("type: %d, code:%d, value:%d\n",
		val->keypad.type, val->keypad.code, val->keypad.value);

	if (last_value != val->keypad.value) {
		// screen on
		sys_wake_lock(FULL_WAKE_LOCK);
		sys_wake_unlock(FULL_WAKE_LOCK);

		last_value = val->keypad.value;
	}
}

static void _keypad_init(void)
{
	struct device *onoff_dev;

	onoff_dev = (struct device *)device_get_binding(CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME);
	if (!onoff_dev) {
		SYS_LOG_ERR("cannot found key dev %s", CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME);
		return;
	}

	input_dev_enable(onoff_dev);
	input_dev_register_notify(onoff_dev, _keypad_callback);
}

static void _lvgl_init(void)
{
	/* LVGL initialize */
	lvx_port_init();
	lvx_display_add_observer(&g_disp_observer);
	lvx_refr_set_manual(NULL);

	/* Set background color */
	lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0);
	lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

	// init font
	lvgl_bitmap_font_init(NULL);
	lvgl_freetype_font_init();
}

static void _lvgl_set_y_anim_cb(void *label, int32_t val)
{
	static const lv_color_t test_colors[] = {
		LV_COLOR_MAKE(255, 50, 0), LV_COLOR_MAKE(0, 255, 0),
		LV_COLOR_MAKE(0, 100, 255), LV_COLOR_MAKE(255, 255, 255),
	};
	static uint8_t color_idx = 0;

	if (val == 0) {
		lv_obj_set_style_text_color(label, test_colors[color_idx], LV_PART_MAIN);
		if (++color_idx >= ARRAY_SIZE(test_colors))
			color_idx = 0;
	}

    lv_obj_set_y(label, val);
}

static void _lvgl_anim_demo(const char* text)
{
	const lv_font_t *font = lv_font_default();

#if 0
	static lv_font_t ttf_font;
	static lv_font_t bmp_font;

	/* open ttf font */
	if (lvgl_freetype_font_open(&ttf_font, DEF_VFONT_FILE, 24)) {
		SYS_LOG_ERR("ttf font not found");
	} else {
		font = &ttf_font;
	}

	/* open bitmap font */
	if (lvgl_bitmap_font_open(&bmp_font, DEF_FONT22_FILE) < 0) {
		SYS_LOG_ERR("bitmap font not found");
	} else {
		font = &bmp_font;
	}
#endif

	lv_obj_t * label = lv_label_create(lv_screen_active());
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
	lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
	lv_label_set_text(label, text);

	lv_anim_t anim;
	lv_anim_init(&anim);
	lv_anim_set_time(&anim, 2000);
	lv_anim_set_playback_time(&anim, 2000);
	lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_var(&anim, label);
	lv_anim_set_exec_cb(&anim, _lvgl_set_y_anim_cb);
	lv_anim_set_values(&anim, 0, lv_obj_get_height(lv_screen_active()) - lv_font_get_line_height(font));
	lv_anim_start(&anim);
}

static int _sensor_callback(int evt_id, sensor_res_t *res)
{
	SYS_LOG_INF("evt_id=%d, handup=%d, step=%d", evt_id, res->handup,
		(uint32_t)res->pedometer.total_steps);

	return 0;
}

static void _sensor_init(void)
{
	sensor_manager_enable(ALGO_ACTIVITY_OUTPUT, 0);
	sensor_manager_enable(ALGO_HANDUP, 0);
	sensor_manager_add_callback(_sensor_callback);
}

static void _process_ui_msg(struct app_msg *msg)
{
	switch (msg->cmd) {
	case CMD_SCREEN_OFF:
		g_disp_active = 0;
		break;
	case CMD_SCREEN_ON:
		g_disp_active = 1;
		lv_obj_invalidate(lv_screen_active());
		break;
	default:
		break;
	}
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int main(void)
{
	// init system
	mem_manager_init();
	system_init();
	bt_manager_init();

	// init app msg
	app_msg_init();

	// init lvgl
	_lvgl_init();

	// ota check and show animation
	// if (ota_is_already_running()) {
	// 	SYS_LOG_ERR("Error: ota running!");
	// } else if (ota_is_already_done()) {
	// 	_lvgl_anim_demo("Ota done!");
	// } else {
		_lvgl_anim_demo("Hello world!");
//	}

	// init key device
	_keypad_init();

	// init sensor
	_sensor_init();

	// init ota
	ota_app_init();
	ota_app_init_bluetooth();

	system_ready();

	// start soft watchdog
	//thread_wdg_start();

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "init");
	dvfs_set_level(DVFS_LEVEL_NORMAL, "launcher");
#endif

	while (1) {
		struct app_msg msg;

		os_strace_void(SYS_TRACE_ID_GUI_TASK);
		uint32_t lv_timeout = lv_task_handler();
		os_strace_end_call(SYS_TRACE_ID_GUI_TASK);

		uint32_t tt_timeout = thread_timer_next_timeout();
		uint32_t timeout = MIN(lv_timeout, tt_timeout);

		int rc = poll_msg(&msg, &g_disp_vsync_sem, timeout);
		switch (rc) {
		case OS_POLL_MSG:
			SYS_LOG_INF("msg type=0x%x, cmd=0x%x", msg.type, msg.cmd);
			switch (msg.type) {
				case MSG_UI:
					_process_ui_msg(&msg);
					break;
				case MSG_OTA:
					ota_app_process();
					break;
			}
			if (msg.callback != NULL) {
				msg.callback(&msg, 0, NULL);
			}
			break;

		case OS_POLL_SEM:
			if (g_disp_active) {
				lvx_refr_all();
			}
			break;
		}

		thread_timer_handle_expired();
	}

	return 0;
}
