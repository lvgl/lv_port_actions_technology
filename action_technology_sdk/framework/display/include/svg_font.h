/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_SVG_FONT_H_
#define FRAMEWORK_DISPLAY_INCLUDE_SVG_FONT_H_

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include <stdbool.h>
#include <vg_lite.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint16_t adv_w; /* The glyph needs this space. Draw the next glyph after this width.*/
    uint16_t box_w; /* Width of the glyph's bounding box */
    uint16_t box_h; /* Height of the glyph's bounding box */
    int16_t ofs_x;  /* x offset of the bounding box */
    int16_t ofs_y;  /* y offset of the bounding box.
	                 * (measured from bottom to top, < 0 if below baseline, > 0 if above baseline */
} svgfont_glyph_dsc_t;

typedef struct {
    float scale;         /* scale factor of the path data */
    int16_t * path_data; /* store the glyph data for VG-Lit path
	                      * (measured from top to bottom, y coord > 0 if below baseline, < 0 if above baseline ) */
    int16_t path_len;    /* length of path_data */
} svgfont_path_dsc_t;

typedef struct {
    void * font;        /* base font */
    uint32_t font_size; /* font size */
    uint16_t line_height; /* the real line height where any text fits */
    uint16_t base_line; /* base line measured from the top of the line_height */
} svgfont_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Open a svg font
 *
 * @param font_path the font path
 * @param font_size the font size
 *
 * @retval the handle of font on success else NULL.
 */
svgfont_t * svgfont_open(const char * font_path, uint32_t font_size);

/**
 * @brief Close a svg font
 *
 * @param font the handle of font returned by svgfont_open()
 *
 * @retval 0 on success else negative code.
 */
int svgfont_close(svgfont_t * font);

/**
 * @brief Set new font size
 *
 * @param font the handle of font returned by svgfont_open()
 * @param font_size new font size
 *
 * @retval 0 on success else negative code.
 */
int svgfont_set_size(svgfont_t * font, uint32_t font_size);

/**
 * @brief Get glyph width of a character
 *
 * @param font the handle of font returned by svgfont_open()
 * @param charcode the character to query
 *
 * @retval 0 on success else negative code.
 */
uint16_t svgfont_get_glyph_width(const svgfont_t * font, uint32_t charcode);

/**
 * @brief Get line height of a character
 *
 * @param font the handle of font returned by svgfont_open()
 *
 * @retval 0 on success else negative code.
 */
static inline uint16_t svgfont_get_line_height(const svgfont_t * font)
{
	return font ? font->line_height : 0;
}

/**
 * @brief Get glyph descriptor of a character
 *
 * @param font the handle of font returned by svgfont_open()
 * @param glyph_dsc pointer to the glyph descriptor to be filled
 * @param charcode the character to query
 *
 * @retval 0 on success else negative code.
 */
int svgfont_get_glyph_dsc(const svgfont_t * font, svgfont_glyph_dsc_t * glyph_dsc, uint32_t charcode);

/**
 * @brief Get path descriptor of a character
 *
 * @param font the handle of font returned by svgfont_open()
 * @param path_dsc pointer to the path descriptor to be filled
 * @param charcode the character to query
 *
 * @retval 0 on success else negative code.
 */
int svgfont_get_path_dsc(const svgfont_t * font, svgfont_path_dsc_t * path_dsc, uint32_t charcode);

/**
 * @brief Helper func to generate VG-Lite path and matrix for a character
 *
 * For example:
 *
 * static void render_text(vg_lite_buffer_t *fb, uint32_t x, uint32_t y,
 *         vg_lite_color_t color, const uint32_t letters[],
 *         uint32_t letter_count, uint8_t letter_space)
 * {
 *     svgfont_t * font = svgfont_open("vfont.ttf", 48);
 *     if (font == NULL) {
 *         printf("svgfont_open %s failed\n", "vfont.ttf");
 *         return;
 *     }
 *
 *     vg_lite_path_t path;
 *     memset(&path, 0, sizeof(path));
 *
 *     vg_lite_matrix_t matrix;
 *     vg_lite_blend_t blend_mode = ((color >> 24) == 0xff) ?
 *             VG_LITE_BLEND_NONE : VG_LITE_BLEND_PREMULTIPLY_SRC_OVER;
 *
 *     svgfont_glyph_dsc_t dsc_out = { 0 };
 *     int err;
 *
 *     for (uint32_t i = 0; i < letter_count; i++, x += letter_space + dsc_out.adv_w) {
 *         err = svgfont_get_glyph_dsc(font, &dsc_out, letters[i]);
 *         if (err) {
 *             printf("get glyph dsc failed (letter=%#x)\n", letters[i]);
 *             continue;
 *         }
 *
 *         // must re-initialize before invoking svgfont_generate_path_and_matrix()
 *         vg_lite_identity(&matrix);
 *
 *         err = svgfont_generate_path_and_matrix(font, &dsc_out, &path, &matrix, letters[i], x, y, false);
 *         if (err != VG_LITE_SUCCESS) {
 *             printf("svgfont_generate_path_and_matrix failed (letter=%#x)\n", letters[i]);
 *             continue;
 *         }
 *
 *         vg_lite_draw(fb, &path, VG_LITE_FILL_NON_ZERO, &matrix, blend_mode, color);
 *         if (err != VG_LITE_SUCCESS) {
 *             printf("vg_lite_draw failed\n");
 *         }
 *
 *         // vg_lite_finish() is not required, since the path data were stored in GPU command buffer
 *         vg_lite_clear_path(&path);
 *     }
 *
 *     vg_lite_finish();
 *     svgfont_close(font);
 * }
 *
 * @param font the handle of font returned by svgfont_open()
 * @param glyph_dsc pointer to the glyph descriptor
 * @param path pointer to the VG-Lite path to be filled
 * @param matrix pointer to the VG-Lite matrix to be adjusted. matrix should be
 *               initialized by vg_lite_identity() or filled with some global matrix first.
 * @param charcode the character to query
 * @param pos_x the x pos of the character in the target buffer. Set 0 if considered already in matrix
 * @param pos_y the y pos of the character in the target buffer. Set 0 if considered already in matrix
 * @param baseline_y indicate the y pos is measured by the base_line on true else the top of line_height
 *
 * @retval 0 on success else negative code.
 */
int svgfont_generate_path_and_matrix(const svgfont_t * font, svgfont_glyph_dsc_t * glyph_dsc,
        vg_lite_path_t * path, vg_lite_matrix_t * matrix, uint32_t charcode,
        float pos_x, float pos_y, bool baseline_y);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_SVG_FONT_H_ */
