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

#ifdef CONFIG_VG_LITE

#include <stdio.h>
#include <string.h>
#include <float.h>

#if defined(CONFIG_LVGL)
#  include <lvgl/lvgl.h>
#  ifdef CONFIG_UI_MANAGER
#    include <ui_mem.h>
#    define NANOSVG_malloc(size)        ui_mem_alloc(MEM_RES, size, __func__)
#    define NANOSVG_free(ptr)           ui_mem_free(MEM_RES, ptr)
#    define NANOSVG_realloc(ptr, size)  ui_mem_realloc(MEM_RES, ptr, size, __func__)
#  else
#    define NANOSVG_malloc(size)        lv_mem_alloc(size)
#    define NANOSVG_free(ptr)           lv_mem_free(ptr)
#    define NANOSVG_realloc(ptr, size)  lv_mem_realloc(ptr, size)
#  endif
#  define PRINT_ERR(str, ...)           LV_LOG_ERROR(str, ##__VA_ARGS__);
#else
#  define NANOSVG_malloc(size)        malloc(size)
#  define NANOSVG_free(ptr)           free(ptr)
#  define NANOSVG_realloc(ptr, size)  realloc(ptr, size)
#  define PRINT_ERR(str, ...)         printf(str, ##__VA_ARGS__);
#endif /* CONFIG_LVGL */

#define NANOSVG_ALL_COLOR_KEYWORDS    // Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION        // Expands NanoSVG implementation

#define RENDER_TEXT_USE_FREETYPE              0
#define DUMP_SVG                              0
#define DUMP_FONT_PATH                        0

#include "nanosvg.h"
#include "vg_lite.h"

#if RENDER_TEXT_USE_FREETYPE
#include "ftoutln.h"
#include "ftbbox.h"
#include "ft2build.h"
#endif

#if DUMP_SVG
#define DUMP_VGL
#include "dump_vgl.h"
#endif

#define VGL_ERROR_CHECK(func) \
    if ((error = func) != VG_LITE_SUCCESS) \
    goto VGLError

#define swap(a,b) {int temp; temp=a; a=b; b=temp;}

#define  PATH_DATA_BUFFER_SIZE  8192
#define  PATTERN_BUFFER_SIZE    1024

vg_lite_float_t path_data_buf[PATH_DATA_BUFFER_SIZE];
vg_lite_float_t pattern_data_buf[PATTERN_BUFFER_SIZE];
#if RENDER_TEXT_USE_FREETYPE
vg_lite_float_t font_path_data_buf[PATH_DATA_BUFFER_SIZE];
#endif

vg_lite_color_ramp_t grad_ramps[VLC_MAX_COLOR_RAMP_STOPS];

uint32_t grad_colors[VLC_MAX_GRADIENT_STOPS];
uint32_t grad_stops[VLC_MAX_GRADIENT_STOPS];

vg_lite_gradient_spreadmode_t vglspread[] = { VG_LITE_GRADIENT_SPREAD_PAD, VG_LITE_GRADIENT_SPREAD_REFLECT, VG_LITE_GRADIENT_SPREAD_REPEAT };

vg_lite_error_t error = VG_LITE_SUCCESS;
vg_lite_buffer_t *fb = NULL;
vg_lite_matrix_t matrix;
vg_lite_path_t vglpath;
#if RENDER_TEXT_USE_FREETYPE
vg_lite_path_t fontpath;
#endif
vg_lite_fill_t fill_rule;
vg_lite_float_t shape_bound[4]; // [minx,miny,maxx,maxy]
vg_lite_float_t scale = 1.0f;
//vg_lite_int32_t width, height;
//vg_lite_char resultname[64];

#if RENDER_TEXT_USE_FREETYPE
#define TEXT_SIZE 2048
static char textdata[TEXT_SIZE] = { 0 };

int move_to(const FT_Vector* to, void* user)
{
    char movedata[32];
    sprintf(movedata, "M %d %d", to->x, -to->y);
    strcat(textdata, movedata);
    return 0;
}
int line_to(const FT_Vector* to, void* user)
{
    char linedata[32];
    sprintf(linedata, "L %d %d", to->x, -to->y);
    strcat(textdata, linedata);
    return 0;
}
int conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
    char qdata[64];
    sprintf(qdata, "Q %d %d %d %d", control->x, -control->y, to->x, -to->y);
    strcat(textdata, qdata);
    return 0;
}
int cubic_to(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
    char cdata[96];
    sprintf(cdata, "C %d %d %d %d %d %d", control1->x, -control1->y, control2->x, -control2->y, to->x, -to->y);
    strcat(textdata, cdata);
    return 0;
}

