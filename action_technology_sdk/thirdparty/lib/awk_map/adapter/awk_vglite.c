/*
 * @brief 系统适配接口
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "awk_defines.h"
#include "awk_adapter.h"
#include "vladimir_font.h"
#include "awk_vglite.h"
#include "awk_render_adapter.h"
#include "awk_system_adapter.h"
#include <memory/mem_cache.h>
#include <svg_font.h>
#include <vg_lite/vglite_util.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <freetype_font_api.h>

#define PPEM_SIZE 24
#define DEFAULT_SIZE   250
#define AWK_SVG2_FONT CONFIG_APP_FAT_DISK"/font/vfont.ttf"
#define AWK_SVG2_FONT_CN CONFIG_APP_FAT_DISK"/font/chinese.txt"

#define FONT_BITMAP 0
#define FONT_EMBED  0
#define FONT_ESVG   1

extern FT_Bitmap bitmap;
uint8_t g_awk = 0;

static vg_lite_path_t path;
static vg_lite_buffer_t dest_vgbuf_bg;

static uint32_t test_letters[32] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',0x56FD,
};

static vg_lite_buffer_format_t convert_format(awk_pixel_mode_t format)
{
    switch (format) {
    case AWK_PIXEL_MODE_RGB_565:
        return VG_LITE_BGR565;
    case AWK_PIXEL_MODE_BGR_888:
        return VG_LITE_BGR888;

	case AWK_PIXEL_MODE_GREY:
        return VG_LITE_L8;

    case AWK_PIXEL_MODE_RGB_888:
        return VG_LITE_RGB888;
    case AWK_PIXEL_MODE_ARGB_8888:
        return VG_LITE_ARGB8888;
    case AWK_PIXEL_MODE_RGBA_8888:
        return VG_LITE_RGBA8888;
    case AWK_PIXEL_MODE_BGRA_8888:
        return VG_LITE_BGRA8888;
    case AWK_PIXEL_MODE_ABGR_8888:
        return VG_LITE_ABGR8888;

    default:
        printf("unrecognized awk format %d", format);
        return VG_LITE_BGR565;
    }
}

static inline vg_lite_error_t vg_lite_bufferconvert(vg_lite_buffer_t * buffer,
                t_awk_view_buffer_info *buffer_info, void *buffer_data)
{
    return vglite_buf_map(buffer, buffer_data, buffer_info->width, buffer_info->height,
                          buffer_info->stride, convert_format(buffer_info->cf));
}

void begin_draw_path(void)
{
	vg_lite_error_t err = VG_LITE_SUCCESS;

	int32_t line_path[] = { /*VG line path*/
        VLC_OP_MOVE, 0, 0,
        VLC_OP_LINE, 0, 455,
        VLC_OP_MOVE, 32, 0,
        VLC_OP_LINE, 32, 455,
        VLC_OP_MOVE, 64, 0,
        VLC_OP_LINE, 64, 455,
        VLC_OP_MOVE, 96, 0,
        VLC_OP_LINE, 96, 455,
        VLC_OP_MOVE, 128, 0,
        VLC_OP_LINE, 128, 455,
        VLC_OP_MOVE, 160, 0,
        VLC_OP_LINE, 160, 455,
        VLC_OP_MOVE, 192, 0,
        VLC_OP_LINE, 192, 455,
        VLC_OP_MOVE, 224, 0,
        VLC_OP_LINE, 224, 455,
        VLC_OP_MOVE, 256, 0,
        VLC_OP_LINE, 256, 455,
        VLC_OP_MOVE, 288, 0,
        VLC_OP_LINE, 288, 455,
        VLC_OP_MOVE, 320, 0,
        VLC_OP_LINE, 320, 455,
        VLC_OP_MOVE, 352, 0,
        VLC_OP_LINE, 352, 455,
        VLC_OP_MOVE, 384, 0,
        VLC_OP_LINE, 384, 455,
        VLC_OP_MOVE, 416, 0,
        VLC_OP_LINE, 416, 455,
        VLC_OP_MOVE, 448, 0,
        VLC_OP_LINE, 448, 455,
        VLC_OP_MOVE, 0, 0,
        VLC_OP_LINE, 455,0,
        VLC_OP_MOVE, 0,32,
        VLC_OP_LINE, 455,32,
        VLC_OP_MOVE,  0,64,
        VLC_OP_LINE, 455,64,
        VLC_OP_MOVE, 0,96,
        VLC_OP_LINE, 455,96,
        VLC_OP_MOVE, 0,128,
        VLC_OP_LINE, 455,128,
        VLC_OP_MOVE, 0,160,
        VLC_OP_LINE, 455,160,
        VLC_OP_MOVE, 0,192,
        VLC_OP_LINE, 455,192,
        VLC_OP_MOVE, 0,224,
        VLC_OP_LINE, 455,224,
        VLC_OP_MOVE, 0,256,
        VLC_OP_LINE, 455,256,
        VLC_OP_MOVE, 0,288,
        VLC_OP_LINE, 455,288,
        VLC_OP_MOVE, 0,320,
        VLC_OP_LINE, 455,320,
        VLC_OP_MOVE,  0,352,
        VLC_OP_LINE, 455,352,
        VLC_OP_MOVE,  0,384,
        VLC_OP_LINE, 455,384,
        VLC_OP_MOVE, 0,416,
        VLC_OP_LINE, 455,416,
        VLC_OP_MOVE,  0,448,
        VLC_OP_LINE, 455,448,
        VLC_OP_END
    };

    err = vg_lite_init_path(&path, VG_LITE_S32, VG_LITE_HIGH, sizeof(line_path), line_path, 0, 0, 466, 466);
    if(err != VG_LITE_SUCCESS)
        printf("Init path failed.\n");


    /*** Draw line ***/
    err = vg_lite_set_draw_path_type(&path, VG_LITE_DRAW_STROKE_PATH);
    if(err != VG_LITE_SUCCESS)
        printf("Set draw path type failed.\n");

    err = vg_lite_set_stroke(&path, VG_LITE_CAP_ROUND, VG_LITE_JOIN_ROUND, 1, 8, NULL, 0, 8, 0xffd3d3d3);
    if(err != VG_LITE_SUCCESS)
    {
        printf("Set stroke failed.\n");
    }

    err = vg_lite_update_stroke(&path);
    if(err != VG_LITE_SUCCESS){
        printf("Update stroke failed.\n");
    }
}

