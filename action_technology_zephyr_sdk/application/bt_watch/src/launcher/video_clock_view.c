/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <assert.h>
#include <app_ui.h>
#include "clock_selector/clock_selector.h"
#include "clock_view.h"
#include "widgets/simple_img.h"
#include "m_effect.h"
#include "widgets/watch_clock.h"
#ifdef CONFIG_VIDEO_PLAYER
#include "video_player.h"
#endif
#include <ui_mem.h>
#include <view_stack.h>
#include <memory/mem_cache.h>
#include "launcher_app.h"
#include "ui_coder/ui_coder.h"

#ifdef CONFIG_UI_MANAGER
#  define UI_VIDEO_MALLOC(size)        ui_mem_aligned_alloc(MEM_RES, 64, size, __func__)
#  define UI_VIDEO_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#  define UI_VIDEO_REALLOC(ptr,size)   ui_mem_realloc(MEM_RES,ptr,size,__func__)
#else
#  define UI_VIDEO_MALLOC(size)        lv_malloc(size)
#  define UI_VIDEO_FREE(ptr)           lv_free(ptr)
#  define UI_VIDEO_REALLOC(ptr,size)   lv_realloc(ptr,size)
#endif

typedef struct video_clock_view_data {
	lv_obj_t *obj_clock;
	lv_image_dsc_t video_dsc;
	
	lvgl_res_group_t grp_hour;
	lvgl_res_group_t grp_minute;
	lvgl_res_group_t grp_second;

	lv_image_dsc_t img_tm_hour;
	lv_image_dsc_t img_tm_min;
	lv_image_dsc_t img_tm_sec;

	lvgl_res_scene_t scene;
	/* click event */
	uint32_t pressing_start;
	bool preview_invoked;
} video_clock_view_data_t;

static bool view_focus = false;
static video_clock_view_data_t *p_video_clock;
static os_mutex video_data_mutex;
#ifdef CONFIG_VIDEO_PLAYER
static video_data_t cur_video_data;
static bool video_opened = false;
#endif

enum {
	VIDEO_MSG_UPDATE_IMG = MSG_VIEW_USER_OFFSET,
	VIDEO_MSG_RELOAD_FINISHED_NOTIFY,
};

#ifdef CONFIG_VIDEO_PLAYER
static void *_ui_video_malloc(int size)
{
	void * ptr = UI_VIDEO_MALLOC(size);
	SYS_LOG_DBG("### _ui_video_malloc %p %d", ptr, size);
	return ptr;
}

static void _ui_video_free(void *ptr)
{
	SYS_LOG_DBG("### _ui_video_free %p",ptr);
	UI_VIDEO_FREE(ptr);
}

static void video_player_callback(vp_status_e status, void *user_data, video_data_t *video_data)
{
	(void)user_data;

	SYS_LOG_DBG("++++++ video_player_callback %d", status);

	if(p_video_clock == NULL)
		return;
	
	switch(status)
	{
		case VP_STATUS_PLAYING:
			os_mutex_lock(&video_data_mutex, OS_FOREVER);
			memcpy(&cur_video_data, video_data, sizeof(video_data_t));
			os_mutex_unlock(&video_data_mutex);
			ui_message_send_async(CLOCK_VIEW, VIDEO_MSG_UPDATE_IMG, 0);
			break;
			
		case VP_STATUS_STOPED:
			os_mutex_lock(&video_data_mutex, OS_FOREVER);
			memset(&cur_video_data, 0, sizeof(video_data_t));
			os_mutex_unlock(&video_data_mutex);
			break;

		default:
			break;
	}

	
}
#endif

#ifdef CONFIG_VIDEO_PLAYER
static void _video_img_update(video_clock_view_data_t *data)
{
	if (data) {
		os_mutex_lock(&video_data_mutex, OS_FOREVER);
		if (cur_video_data.decode_buf && cur_video_data.decode_buf_size) {
			data->video_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
			data->video_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
			data->video_dsc.header.w = cur_video_data.width;
			data->video_dsc.header.h = cur_video_data.height;
			data->video_dsc.header.stride = data->video_dsc.header.w * 2;
			data->video_dsc.data = cur_video_data.decode_buf;
			data->video_dsc.data_size = cur_video_data.decode_buf_size;
		}
		os_mutex_unlock(&video_data_mutex);
		if (data->obj_clock && data->video_dsc.data) {
			lv_obj_set_style_bg_img_src(data->obj_clock, &data->video_dsc, LV_PART_MAIN);
			lv_obj_invalidate(data->obj_clock);
		}
	}
}
#endif

