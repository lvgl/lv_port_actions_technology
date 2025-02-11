/****************************************************************************
*
*    Copyright 2023 Vivante Corporation, Santa Clara, California.
*    All Rights Reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining
*    a copy of this software and associated documentation files (the
*    'Software'), to deal in the Software without restriction, including
*    without limitation the rights to use, copy, modify, merge, publish,
*    distribute, sub license, and/or sell copies of the Software, and to
*    permit persons to whom the Software is furnished to do so, subject
*    to the following conditions:
*
*    The above copyright notice and this permission notice (including the
*    next paragraph) shall be included in all copies or substantial
*    portions of the Software.
*
*    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#define _GNU_SOURCE /* make strtok_r() visible */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>

#ifdef CONFIG_UI_MEMORY_MANAGER
#  include <ui_mem.h>
#  define NANOSVG_MALLOC(size)        ui_mem_alloc(MEM_RES, size, __func__)
#  define NANOSVG_FREE(ptr)           ui_mem_free(MEM_RES, ptr)
#  define NANOSVG_REALLOC(ptr, size)  ui_mem_realloc(MEM_RES, ptr, size, __func__)
#else
#  define NANOSVG_MALLOC(size)        malloc(size)
#  define NANOSVG_FREE(ptr)           free(ptr)
#  define NANOSVG_REALLOC(ptr, size)  realloc(ptr, size)
#endif

#define NANOSVG_PRINT(str, ...)     printf(str, ##__VA_ARGS__)
#define NANOSVG_TRACE(str, ...)     printf(str, ##__VA_ARGS__)
#define NANOSVG_GETTIME             k_uptime_get_32

#define NANOSVG_ALL_COLOR_KEYWORDS                // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION                    // Expands NanoSVG implementation
#define VG_STABLE_MODE                        1   // Enhance stability when drawing complex paths.

#define DUMP_API                              0   // Dump VGLite API.
#define USE_BOUNDARY_JUDGMENT                 0   // When USE_BOUNDARY_JUDGMENT is enabled, determine if the rendering boundary of path is within buffer.


#if VG_STABLE_MODE
#define MAX_COMMITTED_PATHS                   4   // set 4 for hero.svg
static int commit_count = 0;
#endif

#include "nanosvg.h"
#include "vglite_renderer.h"

extern int spng_load_memory(vg_lite_buffer_t * buffer, const void * png_bytes, size_t png_len);
extern int spng_free_buffer(vg_lite_buffer_t * buffer);

#ifdef CONFIG_FREETYPE_FONT
#  include <svg_font.h>
#endif

#define VGL_ERROR_CHECK(func) \
    if ((error = func) != VG_LITE_SUCCESS) \
    goto VGLError

#define swap(a,b) {int temp; temp=a; a=b; b=temp;}

#define  PATH_DATA_BUFFER_SIZE  8192
#define  PATTERN_BUFFER_SIZE    1024

#define  VG_LITE_CMDBUF_SIZE    (128 << 10)

static vg_lite_float_t path_data_buf[PATH_DATA_BUFFER_SIZE];
static vg_lite_float_t pattern_data_buf[PATTERN_BUFFER_SIZE];

static vg_lite_color_ramp_t grad_ramps[VLC_MAX_COLOR_RAMP_STOPS];

static uint32_t grad_colors[VLC_MAX_GRADIENT_STOPS];
static uint32_t grad_stops[VLC_MAX_GRADIENT_STOPS];

static const vg_lite_gradient_spreadmode_t vglspread[] = { VG_LITE_GRADIENT_SPREAD_PAD, VG_LITE_GRADIENT_SPREAD_REFLECT, VG_LITE_GRADIENT_SPREAD_REPEAT };

static vg_lite_error_t error = VG_LITE_SUCCESS;
static vg_lite_matrix_t global_matrix;
static vg_lite_path_t vgl_path;
#ifdef CONFIG_FREETYPE_FONT
static vg_lite_path_t fontpath;
static unsigned int unicode;
#endif
static vg_lite_fill_t fill_rule;
static vg_lite_float_t shape_bound[4]; // [minx,miny,maxx,maxy]
static vg_lite_float_t global_scale = 1.0f;

static const struct NSVGImageDecoder *g_imagedec;

#ifdef CONFIG_FREETYPE_FONT

static void setOpacity(vg_lite_color_t* pcolor, float opacity)
{
    if (opacity != 1.0f)
    {
        vg_lite_color_t color = *pcolor;
        uint8_t a = (uint8_t)(((color & 0xFF000000) >> 24) * opacity);
        uint8_t b = (uint8_t)(((color & 0x00FF0000) >> 16) * opacity);
        uint8_t g = (uint8_t)(((color & 0x0000FF00) >> 8) * opacity);
        uint8_t r = (uint8_t)(((color & 0x000000FF) >> 0) * opacity);
        *pcolor = ((r << 0) | (g << 8) | (b << 16) | (a << 24));
    }
}

static vg_lite_error_t drawCharacter(vg_lite_buffer_t * render_buffer_ptr, NSVGshape* shape, svgfont_t * font, svgfont_glyph_dsc_t * g,
        uint32_t unicode, float x, float y, unsigned int color)
{
    vg_lite_matrix_t font_matrix;
    memcpy(&font_matrix, &global_matrix, sizeof(global_matrix));

    VGL_ERROR_CHECK(svgfont_generate_path_and_matrix(font, g, &fontpath, &font_matrix,
                    unicode, x, y, true));

    if (shape->fill.text->hasStroke == 1)
    {
        vg_lite_cap_style_t linecap = VG_LITE_CAP_BUTT + shape->strokeLineCap;
        vg_lite_join_style_t joinstyle = VG_LITE_JOIN_MITER + shape->strokeLineJoin;
        vg_lite_float_t strokewidth = shape->strokeWidth ;

        vg_lite_color_t stroke_color = shape->stroke.color;

        /* Render the stroke path only */
        fontpath.path_type = VG_LITE_DRAW_STROKE_PATH;
        /* Disable anti-aliasing line rendering for thin stroke line */
        fontpath.quality = VG_LITE_MEDIUM;
        setOpacity(&stroke_color, shape->opacity);

        /* Setup stroke parameters properly */
        VGL_ERROR_CHECK(vg_lite_set_stroke(&fontpath, linecap, joinstyle, strokewidth, shape->miterLimit,
            shape->strokeDashArray, shape->strokeDashCount, shape->strokeDashOffset, 0xFFFFFFFF));
        VGL_ERROR_CHECK(vg_lite_update_stroke(&fontpath));

        /* Draw stroke path with stroke color */
        fontpath.stroke_color = stroke_color;
    }

    VGL_ERROR_CHECK(vg_lite_draw(render_buffer_ptr, &fontpath, VG_LITE_FILL_EVEN_ODD, &font_matrix, VG_LITE_BLEND_SRC_OVER, color));
    VGL_ERROR_CHECK(vg_lite_clear_path(&fontpath));

    return 0;

VGLError:
    return 1;
}

