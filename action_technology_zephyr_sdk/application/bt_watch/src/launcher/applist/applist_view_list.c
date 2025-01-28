/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include <widgets/text_canvas.h>
#include <widgets/watch_list.h>
#include "applist_view_inner.h"
#include <widgets/roll_bar.h>

/**********************
 *   STATIC PROTOTYPES
 **********************/
static int _list_view_create(lv_obj_t * scr);
static void _list_view_delete(lv_obj_t * scr);
static void _list_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);
static void _list_view_update_text(applist_ui_data_t * data, uint8_t idx, const char * text);

/**********************
 *  GLOBAL VARIABLES
 **********************/
const applist_view_cb_t g_applist_list_view_cb = {
	.create = _list_view_create,
	.delete = _list_view_delete,
	.update_icon = _list_view_update_icon,
	.update_text = _list_view_update_text,
};

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void _list_scroll_handler(lv_obj_t *list, lv_point_t *icon_center)
{
	applist_ui_data_t *data = lv_obj_get_user_data(list);
	uint32_t dist_r = data->track_radius * data->track_radius;
	uint32_t dist_y = (icon_center->y - data->track_center.y) *(icon_center->y - data->track_center.y);
	uint32_t dist = (dist_y <= dist_r) ? (dist_r - dist_y) : (dist_y - dist_r);

	lv_sqrt_res_t res;
	lv_sqrt(dist, &res, 0x800);
	icon_center->x = (dist_y <= dist_r) ?
			(data->track_center.x - res.i) : (data->track_center.x + res.i);
}

static int _list_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_list_view_t *list_view = &data->list_view;

	data->cont = watch_list_create(scr);
	lv_obj_set_pos(data->cont, data->res_scene.x, data->res_scene.y);
	lv_obj_set_size(data->cont, data->res_scene.width, data->res_scene.height);
	lv_obj_set_style_pad_top(data->cont, data->pad_top, LV_PART_MAIN);
	lv_obj_set_style_pad_bottom(data->cont, data->pad_bottom, LV_PART_MAIN);
	lv_obj_set_style_text_font(data->cont, &data->font, LV_PART_MAIN); /* inherit text style */
	lv_obj_set_style_text_color(data->cont, data->text[0].color, LV_PART_MAIN);
	lv_obj_set_user_data(data->cont, data);

	lv_style_init(&list_view->sty_btn);
	lv_style_set_height(&list_view->sty_btn, data->item_height);
	lv_style_set_pad_column(&list_view->sty_btn, data->item_space);

	/* Add buttons to the list */
	for (int i = 0; i < NUM_ITEMS; i++) {
		lv_obj_t *btn = watch_list_add_btn(data->cont,
				applist_get_icon(data, i), applist_get_text(data, i));

		lv_obj_enable_style_refresh(false);
		lv_obj_add_style(btn, &list_view->sty_btn, LV_PART_MAIN);
		lv_obj_enable_style_refresh(true);

		lv_obj_add_event_cb(btn, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(btn, (void *)i);
	}

	if (data->track_radius > 0) {
		watch_list_set_scroll_cb(data->cont, _list_scroll_handler);
	}

	lv_obj_t *roll_arc = roll_arc_create(data->cont);
	if (roll_arc) {
		lv_obj_set_pos(roll_arc, -80, -10);
		roll_arc_set_diameter_width(roll_arc, 500, 10);
		roll_arc_set_bg_angles(roll_arc, 0, 40);
		roll_arc_set_rotation(roll_arc, 340);
		roll_bar_set_fade(roll_arc, true);
		roll_bar_set_opa(roll_arc, LV_OPA_0, LV_OPA_100);
		roll_bar_set_color(roll_arc, lv_color_hex(0x3b3b3b), lv_color_white());
		roll_bar_set_min(roll_arc, 30);
	}

	lv_obj_set_scroll_dir(scr, LV_DIR_NONE);

	int16_t scrl_y = (data->track_radius > 0) ? data->item_height : 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);

	/* must update layout before calling lv_obj_scroll_to_y() */
	lv_obj_update_layout(data->cont);
	lv_obj_scroll_to_y(data->cont, scrl_y, LV_ANIM_OFF);
	coder_simulation_tp_register(data->cont, APPLIST_VIEW, LV_DIR_VER, 50, NULL);

	return 0;
}

static void _list_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);
	applist_list_view_t *list_view = &data->list_view;

	lv_style_reset(&list_view->sty_btn);

	data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));
}

static void _list_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	lv_obj_t * obj_icon = watch_list_get_icon(data->cont, idx);
	if (obj_icon)
		simple_img_set_src(obj_icon, src);
}

static void _list_view_update_text(applist_ui_data_t * data, uint8_t idx, const char * text)
{
	lv_obj_t * obj_text = watch_list_get_text(data->cont, idx);
	if (obj_text)
		text_canvas_set_text_static(obj_text, text);
}
