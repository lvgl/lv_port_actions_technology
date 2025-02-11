#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../../font_mempool.h"
#include "freetype_font_api.h"
#ifndef CONFIG_SIMULATOR
#include <os_common_api.h>
#include "ft2build.h"
#include <freetype/ftoutln.h>
#include <freetype/ftbbox.h>
#else
#include "..\ft2build.h"
#endif
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_SIZES_H
#ifdef CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
#include <vg_lite/vg_lite.h>
#endif

#define FT_FONT_USE_PRESET_CACHE	1

#ifdef CONFIG_SIMULATOR
#define SYS_LOG_DBG
#define SYS_LOG_INF	printf
#define SYS_LOG_ERR printf
#endif

#define SVG_BASE_SIZE				128

typedef struct{
	uint32_t size;
	void* vertices;
}svg_path_cache_unit_t;

typedef struct{
	FT_Face face;
	FT_Size sizep;
	int size;
	int ref;
}size_info_t;

static FT_Library ft_library;
static uint8_t ft_lib_ref = 0;

#ifndef FT_FONT_USE_PRESET_CACHE
static FTC_Manager ft_cache_manager;
static FTC_SBitCache ft_sbit_cache;
#endif

static freetype_font_t* opend_font;
//static bbox_metrics_t common_metric;
static int max_fonts;
static freetype_cache_t* ft_font_cache;

static int max_faces;
static int max_sizes;
//static int max_ftccache_bytes;
static internal_ft_face_t* common_faces;

uint32_t _ft_get_glyf_id_cached(freetype_font_t* font, uint32_t unicode);

static int shape_cache_debug_size = 0;
static int shape_cache_peak_size = 0;

static int default_font_cache_size;
static freetype_font_cache_preset_t* font_cache_presets=NULL;
static int font_cache_preset_num=0;

static uint8_t* font_file_addr = NULL;
static int font_file_size = 0;

uint8_t* gray_pool_buffer = NULL;

static freetype_glyphshape_t s_ft_shape;
size_info_t* s_common_sizes = NULL;

static void * ttf_generate_svg_path(freetype_font_t* font, freetype_glyphshape_t* shape, float* scale, uint16_t* path_len);

static uint32_t _find_cache_bytes_for_size(const char* file_path, uint32_t font_size)
{
    int i;
    if(font_cache_presets == NULL)
    {
        return default_font_cache_size;
    }

    for(i=0;i<font_cache_preset_num;i++)
    {
        if(font_cache_presets[i].font_path && font_cache_presets[i].font_size == font_size)
        {
            if(strcmp(font_cache_presets[i].font_path, file_path)==0)
            {
                SYS_LOG_INF("preset font cache size %s, %d\n", file_path, font_cache_presets[i].cache_size_preset);
                return font_cache_presets[i].cache_size_preset;
            }
        }
    }

    SYS_LOG_INF("no preset found for %s, size %d\n", file_path, font_size);
	return default_font_cache_size;
}


void* freetype_font_init(int with_lvgl_api)
{
	FT_Error error;

	bitmap_font_cache_init();

	if(with_lvgl_api)
	{
		max_fonts = freetype_font_get_max_size_num();

		default_font_cache_size = freetype_font_get_font_cache_size();

		max_faces = freetype_font_get_max_face_num();
		max_sizes = max_fonts;

		ft_font_cache = (freetype_cache_t*)bitmap_font_cache_malloc(max_fonts*sizeof(freetype_cache_t));
		if(ft_font_cache == NULL)
		{
			SYS_LOG_ERR("freetype font init failed due to unable to malloc ft_font_cache data\n");
			goto init_failed;
		}	
		memset(ft_font_cache, 0, max_fonts*sizeof(freetype_cache_t));

		opend_font = (freetype_font_t*)bitmap_font_cache_malloc(max_fonts*sizeof(freetype_font_t));
		if(opend_font == NULL)
		{
			SYS_LOG_ERR("freetype font init failed due to unable to malloc opend_font data\n");
			goto init_failed;
		}
		memset(opend_font, 0, sizeof(freetype_font_t)*max_fonts);

		if(freetype_font_use_svg_path())
		{
			common_faces = (internal_ft_face_t*)bitmap_font_cache_malloc(max_faces*sizeof(internal_ft_face_t));
			if(common_faces == NULL)
			{
				SYS_LOG_ERR("freetype font init failed due to unable to malloc common faces data\n");
				goto init_failed;
			}
			memset(common_faces, 0, sizeof(internal_ft_face_t)*max_faces);
		}

		if(freetype_font_use_svg_path())
		{
			memset(&s_ft_shape, 0, sizeof(freetype_glyphshape_t));
			s_ft_shape.vertices = freetype_font_shape_cache_malloc(freetype_font_get_max_vertices()*sizeof(freetype_vertex_t));
			if(s_ft_shape.vertices == NULL)
			{
				SYS_LOG_ERR("freetype font init failed when malloc common shape vertices\n");
				goto init_failed;
			}
			shape_cache_debug_size += freetype_font_get_max_vertices()*sizeof(freetype_vertex_t);
			if(shape_cache_debug_size > shape_cache_peak_size)
			{
				shape_cache_peak_size = shape_cache_debug_size;
			}
		}

		s_common_sizes = bitmap_font_cache_malloc(sizeof(size_info_t)*freetype_font_get_max_size_num());
		if(!s_common_sizes)
		{
			SYS_LOG_ERR("malloc size obj failed\n");
			goto init_failed;
		}
		memset(s_common_sizes, 0, sizeof(size_info_t)*freetype_font_get_max_size_num());
	}

	if(ft_lib_ref == 0)
	{
		if(!gray_pool_buffer)
		{
			gray_pool_buffer = (uint8_t*)bitmap_font_cache_malloc(FT_RENDER_POOL_SIZE);
			if(!gray_pool_buffer)
			{
				SYS_LOG_INF("malloc gray_pool_buffer failed\n");
				goto init_failed;
			}
		}

		error = FT_Init_FreeType(&ft_library);
		if ( error )
		{
			SYS_LOG_INF("Error in FT_Init_FreeType: %d\n", error);
			goto init_failed;
		}
		ft_lib_ref++;
	}
	else
	{
		ft_lib_ref++;
	}

	SYS_LOG_DBG("render pool size init %ld\n", FT_RENDER_POOL_SIZE);
	SYS_LOG_INF("freetype_font_init ok\n");
	return (void*)ft_library;

init_failed:
	if(with_lvgl_api)
	{
		if(ft_font_cache)
		{
			bitmap_font_cache_free(ft_font_cache);
			ft_font_cache = NULL;
		}

		if(opend_font)
		{
			bitmap_font_cache_free(opend_font);
			opend_font = NULL;
		}

		if(common_faces)
		{
			bitmap_font_cache_free(common_faces);
			common_faces = NULL;
		}

		if(s_ft_shape.vertices)
		{
			freetype_font_shape_cache_free(s_ft_shape.vertices);
			memset(&s_ft_shape, 0, sizeof(freetype_glyphshape_t));
		}

		if(s_common_sizes)
		{
			bitmap_font_cache_free(s_common_sizes);
			s_common_sizes = NULL;
		}
	}	

	if(gray_pool_buffer)
	{
		bitmap_font_cache_free(gray_pool_buffer);
		gray_pool_buffer = NULL;
	}

	return NULL;
}