void awk_begin_drawing(t_awk_view_buffer_info *buffer_info,uint8_t *addr)
{
    //printf("awk_begin_drawing\n");
    //uint32_t stc = os_uptime_get_32();

    vg_lite_error_t err = VG_LITE_SUCCESS;
    vg_lite_buffer_t * fb;

    vg_lite_bufferconvert(&dest_vgbuf_bg, buffer_info, addr);

    //awk_point_t *point = param->points;
    fb = &dest_vgbuf_bg;

    vg_lite_matrix_t matrix;
    vg_lite_identity(&matrix);
    err =vg_lite_clear(fb,NULL,0xffffffff);
    if(err != VG_LITE_SUCCESS){
        printf("awk_begin_drawing clear failed.\n");
    }

    //awk_point_t *point2 = point;
    //point2++;

    err =vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xffd3d3d3);
    if(err != VG_LITE_SUCCESS){
        printf("awk_begin_drawing failed.\n");
    }

    vg_lite_finish();
    vglite_buf_unmap(&dest_vgbuf_bg);
    //printf("%s, begin stc:%u, spend %u ms\n", __func__, stc, os_uptime_get_32() - stc);
    //printf("awk_begin_drawing sucess.\n");
}

void begin_dgpath_free()
{
	vg_lite_clear_path(&path);
	memset(&path,0,sizeof(vg_lite_path_t));
}

void awk_begin_drawing_bg(uint32_t map_id,t_awk_view_buffer_info *buffer_info)
{
	//printf("awk_draw_drawing_bg\n");
	//uint32_t stc = os_uptime_get_32();

    vg_lite_error_t err = VG_LITE_SUCCESS;
    vg_lite_blend_t blend = VG_LITE_BLEND_NONE;//VG_LITE_BLEND_SRC_OVER;//VG_LITE_BLEND_NONE;

	//mem_dcache_flush(param->bitmap.buffer, param->bitmap.buffer_size);

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

    vg_lite_rectangle_t src_rect = { 0, 0, buffer_info->width, buffer_info->height};

    vg_lite_filter_t vgfilter = VG_LITE_FILTER_POINT;
    vg_lite_matrix_t vgmatrix;
    vg_lite_identity(&vgmatrix);

    err = vg_lite_blit_rect(&dest_vgbuf, &dest_vgbuf_bg, &src_rect, &vgmatrix, blend, 0xffffffff, vgfilter);
	//err = vg_lite_blit(&dest_vgbuf, &src_vgbuf, &vgmatrix, blend, 0, vgfilter);
    if(err != VG_LITE_SUCCESS)
        printf("awk_draw_drawing_bg failed.\n");

    vg_lite_finish();
    vglite_buf_unmap(&dest_vgbuf);
	//printf("%s, begin stc:%u, spend %u ms\n", __func__, stc, os_uptime_get_32() - stc);
    //printf("awk_draw_drawing_bg succes.\n");
}

void awk_commit_drawing(uint32_t map_id)
{

}