static vg_lite_error_t renderText(vg_lite_buffer_t * render_buffer_ptr, NSVGshape * shape, unsigned int color, void * ft_face)
{
    svgfont_glyph_dsc_t glyph_dsc = { 0 };

    float defaul_size = 16;
    float font_size, pos_x, pos_y;
    char* text_content;

    pos_x = shape->fill.text->x * global_scale;
    pos_y = shape->fill.text->y * global_scale;

    if (svg_scale_image_width > 0)
        font_size = shape->fill.text->fontSize * global_scale * (svg_scale_image_width / svg_image_width);
    else
        font_size = shape->fill.text->fontSize * global_scale;

    text_content = shape->fill.text->content;

    svgfont_set_size(ft_face, font_size != 0 ? font_size  : defaul_size);

    unsigned char character;
    int i = 0;

    while(text_content[i] != '\0')
    {
        character = text_content[i];

        if((character & 0xE0) == 0xC0)
        {
            /* Two bytes UTF - 8 encoding. */
            unicode = ((character & 0x1F) << 6) | (text_content[i + 1] & 0x3F);
            i += 2;
        }
        else if ((character & 0xF0) == 0xE0)
        {
            /*Three bytes UTF-8 encoding. */
            unicode = ((character & 0x0F) << 12) | ((text_content[i+1] & 0x3F) << 6) | (text_content[i+2] & 0x3F);
            i += 3;
        }
        else if ((character & 0xF8) == 0xF0)
        {
            /* Four bytes UTF-8 encoding.*/
            unicode = ((character & 0x07) << 18) | ((text_content[i+1] & 0x3F) << 12) | ((text_content[i+2] & 0x3F) << 6) | (text_content[i+3] & 0x3F);
            i += 4;
        }
        else
        {   /* ASCII encoding.*/
            unicode = character;
            i++;
        }

        if (svgfont_get_glyph_dsc(ft_face, &glyph_dsc, unicode))
        {
            NANOSVG_PRINT("error in load glyph 0x%x.\n", unicode);
            continue;
        }

        drawCharacter(render_buffer_ptr, shape, ft_face, &glyph_dsc, unicode, pos_x, pos_y, color);

        pos_x = pos_x + glyph_dsc.adv_w;
    }
    return 0;
}
#endif /* CONFIG_FREETYPE_FONT */

static vg_lite_error_t renderImage(vg_lite_buffer_t* render_buffer_ptr, NSVGdefsImage* image_ptr, char is_pattern_mode)
{
    if (!image_ptr->drawFlag)
    {
        NANOSVG_PRINT("Bypass an image because of its oversize.\n");
        return 0;
    }

    NSVGdefsImage* image = image_ptr;
    vg_lite_buffer_t src;
    vg_lite_matrix_t image_matrix;
    vg_lite_blend_t blend = OPENVG_BLEND_SRC_OVER;
    const char* s = image->imgdata;
    unsigned char* output;
    size_t len;
    int pos_x, pos_y;
    bool not_url = strcmp(image->format, NSVG_IMAGE_FORMAT_URL);

    if (not_url) {
        /* Decode image data. */
        if (nsvgBase64Decode(s, &output, &len) < 0) {
            NANOSVG_PRINT("nsvgBase64Decode failed\n");
            return 0;
        }

        /* Load decoded png data to vglite buffer. */
        if (spng_load_memory(&src, output, len)) {
            NANOSVG_PRINT("png load failed\n");
            goto VGLError;
        }

        src.compress_mode = VG_LITE_DEC_DISABLE;
        src.image_mode = VG_LITE_ZERO;
        src.tiled = VG_LITE_LINEAR;
    } else if (g_imagedec) {
        if (g_imagedec->decode(g_imagedec, &src, s)) {
            NANOSVG_PRINT("image decode load failed\n");
            return 0;
        }
    } else {
        NANOSVG_PRINT("no image decoder registered\n");
        return 0;
    }

    pos_x = image->x;
    pos_y = image->y;

    if (!is_pattern_mode)
        memcpy(&image_matrix, &global_matrix, sizeof(global_matrix));
    else
        vg_lite_identity(&image_matrix);

    VGL_ERROR_CHECK(vg_lite_translate(pos_x * global_scale, pos_y * global_scale, &image_matrix));
    float image_scale_size = src.height > src.width ? (float)image->height * global_scale / (float)src.height : (float)image->width * global_scale / (float)src.width;
    int image_translate_size;
    if (image->height > image->width)
    {
        image_translate_size = (image->width * global_scale - src.width * image_scale_size) / 2;
        VGL_ERROR_CHECK(vg_lite_translate(image_translate_size, 0, &image_matrix));
    }
    else
    {
        image_translate_size = (image->height * global_scale - src.height * image_scale_size) / 2;
        VGL_ERROR_CHECK(vg_lite_translate(0, image_translate_size, &image_matrix));
    }
    VGL_ERROR_CHECK(vg_lite_scale(image_scale_size, image_scale_size,&image_matrix));

    VGL_ERROR_CHECK(vg_lite_blit(render_buffer_ptr, &src, &image_matrix, blend, 0, 0));
    VGL_ERROR_CHECK(vg_lite_finish());

    if (not_url) {
        spng_free_buffer(&src);
        NANOSVG_FREE(output);
    } else if (g_imagedec && g_imagedec->release) {
        g_imagedec->release(g_imagedec, &src);
    }

    return 0;

VGLError:
    if (not_url)
        NANOSVG_FREE(output);
    return 1;
}