int freetype_font_deinit(int with_lvgl_api)
{
	int32_t i;

	if(with_lvgl_api)
	{
		for(i=0;i<max_fonts;i++)
		{
			if(opend_font[i].internal_face != NULL)
			{
				freetype_font_close(&opend_font[i]);
				memset(&opend_font[i], 0, sizeof(freetype_font_t));
			}
			
			if(ft_font_cache[i].inited == 1)
			{
				ft_font_cache[i].inited = 0;
			}		
			
		}

		if(s_common_sizes)
		{
			bitmap_font_cache_free(s_common_sizes);
			s_common_sizes = NULL;
		}

		if(ft_font_cache)
		{
			bitmap_font_cache_free(ft_font_cache);
		}
		if(opend_font)
		{
			bitmap_font_cache_free(opend_font);
		}	
		if(common_faces)
		{
			bitmap_font_cache_free(common_faces);
		}

		if(freetype_font_use_svg_path())
		{
			if(s_ft_shape.vertices)
			{
				freetype_font_shape_cache_free(s_ft_shape.vertices);
			}
			memset(&s_ft_shape, 0, sizeof(freetype_glyphshape_t));
		}

#ifdef FT_FONT_USE_PRESET_CACHE
		if(font_cache_presets != NULL)
		{
			for(i=0;i<font_cache_preset_num;i++)
			{
				if(font_cache_presets[i].font_path != NULL)
				{
					bitmap_font_cache_free(font_cache_presets[i].font_path);
				}
			}
			bitmap_font_cache_free(font_cache_presets);
		}
#endif
	}

	if(ft_lib_ref > 1)
	{
		ft_lib_ref--;
	}
	else
	{
		FT_Done_FreeType(ft_library);
		ft_library = NULL;
		ft_lib_ref = 0;
	
		if(gray_pool_buffer)
		{
			bitmap_font_cache_free(gray_pool_buffer);
			gray_pool_buffer = NULL;
		}
	}

	return 0;
}

freetype_cache_t* freetype_font_get_cache(freetype_font_t* font)
{
	return font->cache;
}

void freetype_font_dump_info(void)
{

}

int freetype_font_set_default_code(freetype_font_t* font, uint32_t letter_code)
{
	if(font)
	{
		font->default_code = letter_code;
	}

	return 0;
}

int freetype_font_set_default_bitmap(freetype_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp)
{
	bbox_metrics_t* metric_item = NULL;
	uint8_t* data = NULL;
	uint32_t bmp_size = 0;
	int multi = 0;
	int cache_index;
	
	if(font)
	{	
		if(font->cache->default_data != NULL)
		{
			return 0;
		}
	
		if(bitmap)
		{
			metric_item = &font->cache->default_metric;
			metric_item->advance = width +gap;
			metric_item->bbw = width;
			metric_item->bbh = height;
			metric_item->bbx = 0;
			metric_item->bby = 0;

			//bmp_size = metric_item->bbw*metric_item->bbh;
			uint32_t stride = (metric_item->bbw * font->bpp +7)/8;
			bmp_size = stride*metric_item->bbh;			

			multi = bmp_size/font->cache->unit_size + 1;
			
			cache_index = font->cache->cached_max-multi;
			font->cache->default_data = &(font->cache->data[cache_index*font->cache->unit_size]);
			data = font->cache->default_data; 
			memcpy(data, bitmap, bmp_size);	
			font->cache->cached_max -= multi;			
		}
		else
		{
			metric_item = &font->cache->default_metric;
			metric_item->advance = font->font_size;
			metric_item->bbw = font->font_size;
			metric_item->bbh = font->font_size;
			metric_item->bbx = 0;
			metric_item->bby = 0;

			//bmp_size = metric_item->bbw*metric_item->bbh;
			uint32_t stride = (metric_item->bbw * font->bpp +7)/8;
			bmp_size = stride*metric_item->bbh;				

			multi = bmp_size/font->cache->unit_size + 1;
			
			cache_index = font->cache->cached_max-multi;
			font->cache->default_data = &(font->cache->data[cache_index*font->cache->unit_size]);
			data = font->cache->default_data; 
			memset(data, 0, bmp_size);	

			font->cache->cached_max -= multi;
		}
	}

	return 0;
}


uint32_t _ft_get_glyf_id_cached(freetype_font_t* font, uint32_t unicode)
{
	uint32_t glyf_id;
	freetype_cache_t* cache = font->cache;

	if(unicode == cache->last_unicode)
	{
		glyf_id = cache->last_glyph_idx;
		
	}
	else
	{
		glyf_id = FT_Get_Char_Index( font->internal_face->face, unicode );
		if(glyf_id > 0)
		{
			cache->last_unicode = unicode;
			cache->last_glyph_idx = glyf_id;
		}
	}

	return glyf_id;
}

internal_ft_face_t* _ft_get_internal_face(const char* file_path)
{
	int face_idx = -1;
	int i;

	for(i=0;i<max_faces;i++)
	{
		if(strcmp(common_faces[i].font_path, file_path) == 0)
		{
			//found face
			return &common_faces[i];
		}
		else if(face_idx < 0 && (common_faces[i].face == NULL || common_faces[i].face_ref == 0))
		{
			face_idx = i;
		}
	}

	if(face_idx < 0)	
	{
		SYS_LOG_ERR("no available face slot\n");
		return NULL;
	}

	if(common_faces[face_idx].face != NULL)
	{
		SYS_LOG_DBG("%s %d done common face %p\n", __FILE__, __LINE__, common_faces[face_idx].face);
		FT_Done_Face(common_faces[face_idx].face);
		common_faces[face_idx].face = NULL;
		if(freetype_font_use_svg_path())
		{
			int i;
			freetype_cache_t* cache = &common_faces[face_idx].shape_cache;
			freetype_svg_path_t* paths = (freetype_svg_path_t*)cache->data;
			if(cache->inited)
			{
				if(cache->glyph_index)
				{
					for(i=0;i<cache->cached_max;i++)
					{
						if(paths[i].svg_path_data)
						{
							freetype_font_shape_cache_free(paths[i].svg_path_data);
							shape_cache_debug_size -= paths[i].path_len;
						}
					}				
					SYS_LOG_DBG("testfont free shape cache %p for %p for face\n", cache->glyph_index, cache);
					bitmap_font_cache_free(cache->glyph_index);
				}
				memset(cache, 0, sizeof(freetype_cache_t));
			}
		}
	}

	return &common_faces[face_idx];
}

void _ft_deref_internal_face(internal_ft_face_t* intface)
{
	if(intface->face_ref > 1)
	{
		intface->face_ref--;
	}
	else
	{
		//FTC_Manager_RemoveFaceID(ft_cache_manager, opend_font[i].internal_face);
	}

}

int _freetype_cache_init(freetype_font_t* font, freetype_cache_t* cache, const char* file_path, int is_shape_cache)
{
	uint8_t* cache_start;
	uint32_t cache_size;
	
	if(cache == NULL)
	{
		SYS_LOG_ERR("null font cache");
		return -1;
	}

	if(cache->inited == 1)
	{
		return 0;
	}

	if(is_shape_cache)
	{
		cache->is_shape_cache = 1;
		cache_size = freetype_font_get_shape_info_size();
	}
	else
	{
		cache->is_shape_cache = 0;
		cache_size = _find_cache_bytes_for_size(file_path, font->font_size);
	}

	if(cache_size < 3*(cache->unit_size + sizeof(uint32_t) + sizeof(bbox_metrics_t)))
	{
		cache_size = 3*(cache->unit_size + sizeof(uint32_t) + sizeof(bbox_metrics_t));
	}

	cache_start = bitmap_font_cache_malloc(cache_size);
	if(cache_start == NULL)
	{
		SYS_LOG_ERR("malloc cache failed for %p for font %p size %d, is shape %d \n", cache, font, font->font_size, is_shape_cache);
		return -1;
	}
	SYS_LOG_DBG("testfont malloc cache %p for %p for font %p size %d, is shape %d \n", cache_start, cache, font, font->font_size, is_shape_cache);
	memset(cache_start, 0, cache_size);

	SYS_LOG_INF("cache_start %p, cache_size %d\n", cache_start, cache_size);
	
	cache->cached_max = cache_size/(cache->unit_size + sizeof(uint32_t) + sizeof(bbox_metrics_t));
	cache->glyph_index = (uint32_t*)cache_start;
	
	cache->metrics = (bbox_metrics_t*)(cache_start + cache->cached_max*sizeof(uint32_t));
	cache->data = cache_start + cache->cached_max*sizeof(uint32_t) + cache->cached_max*sizeof(bbox_metrics_t);
	
	memset(&cache->default_metric, 0, sizeof(bbox_metrics_t));
	cache->default_data = NULL;
	cache->inited = 1;

	cache->last_glyph_idx = 0;
	cache->last_unicode = 0;
	cache->last_idx = -1;
	cache->cached_total = 0;

	SYS_LOG_INF("metrics_buf %p, data_buf %p, cached max %d\n", cache->metrics, cache->data, cache->cached_max);
	return 0;
}


