/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file watch_tileview.h
 *
 */

#ifndef WIDGETS_WATCH_TILEVIEW_H_
#define WIDGETS_WATCH_TILEVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
	lv_obj_t obj;
	lv_obj_t * tile_act;
	lv_obj_t * tile_center;
	lv_point_t scroll_center;

	uint8_t main_cnt;
	lv_dir_t cross_dir;
	uint8_t main_rotated : 1;
	uint8_t cross_overlapped : 1;

	lv_style_t style;
} watch_tileview_t;

typedef struct {
	lv_obj_t obj;
	lv_dir_t dir;
} watch_tileview_tile_t;

extern const lv_obj_class_t watch_tileview_class;
extern const lv_obj_class_t watch_tileview_tile_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a tileview object
 * @param parent pointer to an object, it will be the parent of the new object
 * @return pointer to the created list object
 */
lv_obj_t * watch_tileview_create(lv_obj_t * parent);

/**
 * Add new tile to a tileview object
 * @param obj pointer to a tileview object
 * @param tiles pointer to the created tile objects in the sequency of left to right, then the up and down tile.
 * @param tile_cnts number of tiles in the left, center, right, up and down direction
 *                  respectively to the center tile, considering main_dir as x-axis.
 * @param main_dir main scroll direction. accepted values: LV_DIR_HOR, LV_DIR_VER
 * @param main_rotated scroll behaviour in main dir
 *       true: scroll using rotated mode
 *       false: scroll using non-rotated mode
 * @param cross_overlapped scroll behaviour in cross dir
 *       true: scroll using overlapped mode
 *       false: scroll using push-pull mode
 * @return N/A
 */
void watch_tileview_add_tiles(lv_obj_t * obj, lv_obj_t * tiles[], const uint8_t tile_cnts[4],
							  lv_dir_t main_dir, bool main_rotated, bool cross_overlapped);

/**
 * Jump to the specific tile
 * @param obj pointer to a tileview object
 * @param anim_en scroll anim enabled or not
 * @return LV_RES_OK on success else LV_RES_INV
 */
lv_res_t watch_tileview_set_tile(lv_obj_t * obj, lv_obj_t * tile_obj, lv_anim_enable_t anim_en);

/*=====================
 * Getter functions
 *====================*/

/**
 * Get the active tile object
 * @return pointer to the active tile object
 */
lv_obj_t * watch_tileview_get_tile_act(lv_obj_t * obj);

/*=====================
 * Other functions
 *====================*/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* WIDGETS_WATCH_TILEVIEW_H_ */