static vg_lite_error_t generateVGLitePath(NSVGpath* shape_path, vg_lite_path_t* vgl_path, vg_lite_path_type_t path_type, char nsvg_path_type)
{
    float* d = &((float*)vgl_path->path)[0];

    for (NSVGpath* path = shape_path; path != NULL; path = path->next)
    {
        float* s = &path->pts[0];

        /* Exit if remaining path_data_buf[] is not sufficient to hold the path data */
        if (nsvg_path_type == NSVG_PATH || nsvg_path_type == NSVG_CLIP_PATH)
        {
            float* bufend = d + PATH_DATA_BUFFER_SIZE;
            if ((bufend - d) < (7 * path->npts + 10) / 3)
            {
                NANOSVG_PRINT("Error: Need to increase PATH_DATA_BUFFER_SIZE for path_data_buf[].\n");
                goto VGLError;
            }
        }
        else if (nsvg_path_type == NSVG_PATH)
        {
            float* bufend = d + PATTERN_BUFFER_SIZE;
            if ((bufend - d) < (7 * path->npts + 10) / 3)
            {
                NANOSVG_PRINT("Error: Need to increase PATTERN_BUFFER_SIZE for path_data_buf[].\n");
                goto VGLError;
            }
        }

        /* Create VGLite cubic bezier path data from path->pts[] */
        *((char*)d) = VLC_OP_MOVE; d++;
        *d++ = (*s++) * global_scale;
        *d++ = (*s++) * global_scale;
        for (int i = 0; i < path->npts - 1; i += 3)
        {
            *((char*)d) = VLC_OP_CUBIC; d++;
            *d++ = (*s++) * global_scale;
            *d++ = (*s++) * global_scale;
            *d++ = (*s++) * global_scale;
            *d++ = (*s++) * global_scale;
            *d++ = (*s++) * global_scale;
            *d++ = (*s++) * global_scale;
        }

        if (path->closed)
        {
            *((char*)d) = VLC_OP_CLOSE; d++;
        }
    }
    if (path_type != VG_LITE_DRAW_STROKE_PATH)
    {
        *((char*)d) = VLC_OP_END; d++;
    }

    /* Compute the accurate VGLite path data length */
    vgl_path->path_length = ((char*)d - (char*)vgl_path->path);

    return 0;

VGLError:
    return 1;
}

