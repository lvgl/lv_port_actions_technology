#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include "../font/font_mempool.h"
#include <lvgl/lvgl_freetype_font.h>
#include <lvgl/src/draw/vg_lite/lv_draw_vg_lite_type.h>
#include <math.h>

#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
static const lv_font_fmt_txt_dsc_t g_freetype_svgfont_dsc = {
	.bpp = LV_FONT_GLYPH_FORMAT_VECTOR,
};
#endif

static const lv_font_fmt_txt_dsc_t g_freetype_bmpfont_dsc = {
	.bpp = CONFIG_FREETYPE_FONT_BITMAP_BPP,
};

static lv_draw_buf_t glyf_draw_buf;

/*
static uint32_t tstart, tend;
static uint32_t ttotal, taverage;
static uint32_t tcount;
*/
bool freetype_font_get_glyph_dsc_cb(const lv_font_t * lv_font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode, uint32_t unicode_next)
{	
	bbox_metrics_t* metric;
	lv_font_fmt_freetype_dsc_t * dsc = (lv_font_fmt_freetype_dsc_t *)(lv_font->user_data);

#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	float scale = 0.0;
#endif

	SYS_LOG_DBG("lv font %p, font %p, unicode 0x%x, func %p\n", lv_font, dsc->font, unicode, __func__);

	dsc_out->gid.index = unicode;

	if(unicode < 0x20) 
	{
		dsc_out->adv_w = 0;
		dsc_out->box_h = 0;
		dsc_out->box_w = 0;
		dsc_out->ofs_x = 0;
		dsc_out->ofs_y = 0;
		dsc_out->format = (lv_font_glyph_format_t)dsc->font->bpp;
		return true;
	}

#ifdef CONFIG_BITMAP_FONT
	if(unicode >= 0x1F300)
	{
		if(bitmap_font_get_max_emoji_num() == 0)
		{
			dsc_out->adv_w = 0;
			dsc_out->box_h = 0;
			dsc_out->box_w = 0;
			dsc_out->ofs_x = 0;
			dsc_out->ofs_y = 0;
			dsc_out->format = (lv_font_glyph_format_t)dsc->font->bpp;
			return true;			
		}
		else
		{
			metric = (bbox_metrics_t*)bitmap_font_get_emoji_glyph_dsc(dsc->emoji_font, unicode, true);
		}
	}
	else
#endif
	{
		//tstart = os_cycle_get_32();
#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
		if(lv_font->dsc == &g_freetype_bmpfont_dsc)
		{
			metric = freetype_font_get_glyph_dsc(dsc->font, dsc->cache, unicode);
		}
		else
		{
			metric = freetype_font_get_glyph_dsc(dsc->font, dsc->font->shape_cache, unicode);
		}
#else
		metric = freetype_font_get_glyph_dsc(dsc->font, dsc->cache, unicode);
#endif
		//tend = os_cycle_get_32();

/*
		taverage = k_cyc_to_us_near32(tend-tstart);
		if(taverage > 50)
		{
			SYS_LOG_INF("0x%x get dsc %d us\n", unicode, taverage);
		}
*/
/*
		taverage = 	k_cyc_to_us_near32(tend-tstart);
		if(taverage > 200 && unicode != 0x20)
		{
			ttotal = ttotal + taverage;
			tcount++;
			if(tcount % 30 == 0)
			{
				taverage = ttotal/tcount;
				SYS_LOG_INF("30 average is %d us\n", taverage);
				ttotal = 0;
				taverage = 0;
				tcount = 0;
			}
		}
*/	
	}

	SYS_LOG_DBG("font size %d, metric size %d\n", dsc->font->font_size, metric->metric_size);


	if(metric == NULL)
	{
		dsc_out->adv_w = 0;
		dsc_out->box_h = 0;
		dsc_out->box_w = 0;
		dsc_out->ofs_x = 0;
		dsc_out->ofs_y = 0;
		dsc_out->format = (lv_font_glyph_format_t)dsc->font->bpp;
		return true;		
	}

	//use scale to compute metrics is not accurate, just load metric from freetype
#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	if(unicode >= 0x1F300)
	{
		dsc_out->adv_w = (uint16_t)(metric->advance);
		dsc_out->box_h = (uint16_t)(metric->bbh); 		/*Height of the bitmap in [px]*/
		dsc_out->box_w = (uint16_t)(metric->bbw);		 /*Width of the bitmap in [px]*/
		dsc_out->ofs_x = (int16_t)metric->bbx; 		/*X offset of the bitmap in [pf]*/
		dsc_out->ofs_y = (int16_t)metric->bby;		  /*Y offset of the bitmap measured from the as line*/

	}	
	else
	{
		scale = (float)(dsc->font->font_size)/(float)(metric->metric_size);
		dsc_out->adv_w = (uint16_t)ceil(metric->advance*scale);
		dsc_out->box_h = (uint16_t)ceil(metric->bbh*scale); 		/*Height of the bitmap in [px]*/
		dsc_out->box_w = (uint16_t)ceil(metric->bbw*scale);		 /*Width of the bitmap in [px]*/
		dsc_out->ofs_x = (int16_t)ceil(metric->bbx*scale); 		/*X offset of the bitmap in [pf]*/
		dsc_out->ofs_y = (int16_t)ceil(metric->bby*scale);		  /*Y offset of the bitmap measured from the as line*/
	}
#else
	dsc_out->adv_w = (uint16_t)(metric->advance);
	dsc_out->box_h = (uint16_t)(metric->bbh); 		/*Height of the bitmap in [px]*/
	dsc_out->box_w = (uint16_t)(metric->bbw);		 /*Width of the bitmap in [px]*/
	dsc_out->ofs_x = metric->bbx; 		/*X offset of the bitmap in [pf]*/
	dsc_out->ofs_y = metric->bby;		  /*Y offset of the bitmap measured from the as line*/
#endif

	SYS_LOG_DBG("0x%x, metrics %d %d %d %d, %d\n", unicode, dsc_out->box_h, dsc_out->box_w, dsc_out->ofs_x, dsc_out->ofs_y, dsc_out->adv_w);

	if(unicode >= 0x1F300)
	{
		dsc_out->ofs_x = 0;
		dsc_out->ofs_y = dsc->font->ascent - metric->bbh - metric->bby;
		dsc_out->format = LV_FONT_GLYPH_FORMAT_IMAGE;
		return true;
	}

#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	if(lv_font->dsc == &g_freetype_bmpfont_dsc)
	{
		dsc_out->format = (lv_font_glyph_format_t)dsc->font->bpp; 		/*Bit per pixel: 1/2/4/8*/
	}
	else
	{
		dsc_out->format = LV_FONT_GLYPH_FORMAT_VECTOR;
	}
#else
	dsc_out->format = (lv_font_glyph_format_t)dsc->font->bpp; 		/*Bit per pixel: 1/2/4/8*/
#endif

	if(unicode == 0x20 || unicode == 0xa0)
	{
		dsc_out->format = dsc->font->bpp;
	}

	return true;
}



