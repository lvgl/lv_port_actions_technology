#ifndef __FREETYPE_FONT_API_H__
#define __FREETYPE_FONT_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

//#include <fs/fs.h>
#include <stdint.h>

/*
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
*/

/*********************
 *      DEFINES
 *********************/
//#define MAX_CACHE_NUM		80
#ifndef CONFIG_SIMULATOR
#define MAX_PATH_LEN	24
#else
#define MAX_PATH_LEN	256
#endif

/**********************
 *      TYPEDEFS
 **********************/
enum {
	FT_vmove = 1,
	FT_vline,
	FT_vcurve,
	FT_vcubic
};

typedef struct {
	int x, y, cx, cy, cx1, cy1; /* 26.6 fixed-point pixel coordinates */
	//uint8_t type, padding;
	uint32_t type;
} freetype_vertex_t;

typedef struct {
	int16_t bbox_x1;
	int16_t bbox_y1;
	int16_t bbox_x2;
	int16_t bbox_y2;
	uint16_t num_vertices;
	freetype_vertex_t * vertices;
//	uint32_t glyf_id;
//	uint16_t current_size;	
} freetype_glyphshape_t;

typedef struct {
	float* svg_path_data;
	uint16_t path_len;
	uint16_t current_size;			
} freetype_svg_path_t;

typedef struct
{
	uint32_t advance;
	uint32_t bbw;
	uint32_t bbh;
	int32_t bbx;
	int32_t bby;
	int32_t metric_size;
}bbox_metrics_t;

typedef struct
{
	uint32_t current;
	uint32_t next;
	uint32_t cached_total;
	uint32_t* glyph_index;
	bbox_metrics_t* metrics;
	uint8_t* data;
	bbox_metrics_t default_metric;
	uint8_t* default_data;
	uint32_t inited;
	uint32_t unit_size;
	uint32_t cached_max;
	uint32_t last_unicode;
	uint32_t last_glyph_idx;
	int32_t last_idx;
	int is_shape_cache;
}freetype_cache_t;


typedef struct 
{
	void*	face;
	uint8_t font_path[MAX_PATH_LEN];
	int cmap_id;
	uint32_t face_ref;
	freetype_cache_t shape_cache;
	uint16_t current_size;
}internal_ft_face_t;


typedef struct
{
	internal_ft_face_t* internal_face;
	void* size;
	freetype_cache_t* cache;
	freetype_cache_t* shape_cache;
	void* image_type;
	uint16_t font_size;
	int16_t ascent;
	int16_t descent;
	uint16_t default_advance;
	uint32_t line_height;
	int32_t base_line;
	uint32_t max_advance;
	uint32_t ref_count;
	uint32_t default_code;
	int bpp;
	float scale;
}freetype_font_t;

typedef struct
{
    char* font_path;
	uint32_t font_size;
    int cache_size_preset;
}freetype_font_cache_preset_t;



/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * open or ref FT_Library for lvgl font api, init font cache
 *
 * @param with_lvgl_api		1 if use freetype with lvgl font api, 0 if use freetype independently
 * @retval FT_Library value on success, NULL on failed attempt
 */
void* freetype_font_init(int with_lvgl_api);

/**
 * deref or Done FT_Library for lvgl font api, release resources
 *
 * @param with_lvgl_api		1 if use freetype with lvgl font api, 0 if use freetype independently
 * @retval N/A
 */
int freetype_font_deinit(int with_lvgl_api);

/**
 * open truetype font
 *
 * @param file_path path of the font file
 * @param font_size font_size to be set
 *
 * @retval pointer to the opened font, NULL for failed attempt
 */
freetype_font_t* freetype_font_open(const char* file_path, uint32_t font_size);

/**
 * close opened truetype font
 *
 * @param font font pointer to previously opened truetype font file.
 *
 * @retval N/A
 */
void freetype_font_close(freetype_font_t* font);

/**
 * obtain glyph bitmap of given unicode for lvgl font api
 *
 * @param font opened font pointer
 * @param cache pointer to cache
 * @param unicode unicode for the glyph
 *
 * @retval pointer to the bitmap data, NULL on failed attempt
 */
uint8_t * freetype_font_get_bitmap(freetype_font_t* font, freetype_cache_t* cache, uint32_t unicode);

/**
 * obtain glyph metrics of given unicode for lvgl font api
 *
 * @param font opened font pointer
 * @param cache pointer to cache
 * @param unicode unicode for the glyph
 *
 * @retval pointer to the glyph metric data, NULL on failed attempt
 */
bbox_metrics_t* freetype_font_get_glyph_dsc(freetype_font_t* font, freetype_cache_t *cache, uint32_t unicode);

/**
 * obtain cache pointer
 *
 * @param font opened font pointer
 *
 * @retval pointer to the cache
 */
freetype_cache_t* freetype_font_get_cache(freetype_font_t* font);

/**
 * set a default unicode to replace the non-exist code in the font file on display
 *
 * @param font opened font pointer
 * @param letter_code unicode of the default code
 *
 * @retval 0 on succusess, negative when failed
 */
int freetype_font_set_default_code(freetype_font_t* font, uint32_t letter_code);

/**
 * set a default bitmap data to replace the non-exist code in the font file on display
 *
 * @param font opened font pointer
 * @param bitmap bitmap data
 * @param width bitmap width
 * @param height bitmap height
 * @param gap gap size on the right side
 * @param bpp bpp of the bitmap
 *
 * @retval 0 on succusess, negative when failed
 */
int freetype_font_set_default_bitmap(freetype_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp);

/**
 * store the presetted font cache config
 *
 * @param preset a list of font cache size for different font sizes
 * @param count item num in the list
 *
 * @retval 0 on success, negative when failed
 */
int freetype_font_cache_preset(freetype_font_cache_preset_t* preset, int count);

/**
 * obtain glyph shape info of given unicode for lvgl font api
 *
 * @param font opened font pointer
 * @param cache pointer to cache
 * @param unicode unicode for the glyph
 * @param pshape output param to store shape info
 *
 * @retval 0 on success, negative on failed attempt
 */
uint8_t* freetype_font_load_glyph_shape(freetype_font_t* font, freetype_cache_t *cache, uint32_t unicode, float* scale);

/**
 * incomplete
 *
 * @param shape pointer to shape info
 *
 * @retval N/A
 */
void freetype_font_free_glyph_shape(freetype_glyphshape_t *shape);

/**********************
 *      MACROS
 **********************/
 
#ifdef __cplusplus
} /* extern "C" */
#endif	
#endif /*__FREETYPE_FONT_API_H__*/
