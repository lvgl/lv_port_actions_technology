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

#ifdef CONFIG_UI_MANAGER
#  include <ui_mem.h>
#endif
#ifdef CONFIG_FREETYPE_FONT
#ifdef CONFIG_SD_FS
#  include <sdfs.h>
#endif
#  include <ft2build.h>
#  include FT_BBOX_H
#  include FT_OUTLINE_H
#  include <freetype_font_api.h>
#endif

#include "svg_img.h"
#include <lvgl/lvgl_ext.h>
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &svg_img_class

/**********************
 *      TYPEDEFS
 **********************/

/** Data of rotate image */
typedef struct {
	lv_obj_t obj;

	struct NSVGimage *image;
	int w;
	int h;

	lv_area_t trans_area; /* image transformed area (relative to the top-left of the object) */
	lv_point_t pivot;     /* Rotation center of the image, relative to the top-left corner of the image */
	uint16_t rotation;    /* Rotation angle in 0.1 degree [0, 3600) of the image */
	uint16_t scale;

#ifdef CONFIG_FREETYPE_FONT
	void * ft_face;
#endif
} svg_img_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void svg_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void svg_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void svg_img_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void draw_img(lv_event_t * e);
static void draw_vector_custom_cb(lv_draw_task_t * task, lv_layer_t * layer, void * user_data);

static void invalidate_image_area(lv_obj_t * svg);
static void refresh_image_area(lv_obj_t * svg);

static lv_result_t parse_svg_input(lv_obj_t *obj, char * svg_input);

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
	.base_class = &lv_obj_class,
	.name = "svg-img",
};

#ifdef CONFIG_FREETYPE_FONT
static FT_Library g_ft_library;
#endif

/**********************
 *      MACROS
 **********************/
#ifdef CONFIG_UI_MANAGER
#  define SVG_MALLOC(size)        ui_mem_alloc(MEM_RES, size, __func__)
#  define SVG_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#else
#  define SVG_MALLOC(size)        lv_malloc(size)
#  define SVG_FREE(ptr)           lv_free(ptr)
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

lv_result_t svg_img_parse(lv_obj_t *obj, const void * data, size_t len)
{
	lv_result_t res;

	LV_ASSERT_OBJ(obj, MY_CLASS);

	if (data == NULL || len == 0)
		return LV_RESULT_INVALID;

	char * svg_input = SVG_MALLOC(len + 1);
	if (svg_input == NULL) {
		LV_LOG_ERROR("malloc %d failed", len + 1);
		return LV_RESULT_INVALID;
	}

	lv_memcpy(svg_input, data, len);
	svg_input[len] = '\0';

	res = parse_svg_input(obj, svg_input);

	SVG_FREE(svg_input);

	return res;
}

lv_result_t svg_img_parse_file(lv_obj_t *obj, const char * filename)
{
	lv_result_t res = LV_RESULT_INVALID;
	lv_fs_file_t file;
	uint32_t len = 0;

	LV_ASSERT_OBJ(obj, MY_CLASS);

	if (filename == NULL)
		return LV_RESULT_INVALID;

	if (LV_FS_RES_OK != lv_fs_open(&file, filename, LV_FS_MODE_RD)) {
		LV_LOG_ERROR("%s not found", filename);
		return LV_RESULT_INVALID;
	}

	lv_fs_seek(&file, 0, LV_FS_SEEK_END);

	lv_fs_tell(&file, &len);
	if (len <= 0) {
		LV_LOG_ERROR("%s empty", filename);
		goto out_exit;
	}

	char * svg_input = SVG_MALLOC(len + 1);
	if (svg_input == NULL) {
		LV_LOG_ERROR("malloc %ld failed", (long int)(len + 1));
		goto out_exit;
	}

	lv_fs_seek(&file, 0, LV_FS_SEEK_SET);
	lv_fs_read(&file, svg_input, len, NULL);

	svg_input[len] = '\0';

	res = parse_svg_input(obj, svg_input);

	SVG_FREE(svg_input);

out_exit:
	lv_fs_close(&file);
	return res;
}