static int _ft_try_get_cached_index(freetype_cache_t* cache, uint32_t glyf_id)
{
	int32_t cindex;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total > 0)
	{	
		if (glyf_id == cache->glyph_index[cache->last_idx]) {
			return cache->last_idx;
		}


		cindex = cache->last_idx + 1;
		for (; cindex < cache->cached_total; cindex++) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_idx = cindex;
				return cindex;
			}
		}

		cindex = cache->last_idx - 1;
		for (; cindex >= 0; cindex--) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_idx = cindex;
				return cindex;
			}
		}
	}

	return -1;
}

static uint8_t* _ft_get_glyph_cache(freetype_cache_t* cache, uint32_t glyf_id);
static int _ft_get_cache_index(freetype_cache_t* cache, uint32_t glyf_id, uint32_t slot_num)
{	
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total+slot_num <= cache->cached_max)
	{
//		SYS_LOG_INF("current %d, current id %d\n", cache->current, cache->glyph_index[cache->current]);
		if(cache->glyph_index[cache->current] == 0)
		{
			cache->cached_total+=slot_num;
			cache->glyph_index[cache->current] = glyf_id;
		}
		else
		{
			cache->cached_total+=slot_num;
			cache->current = cache->next;			
			cache->glyph_index[cache->current] = glyf_id;
		}
		cache->next = cache->current+slot_num;
		if(slot_num > 1)
		{
			int i;
			for(i=1;i<slot_num;i++)
			{
				cache->glyph_index[cache->current+i] = 0xffffffff;
			}
		}		
	}
	else
	{
		cache->current = cache->next;
		cache->next = cache->current+slot_num;
		if(cache->next >= cache->cached_max)
		{
			cache->current = 0;
			cache->next = slot_num;
		}

		uint8_t* data = _ft_get_glyph_cache(cache, cache->glyph_index[cache->current]);
		if(cache->is_shape_cache)
		{
			freetype_svg_path_t* svg_path = (freetype_svg_path_t*)data;
			if(!svg_path)
			{
				SYS_LOG_ERR("invalid shape cache data\n");
				return -1;
			}

			if(svg_path->svg_path_data)
			{
				SYS_LOG_DBG("fttest free index %d, max %d, total %d, glyf_id %d\n", cache->current, cache->cached_max, cache->cached_total, cache->glyph_index[cache->current]);
				freetype_font_shape_cache_free(svg_path->svg_path_data);
				svg_path->svg_path_data = NULL;
				shape_cache_debug_size -= svg_path->path_len;
				svg_path->path_len = 0;
			}
		}

		cache->glyph_index[cache->current] = glyf_id;
		if(slot_num > 1)
		{
			int i;
			for(i=1;i<slot_num;i++)
			{
				cache->glyph_index[cache->current+i] = 0xffffffff;
			}
		}
		if(cache->cached_total < cache->cached_max)
		{
			cache->cached_total = cache->cached_max;
		}
	}

	cache->last_idx = cache->current;

	return cache->current;

}

//when data cache dont meet needs, we need to restore a usable state for allocated cache index
//FIXME: only consider slot num equals 1, since only shape cache need to revert cache index
static void _ft_clear_cache_index(freetype_cache_t* cache, uint32_t cindex)
{
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return;
	}

	cache->glyph_index[cindex] = 0;
	if(cindex > 0)
	{
		cache->next = cindex;
		cache->current = cindex - 1;
		cache->cached_total--;
	}
	SYS_LOG_DBG("cache index cleared, current %d, next %d, total %d\n", cache->current, cache->next, cache->cached_total);
	return;
}

static uint8_t* _ft_get_glyph_cache(freetype_cache_t* cache, uint32_t glyf_id)
{
	int32_t cache_index;
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return NULL;
	}

	cache_index = _ft_try_get_cached_index(cache, glyf_id);
	SYS_LOG_DBG("cache_index %d, unit size %d\n", cache_index, cache->unit_size);
	if(cache_index < 0)
	{
	    SYS_LOG_INF("cant find bitmap for glyf id %d\n", glyf_id);
		return NULL;
	}
	else
	{
		return &(cache->data[cache_index*cache->unit_size]);
	}
}

static int _ft_free_common_size(FT_Face face, int font_size, FT_Size sizep)
{
	int i;

	for(i=0;i<freetype_font_get_max_size_num();i++)
	{
		if(s_common_sizes[i].face == face && s_common_sizes[i].size == font_size)
		{
			if(s_common_sizes[i].sizep != sizep)
			{
				SYS_LOG_INF("free size obj not match %p", sizep);
			}

			if(s_common_sizes[i].ref > 0)
			{
				s_common_sizes[i].ref--;
			}

			if(s_common_sizes[i].ref == 0 && font_size != SVG_BASE_SIZE && s_common_sizes[i].sizep != NULL)
			{
				FT_Done_Size(sizep);
				s_common_sizes[i].sizep = NULL;
				s_common_sizes[i].size = 0;
				s_common_sizes[i].ref = 0;
				s_common_sizes[i].face = NULL;				
			}
			
			return i;
		}
	}

	SYS_LOG_INF("size obj not found %d %p\n", font_size, sizep);
	return -1;
}

static void _ft_clear_common_size(FT_Face face)
{
	int i;
	for(i=0;i<freetype_font_get_max_size_num();i++)
	{
		if(!s_common_sizes[i].sizep)
		{
			continue;
		}

		if(!s_common_sizes[i].sizep->face)
		{
			s_common_sizes[i].sizep = NULL;
			s_common_sizes[i].size = 0;
			s_common_sizes[i].ref = 0;
			s_common_sizes[i].face = NULL;
		}
		else if(s_common_sizes[i].sizep->face == face)
		{
			FT_Done_Size(s_common_sizes[i].sizep);
			s_common_sizes[i].sizep = NULL;
			s_common_sizes[i].size = 0;		
			s_common_sizes[i].ref = 0;
			s_common_sizes[i].face = NULL;
		}
	}	
}

