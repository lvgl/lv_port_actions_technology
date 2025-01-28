/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file font memory interface 
 */

#ifndef FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_
#define FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_

/**
 * @defgroup view_cache_apis View Cache APIs
 * @ingroup system_apis
 * @{
 */

#include <stdint.h>

#ifndef CONFIG_SIMULATOR
#include <sdfs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief get max fonts num
 */
uint32_t bitmap_font_get_max_fonts_num(void);

/**
 * @brief get font cache size
 *
 * @retval N/A
 */
uint32_t bitmap_font_get_font_cache_size(void);

/**
 * @brief get max emoji num
 *
 *
 * @retval N/A
 */
uint32_t bitmap_font_get_max_emoji_num(void);


uint32_t bitmap_font_get_cmap_cache_size(void);

/**
 * @brief dump cache size info
 *
 */
void bitmap_font_cache_info_dump(void);

/**
 * @brief get high freq config status
 *
 */
int bitmap_font_get_high_freq_enabled(void);


int bitmap_font_glyph_debug_is_on(void);

int bitmap_font_glyph_err_print_is_on(void);
void* bitmap_font_cache_malloc(uint32_t size);
void bitmap_font_cache_free(void* ptr);
void bitmap_font_cache_init(void);
uint32_t bitmap_font_cache_get_size(void* ptr);
void bitmap_font_cache_dump_info(void);

void bitmap_font_get_decompress_param(int bmp_size, int font_size, int* in_size, int* line_size);
int freetype_font_get_max_face_num(void);
int freetype_font_get_max_size_num(void);
int freetype_font_get_max_ftccache_bytes(void);
uint32_t freetype_font_get_font_cache_size(void);
int freetype_font_get_font_fixed_bpp(void);
int freetype_font_get_memory_face_enabled(void);
int freetype_font_enable_subpixel(void);
int freetype_font_use_svg_path(void);
int freetype_font_get_max_vertices(void);

int emoji_font_use_mmap(void);

void* freetype_font_shape_cache_malloc(uint32_t size);
void freetype_font_shape_cache_free(void* ptr);
int freetype_font_get_shape_info_size(void);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* FRAMEWORK_DISPLAY_INCLUDE_FONT_MEMPOOL_H_ */