void parseCharacterToPath(NSVGparser* p, NSVGshape* shape, const char* pathdata)
{
    const char* s = pathdata;
    char cmd = '\0';
    float args[10];
    int nargs;
    int rargs = 0;
    char initPoint;
    float cpx, cpy, cpx2, cpy2;
    char closedFlag;
    char item[64];

    if (s) {
        nsvg__resetPath(p);
        cpx = 0; cpy = 0;
        cpx2 = 0; cpy2 = 0;
        initPoint = 0;
        closedFlag = 0;
        nargs = 0;

        while (*s) {
            item[0] = '\0';
            if ((cmd == 'A' || cmd == 'a') && (nargs == 3 || nargs == 4))
                s = nsvg__getNextPathItemWhenArcFlag(s, item);
            if (!*item)
                s = nsvg__getNextPathItem(s, item);
            if (!*item) break;
            if (cmd != '\0' && nsvg__isCoordinate(item)) {
                if (nargs < 10)
                    args[nargs++] = (float)nsvg__atof(item);
                if (nargs >= rargs) {
                    switch (cmd) {
                    case 'm':
                    case 'M':
                        nsvg__pathMoveTo(p, &cpx, &cpy, args, cmd == 'm' ? 1 : 0);
                        // Moveto can be followed by multiple coordinate pairs,
                        // which should be treated as linetos.
                        cmd = (cmd == 'm') ? 'l' : 'L';
                        rargs = nsvg__getArgsPerElement(cmd);
                        cpx2 = cpx; cpy2 = cpy;
                        initPoint = 1;
                        break;
                    case 'l':
                    case 'L':
                        nsvg__pathLineTo(p, &cpx, &cpy, args, cmd == 'l' ? 1 : 0);
                        cpx2 = cpx; cpy2 = cpy;
                        break;
                    case 'H':
                    case 'h':
                        nsvg__pathHLineTo(p, &cpx, &cpy, args, cmd == 'h' ? 1 : 0);
                        cpx2 = cpx; cpy2 = cpy;
                        break;
                    case 'V':
                    case 'v':
                        nsvg__pathVLineTo(p, &cpx, &cpy, args, cmd == 'v' ? 1 : 0);
                        cpx2 = cpx; cpy2 = cpy;
                        break;
                    case 'C':
                    case 'c':
                        nsvg__pathCubicBezTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'c' ? 1 : 0);
                        break;
                    case 'S':
                    case 's':
                        nsvg__pathCubicBezShortTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 's' ? 1 : 0);
                        break;
                    case 'Q':
                    case 'q':
                        nsvg__pathQuadBezTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 'q' ? 1 : 0);
                        break;
                    case 'T':
                    case 't':
                        nsvg__pathQuadBezShortTo(p, &cpx, &cpy, &cpx2, &cpy2, args, cmd == 't' ? 1 : 0);
                        break;
                    case 'A':
                    case 'a':
                        nsvg__pathArcTo(p, &cpx, &cpy, args, cmd == 'a' ? 1 : 0);
                        cpx2 = cpx; cpy2 = cpy;
                        break;
                    default:
                        if (nargs >= 2) {
                            cpx = args[nargs - 2];
                            cpy = args[nargs - 1];
                            cpx2 = cpx; cpy2 = cpy;
                        }
                        break;
                    }
                    nargs = 0;
                }
            }
            else {
                cmd = item[0];
                if (cmd == 'M' || cmd == 'm') {
                    // Commit path.
                    if (p->npts > 0)
                        nsvg__addPath(p, closedFlag);
                    // Start new subpath.
                    nsvg__resetPath(p);
                    closedFlag = 0;
                    nargs = 0;
                }
                else if (initPoint == 0) {
                    // Do not allow other commands until initial point has been set (moveTo called once).
                    cmd = '\0';
                }
                if (cmd == 'Z' || cmd == 'z') {
                    closedFlag = 1;
                    // Commit path.
                    if (p->npts > 0) {
                        // Move current point to first point
                        cpx = p->pts[0];
                        cpy = p->pts[1];
                        cpx2 = cpx; cpy2 = cpy;
                        nsvg__addPath(p, closedFlag);
                    }
                    // Start new subpath.
                    nsvg__resetPath(p);
                    nsvg__moveTo(p, cpx, cpy);
                    closedFlag = 0;
                    nargs = 0;
                }
                rargs = nsvg__getArgsPerElement(cmd);
                if (rargs == -1) {
                    // Command not recognized
                    cmd = '\0';
                    rargs = 0;
                }
            }
        }
        // Commit path.
        if (p->npts)
            nsvg__addPath(p, closedFlag);
    }

    shape->paths = p->plist;
}