static int renderGradientPath(vg_lite_buffer_t* render_buffer_ptr, NSVGpaint *paint, char *gradid, unsigned int color, float opacity,
                              float fill_opacity, float stroke_opacity, int is_fill, int is_stroke, char* patternid,
                              int shape_blend)
{
    vg_lite_blend_t blend_mode = VG_LITE_BLEND_NONE;
    if (is_fill)
    {
        if (opacity < 1 || fill_opacity < 1)
            blend_mode = VG_LITE_BLEND_SRC_OVER;
    }
    else if (is_stroke)
    {
        if (stroke_opacity < 1)
            blend_mode = VG_LITE_BLEND_SRC_OVER;
    }
    if (patternid[0] != '\0' && is_fill)
        blend_mode = VG_LITE_BLEND_SRC_OVER;

    if(shape_blend == VG_MIXBLENDMODE_SRCIN)
        blend_mode = VG_LITE_BLEND_SRC_IN;

    if (gradid[0] != '\0')
    {
        int grad_blend = VG_LITE_BLEND_SRC_OVER;
        if (shape_blend == VG_MIXBLENDMODE_MULTIPLY)
        {
            grad_blend = VG_LITE_BLEND_MULTIPLY;
        }
        else if (shape_blend == VG_MIXBLENDMODE_SCREEN)
        {
            grad_blend = VG_LITE_BLEND_SCREEN;
        }

        if (paint->type == NSVG_PAINT_LINEAR_GRADIENT)
        {
            if (vg_lite_query_feature(gcFEATURE_BIT_VG_LINEAR_GRADIENT_EXT))
            {
                vg_lite_ext_linear_gradient_t grad;
                vg_lite_linear_gradient_parameter_t grad_param;
                vg_lite_matrix_t* grad_matrix;
                float dx, dy, X0, X1, Y0, Y1;

                memset(&grad, 0, sizeof(grad));

                for (int i = 0; i < paint->gradient->nstops; i++) {
                    setOpacity(&(paint->gradient->stops[i].color), opacity);
                    grad_ramps[i].stop = paint->gradient->stops[i].offset;
                    grad_ramps[i].alpha = ((paint->gradient->stops[i].color & 0xFF000000) >> 24) / 255.0f;
                    grad_ramps[i].blue = ((paint->gradient->stops[i].color & 0x00FF0000) >> 16) / 255.0f;
                    grad_ramps[i].green = ((paint->gradient->stops[i].color & 0x0000FF00) >> 8) / 255.0f;;
                    grad_ramps[i].red = (paint->gradient->stops[i].color & 0x000000FF) / 255.0f;;
                }

                X0 = paint->gradient->param[0];
                Y0 = paint->gradient->param[1];
                X1 = paint->gradient->param[2];
                Y1 = paint->gradient->param[3];
                dx = shape_bound[2] - shape_bound[0];
                dy = shape_bound[3] - shape_bound[1];
                if (paint->gradient->units == NSVG_OBJECT_SPACE)
                {
                    grad_param.X0 = shape_bound[0] + dx * X0;
                    grad_param.X1 = shape_bound[0] + dx * X1;
                    grad_param.Y0 = shape_bound[1] + dy * Y0;
                    grad_param.Y1 = shape_bound[1] + dy * Y1;
                }
                else
                {
                    grad_param.X0 = X0 * global_scale;
                    grad_param.X1 = X1 * global_scale;
                    grad_param.Y0 = Y0 * global_scale;
                    grad_param.Y1 = Y1 * global_scale;
                }

                VGL_ERROR_CHECK(vg_lite_set_linear_grad(&grad, paint->gradient->nstops, grad_ramps, grad_param, vglspread[(int)paint->gradient->spread], 0));

                grad_matrix = vg_lite_get_linear_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));
                if (1 /*paint->gradient->xform != NULL*/)
                {
                    int j = 0;
                    for (int i = 0; i < 3; i++)
                    {
                        grad_matrix->m[j % 2][i] = paint->gradient->xform[j];
                        j++;
                        grad_matrix->m[j % 2][i] = paint->gradient->xform[j];
                        j++;
                    }
                }

                VGL_ERROR_CHECK(vg_lite_update_linear_grad(&grad));

                VGL_ERROR_CHECK(vg_lite_draw_linear_grad(render_buffer_ptr, &vgl_path, VG_LITE_FILL_EVEN_ODD, &global_matrix, &grad, color, grad_blend, VG_LITE_FILTER_POINT));

                VGL_ERROR_CHECK(vg_lite_finish());
                VGL_ERROR_CHECK(vg_lite_clear_linear_grad(&grad));
            }
            else
            {
                vg_lite_linear_gradient_t grad;
                vg_lite_matrix_t* grad_matrix;
                unsigned int alpha, blue, green, red;
                int x1, x2;
                float stops, X0, X1, Y0, Y1, angle;

                memset(&grad, 0, sizeof(grad));

                X0 = paint->gradient->param[0];
                Y0 = paint->gradient->param[1];
                X1 = paint->gradient->param[2];
                Y1 = paint->gradient->param[3];

                if ((X0 == 0 && X1 == 0) && ((Y0 != 0) || (Y1 != 0))) {
                    angle = 90;
                    X0 = Y0;
                    X1 = Y1;
                }
                else {
                    angle = 0;
                }

                if (paint->gradient->units == NSVG_OBJECT_SPACE)
                {
                    x1 = shape_bound[0] + (shape_bound[2] - shape_bound[0]) * X0;
                    x2 = shape_bound[0] + (shape_bound[2] - shape_bound[0]) * X1;
                }
                else
                {
                    x1 = paint->gradient->param[0] * global_scale;
                    x2 = paint->gradient->param[2] * global_scale;
                }

                if (x1 > x2)
                    swap(x1, x2);

                for (int i = 0; i < paint->gradient->nstops; i++) {
                    unsigned int temp_color = paint->gradient->stops[i].color;
                    setOpacity(&temp_color, opacity);

                    alpha = (temp_color & 0xFF000000) >> 24;
                    blue = (temp_color & 0x00FF0000) >> 16;
                    green = (temp_color & 0x0000FF00) >> 8;
                    red = (temp_color & 0x000000FF) >> 0;
                    grad_colors[i] = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);

                    stops = x1 + paint->gradient->stops[i].offset * (x2 - x1);
                    grad_stops[i] = stops > 0 ? stops : 0;
                }

                VGL_ERROR_CHECK(vg_lite_init_grad(&grad));
                VGL_ERROR_CHECK(vg_lite_set_grad(&grad, paint->gradient->nstops, grad_colors, grad_stops));
                VGL_ERROR_CHECK(vg_lite_update_grad(&grad));

                grad_matrix = vg_lite_get_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));
                if (paint->gradient->units == NSVG_OBJECT_SPACE)
                    VGL_ERROR_CHECK(vg_lite_rotate(angle, grad_matrix));

                VGL_ERROR_CHECK(vg_lite_draw_grad(render_buffer_ptr, &vgl_path, VG_LITE_FILL_EVEN_ODD, &global_matrix, &grad, grad_blend));

                VGL_ERROR_CHECK(vg_lite_finish());
                VGL_ERROR_CHECK(vg_lite_clear_grad(&grad));
            }
        }
        else if (paint->type == NSVG_PAINT_RADIAL_GRADIENT)
        {
            if (vg_lite_query_feature(gcFEATURE_BIT_VG_RADIAL_GRADIENT)) {
                vg_lite_radial_gradient_t grad;
                vg_lite_radial_gradient_parameter_t grad_param;
                vg_lite_matrix_t* grad_matrix;
                float cx, cy, r, fx, fy;
                float dx, dy;

                memset(&grad, 0, sizeof(grad));

                for (int i = 0; i < paint->gradient->nstops; i++) {

                    if (((((paint->gradient->stops[i].color & 0xFF000000) >> 24) / 255.0f) < 1) && grad_blend == VG_LITE_BLEND_SRC_OVER)
                        grad_blend = OPENVG_BLEND_SRC_OVER;

                    setOpacity(&(paint->gradient->stops[i].color), opacity);
                    grad_ramps[i].stop = paint->gradient->stops[i].offset;
                    grad_ramps[i].alpha = ((paint->gradient->stops[i].color & 0xFF000000) >> 24) / 255.0f;
                    grad_ramps[i].blue = ((paint->gradient->stops[i].color & 0x00FF0000) >> 16) / 255.0f;
                    grad_ramps[i].green = ((paint->gradient->stops[i].color & 0x0000FF00) >> 8) / 255.0f;
                    grad_ramps[i].red = (paint->gradient->stops[i].color & 0x000000FF) / 255.0f;
                }

                cx = paint->gradient->param[0];
                cy = paint->gradient->param[1];
                r = paint->gradient->param[2];
                fx = paint->gradient->param[3];
                fy = paint->gradient->param[4];
                dx = shape_bound[2] - shape_bound[0];
                dy = shape_bound[3] - shape_bound[1];
                if (paint->gradient->units == NSVG_OBJECT_SPACE)
                {
                    grad_param.cx = shape_bound[0] + cx * dx;
                    grad_param.cy = shape_bound[1] + cy * dy;
                    grad_param.r = r * (dx > dy ? dx : dy);
                    grad_param.fx = (fx == 0.0f) ? grad_param.cx : (shape_bound[0] + fx * dx);
                    grad_param.fy = (fy == 0.0f) ? grad_param.cy : (shape_bound[1] + fy * dy);
                }
                else
                {
                    grad_param.cx = cx * global_scale;
                    grad_param.cy = cy * global_scale;
                    grad_param.r = r * global_scale;
                    grad_param.fx = (fx == 0.0f) ? grad_param.cx : (fx * global_scale);
                    grad_param.fy = (fy == 0.0f) ? grad_param.cy : (fy * global_scale);
                }

                VGL_ERROR_CHECK(vg_lite_set_radial_grad(&grad, paint->gradient->nstops, grad_ramps, grad_param, vglspread[(int)paint->gradient->spread], 0));
                VGL_ERROR_CHECK(vg_lite_update_radial_grad(&grad));

                grad_matrix = vg_lite_get_radial_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));
                if (1 /*paint->gradient->xform != NULL*/)
                {
                    int j = 0;
                    for (int i = 0; i < 3; i++)
                    {
                        grad_matrix->m[j % 2][i] = paint->gradient->xform[j];
                        j++;
                        grad_matrix->m[j % 2][i] = paint->gradient->xform[j];
                        j++;
                    }
                }
                VGL_ERROR_CHECK(vg_lite_draw_radial_grad(render_buffer_ptr, &vgl_path, VG_LITE_FILL_EVEN_ODD, &global_matrix, &grad, color, grad_blend, VG_LITE_FILTER_POINT));

                VGL_ERROR_CHECK(vg_lite_finish());
                VGL_ERROR_CHECK(vg_lite_clear_radial_grad(&grad));
            }
            else
            {
                NANOSVG_PRINT("Radial gradient is not supported!\n");
                vg_lite_finish();
                return 0;
            }
        }
    }
    else
    {
        /* Render the shape with VGLite cubic bezier path data */
        VGL_ERROR_CHECK(vg_lite_draw(render_buffer_ptr, &vgl_path, fill_rule, &global_matrix, blend_mode, color));
    }

    return 0;