static int _ft_get_common_size(FT_Face face, int font_size)
{
	int i;
	int empty_slot = -1;
	int expired_slot = -1;

	for(i=0;i<freetype_font_get_max_size_num();i++)
	{
		if(s_common_sizes[i].face == face && s_common_sizes[i].size == font_size)
		{
			s_common_sizes[i].ref++;
			return i;
		}

		if(s_common_sizes[i].size == 0 && empty_slot < 0)
		{
			empty_slot = i;
		}

		if(expired_slot < 0 && s_common_sizes[i].ref == 0)
		{
			expired_slot = i;
		}
	}

	if(empty_slot >= 0)
	{
		s_common_sizes[empty_slot].face = face;
		s_common_sizes[empty_slot].size = font_size;
		s_common_sizes[empty_slot].ref = 1;
		return empty_slot;
	}
	else if(expired_slot >= 0)
	{
		s_common_sizes[expired_slot].face = face;
		s_common_sizes[expired_slot].size = font_size;
		s_common_sizes[expired_slot].ref = 1;
		if(s_common_sizes[expired_slot].sizep)
		{
			FT_Done_Size(s_common_sizes[expired_slot].sizep);
			s_common_sizes[expired_slot].sizep = NULL;
		}
		return expired_slot;
	}
	else
	{
		SYS_LOG_ERR("no empty size slot \n");
		return -1;
	}

}

freetype_font_t* freetype_font_open(const char* file_path, uint32_t font_size)
{
	int32_t ret;
	int32_t error;
	uint32_t i;
	int32_t empty_slot = -1;
	int32_t copy_slot = -1;
	freetype_font_t* ft_font = NULL;
	int bpp;

	SYS_LOG_DBG("max fonts %d\n", max_fonts);

	for(i=0;i<max_fonts;i++)
	{
		if(opend_font[i].internal_face != NULL)
		{
			SYS_LOG_DBG("opend_font[i].internal_face->font_path %s\n", opend_font[i].internal_face->font_path);
			if(strcmp(opend_font[i].internal_face->font_path, file_path) == 0) 
			{
				if(opend_font[i].font_size == font_size)
				{
					ft_font = &opend_font[i];
					ft_font->ref_count++;
					SYS_LOG_INF("open cached: font size %d, size obj %p\n", ft_font->font_size, ft_font->size);
					return ft_font;
				}
				else
				{
					//same face, different size
					copy_slot = i;
				}
			}
		}
		else
		{
			if(empty_slot < 0)
			{
				empty_slot = i;
			}
		}
	}

	if(empty_slot >= 0)
	{
		ft_font = &opend_font[empty_slot];
	}
	else
	{
		SYS_LOG_ERR("no available slot for freetype font %s", file_path);
		return NULL;
	}

	//prepare face
	if(copy_slot >= 0)
	{
		memcpy(ft_font, &opend_font[copy_slot], sizeof(freetype_font_t));
		ft_font->size = NULL;
	}
	else
	{
		if(freetype_font_get_memory_face_enabled())
		{
			memset(ft_font, 0, sizeof(freetype_font_t));	
			if(!freetype_font_use_svg_path())
			{
				ft_font->internal_face = (internal_ft_face_t*)bitmap_font_cache_malloc(sizeof(internal_ft_face_t));
				memset(ft_font->internal_face, 0, sizeof(internal_ft_face_t));
			}
			else
			{
				ft_font->internal_face = _ft_get_internal_face(file_path);
				SYS_LOG_DBG("%s %d internal face %p\n", __FILE__, __LINE__, ft_font->internal_face);
			}

			if(ft_font->internal_face == NULL)
			{
				return NULL;
			}

			if(ft_font->internal_face->face == NULL) {
				memset(ft_font->internal_face, 0, sizeof(internal_ft_face_t));
				strcpy(ft_font->internal_face->font_path, file_path);

				ret = sd_fmap(file_path, (void**)&font_file_addr, &font_file_size);	
				if(ret < 0) {
					//map failed use file face
					ret = FT_New_Face(ft_library, file_path, 0, (FT_Face*)&ft_font->internal_face->face);
					if(ret) {
						SYS_LOG_ERR("FT_New_Face faild %d\n", ret);
						return NULL;
					}					
				} else {
					ret = FT_New_Memory_Face(ft_library, font_file_addr, font_file_size, 0, (FT_Face*)&ft_font->internal_face->face);
					if(ret) {
						ret = FT_New_Face(ft_library, file_path, 0, (FT_Face*)&ft_font->internal_face->face);
						if(ret) {
							SYS_LOG_ERR("FT_New_Face faild %d\n", ret);
							return NULL;
						}
					}
				}
			}

			SYS_LOG_DBG("%s %d face %p\n", __FILE__, __LINE__, ft_font->internal_face->face);
		}
		else
		{
			memset(ft_font, 0, sizeof(freetype_font_t));	
			if(!freetype_font_use_svg_path())
			{
				ft_font->internal_face = (internal_ft_face_t*)bitmap_font_cache_malloc(sizeof(internal_ft_face_t));
				memset(ft_font->internal_face, 0, sizeof(internal_ft_face_t));
			}
			else
			{
				ft_font->internal_face = _ft_get_internal_face(file_path);
			}
			if(ft_font->internal_face->face == NULL)
			{
				memset(ft_font->internal_face, 0, sizeof(internal_ft_face_t));
				strcpy(ft_font->internal_face->font_path, file_path);
				
				ret = FT_New_Face(ft_library, file_path, 0, (FT_Face*)&ft_font->internal_face->face);
				if(ret)
				{
					SYS_LOG_ERR("FT_New_Face faild %d\n", ret);
					return NULL;
				}
			}
		}

	}

	//prepare cache
	ft_font->font_size = font_size;
	bpp = freetype_font_get_font_fixed_bpp();
	ft_font->bpp = bpp;
	SYS_LOG_DBG("empty slot %d, copy slot %d, font size %d\n", empty_slot, copy_slot, font_size);
	if(!freetype_font_use_svg_path())
	{
		ft_font->cache = &ft_font_cache[empty_slot];
		memset(ft_font->cache, 0, sizeof(freetype_cache_t));
		uint32_t bmp_size = font_size;
		//bmp_size = ((bmp_size + 3)/4) * 4;
		bmp_size = (bmp_size+7)/(8/bpp);
		bmp_size = bmp_size*font_size;
		ft_font->cache->unit_size = bmp_size;

		ret = _freetype_cache_init(ft_font, ft_font->cache, file_path, 0);
		if(ret < 0)
		{
			SYS_LOG_INF("font cache init failed\n");
			goto ERR_EXIT;
		}
	}
	else
	{
		//glyf shape cache can be shared between sizes, only record index in shape cache
		if(copy_slot >= 0)
		{
			ft_font->shape_cache = &ft_font->internal_face->shape_cache;
		}
		else
		{
			ft_font->shape_cache = &ft_font->internal_face->shape_cache;
			if(ft_font->shape_cache->inited == 0)
			{
				ft_font->shape_cache->unit_size = sizeof(freetype_glyphshape_t);
				ret = _freetype_cache_init(ft_font, ft_font->shape_cache, file_path, 1);
				if(ret < 0)
				{
					SYS_LOG_INF("font cache init failed\n");
					goto ERR_EXIT;
				}	
			}		
		}

		ft_font->cache = &ft_font_cache[empty_slot]; //dont init it yet
	}
	SYS_LOG_INF("font attr font size %d, per data size %d, max cached number %d\n", ft_font->font_size, ft_font->cache->unit_size, ft_font->cache->cached_max);

	//set size 
	FT_Face face = (FT_Face)ft_font->internal_face->face;
	if(copy_slot < 0)
	{
		int size_idx = _ft_get_common_size(face, font_size);
		if(size_idx < 0)
		{
			SYS_LOG_ERR("no empty size slot\n");
			return NULL;
		}
		if(s_common_sizes[size_idx].sizep == NULL)
		{
			FT_New_Size(ft_font->internal_face->face, &s_common_sizes[size_idx].sizep);
		}
		FT_Activate_Size(s_common_sizes[size_idx].sizep);


		//set face size to font size to retrieve correct ascent/descent info
		error = FT_Set_Pixel_Sizes(ft_font->internal_face->face, 0,font_size);
		if ( error ) {
			SYS_LOG_ERR("Error in FT_Set_Char_Size: %d\n", error);
			return NULL;
		}
		ft_font->internal_face->current_size = font_size;
		ft_font->size = face->size;

		error = FT_Select_Charmap(ft_font->internal_face->face, FT_ENCODING_UNICODE);
		if (error)
		{
			SYS_LOG_ERR("FT_Select_Charmap failed: %d\n", error);
			return NULL;
		}

		SYS_LOG_DBG("common size %p, face size %p\n", s_common_sizes[size_idx].sizep, face->size);
	}
	else
	{
		int size_idx = _ft_get_common_size(face, font_size);
		if(size_idx < 0)
		{
			SYS_LOG_ERR("no empty size slot\n");
			return NULL;
		}
		if(s_common_sizes[size_idx].sizep == NULL)
		{
			FT_New_Size(ft_font->internal_face->face, &s_common_sizes[size_idx].sizep);
		}
		FT_Activate_Size(s_common_sizes[size_idx].sizep);

		//set face size to font size to retrieve correct ascent/descent info
		error = FT_Set_Pixel_Sizes(ft_font->internal_face->face, 0,font_size);
		if ( error ) {
			SYS_LOG_ERR("Error in FT_Set_Char_Size: %d\n", error);
			return NULL;
		}		

		SYS_LOG_DBG("common size %p, face size %p\n", s_common_sizes[size_idx].sizep, face->size);
		ft_font->size = face->size;
		ft_font->internal_face->current_size = font_size;
	}

	ft_font->internal_face->face_ref++;

	ft_font->ref_count=1;

	SYS_LOG_DBG("%s %d, opend size %p\n", __FILE__, __LINE__, face->size);

	ft_font->ascent = (face->size->metrics.ascender >> 6);
	ft_font->descent = (face->size->metrics.descender >> 6);
	ft_font->max_advance = (face->size->metrics.max_advance >> 6);

	ft_font->line_height = ft_font->ascent - ft_font->descent;
	ft_font->base_line = -ft_font->descent; //(dsc->font->ascent);  //Base line measured from the top of line_height

	if(freetype_font_use_svg_path())
	{
		error = FT_Set_Pixel_Sizes(ft_font->internal_face->face, 0,SVG_BASE_SIZE);
		if ( error ) {
			SYS_LOG_ERR("Error in FT_Set_Char_Size: %d\n", error);
			return NULL;
		}		

		ft_font->size = face->size;
		ft_font->internal_face->current_size = SVG_BASE_SIZE;		
	}

	SYS_LOG_DBG("open: font size %d, size obj %p\n", ft_font->font_size, ft_font->size);
	SYS_LOG_INF("font size %d, asc %d, dsc %d, lineheight %d, base line %d, height %d\n", ft_font->font_size, ft_font->ascent, ft_font->descent, 
					ft_font->line_height, ft_font->base_line, (face->size->metrics.height >> 6));

	bitmap_font_cache_dump_info();


	return ft_font;
ERR_EXIT:

	memset(ft_font, 0, sizeof(freetype_font_t));
	return NULL;
}

