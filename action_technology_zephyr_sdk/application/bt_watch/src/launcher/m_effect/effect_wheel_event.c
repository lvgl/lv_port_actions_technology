#include "effect_wheel_event.h"
#include <lvgl/widgets/face_map.h>
#include <app_ui.h>

static void lv_obj_set_index(lv_obj_t * obj, uint32_t index)
{
	lv_obj_t * parent = lv_obj_get_parent(obj);
	if (parent) {
		parent->spec_attr->children[index] = obj;
	}
}

uint32_t set_effect_wheel_permutation(lv_obj_t **obj,uint32_t num)
{
	uint32_t min_id = 0;
	lv_coord_t *z_value = app_mem_malloc(num * sizeof(lv_coord_t));
	if(z_value == NULL)
		return min_id;
	lv_coord_t min_z = 0;
	for(uint16_t i = 0 ; i < num ; i++)
	{
		vertex_t verts[4] = {0};
		face_map_get_periphery_dot(obj[i],verts);
		lv_coord_t z_0 = (lv_coord_t)verts[0].z;
		lv_coord_t z_1 = (lv_coord_t)verts[1].z;
		if(z_0 > z_1)
			z_value[i] = z_1 + (z_0 - z_1)/2;
		else
			z_value[i] = z_0 + (z_1 - z_0)/2;
		if(min_z > z_value[i] || i == 0)
		{
			min_z = z_value[i];
			min_id = i;
		}
	}
	lv_coord_t *index = app_mem_malloc(num * sizeof(uint32_t));
	if(index == NULL)
		return min_id;
	lv_coord_t *id = app_mem_malloc(num * sizeof(uint32_t));
	if(id == NULL)
		return min_id;
	lv_coord_t change_id = 0;
	uint32_t change_index = 0;
	for(uint16_t i = 0 ; i < num ; i++)
	{
		id[i] = i;
		index[i] = lv_obj_get_index(obj[i]);
	}
	for(uint16_t i = 0 ; i < effect_wheel_get_num() - 1; i++)
	{
		for(uint16_t j = i + 1; j < effect_wheel_get_num(); j++)
		{
			if(z_value[id[i]] < z_value[id[j]])
			{
				change_id = id[i];
				id[i] = id[j];
				id[j] = change_id;
			}
			if(index[i] > index[j])
			{
				change_index = index[i];
				index[i] = index[j];
				index[j] = change_index;
			}
		}
	}
	for(uint16_t i = 0 ; i < effect_wheel_get_num(); i++)
		lv_obj_set_index(obj[id[i]],index[i]);
	app_mem_free(z_value);
	app_mem_free(index);
	app_mem_free(id);
	return min_id;
}

void _effect_wheel_in_anim_create(effect_wheel_wheel_scene_data_t *data)
{
	lv_anim_t a;
	if(data->anim_observe_y_cb)
	{
		lv_anim_init(&a);
		lv_anim_set_var(&a, data);
		lv_anim_set_duration(&a, effect_wheel_get_zoom_time());
		lv_anim_set_values(&a,effect_wheel_get_slant_min(),effect_wheel_get_slant_max());
		lv_anim_set_exec_cb(&a, data->anim_observe_y_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_linear);
		lv_anim_start(&a);
	}

	if(data->anim_zoom_cb)
	{
		lv_anim_init(&a);
		lv_anim_set_var(&a, data);
		lv_anim_set_duration(&a, effect_wheel_get_zoom_time());
		lv_anim_set_values(&a,effect_wheel_get_zoom_min(),effect_wheel_get_zoom_max());
		lv_anim_set_exec_cb(&a, data->anim_zoom_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_linear);
		lv_anim_start(&a);
	}
}

void _effect_wheel_out_anim_create(effect_wheel_wheel_scene_data_t *data)
{
	lv_anim_t a;

	if(data->anim_zoom_cb)
	{
		lv_anim_delete(data,data->anim_zoom_cb);
		lv_anim_init(&a);
		lv_anim_set_var(&a, data);
		lv_anim_set_duration(&a, effect_wheel_get_zoom_time());
		lv_anim_set_values(&a,data->anim_zoom,effect_wheel_get_zoom_min());
		lv_anim_set_exec_cb(&a, data->anim_zoom_cb);
		if(data->anim_out_cb)
			lv_anim_set_deleted_cb(&a,data->anim_out_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_linear);
		lv_anim_start(&a);
	}

	if(data->anim_observe_y_cb)
	{
		lv_anim_delete(data, data->anim_observe_y_cb);
		lv_anim_init(&a);
		lv_anim_set_var(&a, data);
		lv_anim_set_duration(&a, effect_wheel_get_zoom_time());
		lv_anim_set_values(&a,data->anim_observe_y,0);
		lv_anim_set_exec_cb(&a, data->anim_observe_y_cb);
		lv_anim_set_path_cb(&a, lv_anim_path_linear);
		lv_anim_start(&a);
	}
}

void _effect_wheel_touch_event_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_indev_t * indev = lv_event_get_param(e);
	effect_wheel_wheel_scene_data_t * data = lv_event_get_user_data(e);

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
			data->move_x += LV_ABS(vect.x);
			int16_t angle_y = -vect.x * effect_wheel_get_move_coefficient() * 10 / 255;
			while(angle_y >= 3600)
				angle_y -= 3600;
			while(angle_y <= -3600)
				angle_y += 3600;
			data->move_angle += angle_y;
			data->angle_y += angle_y;
			_effect_wheel_wheel_angle(data,angle_y);
		}
	}
	else if (LV_EVENT_PRESSED == code)
	{
		data->direction = NULL_SLIDE;
		data->move_x = 0;
		lv_anim_delete(data,data->anim_scroll_cb);
	}
	else if (LV_EVENT_RELEASED == code)
	{
		int32_t auto_time = 0;
		int32_t dx = 0;
		if(data->move_angle != 0)
		{
			int32_t r_time = lv_tick_get() - data->move_time;
			if(r_time == 0)	r_time = 1;
			auto_time = (LV_ABS(data->move_angle) * 1000)/(r_time * effect_wheel_get_accelerated_speed());
			if(data->move_angle > 0)
				dx = (data->move_angle * auto_time / r_time) + ((effect_wheel_get_accelerated_speed() * auto_time * auto_time/ 1000)>>1);
			else
				dx = (data->move_angle * auto_time / r_time) - ((effect_wheel_get_accelerated_speed() * auto_time * auto_time/ 1000)>>1);
			auto_time = auto_time*effect_wheel_get_timer_speed();
			if(auto_time > 1000*effect_wheel_get_timer_speed())
				auto_time = 1000*effect_wheel_get_timer_speed();
			data->time_angle = 0;
		}
		lv_coord_t average_angle = 3600 / effect_wheel_get_num();
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
		if(dx != 0 && data->anim_scroll_cb)
		{
			if(auto_time == 0)
				auto_time = effect_wheel_get_calibration_timer();
			lv_anim_t a;
			lv_anim_init(&a);
			lv_anim_set_var(&a, data);
			lv_anim_set_duration(&a, auto_time);
			lv_anim_set_values(&a, data->time_angle,dx);
			lv_anim_set_exec_cb(&a, data->anim_scroll_cb);
			lv_anim_set_path_cb(&a, lv_anim_path_linear);
			lv_anim_start(&a);
		}
	}
}