#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <os_common_api.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl.h>
#include "../font/font_mempool.h"

static lv_draw_buf_t glyf_draw_buf;

bool bitmap_font_get_glyph_dsc_cb(const lv_font_t * lv_font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode, uint32_t unicode_next)
{
	lv_font_fmt_bitmap_dsc_t* font_dsc;
	bitmap_font_t* font;
	bitmap_cache_t* cache;
	glyph_metrics_t* metric;

	if (unicode == '\n' || unicode == '\r')
		return false;

	if((lv_font == NULL) || (dsc_out == NULL))
	{
		SYS_LOG_ERR("null font info, %p, %p\n", lv_font, dsc_out);
		return false;
	}
	font_dsc = (lv_font_fmt_bitmap_dsc_t*)lv_font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("null bitmap font for font %p\n", lv_font);
		return false;
	}
	font = font_dsc->font;
	cache = font_dsc->cache;

	if(unicode >= 0x1F300)
	{
		if(bitmap_font_get_max_emoji_num() == 0)
		{
			metric = bitmap_font_get_glyph_dsc(font, cache, 0x20);
		}
		else
		{
			metric = bitmap_font_get_emoji_glyph_dsc(font_dsc->emoji_font, unicode, true);
		}
	}
	else
	{
		metric = bitmap_font_get_glyph_dsc(font, cache, unicode);
	}
	
	if(metric == NULL)
	{
		SYS_LOG_ERR("null metric for glyph 0x%x\n", unicode);
		return false;
	}
	
	dsc_out->adv_w = metric->advance;
	dsc_out->ofs_x = metric->bbx;
	dsc_out->ofs_y = metric->bby - font->descent;
	dsc_out->box_w = metric->bbw;
	dsc_out->box_h = metric->bbh;
	dsc_out->gid.index = unicode;
	dsc_out->is_placeholder = false;

	if(unicode >= 0x1F300)
	{
		dsc_out->ofs_x = 0;
		dsc_out->ofs_y = font->ascent - metric->bbh - metric->bby;
		dsc_out->format = LV_FONT_GLYPH_FORMAT_IMAGE;
		return true;
	}
	else
	{
		dsc_out->format = (lv_font_glyph_format_t)font->bpp;
	}

//	SYS_LOG_INF("dsc out %d x %d y %d w %d h %d\n\n", dsc_out->adv_w, dsc_out->ofs_x, dsc_out->ofs_y, dsc_out->box_w, dsc_out->box_h);
	return true;

}

const void * bitmap_font_get_bitmap_cb(lv_font_glyph_dsc_t *glyph_dsc, lv_draw_buf_t *draw_buf)
{
	const lv_font_t * lv_font = glyph_dsc->resolved_font;
	uint32_t unicode = glyph_dsc->gid.index;
	uint8_t* data;
	lv_font_fmt_bitmap_dsc_t* font_dsc;
	bitmap_font_t* font;
	bitmap_cache_t* cache;
	glyph_metrics_t* metric = NULL;

	if(lv_font == NULL)
	{
		SYS_LOG_ERR("null font info, %p\n", lv_font);
		return NULL;
	}
	font_dsc = (lv_font_fmt_bitmap_dsc_t*)lv_font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("null bitmap font for font %p\n", lv_font);
		return false;
	}	
	font = font_dsc->font;
	cache = font_dsc->cache;

	if(unicode >= 0x1F300)
	{
		if(bitmap_font_get_max_emoji_num() == 0)
		{
			metric = bitmap_font_get_glyph_dsc(font, cache, 0x20);
			data = bitmap_font_get_bitmap(font, cache, 0x20);
		}
		else
		{
			data = bitmap_font_get_emoji_bitmap(font_dsc->emoji_font, unicode);
			return data;
		}
	}
	else
	{
		metric = bitmap_font_get_glyph_dsc(font, cache, unicode);
		data = bitmap_font_get_bitmap(font, cache, unicode);
	}

	if(!data || !metric)
	{
		return NULL;
	}

	uint32_t cf = LV_COLOR_FORMAT_A1;
	uint32_t stride = (metric->bbw * font->bpp + 7)/8;
	uint32_t data_size = stride*metric->bbh;
	switch(font->bpp)
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

int lvgl_bitmap_font_init(const char *def_font_path)
{
	bitmap_font_init();


	if(bitmap_font_get_high_freq_enabled()==1)
	{
		if (def_font_path) 
		{
			bitmap_font_load_high_freq_chars(def_font_path);
		}
	}
	return 0;
}

int lvgl_bitmap_font_deinit(void)
{
	bitmap_font_deinit();
	return 0;
}

