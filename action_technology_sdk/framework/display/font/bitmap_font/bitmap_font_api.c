#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <ui_mem.h>
#include <memory/mem_cache.h>
#include "../font_mempool.h"
#include "bitmap_font_api.h"
#include <lvgl.h>

#ifndef CONFIG_SIMULATOR
#include <sdfs.h>
#else
//#include <fs/fs.h>

#define FS_O_READ		0x01
#define FS_SEEK_SET     SEEK_SET
#define FS_SEEK_END     SEEK_END
#endif


#define DEF_HIGH_FREQ_BIN				"highfreq.bin"


#define LVGL_FONT_SIZE_OFFSET				14
#define LVGL_FONT_ASCENT_OFFSET				16
#define LVGL_FONT_DESCENT_OFFSET			18
#define LVGL_FONT_ADVANCE_OFFSET			30
#define LVGL_FONT_LOCA_FMT_OFFSET			34
#define LVGL_FONT_GLYFID_FMT_OFFSET			35
#define LVGL_FONT_ADW_FMT_OFFSET			36
#define LVGL_FONT_BBXY_LENGTH_OFFSET		38
#define LVGL_FONT_BBWH_LENGTH_OFFSET		39
#define LVGL_FONT_ADW_LENGTH_OFFSET			40


#define MAX_HIGH_FREQ_NUM					3500


typedef struct
{
	uint32_t data_offset;  //relative to cmap offset
	uint32_t range_start;
	uint16_t range_length;
	uint16_t glyf_id_offset;
	uint16_t entry_count;
	uint8_t sub_format;
	uint8_t align;
}cmap_sub_header_t;

typedef union
{
	int8_t bits7:7;
	int8_t bits6:6;
	int8_t bits5:5;
	int8_t bits4:4;
	int8_t bits3:3;
	int8_t bits2:2;
	int8_t xy;
}bbxy_t;

typedef enum
{
	CACHE_TYPE_NORMAL,
	CACHE_TYPE_HIGH_FREQ,
}bitmap_cache_type_e;

typedef struct
{
	glyph_metrics_t* metrics;
	uint8_t* data;
	uint32_t font_size;
	uint32_t unit_size;
	uint32_t last_cache_idx;
	uint16_t last_unicode_idx;
}high_freq_cache_t;

typedef struct{
	char magic[4];
	int32_t count;
	uint8_t width;
	uint8_t height;
	uint8_t reserved[6];
}emoji_font_header_t;


static uint8_t* cmap_sub_data;
static uint32_t metrics32[4];

static bitmap_cache_t* bitmap_cache;
static bitmap_font_t* opend_font;

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
static high_freq_cache_t high_freq_cache;
static uint16_t* high_freq_codes;
#endif

static uint32_t elapse_count=0;

static bitmap_emoji_font_t opend_emoji_font;
static bitmap_cache_t emoji_bitmap_cache;
static int max_fonts;
static int default_font_cache_size;

static bitmap_font_cache_preset_t* font_cache_presets=NULL;
static int font_cache_preset_num=0;

static int decomp_buf_size = 0;
static int decomp_tmp_size = 0;
static uint8_t* decomp_in_buf = NULL;
static uint8_t* decomp_tmp1 = NULL;
static uint8_t* decomp_tmp2 = NULL;

void bitmap_font_load_high_freq_chars(const uint8_t* file_path);
static int32_t _search_emoji_glyf_id(bitmap_emoji_font_t* font, uint32_t unicode, uint32_t* glyf_id);

extern void decompress_glyf_bitmap(const uint8_t * in, uint8_t * out, int16_t w, int16_t h, uint8_t bpp, bool prefilter, uint8_t* linebuf1, uint8_t* linebuf2);

uint32_t _find_cache_size_for_font(const char* file_path, uint32_t file_size)
{
    int i;
    if(font_cache_presets == NULL)
    {
        if(file_size < default_font_cache_size)
        {
            return file_size;
        }
        else
        {
            return default_font_cache_size;
        }
    }

    for(i=0;i<font_cache_preset_num;i++)
    {
        if(font_cache_presets[i].font_path)
        {
            if(strcmp(font_cache_presets[i].font_path, file_path)==0)
            {
                SYS_LOG_INF("preset font cache size %s, %d\n", file_path, font_cache_presets[i].cache_size_preset);
                return font_cache_presets[i].cache_size_preset;
            }
        }
    }

    SYS_LOG_INF("no preset found for %s\n", file_path);
    if(file_size < default_font_cache_size)
    {
        return file_size;
    }
    else
    {
        return default_font_cache_size;
    }
}

int _bitmap_cache_init(bitmap_cache_t* cache, const char* file_path, uint32_t file_size)
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

	cache_size = _find_cache_size_for_font(file_path, file_size);

	if(cache_size < 3*(cache->unit_size + sizeof(uint32_t) + sizeof(glyph_metrics_t)))
	{
		cache_size = 3*(cache->unit_size + sizeof(uint32_t) + sizeof(glyph_metrics_t));
	}

	cache_start = bitmap_font_cache_malloc(cache_size);
	if(cache_start == NULL)
	{
		SYS_LOG_ERR("no memory for font cache\n");
		return -1;
	}
	SYS_LOG_INF("cache_start %p\n", cache_start);

	cache->cache_max_size = cache_size;
	cache->cached_max = cache_size/(cache->unit_size + sizeof(uint32_t) + sizeof(glyph_metrics_t));
	cache->glyph_index = (uint32_t*)cache_start;

	memset(cache->glyph_index, 0, cache->cached_max*sizeof(uint32_t));
	cache->metrics = (glyph_metrics_t*)(cache_start + cache->cached_max*sizeof(uint32_t));
	cache->data = cache_start + cache->cached_max*sizeof(uint32_t) + cache->cached_max*sizeof(glyph_metrics_t);

	memset(&cache->default_metric, 0, sizeof(glyph_metrics_t));
	cache->default_data = NULL;
	cache->inited = 1;

	SYS_LOG_INF("metrics_buf %p, data_buf %p\n", cache->metrics, cache->data);
	return 0;
}

int bitmap_font_init(void)
{
	int i;

    bitmap_font_cache_init();

	max_fonts = bitmap_font_get_max_fonts_num();
	default_font_cache_size = bitmap_font_get_font_cache_size();

	bitmap_cache = (bitmap_cache_t*)mem_malloc(max_fonts*sizeof(bitmap_cache_t));
	if(bitmap_cache == NULL)
	{
		SYS_LOG_ERR("bitmap font init failed due to unable to malloc bitmap_cache data\n");
		return -1;
	}

	opend_font = (bitmap_font_t*)mem_malloc(max_fonts*sizeof(bitmap_font_t));
	if(opend_font == NULL)
	{
		SYS_LOG_ERR("bitmap font init failed due to unable to malloc opend_font data\n");
		mem_free(bitmap_cache);
		return -1;
	}

	for(i = 0;i < max_fonts; i++)
	{
		bitmap_cache[i].inited = 0;
	}
	memset(opend_font, 0, sizeof(bitmap_font_t)*max_fonts);
	memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	if(bitmap_font_get_high_freq_enabled())
	{
		high_freq_codes = (uint16_t*)(bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts);
		high_freq_cache.metrics = (glyph_metrics_t*)(bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts + MAX_HIGH_FREQ_NUM*sizeof(uint16_t));
		high_freq_cache.data = bitmap_font_get_cache_buffer() + font_cache_size*max_fonts + lvgl_cmap_cache_size*max_fonts + MAX_HIGH_FREQ_NUM*sizeof(uint16_t) + MAX_HIGH_FREQ_NUM*sizeof(glyph_metrics_t);
		high_freq_cache.font_size = 0;
	}
#endif

	return 0;
}