vg_lite_error_t drawCharacter(NSVGshape* shape, float size, float x, float y, float scale, int box_xMin, int box_yMin, int box_xMax, int box_yMax)
{
    vg_lite_init_path(&fontpath, VG_LITE_FP32, VG_LITE_HIGH, 0, &font_path_data_buf[0], box_xMin, box_yMin, box_xMax, box_yMax);

    float* d = &((float*)fontpath.path)[0];
    float* bufend = d + PATH_DATA_BUFFER_SIZE;
    vg_lite_matrix_t font_matrix;

    for (NSVGpath* path = shape->paths; path != NULL; path = path->next)
    {
        float* s = &path->pts[0];
        /* Exit if remaining path_data_buf[] is not sufficient to hold the path data */
        if ((bufend - d) < (7 * path->npts + 10) / 3)
        {
            goto VGLError;
        }

        /* Create VGLite cubic bezier path data from path->pts[] */
        *((char*)d) = VLC_OP_MOVE; d++;
        *d++ = (*s++) / scale;
        *d++ = (*s++) / scale;
        for (int i = 0; i < path->npts - 1; i += 3)
        {
            *((char*)d) = VLC_OP_CUBIC; d++;
            *d++ = (*s++) / scale;
            *d++ = (*s++) / scale;
            *d++ = (*s++) / scale;
            *d++ = (*s++) / scale;
            *d++ = (*s++) / scale;
            *d++ = (*s++) / scale;
        }
    }
    *((char*)d) = VLC_OP_END; d++;

    /* Compute the accurate VGLite path data length */
    fontpath.path_length = ((char*)d - (char*)fontpath.path);

#if DUMP_FONT_PATH
    FILE* fd;
    char font_data[10] = "10";
    sprintf(font_data, "font.raw");
    fd = fopen(font_data, "wb+");
    fwrite(&fontpath, sizeof(fontpath), 1, fd);
    fwrite(fontpath.path, 1, fontpath.path_length, fd);
    fclose(fd);
#endif

    vg_lite_identity(&font_matrix);
    vg_lite_translate(x, y, &font_matrix);
    VGL_ERROR_CHECK(vg_lite_draw(fb, &fontpath, VG_LITE_FILL_EVEN_ODD, &font_matrix, VG_LITE_BLEND_SRC_OVER, 0xff000000));
    vg_lite_finish();
    return 0;
VGLError:
    return 1;
}

char* font_path = "./freetype/font/kaiu.ttf";
//char* font_path = "./freetype/font/VIPRoman-Regular.ttf";

