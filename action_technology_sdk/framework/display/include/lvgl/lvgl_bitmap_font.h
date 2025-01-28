#ifndef _LV_FONT_BITMAP_H
#define _LV_FONT_BITMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <lvgl/lvgl.h>
#include <bitmap_font_api.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
 
typedef struct {
    bitmap_font_t*  font;      /* handle to face object */
	bitmap_cache_t* cache;
	bitmap_emoji_font_t* emoji_font;
}lv_font_fmt_bitmap_dsc_t;


/**********************
 * GLOBAL PROTOTYPES
 **********************/
 
int lvgl_bitmap_font_init(const char *def_font_path);
int lvgl_bitmap_font_deinit(void);
int lvgl_bitmap_font_open(lv_font_t* font, const char * font_path);
void lvgl_bitmap_font_close(lv_font_t* font);
int lvgl_bitmap_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path);
int lvgl_bitmap_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_image_dsc_t* dsc, lv_point_t* pos, bool force_retrieve);
/**
* @brief set default code point for bitmap font
*
* @param font pointer to font data
* @param word_code default code for unicode basic plane(0x0-0xffff)
* @param emoji_code default code for emoji (>= 0x1f300)
*
* @return 0 if invoked succsess.
*/
int lvgl_bitmap_font_set_default_code(lv_font_t* font, uint32_t word_code, uint32_t emoji_code);

/**
* @brief preset cache size for each font
*
* @param preset pointer to preset data list
* @param count item num in preset list
*
* @return 0 if invoked succsess.
*/
int lvgl_bitmap_font_cache_preset(bitmap_font_cache_preset_t* preset, int count);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_FONT_BITMAP_H*/