lv_result_t svg_img_set_font(lv_obj_t *obj, const char * filename)
{
#ifdef CONFIG_FREETYPE_FONT
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (g_ft_library == NULL) {
		g_ft_library = freetype_font_init(0);
		if (g_ft_library == NULL)
			return LV_RESULT_INVALID;
	}

	if (svg->ft_face) {
		FT_Done_Face(svg->ft_face);
		svg->ft_face = NULL;
	}

	void *font_faddr = NULL;
	int font_fsize = 0;
	int err = -1;

#ifdef CONFIG_SD_FS
	err = sd_fmap(filename, &font_faddr, &font_fsize);
#endif

	if (err == 0) {
		err = FT_New_Memory_Face(g_ft_library, font_faddr, font_fsize, 0, (FT_Face *)&svg->ft_face);
	} else {
		err = FT_New_Face(g_ft_library, filename, 0, (FT_Face *)&svg->ft_face);
	}

	if (err == 0) {
		FT_Set_Pixel_Sizes(svg->ft_face, 1, 0);
		return LV_RESULT_OK;
	}

	return LV_RESULT_INVALID;

#else
	return LV_RESULT_OK;
#endif /* CONFIG_FREETYPE_FONT */
}

void svg_img_set_pivot(lv_obj_t * obj, lv_coord_t pivot_x, lv_coord_t pivot_y)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (svg->pivot.x != pivot_x || svg->pivot.y != pivot_y) {
		svg->pivot.x = pivot_x;
		svg->pivot.y = pivot_y;

		if (svg->image && (svg->rotation > 0 || svg->scale != LV_SCALE_NONE))
			refresh_image_area(obj);
	}
}

void svg_img_set_rotation(lv_obj_t *obj, int16_t angle)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	while (angle >= 3600) angle -= 3600;
	while (angle < 0) angle += 3600;

	if (angle != svg->rotation) {
		svg->rotation = angle;

		if (svg->image)
			refresh_image_area(obj);
	}
}

void svg_img_set_scale(lv_obj_t * obj, uint16_t zoom)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	if (zoom == 0) zoom = 1;

	if (zoom != svg->scale) {
		svg->scale = zoom;

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

uint16_t svg_img_get_rotation(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	return svg->rotation;
}

uint16_t svg_img_get_scale(lv_obj_t * obj)
{
	LV_ASSERT_OBJ(obj, MY_CLASS);

	svg_img_t * svg = (svg_img_t *)obj;

	return svg->scale;
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
	svg->scale = LV_SCALE_NONE;

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(obj, LV_OBJ_FLAG_ADV_HITTEST);

	LV_TRACE_OBJ_CREATE("finished");
}

static void svg_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);

	svg_img_t * svg = (svg_img_t *)obj;
	if (svg->image)
		deleteSVGImage(svg->image);

#ifdef CONFIG_FREETYPE_FONT
	if (svg->ft_face)
		FT_Done_Face(svg->ft_face);
#endif
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
		lv_result_t res = lv_obj_event_base(MY_CLASS, e);
		if (res != LV_RESULT_OK) return;
	}

	if (code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
		if (svg->rotation > 0 || svg->scale != LV_SCALE_NONE) {
			lv_coord_t * s = lv_event_get_param(e);
			lv_coord_t w = lv_obj_get_width(obj);
			lv_coord_t h = lv_obj_get_height(obj);

			*s = LV_MAX(*s, -svg->trans_area.x1);
			*s = LV_MAX(*s, -svg->trans_area.y1);
			*s = LV_MAX(*s, svg->trans_area.x2 - w + 1);
			*s = LV_MAX(*s, svg->trans_area.y2 - h + 1);
		}
	} else if (code == LV_EVENT_HIT_TEST) {
		lv_hit_test_info_t * info = lv_event_get_param(e);

		if (svg->rotation > 0 || svg->scale != LV_SCALE_NONE) {
			lv_area_t coords;
			coords.x1 = svg->trans_area.x1 + obj->coords.x1;
			coords.y1 = svg->trans_area.y1 + obj->coords.y1;
			coords.x2 = svg->trans_area.x2 + obj->coords.x1;
			coords.y2 = svg->trans_area.y2 + obj->coords.y1;

			info->res = lv_area_is_point_on(&coords, info->point, 0);
		} else {
            lv_area_t a;
            lv_obj_get_click_area(obj, &a);
            info->res = lv_area_is_point_on(&a, info->point, 0);
		}
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
		lv_cover_check_info_t * info = lv_event_get_param(e);
		if (info->res == LV_COVER_RES_MASKED) return;

		if (svg->image == NULL || svg->rotation != 0) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}

		lv_area_t coords;
		lv_area_set(&coords, svg->trans_area.x1 + obj->coords.x1,
					svg->trans_area.y1 + obj->coords.y1,
					svg->trans_area.x2 + obj->coords.x1,
					svg->trans_area.y2 + obj->coords.y1);

		if (lv_area_is_in(info->area, &coords, 0) == false) {
			info->res = LV_COVER_RES_NOT_COVER;
			return;
		}
	} else if (code == LV_EVENT_DRAW_MAIN) {
		if (svg->image == NULL)
			return;

		lv_layer_t * layer = lv_event_get_layer(e);
		lv_draw_vector_custom_dsc_t draw_dsc;
		lv_draw_vector_custom_dsc_init(&draw_dsc);
		draw_dsc.exec_cb = draw_vector_custom_cb;
		draw_dsc.user_data = obj;
		lv_draw_vector_custom(layer, &draw_dsc, NULL);
	}
}