void freetype_font_close(freetype_font_t* ft_font)
{
	int32_t i;
	
	if(ft_font != NULL)
	{
		for(i=0;i<max_fonts;i++)
		{
			if(ft_font == &opend_font[i])
			{
				break;				
			}
		}

		if(i >= max_fonts)
		{
			return;
		}

		if(opend_font[i].internal_face != NULL)
		{
			if(opend_font[i].ref_count > 1)
			{
				opend_font[i].ref_count--;
			}
			else
			{
					
				SYS_LOG_DBG("%s %d done size %p\n", __FILE__, __LINE__, opend_font[i].size);

				_ft_free_common_size(opend_font[i].internal_face->face, opend_font[i].font_size, opend_font[i].size);

				if(opend_font[i].internal_face->face_ref > 1)
				{
					opend_font[i].internal_face->face_ref--;
				}
				else
				{
					if(freetype_font_use_svg_path())
					{
						opend_font[i].internal_face->face_ref = 0;
					}
					else
					{
						FT_Face face = opend_font[i].internal_face->face;
						_ft_clear_common_size(face);
						FT_Done_Face(face);
						bitmap_font_cache_free(opend_font[i].internal_face);
					}
				}

				//FIXME: how to reserve internal face
				
				memset(&opend_font[i], 0, sizeof(freetype_font_t));	
				
				//no matter svg path enabled or not , bitmap cache maybe inited				
				if(ft_font_cache[i].glyph_index)
				{
					SYS_LOG_DBG("cache_start %p freed\n", ft_font_cache[i].glyph_index);
					SYS_LOG_DBG("testfont free cache %p for %p for font %p\n", ft_font_cache[i].glyph_index, 
						&ft_font_cache[i], &opend_font[i]);
					bitmap_font_cache_free(ft_font_cache[i].glyph_index);
					ft_font_cache[i].glyph_index = NULL;
					ft_font_cache[i].metrics = NULL;
					ft_font_cache[i].data = NULL;
				}

				if(ft_font_cache[i].inited == 1)
				{
					ft_font_cache[i].inited = 0;
				}	
			}
		}

	
	}
}


int freetype_font_cache_preset(freetype_font_cache_preset_t* preset, int count)
{
    int i;
    int path_len;
    
    if(preset == NULL)
    {
        SYS_LOG_ERR("null font cache preset");
        return -1;
    }

    if(font_cache_presets != NULL)
    {
        for(i=0;i<font_cache_preset_num;i++)
        {
            if(font_cache_presets[i].font_path != NULL)
            {
                bitmap_font_cache_free(font_cache_presets[i].font_path);
            }
        }
        bitmap_font_cache_free(font_cache_presets);
        font_cache_presets = NULL;
    }

    font_cache_presets = bitmap_font_cache_malloc(sizeof(freetype_font_cache_preset_t)*count);
    if(font_cache_presets == NULL)
    {
        SYS_LOG_ERR("font cache preset malloc failed");
        return -1;
    }

    for(i=0;i<count;i++)
    {
        if(preset[i].font_path == NULL)
        {
            continue;
        }
        path_len = strlen(preset[i].font_path)+1;
        font_cache_presets[i].font_path = bitmap_font_cache_malloc(path_len);
        strcpy(font_cache_presets[i].font_path, preset[i].font_path);
		font_cache_presets[i].font_size = preset[i].font_size;
        font_cache_presets[i].cache_size_preset = preset[i].cache_size_preset;
    }
    font_cache_preset_num = count;

    return 0;
}

uint8_t * freetype_font_get_bitmap(freetype_font_t* font, freetype_cache_t* cache, uint32_t unicode)
{
	uint32_t glyf_id;
	uint8_t* data;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null cache info, %p\n", cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}	

	glyf_id = _ft_get_glyf_id_cached(font, unicode);
	
	if(glyf_id == 0 && font->default_code > 0)
	{
		//glyf not found && default code set
		glyf_id = _ft_get_glyf_id_cached(font, font->default_code);
	}

	if(glyf_id == 0)
	{
		//glyf not found with no default code or default code not exsit
		if(font->cache->default_data != NULL)
		{
			return font->cache->default_data;
		}
		glyf_id = 1;
	}
	data = _ft_get_glyph_cache(cache, glyf_id);
	if(data == NULL && font->cache->default_data != NULL)
	{
	    SYS_LOG_INF("null data, return default data\n");
	    return font->cache->default_data;
	}

	return data;
}