vg_lite_error_t renderText(vg_lite_buffer_t * fb, NSVGshape * shape)
{
    FT_GlyphSlot slot;
    FT_Outline outline;
    FT_Outline_Funcs callbacks;
    FT_Error error;
    FT_Library library;
    FT_Face face;
    FT_BBox box = { 0 };

    float scale = 64, defaul_size = 16, dx = 0, dy = 0;
    float font_size, x, y, trans_x, space_width;
    char* text_content;
    int text_length = 0, count = 0;
    unsigned int index;

    x = shape->fill.text->x;
    y = shape->fill.text->y;
    font_size = shape->fill.text->fontSize;
    trans_x = x;

    text_content = shape->fill.text->content;
    text_length = strlen(text_content);
    scale = font_size != 0 ? scale / font_size : scale / defaul_size;

    error = FT_Init_FreeType(&library);
    if (error)
    {
        printf("error in init freetype.");
        goto VGLError;
    }

    error = FT_New_Face(library, font_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        printf("error in file format.");
        goto VGLError;
    }
    else if (error)
    {
        printf("error in creat face.");
        goto VGLError;
    }

    error = FT_Set_Pixel_Sizes(face, 1, 0);
    if (error)
    {
        printf("error in set pixel size.");
        goto VGLError;
    }

    {  /* To get space width. */
        index = FT_Get_Char_Index(face, 'a');
        error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
        slot = face->glyph;
        outline = slot->outline;
        callbacks.move_to = move_to;
        callbacks.line_to = line_to;
        callbacks.conic_to = conic_to;
        callbacks.cubic_to = cubic_to;
        callbacks.shift = 0;
        callbacks.delta = 0;

        error = FT_Outline_Decompose(&outline, &callbacks, 0);
        error = FT_Outline_Get_BBox(&outline, &box);
        space_width = (box.xMax - box.xMin) / scale;
        memset(textdata, 0, TEXT_SIZE);
    }
    
    NSVGparser* p;
    p = nsvg__createParser();
    unsigned int unicode;
    unsigned char character;
    int i = 0;

    while(text_content[i] != '\0')
    {
        character = text_content[i];

        if (character == ' ')
        {
            trans_x += space_width;
            trans_x -= dx * 0.5 - dx * (0.333);
            i++;
            continue;
        }

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
        else if ((character & 0xF8) == 0xF0) {
            /* Four bytes UTF-8 encoding.*/
            unicode = ((character & 0x07) << 18) | ((text_content[i+1] & 0x3F) << 12) | ((text_content[i+2] & 0x3F) << 6) | (text_content[i+3] & 0x3F);
            i += 4;
        }
        else
        {   /* ASCII encoding.*/
            unicode = character;
            i++;
        }

        index = FT_Get_Char_Index(face, unicode);
        error = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
        if (error)
        {
            printf("error in load glyph.");
            goto VGLError;
        }

        slot = face->glyph;
        outline = slot->outline;
        callbacks.move_to = move_to;
        callbacks.line_to = line_to;
        callbacks.conic_to = conic_to;
        callbacks.cubic_to = cubic_to;
        callbacks.shift = 0;
        callbacks.delta = 0;

        error = FT_Outline_Decompose(&outline, &callbacks, 0);
        error = FT_Outline_Get_BBox(&outline, &box);
        dx = (box.xMax - box.xMin) / scale;
        dy = (box.yMax - box.yMin) / scale;

        parseCharacterToPath(p, shape, textdata);
        memset(textdata, 0, TEXT_SIZE);

        if (count != 0)
        {
            if (character >= '!' && character <= '/')
                trans_x = trans_x + 2;
            else
                trans_x = trans_x + dx * 0.333;
        }
        drawCharacter(shape, font_size, trans_x, y, scale, box.xMin, box.yMin, box.xMax, box.yMax);

        trans_x = trans_x + dx * 0.5 + dx * 0.333;
        count++;
        nsvg__deletePaths(p->plist);
        p->plist = NULL; // For safe free.
    }

    nsvg__deleteParser(p);
    shape->paths = NULL; // For safe free.

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;

VGLError:
    return 1;
}
#endif /* RENDER_TEXT_USE_FREETYPE */

void setOpacity(vg_lite_color_t * pcolor, float opacity)
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