#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
static lvx_vg_lite_glyph_format_dsc_t ft_svgmap;
#endif
/* Get the bitmap of `unicode_letter` from `font`. */
static const void * freetype_font_get_glyph_bitmap_cb(lv_font_glyph_dsc_t *glyph_dsc, lv_draw_buf_t *draw_buf)
{
	const lv_font_t * font = glyph_dsc->resolved_font;
	uint32_t unicode = glyph_dsc->gid.index;
	uint8_t* data;
	lv_font_fmt_freetype_dsc_t* font_dsc;
	bbox_metrics_t* metric = NULL;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font info, %p\n", font);
		return NULL;
	}
	font_dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;

#ifdef CONFIG_BITMAP_FONT
	if(unicode >= 0x1F300)
	{
		if(bitmap_font_get_max_emoji_num() == 0)
		{
			return NULL;
		}
		else
		{
			data = bitmap_font_get_emoji_bitmap(font_dsc->emoji_font, unicode);
			return data;
		}
	}
	else
#endif		
	{		
#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
		if(font->dsc == &g_freetype_bmpfont_dsc)
		{
			metric = freetype_font_get_glyph_dsc(font_dsc->font, font_dsc->cache, unicode);
			data = freetype_font_get_bitmap(font_dsc->font, font_dsc->cache, unicode);
		}
		else
		{
			float scale = 1.0/64.0;
			data = freetype_font_load_glyph_shape(font_dsc->font, font_dsc->font->shape_cache, unicode, &scale);
			scale = scale/64.0;
			if(data)
			{
				SYS_LOG_DBG("unicode 0x%x, shape scale %f\n", unicode, scale);
				ft_svgmap.scale = scale; /* TODO: set the proper scale factor */
				ft_svgmap.path_len = *(short *)data;
				ft_svgmap.path_data = (short *)data + 1;
				return (uint8_t * )&ft_svgmap;			
			}
		}
#else
		metric = freetype_font_get_glyph_dsc(font_dsc->font, font_dsc->cache, unicode);
		data = freetype_font_get_bitmap(font_dsc->font, font_dsc->cache, unicode);
#endif
	}

	if(data && metric)
	{
		uint32_t bpp = font_dsc->font->bpp;
		uint32_t cf = LV_COLOR_FORMAT_A1;
		uint32_t stride = (metric->bbw * bpp + 7)/8;
		uint32_t data_size = stride*metric->bbh;
		switch(bpp)
		{
		case 2:
			cf = LV_COLOR_FORMAT_A2;
			break;
		case 4:
			cf = LV_COLOR_FORMAT_A4;
			break;
		case 8:
			cf = LV_COLOR_FORMAT_A8;
			break;
		}
		SYS_LOG_DBG("unicode 0x%x, w %d, h %d, cf %d, stride %d, data_size %d, data %p\n", unicode, metric->bbw, metric->bbh, cf, stride, data_size, data);
		lv_draw_buf_init(&glyf_draw_buf, metric->bbw, metric->bbh, cf, stride, data, data_size);

		return &glyf_draw_buf;
	}

	//SYS_LOG_INF("0x%x get bitmap %d us\n", unicode, k_cyc_to_us_near32(tend-tstart));

	return NULL;

}