int bitmap_font_deinit(void)
{
	int32_t i;

	for(i=0;i<max_fonts;i++)
	{
		if(opend_font[i].font_fp.filep != NULL)
		{
			bitmap_font_close(&opend_font[i]);
			memset(&opend_font[i], 0, sizeof(bitmap_font_t));
		}

		if(bitmap_cache[i].inited == 1)
		{
			bitmap_cache[i].inited = 0;
		}

	}

	if(bitmap_cache)
	{
		mem_free(bitmap_cache);
	}
	if(opend_font)
	{
		mem_free(opend_font);
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
	}

	if(decomp_in_buf)
	{
		bitmap_font_cache_free(decomp_in_buf);
		decomp_in_buf = NULL;
	}

	if(decomp_tmp1)
	{
		bitmap_font_cache_free(decomp_tmp1);
		decomp_tmp1 = NULL;
	}

	if(decomp_tmp2)
	{
		bitmap_font_cache_free(decomp_tmp2);
		decomp_tmp2 = NULL;
	}	

	decomp_buf_size = 0;
	decomp_tmp_size = 0;
	return 0;
}


bitmap_cache_t* bitmap_font_get_cache(bitmap_font_t* font)
{
	return font->cache;
}

bitmap_emoji_font_t* bitmap_emoji_font_open(const char* file_path)
{
	int ret;
	uint32_t i;
	uint32_t glyf_size;
	uint32_t file_size;

	if(opend_emoji_font.inited == 1)
	{
		//suppose only 1 emoji font size used
		opend_emoji_font.ref_count++;
		SYS_LOG_INF("opend emoji font attr size %d, adw %d\n", opend_emoji_font.font_height, opend_emoji_font.default_advance);
		return &opend_emoji_font;
	}

	if(file_path == NULL)
	{
		SYS_LOG_ERR(" invalid emoji font file path \n");
		return NULL;
	}

	memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));
	opend_emoji_font.cache = &emoji_bitmap_cache;
	memset(opend_emoji_font.cache, 0, sizeof(bitmap_cache_t));

	ret = fs_open(&opend_emoji_font.font_fp, file_path, FS_O_READ);
	if ( ret < 0 )
	{
		SYS_LOG_ERR(" open font file %s failed! \n", file_path);
		return NULL;
	}

    fs_seek(&opend_emoji_font.font_fp, 0, FS_SEEK_END);
	file_size = fs_tell(&opend_emoji_font.font_fp);
	fs_seek(&opend_emoji_font.font_fp, 0, FS_SEEK_SET);

	//FIXME: read emoji font head and things
	emoji_font_header_t header;
	ret = fs_read(&opend_emoji_font.font_fp, &header, sizeof(emoji_font_header_t));
	if(ret < sizeof(emoji_font_header_t))
	{
		fs_close(&opend_emoji_font.font_fp);
		SYS_LOG_ERR(" read emoji font file header failed! \n");
		return NULL;
	}

	opend_emoji_font.bpp = 3;
    opend_emoji_font.glyf_list = (emoji_font_entry_t*)bitmap_font_cache_malloc(sizeof(emoji_font_entry_t)*header.count);
    if(opend_emoji_font.glyf_list==NULL)
    {
        SYS_LOG_ERR("emoji glyf list malloc failed");
        fs_close(&opend_emoji_font.font_fp);
        return NULL;
    }
	opend_emoji_font.glyf_count = header.count;
	opend_emoji_font.inited = 1;

	ret = fs_read(&opend_emoji_font.font_fp, opend_emoji_font.glyf_list, sizeof(emoji_font_entry_t)*header.count);
	if(ret < sizeof(emoji_font_entry_t)*header.count)
	{
		fs_close(&opend_emoji_font.font_fp);
		bitmap_font_cache_free(opend_emoji_font.glyf_list);
		SYS_LOG_ERR(" read emoji font file entries failed! \n");
		return NULL;
	}

	uint32_t max_height = 0;
	uint32_t max_width = 0;
	for(i=0;i<opend_emoji_font.glyf_count;i++)
	{
		if(i==0)
		{
			max_width = opend_emoji_font.glyf_list[i].width;
			max_height = opend_emoji_font.glyf_list[i].height;
			continue;
		}
	
		if(max_width < opend_emoji_font.glyf_list[i].width && max_width*2 > opend_emoji_font.glyf_list[i].width)
		{
			max_width = opend_emoji_font.glyf_list[i].width;
		}

		if(max_height < opend_emoji_font.glyf_list[i].height && max_height*2 > opend_emoji_font.glyf_list[i].height)
		{
			max_height = opend_emoji_font.glyf_list[i].height;
		}
	}

	max_width = ((max_width+3)/4)*4;

	opend_emoji_font.font_width = max_width;
	opend_emoji_font.font_height = max_height;
	opend_emoji_font.ascent = max_height;
	opend_emoji_font.descent = 0;
	opend_emoji_font.ref_count = 1;
	opend_emoji_font.default_advance = max_width;


	glyf_size = opend_emoji_font.font_width*opend_emoji_font.bpp;
	glyf_size = ((glyf_size+3)/4)*4;
	glyf_size *= opend_emoji_font.font_height;

	if(!emoji_font_use_mmap())
	{
		opend_emoji_font.cache->unit_size = glyf_size + sizeof(lv_image_dsc_t);
	}
	else
	{
		opend_emoji_font.cache->unit_size = sizeof(lv_image_dsc_t);

	}


	ret = _bitmap_cache_init(opend_emoji_font.cache, file_path, file_size);
	if(ret < 0)
	{
		SYS_LOG_ERR("emoji font cache init failed\n");
		fs_close(&opend_emoji_font.font_fp);
		bitmap_font_cache_free(opend_emoji_font.glyf_list);	
		return NULL;
	}

	if(emoji_font_use_mmap())
	{
		//map emoji file if configd
		int file_len;
#ifndef CONFIG_SIMULATOR
		ret = sd_fmap(file_path, (void**)&opend_emoji_font.emoji_mmap_addr, &file_len);	
		if(ret < 0)
		{
			SYS_LOG_INF("mmap style file failed\n");
			opend_emoji_font.emoji_mmap_addr = NULL;
		}		
#else
		FILE* emojifp = fopen(file_path, "rb");
		if (emojifp == NULL)
		{
			printf("open emoji font %s faild \n", file_path);
			return NULL;
		}

		fseek(emojifp, 0, SEEK_END);
		file_len = ftell(emojifp);
		opend_emoji_font.emoji_mmap_addr = malloc(file_len);
		fseek(emojifp, 0, SEEK_SET);
		ret = fread(opend_emoji_font.emoji_mmap_addr, 1, file_len, emojifp);
		fclose(emojifp);
#endif
	}

	//record first code for placeholder
	opend_emoji_font.default_code = 0;
	opend_emoji_font.first_code = opend_emoji_font.glyf_list[0].unicode;

	SYS_LOG_INF("opend_emoji_font cache %p, unit size %d, cached max %d, first code 0x%x\n",
				opend_emoji_font.cache->data, opend_emoji_font.cache->unit_size,
				opend_emoji_font.cache->cached_max, opend_emoji_font.first_code);

	return &opend_emoji_font;
}

