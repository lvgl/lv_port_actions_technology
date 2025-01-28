/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/

#define LOG_MODULE_CUSTOMER
#include <os_common_api.h>

#include <math.h>
#include <errno.h>
#include <svg_font.h>
#include <freetype_font_api.h>
#include <mem_manager.h>

LOG_MODULE_REGISTER(svgfont, LOG_LEVEL_INF);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

svgfont_t * svgfont_open(const char * font_path, uint32_t font_size)
{
#ifndef CONFIG_LVGL_USE_FREETYPE_FONT
	static uint8_t svgfont_inited = 0;
	if (svgfont_inited == 0) {
		svgfont_inited = 1;
		freetype_font_init(1);
	}
#endif

	svgfont_t *font = mem_malloc(sizeof(*font));
	if (!font)
		return NULL;

	freetype_font_t *ffont = freetype_font_open(font_path, font_size);
	if (!ffont) {
		SYS_LOG_ERR("freetype_font_open %s (size=%u) failed\n", font_path, font_size);
		mem_free(font);
		return NULL;
	}

	font->font = ffont;
	font->font_size = font_size;
	font->line_height = ffont->line_height;
	font->base_line = ffont->line_height - ffont->base_line;
	return font;
}

int svgfont_close(svgfont_t * font)
{
	if (font) {
		freetype_font_close(font->font);
		mem_free(font);
		return 0;
	}

	return -EINVAL;
}

int svgfont_set_size(svgfont_t * font, uint32_t font_size)
{
	if (font && font_size > 0) {
		freetype_font_t *ffont = font->font;
		font->font_size = font_size;
		font->line_height = (uint16_t)ceil((float)ffont->line_height * font_size / ffont->font_size);
		font->base_line = (uint16_t)round((float)(ffont->line_height - ffont->base_line) *
		                                  font_size / ffont->font_size);
		return 0;
	}

	return -EINVAL;
}

uint16_t svgfont_get_glyph_width(const svgfont_t * font, uint32_t charcode)
{
	svgfont_glyph_dsc_t g;
	if (!svgfont_get_glyph_dsc(font, &g, charcode))
		return g.adv_w;

	return 0;
}

int svgfont_get_glyph_dsc(const svgfont_t * font, svgfont_glyph_dsc_t * glyph_dsc, uint32_t charcode)
{
	freetype_font_t *ffont;
	bbox_metrics_t *metric;
	float scale = 0.0;

	if (!font)
		return -EINVAL;

	if (charcode < 0x20 || charcode >= 0x1F300)  {
		glyph_dsc->adv_w = 0;
		glyph_dsc->box_h = 0;
		glyph_dsc->box_w = 0;
		glyph_dsc->ofs_x = 0;
		glyph_dsc->ofs_y = 0;
		return -EINVAL;
	}

	ffont = font->font;

	metric = freetype_font_get_glyph_dsc(ffont, ffont->shape_cache, charcode);
	if (metric) {
		//use scale to compute metrics is not accurate, just load metric from freetype
		//if(force_bitmap_output || charcode >= 0x1F300)

		scale = (float)font->font_size / metric->metric_size;
		glyph_dsc->adv_w = (uint16_t)ceil(metric->advance * scale);
		glyph_dsc->box_h = (uint16_t)ceil(metric->bbh * scale); /*Height of the bitmap in [px]*/
		glyph_dsc->box_w = (uint16_t)ceil(metric->bbw * scale); /*Width of the bitmap in [px]*/
		glyph_dsc->ofs_x = (int16_t)ceil(metric->bbx * scale);  /*X offset of the bitmap in [pf]*/
		glyph_dsc->ofs_y = (int16_t)ceil(metric->bby * scale);  /*Y offset of the bitmap measured from the as line*/
		return 0;
	}

	return -EINVAL;
}

int svgfont_get_path_dsc(const svgfont_t * font, svgfont_path_dsc_t * path_dsc, uint32_t charcode)
{
	freetype_font_t *ffont;
	float scale = 1.0 / 64.0;
	uint8_t *data;

	if (!font)
		return -EINVAL;

	ffont = font->font;

	data = freetype_font_load_glyph_shape(ffont, ffont->shape_cache, charcode, &scale);
	if (data) {
		path_dsc->scale = scale * font->font_size / ffont->font_size / 64.0; /* TODO: set the proper scale factor */
		path_dsc->path_len = *(int16_t *)data;
		path_dsc->path_data = (int16_t *)data + 1;
		return 0;
	}

	return -EINVAL;
}

int svgfont_generate_path_and_matrix(const svgfont_t * font, svgfont_glyph_dsc_t * glyph_dsc,
		vg_lite_path_t * path, vg_lite_matrix_t * matrix, uint32_t charcode,
		float pos_x, float pos_y, bool baseline_y)
{
	freetype_font_t *ffont;
	int err;

	if (!font)
		return -EINVAL;

	ffont = font->font;

	svgfont_path_dsc_t path_dsc;
	err = svgfont_get_path_dsc(font, &path_dsc, charcode);
	if (err) {
		SYS_LOG_ERR("get glyph path failed\n");
		return -EINVAL;
	}

	vg_lite_float_t box_x1 = glyph_dsc->ofs_x / path_dsc.scale;
	vg_lite_float_t box_x2 = (float)(glyph_dsc->ofs_x + (int16_t)glyph_dsc->box_w) / path_dsc.scale;
	vg_lite_float_t box_y1 = (float)(-glyph_dsc->ofs_y - (int16_t)glyph_dsc->box_h) / path_dsc.scale;
	vg_lite_float_t box_y2 = (float)(-glyph_dsc->ofs_y) / path_dsc.scale;
	err = vg_lite_init_path(path, VG_LITE_S16, VG_LITE_HIGH,
					path_dsc.path_len, path_dsc.path_data, box_x1, box_y1, box_x2, box_y2);
	if (err != VG_LITE_SUCCESS) {
		SYS_LOG_ERR("vg_lite_init_path failed\n");
		return err;
	}

	/* matrix already initialized outside */
	vg_lite_translate(pos_x, pos_y + (baseline_y ? 0 : font->base_line), matrix);
	vg_lite_scale(path_dsc.scale, path_dsc.scale, matrix);
	return 0;
}