//uint8_t arry[512*255*2] __attribute__((aligned(64)));
void awk_draw_bitmap(t_draw_bitmap_param *param,t_awk_view_buffer_info *buffer_info)
{
	//awk_draw_textsvg2((t_draw_text_param*)param, buffer_info);
	//awk_draw_polyline((t_drawing_points_param*)param, buffer_info);
	//return;
	//printf("awk_draw_bitmap\n");

    vg_lite_error_t err = VG_LITE_SUCCESS;
	//memcpy(arry, param->bitmap.buffer, param->bitmap.buffer_size);
    vg_lite_color_t vgcol = param->style->color;
    vg_lite_blend_t blend = VG_LITE_BLEND_NONE;//VG_LITE_BLEND_SRC_OVER;//VG_LITE_BLEND_NONE;
    float angle = param->style->angle;
	//printf("source param->bitmap.stride is %d\n", param->bitmap.stride);
    //printf("source param->bitmap.pixel_mode is %d\n", param->bitmap.pixel_mode);
    //printf("target buffer_info->cf is %d\n", buffer_info->cf);
	//printf("awk:param->bitmap.pixel_mode is %d\n",param->bitmap.pixel_mode);
    vg_lite_buffer_t src_vgbuf;
    vglite_buf_map(&src_vgbuf, param->bitmap.buffer, param->bitmap.width, param->bitmap.height,
                   param->bitmap.stride * param->bitmap.width, VG_LITE_BGR565);

	//printf("src_vgbuf.memory is %p\n",src_vgbuf.memory);
    //printf("src_vgbuf.address is %d\n",src_vgbuf.address);
	//printf("source src_vgbuf.stride is %d\n", src_vgbuf.stride);

	mem_dcache_flush(param->bitmap.buffer, param->bitmap.buffer_size);

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

	vg_lite_int32_t src_width = 0;
    vg_lite_int32_t src_height = 0;
	src_width = param->area.width > src_vgbuf.width ? src_vgbuf.width : param->area.width;
	src_height = param->area.height > src_vgbuf.height ? src_vgbuf.height : src_vgbuf.height;
    //vg_lite_rectangle_t src_rect = { param->area.x, param->area.y, src_width, src_height};
    vg_lite_rectangle_t src_rect = { 0, 0, src_width, src_height};
    //vg_lite_rectangle_t src_rect = { 0, 0, src_vgbuf.width, src_vgbuf.height};
    //printf("param->area.x is %d\n",param->area.x);
	//printf("param->area.y is %d\n",param->area.y);
	//printf("param->area.width is %d\n",param->area.width);
	//printf("param->area.height is %d\n",param->area.height);
	//printf("src_width is %d\n",src_width);
	//printf("src_height is %d\n",src_height);
	//printf("src_vgbuf.width is %d\n",src_vgbuf.width);
	//printf("src_vgbuf.height is %d\n",src_vgbuf.height);
    vg_lite_filter_t vgfilter = VG_LITE_FILTER_POINT;
    vg_lite_matrix_t vgmatrix;
    vg_lite_identity(&vgmatrix);
    //vg_lite_translate(466 / 2 - 170 * 466 / 466.0f, 466 / 2 - 170 * 466 / 466.0f, &vgmatrix);
	//vg_lite_scale(1.5, 1.5, &vgmatrix);
	vg_lite_translate( param->area.x, param->area.y, &vgmatrix);
    vg_lite_rotate(angle, &vgmatrix);

    //printf("vg_lite_blit_rect format is %d\n",dest_vgbuf.format);
    err = vg_lite_blit_rect(&dest_vgbuf, &src_vgbuf, &src_rect, &vgmatrix, blend, vgcol, vgfilter);
    if(err != VG_LITE_SUCCESS) {
        printf("vg_lite_blit_rect failed. (err=%d)\n", err);
        printf("dest: mem %p, fmt %x, w %u, h %u, stride %u\n", dest_vgbuf.memory,
                dest_vgbuf.format, dest_vgbuf.width, dest_vgbuf.height, dest_vgbuf.stride);
        printf("src: mem %p, fmt %x, w %u, h %u, stride %u\n", src_vgbuf.memory,
                src_vgbuf.format, src_vgbuf.width, src_vgbuf.height, src_vgbuf.stride);
    }

    vg_lite_finish();
    vglite_buf_unmap(&dest_vgbuf);
    vglite_buf_unmap(&src_vgbuf);
    //printf("awk_draw_bitmap succes.\n");
}

void build_path(float* dst, float *src, size_t size)
{
    float *dst_path = dst;
    float *src_path = src;
    uint32_t count = size/4;
    uint32_t iter = 0;
    uint8_t *operation;
    uint8_t op;
	printf("build_path size is %d\n",size);
    for(iter = 0 ; iter < count; iter++)
    {
        op = (uint8_t) *src_path;
        *dst_path = *src_path++;
        operation = (uint8_t *)dst_path++;
        *operation = op;
		 printf("build_path op is %d\n",op);
        switch(op)
        {

        case VLC_OP_END:
            break;
        case VLC_OP_QUAD:
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;

            iter += 4;
            break;
        case VLC_OP_MOVE:
        case VLC_OP_LINE:
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;
			printf("src_path is %f",*src_path);
            *dst_path ++ = *src_path++;
            iter += 2;
            break;
        default:
            break;
        }
    }
    return;
}

