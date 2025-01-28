/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include <drivers/rtc.h>
#include <app_ui.h>
#include <widgets/draw_util.h>
#include <view_stack.h>
#include "launcher_app.h"
#include "effect_wheel_event.h"
#include "m_effect.h"

/**********************
 *      TYPEDEFS
 **********************/
const static uint32_t effect_wheel_bmp_ids[] = {
	PIC_FACE_WHEEL_1,PIC_FACE_WHEEL_2,PIC_FACE_WHEEL_3,PIC_FACE_WHEEL_4,PIC_FACE_WHEEL_5,PIC_FACE_WHEEL_6
};

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void _effect_wheel_wheel_observe_y(void * var, int32_t v)
{
	effect_wheel_wheel_scene_data_t *data = (effect_wheel_wheel_scene_data_t *)var;
	data->anim_observe_y = v;
	for(uint32_t i = 0 ; i < ICON_NUM ; i++)
		face_map_set_observe(data->effect_wheel[i],0,data->anim_observe_y,SURROUND_RADIUS*12,true);
}

static void _effect_wheel_wheel_zoom(void * var, int32_t v)
{
	effect_wheel_wheel_scene_data_t *data = (effect_wheel_wheel_scene_data_t *)var;
	data->anim_zoom = v;
	for(uint32_t i = 0 ; i < ICON_NUM ; i++)
	{
		face_map_set_observe_zoom(data->effect_wheel[i],v);
	}
	uint32_t clock_z = data->clock_zoom * v / LV_SCALE_NONE;
	face_map_set_observe_zoom(data->effect_wheel[1],clock_z);
}