int lvgl_freetype_font_init(void)
{
	void* ret;
	ret = freetype_font_init(1);
	if(ret == NULL)
	{
		SYS_LOG_ERR("freetype library init failed\n");
		return -1;
	}
	return 0;
}

int lvgl_freetype_font_deinit(void)
{
	freetype_font_deinit(1);
	return 0;
}

int lvgl_freetype_font_open(lv_font_t* font, const char * font_path, uint32_t font_size)
{
	lv_font_fmt_freetype_dsc_t * dsc = bitmap_font_cache_malloc(sizeof(lv_font_fmt_freetype_dsc_t));
	if(dsc == NULL) return -1;

	SYS_LOG_INF("open ftxxx %s %d\n", font_path, font_size);

	dsc->font = freetype_font_open(font_path, font_size);
	if(!dsc->font)
	{
		SYS_LOG_ERR("freetype opne font %s faild\n", font_path);
		bitmap_font_cache_free(dsc);
		return -1;
	}
	
	dsc->cache = freetype_font_get_cache(dsc->font);
	SYS_LOG_INF("dsc->font %p\n", dsc->font);
	//dsc->font_size = font_size;

#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	font->dsc = &g_freetype_svgfont_dsc;
#else
	font->dsc = &g_freetype_bmpfont_dsc;
#endif

	font->get_glyph_dsc = freetype_font_get_glyph_dsc_cb; 	   /*Set a callback to get info about gylphs*/
	font->get_glyph_bitmap = freetype_font_get_glyph_bitmap_cb;  /*Set a callback to get bitmap of a glyp*/

	if(font->user_data != NULL)
	{
		SYS_LOG_INF("ftxxx non null font user data %p\n", font->user_data);
	}

	font->user_data = dsc;
	font->line_height = dsc->font->line_height;
	font->base_line = dsc->font->base_line;  /*Base line measured from the top of line_height*/
	if(freetype_font_enable_subpixel())
	{
		font->subpx = LV_FONT_SUBPX_HOR;
	}
	else
	{
		font->subpx = LV_FONT_SUBPX_NONE;
	}

	SYS_LOG_INF("line height %d, base line %d, dsc %p\n", font->line_height, font->base_line, dsc);
	return 0;
		
}

void lvgl_freetype_font_close(lv_font_t* font)
{
    lv_font_fmt_freetype_dsc_t * dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font pointer\n");
		return;
	}	

	dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;
	
	if(dsc == NULL)
	{
		SYS_LOG_ERR("invalid font dsc \n");
		return;
	}

	SYS_LOG_INF("close ftxxx %p, %d\n", dsc, dsc->font->font_size);
	freetype_font_close(dsc->font);	
/*	
	if(dsc->emoji_font != NULL)
	{
		bitmap_emoji_font_close(dsc->emoji_font);
	}	
*/
	bitmap_font_cache_free(dsc);
	font->user_data = NULL;
}