static void draw_vector_custom_cb(lv_draw_task_t * task, lv_layer_t * layer, void * user_data)
{
	lv_obj_t * obj = user_data;
	svg_img_t * svg = (svg_img_t *)obj;

	vg_lite_matrix_t matrix;
	vg_lite_identity(&matrix);
	vg_lite_translate(obj->coords.x1 + svg->pivot.x - layer->buf_area.x1,
			obj->coords.y1 + svg->pivot.y - layer->buf_area.y1, &matrix);
	vg_lite_rotate(svg->rotation / 10.0, &matrix);
	vg_lite_scale((float)svg->scale / LV_SCALE_NONE,
			(float)svg->scale / LV_SCALE_NONE, &matrix);
	vg_lite_translate(-svg->pivot.x, -svg->pivot.y, &matrix);

	vg_lite_buffer_t fb;
	lvx_vglite_map_draw_buf(&fb, layer->draw_buf);

#ifdef CONFIG_FREETYPE_FONT
	renderSVGImage(svg->image, &fb, &matrix, 1.0f, svg->ft_face);
#else
	renderSVGImage(svg->image, &fb, &matrix, 1.0f, NULL);
#endif
}

static void invalidate_image_area(lv_obj_t * obj)
{
	svg_img_t * svg = (svg_img_t *)obj;
	lv_area_t abs_area;

	abs_area.x1 = svg->trans_area.x1 + obj->coords.x1;
	abs_area.y1 = svg->trans_area.y1 + obj->coords.y1;
	abs_area.x2 = svg->trans_area.x2 + obj->coords.x1;
	abs_area.y2 = svg->trans_area.y2 + obj->coords.y1;

	lv_obj_invalidate_area(obj, &abs_area);
}

static void refresh_image_area(lv_obj_t * obj)
{
	svg_img_t * svg = (svg_img_t *)obj;

	lv_obj_update_layout(obj);  /*Be sure the object's size is calculated*/
	invalidate_image_area(obj);

	if (svg->rotation > 0 || svg->scale != LV_SCALE_NONE) {
		lv_image_buf_get_transformed_area(&svg->trans_area, svg->w,
				svg->h, svg->rotation, svg->scale, svg->scale, &svg->pivot);
	} else {
		svg->trans_area.x1 = 0;
		svg->trans_area.y1 = 0;
		svg->trans_area.x2 = svg->w - 1;
		svg->trans_area.y2 = svg->h - 1;
	}

	/* Disable invalidations because lv_obj_refresh_ext_draw_size would invalidate
	 * the whole ext draw area */
	lv_display_t * disp = lv_obj_get_display(obj);
	lv_display_enable_invalidation(disp, false);
	lv_obj_refresh_ext_draw_size(obj);
	lv_display_enable_invalidation(disp, true);

	invalidate_image_area(obj);
}

static lv_result_t parse_svg_input(lv_obj_t *obj, char * svg_input)
{
	svg_img_t * svg = (svg_img_t *)obj;

	if (svg->image)
		deleteSVGImage(svg->image);

	svg->image = parseSVGImage(svg_input, &svg->w, &svg->h);
	if (svg->image == NULL) {
		SVG_FREE(svg_input);
		return LV_RESULT_INVALID;
	}

	lv_obj_invalidate(obj);
	lv_obj_refresh_self_size(obj);

	svg->pivot.x = svg->w / 2;
	svg->pivot.y = svg->h / 2;
	refresh_image_area(obj);

	return LV_RESULT_OK;
}

#endif /* CONFIG_VG_LITE */