int lvgl_bitmap_font_set_emoji_font(lv_font_t* lv_font, const char* emoji_font_path)
{
	lv_font_fmt_bitmap_dsc_t * dsc = (lv_font_fmt_bitmap_dsc_t*)lv_font->user_data;

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

int lvgl_bitmap_font_get_emoji_dsc(const lv_font_t* lv_font, uint32_t unicode, lv_image_dsc_t* dsc, lv_point_t* pos, bool force_retrieve)
{
	lv_font_fmt_bitmap_dsc_t* font_dsc;
	glyph_metrics_t* metric;
	lv_image_dsc_t* emoji_dsc;

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

	font_dsc = (lv_font_fmt_bitmap_dsc_t*)lv_font->user_data;
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

	emoji_dsc = (lv_image_dsc_t*)bitmap_font_get_emoji_bitmap(font_dsc->emoji_font, unicode);
	if(!emoji_dsc)
	{
		return -1;
	}
	memcpy(dsc, emoji_dsc, sizeof(lv_image_dsc_t));

	if(pos)
	{
		pos->y += metric->bby;
	}
	return 0;
}

int lvgl_bitmap_font_set_default_code(lv_font_t* font, uint32_t word_code, uint32_t emoji_code)
{
	lv_font_fmt_bitmap_dsc_t* font_dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font");
		return -1;
	}
	
	font_dsc = (lv_font_fmt_bitmap_dsc_t*)font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("cant set default code to an unopend font %p", font);
		return -1;
	}

	bitmap_font_set_default_code(font_dsc->font, word_code);

	if(bitmap_font_get_max_emoji_num() > 0)
	{
		bitmap_font_set_default_emoji_code(font_dsc->emoji_font, emoji_code);
	}

	return 0;
}

int lvgl_bitmap_font_set_default_bitmap(lv_font_t* font, uint8_t* bitmap, uint32_t width, uint32_t height, uint32_t gap, uint32_t bpp)
{
	lv_font_fmt_bitmap_dsc_t* font_dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font");
		return -1;
	}
	
	font_dsc = (lv_font_fmt_bitmap_dsc_t*)font->user_data;
	if(font_dsc == NULL)
	{
		SYS_LOG_ERR("cant set default code to an unopend font %p", font);
		return -1;
	}

	bitmap_font_set_default_bitmap(font_dsc->font, bitmap, width, height, gap, bpp);

	return 0;

}

int lvgl_bitmap_font_cache_preset(bitmap_font_cache_preset_t* preset, int count)
{
    return bitmap_font_cache_preset(preset, count);
}

int lvgl_bitmap_font_open(lv_font_t* font, const char * font_path)
{
	lv_font_fmt_bitmap_dsc_t * dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font pointer");
		return -1;
	}

	if(font_path == NULL)
	{
		SYS_LOG_ERR("null path\n");
		return -1;
	}

	dsc = mem_malloc(sizeof(lv_font_fmt_bitmap_dsc_t));
	if(dsc == NULL)
	{
		SYS_LOG_ERR("malloc font dsc data failed\n");
		return -1;
	}
	
	memset(dsc, 0, sizeof(lv_font_fmt_bitmap_dsc_t));

	dsc->font = bitmap_font_open(font_path);
	if(dsc->font == NULL)
	{
		SYS_LOG_ERR("open bitmap font failed\n");
		goto ERR_EXIT;
	}
	dsc->cache = bitmap_font_get_cache(dsc->font);

	lv_font_fmt_txt_dsc_t* font_fmt_dsc = (lv_font_fmt_txt_dsc_t*)bitmap_font_cache_malloc(sizeof(lv_font_fmt_txt_dsc_t));
	if(!font_fmt_dsc)
	{
		goto ERR_EXIT;
	}
	memset(font_fmt_dsc, 0, sizeof(lv_font_fmt_txt_dsc_t));
	font_fmt_dsc->bpp = dsc->font->bpp;
	font->dsc = font_fmt_dsc;
	font->get_glyph_dsc = bitmap_font_get_glyph_dsc_cb;        /*Set a callback to get info about gylphs*/
	font->get_glyph_bitmap = bitmap_font_get_bitmap_cb;		/*Set a callback to get bitmap of gylphs*/

	font->user_data = dsc;
	
	if(dsc->font->ascent <= dsc->font->font_size)
	{
		font->line_height = dsc->font->ascent - dsc->font->descent;
		font->base_line = 0; //(dsc->font->ascent);  //Base line measured from the top of line_height
	}
	else
	{
		font->line_height = dsc->font->ascent;
		font->base_line = dsc->font->descent/2; //(dsc->font->ascent);  /*Base line measured from the top of line_height*/
	}
	font->subpx = LV_FONT_SUBPX_NONE;
	return 0;

ERR_EXIT:
	if(dsc->font != NULL)
	{
		bitmap_font_close(dsc->font);
	}

	mem_free(dsc);
	return -1;
}

void lvgl_bitmap_font_close(lv_font_t* font)
{
	lv_font_fmt_bitmap_dsc_t * dsc;
	lv_font_fmt_txt_dsc_t* font_fmt_dsc;

	if(font == NULL)
	{
		SYS_LOG_ERR("null font pointer\n");
		return;
	}

	font_fmt_dsc = (lv_font_fmt_txt_dsc_t*)font->dsc;
	if(font_fmt_dsc)
	{
		bitmap_font_cache_free(font_fmt_dsc);
		font->dsc = NULL;
	}

	dsc = (lv_font_fmt_bitmap_dsc_t*)font->user_data;
	if(dsc == NULL)
	{
		SYS_LOG_ERR("null font dsc pointer for font %p\n", font);
		return;
	}
	bitmap_font_close(dsc->font);
	if(dsc->emoji_font != NULL)
	{
		bitmap_emoji_font_close(dsc->emoji_font);
	}
	mem_free(dsc);
	font->user_data = NULL;
}


