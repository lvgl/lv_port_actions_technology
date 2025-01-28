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

/**********************
 *      TYPEDEFS
 **********************/
enum {
	NULL_SLIDE = 0,
	LEFT_SLIDE,
	RIGHT_SLIDE
};
#define ICON_NUM 6
#define INCLINE_RADIUS 200
#define SURROUND_RADIUS 250
#define MOVE_COEFFICIENT 110	//MOVE_COEFFICIENT/255
#define D_ICON_ACCELERATED_SPEED 80 //°/s
#define D_ICON_TIMER_SPEED	6	//
typedef struct {
	lv_obj_t *cont;
	lv_obj_t *face[ICON_NUM];
	lv_image_dsc_t face_imgs[ICON_NUM];

	lv_coord_t angle_y;
	uint32_t move_time;
	lv_coord_t move_angle;
	lv_coord_t time_angle;
	lv_font_t font;
	uint8_t direction;
} face_wheel_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static int _face_scene_load_resource(face_wheel_scene_data_t *data)
{
	lvgl_res_scene_t res_scene;
	int ret;

	ret = lvgl_res_load_scene(SCENE_CUBEBOX_VIEW, &res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("scene 0x%x not found", FACE_WHEEL_VIEW);
		return -ENOENT;
	}

	lvgl_res_picregion_t picreg;
	ret = lvgl_res_load_picregion_from_scene(&res_scene, PIC_CUBE, &picreg);
	if (ret || picreg.frames < ARRAY_SIZE(data->face_imgs)) {
		SYS_LOG_ERR("cannot find picreg 0x%x\n", PIC_CUBE);
		ret = -ENOENT;
		goto fail_exit;
	}

	ret = lvgl_res_load_pictures_from_picregion(&picreg, 0, ARRAY_SIZE(data->face_imgs) - 1, data->face_imgs);
	lvgl_res_unload_picregion(&picreg);

	ret = LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);

	if (ret) {
		SYS_LOG_ERR("cannot load picreg 0x%x pictures\n", PIC_CUBE);
		goto fail_exit;
	}

fail_exit:
	lvgl_res_unload_scene(&res_scene);
	return ret;

}

static void lv_obj_exchange_index(lv_obj_t * obj, lv_obj_t * obj_1,bool draw)
{
    const int32_t old_index = lv_obj_get_index(obj);
	const int32_t old_index_1 = lv_obj_get_index(obj_1);

    lv_obj_t * parent = lv_obj_get_parent(obj);
	lv_obj_t * parent_1 = lv_obj_get_parent(obj_1);

	if(!parent || !parent_1 || parent != parent_1)
		return;
    parent->spec_attr->children[old_index] = obj_1;
	parent->spec_attr->children[old_index_1] = obj;
	if(draw)
	{
    	lv_obj_send_event(parent, LV_EVENT_CHILD_CHANGED, NULL);
    	lv_obj_invalidate(parent);
	}
}

static void set_face_permutation(face_wheel_scene_data_t * data)
{
	int16_t z_value[ICON_NUM] = {0};
	for(uint16_t i = 0 ; i < ICON_NUM ; i++)
	{
		vertex_t verts[4] = {0};
		face_map_get_periphery_dot(data->face[i],verts);
		int16_t z_0 = (int16_t)verts[0].z;
		int16_t z_1 = (int16_t)verts[1].z;
		if(z_0 > z_1)
			z_value[i] = z_1 + (z_0 - z_1)/2;
		else
			z_value[i] = z_0 + (z_1 - z_0)/2;
	}

	for(uint16_t i = 0 ; i < ICON_NUM ; i++)
	{
		for(uint16_t j = 0 ; j < ICON_NUM; j++)
		{
			if(z_value[i] < z_value[j] && lv_obj_get_index(data->face[i]) < lv_obj_get_index(data->face[j]))
				lv_obj_exchange_index(data->face[i],data->face[j],false);
		}
	}
}

static void _face_wheel_angle(face_wheel_scene_data_t * data,lv_coord_t angle)
{
	for(uint8_t i = 0 ; i < ICON_NUM ; i++)
	{
		//face_map_set_angle_vect(data->face[i],-INCLINE_RADIUS, 0, 0,false);
		face_map_set_angle_vect(data->face[i],0, angle, 0 , false);
		//face_map_set_angle_vect(data->face[i],INCLINE_RADIUS, 0, 0,false);
	}
	set_face_permutation(data);
	lv_obj_invalidate(data->cont);
}

static void _face_wheel_scroll_anim(void * var, int32_t v)
{
	face_wheel_scene_data_t *data = (face_wheel_scene_data_t *)var;
	lv_coord_t angle = v - data->time_angle;
	data->angle_y += angle;
	_face_wheel_angle(data,angle);
	data->time_angle = v;
}