vg_lite_error_t generateVGLitePath(NSVGshape* shape, vg_lite_path_t* vglpath, vg_lite_path_type_t path_type)
{
    float* d = &((float*)vglpath->path)[0];
    float* bufend = d + PATH_DATA_BUFFER_SIZE;

    for (NSVGpath* path = shape->paths; path != NULL; path = path->next)
    {
        float* s = &path->pts[0];

        /* Exit if remaining path_data_buf[] is not sufficient to hold the path data */
        if ((bufend - d) < (7 * path->npts + 10) / 3)
        {
            printf("Error: Need to increase PATH_DATA_BUFFER_SIZE for path_data_buf[].\n");
            goto VGLError;
        }

        /* Create VGLite cubic bezier path data from path->pts[] */
        *((char*)d) = VLC_OP_MOVE; d++;
        *d++ = (*s++) * scale;
        *d++ = (*s++) * scale;
        for (int i = 0; i < path->npts - 1; i += 3)
        {
            *((char*)d) = VLC_OP_CUBIC; d++;
            *d++ = (*s++) * scale;
            *d++ = (*s++) * scale;
            *d++ = (*s++) * scale;
            *d++ = (*s++) * scale;
            *d++ = (*s++) * scale;
            *d++ = (*s++) * scale;
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
    vglpath->path_length = ((char*)d - (char*)vglpath->path);

    return 0;

VGLError:
    return 1;
}

int renderGradientPath(NSVGpaint *paint, char *gradid, unsigned int color, float opacity)
{
    if (gradid[0] != '\0')
    {
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
                    grad_param.X0 = X0 * scale;
                    grad_param.X1 = X1 * scale;
                    grad_param.Y0 = Y0 * scale;
                    grad_param.Y1 = Y1 * scale;
                }

                VGL_ERROR_CHECK(vg_lite_set_linear_grad(&grad, paint->gradient->nstops, grad_ramps, grad_param, vglspread[paint->gradient->spread], 0));
                VGL_ERROR_CHECK(vg_lite_update_linear_grad(&grad));

                grad_matrix = vg_lite_get_linear_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));

                VGL_ERROR_CHECK(vg_lite_draw_linear_grad(fb, &vglpath, VG_LITE_FILL_EVEN_ODD, &matrix, &grad, color, VG_LITE_BLEND_SRC_OVER, VG_LITE_FILTER_POINT));

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
                    x1 = paint->gradient->param[0] * scale;
                    x2 = paint->gradient->param[2] * scale;
                }

                if (x1 > x2)
                    swap(x1, x2);

                for (int i = 0; i < paint->gradient->nstops; i++) {
                    setOpacity(&(paint->gradient->stops[i].color), opacity);

                    alpha = (paint->gradient->stops[i].color & 0xFF000000) >> 24;
                    blue = (paint->gradient->stops[i].color & 0x00FF0000) >> 16;
                    green = (paint->gradient->stops[i].color & 0x0000FF00) >> 8;
                    red = (paint->gradient->stops[i].color & 0x000000FF) >> 0;
                    grad_colors[i] = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);

                    stops = x1 + paint->gradient->stops[i].offset * (x2 - x1);
                    grad_stops[i] = stops > 0 ? stops : 0;
                }

                VGL_ERROR_CHECK(vg_lite_init_grad(&grad));
                VGL_ERROR_CHECK(vg_lite_set_grad(&grad, paint->gradient->nstops, grad_colors, grad_stops));
                VGL_ERROR_CHECK(vg_lite_update_grad(&grad));

                grad_matrix = vg_lite_get_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));
                VGL_ERROR_CHECK(vg_lite_rotate(angle, grad_matrix));

                VGL_ERROR_CHECK(vg_lite_draw_grad(fb, &vglpath, VG_LITE_FILL_EVEN_ODD, &matrix, &grad, VG_LITE_BLEND_SRC_OVER));

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
                    setOpacity(&(paint->gradient->stops[i].color), opacity);
                    grad_ramps[i].stop = paint->gradient->stops[i].offset;
                    grad_ramps[i].alpha = ((paint->gradient->stops[i].color & 0xFF000000) >> 24) / 255.0f;
                    grad_ramps[i].blue = ((paint->gradient->stops[i].color & 0x00FF0000) >> 16) / 255.0f;
                    grad_ramps[i].green = ((paint->gradient->stops[i].color & 0x0000FF00) >> 8) / 255.0f;;
                    grad_ramps[i].red = (paint->gradient->stops[i].color & 0x000000FF) / 255.0f;;
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
                    grad_param.cx = cx * scale;
                    grad_param.cy = cy * scale;
                    grad_param.r = r * scale;
                    grad_param.fx = (fx == 0.0f) ? grad_param.cx : (fx * scale);
                    grad_param.fy = (fy == 0.0f) ? grad_param.cy : (fy * scale);
                }

                VGL_ERROR_CHECK(vg_lite_set_radial_grad(&grad, paint->gradient->nstops, grad_ramps, grad_param, vglspread[paint->gradient->spread], 0));
                VGL_ERROR_CHECK(vg_lite_update_radial_grad(&grad));

                grad_matrix = vg_lite_get_radial_grad_matrix(&grad);
                VGL_ERROR_CHECK(vg_lite_identity(grad_matrix));

                VGL_ERROR_CHECK(vg_lite_draw_radial_grad(fb, &vglpath, VG_LITE_FILL_EVEN_ODD, &matrix, &grad, color, VG_LITE_BLEND_SRC_OVER, VG_LITE_FILTER_POINT));

                VGL_ERROR_CHECK(vg_lite_finish());
                VGL_ERROR_CHECK(vg_lite_clear_radial_grad(&grad));
            }
            else
            {
                PRINT_ERR("Radial gradient is not supported!");
                return -1;
            }
        }
    }
    else
    {
        /* Render the shape with VGLite cubic bezier path data */
        VGL_ERROR_CHECK(vg_lite_draw(fb, &vglpath, fill_rule, &matrix, VG_LITE_BLEND_SRC_OVER, color));

        VGL_ERROR_CHECK(vg_lite_finish());
    }

    return 0;

