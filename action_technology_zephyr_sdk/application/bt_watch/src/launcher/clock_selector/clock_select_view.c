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
#include "clock_selector.h"
#include "clock_selector_view.h"

/**********************
 *      TYPEDEFS
 **********************/
#define FACE_INTERVAL 300
#define MIN_ZOOM_VALUE 125
#define MAX_TURN_ANGLE 700
#define MOVE_COEFFICIENT 330	//MOVE_COEFFICIENT/255
#define D_TIMER_SPEED 100
enum {
	NULL_SLIDE = 0,
	LEFT_SLIDE,
	RIGHT_SLIDE
};

typedef struct {
	lv_obj_t *cont;
	lv_obj_t *face[NUM_CLOCK_IDS];
	lvgl_res_picregion_t picreg;
	lv_image_dsc_t face_imgs[NUM_CLOCK_IDS];
	lv_coord_t move_x;
	lv_coord_t st_x;
	lv_coord_t end_x;
	lv_coord_t timer_move;
	uint8_t direction;
} clock_select_scene_data_t;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/

static int _clock_select_scene_load_resource(clock_select_scene_data_t *data)
{
	lvgl_res_scene_t res_scene;
	int res;

	res = lvgl_res_load_scene(SCENE_CLOCK_SEL_VIEW, &res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (res < 0) {
		SYS_LOG_ERR("scene 0x%x not found", SCENE_CLOCK_SEL_VIEW);
		return -ENOENT;
	}

	res = lvgl_res_load_picregion_from_scene(&res_scene, RES_THUMBNAIL, &data->picreg);
	if (res || data->picreg.frames < ARRAY_SIZE(data->face_imgs)) {
		SYS_LOG_ERR("cannot find picreg 0x%x\n", RES_THUMBNAIL);
		res = -ENOENT;
		goto fail_exit;
	}

	/*res = lvgl_res_load_pictures_from_picregion(&picreg, 0, NUM_CLOCK_IDS - 1, data->face_imgs);

	if (res) {
		SYS_LOG_ERR("cannot load picreg 0x%x pictures\n", SCENE_CLOCK_SEL_VIEW);
		goto fail_exit;
	}*/

fail_exit:
	lvgl_res_unload_scene(&res_scene);
	return res;

}

static void lv_obj_exchange_index(lv_obj_t * obj, lv_obj_t * obj_1,bool draw)
{
    const int32_t old_index = lv_obj_get_index(obj);
	const int32_t old_index_1 = lv_obj_get_index(obj_1);

    lv_obj_t * parent = lv_obj_get_parent(obj);
	lv_obj_t * parent_1 = lv_obj_get_parent(obj_1);

	if(parent == NULL || parent != parent_1)
		return;

    parent->spec_attr->children[old_index] = obj_1;
	parent->spec_attr->children[old_index_1] = obj;
	if(draw)
	{
    	lv_obj_send_event(parent, LV_EVENT_CHILD_CHANGED, NULL);
    	lv_obj_invalidate(parent);
	}
}

static void _clock_select_move_x(clock_select_scene_data_t * data,lv_coord_t id, lv_coord_t x ,lv_coord_t zoom ,lv_coord_t angle)
{
	lv_obj_set_x(data->face[id],x);
	uint32_t zoom_w = data->picreg.height * zoom / LV_SCALE_NONE;
	lv_coord_t w = zoom_w * lv_trigo_cos(angle/10) / LV_TRIGO_SIN_MAX;
	x += (data->picreg.height - w)>>1;
	lv_coord_t x2 = x + w;
	if(x >= DEF_UI_WIDTH || x2 <= 0)
	{
		face_map_set_src(data->face[id], NULL);
		if(data->face_imgs[id].data)
		{
			lvgl_res_unload_pictures(&data->face_imgs[id], 1);
			data->face_imgs[id].data = NULL;
		}
	}
	else
	{
		if(data->face_imgs[id].data == NULL)
		{
			if(lvgl_res_load_pictures_from_picregion(&data->picreg, id, id, &data->face_imgs[id]) < 0)
				SYS_LOG_ERR("_clock_select_move_x load error %d\n", id);
		}
		if(data->face_imgs[id].data)
			face_map_set_src(data->face[id], &data->face_imgs[id]);
	}
}

static void _clock_select_move(clock_select_scene_data_t * data,lv_point_t point_v)
{
	lv_coord_t point_x = point_v.x + data->move_x;
	if(data->move_x != 0 && point_x > 0)
		point_x = 0;
	else if(data->move_x != -(data->end_x - data->st_x) * MOVE_COEFFICIENT / 255 && point_x < -(data->end_x - data->st_x) * MOVE_COEFFICIENT / 255)
		point_x = -(data->end_x - data->st_x) * MOVE_COEFFICIENT / 255;

	if(point_x <= 0 && point_x >= -(data->end_x - data->st_x) * MOVE_COEFFICIENT / 255)
	{
		data->move_x = point_x;
		lv_coord_t num_x[NUM_CLOCK_IDS];
		lv_coord_t face_angle[NUM_CLOCK_IDS];
		for(uint16_t i = 0;i<NUM_CLOCK_IDS;i++)
		{
			num_x[i] = data->move_x  * 255 / MOVE_COEFFICIENT + FACE_INTERVAL * i;
			face_angle[i] = MAX_TURN_ANGLE * num_x[i] / FACE_INTERVAL;
			if(LV_ABS(face_angle[i]) > MAX_TURN_ANGLE)
				face_angle[i] = face_angle[i] > 0 ? MAX_TURN_ANGLE : -MAX_TURN_ANGLE;
			face_map_periphery_dot_rest(data->face[i],false);
			face_map_set_angle_vect(data->face[i],0, -face_angle[i], 0,false);
		}
		lv_coord_t zoom[NUM_CLOCK_IDS];
		lv_coord_t w = data->picreg.width;
		lv_coord_t min_num_x = LV_ABS(num_x[0]);
		int16_t id = 0;
		for(uint16_t i = 0;i<NUM_CLOCK_IDS;i++)
		{
			for(uint16_t j = 0 ; j < NUM_CLOCK_IDS; j++)
			{
				if(LV_ABS(num_x[i]) < LV_ABS(num_x[j]) && lv_obj_get_index(data->face[i]) < lv_obj_get_index(data->face[j]))
					lv_obj_exchange_index(data->face[i],data->face[j],false);
			}
			zoom[i] = LV_SCALE_NONE - LV_ABS(num_x[i]) * (LV_SCALE_NONE - MIN_ZOOM_VALUE) / DEF_UI_VIEW_WIDTH;
			if(zoom[i] < MIN_ZOOM_VALUE)
				zoom[i] = MIN_ZOOM_VALUE;
			face_map_set_zoom(data->face[i],zoom[i]);
			if(min_num_x > LV_ABS(num_x[i]))
			{
				min_num_x = LV_ABS(num_x[i]);
				id = i;
			}
		}
		lv_coord_t x_diff = ((w - LV_ABS(lv_trigo_cos(face_angle[id]/10)) * w / LV_TRIGO_SIN_MAX * zoom[id] / LV_SCALE_NONE)>>1);
		lv_coord_t x_diff_j = x_diff;
		lv_coord_t x_diff_z = -x_diff;
		if(num_x[id] > 0)
			x_diff = -x_diff;
		x_diff_j += x_diff;
		x_diff_z += x_diff;
		lv_coord_t at_x = data->move_x * 255 / MOVE_COEFFICIENT + data->st_x + FACE_INTERVAL * id;
		_clock_select_move_x(data,id,at_x + x_diff,zoom[id],face_angle[id]);
		for(uint16_t i = 1;i<NUM_CLOCK_IDS;i++)
		{
			if(id - i >= 0)
			{
				lv_coord_t dx = ((w - LV_ABS(lv_trigo_cos(face_angle[id - i]/10)) * w / LV_TRIGO_SIN_MAX * zoom[id - i] / LV_SCALE_NONE)>>1);
				x_diff_j += dx;
				_clock_select_move_x(data,id - i,at_x - FACE_INTERVAL * i + x_diff_j,zoom[id - i],face_angle[id - i]);
				x_diff_j += dx;
			}
			if(id + i < NUM_CLOCK_IDS)
			{
				lv_coord_t dx = ((w - LV_ABS(lv_trigo_cos(face_angle[id + i]/10)) * w / LV_TRIGO_SIN_MAX * zoom[id + i] / LV_SCALE_NONE)>>1);
				x_diff_z -= dx;
				_clock_select_move_x(data,id + i,at_x + FACE_INTERVAL * i + x_diff_z,zoom[id + i],face_angle[id + i]);
				x_diff_z -= dx;
			}
		}
	}
}

static void _clock_select_scroll_anim(void * var, int32_t v)
{
	clock_select_scene_data_t *data = (clock_select_scene_data_t *)var;
	lv_point_t point_v = {0};
	point_v.x = v - data->timer_move;
	data->timer_move = v;
	_clock_select_move(data,point_v);
}

static void _clock_select_touch_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t * indev = lv_event_get_param(e);
	clock_select_scene_data_t * data = lv_event_get_user_data(e);
	lv_point_t point_v = {0};
	lv_indev_get_vect(indev,&point_v);
	if (code == LV_EVENT_PRESSED) {
		data->direction = NULL_SLIDE;
		lv_anim_delete(data,_clock_select_scroll_anim);
	}
	else if (code == LV_EVENT_PRESSING) {
		if(point_v.x > 0)
			data->direction = RIGHT_SLIDE;
		else if(point_v.x < 0)
			data->direction = LEFT_SLIDE;
		_clock_select_move(data,point_v);
	}
	else if (code == LV_EVENT_RELEASED) {
		lv_coord_t residue_move = (LV_ABS(data->move_x) * 255 / MOVE_COEFFICIENT) % FACE_INTERVAL;
		if(residue_move != 0)
		{
			lv_coord_t end_x = (data->move_x * 255 / MOVE_COEFFICIENT / FACE_INTERVAL) * FACE_INTERVAL;
			if(LEFT_SLIDE == data->direction)
			{
				if(residue_move > (FACE_INTERVAL / 3))
					end_x -= FACE_INTERVAL;
			}
			else if(RIGHT_SLIDE == data->direction)
			{
				if(residue_move > FACE_INTERVAL - (FACE_INTERVAL / 3))
					end_x -= FACE_INTERVAL;
			}
			end_x = end_x * MOVE_COEFFICIENT / 255;
			data->timer_move = data->move_x;
			lv_anim_t a;
			lv_anim_init(&a);
			lv_anim_set_var(&a, data);
			lv_anim_set_duration(&a, D_TIMER_SPEED);
			lv_anim_set_values(&a,data->move_x,end_x);
			lv_anim_set_exec_cb(&a, _clock_select_scroll_anim);
			lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
			lv_anim_start(&a);
		}
		data->direction = NULL_SLIDE;
	}
}