bitmap_font_t* bitmap_font_open(const char* file_path)
{
	int32_t ret;
	uint8_t attr_tmp;
	uint32_t head_size;
	uint32_t cmap_size;
	uint32_t loca_size;
	uint32_t i;
	uint32_t cmap_sub_count;
	int32_t empty_slot = -1;
	bitmap_font_t* bmp_font = NULL;
	uint32_t file_size;

	if(file_path == NULL)
	{
		return NULL;
	}

	for(i=0;i<max_fonts;i++)
	{
		if(opend_font[i].font_fp.filep != NULL)
		{
			if(strcmp(opend_font[i].font_path, file_path) == 0)
			{
				bmp_font = &opend_font[i];
				bmp_font->ref_count++;
				SYS_LOG_INF("\n opend font cmap_sub_headers %p\n", bmp_font->cmap_sub_headers);
				SYS_LOG_INF("opend font attr size %d, asc %d, desc %d, adw %d\n", bmp_font->font_size, bmp_font->ascent, bmp_font->descent, bmp_font->default_advance);
				SYS_LOG_INF("opend font attr adw len %d, bbxy len %d, bbwh len %d\n", bmp_font->adw_length, bmp_font->bbxy_length, bmp_font->bbwh_length);
				return bmp_font;
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
		bmp_font = &opend_font[empty_slot];
	}
	else
	{
		SYS_LOG_ERR("no available slot for bitmap font %s", file_path);
		return NULL;
	}

	memset(bmp_font, 0, sizeof(bitmap_font_t));
	bmp_font->cache = &bitmap_cache[empty_slot];
	memset(bmp_font->cache, 0, sizeof(bitmap_cache_t));

	ret = fs_open(&bmp_font->font_fp, file_path,FS_O_READ);

	if ( ret < 0 )
	{
		SYS_LOG_ERR(" open font file %s failed! \n", file_path);
		goto ERR_EXIT;
	}

    fs_seek(&bmp_font->font_fp, 0, FS_SEEK_END);
	file_size = fs_tell(&bmp_font->font_fp);
	fs_seek(&bmp_font->font_fp, 0, FS_SEEK_SET);

	ret = fs_read(&bmp_font->font_fp, &head_size, 4);
	if ( ret < 4 )
	{
		SYS_LOG_ERR( "read font head failed !!! " );
		goto ERR_EXIT;
	}

	fs_seek(&bmp_font->font_fp, 0, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &head_size, 4);
	bmp_font->cmap_offset = head_size;

	fs_seek(&bmp_font->font_fp, LVGL_FONT_SIZE_OFFSET, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &bmp_font->font_size, 2);

	fs_seek(&bmp_font->font_fp, LVGL_FONT_ASCENT_OFFSET, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &bmp_font->ascent, 2);

	fs_seek(&bmp_font->font_fp, LVGL_FONT_DESCENT_OFFSET, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &bmp_font->descent, 2);

	fs_seek(&bmp_font->font_fp, LVGL_FONT_ADVANCE_OFFSET, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &bmp_font->default_advance, 2);

	fs_seek(&bmp_font->font_fp, LVGL_FONT_LOCA_FMT_OFFSET, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->loca_format = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->glyfid_format = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->adw_format = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bpp = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bbxy_length = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->bbwh_length = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->adw_length = attr_tmp;

	ret = fs_read(&bmp_font->font_fp, &attr_tmp, 1);
	bmp_font->compress_alg = attr_tmp;

	if(bmp_font->adw_format == 1)
	{
		bmp_font->default_advance = bmp_font->default_advance >> 4;
	}

	SYS_LOG_INF("font attr size %d, asc %d, desc %d, adw %d\n", bmp_font->font_size, bmp_font->ascent, bmp_font->descent, bmp_font->default_advance);
	SYS_LOG_INF("font attr adw fmt %d, adw len %d, bbxy len %d, bbwh len %d\n", bmp_font->adw_format, bmp_font->adw_length, bmp_font->bbxy_length, bmp_font->bbwh_length);
	SYS_LOG_INF("font compress_alg %d\n", bmp_font->compress_alg);

	//FIXME: cache cmap
	fs_seek(&bmp_font->font_fp, bmp_font->cmap_offset, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &cmap_size, 4);
	bmp_font->loca_offset = bmp_font->cmap_offset + cmap_size;
	//get subtable count

	fs_seek(&bmp_font->font_fp, bmp_font->cmap_offset+8, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &cmap_sub_count, 4);
	bmp_font->cmap_sub_headers = (uint8_t*)bitmap_font_cache_malloc(cmap_size);
    if(bmp_font->cmap_sub_headers == NULL)
    {
        SYS_LOG_ERR("font cmap buffer malloc failed\n");
        goto ERR_EXIT;
    }
	bmp_font->cmap_sub_count = cmap_sub_count;

	fs_read(&bmp_font->font_fp, bmp_font->cmap_sub_headers, cmap_size-12);

	cmap_sub_data = (uint8_t*)bmp_font->cmap_sub_headers + cmap_sub_count*16;

	//read loca offset, loca table too big for cache
	fs_seek(&bmp_font->font_fp, bmp_font->loca_offset, FS_SEEK_SET);
	ret = fs_read(&bmp_font->font_fp, &loca_size, 4);
	bmp_font->glyf_offset = bmp_font->loca_offset + loca_size;
	bmp_font->ref_count = 1;

	uint32_t bmp_size = (bmp_font->font_size+7)/(8/bmp_font->bpp);
	bmp_size *= bmp_font->font_size;
	bmp_size = ((bmp_size + 3)/4) * 4;
	bmp_font->cache->unit_size = bmp_size;
	SYS_LOG_INF("font attr per data size %d, max cached number %d\n", bmp_font->cache->unit_size, bmp_font->cache->cached_max);
	ret = _bitmap_cache_init(bmp_font->cache, file_path, file_size);
	if(ret < 0)
	{
		SYS_LOG_INF("font cache init failed\n");
		goto ERR_EXIT;
	}

	strcpy(bmp_font->font_path, file_path);

	bitmap_font_set_default_bitmap(bmp_font, NULL, 0, 0, 0, 0);

	if(bmp_font->compress_alg == 1)
	{
		int in_size = bmp_size*3/2;
		int tmp_w = ((bmp_font->font_size+3)/4)*4*2;
		bitmap_font_get_decompress_param(bmp_size, bmp_font->font_size, &in_size, &tmp_w);
		if(in_size > decomp_buf_size)
		{
			if(decomp_in_buf)
			{
				bitmap_font_cache_free(decomp_in_buf);
				decomp_in_buf = NULL;
			}

			decomp_in_buf = bitmap_font_cache_malloc(in_size);
			if(!decomp_in_buf)
			{
				SYS_LOG_INF("malloc decompress buffer faild\n");
				goto ERR_EXIT;
			}

			decomp_buf_size = in_size;
		}

		if(tmp_w > decomp_tmp_size)
		{
			if(decomp_tmp1)
			{
				bitmap_font_cache_free(decomp_tmp1);
				decomp_tmp1 = NULL;
			}

			if(decomp_tmp2)
			{
				bitmap_font_cache_free(decomp_tmp2);
				decomp_tmp2 = NULL;
			}

			decomp_tmp1 = bitmap_font_cache_malloc(tmp_w);
			decomp_tmp2 = bitmap_font_cache_malloc(tmp_w);
			if(!decomp_tmp1 || !decomp_tmp2)
			{
				bitmap_font_cache_free(decomp_in_buf);
				decomp_in_buf = NULL;
				SYS_LOG_INF("malloc decompress tmp buf failed\n");
				goto ERR_EXIT;
			}

			decomp_tmp_size = tmp_w;
		}		
	}
	
	return bmp_font;
ERR_EXIT:
	fs_close(&bmp_font->font_fp);
	if(bmp_font->cmap_sub_headers)
	{
	    bitmap_font_cache_free(bmp_font->cmap_sub_headers);
	}

	memset(bmp_font, 0, sizeof(bitmap_font_t));
	return NULL;
}

void bitmap_emoji_font_close(bitmap_emoji_font_t* emoji_font)
{
	if(emoji_font != NULL)
	{
		if(opend_emoji_font.inited)
		{
			if(opend_emoji_font.ref_count > 1)
			{
				opend_emoji_font.ref_count--;
			}
			else
			{
				fs_close(&opend_emoji_font.font_fp);
				if(opend_emoji_font.glyf_list)
				{
				    bitmap_font_cache_free(opend_emoji_font.glyf_list);
				}

                bitmap_font_cache_free(opend_emoji_font.cache->glyph_index);
                opend_emoji_font.cache->glyph_index = NULL;
                opend_emoji_font.cache->metrics = NULL;
                opend_emoji_font.cache->data = NULL;

				opend_emoji_font.cache->inited = 0;
				memset(&opend_emoji_font, 0, sizeof(bitmap_emoji_font_t));
			}
		}
	}
}

void bitmap_font_close(bitmap_font_t* bmp_font)
{
	int32_t i;

	if(bmp_font != NULL)
	{
		for(i=0;i<max_fonts;i++)
		{
			if(bmp_font == &opend_font[i])
			{
				break;
			}
		}

		if(i >= max_fonts)
		{
			return;
		}

		if(opend_font[i].font_path[0] != 0)
		{
			if(opend_font[i].ref_count > 1)
			{
				opend_font[i].ref_count--;
			}
			else
			{
				fs_close(&opend_font[i].font_fp);
				if(opend_font[i].cmap_sub_headers)
				{
				    bitmap_font_cache_free(opend_font[i].cmap_sub_headers);
				}
				memset(&opend_font[i], 0, sizeof(bitmap_font_t));

                if(bitmap_cache[i].glyph_index)
                {
                    bitmap_font_cache_free(bitmap_cache[i].glyph_index);
                    bitmap_cache[i].glyph_index = NULL;
                    bitmap_cache[i].metrics = NULL;
                    bitmap_cache[i].data = NULL;
                }

				if(bitmap_cache[i].inited == 1)
				{
					bitmap_cache[i].inited = 0;
				}
			}
		}


	}
}

int bitmap_font_set_default_emoji_code(bitmap_emoji_font_t* font, uint32_t emoji_code)
{
	int ret;
	int glyf_id=0;	
	uint32_t glyf_loca;
	uint32_t glyf_size;
	glyph_metrics_t* metric_item = NULL;
	emoji_font_entry_t* entry = NULL;
	uint8_t* data;
	int multi;
	int cache_index;

	if(font)
	{
		if(font->default_code == emoji_code && font->cache->default_data != NULL)
		{
			return 0;
		}
		
		ret = _search_emoji_glyf_id(font, emoji_code, &glyf_id);	
		if(ret < 0)
		{
			SYS_LOG_INF("wrong default emoji code, not found\n");
			font->default_code = font->first_code;
		}
		else
		{
			font->default_code = emoji_code;
		}

		metric_item = &font->cache->default_metric;
		
		entry = font->glyf_list + glyf_id;
		metric_item->advance = entry->advance;
		metric_item->bbx = 0;
		metric_item->bby = entry->descent;
		metric_item->bbw = entry->width;
		metric_item->bbh = entry->height;
		glyf_size = entry->width*entry->height*font->bpp;
		glyf_loca = entry->offset;

		if(!emoji_font_use_mmap())
		{
			ret = fs_seek(&font->font_fp, glyf_loca, FS_SEEK_SET);
			if(ret < 0)
			{
				SYS_LOG_ERR("%d seek font file error %d\n", __LINE__, glyf_loca);
				return -1;
			}

			SYS_LOG_INF("default size %d, unit size %d, max %d\n", glyf_size, font->cache->unit_size, font->cache->cached_max);
			//default code first read, malloc
			multi = glyf_size/font->cache->unit_size + 1;
			cache_index = font->cache->cached_max-multi;
			font->cache->default_data = &(font->cache->data[cache_index*font->cache->unit_size]);
			data = font->cache->default_data + sizeof(lv_image_dsc_t);

			ret = fs_read(&font->font_fp, data, glyf_size);
			if(ret < glyf_size)
			{
				SYS_LOG_ERR("read font file error\n");
				return -1;
			}		

			lv_image_dsc_t* emoji_dsc = (lv_image_dsc_t*)font->cache->default_data;
			emoji_dsc->data = data;
			emoji_dsc->header.w = entry->width;
			emoji_dsc->header.h = entry->height;
			emoji_dsc->header.stride = entry->width * 3;
			emoji_dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
			emoji_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
			emoji_dsc->header.flags = 0;
			emoji_dsc->data_size = entry->width*entry->height*3;			
			font->cache->cached_max -= multi;
		}
		else
		{
			cache_index = font->cache->cached_max-1;
			font->cache->default_data = &(font->cache->data[cache_index*font->cache->unit_size]);

			lv_image_dsc_t* emoji_dsc = (lv_image_dsc_t*)font->cache->default_data;
			emoji_dsc->data = (uint8_t*)font->emoji_mmap_addr+glyf_loca;
			emoji_dsc->header.w = entry->width;
			emoji_dsc->header.h = entry->height;
			emoji_dsc->header.stride = entry->width * 3;
			emoji_dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
			emoji_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
			emoji_dsc->header.flags = 0;
			emoji_dsc->data_size = entry->width*entry->height*3;
			font->cache->cached_max--;
		}
	}

	return 0;
}

int bitmap_font_set_default_code(bitmap_font_t* font, uint32_t letter_code)
{
	if(font)
	{
		font->default_code = letter_code;
	}

	return 0;
}

int bitmap_font_set_default_bitmap(bitmap_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp)
{
	glyph_metrics_t* metric_item = NULL;
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

			bmp_size = metric_item->bbw*metric_item->bbh;
			if(font->bpp == 4)
			{
				if(bmp_size%2 != 0)
				{
					bmp_size = bmp_size/2 + 1;
				}
				else
				{
					bmp_size = bmp_size/2;
				}
			}
			else if(font->bpp == 2)
			{
				if(bmp_size%4 != 0)
				{
					bmp_size = bmp_size/4 + 1;
				}
				else
				{
					bmp_size = bmp_size/4;
				}
			}
			else if(font->bpp == 1)
			{
				if(bmp_size%8 != 0)
				{
					bmp_size = bmp_size/8 + 1;
				}
				else
				{
					bmp_size = bmp_size/4;
				}
			}			

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

			bmp_size = metric_item->bbw*metric_item->bbh;
			if(font->bpp == 4)
			{
				if(bmp_size%2 != 0)
				{
					bmp_size = bmp_size/2 + 1;
				}
				else
				{
					bmp_size = bmp_size/2;
				}
			}
			else if(font->bpp == 2)
			{
				if(bmp_size%4 != 0)
				{
					bmp_size = bmp_size/4 + 1;
				}
				else
				{
					bmp_size = bmp_size/4;
				}
			}
			else if(font->bpp == 1)
			{
				if(bmp_size%8 != 0)
				{
					bmp_size = bmp_size/8 + 1;
				}
				else
				{
					bmp_size = bmp_size/4;
				}
			}

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

int bitmap_font_cache_preset(bitmap_font_cache_preset_t* preset, int count)
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

    font_cache_presets = bitmap_font_cache_malloc(sizeof(bitmap_font_cache_preset_t)*count);
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
        font_cache_presets[i].cache_size_preset = preset[i].cache_size_preset;
    }
    font_cache_preset_num = count;

    return 0;
}


#if USE_BSEARCH_IN_GLYPH_ID > 0
static int unicode_to_glyph_compare(const void *key, const void *element)
{
	const uint16_t *key16 = key;
	const uint16_t *elem16 = element;

	return *key16 - *elem16;
}
#endif

uint32_t _get_glyf_id_cached(bitmap_cache_t* cache, uint8_t* cmap_headers, uint32_t cmap_sub_count, uint32_t unicode)
{
	int32_t i;
	uint32_t glyf_id;
	cmap_sub_header_t* sub_header;

#if MAX_CACHED_GLYPH_IDS > 1
	if (unicode == cache->last_unicode[cache->last_unicode_idx])
		return cache->last_glyph_id[cache->last_unicode_idx];
	for (i = cache->last_unicode_idx - 1; i >= 0; i--) {
		if (unicode == cache->last_unicode[i])
			return cache->last_glyph_id[i];
	}

	for (i = cache->last_unicode_idx + 1; i < MAX_CACHED_GLYPH_IDS; i++) {
		if (unicode == cache->last_unicode[i])
			return cache->last_glyph_id[i];
	}
#else
	if (unicode == cache->last_unicode[0])
		return cache->last_glyph_id[0];
#endif

	sub_header = (cmap_sub_header_t*)cmap_headers;
//	SYS_LOG_INF("cmap_sub_count %d, cmap_sub_headers 0x%x\n", cmap_sub_count, cmap_headers);
	for(i=0;i<cmap_sub_count;i++)
	{
		if((unicode >= sub_header->range_start)&&
			(unicode < sub_header->range_start+sub_header->range_length))
		{
			//zero mini
			//SYS_LOG_INF("unicode 0x%x, start 0x%0.4x, length 0x%0.4x\n", unicode, sub_header->range_start, sub_header->range_length);
			if(sub_header->sub_format == 0)
			{
				uint8_t delta = (uint8_t)(unicode - sub_header->range_start);
				uint8_t* value = (uint8_t*)cmap_headers + sub_header->data_offset - 12;
				//SYS_LOG_INF("delta %d, value[delta] %d\n", delta, value[delta]);
				glyf_id = value[delta] + sub_header->glyf_id_offset;
				goto ok_exit;
			}
			else if(sub_header->sub_format == 2)
			{
				glyf_id = sub_header->glyf_id_offset + (unicode - sub_header->range_start);
				goto ok_exit;
			}
			else if(sub_header->sub_format == 3)
			{
				uint16_t delta = (uint16_t)(unicode - sub_header->range_start);
				uint8_t* value8 = (uint8_t*)cmap_headers + sub_header->data_offset - 12;
				uint16_t* value = (uint16_t*)value8;

#if USE_BSEARCH_IN_GLYPH_ID > 0
				uint16_t *ptr16 = bsearch(&delta, value, sub_header->entry_count, 2, unicode_to_glyph_compare);
				if (ptr16) {
					glyf_id = sub_header->glyf_id_offset + (uint32_t)(ptr16 - value);
					goto ok_exit;
				}
#else
				int j;
				for (j = 0; j < sub_header->entry_count; j++) {
					if(value[j] == delta) {
						glyf_id = sub_header->glyf_id_offset + j;
						goto ok_exit;
					}
				}
#endif
			}
			else
			{
				SYS_LOG_INF("unsupported subformat %d\n", sub_header->sub_format);
			}
		}
		//how much space do cmap_sub_header_t occupy in reality
		sub_header++;
	}

	if(bitmap_font_glyph_err_print_is_on())
	{
		SYS_LOG_ERR("glyf id not found: 0x%x\n", unicode);
	}

	return 0;
ok_exit:
#if MAX_CACHED_GLYPH_IDS > 1
	cache->last_unicode[cache->curr_unicode_idx] = unicode;
	cache->last_glyph_id[cache->curr_unicode_idx] = glyf_id;
	cache->curr_unicode_idx = (cache->curr_unicode_idx + 1) & (MAX_CACHED_GLYPH_IDS - 1);
	cache->last_unicode_idx = cache->curr_unicode_idx;
#else
	cache->last_unicode[0] = unicode;
	cache->last_glyph_id[0] = glyf_id;
#endif

	return glyf_id;
}

uint32_t _get_glyf_loca(bitmap_font_t* font, uint32_t glyf_id)
{
	uint32_t loca_offset;
	uint32_t glyf_offset;
	uint16_t glyf_offset16;

	if(font->loca_format == 0)
	{
		loca_offset = font->loca_offset+12+2*glyf_id;
	}
	else
	{
		loca_offset = font->loca_offset+12+4*glyf_id;
	}

//	SYS_LOG_INF("loca_origin 0x%x, loca_offset 0x%x, font fp 0x%x\n", font->loca_offset, loca_offset, font->font_fp.filep);
	fs_seek(&font->font_fp, loca_offset, FS_SEEK_SET);
	if(font->loca_format == 0)
	{
		fs_read(&font->font_fp, &glyf_offset16, 2);
		glyf_offset = glyf_offset16;
	}
	else
	{
		fs_read(&font->font_fp, &glyf_offset, 4);
	}
	return glyf_offset;
}

int _try_get_cached_index(bitmap_cache_t* cache, uint32_t glyf_id)
{
	int32_t cindex;

	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return -1;
	}

	if(cache->cached_total > 0)
	{
		if (glyf_id == cache->glyph_index[cache->last_glyph_idx]) {
			return cache->last_glyph_idx;
		}

		cindex = cache->last_glyph_idx + 1;
		for (; cindex < cache->cached_total; cindex++) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_glyph_idx = cindex;
				return cindex;
			}
		}

		cindex = cache->last_glyph_idx - 1;
		for (; cindex >= 0; cindex--) {
			if (glyf_id == cache->glyph_index[cindex]) {
				cache->last_glyph_idx = cindex;
				return cindex;
			}
		}
	}

	return -1;
}

