#ifndef _LV_FONT_FREETYPE_H
#define _LV_FONT_FREETYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl.h>
#include "freetype_font_api.h"
#ifdef CONFIG_BITMAP_FONT
#include "bitmap_font_api.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
 
typedef struct {
	freetype_font_t*  font;      /* handle to face object */
	freetype_cache_t* cache;
//	uint32_t font_size;
#ifdef CONFIG_BITMAP_FONT
	bitmap_emoji_font_t* emoji_font;
#endif
}lv_font_fmt_freetype_dsc_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
 
int lvgl_freetype_font_init(void);
int lvgl_freetype_font_open(lv_font_t* font, const char * font_path, uint32_t font_size);
void lvgl_freetype_font_close(lv_font_t* font);
int lvgl_freetype_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path);
int lvgl_freetype_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_image_dsc_t* dsc, lv_point_t* pos, bool force_retrieve);
int lvgl_freetype_font_set_default_code(lv_font_t* font, uint32_t word_code, uint32_t emoji_code);
int lvgl_freetype_font_set_default_bitmap(lv_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp);
int lvgl_freetype_font_cache_preset(freetype_font_cache_preset_t* preset, int count);
void lvgl_freetype_force_bitmap(lv_font_t* font, int enable);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_BITMAP_H*/
