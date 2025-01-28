/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file svg_img.c
 *
 */

#ifdef CONFIG_VG_LITE

/*********************
 *      INCLUDES
 *********************/
#include <sdfs.h>
#ifdef CONFIG_UI_MANAGER
#  include <ui_mem.h>
#endif
#include "svg_img.h"
#include "draw_util.h"

#include <lvgl/porting/gpu/vglite/lv_vglite_buf.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &svg_img_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void svg_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void svg_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void svg_img_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_img(lv_event_t * e);

static void invalidate_image_area(lv_obj_t * svg);
static void refresh_image_area(lv_obj_t * svg);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t svg_img_class = {
	.constructor_cb = svg_img_constructor,
	.destructor_cb = svg_img_destructor,
	.event_cb = svg_img_event,
	.width_def = LV_SIZE_CONTENT,
	.height_def = LV_SIZE_CONTENT,
	.instance_size = sizeof(svg_img_t),
	.base_class = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/
#ifdef CONFIG_UI_MANAGER
#  define SVG_MALLOC(size)        ui_mem_alloc(MEM_RES, size, __func__)
#  define SVG_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#else
#  define SVG_MALLOC(size)        lv_mem_alloc(size)
#  define SVG_FREE(ptr)           lv_mem_free(ptr)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * svg_img_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

/*=====================
 * Setter functions
 *====================*/

lv_res_t svg_img_parse(lv_obj_t *obj, const void * data, size_t len)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (data == NULL || len == 0)
		return LV_RES_INV;

	char * svg_input = SVG_MALLOC(len + 1);
	if (svg_input == NULL)
		return LV_RES_INV;

	memcpy(svg_input, data, len);
	svg_input[len] = '\0';

	if (svg->image)
		deleteSVGImage(svg->image);

	svg->image = parseSVGImage(svg_input, &svg->w, &svg->h);
	if (svg->image == NULL) {
		SVG_FREE(svg_input);
		return LV_RES_INV;
	}

	SVG_FREE(svg_input);

	lv_obj_invalidate(obj);
	lv_obj_refresh_self_size(obj);

	svg->pivot.x = svg->w / 2;
	svg->pivot.y = svg->h / 2;
	refresh_image_area(obj);

	return LV_RES_OK;
}

lv_res_t svg_img_parse_file(lv_obj_t *obj, const char * filename)
{
	void *vaddr = NULL;
	int len = 0;

	if (sd_fmap(filename, &vaddr, &len)) {
		LV_LOG_ERROR("file %s not found", filename);
		return LV_RES_INV;
	}

	lv_res_t res = svg_img_parse(obj, vaddr, len);

#ifdef _WIN32
	sd_funmap(vaddr);
#endif

	return res;
}

void svg_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (svg->pivot.x != pivot_x || svg->pivot.y != pivot_y) {
		svg->pivot.x = pivot_x;
		svg->pivot.y = pivot_y;

		if (svg->image && (svg->angle > 0 || svg->zoom != LV_IMG_ZOOM_NONE))
			refresh_image_area(obj);
	}
}

void svg_img_set_angle(lv_obj_t *obj, int16_t angle)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	while (angle >= 3600) angle -= 3600;
	while (angle < 0) angle += 3600;

	if (angle != svg->angle) {
		svg->angle = angle;

		if (svg->image)
			refresh_image_area(obj);
	}
}

void svg_img_set_zoom(lv_obj_t * obj, uint16_t zoom)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (zoom == 0) zoom = 1;

	if (zoom != svg->zoom) {
		svg->zoom = zoom;

		if (svg->image)
			refresh_image_area(obj);
	}
}

void svg_img_get_pivot(lv_obj_t * obj, lv_point_t * pivot)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	*pivot = svg->pivot;
}

uint16_t svg_img_get_angle(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	return svg->angle;
}

uint16_t svg_img_get_zoom(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	return svg->zoom;
}

void svg_img_get_size(lv_obj_t * obj, lv_point_t * size_res)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;
	size_res->x = svg->w;
	size_res->y = svg->h;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void svg_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	svg_img_t * svg = (svg_img_t *)obj;
	svg->zoom = LV_IMG_ZOOM_NONE;

	lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

	LV_TRACE_OBJ_CREATE("finished");
}

static void svg_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);

	svg_img_t * svg = (svg_img_t *)obj;
	if (svg->image)
		deleteSVGImage(svg->image);
}