int _get_cache_index(bitmap_cache_t* cache, uint32_t glyf_id, uint32_t slot_num)
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

	cache->last_glyph_idx = cache->current;

	return cache->current;
}

uint8_t* _get_glyph_cache(bitmap_cache_t* cache, uint32_t glyf_id)
{
	int32_t cache_index;
	if(cache == NULL)
	{
		SYS_LOG_ERR("null glyph cache\n");
		return NULL;
	}

	cache_index = _try_get_cached_index(cache, glyf_id);
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

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
int32_t _check_high_freq_chars(uint16_t unicode)
{
	uint16_t key = unicode;

	if(unicode == high_freq_cache.last_unicode_idx)
	{
		return high_freq_cache.last_cache_idx;
	}
	uint16_t *ptr16 = bsearch(&key, high_freq_codes, MAX_HIGH_FREQ_NUM, 2, unicode_to_glyph_compare);
	if(ptr16)
	{
		int32_t cache_index = ptr16 - high_freq_codes;
		high_freq_cache.last_unicode_idx = unicode;
		high_freq_cache.last_cache_idx = cache_index;
		return cache_index;
	}
	else
	{
		return -1;
	}
}
#endif

uint32_t bitmap_font_get_elapse_count(void)
{
	return elapse_count;
}


static int32_t _search_emoji_glyf_id(bitmap_emoji_font_t* font, uint32_t unicode, uint32_t* glyf_id)
{
	emoji_font_entry_t* entry = font->glyf_list;
	int low, mid, high;

	low = 0;
	high = font->glyf_count-1;
	while(low <= high)
	{
		mid = (low+high)/2;
		if(entry[mid].unicode > unicode)
		{
			high = mid - 1;
		}
		else if(entry[mid].unicode < unicode)
		{
			low = mid + 1;
		}
		else
		{
			//found
			*glyf_id = mid;
			return 0;
		}
	}

	return -1;
}

uint8_t * bitmap_font_get_emoji_bitmap(bitmap_emoji_font_t* font, uint32_t unicode)
{
	uint8_t* data;
	int ret;
	int glyf_id = 0;

	//SYS_LOG_INF("get 0x%x data, default code 0x%x\n", unicode, font->default_code);
	if(unicode == font->default_code)
	{
		return font->cache->default_data;
	}

	ret = _search_emoji_glyf_id(font, unicode, &glyf_id);
	if(ret < 0 && font->default_code > 0)
	{
		//SYS_LOG_INF("get unknown code data");
		data = font->cache->default_data;
	}
	else
	{
		data = _get_glyph_cache(font->cache, unicode);
		//SYS_LOG_INF("unicode 0x%x, data %p\n", unicode, data);
		if(data == NULL && font->default_code > 0)
		{
			//SYS_LOG_INF("get cached out code data");
			data = font->cache->default_data;
		}
	}

	return data;
}

glyph_metrics_t* _font_get_emoji_glyph_dsc(bitmap_emoji_font_t* font, uint32_t unicode, uint32_t glyf_id, int32_t cache_index)
{
	uint32_t glyf_loca;
	uint32_t glyf_size;
	glyph_metrics_t* metric_item = NULL;
	emoji_font_entry_t* entry = NULL;
	uint8_t* data;
	int ret;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info %p\n", font);
		return NULL;
	}



	//SYS_LOG_INF("emoji 0x%x, glyf id %d\n",unicode, glyf_id);
	metric_item = &(font->cache->metrics[cache_index]);

	entry = font->glyf_list + glyf_id;
	metric_item->advance = entry->advance;
	metric_item->bbx = 0;
	metric_item->bby = entry->descent;
	metric_item->bbw = entry->width;
	metric_item->bbh = entry->height;
	glyf_size = entry->width*entry->height*font->bpp;
	glyf_loca = entry->offset;

	data = _get_glyph_cache(font->cache, unicode);
	if(data == NULL)
	{
		return NULL;
	}
	
	if(!emoji_font_use_mmap())
	{
		uint8_t* glyf_data = data + sizeof(lv_image_dsc_t);
		ret = fs_seek(&font->font_fp, glyf_loca, FS_SEEK_SET);
		if(ret < 0)
		{
			SYS_LOG_ERR("%d seek font file error\n", __LINE__);
			return NULL;
		}

		ret = fs_read(&font->font_fp, glyf_data, glyf_size);
		if(ret < glyf_size)
		{
			SYS_LOG_ERR("read font file error\n");
			return NULL;
		}
		mem_dcache_clean(glyf_data, glyf_size);

		lv_image_dsc_t* emoji_dsc = (lv_image_dsc_t*)data;
		emoji_dsc->data = glyf_data;
		emoji_dsc->header.w = entry->width;
		emoji_dsc->header.h = entry->height;
		emoji_dsc->header.stride = entry->width * 3;
		emoji_dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
		emoji_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
		emoji_dsc->header.flags = 0;
		emoji_dsc->data_size = entry->width*entry->height*3;	
	}
	else
	{
		//no need to fetch bitmap now, but need to store glyf loca	
		lv_image_dsc_t* emoji_dsc = (lv_image_dsc_t*)data;
		emoji_dsc->data = (uint8_t*)font->emoji_mmap_addr+glyf_loca;
		emoji_dsc->header.w = entry->width;
		emoji_dsc->header.h = entry->height;
		emoji_dsc->header.stride = entry->width * 3;
		emoji_dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
		emoji_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
		emoji_dsc->header.flags = 0;
		emoji_dsc->data_size = entry->width*entry->height*3;
	}

	return metric_item;
}