static void _video_clock_view_unload_data(video_clock_view_data_t *data)
{
	lvgl_res_preload_cancel_scene(SCENE_CLOCK_VIEW);
	lvgl_res_unload_scene_compact(SCENE_CLOCK_VIEW);
	if(data) {
		lvgl_res_unload_pictures(&data->img_tm_hour, 1);
		lvgl_res_unload_pictures(&data->img_tm_min, 1);
		lvgl_res_unload_pictures(&data->img_tm_sec, 1);
		lvgl_res_unload_group(&data->grp_hour);
		lvgl_res_unload_group(&data->grp_minute);
		lvgl_res_unload_group(&data->grp_second);
		lvgl_res_unload_scene(&data->scene);
	}
}
static int _video_clock_view_load_data(video_clock_view_data_t *data)
{
	uint32_t pic_id;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_CLOCK_VIEW, &data->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", SCENE_CLOCK_VIEW);
		return -ENOENT;
	}
	pic_id = PIC_0;

	ret = lvgl_res_load_group_from_scene(&data->scene, RES_TM_HOUR, &data->grp_hour);
	if (ret < 0)
		goto out_exit;
	ret = lvgl_res_load_pictures_from_group(&data->grp_hour, &pic_id,&data->img_tm_hour, NULL, 1);
	if (ret < 0)
		goto out_exit;

	ret = lvgl_res_load_group_from_scene(&data->scene, RES_TM_MINUTE, &data->grp_minute);
	if (ret < 0)
		goto out_exit;
	ret = lvgl_res_load_pictures_from_group(&data->grp_minute, &pic_id,&data->img_tm_min, NULL, 1);
	if (ret < 0)
		goto out_exit;
		

	ret = lvgl_res_load_group_from_scene(&data->scene, RES_TM_SECOND, &data->grp_second);
	if (ret < 0)
		goto out_exit;
	ret = lvgl_res_load_pictures_from_group(&data->grp_second, &pic_id,&data->img_tm_sec, NULL, 1);
	if (ret < 0)
		goto out_exit;

out_exit:
	lvgl_res_unload_group(&data->grp_hour);
	lvgl_res_unload_group(&data->grp_minute);
	lvgl_res_unload_group(&data->grp_second);
	if (ret < 0)
		_video_clock_view_unload_data(data);
	return ret;
}

static int _video_clock_view_preload(view_data_t *view_data)
{
	int ret;
	ret = lvgl_res_preload_scene_compact(SCENE_CLOCK_VIEW , NULL , 0 ,
			lvgl_res_scene_preload_default_cb_for_view, (void *)CLOCK_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret) {
		LV_LOG_ERROR("%d",ret);
		return ret;
	}
	return 0;
}

static int _video_clock_view_paint(view_data_t *view_data)
{
	video_clock_view_data_t *data = view_data->user_data;

	if (data == NULL) {
		return 0;
	}
	struct rtc_time time;
	//int32_t value;
	/* update data via presenter*/
	clock_presenter->get_time(&time);
	SYS_LOG_DBG("time %02u:%02u:%02u.%03u\n", time.tm_hour, time.tm_min, time.tm_sec, time.tm_ms);

	watch_clock_set_time(data->obj_clock, time.tm_hour, time.tm_min, time.tm_sec * 1000 + time.tm_ms);

	lv_obj_invalidate(data->obj_clock);

	return 0;
}

static void _video_clock_event_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	view_data_t *view_data = lv_event_get_user_data(e);
	video_clock_view_data_t *data = view_data->user_data;
	uint32_t pressing_duration = 0;
	if (data->preview_invoked)
		return;

	if (event == LV_EVENT_PRESSED) {
		data->pressing_start = os_uptime_get_32();
	} else if (event == LV_EVENT_RELEASED || event == LV_EVENT_PRESSING) {
		pressing_duration = os_uptime_get_32() - data->pressing_start;
	}

	if (pressing_duration > CLOCK_SWITCH_PRESSING_TIME) {
		if (clock_presenter->invoke_preview) {
			data->preview_invoked = clock_presenter->invoke_preview() ? false : true;
		}
	}
}

static void _video_clock_btn_event_handler(lv_event_t * e)
{
	if(lvgl_click_decision())
	{
		go_to_m_effect();
		lv_obj_remove_event_cb(lv_event_get_user_data(e),_video_clock_btn_event_handler);
	}
}