static int _effect_wheel_scene_load_resource(effect_wheel_wheel_scene_data_t *data)
{
	lvgl_res_scene_t res_scene;
	int res;

	res = lvgl_res_load_scene(ONE_EFFECT_VIEW, &res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (res < 0) {
		SYS_LOG_ERR("scene 0x%x not found", EFFECT_WHEEL_VIEW);
		return -ENOENT;
	}

	res = lvgl_res_load_pictures_from_scene(&res_scene, effect_wheel_bmp_ids, data->effect_wheel_imgs,
			NULL, 1);
	if (res) {
		goto fail_exit;
	}

	res = lvgl_res_load_pictures_from_scene(&res_scene, &effect_wheel_bmp_ids[2], &data->effect_wheel_imgs[2],
			NULL, ARRAY_SIZE(effect_wheel_bmp_ids) - 2);

	if (res) {
		goto fail_exit;
	}

	lvgl_res_scene_t clock_scene;
	res = lvgl_res_load_scene(SCENE_CLOCK_SEL_VIEW, &clock_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (res < 0) {
		SYS_LOG_ERR("scene clock 0x%x not found", EFFECT_WHEEL_VIEW);
		goto fail_exit;
	}

	lvgl_res_picregion_t picreg;
	res = lvgl_res_load_picregion_from_scene(&clock_scene, RES_THUMBNAIL, &picreg);
	if (res) {
		SYS_LOG_ERR("cannot find picreg 0x%x\n", RES_THUMBNAIL);
		goto fail_exit;
	}


	uint8_t clock_id = m_effect_get_clock_id();
	res = lvgl_res_load_pictures_from_picregion(&picreg,clock_id,clock_id, &data->effect_wheel_imgs[1]);
	lvgl_res_unload_picregion(&picreg);

	data->clock_zoom = data->effect_wheel_imgs[0].header.w * LV_SCALE_NONE / data->effect_wheel_imgs[1].header.w;

fail_exit:
	lvgl_res_unload_scene(&res_scene);
	lvgl_res_unload_scene(&clock_scene);
	return res;

}

static void _effect_wheel_wheel_scroll_anim(void * var, int32_t v)
{
	effect_wheel_wheel_scene_data_t *data = (effect_wheel_wheel_scene_data_t *)var;
	lv_coord_t angle = v - data->time_angle;
	data->angle_y += angle;
	_effect_wheel_wheel_angle(data,angle);
	data->time_angle = v;
}

static void _effect_wheel_wheel_deleted(lv_anim_t * a)
{
	effect_wheel_wheel_scene_data_t *data = (effect_wheel_wheel_scene_data_t *)a->var;
	ui_switch_effect_set_type(UI_SWITCH_EFFECT_NONE);
	view_stack_clean();
	const view_cache_dsc_t *app_view_cache_dsc = get_app_view_cache_dsc();
	uint32_t id = data->effect_wheel_id;
	if(id > app_view_cache_dsc->num)
		id = app_view_cache_dsc->num - 1;
	view_stack_push_cache(app_view_cache_dsc, app_view_cache_dsc->vlist[id]);
	launcher_app_t *app = launcher_app_get();
	ui_switch_effect_set_type(app->switch_effect_mode);
}

void _effect_wheel_wheel_angle(effect_wheel_wheel_scene_data_t * data,lv_coord_t angle)
{
	for(uint8_t i = 0 ; i < ICON_NUM ; i++)
	{
		face_map_set_angle_vect(data->effect_wheel[i],0, angle, 0 , false);
	}
	data->effect_wheel_id = set_effect_wheel_permutation(data->effect_wheel,ICON_NUM);
	lv_obj_invalidate(data->cont);
}

static void _effect_wheel_short_clicked_event_cb(lv_event_t * e)
{
	effect_wheel_wheel_scene_data_t * data = lv_event_get_user_data(e);
	if(data->move_x < 5 && 
		lv_anim_get(data,data->anim_scroll_cb) == NULL &&
		lv_anim_get(data,data->anim_zoom_cb) == NULL)
	{
		_effect_wheel_out_anim_create(data);
		lv_obj_remove_event_cb(lv_obj_get_user_data(data->cont),_effect_wheel_short_clicked_event_cb);
	}
}

static int _effect_wheel_wheel_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(ONE_EFFECT_VIEW, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)EFFECT_WHEEL_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _effect_wheel_wheel_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	effect_wheel_wheel_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	if (_effect_wheel_scene_load_resource(data)) {
		SYS_LOG_ERR("load res failed");
		app_mem_free(data);
		return -ENOENT;
	}
	data->anim_observe_y_cb = _effect_wheel_wheel_observe_y;
	data->anim_zoom_cb = _effect_wheel_wheel_zoom;
	data->anim_out_cb = _effect_wheel_wheel_deleted;
	data->anim_scroll_cb = _effect_wheel_wheel_scroll_anim;

	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_VIEW_WIDTH,DEF_UI_VIEW_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _effect_wheel_touch_event_cb, LV_EVENT_ALL, data);

	lv_obj_t * btn_obj = lv_obj_create(data->cont);
	lv_obj_set_size(btn_obj,190,190);
	lv_obj_align(btn_obj,LV_ALIGN_CENTER,0,80);
	lv_obj_set_style_pad_all(btn_obj,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(btn_obj,0,LV_PART_MAIN);
	lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_set_user_data(data->cont,btn_obj);
	lv_obj_add_event_cb(btn_obj, _effect_wheel_short_clicked_event_cb, LV_EVENT_SHORT_CLICKED, data);

	lv_coord_t w = data->effect_wheel_imgs[0].header.w,h = data->effect_wheel_imgs[0].header.h;
	lv_coord_t all_x = w , all_y = h ;//,all_z = SURROUND_RADIUS * 2;
	lv_coord_t centre_x = w/2, centre_y = h/2, centre_z = SURROUND_RADIUS;

	const vertex_t verts[4] = {
		{-centre_x			,-centre_y			,-centre_z},
		{all_x - centre_x	,-centre_y			,-centre_z},
		{all_x - centre_x	,all_y - centre_y	,-centre_z},
		{-centre_x			,all_y - centre_y	,-centre_z},
	};

	int16_t average_angle = 3600 / ICON_NUM;
	int16_t angle_offset = 0;

	const view_cache_dsc_t *app_view_cache_dsc = get_app_view_cache_dsc();

	uint8_t view_num = view_stack_get_num();
	for (uint32_t i = 0 ; i < app_view_cache_dsc->num ; i++) {
		if (view_num > 1) {
			for (int j = view_num - 1 ; j >= 0 ; j--) {
				if(app_view_cache_dsc->vlist[i] == view_stack_level_get_view_id(j))	{
					angle_offset = average_angle*i;
					break;
				}
			}
		}
	}

	for(uint32_t i = 0 ; i < ICON_NUM ; i++)
	{
		data->effect_wheel[i] = face_map_create(data->cont);
		if(i != 1)
		{
			face_map_set_periphery_dot(data->effect_wheel[i],(vertex_t *)verts);
			face_map_set_pivot(data->effect_wheel[i],centre_x,centre_y,centre_z);
			face_map_set_angle_vect(data->effect_wheel[i],0, -average_angle*i + angle_offset, 0,false);
		}
		face_map_set_normals(data->effect_wheel[i],false);
		lv_obj_center(data->effect_wheel[i]);
		face_map_set_src(data->effect_wheel[i], &data->effect_wheel_imgs[i]);
		lv_obj_add_flag(data->effect_wheel[i], LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_set_user_data(data->effect_wheel[i],(void *)i);
	}

	all_x = data->effect_wheel_imgs[1].header.w;
	all_y = data->effect_wheel_imgs[1].header.h;
	centre_x = all_x / 2;
	centre_y = all_y / 2;
	centre_z = centre_z * LV_SCALE_NONE / data->clock_zoom;
	const vertex_t verts_clock[4] = {
		{-centre_x			,-centre_y			,-centre_z},
		{all_x - centre_x	,-centre_y			,-centre_z},
		{all_x - centre_x	,all_y - centre_y	,-centre_z},
		{-centre_x			,all_y - centre_y	,-centre_z},
	};
	face_map_set_periphery_dot(data->effect_wheel[1],(vertex_t *)verts_clock);
	face_map_set_pivot(data->effect_wheel[1],centre_x,centre_y,centre_z);
	face_map_set_angle_vect(data->effect_wheel[1],0, -average_angle + angle_offset, 0,false);
	face_map_set_observe_zoom(data->effect_wheel[1],data->clock_zoom);
	view_data->user_data = data;
	data->effect_wheel_id = set_effect_wheel_permutation(data->effect_wheel,ICON_NUM);
	_effect_wheel_in_anim_create(data);

	return 0;
}

static int _effect_wheel_wheel_view_delete(view_data_t *view_data)
{
	effect_wheel_wheel_scene_data_t * data = view_data->user_data;

	if (data) {
		lv_anim_delete(data, NULL);
		lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
		lv_obj_clean(scr);
		lvgl_res_unload_pictures(data->effect_wheel_imgs, ARRAY_SIZE(data->effect_wheel_imgs));
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(ONE_EFFECT_VIEW);
	}

	lvgl_res_unload_scene_compact(ONE_EFFECT_VIEW);
	return 0;
}

static int _effect_wheel_wheel_view_proc_key(view_data_t *view_data, ui_key_msg_data_t * key_data)
{
	effect_wheel_wheel_scene_data_t * data = view_data->user_data;
	if (data == NULL)
		return 0;

	SYS_LOG_ERR("_effect_wheel_wheel_view_proc_key %d",KEY_VALUE(key_data->event));
	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT)
		key_data->done = true;
	else if (KEY_VALUE(key_data->event) == KEY_POWER)
	{
		if(lv_anim_get(data,data->anim_zoom_cb))
			key_data->done = true;
		//_effect_wheel_out_anim_create(data);
	}

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int effect_wheel_wheel_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == EFFECT_WHEEL_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _effect_wheel_wheel_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _effect_wheel_wheel_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _effect_wheel_wheel_view_delete(view_data);
	case MSG_VIEW_KEY:
		return _effect_wheel_wheel_view_proc_key(view_data, msg_data);
	case MSG_VIEW_FOCUS:
	case MSG_VIEW_DEFOCUS:
	default:
		return 0;
	}
}

VIEW_DEFINE2(effect_wheel_wheel, effect_wheel_wheel_view_handler, NULL, NULL, EFFECT_WHEEL_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