glyph_metrics_t* bitmap_font_get_emoji_glyph_dsc(bitmap_emoji_font_t* font, uint32_t unicode, bool force_retrieve)
{
	int32_t cache_index;
	glyph_metrics_t* metric_item;
	int ret;
	int32_t glyf_id = 0; //here refers to the index in glyf list

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info, %p\n", font);
		return NULL;
	}

	if(unicode == font->default_code)
	{
		return &font->cache->default_metric;
	}

	cache_index = _try_get_cached_index(font->cache, unicode);
	if(cache_index >= 0)
	{
		//found cached, fill data to glyph dsc

		return &font->cache->metrics[cache_index];
	}
	else
	{

		if((font->cache->cached_total >= font->cache->cached_max) && (force_retrieve == false))
		{
			//SYS_LOG_INF("0x%x cached out\n", unicode);
			return NULL;
		}

		//SYS_LOG_INF("cached total %d, cached max %d\n", font->cache->cached_total, font->cache->cached_max);

		ret = _search_emoji_glyf_id(font, unicode, &glyf_id);
		if(ret < 0)
		{
			if(font->default_code > 0)
			{
				//search for default code instead;
				return &font->cache->default_metric;
			}
			else
			{	
				//nothing, report error;
				SYS_LOG_ERR("neither unicode 0x%x nor default code found\n", unicode);
				return NULL;
			}
		}

		
		cache_index = _get_cache_index(font->cache, unicode, 1);
		metric_item = _font_get_emoji_glyph_dsc(font, unicode, glyf_id, cache_index);
		if(metric_item == NULL)
		{
			SYS_LOG_ERR("no metric item found for 0x%x\n", unicode);
			return NULL;
		}
		return metric_item;
	}

	return NULL;
}