static void _face_touch_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t * indev = lv_event_get_param(e);
	face_wheel_scene_data_t * data = lv_event_get_user_data(e);

	if (LV_EVENT_PRESSING == code) {
		lv_point_t vect = { 0, 0 };
		lv_indev_get_vect(indev, &vect);
		if(vect.x != 0)
		{
			if(vect.x > 0)
			{
				if(RIGHT_SLIDE != data->direction)
				{
					data->direction = RIGHT_SLIDE;
					data->move_angle = 0;
					data->move_time = lv_tick_get();
				}
			}
			else if(vect.x < 0)
			{
				if(LEFT_SLIDE != data->direction)
				{
					data->direction = LEFT_SLIDE;
					data->move_angle = 0;
					data->move_time = lv_tick_get();
				}
			}
			int16_t angle_y = -vect.x * MOVE_COEFFICIENT * 10 / 255;
			while(angle_y >= 3600)
				angle_y -= 3600;
			while(angle_y <= -3600)
				angle_y += 3600;
			data->move_angle += angle_y;
			data->angle_y += angle_y;
			_face_wheel_angle(data,angle_y);
		}
	}
	else if (LV_EVENT_PRESSED == code)
	{
		data->direction = NULL_SLIDE;
		lv_anim_delete(data,_face_wheel_scroll_anim);
	}
	else if (LV_EVENT_RELEASED == code)
	{
		int32_t r_time = lv_tick_get() - data->move_time;
		int32_t auto_time = (LV_ABS(data->move_angle) * 1000)/(r_time * D_ICON_ACCELERATED_SPEED);
		int32_t dx = 0;
		if(data->move_angle != 0)
		{
			if(data->move_angle > 0)
				dx = (data->move_angle * auto_time / r_time) + ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
			else
				dx = (data->move_angle * auto_time / r_time) - ((D_ICON_ACCELERATED_SPEED * auto_time * auto_time/ 1000)>>1);
			auto_time = auto_time*D_ICON_TIMER_SPEED;
			if(auto_time > 1000*D_ICON_TIMER_SPEED)
				auto_time = 1000*D_ICON_TIMER_SPEED;
			data->time_angle = 0;
			lv_coord_t average_angle = 3600 / ICON_NUM;
			lv_coord_t residue = LV_ABS(dx + data->angle_y) % average_angle;
			if(dx + data->angle_y > 0)
			{
				if(residue > average_angle / 3)
					dx += (average_angle - residue);
				else
					dx -= residue;
			}
			else
			{
				if(residue > average_angle / 3)
					dx -= (average_angle - residue);
				else
					dx += residue;
			}
			lv_anim_t a;
			lv_anim_init(&a);
			lv_anim_set_var(&a, data);
			lv_anim_set_duration(&a, auto_time);
			lv_anim_set_values(&a, data->time_angle,dx);
			lv_anim_set_exec_cb(&a, _face_wheel_scroll_anim);
			lv_anim_set_path_cb(&a, lv_anim_path_linear);
			lv_anim_start(&a);
		}
	}
}

static int _face_wheel_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_CUBEBOX_VIEW, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)FACE_WHEEL_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _face_wheel_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	face_wheel_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	if (_face_scene_load_resource(data)) {
		SYS_LOG_ERR("load res failed");
		app_mem_free(data);
		return -ENOENT;
	}

	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_VIEW_WIDTH,DEF_UI_VIEW_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _face_touch_event_cb, LV_EVENT_ALL, data);

	lv_coord_t w = data->face_imgs[0].header.w,h = data->face_imgs[0].header.h;
	lv_coord_t all_x = w , all_y = h ;//,all_z = SURROUND_RADIUS * 2;
	lv_coord_t centre_x = w/2, centre_y = h/2, centre_z = SURROUND_RADIUS;

	const vertex_t verts[4] = {
		{all_x - centre_x	,-centre_y			,-centre_z},
		{-centre_x			,-centre_y			,-centre_z},
		{-centre_x			,all_y - centre_y	,-centre_z},
		{all_x - centre_x	,all_y - centre_y	,-centre_z},
	};

	int16_t average_angle = 3600 / ICON_NUM;

	for(uint32_t i = 0 ; i < ICON_NUM ; i++)
	{
		data->face[i] = face_map_create(data->cont);
		face_map_set_periphery_dot(data->face[i],(vertex_t *)verts);
		face_map_set_pivot(data->face[i],centre_x,centre_y,centre_z);
		face_map_set_angle_vect(data->face[i],0, average_angle*i, 0,false);
		//face_map_set_angle_vect(data->face[i],INCLINE_RADIUS, 0, 0,false);
		face_map_set_observe(data->face[i],0,-centre_z*4,centre_z*10,true);
		face_map_set_normals(data->face[i],false);
		lv_obj_center(data->face[i]);
		face_map_set_zoom(data->face[i],200);
		face_map_set_src(data->face[i], &data->face_imgs[i]);
		lv_obj_add_flag(data->face[i], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(data->face[i], LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_set_user_data(data->face[i],(void *)i);
	}
	view_data->user_data = data;
	set_face_permutation(data);
	return 0;
}

static int _face_wheel_view_delete(view_data_t *view_data)
{
	face_wheel_scene_data_t * data = view_data->user_data;
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	if (data) {
		lv_anim_delete(data,_face_wheel_scroll_anim);
		lv_obj_clean(scr);
		LVGL_FONT_CLOSE(&data->font);
		lvgl_res_unload_pictures(data->face_imgs, ARRAY_SIZE(data->face_imgs));
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(SCENE_CUBEBOX_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_CUBEBOX_VIEW);
	return 0;
}

static int _face_wheel_view_proc_key(view_data_t *view_data, ui_key_msg_data_t * key_data)
{
	face_wheel_scene_data_t * data = view_data->user_data;
	if (data == NULL)
		return 0;

	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT)
		key_data->done = true;

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int face_wheel_view_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == FACE_WHEEL_VIEW);
	SYS_LOG_ERR("face_wheel_view_handler %d",msg_id);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _face_wheel_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _face_wheel_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _face_wheel_view_delete(view_data);
	case MSG_VIEW_KEY:
		return _face_wheel_view_proc_key(view_data, msg_data);
	default:
		return 0;
	}
}

VIEW_DEFINE2(face_wheel, face_wheel_view_handler, NULL, NULL, FACE_WHEEL_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