VGLError:
    return 1;
}

static int renderPatternPath(vg_lite_buffer_t *render_buffer_ptr, NSVGpaint* paint, char* patternid)
{
    if (patternid[0] != '\0')
    {
        /* Render path to source buffer */
        vg_lite_path_t pattern_path;
        vg_lite_float_t pattern_shape_bound[4];
        NSVGshape* pattern_shape = paint->pattern->shape;
        vg_lite_path_type_t path_type;
        vg_lite_matrix_t matrix;
        vg_lite_matrix_t vglmatrix;
        vg_lite_matrix_t draw_pattern_matrix;
        vg_lite_buffer_t pattern_buf;

        memset(&pattern_buf, 0, sizeof(vg_lite_buffer_t));

        VGL_ERROR_CHECK(vg_lite_identity(&matrix));
        VGL_ERROR_CHECK(vg_lite_identity(&draw_pattern_matrix));
        VGL_ERROR_CHECK(vg_lite_identity(&vglmatrix));

        if (paint->pattern->units == NSVG_OBJECT_SPACE)
        {
            pattern_buf.width = (shape_bound[2] - shape_bound[0]) * paint->pattern->width * global_scale;
            pattern_buf.height = (shape_bound[3] - shape_bound[1]) * paint->pattern->height * global_scale;
        }
        else
        {
            pattern_buf.width = paint->pattern->width * global_scale;
            pattern_buf.height = paint->pattern->height * global_scale;
        }

        pattern_buf.format = VG_LITE_RGBA8888;
        VGL_ERROR_CHECK(vg_lite_allocate(&pattern_buf));
        VGL_ERROR_CHECK(vg_lite_clear(&pattern_buf, NULL, 0xFFFFFFFF));

        if (pattern_shape != NULL)
        {
            fill_rule = VG_LITE_FILL_NON_ZERO - pattern_shape->fillRule;

            for (; pattern_shape; pattern_shape = pattern_shape->next)
            {
                memset(&pattern_path, 0, sizeof(vg_lite_path_t));
                pattern_shape_bound[0] = pattern_shape->bounds[0] * global_scale;
                pattern_shape_bound[1] = pattern_shape->bounds[1] * global_scale;
                pattern_shape_bound[2] = pattern_shape->bounds[2] * global_scale;
                pattern_shape_bound[3] = pattern_shape->bounds[3] * global_scale;

                VGL_ERROR_CHECK(vg_lite_init_path(&pattern_path, VG_LITE_FP32, VG_LITE_MEDIUM, 0, &pattern_data_buf[0],
                    pattern_shape_bound[0], pattern_shape_bound[1], pattern_shape_bound[2], pattern_shape_bound[3]));

                path_type = 0;
                if (pattern_shape->fill.type != NSVG_PAINT_NONE)
                    path_type |= VG_LITE_DRAW_FILL_PATH;
                if (pattern_shape->stroke.type != NSVG_PAINT_NONE)
                    path_type |= VG_LITE_DRAW_STROKE_PATH;


                VGL_ERROR_CHECK(generateVGLitePath(pattern_shape->paths, &pattern_path, path_type, pattern_shape->pathType));

                if (pattern_shape->stroke.type != NSVG_PAINT_NONE)
                {
                    vg_lite_cap_style_t linecap = VG_LITE_CAP_BUTT + pattern_shape->strokeLineCap;
                    vg_lite_join_style_t joinstyle = VG_LITE_JOIN_MITER + pattern_shape->strokeLineJoin;
                    vg_lite_float_t strokewidth = pattern_shape->strokeWidth * global_scale;
                    vg_lite_color_t stroke_color = pattern_shape->stroke.color;

                    pattern_path.path_type = VG_LITE_DRAW_STROKE_PATH;
                    if (pattern_shape->opacity < 1 || pattern_shape->fill_opacity == 0)
                    {
                        vg_lite_color_t color = stroke_color;
                        uint8_t a = (uint8_t)(((color & 0xFF000000) >> 24) * pattern_shape->opacity);
                        stroke_color = (color & 0x00ffffff) | ((a << 24) & 0xff000000);
                    }
                    else
                    {
                        setOpacity(&stroke_color, pattern_shape->opacity);
                    }

                    VGL_ERROR_CHECK(vg_lite_set_stroke(&pattern_path, linecap, joinstyle, strokewidth, pattern_shape->miterLimit,
                        pattern_shape->strokeDashArray, pattern_shape->strokeDashCount, pattern_shape->strokeDashOffset, 0xFFFFFFFF));
                    VGL_ERROR_CHECK(vg_lite_update_stroke(&pattern_path));

                    pattern_path.stroke_color = stroke_color;
                }

                VGL_ERROR_CHECK(vg_lite_draw(&pattern_buf, &pattern_path, fill_rule, &draw_pattern_matrix, VG_LITE_BLEND_NONE, pattern_shape->fill.color));

            }
        }
        else
        {
            renderImage(&pattern_buf, paint->pattern->image, 1);
        }

        if (vg_lite_query_feature(gcFEATURE_BIT_VG_IM_REPEAT_REFLECT))
        {
            /* Render source buffer to the specified path with repaet/reflect support */
            if (paint->pattern->units == NSVG_OBJECT_SPACE)
            {
                VGL_ERROR_CHECK(vg_lite_translate((shape_bound[2] - shape_bound[0]) * paint->pattern->x + shape_bound[0], (shape_bound[3] - shape_bound[1]) * paint->pattern->y + shape_bound[1], &matrix));
            }
            else
            {
                VGL_ERROR_CHECK(vg_lite_translate(paint->pattern->x, paint->pattern->y, &matrix));
            }

            VGL_ERROR_CHECK(vg_lite_draw_pattern(render_buffer_ptr, &vgl_path, VG_LITE_FILL_EVEN_ODD, &vglmatrix, &pattern_buf, &matrix, VG_LITE_BLEND_NONE, VG_LITE_PATTERN_REPEAT, 0, 0, VG_LITE_FILTER_POINT));
            VGL_ERROR_CHECK(vg_lite_finish());
        }
        else
        {
            /* Render source buffer to the specified path with workaround */
            vg_lite_buffer_t temp_buf;
            vg_lite_matrix_t temp_mat;

            memset(&temp_buf, 0, sizeof(vg_lite_buffer_t));
            temp_buf.width = shape_bound[2] - shape_bound[0];
            temp_buf.height = shape_bound[3] - shape_bound[1];
            temp_buf.format = VG_LITE_RGBA8888;

            VGL_ERROR_CHECK(vg_lite_allocate(&temp_buf));
            VGL_ERROR_CHECK(vg_lite_clear(&temp_buf, NULL, 0x00000000));

            VGL_ERROR_CHECK(vg_lite_identity(&temp_mat));
            VGL_ERROR_CHECK(vg_lite_translate(-shape_bound[0], -shape_bound[1], &temp_mat));
            VGL_ERROR_CHECK(vg_lite_draw(&temp_buf, &vgl_path, VG_LITE_FILL_EVEN_ODD, &temp_mat, VG_LITE_BLEND_SRC_OVER, 0xFFFFFFFF));

            int start_x = 0;
            int start_y = 0;
            int offset_x = 0;
            int offset_y = 0;

            if (paint->pattern->units == NSVG_OBJECT_SPACE) {
                start_x = paint->pattern->x > 1 ? 0 : (shape_bound[2] - shape_bound[0]) * paint->pattern->x * global_scale;
                start_y = paint->pattern->y > 1 ? 0 : (shape_bound[3] - shape_bound[1]) * paint->pattern->y * global_scale;
                while (start_x > 0)
                    start_x -= temp_buf.width;
                while (start_y > 0)
                    start_y -= temp_buf.height;
            }
            else
            {
                start_x = paint->pattern->x;
                start_y = paint->pattern->y;
                while (start_x > shape_bound[0])
                    start_x -= temp_buf.width;
                while (start_y > shape_bound[1])
                    start_y -= temp_buf.height;
                offset_x = shape_bound[0];
                offset_y = shape_bound[1];
            }

            for (; start_x < offset_x + temp_buf.width; start_x += pattern_buf.width)
            {
                int tem_y = start_y;
                for (; start_y < offset_y + temp_buf.height; start_y += pattern_buf.height)
                {
                    VGL_ERROR_CHECK(vg_lite_identity(&temp_mat));
                    VGL_ERROR_CHECK(vg_lite_translate(-offset_x, -offset_y, &temp_mat));
                    VGL_ERROR_CHECK(vg_lite_translate(start_x, start_y, &temp_mat));
                    VGL_ERROR_CHECK(vg_lite_blit(&temp_buf, &pattern_buf, &temp_mat, VG_LITE_BLEND_SRC_IN, 0, 0));
                }
                start_y = tem_y;
            }

            memcpy(&temp_mat, &global_matrix, sizeof(global_matrix));
            VGL_ERROR_CHECK(vg_lite_translate(shape_bound[0], shape_bound[1], &temp_mat));
            VGL_ERROR_CHECK(vg_lite_blit(render_buffer_ptr, &temp_buf, &temp_mat, VG_LITE_BLEND_SRC_OVER, 0, 0));
            VGL_ERROR_CHECK(vg_lite_finish());

            VGL_ERROR_CHECK(vg_lite_free(&temp_buf));
        }

        VGL_ERROR_CHECK(vg_lite_free(&pattern_buf));
        return 0;

    VGLError:
        vg_lite_free(&pattern_buf);
        return 1;
    }

    return 0;
}

