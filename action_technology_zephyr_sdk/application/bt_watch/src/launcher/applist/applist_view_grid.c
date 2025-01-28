/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <widgets/simple_img.h>
#include "applist_view_inner.h"

/**********************
 *   DEFINES
 **********************/
#if LV_USE_GRID
#  define NUM_GRID_COLS   3
#  define NUM_GRID_ROWS   ((NUM_ITEMS + 2) / 3)
#endif

/**********************
 *   STATIC PROTOTYPES
 **********************/
static int _grid_view_create(lv_obj_t * scr);
static void _grid_view_delete(lv_obj_t * scr);
static void _grid_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src);

/**********************
 *  GLOBAL VARIABLES
 **********************/
const applist_view_cb_t g_applist_grid_view_cb = {
	.create = _grid_view_create,
	.delete = _grid_view_delete,
	.update_icon = _grid_view_update_icon,
	.update_text = NULL,
};

/**********************
 *  STATIC VARIABLES
 **********************/

static int _grid_view_create(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);

	data->cont = lv_obj_create(scr);
	lv_obj_set_pos(data->cont, data->res_scene.x, data->res_scene.y);
	lv_obj_set_size(data->cont, data->res_scene.width, data->res_scene.height);
	lv_obj_set_scroll_dir(data->cont, LV_DIR_VER);
	lv_obj_set_style_pad_all(data->cont, 20, LV_PART_MAIN);
	lv_obj_set_style_pad_row(data->cont, data->item_space, LV_PART_MAIN);
	lv_obj_set_style_pad_column(data->cont, data->item_space, LV_PART_MAIN);
	lv_obj_set_user_data(data->cont, data);

#if LV_USE_GRID
	applist_grid_view_t *grid_view = &data->grid_view;
	grid_view->col_dsc = lv_malloc(sizeof(lv_coord_t) * (NUM_GRID_COLS + 1));
	grid_view->row_dsc = lv_malloc(sizeof(lv_coord_t) * (NUM_GRID_ROWS + 1));
	if (grid_view->col_dsc == NULL || grid_view->row_dsc == NULL) {
		lv_free(grid_view->col_dsc);
		lv_free(grid_view->row_dsc);
		return -ENOMEM;
	}

	grid_view->col_dsc[NUM_GRID_COLS] = LV_GRID_TEMPLATE_LAST;
	for (int i = 0; i < NUM_GRID_COLS; i++)
		grid_view->col_dsc[i] = LV_GRID_CONTENT;

	grid_view->row_dsc[NUM_GRID_ROWS] = LV_GRID_TEMPLATE_LAST;
	for (int i = 0; i < NUM_GRID_ROWS; i++)
		grid_view->row_dsc[i] = LV_GRID_CONTENT;

	lv_obj_set_grid_dsc_array(data->cont, grid_view->col_dsc, grid_view->row_dsc);
	lv_obj_set_grid_align(data->cont, LV_GRID_ALIGN_SPACE_AROUND, LV_GRID_ALIGN_START);

	/* Add buttons to the list */
	for (int i = 0; i < NUM_ITEMS; i++) {
		lv_obj_t *btn = simple_img_create(data->cont);
		simple_img_set_src(btn, applist_get_icon(data, i));

		lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(btn, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(btn, (void *)i);

		uint8_t col = i % NUM_GRID_COLS;
		uint8_t row = i / NUM_GRID_COLS;
		lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, col, 1,
				LV_GRID_ALIGN_STRETCH, row, 1);
	}

#else
	lv_obj_set_flex_flow(data->cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_style_flex_main_place(data->cont, LV_FLEX_ALIGN_START, LV_PART_MAIN);
	//lv_obj_set_style_flex_main_place(data->cont, LV_FLEX_ALIGN_SPACE_AROUND, LV_PART_MAIN);

	/* Add buttons to the list */
	for (int i = 0; i < NUM_ITEMS; i++) {
		lv_obj_t *btn = simple_img_create(data->cont);
		simple_img_set_src(btn, applist_get_icon(data, i));

		lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(btn, applist_btn_event_def_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(btn, (void *)i);
	}
#endif /*LV_USE_GRID*/

	int16_t scrl_y = 0;
	data->presenter->load_scroll_value(NULL, &scrl_y);

	/* must update layout before calling lv_obj_scroll_to_y() */
	lv_obj_update_layout(data->cont);
	lv_obj_scroll_to_y(data->cont, scrl_y, LV_ANIM_OFF);

	return 0;
}

static void _grid_view_delete(lv_obj_t * scr)
{
	applist_ui_data_t *data = lv_obj_get_user_data(scr);

	data->presenter->save_scroll_value(0, lv_obj_get_scroll_y(data->cont));

#if LV_USE_GRID
	applist_grid_view_t *grid_view = &data->grid_view;
	lv_free(grid_view->col_dsc);
	lv_free(grid_view->row_dsc);
#endif
}

static void _grid_view_update_icon(applist_ui_data_t * data, uint8_t idx, const lv_image_dsc_t * src)
{
	lv_obj_t * obj_icon = lv_obj_get_child(data->cont, idx);
	simple_img_set_src(obj_icon, src);
}