static int _video_clock_view_layout(view_data_t *view_data)
{
	video_clock_view_data_t *data = view_data->user_data;
	
	if(data == NULL)
	{
		data = app_mem_malloc(sizeof(*data));
		if (data == NULL) {
			return -ENOMEM;
		}
		memset(data, 0, sizeof(*data));
		view_data->user_data = data;
	}

	if (_video_clock_view_load_data(data)) {
		return -ENOENT;
	}

	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	coder_simulation_tp_register(scr, CLOCK_VIEW, LV_DIR_HOR, 50, NULL);
	data->obj_clock = watch_clock_create(scr);

#ifdef CONFIG_VIDEO_PLAYER
	int ret;
	memset(&cur_video_data, 0, sizeof(cur_video_data));
	p_video_clock = data;
	video_init_param_t init_param = {0};
	init_param.cb = video_player_callback;
	init_param.cb_data = data;
	init_param.need_decode = true;
	init_param.url = CONFIG_APP_FAT_DISK"/test.avi";
	init_param.alloc = _ui_video_malloc;
	init_param.free = _ui_video_free;
#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
	init_param.mute = false;
#else
	init_param.mute = true;
#endif
	init_param.preload = true;
	init_param.repeat = true;
	ret = video_player_open(&init_param);
	video_opened = true;
	if (ret == 0) {
		_video_img_update(data);
	}
#endif
	
	watch_clock_set_type(data->obj_clock, ANALOG_CLOCK, true);
	watch_clock_set_24hour(data->obj_clock,false);
	lv_obj_set_size(data->obj_clock,466,466);
	lv_obj_center(data->obj_clock);
	lv_obj_set_style_bg_image_opa(data->obj_clock, LV_OPA_COVER, LV_PART_MAIN);

	lv_coord_t pivot_x, pivot_y;

	pivot_x = (DEF_UI_WIDTH / 2) - data->grp_hour.x;
	pivot_y = (DEF_UI_HEIGHT / 2) - data->grp_hour.y;
	watch_clock_set_pointer_images(data->obj_clock, CLOCK_HOUR, 1, &data->img_tm_hour, pivot_x, pivot_y);

	pivot_x = (DEF_UI_WIDTH / 2) - data->grp_minute.x;
	pivot_y = (DEF_UI_HEIGHT / 2) - data->grp_minute.y;
	watch_clock_set_pointer_images(data->obj_clock, CLOCK_MIN, 1, &data->img_tm_min, pivot_x, pivot_y);

	pivot_x = (DEF_UI_WIDTH / 2) - data->grp_second.x;
	pivot_y = (DEF_UI_HEIGHT / 2) - data->grp_second.y;
	watch_clock_set_pointer_images(data->obj_clock, CLOCK_SEC, 1, &data->img_tm_sec, pivot_x, pivot_y);
	lv_obj_add_event_cb(data->obj_clock, _video_clock_event_handler, LV_EVENT_ALL, view_data);
	lv_obj_add_event_cb(data->obj_clock, _video_clock_btn_event_handler, LV_EVENT_SHORT_CLICKED, data->obj_clock);

	_video_clock_view_paint(view_data);
	
	SYS_LOG_INF("video view inflated");
	return 0;
}

static int _video_clock_view_focus_changed(view_data_t *view_data, bool focused)
{
	/* data is allocate in preload() */
	SYS_LOG_INF("_video_clock_view_focus_changed %d",focused);
	video_clock_view_data_t *data = view_data->user_data;

	view_focus = focused;

	if (focused) {
		if (data) {
			_video_clock_view_load_data(data);
		}

		const clock_dsc_t * clock_dsc = clocksel_get_clock_dsc(clock_presenter->get_clock_id());
		if (clock_dsc) {
			clock_presenter->set_rtc(focused, clock_dsc->period);
		}

#ifdef CONFIG_VIDEO_PLAYER
		if (video_opened) {
		#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
			launcher_video_control(MSG_VIDEO_VIEW_FOCUS);
		#else
			video_player_play();
		#endif
		}
#endif
	} else {
#ifdef CONFIG_VIDEO_PLAYER
		if (video_opened) {
		#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
			launcher_video_control(MSG_VIDEO_VIEW_DEFOCUS);
		#else
			video_player_pause(true, false);
		#endif
		}
#endif
		if (data) {
			_video_clock_view_unload_data(data);
		}
	}

	return 0;
}

static int _video_clock_view_delete(view_data_t *view_data)
{
	video_clock_view_data_t *data = view_data->user_data;
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
#ifdef CONFIG_VIDEO_PLAYER
	video_player_close();
	video_opened = false;
#endif
	lv_obj_clean(scr);
	if (data) {		
		_video_clock_view_unload_data(data);
		app_mem_free(data);
	}
	p_video_clock = NULL;
	return 0;
}

int _video_clock_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	static bool inited = false;

	if(inited == false) {
		os_mutex_init(&video_data_mutex);
		inited = true;
	}
	
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _video_clock_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _video_clock_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _video_clock_view_delete(view_data);
	case MSG_VIEW_PAINT:
	case MSG_VIEW_RESUME_DISPLAY:
		return _video_clock_view_paint(view_data);
	case MSG_VIEW_FOCUS:
		return _video_clock_view_focus_changed(view_data, true);
	case MSG_VIEW_DEFOCUS:
		return _video_clock_view_focus_changed(view_data, false);
	case MSG_VIEW_UPDATE:
		return _video_clock_view_paint(view_data);
#ifdef CONFIG_VIDEO_PLAYER
	case VIDEO_MSG_UPDATE_IMG:
		_video_img_update(p_video_clock);
		return 0;
	case VIDEO_MSG_RELOAD_FINISHED_NOTIFY:
		if (view_focus) {
		#ifdef CONFIG_VIDEO_APP_AUDIO_ENABLE
			launcher_video_control(MSG_VIDEO_VIEW_FOCUS);
		#else
			video_player_play();
		#endif
		}
		return 0;
#endif
	default:
		return 0;
	}
}

void video_clock_view_cache_event_callback(uint8_t event)
{
	if (event == VIEW_CACHE_EVT_LOAD_END) {
		SYS_LOG_INF("video_clock_view proc %d",event);
#ifdef CONFIG_VIDEO_PLAYER
		ui_message_send_async(CLOCK_VIEW, VIDEO_MSG_RELOAD_FINISHED_NOTIFY, 0);
#endif
	}
}