static void svg_img_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
	LV_UNUSED(class_p);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	svg_img_t * svg = (svg_img_t *)obj;

	/*Ancestor events will be called during drawing*/
	if (code != LV_EVENT_COVER_CHECK && code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_DRAW_POST) {
		/*Call the ancestor's event handler*/
		lv_res_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RES_OK) return;
	}

	if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		if (svg->angle > 0 || svg->zoom != LV_IMG_ZOOM_NONE) {
			lv_coord_t * s = lv_event_get_param(e);
			lv_coord_t pad_ori = *s;
			lv_coord_t w = lv_obj_get_width(obj);
			lv_coord_t h = lv_obj_get_height(obj);

			*s = LV_MAX(*s, pad_ori - svg->trans_area.x1);
			*s = LV_MAX(*s, pad_ori - svg->trans_area.y1);
			*s = LV_MAX(*s, pad_ori + svg->trans_area.x2 - w + 1);
			*s = LV_MAX(*s, pad_ori + svg->trans_area.y2 - h + 1);
		}
	} else if (code == LV_EVENT_HIT_TEST) {
		lv_hit_test_info_t * info = lv_event_get_param(e);
		lv_area_t coords;
		coords.x1 = svg->trans_area.x1 + obj->coords.x1;
		coords.y1 = svg->trans_area.y1 + obj->coords.y1;
		coords.x2 = svg->trans_area.x2 + obj->coords.x1;
		coords.y2 = svg->trans_area.y2 + obj->coords.y1;

		info->res = _lv_area_is_point_on(&coords, info->point, 0);
	} else if (code == LV_EVENT_GET_SELF_SIZE) {
		lv_point_t * p = lv_event_get_param(e);
		if (svg->image) {
			p->x = svg->w;
			p->y = svg->h;
		}
	} else if (code == LV_EVENT_DRAW_MAIN || code == LV_EVENT_DRAW_POST || code == LV_EVENT_COVER_CHECK) {
		draw_img(e);
	}
}

static void draw_img(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * obj = lv_event_get_target(e);
	svg_img_t * svg = (svg_img_t *)obj;

	if (code == LV_EVENT_COVER_CHECK) {
		lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
	} else if (code == LV_EVENT_DRAW_MAIN) {
		lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

		if (svg->image == NULL)
			return;

		vg_lite_matrix_t matrix;
		vg_lite_identity(&matrix);
		vg_lite_translate(obj->coords.x1 + svg->pivot.x - draw_ctx->buf_area->x1,
				obj->coords.y1 + svg->pivot.y - draw_ctx->buf_area->y1, &matrix);
		vg_lite_rotate(svg->angle / 10.0, &matrix);
		vg_lite_scale((float)svg->zoom / LV_IMG_ZOOM_NONE,
				(float)svg->zoom / LV_IMG_ZOOM_NONE, &matrix);
		vg_lite_translate(-svg->pivot.x, -svg->pivot.y, &matrix);

		lv_draw_wait_for_finish(draw_ctx);

		renderSVGImage(lv_vglite_get_dest_buf(), svg->image, &matrix);

		vg_lite_finish();
	}
}

static void invalidate_image_area(lv_obj_t * obj)
{
	svg_img_t * svg = (svg_img_t *)obj;
	lv_coord_t ext_inv = (svg->zoom != LV_IMG_ZOOM_NONE) ? 1 : 0;
	lv_area_t abs_area;

	abs_area.x1 = svg->trans_area.x1 + obj->coords.x1 - ext_inv;
	abs_area.y1 = svg->trans_area.y1 + obj->coords.y1 - ext_inv;
	abs_area.x2 = svg->trans_area.x2 + obj->coords.x1 + ext_inv;
	abs_area.y2 = svg->trans_area.y2 + obj->coords.y1 + ext_inv;

	lv_obj_invalidate_area(obj, &abs_area);
}

static void refresh_image_area(lv_obj_t * obj)
{
	svg_img_t * svg = (svg_img_t *)obj;

	invalidate_image_area(obj);

	svg->trans_area.x1 = 0;
	svg->trans_area.y1 = 0;
	svg->trans_area.x2 = svg->w - 1;
	svg->trans_area.y2 = svg->h - 1;

	if (svg->angle > 0 || svg->zoom != LV_IMG_ZOOM_NONE) {
		int16_t x1, y1, x2, y2;

		/* zoom is to .8 fixedpoint */
		sw_transform_area16(&x1, &y1, &x2, &y2,
				svg->trans_area.x1, svg->trans_area.y1, svg->trans_area.x2, svg->trans_area.y2,
				svg->pivot.x, svg->pivot.y, svg->angle, svg->zoom, svg->zoom, 8);

		/* in case that LV_USE_LARGE_COORD set */
		svg->trans_area.x1 = x1;
		svg->trans_area.y1 = y1;
		svg->trans_area.x2 = x2;
		svg->trans_area.y2 = y2;

		lv_obj_refresh_ext_draw_size(obj);
	}

	invalidate_image_area(obj);
}

#endif /* CONFIG_VG_LITE */