void awk_draw_bitmap_embedtext(t_draw_text_param *param,t_awk_view_buffer_info *buffer_info)
{
	awk_font();

	char string[] = "Hello!";
	int iter = 0;
	vg_lite_int32_t src_width_point = 0;
    vg_lite_int32_t src_height_point = 0;

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

	for(iter = 0; string[iter] != '\0'; iter ++)
    {
		awk_font_render(string[iter]);
		printf("A bitmap buffer is %p\n",bitmap.buffer);
		printf("awk_draw_bitmap_text:A bitmap rows is %d\n",bitmap.rows);
		printf("awk_draw_bitmap_text:A bitmap width is %d\n",bitmap.width);
		printf("awk_draw_bitmap_text:A bitmap pitch is %d\n",bitmap.pitch);
		printf("awk_draw_bitmap_text:A bitmap pixel_mode is %d\n",bitmap.pixel_mode);

        printf("awk_draw_bitmap_text\n");
        //awk_draw_text((t_draw_text_param*)param,buffer_info);

        vg_lite_error_t err = VG_LITE_SUCCESS;
        vg_lite_color_t vgcol = 0xff000000;//param->style->color;
        vg_lite_blend_t blend = VG_LITE_BLEND_NONE;//VG_LITE_BLEND_SRC_OVER;//VG_LITE_BLEND_NONE;
        printf("target buffer_info->cf is %d\n", buffer_info->cf);

        vg_lite_buffer_t src_vgbuf;
        vglite_buf_map(&src_vgbuf, bitmap.buffer, bitmap.width, bitmap.rows,
                    bitmap.pitch, VG_LITE_L8);

        printf("src_vgbuf.memory is %p\n",src_vgbuf.memory);
        printf("src_vgbuf.address is %d\n",src_vgbuf.address);
        printf("source src_vgbuf.stride is %d\n", src_vgbuf.stride);

        mem_dcache_flush(src_vgbuf.memory, bitmap.rows * bitmap.pitch);

        vg_lite_int32_t src_width = 0;
        vg_lite_int32_t src_height = 0;
        src_width = src_vgbuf.width ;
        //src_width_point +=2*src_width;
        src_height = src_vgbuf.height;
        src_height_point += src_height;
        vg_lite_rectangle_t src_rect = { 0, 0, src_width, src_height};
        if(src_width > 20){
            src_width_point +=48;
        }
        else{
            src_width_point +=48;
        }
        //vg_lite_rectangle_t src_rect = { 0, 0, src_vgbuf.width, src_vgbuf.height};
        //printf("param->area.x is %d\n",param->area.x);
        //printf("param->area.y is %d\n",param->area.y);
        //printf("param->area.width is %d\n",param->area.width);
        //printf("param->area.height is %d\n",param->area.height);
        printf("src_width is %d\n",src_width);
        printf("src_height is %d\n",src_height);
        printf("src_vgbuf.width is %d\n",src_vgbuf.width);
        printf("src_vgbuf.height is %d\n",src_vgbuf.height);
        vg_lite_filter_t vgfilter = VG_LITE_FILTER_POINT;
        vg_lite_matrix_t vgmatrix;
        vg_lite_identity(&vgmatrix);

        //vg_lite_clear(&dest_vgbuf,NULL,0xffffffff);
        vg_lite_finish();
        vg_lite_translate(466 / 2 - 200 * 466 / 466.0f + src_width_point, 466 / 2 -50 * 466 / 466.0f + 0*src_height, &vgmatrix);
        vg_lite_scale(1, 1, &vgmatrix);
        //vg_lite_translate( iter*src_width, 0*src_height, &vgmatrix);
    // vg_lite_rotate(angle, &vgmatrix);

        printf("vg_lite_blit_rect format is %d\n",dest_vgbuf.format);
        err = vg_lite_blit_rect(&dest_vgbuf, &src_vgbuf, &src_rect, &vgmatrix, blend, vgcol, vgfilter);
        //err = vg_lite_blit(&dest_vgbuf, &src_vgbuf, &vgmatrix, blend, 0, vgfilter);
        if(err != VG_LITE_SUCCESS) {
            printf("vg_lite_blit_rect failed.\n");
            printf("dest: mem %p, fmt %x, w %u, h %u, stride %u\n", dest_vgbuf.memory,
                    dest_vgbuf.format, dest_vgbuf.width, dest_vgbuf.height, dest_vgbuf.stride);
            printf("src: mem %p, fmt %x, w %u, h %u, stride %u\n", src_vgbuf.memory,
                    src_vgbuf.format, src_vgbuf.width, src_vgbuf.height, src_vgbuf.stride);
        }

        vg_lite_finish();
        vglite_buf_unmap(&src_vgbuf);
        printf("awk_draw_bitmap succes.\n");
	}

    vglite_buf_unmap(&dest_vgbuf);
    awk_font_free();
}
//extern int resolver_awk_font_metric(freetype_font_t*font,unsigned long unicode,uint8_t *w,uint8_t*h,uint8_t *advance,uint8_t *x,uint8_t *y);
//extern freetype_font_t* resolver_awk_font_init();
//extern uint8_t* resolver_awk_font(freetype_font_t*font,unsigned long unicode);
//extern int resolver_awk_font_close(freetype_font_t * font);