VGLError:
    return 1;
}

int renderPatternPath(NSVGpaint* paint, char* patternid)
{
    if (patternid[0] != '\0')
    {
        if (!vg_lite_query_feature(gcFEATURE_BIT_VG_IM_REPEAT_REFLECT))
        {
            printf("Repeat and reflect mode of draw pattern is not supported!");
            return -1;
        }
        vg_lite_path_t patternpath;
        vg_lite_float_t pattern_shape_bound[4];
        NSVGshape* pattern_shape = paint->pattern->shape;
        vg_lite_path_type_t path_type;
        vg_lite_matrix_t matrix;
        vg_lite_matrix_t vglmatrix;
        vg_lite_matrix_t draw_pattern_matrix;
        vg_lite_buffer_t patternbuf;

        memset(&patternbuf, 0, sizeof(vg_lite_buffer_t));
        memset(&patternpath, 0, sizeof(vg_lite_path_t));
        VGL_ERROR_CHECK(vg_lite_identity(&matrix));
        VGL_ERROR_CHECK(vg_lite_identity(&draw_pattern_matrix));
        VGL_ERROR_CHECK(vg_lite_identity(&vglmatrix));

        patternbuf.width = paint->pattern->width * scale;
        patternbuf.height = paint->pattern->height * scale;
        patternbuf.format = VG_LITE_RGBA8888;
        VGL_ERROR_CHECK(vg_lite_allocate(&patternbuf));
        VGL_ERROR_CHECK(vg_lite_clear(&patternbuf, NULL, 0xFFFFFFFF));

        pattern_shape_bound[0] = pattern_shape->bounds[0] * scale;
        pattern_shape_bound[1] = pattern_shape->bounds[1] * scale;
        pattern_shape_bound[2] = pattern_shape->bounds[2] * scale;
        pattern_shape_bound[3] = pattern_shape->bounds[3] * scale;

        VGL_ERROR_CHECK(vg_lite_init_path(&patternpath, VG_LITE_FP32, VG_LITE_HIGH, 0, &pattern_data_buf[0],
            pattern_shape_bound[0], pattern_shape_bound[1], pattern_shape_bound[2], pattern_shape_bound[3]));

        path_type = 0;
        if (pattern_shape->fill.type != NSVG_PAINT_NONE) {
            path_type |= VG_LITE_DRAW_FILL_PATH;
        }
        if (pattern_shape->stroke.type != NSVG_PAINT_NONE) {
            path_type |= VG_LITE_DRAW_STROKE_PATH;
        }

        VGL_ERROR_CHECK(generateVGLitePath(pattern_shape, &patternpath, path_type));

        VGL_ERROR_CHECK(vg_lite_translate(paint->pattern->x, paint->pattern->y, &matrix));
        /* Build pattern image */
        VGL_ERROR_CHECK(vg_lite_draw(&patternbuf, &patternpath, VG_LITE_FILL_EVEN_ODD, &draw_pattern_matrix, VG_LITE_BLEND_SRC_OVER, pattern_shape->fill.color));
        /* Fill pattern image to destination buffer */
        VGL_ERROR_CHECK(vg_lite_draw_pattern(fb, &vglpath, VG_LITE_FILL_EVEN_ODD, &vglmatrix, &patternbuf, &matrix, VG_LITE_BLEND_NONE, VG_LITE_PATTERN_REPEAT, 0, 0, VG_LITE_FILTER_POINT));
        VGL_ERROR_CHECK(vg_lite_finish());
        VGL_ERROR_CHECK(vg_lite_free(&patternbuf));
        return 0;

    VGLError:
        vg_lite_free(&patternbuf);
        return 1;
    }

    return 0;
}