static vg_lite_buffer_t* renderClipPath(vg_lite_buffer_t* render_buffer_ptr, NSVGshape* shape, char* clip_flag_ptr)
{
    *clip_flag_ptr = 1;
    shape->blendMode = VG_MIXBLENDMODE_SRCIN;
    static vg_lite_buffer_t clip_path_buffer;
    vg_lite_path_t clip_path;
    vg_lite_matrix_t clip_path_matrix;

    memset(&clip_path_buffer, 0, sizeof(vg_lite_buffer_t));
    memset(&clip_path, 0, sizeof(vg_lite_path_t));

    clip_path_buffer.width = render_buffer_ptr->width;
    clip_path_buffer.height = render_buffer_ptr->height;
    clip_path_buffer.format = VG_LITE_RGBA8888;

    VGL_ERROR_CHECK(vg_lite_allocate(&clip_path_buffer));
    VGL_ERROR_CHECK(vg_lite_clear(&clip_path_buffer, NULL, 0x00000000));
    VGL_ERROR_CHECK(vg_lite_identity(&clip_path_matrix));
    VGL_ERROR_CHECK(vg_lite_init_path(&clip_path, VG_LITE_FP32, VG_LITE_MEDIUM, 0, &path_data_buf[0],
        shape->clipPath->path->bounds[0], shape->clipPath->path->bounds[1], shape->clipPath->path->bounds[2], shape->clipPath->path->bounds[3]));
    generateVGLitePath(shape->clipPath->path, &clip_path, VG_LITE_DRAW_FILL_PATH, shape->pathType);
    VGL_ERROR_CHECK(vg_lite_draw(&clip_path_buffer, &clip_path, VG_LITE_FILL_EVEN_ODD, &global_matrix, VG_LITE_BLEND_NONE, 0xFF000000));
    VGL_ERROR_CHECK(vg_lite_blit(&clip_path_buffer, render_buffer_ptr, &clip_path_matrix, VG_LITE_BLEND_SRC_IN, 0, 0));
    VGL_ERROR_CHECK(vg_lite_finish());
    VGL_ERROR_CHECK(vg_lite_clear_path(&clip_path));

    return &clip_path_buffer;

VGLError:
    vg_lite_free(&clip_path_buffer);
    return NULL;
}

