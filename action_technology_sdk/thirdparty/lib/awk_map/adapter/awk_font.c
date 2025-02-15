#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <stdio.h>
#include "freetype_font_api.h"
FT_Library library;
FT_Face face;
FT_Outline *outline;

#define AWK_SVG_FONT CONFIG_APP_FAT_DISK"/font/MOD20.ttf"
FT_Bitmap bitmap;
extern uint8_t arry[512*512*2];
#include <file_stream.h>
int awk_font() {


    // 初始化 FreeType 库
    if (FT_Init_FreeType(&library)) {
        fprintf(stderr, "Error initializing FreeType library\n");
        exit(1);
    }

    // 加载字体
    if (FT_New_Face(library, AWK_SVG_FONT, 0, &face)) {
        fprintf(stderr, "Error opening font file\n");
        exit(1);
    }

    // 设置字体大小
    FT_Set_Pixel_Sizes(face, 0, 48);
	return 0;

}

int awk_font_render(unsigned long unicode){
	// 加载字符 'A'
    FT_UInt glyph_index = FT_Get_Char_Index(face, unicode);
    FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    outline = &face->glyph->outline;
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	bitmap = face->glyph->bitmap;
	printf("A bitmap buffer is %p\n",bitmap.buffer);
	printf("A bitmap rows is %d\n",bitmap.rows);
	printf("A bitmap width is %d\n",bitmap.width);
	printf("A bitmap pitch is %d\n",bitmap.pitch);
	printf("A bitmap pixel_mode is %d\n",bitmap.pixel_mode);
	bitmap = face->glyph->bitmap;
	
	for (int i = 0; i < bitmap.rows; i++) {
		for (int j = 0; j < bitmap.width; j++) {
			// 计算当前像素在 buffer 中的索引
			int index = (i * bitmap.pitch) + j;
			// 反转像素值
			bitmap.buffer[index] = 255 - bitmap.buffer[index];
		}
	}

    // 遍历轮廓中的点
    for (int n = 0; n < outline->n_contours; n++) {
        int start = outline->contours[n];
        int end = (n + 1 < outline->n_contours) ? outline->contours[n + 1] : outline->n_points;

        printf("Contour %d: from point %d to %d\n", n, start, end - 1);

        for (int i = start; i < end; i++) {
            FT_Vector v = outline->points[i];
            FT_Byte tag = outline->tags[i];
			printf("  Point %d: (%d, %d), tag = %d,on curve\n", i, v.x, v.y, tag);
            if (tag == FT_CURVE_TAG_ON) {
                printf("  Point %d: (%d, %d), on curve\n", i, v.x, v.y);
            } else if (tag == FT_CURVE_TAG_CONIC) {
                printf("  Point %d: (%d, %d), conic control\n", i, v.x, v.y);
            } else if (tag == FT_CURVE_TAG_CUBIC) {
                printf("  Point %d: (%d, %d), cubic control\n", i, v.x, v.y);
            }
        }
    }

    // 清理资源
    //FT_Done_Face(face);
    //FT_Done_FreeType(library);

    return 0;
}


void awk_font_free() {
// 清理资源
    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

//freetype_font_t* font = NULL;
freetype_font_t* resolver_awk_font_init(){
	freetype_font_t* font = NULL;
	freetype_font_init(0);
	font = freetype_font_open(AWK_SVG_FONT, 32);
	return font;
}

int resolver_awk_font_close(freetype_font_t * font)
{
	if (font) {
		freetype_font_close(font);
		//mem_free(font);
		font = NULL;
		return 0;
	}

	return -EINVAL;
}

int resolver_awk_font_metric(freetype_font_t* font,unsigned long unicode,uint8_t *w,uint8_t*h,uint8_t *advance,uint8_t *x,uint8_t *y){
	bbox_metrics_t* metric;
	//获取0x4e00的点阵
	metric = freetype_font_get_glyph_dsc(font, font->cache, unicode);
	if (!metric) {
		printf("freetype_font_get_glyph_dsc is fail\n");
	}
	*w = metric->bbw;
	*h = metric->bbh;
	*advance =metric->advance;
	*x = metric->bbx;
	*y = metric->bby;
	printf("A bitmap rows is %d\n",*h);
	printf("A bitmap width is %d\n",*w);
	printf("A bitmap advance is %d\n",*advance);
	printf("A bitmap bbx is %d\n",*x);
	printf("A bitmap bby is %d\n",*y);
	return 0;
}

uint8_t* resolver_awk_font(freetype_font_t* font,unsigned long unicode){
	uint8_t* data;
	data = freetype_font_get_bitmap(font, font->cache, unicode);
	if(!data)
	{
		printf("freetype_font_get_bitmap is fail\n");
	}
	printf("A bitmap buffer is %p\n",data);
	return data;
}