int renderSVGImage(vg_lite_buffer_t *target, NSVGimage *image, vg_lite_matrix_t *matrix_p)
{
    NSVGshape *shape = NULL;
    //vg_lite_buffer_t buffer;
    vg_lite_path_type_t path_type;
    //int w, h;

    //w = (int)(image->width * scale);
    //h = (int)(image->height * scale);

    //memset(&buffer, 0, sizeof(vg_lite_buffer_t));
    memset(&vglpath, 0, sizeof(vg_lite_path_t));

    /* Initialize vg_lite engine. */
    //VGL_ERROR_CHECK(vg_lite_init(w, h));

    /* Allocate VGLite frame buffer. */
    //buffer.width  = w;
    //buffer.height = h;
    //buffer.format = VG_LITE_RGBA8888;
    //VGL_ERROR_CHECK(vg_lite_allocate(&buffer));
    //fb = &buffer;
    fb = target;

    /* Clear the frame buffer to white color */
    //VGL_ERROR_CHECK(vg_lite_clear(fb, NULL, 0xFFFFFFFF));
    //VGL_ERROR_CHECK(vg_lite_identity(&matrix));
    memcpy(&matrix, matrix_p, sizeof(matrix));

    /* Loop through all shape structures in NSVGimage to render all primitves */
    for (shape = image->shapes; shape != NULL; shape = shape->next)
    {
        /* Print SVG primitive ID */
        //printf("%s\n", shape->id);

        if (!(shape->flags & NSVG_FLAGS_VISIBLE))
            continue;

        shape_bound[0] = shape->bounds[0] * scale;
        shape_bound[1] = shape->bounds[1] * scale;
        shape_bound[2] = shape->bounds[2] * scale;
        shape_bound[3] = shape->bounds[3] * scale;

        VGL_ERROR_CHECK(vg_lite_init_path(&vglpath, VG_LITE_FP32, VG_LITE_HIGH, 0, &path_data_buf[0],
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
        VGL_ERROR_CHECK(generateVGLitePath(shape, &vglpath, path_type));

        /* Render VGLite fill path with linear/radial gradient support */
        if (shape->fill.type != NSVG_PAINT_NONE)
        {
            /* Render the fill path only */
            vglpath.path_type = VG_LITE_DRAW_FILL_PATH;

            vg_lite_color_t fill_color = shape->fill.color;
            setOpacity(&fill_color, shape->opacity);

            if (shape->fill.type == NSVG_PAINT_TEXT) {
#if RENDER_TEXT_USE_FREETYPE
                VGL_ERROR_CHECK(renderText(fb, shape));
#else
				PRINT_ERR("Text is not supported!");
#endif
            }
            else {
                VGL_ERROR_CHECK(renderPatternPath(&shape->fill, shape->fillPattern));

                VGL_ERROR_CHECK(renderGradientPath(&shape->fill, shape->fillGradient, fill_color, shape->opacity));
            }
        }

        /* Render VGLite stroke path with linear/radial gradient support */
        if (shape->stroke.type != NSVG_PAINT_NONE)
        {
            vg_lite_cap_style_t linecap = VG_LITE_CAP_BUTT + shape->strokeLineCap;
            vg_lite_join_style_t joinstyle = VG_LITE_JOIN_MITER + shape->strokeLineJoin;
            vg_lite_float_t strokewidth = shape->strokeWidth * scale;
            vg_lite_color_t stroke_color = shape->stroke.color;

            /* Render the stroke path only */
            vglpath.path_type = VG_LITE_DRAW_STROKE_PATH;
            /* Disable anti-aliasing line rendering for thin stroke line */
            vglpath.quality = (strokewidth <= 1.0) ? VG_LITE_LOW : VG_LITE_HIGH;
            setOpacity(&stroke_color, shape->opacity);

            /* Setup stroke parameters properly */
            VGL_ERROR_CHECK(vg_lite_set_stroke(&vglpath, linecap, joinstyle, strokewidth, shape->miterLimit,
                shape->strokeDashArray, shape->strokeDashCount, shape->strokeDashOffset, 0xFFFFFFFF));
            VGL_ERROR_CHECK(vg_lite_update_stroke(&vglpath));

            /* Clear stroke path with color 0xFFFFFFFF if stroke color is transparent */
            if ((stroke_color & 0xFF000000) != 0xFF000000) {
                vg_lite_draw(fb, &vglpath, fill_rule, &matrix, VG_LITE_BLEND_NONE, 0);
            }

            /* Draw stroke path with stroke color */
            vglpath.stroke_color = stroke_color;
            VGL_ERROR_CHECK(renderGradientPath(&shape->stroke, shape->strokeGradient, stroke_color, shape->opacity));
        }

        /* Clear vglpath. */
        VGL_ERROR_CHECK(vg_lite_clear_path(&vglpath));
    }

    /* Use stbi_write_png to generate a PNG file. VG_LITE_RGBA8888 FB has 4 components */
    //stbi_write_png("SVG_Render.png", fb->width, fb->height, 4, fb->memory, fb->stride);

    //if (fb && fb->handle) {
    //    VGL_ERROR_CHECK(vg_lite_free(fb));
    //}

    return 0;

VGLError:
    vg_lite_clear_path(&vglpath);
    //if (buffer.handle) {
    //    vg_lite_free(&buffer);
    //}
    return 1;
}

NSVGimage* parseSVGImage(char* input, int* pwidth, int* pheight)
{
    NSVGimage* image = nsvgParse(input, "px", 96.0f);

    if (image) {
        *pwidth = (int)ceil(image->width);
        *pheight = (int)ceil(image->height);
    }

    return image;
}

NSVGimage* parseSVGImageFromFile(const char* filename, int *pwidth, int *pheight)
{
#if 0
    NSVGimage* image = nsvgParseFromFile(filename, "px", 96.0f);

    if (image) {
        *pwidth = (int)ceil(image->width);
        *pheight = (int)ceil(image->height);
    }

    return image;
#else
    return NULL;
#endif
}

void deleteSVGImage(NSVGimage* image)
{
    nsvgDelete(image);
}

#if 0
void generate_result_name(char* filename)
{
    char* tmpname;
    int len;
    tmpname = strrchr(filename, '/');
    if (tmpname != NULL)
        tmpname++;
    else
        tmpname = filename;

    char* dot = strchr(tmpname, '.');
    if (dot != NULL) {
        len = dot - tmpname;
        strncpy(resultname, tmpname, len);
    }
    strcat(resultname, ".png");
}

int main(int argc, char **argv)
{
    NSVGimage *image = NULL;
    unsigned char *img = NULL;
    char *filename = NULL;
    float size;

    if (argc != 2) {
        fprintf(stderr, "Incorrect usage!\n\n");
        fprintf(stderr, "Usage: SvgVGLiteRenderer svg_file\n");
        return 1;
    }

    filename = argv[1];
    printf("Parsing SVG file: %s\n", filename);
    generate_result_name(filename);

    image = nsvgParseFromFile(filename, "px", 96.0f);
    if (image == NULL) {
        printf("Error: Could not parse SVG file %s!\n", filename);
        goto error;
    }

    /* Compute scale factor for SVG renderer. Scale small image to 400 */
    size = (image->width > image->height) ? image->width : image->height;
    if (size < 400.0f) {
        scale = 400.0f / size;
    }

    printf("Rendering SVG image with VGLite renderer\n");

    width = (int)(image->width * scale);
    height = (int)(image->height * scale);

    if (vg_lite_init(width, height))
    {
        printf("VGLite init fail!\n");
        goto error;
    }

    if (renderSVGImage(image))
    {
        printf("Error: renderSVGImage() failed with VGLite API calls!!!\n");
    }
    else {
        printf("Complete renderSVGImage() with VGLite API.\n");
    }

error:
    vg_lite_close();
    nsvgDelete(image);
    return 0;
}
#endif

#endif /* CONFIG_VG_LITE */