int renderSVGImage(NSVGimage *image, vg_lite_buffer_t *render_buffer_ptr, vg_lite_matrix_t *extra_matrix, vg_lite_float_t scale_size, void *ft_face)
{
    NSVGshape *shape = NULL;
    vg_lite_path_type_t path_type;

    vg_lite_buffer_t* current_render_buffer_ptr;
    char clip_flag = 0;

    memset(&vgl_path, 0, sizeof(vg_lite_path_t));
    current_render_buffer_ptr = render_buffer_ptr;
    global_scale = scale_size;

    /* Clear the frame buffer to white color */
    //VGL_ERROR_CHECK(vg_lite_clear(fb, NULL, 0xFFFFFFFF));

    if (extra_matrix) {
        memcpy(&global_matrix, extra_matrix, sizeof(global_matrix));
    }
    else {
        VGL_ERROR_CHECK(vg_lite_identity(&global_matrix));
    }

    /* Loop through all shape structures in NSVGimage to render all primitves */
    for (shape = image->shapes; shape != NULL; shape = shape->next)
    {
        if (!(shape->flags & NSVG_FLAGS_VISIBLE))
            continue;

        if (shape->mediaFlag)
        {
            if ((shape->maxWidth > 0 && shape->maxWidth < global_matrix.m[0][0] * render_buffer_ptr->width) || (shape->maxHeight > 0 && shape->maxHeight < global_matrix.m[1][1] * render_buffer_ptr->height))
                continue;
            if (shape->minWidth > global_matrix.m[0][0] * render_buffer_ptr->width || shape->minHeight > global_matrix.m[1][1] * render_buffer_ptr->height)
                continue;
        }

#if USE_BOUNDARY_JUDGMENT
        if (shape->fill.type == NSVG_PAINT_TEXT)
        {
            float font_x = global_matrix.m[0][0] * shape->fill.text->x * global_scale + global_matrix.m[0][1] * shape->fill.text->y * global_scale + global_matrix.m[0][2];
            float font_y = global_matrix.m[1][0] * shape->fill.text->x * global_scale + global_matrix.m[1][1] * shape->fill.text->y * global_scale + global_matrix.m[1][2];

            if (font_x <= 0 || font_y <= 0 || font_x >= render_buffer_ptr->width || font_y >= render_buffer_ptr->height)
                continue;
        }
        else if (shape->fill.type == NSVG_PAINT_IMAGE)
        {
            float image_x = global_matrix.m[0][0] * shape->fill.image->x * global_scale + global_matrix.m[0][1] * shape->fill.image->y * global_scale + global_matrix.m[0][2];
            float image_y = global_matrix.m[1][0] * shape->fill.image->x * global_scale + global_matrix.m[1][1] * shape->fill.image->y * global_scale + global_matrix.m[1][2];

            if (image_x <= 0 || image_y <= 0 || image_x >= render_buffer_ptr->width || image_y >= render_buffer_ptr->height)
                continue;
        }
        else if (shape->fill.type != NSVG_PAINT_NONE || shape->stroke.type != NSVG_PAINT_NONE)
        {
                float path_minx = global_matrix.m[0][0] * shape->paths->bounds[0] + global_matrix.m[0][1] * shape->paths->bounds[1] + global_matrix.m[0][2];
                float path_miny = global_matrix.m[1][0] * shape->paths->bounds[0] + global_matrix.m[1][1] * shape->paths->bounds[1] + global_matrix.m[1][2];
                float path_maxx = global_matrix.m[0][0] * shape->paths->bounds[2] + global_matrix.m[0][1] * shape->paths->bounds[3] + global_matrix.m[0][2];
                float path_maxy = global_matrix.m[1][0] * shape->paths->bounds[2] + global_matrix.m[1][1] * shape->paths->bounds[2] + global_matrix.m[1][2];

                if (path_maxx <= 0 || path_maxy <= 0 || path_minx >= render_buffer_ptr->width || path_miny >= render_buffer_ptr->height)
                    continue;
        }
#endif

        if (shape->clipPath != NULL && !clip_flag) {
            current_render_buffer_ptr = renderClipPath(current_render_buffer_ptr, shape, &clip_flag);
        }

        shape_bound[0] = shape->bounds[0] * global_scale;
        shape_bound[1] = shape->bounds[1] * global_scale;
        shape_bound[2] = shape->bounds[2] * global_scale;
        shape_bound[3] = shape->bounds[3] * global_scale;

        VGL_ERROR_CHECK(vg_lite_init_path(&vgl_path, VG_LITE_FP32, VG_LITE_MEDIUM, 0, &path_data_buf[0],
            shape_bound[0], shape_bound[1], shape_bound[2], shape_bound[3]));

        path_type = 0;
        if (shape->fill.type != NSVG_PAINT_NONE) {
            path_type |= VG_LITE_DRAW_FILL_PATH;
        }
        if (shape->stroke.type != NSVG_PAINT_NONE) {
            path_type |= VG_LITE_DRAW_STROKE_PATH;
        }

        /* Set VGLite fill rule */
        fill_rule = VG_LITE_FILL_NON_ZERO - shape->fillRule;

        /* Generate VGLite cubic bezier path data from current shape */
        VGL_ERROR_CHECK(generateVGLitePath(shape->paths, &vgl_path, path_type, shape->pathType));

        /* Render VGLite fill path with linear/radial gradient support */
        if (shape->fill.type != NSVG_PAINT_NONE)
        {
            int is_fill = 1;
            int is_stroke = 0;
            /* Render the fill path only */
            vgl_path.path_type = VG_LITE_DRAW_FILL_PATH;

            vg_lite_color_t fill_color = shape->fill.color;
            setOpacity(&fill_color, shape->opacity);

            if (shape->fill.type == NSVG_PAINT_TEXT) {
#ifdef CONFIG_FREETYPE_FONT
                if (ft_face) {
                    VGL_ERROR_CHECK(renderText(current_render_buffer_ptr, shape, fill_color, ft_face));
                } else {
                    NANOSVG_PRINT("FT face is null\n");
                }
#else
                NANOSVG_PRINT("Text is not supported!\n");
#endif
            }
            else if (shape->fill.type == NSVG_PAINT_IMAGE) {
                VGL_ERROR_CHECK(renderImage(current_render_buffer_ptr, shape->fill.image, 0));
            }
            else {
                VGL_ERROR_CHECK(renderPatternPath(current_render_buffer_ptr, &shape->fill, shape->fillPattern));

                VGL_ERROR_CHECK(renderGradientPath(current_render_buffer_ptr, &shape->fill, shape->fillGradient, fill_color, shape->opacity,
                                shape->fill_opacity, shape->stroke_opacity, is_fill, is_stroke, shape->fillPattern,
                                shape->blendMode));
            }
        }

        /* Render VGLite stroke path with linear/radial gradient support */
        if (shape->stroke.type != NSVG_PAINT_NONE)
        {
            int is_fill = 0;
            int is_stroke = 1;
            vg_lite_cap_style_t linecap = VG_LITE_CAP_BUTT + shape->strokeLineCap;
            vg_lite_join_style_t joinstyle = VG_LITE_JOIN_MITER + shape->strokeLineJoin;
            vg_lite_float_t strokewidth = shape->strokeWidth * global_scale;
            vg_lite_color_t stroke_color = shape->stroke.color;

            /* Render the stroke path only */
            vgl_path.path_type = VG_LITE_DRAW_STROKE_PATH;
            if (shape->opacity < 1 || shape->fill_opacity == 0)
            {
                vg_lite_color_t color = stroke_color;
                uint8_t a = (uint8_t)(((color & 0xFF000000) >> 24) * shape->opacity);
                stroke_color = (color&0x00ffffff) | ((a << 24) & 0xff000000);
            }
            else {
                setOpacity(&stroke_color, shape->opacity);
            }

            /* Setup stroke parameters properly */
            VGL_ERROR_CHECK(vg_lite_set_stroke(&vgl_path, linecap, joinstyle, strokewidth, shape->miterLimit,
                shape->strokeDashArray, shape->strokeDashCount, shape->strokeDashOffset, 0xFFFFFFFF));
            VGL_ERROR_CHECK(vg_lite_update_stroke(&vgl_path));

            /* Draw stroke path with stroke color */
            vgl_path.stroke_color = stroke_color;
            VGL_ERROR_CHECK(renderGradientPath(current_render_buffer_ptr, &shape->stroke, shape->strokeGradient, stroke_color, shape->opacity,
                            shape->fill_opacity, shape->stroke_opacity, is_fill, is_stroke, shape->fillPattern,
                            shape->blendMode));
        }

        /* Clear vglpath. */
        VGL_ERROR_CHECK(vg_lite_clear_path(&vgl_path));

        if (shape->next != NULL && shape->next->clipPath != NULL)
            shape->next->blendMode = VG_MIXBLENDMODE_SRCIN;

        if ((shape->next == NULL && clip_flag ==1) || (shape->clipPath != NULL && shape->next!= NULL && (shape->next->clipPath == NULL || shape->clipPath != shape->next->clipPath)))
        {
            clip_flag = 0;

            vg_lite_matrix_t temp_mat;
            vg_lite_identity(&temp_mat);

            vg_lite_blit(render_buffer_ptr, current_render_buffer_ptr, &temp_mat, VG_LITE_BLEND_SRC_OVER, 0, 0);
            vg_lite_finish();

            vg_lite_free(current_render_buffer_ptr);
            current_render_buffer_ptr = render_buffer_ptr;
        }

#if VG_STABLE_MODE
        if (++commit_count >= MAX_COMMITTED_PATHS) {
            VGL_ERROR_CHECK(vg_lite_flush());
            commit_count = 0;
        }
#endif
    }

#if VG_STABLE_MODE
    commit_count = 0;
#endif

    VGL_ERROR_CHECK(vg_lite_frame_delimiter(VG_LITE_FRAME_END_FLAG));

    return 0;

VGLError:
    NANOSVG_PRINT("render SVG error!\n");
    vg_lite_clear_path(&vgl_path);

    return 1;
}

NSVGimage* parseSVGImage(char *input, int *pwidth, int *pheight)
{
    NSVGimage *image = nsvgParse(input, "px", 96.0f);

    if (image) {
        *pwidth = (int)(image->width * global_scale);
        *pheight = (int)(image->height * global_scale);
    }

    return image;
}

NSVGimage* parseSVGImageFromFile(const char *filename, int *pwidth, int *pheight)
{
#if 0
    NSVGimage *image = nsvgParseFromFile(filename, "px", 96.0f);

    if (image) {
        *pwidth = (int)(image->width * global_scale);
        *pheight = (int)(image->height * global_scale);
    }

    return image;
#else
    return NULL;
#endif
}

void deleteSVGImage(NSVGimage *image)
{
    nsvgDelete(image);
}

void setSVGImageDecoder(const struct NSVGImageDecoder *decoder)
{
    if (decoder && decoder->decode)
        g_imagedec = decoder;
}