void _read_bitmap_in_bpp(uint8_t* data, uint8_t* gray_data, int pitch, int rows, int dst_bpp)
{
	int i, j, k;
	int ppb = 8/dst_bpp; //pixels per byte
	int stride = (pitch * dst_bpp + 7)/8;

	//SYS_LOG_DBG("pitch %d, rows %d, stride %d, ppb %d, data %p\n", pitch, rows, stride, ppb, &data[0]);
	k = 0;
	for(i=0;i<rows;i++) {
		for(j=0;j < stride ;j++) {
			switch(ppb) {
			case 2:
				k=i*pitch + 2*j + 1;
				data[i*stride + j] = (gray_data[k]&0xf0) | ((gray_data[k+1]&0xf0)>>4);
				//printk("0x%0.2x ", data[i*stride+j]);
				break;
			case 4:
				k=i*pitch + 4*j;
				data[i*stride + j] = (gray_data[k]&0xc0) | ((gray_data[k+1]&0xc0)>>2) 
					| 				((gray_data[k+2]&0xc0)>>4) | ((gray_data[k+3]&0xc0)>>6);
				break;
			case 8:
				break;
			}

			if(ppb == 2 && 2*j+1 >= pitch) {
				break;
			} else if (ppb == 4 && 4*j+3 >= pitch) {
				break;
			}

		}

		if(k < i*pitch + pitch -1) {
			switch(ppb) {
			case 2:
				data[i*stride+j] = gray_data[i*pitch + 2*j]&0xf0;
				break;
			case 4:
				for(k=4*j;k<pitch;k++) {
					data[i*stride+j] |= (gray_data[i*pitch+k]&0xc0)>>2*k;
				}
				break;
			case 8:
				break;
			}

		}

	}
}


static int _move_to(const FT_Vector * to, void * user)
{
	freetype_glyphshape_t *pshape = user;

	if (pshape->num_vertices < freetype_font_get_max_vertices()) {
		pshape->vertices[pshape->num_vertices].x = to->x;
		pshape->vertices[pshape->num_vertices].y = to->y;
		pshape->vertices[pshape->num_vertices++].type = FT_vmove;
	} else {
		SYS_LOG_ERR("increase ft shape vertex cache !");
	}

	return 0;
}

static int _line_to(const FT_Vector * to, void * user)
{
	freetype_glyphshape_t *pshape = user;

	if (pshape->num_vertices < freetype_font_get_max_vertices()) {
		pshape->vertices[pshape->num_vertices].x = to->x;
		pshape->vertices[pshape->num_vertices].y = to->y;
		pshape->vertices[pshape->num_vertices++].type = FT_vline;
	} else {
		SYS_LOG_ERR("increase ft shape vertex cache !");
	}

	return 0;
}

static int _conic_to(const FT_Vector * control, const FT_Vector * to, void * user)
{
	freetype_glyphshape_t *pshape = user;

	if (pshape->num_vertices < freetype_font_get_max_vertices()) {
		pshape->vertices[pshape->num_vertices].x = to->x;
		pshape->vertices[pshape->num_vertices].y = to->y;
		pshape->vertices[pshape->num_vertices].cx = control->x;
		pshape->vertices[pshape->num_vertices].cy = control->y;
		pshape->vertices[pshape->num_vertices++].type = FT_vcurve;
	} else {
		SYS_LOG_ERR("increase ft shape vertex cache !");
	}

	return 0;
}

static int _cubic_to(const FT_Vector * control1, const FT_Vector * control2, const FT_Vector * to, void * user)
{
	freetype_glyphshape_t *pshape = user;

	if (pshape->num_vertices < freetype_font_get_max_vertices()) {
		pshape->vertices[pshape->num_vertices].x = to->x;
		pshape->vertices[pshape->num_vertices].y = to->y;
		pshape->vertices[pshape->num_vertices].cx = control1->x;
		pshape->vertices[pshape->num_vertices].cy = control1->y;
		pshape->vertices[pshape->num_vertices].cx1 = control2->x;
		pshape->vertices[pshape->num_vertices].cy1 = control2->y;
		pshape->vertices[pshape->num_vertices++].type = FT_vcubic;
	} else {
		SYS_LOG_ERR("increase ft shape vertex cache !");
	}

	return 0;
}

static const FT_Outline_Funcs s_ft_outline_callback = {
	.move_to = _move_to,
	.line_to = _line_to,
	.conic_to = _conic_to,
	.cubic_to = _cubic_to,
};