uint8_t * bitmap_font_get_bitmap(bitmap_font_t* font, bitmap_cache_t* cache, uint32_t unicode)
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

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	if(bitmap_font_get_high_freq_enabled())
	{
		int cache_index;
		if(font->font_size == high_freq_cache.font_size)
		{
			cache_index = _check_high_freq_chars((uint16_t)unicode);
	//		SYS_LOG_INF("unicode 0x%x, cache_index %d\n", unicode, cache_index);
			if(cache_index >= 0)
			{
				return high_freq_cache.data+cache_index*high_freq_cache.unit_size;
			}
		}
	}
#endif

	glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, unicode);
	if(glyf_id == 0 && font->default_code > 0)
	{
		//glyf not found && default code set
		glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, font->default_code);
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
	data = _get_glyph_cache(cache, glyf_id);
	if(data == NULL && font->cache->default_data != NULL)
	{
	    SYS_LOG_INF("null data, return default data\n");
	    return font->cache->default_data;
	}

	return data;
}

void stride_glyf_bitmap(const uint8_t * in, uint8_t * out, int16_t w, int16_t h, uint8_t bpp)
{
    int32_t y;
    int32_t x;
	int32_t k;
	uint8_t out_k;
	uint32_t stride = (w * bpp + 7)/8;
	uint32_t bit_off = 0;
	uint8_t out_x;

	for(y = 0; y < h; y++) {
		if(bpp == 2) {
			if(w % 4 == 0 && bit_off == 0) {
				memcpy(out, in, stride);
				in += stride;
			} else {
				if(bit_off == 0) {
					memcpy(out, in, stride-1);
					out_x = in[stride-1];
					bit_off = (w - 4*(stride-1))*2;
					out[stride-1] = (out_x>>(8-bit_off))<<(8-bit_off);
					in += (stride - 1);
				} else {
					for(x=0,k=0; x < w; k++,x+=4)
					{
						if(x+3<w)
						{
							out_k = in[x/4];
							out[k] = out_k<<bit_off;
							out_k = in[x/4+1]>>(8-bit_off);
							out[k] |= out_k;
						}
						else
						{
							uint8_t tail = (w - x)*2;
							out[k] = in[x/4];
							if(bit_off + tail <= 8)
							{
								out[k] = (out[k]>>(8-bit_off-tail))<<bit_off;
								if(bit_off + tail < 8)
								{
									bit_off = bit_off+tail;
									in += (stride-1);
								}
								else
								{
									bit_off = 0;
									in += stride;
								}
							}
							else
							{
								out[k] = out[k]<<bit_off;
								tail = bit_off+tail-8;
								out_k = (in[x/4+1]>>(8-tail))<<(8-tail);
								bit_off = tail;
								in += stride;
							}
							break;
						}
					}

					if(x >= w)
					{
						in += (stride-1);
					}
					
				}
			}
		} else if(bpp == 4) {
			if(w % 2 == 0 && bit_off == 0) {
				memcpy(out, in, w/2);
				in += stride;
			} else {
				if(bit_off == 0) {
					memcpy(out, in, stride-1);
					out[stride-1] = in[stride-1]&0xf0;
					bit_off = 4;
					in += (stride - 1);
				} else {
					for(x=0;x < w; x+=2)
					{
						if(x+1 < w)
						{
							out[x/2] = ((in[x/2]&0xf)<<4) | ((in[x/2+1]&0xf0)>>4);							
						}
						else
						{
							out[x/2] = ((in[x/2]&0xf)<<4);
							bit_off = 0;
							in += stride;
							break;
						}
					}

					if(x >= w)
					{
						bit_off = 4;
						in += (stride - 1);
					}
					
				}				
			}		
		} else {

		}

		out += stride;
	}	
}