/*static int _clock_select_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_CLOCK_SEL_VIEW, NULL, 0,
			lvgl_res_scene_preload_default_cb_for_view, (void *)CLOCK_SELECTOR_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}*/

static void _clocksel_slide_click_event_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);
	clock_select_scene_data_t * data = (clock_select_scene_data_t *)view_data->user_data;
	if(NULL_SLIDE == data->direction && lvgl_click_decision())
	{
		const clock_selector_view_presenter_t *presenter = view_get_presenter(view_data);
		lv_obj_t *face = lv_event_get_current_target(e);
		int32_t id = (int32_t)lv_obj_get_user_data(face);
		presenter->set_clock_id((uint8_t)id);
	}
}

static int _clock_select_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);

	clock_select_scene_data_t * data = app_mem_malloc(sizeof(* data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(* data));

	if (_clock_select_scene_load_resource(data)) {
		SYS_LOG_ERR("load res failed");
		app_mem_free(data);
		return -ENOENT;
	}
	view_data->user_data = data;

	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont,0,0);
	lv_obj_set_size(data->cont,DEF_UI_VIEW_WIDTH,DEF_UI_VIEW_HEIGHT);
	lv_obj_set_style_pad_all(data->cont,0,LV_PART_MAIN);
	lv_obj_set_style_border_width(data->cont,0,LV_PART_MAIN);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_NONE);
	lv_obj_set_scrollbar_mode(data->cont, LV_SCROLLBAR_MODE_OFF); /* default no scrollbar */
	lv_obj_add_event_cb(data->cont, _clock_select_touch_event_cb, LV_EVENT_ALL, data);

	lv_coord_t w = data->picreg.width,h = data->picreg.height;
	lv_coord_t all_x = w , all_y = h ;
	lv_coord_t centre_x = w/2, centre_y = h/2, centre_z = 0;

	const vertex_t verts[4] = {
		{-centre_x			,-centre_y			,0},
		{all_x - centre_x	,-centre_y			,0},
		{all_x - centre_x	,all_y - centre_y	,0},
		{-centre_x			,all_y - centre_y	,0},
	};
	data->st_x = (DEF_UI_VIEW_WIDTH-w)>>1;
	data->end_x = data->st_x + FACE_INTERVAL * (NUM_CLOCK_IDS - 1);

	for(uint32_t i = 0 ; i < NUM_CLOCK_IDS ; i++)
	{
		data->face[i] = face_map_create(data->cont);
		face_map_set_periphery_dot(data->face[i],(vertex_t *)verts);
		face_map_set_pivot(data->face[i],centre_x,centre_y,centre_z);
		face_map_set_normals(data->face[i],false);
		lv_obj_align(data->face[i],LV_ALIGN_LEFT_MID,data->st_x + FACE_INTERVAL * i,0);
		face_map_set_observe(data->face[i],0,0,centre_x*10,true);
		//face_map_set_src(data->face[i], &data->face_imgs[i]);
		lv_obj_add_flag(data->face[i], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(data->face[i], LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_add_event_cb(data->face[i], _clocksel_slide_click_event_handler, LV_EVENT_SHORT_CLICKED, view_data);
		lv_obj_set_user_data(data->face[i],(void *)i);
	}

	const clock_selector_view_presenter_t *presenter = view_get_presenter(view_data);
	uint8_t id = presenter->get_clock_id();
	lv_point_t point_v = {0};
	point_v.x -= id * FACE_INTERVAL * MOVE_COEFFICIENT / 255;
	_clock_select_move(data,point_v);
	return 0;
}

static int _clock_select_view_delete(view_data_t *view_data)
{
	clock_select_scene_data_t * data = view_data->user_data;
	lv_anim_delete(data,_clock_select_scroll_anim);
	if (data) {
		lvgl_res_unload_pictures(data->face_imgs, ARRAY_SIZE(data->face_imgs));
		lvgl_res_unload_picregion(&data->picreg);
		app_mem_free(data);
	} else {
		lvgl_res_preload_cancel_scene(SCENE_CLOCK_SEL_VIEW);
	}

	lvgl_res_unload_scene_compact(SCENE_CLOCK_SEL_VIEW);
	return 0;
}

static int _clock_select_view_proc_key(view_data_t *view_data, ui_key_msg_data_t * key_data)
{
	clock_select_scene_data_t * data = view_data->user_data;
	if (data == NULL)
		return 0;

	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT)
		key_data->done = true;

	return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int clock_selector_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	assert(view_id == CLOCK_SELECTOR_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return ui_view_layout(view_id);
	case MSG_VIEW_LAYOUT:
		return _clock_select_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _clock_select_view_delete(view_data);
	case MSG_VIEW_KEY:
		return _clock_select_view_proc_key(view_data, msg_data);
	case MSG_VIEW_FOCUS:
	case MSG_VIEW_DEFOCUS:
	default:
		return 0;
	}
}

VIEW_DEFINE2(clock_select3d, clock_selector_view_handler, NULL, NULL, CLOCK_SELECTOR_VIEW,
		NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