void awk_draw_bitmap_text(t_draw_text_param *param,t_awk_view_buffer_info *buffer_info)
{
	printf("awk_draw_bitmap_text\n");
	freetype_font_t* font = resolver_awk_font_init();
	if(!font)
	{
		printf("resolver_awk_font_init is fail!\n");
	}

	char string[] = "Hello!";
	int iter = 0;
	float offsetX = 0;
	float startX = 150.0f;
    float startY = 150;
	uint8_t* data_font = NULL;
	vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);
	vg_lite_clear(&dest_vgbuf,NULL,0x0);

	vg_lite_filter_t vgfilter = VG_LITE_FILTER_POINT;
	vg_lite_matrix_t matrix;
    vg_lite_identity(&matrix);
    /* Translate the matrix to the center of the buffer.*/
    vg_lite_translate( startX, startY, &matrix);

	for(iter = 0; string[iter] != '\0'; iter ++)
    {
    	uint8_t w = 0;
		uint8_t h = 0;
		uint8_t x = 0;
		uint8_t y = 0;
		uint8_t advance = 0;
		resolver_awk_font_metric(font,string[iter],&w,&h,&advance,&x,&y);
		data_font = resolver_awk_font(font,string[iter]);

		 if(string[iter] == '\n')
        {
            vg_lite_translate(-offsetX, 32, &matrix);
            offsetX = 0;
            continue;
        }

        vg_lite_error_t err = VG_LITE_SUCCESS;
        vg_lite_color_t vgcol = 0x0;//param->style->color;
        vg_lite_blend_t blend = VG_LITE_BLEND_SRC_OVER ;//VG_LITE_BLEND_SRC_OVER;//VG_LITE_BLEND_NONE;VG_LITE_BLEND_SRC_IN,VG_LITE_BLEND_DST_OVER

        vg_lite_buffer_t src_vgbuf;
        vglite_buf_map(&src_vgbuf, data_font, w, h, w, VG_LITE_L8);

        printf("src_vgbuf.memory is %p\n",src_vgbuf.memory);
        printf("src_vgbuf.address is %d\n",src_vgbuf.address);
        printf("source src_vgbuf.stride is %d\n", src_vgbuf.stride);

        mem_dcache_flush(src_vgbuf.memory, src_vgbuf.height * src_vgbuf.stride);
        vg_lite_int32_t src_width = 0;
        vg_lite_int32_t src_height = 0;
        src_width = src_vgbuf.width ;
        src_height = src_vgbuf.height;

        vg_lite_rectangle_t src_rect = { 0, 0, src_width, src_height};

        printf("src_width is %d\n",src_width);
        printf("src_height is %d\n",src_height);


        //vg_lite_translate(0, src_height/2, &matrix);
        vg_lite_scale(1, 1, &matrix);
        //vg_lite_translate( iter*src_width, 0*src_height, &vgmatrix);

        printf("vg_lite_blit_rect format is %d\n",dest_vgbuf.format);
        err = vg_lite_blit_rect(&dest_vgbuf, &src_vgbuf, &src_rect, &matrix, blend, vgcol, vgfilter);
        if(err != VG_LITE_SUCCESS) {
            printf("vg_lite_blit_rect failed.\n");
            printf("dest: mem %p, fmt %x, w %u, h %u, stride %u\n", dest_vgbuf.memory,
                    dest_vgbuf.format, dest_vgbuf.width, dest_vgbuf.height, dest_vgbuf.stride);
            printf("src: mem %p, fmt %x, w %u, h %u, stride %u\n", src_vgbuf.memory,
                    src_vgbuf.format, src_vgbuf.width, src_vgbuf.height, src_vgbuf.stride);
        }

		vg_lite_translate(advance, 0, &matrix);
        offsetX += advance;

        vglite_buf_unmap(&src_vgbuf);
	}

	vg_lite_finish();
    vglite_buf_unmap(&dest_vgbuf);
	printf("awk_draw_bitmap_text1 succes.\n");

    resolver_awk_font_close(font);
    return;
}

void awk_draw_text(t_draw_text_param *param, t_awk_view_buffer_info *buffer_info)
{
#ifdef FONT_BITMAP
	awk_draw_bitmap_text(param, buffer_info);
#endif

#ifdef FONT_EMBED
	awk_draw_bitmap_embedtext(param, buffer_info);
#endif

#ifdef FONT_ESVG
	awk_draw_textsvg2(param, buffer_info);
#endif
	printf("awk_draw_text success\n");
	return;
}

static void render_text(vg_lite_buffer_t *fb, uint32_t x, uint32_t y,
        vg_lite_color_t color, const uint32_t letters[],
        uint32_t letter_count, uint8_t letter_space)
{
    svgfont_t *font = svgfont_open(AWK_SVG2_FONT, 24);
    if (font == NULL) {
        printf("svgfont_open %s failed\n", AWK_SVG2_FONT"/font/vfont.ttf");
        return;
    }

    svgfont_set_size(font, 48);

    vg_lite_path_t path;
    memset(&path, 0, sizeof(path));

    vg_lite_matrix_t matrix;
    vg_lite_blend_t blend_mode = ((color >> 24) == 0xff) ?
            VG_LITE_BLEND_NONE : VG_LITE_BLEND_PREMULTIPLY_SRC_OVER;

    svgfont_glyph_dsc_t dsc_out = { 0 };
    int err;

    for (uint32_t i = 0; i < letter_count; i++, x += letter_space + dsc_out.adv_w) {
        err = svgfont_get_glyph_dsc(font, &dsc_out, letters[i]);
        if (err) {
            printf("get glyph dsc failed (letter=%#x)\n", letters[i]);
            continue;
        }

        vg_lite_identity(&matrix);
        err = svgfont_generate_path_and_matrix(font, &dsc_out, &path, &matrix, letters[i], x, y, false);
        if (err != VG_LITE_SUCCESS) {
            printf("svgfont_generate_path_and_matrix failed (letter=%#x)\n", letters[i]);
            vg_lite_clear_path(&path);
            continue;
        }

        err = vg_lite_draw(fb, &path, VG_LITE_FILL_NON_ZERO, &matrix, blend_mode, color);
        if (err != VG_LITE_SUCCESS) {
            printf("vg_lite_draw failed\n");
        }
        vg_lite_clear_path(&path);
    }

    vg_lite_finish();
    svgfont_close(font);
}

int awk_draw_textsvg2(t_draw_text_param *param, t_awk_view_buffer_info *buffer_info)
{
	vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

	#if 1
	uint8_t k = 0;
	const char *str = param->text;
	//char str[] = "你好He世界"; // 字符串
    printf("Unicode code points for each character in the string:\n");
    for (int i = 0; str[i] != '\0'; ) {
        if ((str[i] & 0x80) == 0) {
            // 单字节字符（ASCII）
            //printf("U+%04X ", (unsigned char)str[i++]);
			test_letters[k++] = (unsigned char)str[i++];
        } else if ((str[i] & 0xE0) == 0xC0) {
            // 双字节字符（非中文）
            //printf("U+%04X ", ((str[i] & 0x1F) << 6) | (str[i+1] & 0x3F));
			test_letters[k++] = ((str[i] & 0x1F) << 6) | (str[i+1] & 0x3F);
            i += 2;
        } else if ((str[i] & 0xF0) == 0xE0) {
            // 三字节字符（中文）
            test_letters[k++] =  ((str[i] & 0x0F) << 12) | ((str[i+1] & 0x3F) << 6) | (str[i+2] & 0x3F);
            i += 3;
        }
    }
	#endif
	render_text(&dest_vgbuf, 10, 233, 0xff000000, test_letters, k, 4);
    vglite_buf_unmap(&dest_vgbuf);
	printf("awk_draw_textsvg2\n");

	return 0;
}

void awk_draw_polygon(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info)
{
	printf("awk_draw_polygon\n");

    vg_lite_error_t err = VG_LITE_SUCCESS;
	vg_lite_buffer_t * fb;
    //vg_lite_filter_t filter = VG_LITE_FILTER_POINT;
    uint32_t data_size;
    vg_lite_path_t path;
	vg_lite_color_t vgcol = param->style->color;
    float angle = param->style->angle;

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

#if 1
    data_size = param->point_size;
    uint8_t *draw_point_cmd = awk_mem_malloc_adapter((data_size+1)*sizeof(uint8_t));
    float *draw_point_data_left = awk_mem_malloc_adapter((data_size*2)*sizeof(float));
    awk_point_t *point = param->points;
    int i = 0 ;
    for( i = 0 ; i < (data_size + 1); i++)
    {
        if( i == 0) {
            draw_point_cmd[i] = VLC_OP_MOVE;
        } else if(i == data_size ){
            draw_point_cmd[i] = VLC_OP_END;
        } else{
            draw_point_cmd[i] = VLC_OP_LINE;
        }
    }

    for( i = 0 ; i < data_size + 1; i++)
    {
        printf("draw_point_cmd[%d] is %d\n", i, draw_point_cmd[i]);
    }

    for( i = 0 ; i < data_size; i++)
    {

        draw_point_data_left[ i * 2 + 0] = point->x;
        draw_point_data_left[ i * 2 + 1] = point->y;
        point++;
    }

    for( i = 0 ; i < ((data_size) * 2); i++)
    {
        printf("draw_point_data_left[%d] is %f\n", i, draw_point_data_left[i]);
    }
    fb = &dest_vgbuf;

    vg_lite_matrix_t matrix;
    vg_lite_identity(&matrix);
	vg_lite_rotate(angle, &matrix);
    //vg_lite_clear(&dest_vgbuf, NULL, 0xFFFF00FF);

    // vg_lite_translate(fb_width / 2 - 100 * fb_width / 466.0f,
    //                 fb_height / 2 - 200 * fb_height / 466.0f, &matrix);
    // vg_lite_scale(1, 1, &matrix);
    // vg_lite_scale(fb_width / 466.0f, fb_height / 466.0f, &matrix);
    uint32_t cmd_size = data_size + 1;
    data_size = vg_lite_get_path_length(draw_point_cmd, cmd_size, VG_LITE_FP32);
    printf("data_size0 is %d\n",data_size);
    err = vg_lite_init_path(&path, VG_LITE_FP32, VG_LITE_HIGH, data_size  , NULL, 0, 0, 0, 0);
    if(err != VG_LITE_SUCCESS)
        printf("vg_lite_init_path failed.\n");
    printf("sizeof(draw_point_cmd) is %d\n",sizeof(draw_point_cmd));
    err = vg_lite_append_path(&path, draw_point_cmd, draw_point_data_left, cmd_size );
    if(err != VG_LITE_SUCCESS)
        printf("vg_lite_append_path failed.\n");

    err =vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, vgcol);
    if(err != VG_LITE_SUCCESS){
        printf("vg_lite_draw failed.\n");
    }
    vg_lite_finish();
    printf("awk_draw_polygon sucess.\n");
    vglite_buf_unmap(&dest_vgbuf);
	vg_lite_clear_path(&path);
    memset(&path,0,sizeof(vg_lite_path_t));

	awk_mem_free_adapter(draw_point_cmd);
	awk_mem_free_adapter(draw_point_data_left);
#endif

}