glyph_metrics_t* _font_get_glyph_dsc(bitmap_font_t* font, bitmap_cache_t* cache, high_freq_cache_t* hcache,  uint32_t glyf_id, int32_t* pcache_index, uint32_t load_cache_type)
{
	uint32_t glyf_loca;
	uint32_t metric_len;
	uint32_t metric_off;
	uint32_t bits_total;
	uint32_t bits_off;
	uint32_t off;
	uint32_t bmp_size;
	uint8_t* data = NULL;
	uint8_t* cache_data = NULL;
	uint8_t* metrics;
	bbxy_t tmp;
	glyph_metrics_t* metric_item = NULL;
	glyph_metrics_t metric_data;
	int ret;
	uint8_t* out_dest = NULL;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info, %p, %p\n", font, cache);
		return NULL;
	}


	glyf_loca = _get_glyf_loca(font, glyf_id);
	ret = fs_seek(&font->font_fp, glyf_loca+font->glyf_offset, FS_SEEK_SET);
	if(ret < 0)
	{
		SYS_LOG_ERR("%d seek font file error\n", __LINE__);
		return NULL;
	}


	bits_total = font->adw_length+2*font->bbxy_length+2*font->bbwh_length;
	metric_len = (bits_total+7)/8;

	metrics = (uint8_t*)metrics32;
	ret = fs_read(&font->font_fp, metrics, metric_len);
	if(ret < metric_len)
	{
		SYS_LOG_ERR("read font file error\n");
		return NULL;
	}
	metric_off = 0;

    metric_item = &metric_data;

	if(font->adw_length <= 8)
	{
		if(font->adw_length == 0)
		{
			metric_item->advance = font->default_advance;
		}
		else
		{
			metric_item->advance = (uint32_t)(metrics[0]>>(8-font->adw_length));
		}
		bits_off = font->adw_length;
	}
	else
	{
		//suppose adw length less than 16
		if(font->adw_format == 1)
		{
			uint32_t adw_len = font->adw_length-4;
			if(adw_len <= 8)
			{
				metric_item->advance = (uint32_t)(metrics[0]>>(8-adw_len));
			}
			else
			{
				metric_item->advance = (uint32_t)((metrics[0]<<(adw_len-8))|(metrics[1]>>(16-adw_len)));
			}
			metric_off++;
		}
		else
		{
			metric_item->advance = (uint32_t)((metrics[0]<<(font->adw_length-8))|(metrics[1]>>(16-font->adw_length)));
			metric_off++;
		}
		bits_off = font->adw_length - 8;
	}

	if(font->bbxy_length <= 8 - bits_off)
	{
		tmp.xy = (metrics[metric_off]<<bits_off)>>(8-font->bbxy_length);
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bbx = tmp.bits2;
			break;
		case 3:
			metric_item->bbx = tmp.bits3;
			break;
		case 4:
			metric_item->bbx = tmp.bits4;
			break;
		case 5:
			metric_item->bbx = tmp.bits5;
			break;
		case 6:
			metric_item->bbx = tmp.bits6;
			break;
		case 7:
			metric_item->bbx = tmp.bits7;
			break;
		default:
			metric_item->bbx = tmp.xy;
			break;
		}
		bits_off = bits_off + font->bbxy_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbxy_length - bits_off));
		tmp.xy = p1<<(font->bbxy_length + bits_off - 8) | p2;
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bbx = tmp.bits2;
			break;
		case 3:
			metric_item->bbx = tmp.bits3;
			break;
		case 4:
			metric_item->bbx = tmp.bits4;
			break;
		case 5:
			metric_item->bbx = tmp.bits5;
			break;
		case 6:
			metric_item->bbx = tmp.bits6;
			break;
		case 7:
			metric_item->bbx = tmp.bits7;
			break;
		default:
			metric_item->bbx = tmp.xy;
			break;
		}

		bits_off = font->bbxy_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}

	if(font->bbxy_length <= 8 - bits_off)
	{
		tmp.xy = (metrics[metric_off]<<bits_off)>>(8-font->bbxy_length);
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bby = tmp.bits2;
			break;
		case 3:
			metric_item->bby = tmp.bits3;
			break;
		case 4:
			metric_item->bby = tmp.bits4;
			break;
		case 5:
			metric_item->bby = tmp.bits5;
			break;
		case 6:
			metric_item->bby = tmp.bits6;
			break;
		case 7:
			metric_item->bby = tmp.bits7;
			break;
		default:
			metric_item->bby = tmp.xy;
			break;
		}
		bits_off = bits_off + font->bbxy_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbxy_length - bits_off));
		tmp.xy = p1<<(font->bbxy_length + bits_off - 8) | p2;
		switch(font->bbxy_length)
		{
		case 2:
			metric_item->bby = tmp.bits2;
			break;
		case 3:
			metric_item->bby = tmp.bits3;
			break;
		case 4:
			metric_item->bby = tmp.bits4;
			break;
		case 5:
			metric_item->bby = tmp.bits5;
			break;
		case 6:
			metric_item->bby = tmp.bits6;
			break;
		case 7:
			metric_item->bby = tmp.bits7;
			break;
		default:
			metric_item->bby = tmp.xy;
			break;
		}
		bits_off = font->bbxy_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}

	if(font->bbwh_length <= 8 - bits_off)
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		metric_item->bbw = p1>>(8-font->bbwh_length);
//		metric_item->bbw = (metrics[metric_off]<<bits_off)>>bits_off;
		bits_off = bits_off + font->bbwh_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbwh_length - bits_off));
		metric_item->bbw = p1<<(font->bbwh_length + bits_off - 8) | p2;
		bits_off = font->bbwh_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}

	if(font->bbwh_length <= 8 - bits_off)
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		metric_item->bbh = p1>>(8-font->bbwh_length);
		bits_off = bits_off + font->bbwh_length;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}
	}
	else
	{
		uint8_t p1 = (metrics[metric_off]<<bits_off);
		p1 = p1>>bits_off;
		metric_off++;
		uint8_t p2 = (metrics[metric_off]>>(16- font->bbwh_length - bits_off));
		metric_item->bbh = p1<<(font->bbwh_length + bits_off - 8) | p2;
		bits_off = font->bbwh_length + bits_off - 8;
		if(bits_off == 8)
		{
			bits_off = 0;
			metric_off++;
		}

	}

	uint32_t stride = (metric_item->bbw * font->bpp +7)/8;
	bmp_size = stride*metric_item->bbh;

    int slots = bmp_size/cache->unit_size;
    if(bmp_size%cache->unit_size != 0 || bmp_size == 0)
    {
        slots += 1;
    }
    int cache_index = _get_cache_index(cache, glyf_id, slots);

	if(hcache != NULL)
	{
		metric_item = &(hcache->metrics[cache_index]);
		cache_data = hcache->data+hcache->unit_size*cache_index;
	}
	else
	{
		metric_item = &(cache->metrics[cache_index]);
		cache_data = _get_glyph_cache(cache, glyf_id);
	}

	if(cache_data == NULL || metric_item == NULL)
	{
		SYS_LOG_ERR("get glyph cache faild for glyph id %d\n", glyf_id);
		return NULL;
	}

    *pcache_index = cache_index;
    memcpy(metric_item, &metric_data, sizeof(glyph_metrics_t));


	data = cache_data;
	/*it's necessary to read bitmap to a seperate buffer, for striding or decompress*/
	{
		//realloc decompress buffer if glyf too big and keep the buffer
		if(bmp_size > decomp_buf_size)
		{
			if(decomp_in_buf)
			{
				bitmap_font_cache_free(decomp_in_buf);
				decomp_in_buf = NULL;
			}
			decomp_in_buf = bitmap_font_cache_malloc(bmp_size+4);
			decomp_buf_size = bmp_size;
		}
	
		out_dest = data;
		data = decomp_in_buf;
		if(data == NULL)
		{
			SYS_LOG_INF("malloc glyf compressed data failed\n");
			data = out_dest;
			out_dest = NULL;
		}
	}

	off = 0;
	if(bits_off != 0)
	{
		fs_read(&font->font_fp, data+4, bmp_size);
		data[off] =  (metrics[metric_off]<<bits_off)|(data[4]>>(8-bits_off));
		off++;
		while(off < bmp_size)
		{
			data[off] = (data[off+3]<<bits_off)|(data[off+4]>>(8-bits_off));
			off++;
		}
	}
	else
	{
		fs_read(&font->font_fp, data, bmp_size);
	}

	memset(out_dest, 0, bmp_size);
	if(font->compress_alg == 1 && out_dest != NULL) {
		//compressed data, decompress it
	    decompress_glyf_bitmap(data, out_dest, metric_item->bbw, metric_item->bbh, (uint8_t)font->bpp, true, decomp_tmp1, decomp_tmp2);		
	} else {
		//stride every line
		stride_glyf_bitmap(data, out_dest, metric_item->bbw, metric_item->bbh, (uint8_t)font->bpp);
	}

//	SYS_LOG_INF("adw %d x %d y %d w %d h %d\n", metric_item->advance, metric_item->bbx, metric_item->bby, metric_item->bbw, metric_item->bbh);
	return metric_item;

}

glyph_metrics_t* bitmap_font_get_glyph_dsc(bitmap_font_t* font, bitmap_cache_t *cache, uint32_t unicode)
{
	uint32_t glyf_id;
	int32_t cache_index;
	glyph_metrics_t* metric_item;

	if((font == NULL) || (cache == NULL))
	{
		SYS_LOG_ERR("unicode 0x%x, null font info, %p, %p\n", unicode, font, cache);
		return NULL;
	}

	if(unicode == 0)
	{
		unicode = 0x20;
	}

#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	if(bitmap_font_get_high_freq_enabled())
	{
		if(font->font_size == high_freq_cache.font_size)
		{
			cache_index = _check_high_freq_chars((uint16_t)unicode);
			if(cache_index >= 0)
			{
				return &(high_freq_cache.metrics[cache_index]);
			}
		}
	}
#endif

	glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, unicode);
	if(glyf_id == 0 && font->default_code > 0)
	{
		//glyf not found && default code set
		glyf_id = _get_glyf_id_cached(cache, font->cmap_sub_headers, font->cmap_sub_count, font->default_code);
	}

	if(glyf_id == 0)
	{
		//glyf not found with no default code or default code not exsit
		if(font->cache->default_data != NULL)
		{
			return &font->cache->default_metric;
		}
		glyf_id = 1;
	}

	cache_index = _try_get_cached_index(cache, glyf_id);
	if(cache_index >= 0)
	{
		//found cached, fill data to glyph dsc

		return &cache->metrics[cache_index];
	}
	else
	{
		if(bitmap_font_glyph_debug_is_on())
		{
			SYS_LOG_INF("no cache 0x%x\n", unicode);
		}
//		cache_index = _get_cache_index(cache, glyf_id, 1);

		metric_item = _font_get_glyph_dsc(font, cache, NULL, glyf_id, &cache_index, CACHE_TYPE_NORMAL);
		if(metric_item == NULL)
		{
			return NULL;
		}

		SYS_LOG_INF("unicode 0x%x, metrics %d %d %d %d\n", unicode, metric_item->bbx, metric_item->bby, metric_item->bbw, metric_item->bbh);
		//FIXME: no way to adjust vertical position of some letters automatically
		if(unicode == 0x4e00  || unicode == 0x2014 || unicode == 0xbbd2)
		{
			int32_t diff = metric_item->bby - metric_item->bbh;
			if(diff > font->font_size/2)
			{
				metric_item->bby -= font->font_size/3;
			}
		}
		return metric_item;
	}

}

void bitmap_font_dump_info(void)
{
    int i;
	int cached_size;
	int per_size;
    SYS_LOG_INF("bitmap font info dump:\n");
    for(i=0;i<max_fonts;i++)
    {
        if(opend_font[i].font_fp.filep != NULL)
        {
        	per_size = opend_font[i].cache->unit_size+sizeof(glyph_metrics_t)+sizeof(uint32_t);
        	cached_size = (opend_font[i].cache->cached_total+2)*per_size;
            SYS_LOG_INF("font %d, path %s, metric buf %p, data buf %p, cached total %d, cached max %d, cache size now %d, cache size max %d\n", 
						i, opend_font[i].font_path, opend_font[i].cache->metrics, opend_font[i].cache->data, 
						opend_font[i].cache->cached_total+2, opend_font[i].cache->cached_max, cached_size, opend_font[i].cache->cache_max_size);
        }
    }
    bitmap_font_cache_dump_info();
}

void bitmap_font_load_high_freq_chars(const uint8_t* file_path)
{
#ifndef CONFIG_SIMULATOR
#ifdef CONFIG_BITMAP_FONT_USE_HIGH_FREQ_CACHE
	uint32_t glyf_id;
	uint32_t i;
	glyph_metrics_t* metric_item;
	int ret;
	struct fs_file_t hffp;
	bitmap_font_t* font = NULL;
	char* high_freq_bin_path;
	char* bin_name;
	int cache_index;

	if(!bitmap_font_get_high_freq_enabled())
	{
		return;
	}


	if(file_path == NULL)
	{
		SYS_LOG_INF("null font path\n");
		return;
	}

	if(high_freq_cache.font_size != 0)
	{
		SYS_LOG_INF("high freq bin loaded\n");
		return;
	}

	high_freq_bin_path = (char*)mem_malloc(strlen(file_path)+12);
	if(high_freq_bin_path == NULL)
	{
		SYS_LOG_INF("high freq bin load failed\n");
		return;
	}
	memset(high_freq_bin_path, 0, strlen(file_path)+12);
	strcpy(high_freq_bin_path, file_path);
	bin_name = strrchr(high_freq_bin_path, '/')+1;
	strcpy(bin_name, DEF_HIGH_FREQ_BIN);

	memset(&hffp, 0, sizeof(struct fs_file_t));
	ret = fs_open(&hffp, high_freq_bin_path, FS_O_READ);
    if ( ret < 0 )
    {
        SYS_LOG_ERR(" open high freq file %s failed! %d \n", high_freq_bin_path, ret);
		mem_free(high_freq_bin_path);
		return;
    }

	ret = fs_read(&hffp, high_freq_codes, sizeof(uint16_t)*MAX_HIGH_FREQ_NUM);
	if(ret < sizeof(uint16_t)*MAX_HIGH_FREQ_NUM)
	{
		SYS_LOG_ERR("read high freq codes failed\n");
		fs_close(&hffp);
		mem_free(high_freq_bin_path);
		return;
	}
	fs_close(&hffp);
	mem_free(high_freq_bin_path);

	font = bitmap_font_open(file_path);
	if(font == NULL)
	{
		SYS_LOG_ERR("failed to open font file to load high freqency codes");
		return;
	}

	high_freq_cache.unit_size = (font->font_size*font->font_size)/4;
	SYS_LOG_INF("high freq fontsize %d, unitsize %d\n", high_freq_cache.font_size, high_freq_cache.unit_size);
	for(i=0;i<MAX_HIGH_FREQ_NUM;i++)
	{
		glyf_id = _get_glyf_id_cached(font->cache, font->cmap_sub_headers, font->cmap_sub_count, (uint32_t)high_freq_codes[i]);
		if(glyf_id == 0)
		{
			SYS_LOG_INF("high freq glyf not found 0x%x\n", high_freq_codes[i]);
			continue;
		}
		metric_item = _font_get_glyph_dsc(font, NULL, &high_freq_cache, glyf_id, &cache_index, CACHE_TYPE_HIGH_FREQ);
		if(metric_item == NULL)
		{
			SYS_LOG_INF("high freq metric not found 0x%x\n", high_freq_codes[i]);
			continue;
		}

		//FIXME: no way to adjust vertical position of some letters automatically
		if(high_freq_codes[i] == 0x4e00  || high_freq_codes[i] == 0x2014 || high_freq_codes[i] == 0xbbd2)
		{
			int32_t diff = metric_item->bby - metric_item->bbh;
			if(diff > font->font_size/2)
			{
				metric_item->bby -= font->font_size/3;
			}
		}

	}

	SYS_LOG_INF("high freq preview:\n");
	for(i=0;i<16;i++)
	{
		printf("0x%x, ", high_freq_codes[i]);
	}
	SYS_LOG_INF("\n");
	high_freq_cache.font_size = font->font_size;
	for(i='0';i<='9';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}
	for(i='a';i<='z';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}
	for(i='A';i<='Z';i++)
	{
		bitmap_font_get_glyph_dsc(font, font->cache, i);
	}

//	bitmap_font_get_glyph_dsc(font, font->cache, 0x2e);
	bitmap_font_get_glyph_dsc(font, font->cache, 0xff0c);
	bitmap_font_get_glyph_dsc(font, font->cache, 0x3002);
//	bitmap_font_close(font);
#else
    return;
#endif
#else //SIMULATOR
	return;
#endif
}