#ifdef CONFIG_BITMAP_FONT
int lvgl_freetype_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path)
{
	lv_font_fmt_freetype_dsc_t * dsc = (lv_font_fmt_freetype_dsc_t*)lv_font->user_data;

	if(dsc == NULL)
	{
		SYS_LOG_ERR("null bitmap font for font %p\n", lv_font);
		return -1;
	}

	if(bitmap_font_get_max_emoji_num() == 0)
	{
		SYS_LOG_ERR("emoji not supported\n");
		return -1;	
	}
	
	dsc->emoji_font = bitmap_emoji_font_open(emoji_font_path);
	if(dsc->emoji_font == NULL)
	{
		SYS_LOG_ERR("open bitmap emoji font failed\n");
		return -1;
	}
	return 0;
}

int lvgl_freetype_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_image_dsc_t* dsc, lv_point_t* pos, bool force_retrieve)
{
	lv_font_fmt_freetype_dsc_t* font_dsc;
	glyph_metrics_t* metric;

	if(bitmap_font_get_max_emoji_num() == 0)
	{
		SYS_LOG_ERR("emoji not supported\n");
		return -1;	
	}

	if(dsc == NULL)
	{
		SYS_LOG_ERR("invalid img dsc for emoji 0x%x\n", unicode);
		return -1;
	}

	if(unicode < 0x1F300)
	{
		SYS_LOG_ERR("invalid emoji code 0x%x\n", unicode);
		return -1;
	}

	font_dsc = (lv_font_fmt_freetype_dsc_t*)lv_font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("null bitmap font for font %p\n", lv_font);
		return -1;
	}

	metric = bitmap_font_get_emoji_glyph_dsc(font_dsc->emoji_font, unicode, force_retrieve);
	if(!metric)
	{
		return -1;
	}
	
	dsc->header.cf = LV_COLOR_FORMAT_ARGB8565;
	dsc->header.w = metric->bbw;
	dsc->header.h = metric->bbh;
	dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
	dsc->header.flags = 0;
	dsc->header.stride = dsc->header.w * 3;
	dsc->data_size = metric->bbw*metric->bbh*3;
//	SYS_LOG_INF("emoji metric %d %d, dsc %d %d, size %d\n", metric->bbw, metric->bbh, dsc->header.w, dsc->header.h, dsc->data_size);
	dsc->data = bitmap_font_get_emoji_bitmap(font_dsc->emoji_font, unicode);

	if(pos)
	{
		pos->y += metric->bby;
	}
	return 0;
}
#else
int lvgl_freetype_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path)
{
	return 0;
}

int lvgl_freetype_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_image_dsc_t* dsc, lv_point_t* pos, bool force_retrieve)
{
	return 0;
}

#endif

int lvgl_freetype_font_set_default_code(lv_font_t* font, uint32_t word_code, uint32_t emoji_code)
{
	lv_font_fmt_freetype_dsc_t* font_dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font");
		return -1;
	}
	
	font_dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("cant set default code to an unopend font %p", font);
		return -1;
	}

	freetype_font_set_default_code(font_dsc->font, word_code);

#ifdef CONFIG_BITMAP_FONT
	if(bitmap_font_get_max_emoji_num() > 0)
	{
		bitmap_font_set_default_emoji_code(font_dsc->emoji_font, emoji_code);
	}
#endif
	return 0;
}

int lvgl_freetype_font_set_default_bitmap(lv_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp)
{
	lv_font_fmt_freetype_dsc_t* font_dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font");
		return -1;
	}
	
	font_dsc = (lv_font_fmt_freetype_dsc_t*)font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("cant set default code to an unopend font %p", font);
		return -1;
	}

	freetype_font_set_default_bitmap(font_dsc->font, bitmap, width, height, gap, bpp);

	return 0;

}

int lvgl_freetype_font_cache_preset(freetype_font_cache_preset_t* preset, int count)
{
    return freetype_font_cache_preset(preset, count);
}

int lvgl_freetype_force_bitmap(lv_font_t* font, int enable)
{
	int prev_en = 0;

#if CONFIG_FREETYPE_FONT_ENABLE_SVG_PATH
	if (font->get_glyph_dsc == freetype_font_get_glyph_dsc_cb) {
		prev_en = (font->dsc == &g_freetype_bmpfont_dsc);
		font->dsc = enable ? &g_freetype_bmpfont_dsc : &g_freetype_svgfont_dsc;
	}
#endif

	return prev_en;
}