void awk_draw_polyline(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info)
{
	printf("awk_draw_polyline\n");
	uint32_t stc = os_uptime_get_32();

#if 1
	vg_lite_error_t err = VG_LITE_SUCCESS;
	vg_lite_buffer_t * fb;

    //uint32_t data_size;
    vg_lite_path_t path;
	vg_lite_color_t vgcol = param->style->color;
	printf("awk_draw_polyline is 0x%0x",vgcol);
    float angle = param->style->angle;
    uint32_t paint_width = param->style->width;

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

    awk_point_t *point = NULL;//param->points;
    fb = &dest_vgbuf;

    vg_lite_matrix_t matrix;
    vg_lite_identity(&matrix);
	vg_lite_rotate(angle, &matrix);

	#if 1
	point = param->points;
	awk_point_t *point2 = point;
    point2++;
	//printf("BBA:point.x is %d\n",point->x);
	//printf("BBA:point.y is %d\n",point->y);
	//printf("BBA:point2.x is %d\n",point2->x);
	//printf("BBA:point2.y is %d\n",point2->y);
	#else
    awk_point_t point_test;
	point_test.x = 280;
	point_test.y = 0;
	point = &point_test;
	awk_point_t point_test1;
	point_test1.x = 280;
	point_test1.y = 466;
    awk_point_t *point2 = &point_test1;
	#endif
#if 0

    int32_t line_path[] = { /*VG line path*/
        VLC_OP_MOVE, point->x, point->y,
        VLC_OP_LINE, point2->x, point2->y,
        VLC_OP_END
    };
#endif
	int32_t painted_length = param->style->dash_style.painted_length;     //!< ?斆???Ƞ
	int32_t unpainted_length = param->style->dash_style.unpainted_length;   //!< ?հ׳??Ƞ
	int32_t offset = param->style->dash_style.offset;             //!< ưʼλփ
	int32_t x_point = point->x;
	int32_t y_point = point->y;
	int32_t *line_path_temp = NULL;
	uint32_t linepointsize = 0;
	int k = 0;
	uint32_t max_y = point->y > point2->y ? point->y : point2->y;
	uint32_t min_y = point->y > point2->y ? point2->y : point->y;
	uint32_t max_x = point->x > point2->x ? point->x : point2->x;
	uint32_t min_x = point->x > point2->x ? point2->x : point->x;
	if(point->x == point2->x){
		linepointsize = (point2->y > point->y)  ? ( point2->y - point->y)/(painted_length + unpainted_length) : ( point->y - point2->y )/(painted_length + unpainted_length);
		//linepointsize = (uint16_t)ceil((double)abs(point2->y - point->y) / (painted_length + unpainted_length));
		int remainder = (point2->y > point->y)  ? ( point2->y - point->y)%(painted_length + unpainted_length) : ( point->y - point2->y )%(painted_length + unpainted_length);

		int dotCount = linepointsize * 2;
		if (remainder > 0) {
			//if (remainder >= unpainted_length) {
			dotCount = (linepointsize + 1) * 2;
			//}
		}
		//printf("line_path_temp is %d\n",sizeof(int32_t) * ( dotCount * 3 + 1));
		line_path_temp = (int32_t *)awk_mem_malloc_adapter( sizeof(int32_t) * ( dotCount * 3 + 1));
		if(line_path_temp == NULL)
		{
				printf("malloc line_path_temp is fail\n");
		}
		int i = 0;
		uint32_t y1_point = min_y + offset;
		for(i = 0; i < linepointsize; i++)
		{
			line_path_temp[k++]= VLC_OP_MOVE;
			line_path_temp[k++]= x_point;
			line_path_temp[k++]= y1_point;
			line_path_temp[k++]= VLC_OP_LINE;
			line_path_temp[k++]= x_point;
			y1_point += painted_length;
			if(y1_point > max_y)
				y1_point = max_y;
			line_path_temp[k++]= y1_point;
			//printf("BBA:point.x is %d\n",x_point);
			//printf("BBA:point.y is %d\n",y1_point);
			y1_point += unpainted_length;
			if(y1_point > max_y)
				y1_point = max_y;
		}
		line_path_temp[k] = VLC_OP_END;
	}

	if(point->y == point2->y){
		linepointsize = (point2->x > point->x) ? ( point2->x - point->x)/(painted_length + unpainted_length) : ( point->x - point2->x)/(painted_length + unpainted_length);
		//linepointsize = (uint16_t)ceil((double)abs(point2->x - point->x) / (painted_length + unpainted_length));
		int remainder = (point2->x > point->x) ? ( point2->x - point->x)/(painted_length + unpainted_length) : ( point->x - point2->x)/(painted_length + unpainted_length);
		int dotCount = linepointsize * 2;
		if (remainder > 0) {
			//if (remainder >= unpainted_length) {
			dotCount = (linepointsize + 1) * 2;
			//}
		}
		//printf("line_path_temp is %d\n",sizeof(int32_t) * ( dotCount * 3 + 1));
		line_path_temp = (int32_t *)awk_mem_malloc_adapter( sizeof(int32_t) * ( dotCount * 3 + 1));
			if(line_path_temp == NULL)
				printf("malloc line_path_temp is fail\n");
			int i = 0;

		uint32_t x1_point = min_x + offset;
			for(i = 0; i < linepointsize; i++)
			{
				line_path_temp[k++]= VLC_OP_MOVE;
				line_path_temp[k++]= x1_point;
				line_path_temp[k++]= y_point;
				line_path_temp[k++]= VLC_OP_LINE;
				x1_point += painted_length;
				if(x1_point > max_x) {
					x1_point = max_x;
				}
				line_path_temp[k++]= x1_point;
				line_path_temp[k++]= y_point;
				x1_point += unpainted_length;
				if(x1_point > max_x) {
					x1_point = max_x;
				}
			}
			line_path_temp[k] = VLC_OP_END;
		}


    //printf("draw_point  point->x is %d\n", point->x);
    //printf("draw_point  point->y is %d\n", point->y);
    //printf("draw_point  point2->x is %d\n", point2->x);
    //printf("draw_point  point2->y is %d\n", point2->x);
	//printf("draw_point line_path is %d\n", sizeof(line_path));
	uint32_t path_size_t = sizeof(int32_t) * ( linepointsize * 3 + 1);
	printf("awk path_size_t is %d\n ",path_size_t);

    err = vg_lite_init_path(&path, VG_LITE_S32, VG_LITE_HIGH, sizeof(int32_t) * k, line_path_temp, 0, 0, 466, 466);
	if(err != VG_LITE_SUCCESS)
		   printf("Init path failed.\n");


    /*** Draw line ***/
    err = vg_lite_set_draw_path_type(&path, VG_LITE_DRAW_STROKE_PATH);
    if(err != VG_LITE_SUCCESS)
        printf("Set draw path type failed.\n");

    err = vg_lite_set_stroke(&path, VG_LITE_CAP_ROUND, VG_LITE_JOIN_ROUND, paint_width, 8, NULL, 0, 8, vgcol/*0xFF000000*/);
    if(err != VG_LITE_SUCCESS)
    {
    	printf("Set stroke failed.\n");
    }

    err = vg_lite_update_stroke(&path);
    if(err != VG_LITE_SUCCESS){
		printf("Update stroke failed.\n");
	}
	//for(int i=0;i<30;i++){
    	err =vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, vgcol);
    	if(err != VG_LITE_SUCCESS){
        	printf("vg_lite_draw failed.\n");
    	}
	//}
    vg_lite_finish();
	printf("%s, begin stc:%u, spend %u ms\n", __func__, stc, os_uptime_get_32() - stc);
    printf("awk_draw_polyline sucess.\n");

    vglite_buf_unmap(&dest_vgbuf);
	vg_lite_clear_path(&path);
	awk_mem_free_adapter(line_path_temp);
    memset(&path,0,sizeof(vg_lite_path_t));
#endif
}

void awk_draw_point(t_drawing_points_param *param, t_awk_view_buffer_info *buffer_info)
{
	printf("awk_draw_point\n");

	vg_lite_error_t err = VG_LITE_SUCCESS;
	vg_lite_buffer_t * fb;
    //uint32_t data_size;
    vg_lite_path_t path;
    float angle = param->style->angle;

    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

    awk_point_t *point = param->points;
    fb = &dest_vgbuf;

    vg_lite_matrix_t matrix;
    vg_lite_identity(&matrix);
	vg_lite_rotate(angle, &matrix);
    //vg_lite_clear(&dest_vgbuf, NULL, 0xFF000000);

    // vg_lite_translate(fb_width / 2 - 100 * fb_width / 466.0f,
    //                 fb_height / 2 - 200 * fb_height / 466.0f, &matrix);
    // vg_lite_scale(1, 1, &matrix);
    // vg_lite_scale(fb_width / 466.0f, fb_height / 466.0f, &matrix);

	// awk_point_t *point2 = point;
    // point2++;
    int32_t point_path[] = { /*VG line path*/
        VLC_OP_MOVE, point->x, point->y,
        VLC_OP_LINE, point->x + 1, point->y + 1,
        VLC_OP_END
    };
    printf("draw_point  point->x is %d\n", point->x);
    printf("draw_point  point->y is %d\n", point->y);
    // printf("draw_point  point2->x is %d\n", point2->x);
    // printf("draw_point  point2->y is %d\n", point2->x);
    err = vg_lite_init_path(&path, VG_LITE_S32, VG_LITE_HIGH, sizeof(point_path), point_path, 0, 0, 466, 466);
	if(err != VG_LITE_SUCCESS)
		   printf("Init path failed.\n");

    /*** Draw line ***/
    err = vg_lite_set_draw_path_type(&path, VG_LITE_DRAW_STROKE_PATH);
    if(err != VG_LITE_SUCCESS)
        printf("Set draw path type failed.\n");

    err = vg_lite_set_stroke(&path, VG_LITE_CAP_ROUND, VG_LITE_JOIN_ROUND, 30, 8, NULL, 0, 8, 0xFFFFFFFF);
    if(err != VG_LITE_SUCCESS)
        printf("Set stroke failed.\n");

    err = vg_lite_update_stroke(&path);
    if(err != VG_LITE_SUCCESS)
        printf("Update stroke failed.\n");


    err =vg_lite_draw(fb, &path, VG_LITE_FILL_EVEN_ODD, &matrix, VG_LITE_BLEND_NONE, 0xFFFFFFFF);
    if(err != VG_LITE_SUCCESS){
        printf("vg_lite_draw failed.\n");
    }
    vg_lite_finish();
    printf("awk_draw_point sucess.\n");
    vglite_buf_unmap(&dest_vgbuf);
	vg_lite_clear_path(&path);
    memset(&path,0,sizeof(vg_lite_path_t));
}

void awk_draw_color(t_draw_color_param *param,t_awk_view_buffer_info *buffer_info)
{
	//awk_draw_text((t_draw_text_param*)param, buffer_info);
	//awk_draw_textsvg2((t_draw_text_param*)param, buffer_info);
	printf("awk_draw_color\n");

    vg_lite_error_t err = VG_LITE_SUCCESS;
    vg_lite_color_t vgcol = param->style->color;
	printf("param->style->color is %d\n", vgcol);
    printf("target buffer_info->cf is %d\n", buffer_info->cf);

    //float angle = param->style->angle;
    vg_lite_buffer_t dest_vgbuf;
    vg_lite_bufferconvert(&dest_vgbuf, buffer_info, buffer_info->data);

    vg_lite_rectangle_t src_rect = {
        .x = param->area.x,
        .y =param->area.y,
        .width = param->area.width,
        .height = param->area.height
    };

    err = vg_lite_clear(&dest_vgbuf, &src_rect, vgcol);
    if(err != VG_LITE_SUCCESS)
        printf("vg_lite_clear failed.\n");

    vg_lite_finish();
    vglite_buf_unmap(&dest_vgbuf);
    printf("awk_draw_color succes.\n");
	printf("awk_draw_color is %d.\n", vgcol);
}