uint8_t* freetype_font_load_glyph_shape(freetype_font_t* font, freetype_cache_t *cache,
		uint32_t unicode, float* scale)
{
	uint32_t glyf_id;
	uint8_t* data;

	if((font == NULL) || (cache == NULL) || (cache != font->shape_cache))
	{
		SYS_LOG_ERR("null or invalid font info, %p, %p\n", font, cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}	

	SYS_LOG_DBG("freetype_font_load_glyph_shape entry 0x%x\n", unicode);
	glyf_id = _ft_get_glyf_id_cached(font, unicode);
	
	if(glyf_id == 0 && font->default_code > 0)
	{
		//glyf not found && default code set
		glyf_id = _ft_get_glyf_id_cached(font, font->default_code);
	}

	if(glyf_id == 0)
	{
		//glyf not found with no default code or default code not exsit
		SYS_LOG_ERR("glyf not found 0x%x\n", unicode);
		return NULL;
	}

	data = _ft_get_glyph_cache(cache, glyf_id);
	if(data == NULL)
	{
		SYS_LOG_ERR("get glyph cache faild 2 for glyph id %d\n", glyf_id);
		return NULL;
	}
	freetype_svg_path_t* svg_path = (freetype_svg_path_t*)data;

	if(svg_path->path_len == 0)
	{
		//FIXME: it's impossible?
		SYS_LOG_ERR("invalid svg path len \n");
/*
		//shape not cached, why?
		freetype_glyphshape_t shape;

		SYS_LOG_DBG("shape not cached for 0x%x\n", unicode);
		face = (FT_Face)font->internal_face->face;

		//switch size obj
		if(font->size != NULL && face->size != font->size)
		{
			FT_Activate_Size((FT_Size)font->size);
			FT_Set_Pixel_Sizes(font->internal_face->face, 0,SVG_BASE_SIZE);
			font->internal_face->current_size = SVG_BASE_SIZE;
		}

		error = FT_Load_Glyph(face, glyf_id, FT_LOAD_NO_BITMAP);
		if ( error )
		{
			SYS_LOG_ERR("Error in FT_Load_Glyph: %d, unicode 0x%x, glyf id %d\n", error, unicode, glyf_id);
			return NULL;
		}

		s_ft_shape.num_vertices = 0;
		FT_Outline_Decompose(&face->glyph->outline, &s_ft_outline_callback, &s_ft_shape);
		
		FT_BBox box;
		FT_Outline_Get_BBox(&face->glyph->outline, &box);

		memset(&shape, 0, sizeof(freetype_glyphshape_t));
		shape.bbox_x1 = box.xMin;
		shape.bbox_y1 = box.yMin;
		shape.bbox_x2 = box.xMax;
		shape.bbox_y2 = box.yMax;
		shape.num_vertices = s_ft_shape.num_vertices;
		
		//memcpy(vertices_data, s_ft_shape.vertices, s_ft_shape.num_vertices*sizeof(freetype_vertex_t));
		shape.vertices = (freetype_vertex_t*)s_ft_shape.vertices;

		svg_path->svg_path_data = ttf_generate_svg_path(font, &shape, scale, &svg_path->path_len);
		if(svg_path->svg_path_data == NULL)
		{
			FIXME
		}

		svg_path->current_size = SVG_BASE_SIZE;

		shape_cache_debug_size += svg_path->path_len;
		if(shape_cache_debug_size > shape_cache_peak_size)
		{
			shape_cache_peak_size = shape_cache_debug_size;
		}	
*/
	}
	else
	{
		if(svg_path->current_size > 0)
		{
			float sca = (float)(font->font_size)/(svg_path->current_size);
			*scale = sca;		
		}
	}
	
	return (uint8_t*)svg_path->svg_path_data;
}

void freetype_font_free_glyph_shape(freetype_glyphshape_t *shape)
{
}
 

bbox_metrics_t* freetype_font_get_glyph_dsc(freetype_font_t* font, freetype_cache_t *cache, uint32_t unicode)
{
	uint32_t glyf_id;
	uint8_t* data;
	int32_t cache_index;
	int error;
	FT_Face face;
	uint32_t bmp_size;
	float scale = 0.0;

	if((font == NULL) || (cache == NULL))
	{
		SYS_LOG_ERR("null font info, %p, %p\n", font, cache);
		return NULL;
	}

	//SYS_LOG_INF("in freetype_font_get_glyph_dsc \n");
	face = (FT_Face)font->internal_face->face;
	glyf_id = _ft_get_glyf_id_cached(font, unicode);
	//SYS_LOG_INF("unicode 0x%x, glyf id %d, font->font_size %d\n", unicode, glyf_id, font->font_size);

	if(glyf_id == 0 && font->default_code > 0)
	{
		//glyf not found && default code set
		SYS_LOG_DBG("use default code\n");
		glyf_id = _ft_get_glyf_id_cached(font, font->default_code);
	}


	if(glyf_id == 0)
	{
		//glyf not found with no default code or default code not exsit
		if(font->cache && font->cache->default_data != NULL)
		{
			return &font->cache->default_metric;
		}
		glyf_id = 1;
	}

	if(freetype_font_use_svg_path() && cache == font->cache && font->cache->inited == 0)
	{
		memset(font->cache, 0, sizeof(freetype_cache_t));
		uint32_t bmp_size = font->font_size;
		bmp_size = ((bmp_size + 3)/4) * 4;
		bmp_size = bmp_size/(8/font->bpp);
		bmp_size = bmp_size*font->font_size;
		bmp_size = ((bmp_size + 3)/4) * 4;
		font->cache->unit_size = bmp_size;

		int ret = _freetype_cache_init(font, font->cache, font->internal_face->font_path, 0);
		if(ret < 0)
		{
			SYS_LOG_INF("font cache init failed\n");
			return NULL;
		}			
	}

	cache_index = _ft_try_get_cached_index(cache, glyf_id);
	if(cache_index >= 0)
	{
		//found cached, fill data to glyph dsc
		SYS_LOG_DBG("cached cache_idx %d for 0x%x\n", cache_index, unicode);
		
		return &cache->metrics[cache_index];
	}
	else
	{
		//switch size obj
		//if(font->size != NULL && face->size != font->size)
		{
			FT_Activate_Size((FT_Size)font->size);
			if(!freetype_font_use_svg_path() || cache != font->shape_cache)
			{
				FT_Set_Pixel_Sizes(font->internal_face->face, 0,font->font_size);
				font->internal_face->current_size = font->font_size;
			}
			else
			{
				FT_Set_Pixel_Sizes(font->internal_face->face, 0,SVG_BASE_SIZE);
				font->internal_face->current_size = SVG_BASE_SIZE;
			}
			
		}

		int load_flag;
		if(freetype_font_enable_subpixel())
		{
			load_flag = FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
		}
		else
		{
			load_flag = FT_LOAD_DEFAULT;
		}

		error = FT_Load_Glyph(
			 face,			/* handle to face object */
			 glyf_id,	/* glyph index			 */
			 load_flag);  /* load flags, see below *///FT_LOAD_MONOCHROME|FT_LOAD_NO_AUTOHINTING|FT_LOAD_NO_HINTING
		if ( error )
		{
			SYS_LOG_ERR("Error in FT_Load_Glyph: %d, unicode 0x%x, glyf id %d\n", error, unicode, glyf_id);
			SYS_LOG_ERR("font %s, size %d, face %p, size %p\n", font->internal_face->font_path, font->font_size, font->internal_face->face, font->size);

			return NULL;
		}

		if(unicode == 0x20 || unicode == 0xa0)
		{
			cache_index = _ft_get_cache_index(cache, glyf_id, 1);
			data = _ft_get_glyph_cache(cache, glyf_id);

			cache->metrics[cache_index].advance = (face->glyph->advance.x >> 6);
			cache->metrics[cache_index].bbh = 0; 		/*Height of the bitmap in [px]*/
			cache->metrics[cache_index].bbw = 0;		 /*Width of the bitmap in [px]*/
			cache->metrics[cache_index].bbx = 0; 		/*X offset of the bitmap in [pf]*/
			cache->metrics[cache_index].bby = 0;		  /*Y offset of the bitmap measured from the as line*/
			if(!freetype_font_use_svg_path() || cache != font->shape_cache)
			{
				cache->metrics[cache_index].metric_size = font->font_size;
			}
			else
			{
				cache->metrics[cache_index].metric_size = SVG_BASE_SIZE;
			}

			return &cache->metrics[cache_index];
		}

		if(!freetype_font_use_svg_path() || cache != font->shape_cache) //either not using svg path or under text canvas context when enabling svg path
		{
			int render_flag;
			if(freetype_font_enable_subpixel())
			{
				render_flag = FT_RENDER_MODE_LCD;
			}
			else
			{
				render_flag = FT_RENDER_MODE_NORMAL;
			}
			error = FT_Render_Glyph( face->glyph, render_flag );
			if ( error )
			{
				SYS_LOG_ERR("Error in FT_Render_Glyph: %d\n", error);
				return NULL;
			}

		
	//		SYS_LOG_INF("pitch %d, width %d, rows %d, unit_size %d, font->bpp %d\n", face->glyph->bitmap.pitch, face->glyph->bitmap.width, 
	//					face->glyph->bitmap.rows, cache->unit_size, font->bpp);

			uint32_t stride = (face->glyph->bitmap.pitch * font->bpp + 7)/8;
			//bmp_size = (face->glyph->bitmap.pitch * face->glyph->bitmap.rows)/(8/font->bpp);
			bmp_size = stride * face->glyph->bitmap.rows;
			int slots = bmp_size/cache->unit_size;
			if(bmp_size%cache->unit_size != 0 || bmp_size == 0)
			{
				slots += 1;
			}
			cache_index = _ft_get_cache_index(cache, glyf_id, slots);
			uint8_t* cache_data = _ft_get_glyph_cache(cache, glyf_id);

			data = cache_data;
			if(data == NULL)
			{
				SYS_LOG_ERR("get glyph cache faild for glyph id %d\n", glyf_id);
				return NULL;
			}
			memset(data, 0, bmp_size);
			if(font->bpp != 8)
			{
				_read_bitmap_in_bpp(data, face->glyph->bitmap.buffer, face->glyph->bitmap.pitch, face->glyph->bitmap.rows, font->bpp);
			}
			else
			{
				memcpy(data, face->glyph->bitmap.buffer, face->glyph->bitmap.pitch*face->glyph->bitmap.rows);
			}
		}
		else
		{
			//use gpu, get svg path			
			cache_index = _ft_get_cache_index(cache, glyf_id, 1);
			data = _ft_get_glyph_cache(cache, glyf_id);
			if(data == NULL)
			{
				SYS_LOG_ERR("get glyph cache faild 1 for glyph id %d\n", glyf_id);
				return NULL;
			}

			freetype_svg_path_t* svg_path = (freetype_svg_path_t*)data;
			freetype_glyphshape_t shape;

			s_ft_shape.num_vertices = 0;
			FT_Outline_Decompose(&face->glyph->outline, &s_ft_outline_callback, &s_ft_shape);
			
			FT_BBox box;
			FT_Outline_Get_BBox(&face->glyph->outline, &box);

			SYS_LOG_DBG("get dsc cache_index %d, glyf id %d\n", cache_index, glyf_id);
			SYS_LOG_DBG("face size %d, get dsc bbox %d, %d, %d, %d\n", (face->size->metrics.height)>>6, box.xMin, box.yMin, box.xMax, box.yMax);

			memset(&shape, 0, sizeof(freetype_glyphshape_t));
			shape.bbox_x1 = box.xMin;
			shape.bbox_y1 = box.yMin;
			shape.bbox_x2 = box.xMax;
			shape.bbox_y2 = box.yMax;
			shape.num_vertices = s_ft_shape.num_vertices;
			
			//memcpy(vertices_data, s_ft_shape.vertices, s_ft_shape.num_vertices*sizeof(freetype_vertex_t));
			shape.vertices = (freetype_vertex_t*)s_ft_shape.vertices;

			svg_path->svg_path_data = ttf_generate_svg_path(font, &shape, &scale, &svg_path->path_len);
			if(svg_path->svg_path_data == NULL)
			{
				//FIXME
				SYS_LOG_ERR("get shape cache faild for unicode 0x%x\n", unicode);
				_ft_clear_cache_index(cache, cache_index);
				return NULL;
			}
			svg_path->current_size = SVG_BASE_SIZE;

			shape_cache_debug_size += svg_path->path_len;
			if(shape_cache_debug_size > shape_cache_peak_size)
			{
				shape_cache_peak_size = shape_cache_debug_size;
			}	
			SYS_LOG_DBG("shape cache peak size %d, used size %d, add size %d\n", shape_cache_peak_size, shape_cache_debug_size, svg_path->path_len);
			
		}

		if(!freetype_font_use_svg_path() || cache != font->shape_cache)
		{
			cache->metrics[cache_index].advance = (face->glyph->metrics.horiAdvance >> 6);
			cache->metrics[cache_index].bbh = face->glyph->bitmap.rows; 		/*Height of the bitmap in [px]*/
			cache->metrics[cache_index].bbw = face->glyph->bitmap.pitch;		 /*Width of the bitmap in [px]*/
			cache->metrics[cache_index].bbx = face->glyph->bitmap_left; 		/*X offset of the bitmap in [pf]*/
			cache->metrics[cache_index].bby = face->glyph->bitmap_top - face->glyph->bitmap.rows;		  /*Y offset of the bitmap measured from the as line*/
			cache->metrics[cache_index].metric_size = font->font_size;
		}
		else
		{
			cache->metrics[cache_index].advance = (face->glyph->metrics.horiAdvance >> 6);
			cache->metrics[cache_index].bbh = (face->glyph->metrics.height >> 6); 		/*Height of the bitmap in [px]*/
			cache->metrics[cache_index].bbw = (face->glyph->metrics.width >> 6);		 /*Width of the bitmap in [px]*/
			cache->metrics[cache_index].bbx = (face->glyph->metrics.horiBearingX >> 6); 		/*X offset of the bitmap in [pf]*/
			cache->metrics[cache_index].bby = ((face->glyph->metrics.horiBearingY - face->glyph->metrics.height)>>6);		  /*Y offset of the bitmap measured from the as line*/
			cache->metrics[cache_index].metric_size = SVG_BASE_SIZE;
		}

		SYS_LOG_DBG("unicode 0x%x, adw %d x %d y %d w %d h %d, metric size %d\n", unicode, cache->metrics[cache_index].advance, 
				cache->metrics[cache_index].bbx, cache->metrics[cache_index].bby, cache->metrics[cache_index].bbw, 
				cache->metrics[cache_index].bbh, cache->metrics[cache_index].metric_size);
		return &cache->metrics[cache_index];
	}
	
}


#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
static int ttf_get_svg_path_size(freetype_vertex_t * vertices, int num_verts)
{
    /*At least, VLC_OP_END, bounding box and path_length*/
    int len = sizeof(int16_t) + sizeof(int16_t);

    for (; num_verts > 0; num_verts--, vertices++) {
        switch (vertices[0].type) {
        case FT_vmove:
            len += (2 + 1) * sizeof(int16_t);
            break;
        case FT_vline:
            len += (2 + 1) * sizeof(int16_t);
            break;
        case FT_vcurve:
            len += (4 + 1) * sizeof(int16_t);
            break;
        case FT_vcubic:
            len += (6 + 1) * sizeof(int16_t);
            break;
        default:
            break;
        }
    }

    return len;
}

static void ttf_convert_svg_path(short * path_data, int path_size, freetype_vertex_t * vertices, int num_verts)
{
    /*path length*/
    *(short *)path_data = path_size - sizeof(int16_t);
    path_data++;

    /*
     * All the vertices coordinate are relative to "Current Point" metioned in stb_truetype.
     * Also invert y to follow the display screen Y direction.
     */
	short* path_tmp = (int16_t*)path_data;
    for (; num_verts > 0; num_verts--, vertices++) {
        switch (vertices[0].type) {
        case FT_vmove:
            *(uint8_t *)path_tmp = VLC_OP_MOVE;
            path_tmp++;
            *path_tmp++ = (int16_t)vertices[0].x;
            *path_tmp++ = (int16_t)(-vertices[0].y);
            break;
        case FT_vline:
            *(uint8_t *)path_tmp = VLC_OP_LINE;
            path_tmp++;
            *path_tmp++ = (int16_t)vertices[0].x;
            *path_tmp++ = (int16_t)(-vertices[0].y);
            break;
        case FT_vcurve:
            *(uint8_t *)path_tmp = VLC_OP_QUAD;
            path_tmp++;
            *path_tmp++ = (int16_t)vertices[0].cx;
            *path_tmp++ = (int16_t)-vertices[0].cy;
            *path_tmp++ = (int16_t)vertices[0].x;
            *path_tmp++ = (int16_t)-vertices[0].y;
            break;
        case FT_vcubic:
            *(uint8_t *)path_tmp = VLC_OP_CUBIC;
            path_tmp++;
            *path_tmp++ = (int16_t)vertices[0].cx;
            *path_tmp++ = (int16_t)(-vertices[0].cy);
            *path_tmp++ = (int16_t)vertices[0].cx1;
            *path_tmp++ = (int16_t)(-vertices[0].cy1);
            *path_tmp++ = (int16_t)vertices[0].x;
            *path_tmp++ = (int16_t)(-vertices[0].y);
            break;
        default:
            break;
        }
    }

    *(uint8_t *)path_tmp = VLC_OP_END;
}

//static float ttf_svg_cache[2048];

static void * ttf_generate_svg_path(freetype_font_t* font, freetype_glyphshape_t* shape, float* scale, uint16_t* path_len)
{
	float sca;

	sca = (float)(font->font_size)/(font->internal_face->current_size);
	*scale = sca;

    int path_size = ttf_get_svg_path_size(shape->vertices, shape->num_vertices);

	SYS_LOG_DBG("path size %d\n", path_size);
	*path_len = path_size;
	short* path_data = freetype_font_shape_cache_malloc(path_size);
	if(!path_data)
	{
		return NULL;
	}

    /* CARE: the vertex can not have subpixel shift which applied after scaling */
    ttf_convert_svg_path(path_data, path_size, shape->vertices, shape->num_vertices);

    return path_data;
}
#else
static void * ttf_generate_svg_path(freetype_font_t* font, freetype_glyphshape_t* shape, float* scale, uint16_t* path_len)
{
	return NULL;
}
#endif /* CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH */

