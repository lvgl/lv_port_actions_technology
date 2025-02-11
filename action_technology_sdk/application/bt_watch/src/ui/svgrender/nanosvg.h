/*
 * Copyright (c) 2013-14 Mikko Mononen memon@inside.org
 * Copyright (c) 2023-24 Vivante Corporation, Santa Clara, California.
 * All Rights Reserved.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * The SVG parser is based on Anti-Grain Geometry 2.4 SVG example
 * Copyright (C) 2002-2004 Maxim Shemanarev (McSeem) (http://www.antigrain.com/)
 *
 * Arc calculation code based on canvg (https://code.google.com/p/canvg/)
 *
 * Bounding box calculation based on http://blog.hackers-cafe.net/2009/06/how-to-calculate-bezier-curves-bounding.html
 *
 */

#ifndef NANOSVG_H
#define NANOSVG_H

#ifndef NANOSVG_CPLUSPLUS
#ifdef __cplusplus
extern "C" {
#endif
#endif

// atof() does not work correctly with Keil MDK.

extern char *strtok(char * __restrict /*s1*/, const char * __restrict /*s2*/);

#if defined(__ZEPHYR__)
static char *strtok_s(char *str, const char *sep, char **state)
{
    char *start, *end;

    start = str ? str : *state;

    /* skip leading delimiters */
    while (*start && strchr(sep, *start)) {
        start++;
    }

    if (*start == '\0') {
        *state = start;
        return NULL;
    }

    /* look for token chars */
    end = start;
    while (*end && !strchr(sep, *end)) {
        end++;
    }

    if (*end != '\0') {
        *end = '\0';
        *state = end + 1;
    } else {
        *state = end;
    }

    return start;
}
#elif defined(__linux)
#define strtok_s strtok_r
#endif

// NanoSVG is a simple stupid single-header-file SVG parse. The output of the parser is a list of cubic bezier shapes.
//
// The library suits well for anything from rendering scalable icons in your editor application to prototyping a game.
//
// NanoSVG supports a wide range of SVG features, but something may be missing, feel free to create a pull request!
//
// The shapes in the SVG images are transformed by the viewBox and converted to specified units.
// That is, you should get the same looking data as your designed in your favorite app.
//
// NanoSVG can return the paths in few different units. For example if you want to render an image, you may choose
// to get the paths in pixels, or if you are feeding the data into a CNC-cutter, you may want to use millimeters.
//
// The units passed to NanoSVG should be one of: 'px', 'pt', 'pc' 'mm', 'cm', or 'in'.
// DPI (dots-per-inch) controls how the unit conversion is done.
//
// If you don't know or care about the units stuff, "px" and 96 should get you going.


/* Example Usage:
    // Load SVG
    NSVGimage* image;
    image = nsvgParseFromFile("test.svg", "px", 96);
    printf("size: %f x %f\n", image->width, image->height);
    // Use...
    for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
        for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
            for (int i = 0; i < path->npts-1; i += 3) {
                float* p = &path->pts[i*2];
                drawCubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7]);
            }
        }
    }
    // Delete
    nsvgDelete(image);
*/

enum NSVGpaintType {
    NSVG_PAINT_UNDEF = -1,
    NSVG_PAINT_NONE = 0,
    NSVG_PAINT_COLOR = 1,
    NSVG_PAINT_LINEAR_GRADIENT = 2,
    NSVG_PAINT_RADIAL_GRADIENT = 3,
    NSVG_PAINT_PATTERN = 4,
    NSVG_PAINT_TEXT = 5,
    NSVG_PAINT_IMAGE = 6,
};

enum NSVGspreadType {
    NSVG_SPREAD_PAD = 0,
    NSVG_SPREAD_REFLECT = 1,
    NSVG_SPREAD_REPEAT = 2
};

enum NSVGlineJoin {
    NSVG_JOIN_MITER = 0,
    NSVG_JOIN_ROUND = 1,
    NSVG_JOIN_BEVEL = 2
};

enum NSVGlineCap {
    NSVG_CAP_BUTT = 0,
    NSVG_CAP_ROUND = 1,
    NSVG_CAP_SQUARE = 2
};

enum NSVGfillRule {
    NSVG_FILLRULE_NONZERO = 0,
    NSVG_FILLRULE_EVENODD = 1
};

enum NSVGflags {
    NSVG_FLAGS_VISIBLE = 0x01
};

typedef enum TransformType
{
    TRANSLATE = 0,
    ROTATE = 1,
    SCALE = 2,
}TransformType;

typedef struct NSVGgradientStop {
    unsigned int color;
    float offset;
} NSVGgradientStop;

typedef struct NSVGgradient {
    float xform[6];
    float param[5];             // Storage for x1, y1, x2, y2 or cx, cy, r, fx, fy
    char units;                 // Gradient units NSVG_USER_SPACE or NSVG_OBJECT_SPACE
    char spread;
    float fx, fy;
    int nstops;
    NSVGgradientStop stops[1];
} NSVGgradient;

typedef struct NSVGpaint {
    signed char type;
    union {
        unsigned int color;
        NSVGgradient* gradient;
    };
    struct NSVGpattern* pattern;
    struct NSVGtext* text;
    struct NSVGdefsImage* image;
} NSVGpaint;

typedef struct NSVGpath
{
    float* pts;                    // Cubic bezier points: x0,y0, [cpx1,cpx1,cpx2,cpy2,x1,y1], ...
    int npts;                    // Total number of bezier points.
    char closed;                // Flag indicating if shapes should be treated as closed.
    float bounds[4];            // Tight bounding box of the shape [minx,miny,maxx,maxy].
    struct NSVGpath* next;        // Pointer to next path, or NULL if last element.
} NSVGpath;

typedef struct NSVGanimate
{
    char attributeName[16];            // The name of the attribute of the target element that is going to be changed during an animation
    char attributeType[16];            // Specifies the namespace in which the target attribute
    TransformType type;                // Define the type of transformation, such as translate¡¢scale¡¢rotate¡¢
    int form[3];                       // Starting data during an animation
    int to[3];                         // End data during an animation
    int begin;                         // Animation start time
    int dur;                           // Animation execution time
    int repeatCount;                   // The number of times the animation is repeatedly executed
    char transformBox[16];             // Define the layout box
    char transformOrigin[16];          // The origin for transformations
} NSVGanimate;

typedef struct VGLITEpath
{
    float* vgpath;                      // Pointer to array of 'vglite path' points
    unsigned char* opcode;              // Pointer to array of 'vglite path' opcode
    int vgpath_number;                  // Total number of 'vglite path' points
    int opcode_number;                  // Total number of 'vglite path' opcode
    struct VGLITEpath* next;            // Pointer to next VGLITEpath, or NULL if last element.
} VGLITEpath;

typedef struct NSVGshape
{
    char id[64];                // Optional 'id' attr of the shape or its group
    NSVGpaint fill;                // Fill paint
    NSVGpaint stroke;            // Stroke paint
    float opacity;                // Opacity of the shape.
    float fill_opacity;            // Opacity of the fill.
    float stroke_opacity;         // Opacity of the stroke.
    float strokeWidth;            // Stroke width (scaled).
    float strokeDashOffset;        // Stroke dash offset (scaled).
    float strokeDashArray[8];    // Stroke dash array (scaled).
    char strokeDashCount;        // Number of dash values in dash array.
    char strokeLineJoin;        // Stroke join type.
    char strokeLineCap;            // Stroke cap type.
    float miterLimit;            // Miter limit
    char fillRule;                // Fill rule, see NSVGfillRule.
    unsigned char flags;        // Logical or of NSVG_FLAGS_* flags
    float bounds[4];            // Tight bounding box of the shape [minx,miny,maxx,maxy].
    char fillGradient[64];        // Optional 'id' of fill gradient
    char fillPattern[64];        // Optional 'id' of fill pattern
    char strokeGradient[64];    // Optional 'id' of stroke gradient
    float xform[6];                // Root transformation for fill/stroke gradient
    float fontSize;             // the size of font
    NSVGpath* paths;            // Linked list of paths in the image.
    int mediaFlag;              // 'media' flag
    float minWidth;             // the min width of 'media'
    float minHeight;             // the min height of 'media'
    float maxWidth;             // the max width of 'media'
    float maxHeight;             // the max height of 'media'
    int blendMode;               // blend mode of shape
    int blend_flag;              // 'blendmode' flag
    int animateFlag;             // animate flag
    int pathType;                // path type
    struct NSVGclipPathData* clipPath; // Pointer of structure NSVGclipPathData, include clip-path data.
    struct NSVGshape* next;        // Pointer to next shape, or NULL if last element.
    VGLITEpath* vgPathOfCharacter;   // Pointer of structure VGLITEpath, include path data of character.
    NSVGanimate* animate;       // Pointer of structure NSVGanimate, include animete data
} NSVGshape;

typedef struct NSVGimage
{
    float width;                // Width of the image.
    float height;                // Height of the image.
    NSVGshape* shapes;            // Linked list of shapes in the image.
} NSVGimage;

// Parses SVG file from a file, returns SVG image as paths.
NSVGimage* nsvgParseFromFile(const char* filename, const char* units, float dpi);

// Parses SVG file from a null terminated string, returns SVG image as paths.
// Important note: changes the string.
NSVGimage* nsvgParse(char* input, const char* units, float dpi);

// Duplicates a path.
NSVGpath* nsvgDuplicatePath(NSVGpath* p);

// Deletes an image.
void nsvgDelete(NSVGimage* image);

#ifndef NANOSVG_CPLUSPLUS
#ifdef __cplusplus
}
#endif
#endif

#ifdef NANOSVG_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define NSVG_PI (3.14159265358979323846264338327f)
#define NSVG_KAPPA90 (0.5522847493f)    // Length proportional to radius of a cubic bezier handle for 90deg arcs.

#define NSVG_ALIGN_MIN 0
#define NSVG_ALIGN_MID 1
#define NSVG_ALIGN_MAX 2
#define NSVG_ALIGN_NONE 0
#define NSVG_ALIGN_MEET 1
#define NSVG_ALIGN_SLICE 2
#define IMGDATA_SIZE 8300
#define GROUP_CLASS_NUMS 10

#define NSVG_NOTUSED(v) do { (void)(1 ? (void)0 : ( (void)(v) ) ); } while(0)
#define NSVG_RGB(r, g, b) (((unsigned int)r) | ((unsigned int)g << 8) | ((unsigned int)b << 16))

#ifdef _MSC_VER
    #pragma warning (disable: 4996) // Switch off security warnings
    #pragma warning (disable: 4100) // Switch off unreferenced formal parameter warnings
    #ifdef __cplusplus
    #define NSVG_INLINE inline
    #else
    #define NSVG_INLINE
    #endif
#else
    #define NSVG_INLINE inline
#endif

/*Image URL format*/
#define NSVG_IMAGE_FORMAT_URL "url"

typedef enum VGBlendMode
{
    VG_MIXBLENDMODE_MULTIPLY = 5,
    VG_MIXBLENDMODE_SCREEN = 6,
    VG_MIXBLENDMODE_SRCIN =7,
} VGBlendMode;

/* Theoretical width and height of image in .svg file */
static float svg_image_width = 0;
static float svg_image_height = 0;
static float svg_scale_image_width = 0;
static float svg_scale_image_height = 0;
/* String pointers and flags for SVG defs/symbol handling */
static char* content_start = NULL;
static char* content_end = NULL;
static int   insideDefs = 0;
static int   emptyDefs = 0;
/*The number of "class" in group tree */
static char g_class_flag[GROUP_CLASS_NUMS];
static int  class_nums_count = 0;
static int  g_index = 0;

static void nsvg__startElement(void* ud, const char* el, const char** attr);
static void nsvg__endElement(void* ud, const char* el);

static int nsvg__isspace(char c)
{
    return strchr(" \t\n\v\f\r", c) != 0;
}

static int nsvg__isdigit(char c)
{
    return c >= '0' && c <= '9';
}

static NSVG_INLINE float nsvg__minf(float a, float b) { return a < b ? a : b; }
static NSVG_INLINE float nsvg__maxf(float a, float b) { return a > b ? a : b; }


// Simple XML parser

#define NSVG_XML_TAG 1
#define NSVG_XML_CONTENT 2
#define NSVG_XML_MAX_ATTRIBS 256

void nsvgDetectStyle(char* s,
                     void (*startelCb)(void* ud, const char* el, const char** attr),
                     void* ud);

static void nsvg__parseContent(char* s,
                               void (*startelCb)(void* ud, const char* el, const char** attr),
                               void (*contentCb)(void* ud, const char* s),
                               void* ud)
{
    // Trim start white spaces
    while (*s && nsvg__isspace(*s)) s++;
    if (!*s) return;

    nsvgDetectStyle(s, startelCb, ud);

    if (contentCb)
        (*contentCb)(ud, s);
}

static void nsvg__parseElement(char* s,
                               void (*startelCb)(void* ud, const char* el, const char** attr),
                               void (*endelCb)(void* ud, const char* el),
                               void* ud)
{
    const char* attr[NSVG_XML_MAX_ATTRIBS];
    int nattr = 0;
    char* name;
    int start = 0;
    int end = 0;
    char quote;

    // Skip white space after the '<'
    while (*s && nsvg__isspace(*s)) s++;

    // Check if the tag is end tag
    if (*s == '/') {
        s++;
        end = 1;
    } else {
        start = 1;
    }

    // Skip comments, data and preprocessor stuff.
    if (!*s || *s == '?' || *s == '!')
        return;

    // Get tag name
    name = s;
    while (*s && !nsvg__isspace(*s)) s++;
    if (*s) { *s++ = '\0'; }

    // Get attribs
    while (!end && *s && nattr < NSVG_XML_MAX_ATTRIBS-3) {
        char* name = NULL;
        char* value = NULL;

        // Skip white space before the attrib name
        while (*s && nsvg__isspace(*s)) s++;
        if (!*s) break;
        if (*s == '/') {
            end = 1;
            break;
        }
        name = s;
        // Find end of the attrib name.
        while (*s && !nsvg__isspace(*s) && *s != '=') s++;
        if (*s) { *s++ = '\0'; }
        // Skip until the beginning of the value.
        while (*s && *s != '\"' && *s != '\'') s++;
        if (!*s) break;
        quote = *s;
        s++;
        // Store value and find the end of it.
        value = s;
        while (*s && *s != quote) s++;
        if (*s) { *s++ = '\0'; }

        // Store only well formed attributes
        if (name && value) {
            attr[nattr++] = name;
            attr[nattr++] = value;
        }
    }

    // List terminator
    attr[nattr++] = 0;
    attr[nattr++] = 0;

    if (start && end && strcmp(name, "defs") == 0) {
        emptyDefs = 1;
    }

    // Call callbacks.
    if (start && startelCb)
        (*startelCb)(ud, name, attr);
    if (end && endelCb)
        (*endelCb)(ud, name);
}

int nsvg__parseXML(char* input,
                   void (*startelCb)(void* ud, const char* el, const char** attr),
                   void (*endelCb)(void* ud, const char* el),
                   void (*contentCb)(void* ud, const char* s),
                   void* ud)
{
    char* s = input;
    char* mark = s;
    int state = NSVG_XML_CONTENT;

    /* reset global variables */
    memset(g_class_flag, 0, sizeof(g_class_flag));
    class_nums_count = 0;
    g_index = 0;

    svg_image_width = 0;
    svg_image_height = 0;
    svg_scale_image_width = 0;
    svg_scale_image_height = 0;

    content_start = NULL;
    content_end = NULL;
    insideDefs = 0;
    emptyDefs = 0;

    while (*s) {
        if (*s == '<' && state == NSVG_XML_CONTENT) {
            if (memcmp(s, "<!--", 4) == 0) {
                // Skip multiline comment
                while (memcmp(s++, "-->", 3) != 0);
                s += 2;
                continue;
            }
            if (memcmp(s, "</g>", 4) == 0) {
                if (g_index && g_class_flag[g_index - 1]) {
                    g_class_flag[g_index - 1] = 0;
                    class_nums_count--;
                }
                if (g_index)
                    g_index--;
            }
            // Start of a tag
            *s++ = '\0';
            nsvg__parseContent(mark, startelCb, contentCb, ud);
            mark = s;
            state = NSVG_XML_TAG;
        } else if (*s == '>' && state == NSVG_XML_TAG) {
            // Start of a content or new tag.
            *s++ = '\0';
            if (insideDefs || memcmp(mark, "symbol", 6) == 0) {
                content_start = mark;
                content_end = NULL;
            }
            nsvg__parseElement(mark, startelCb, endelCb, ud);
            if (content_end) {
                // Skip <defs> and <symbol> string parsing
                s = content_end;
                content_end = NULL;
            }
            mark = s;
            state = NSVG_XML_CONTENT;
        } else {
            s++;
        }
    }

    return 1;
}

/* Simple SVG parser. */

#define NSVG_MAX_ATTR 128

enum NSVGgradientUnits {
    NSVG_USER_SPACE = 0,
    NSVG_OBJECT_SPACE = 1
};

#define NSVG_MAX_DASHES 8

enum NSVGunits {
    NSVG_UNITS_USER,
    NSVG_UNITS_PX,
    NSVG_UNITS_PT,
    NSVG_UNITS_PC,
    NSVG_UNITS_MM,
    NSVG_UNITS_CM,
    NSVG_UNITS_IN,
    NSVG_UNITS_PERCENT,
    NSVG_UNITS_EM,
    NSVG_UNITS_EX
};

enum NSVGpathType {
    NSVG_PATH = 0,
    NSVG_PATTERN_PATH = 1,
    NSVG_CLIP_PATH = 2,
};

typedef struct NSVGcoordinate {
    float value;
    int units;
} NSVGcoordinate;

typedef struct NSVGlinearData {
    NSVGcoordinate x1, y1, x2, y2;
} NSVGlinearData;

typedef struct NSVGradialData {
    NSVGcoordinate cx, cy, r, fx, fy;
} NSVGradialData;

typedef struct NSVGgradientData
{
    char id[64];
    char ref[64];
    signed char type;
    union {
        NSVGlinearData linear;
        NSVGradialData radial;
    };
    char spread;
    char units;
    float xform[6];
    int nstops;
    int unitsFlag;
    int xformFlag;
    NSVGgradientStop* stops;
    struct NSVGgradientData* next;
} NSVGgradientData;

typedef struct NSVGsymbolData
{
    char id[64];
    char* content;
    int  length;
    struct NSVGsymbolData* next;
} NSVGsymbolData;

typedef struct NSVGdefsTagData
{
    char id[64];
    char tag[64];
    char* content;
    int  length;
    struct NSVGdefsTagData* next;
} NSVGdefsTagData;

typedef struct NSVGtextData
{
    NSVGcoordinate x, y, dx, dy, rotate, textLength, fontSize;
    char class_name[64];
    char content[128];
    char font_family[64];
} NSVGtextData;

typedef struct NSVGtextpathData
{
    NSVGcoordinate textLength;
    char class_name[64];
    char ref[64];
    char spacing[64];
    char method[64];
    char content[128];
} NSVGtextpathData;

typedef struct NSVGdefsImage
{
    char name[64];
    char format[64];
    char encoding[64];
    float x, y, width, height;
    char imgdata[IMGDATA_SIZE];
    int drawFlag;
    struct NSVGdefsImage* next;
}NSVGdefsImage;

typedef struct NSVGpatternData
{
    char id[64];
    char ref[64];
    signed char type;
    NSVGcoordinate x, y, width, height;
    float viewMinx, viewMiny, viewWidth, viewHeight;
    char units;
    char patternContentUnits;
    char patternTransform;
    float preserveAspectRatio;
    struct NSVGpatternData* next;
    NSVGshape* shape;
    NSVGdefsImage* image;
} NSVGpatternData;

typedef struct NSVGclipPathData {
    char id[64];
    NSVGpath* path;
    char units;
    struct NSVGclipPathData* next;
    char premul_flag;
    char use_flag;
    char flag;
} NSVGclipPathData;

typedef struct NSVGanimateData
{
    char attributeName[16];
    char attributeType[16];
    char type[10];
    int form[3];
    int to[3];
    int begin;
    int dur;
    int repeatCount;
} NSVGanimateData;

typedef struct NSVGpattern {
    float x, y, width, height;
    char units;
    NSVGshape* shape;
    NSVGdefsImage* image;
} NSVGpattern;

typedef struct NSVGtext {
    float x, y, dx, dy, rotate, textLength, fontSize;
    char content[128];
    int hasStroke;
} NSVGtext;

typedef struct NSVGattrib
{
    char id[64];
    float xform[6];
    unsigned int fillColor;
    unsigned int strokeColor;
    float opacity;
    float fillOpacity;
    float strokeOpacity;
    char fillGradient[64];
    char fillPattern[64];
    char strokeGradient[64];
    char clipPath[64];
    float strokeWidth;
    float strokeDashOffset;
    float strokeDashArray[NSVG_MAX_DASHES];
    int strokeDashCount;
    char strokeLineJoin;
    char strokeLineCap;
    float miterLimit;
    char fillRule;
    float fontSize;
    unsigned int stopColor;
    float stopOpacity;
    float stopOffset;
    char hasFill;
    char fillFlag;
    char hasStroke;
    char strokeFlag;
    char visible;
    char transformOrigin[16];
    char transformBox[16];
} NSVGattrib;

typedef struct NSVGstyle
{
    char name[64];
    char hasStroke;
    char strokeFlag;
    char strokeLineJoin;
    char strokeLineCap;
    char hasFill;
    char fillFlag;
    char fillRule;
    char visible;
    float strokeWidth;
    float strokeDashArray[NSVG_MAX_DASHES];
    float strokeDashOffset;
    float miterLimit;
    float strokeOpacity;
    float fillOpacity;
    float opacity;
    NSVGcoordinate fontsize;

    unsigned int fillColor;
    unsigned int strokeColor;

    int mediaFlag;
    float minWidth;
    float minHeight;
    float maxWidth;
    float maxHeight;

    char fillGradient[64];
    char fillPattern[64];

    char mixBlendMode[64];

    struct NSVGstyle* next;
} NSVGstyle;

typedef struct NSVGdefspath
{
    char name[64];
    float x, y;

    struct NSVGdefspath* next;
}NSVGdefspath;

typedef struct NSVGtextFlag {
    char textTegFlag;
    char textClassFlag;

    char textpathTegFlag;
    char textpathClassFlag;
}NSVGtextFlag;

typedef struct NSVGparser
{
    NSVGattrib attr[NSVG_MAX_ATTR];
    int attrHead;
    float* pts;
    int npts;
    int cpts;
    NSVGpath* plist;
    NSVGimage* image;
    NSVGgradientData* gradients;
    NSVGpatternData* patterns;
    NSVGclipPathData* clipPaths;
    NSVGtextData* text;
    NSVGtextpathData* textpath;
    NSVGshape* shapesTail;
    NSVGsymbolData* symbols;
    NSVGdefsTagData* defTags;
    NSVGanimateData* animateData;
    char animateFlag;
    char* defsString;
    char* defsStart;
    float viewMinx, viewMiny, viewWidth, viewHeight;
    int alignX, alignY, alignType;
    float dpi;
    char pathFlag;
    char defsFlag;
    char patternFlag;
    char clipPathFlag;
    char styleFlag;
    NSVGstyle* style;
    int class_flag;
    char class_name[GROUP_CLASS_NUMS][64];
    NSVGdefspath* defspath;
    NSVGdefsImage* defsimage;
    NSVGtextFlag textFlag;
} NSVGparser;

static void nsvg__xformIdentity(float* t)
{
    t[0] = 1.0f; t[1] = 0.0f;
    t[2] = 0.0f; t[3] = 1.0f;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetTranslation(float* t, float tx, float ty)
{
    t[0] = 1.0f; t[1] = 0.0f;
    t[2] = 0.0f; t[3] = 1.0f;
    t[4] = tx; t[5] = ty;
}

static void nsvg__xformSetScale(float* t, float sx, float sy)
{
    t[0] = sx; t[1] = 0.0f;
    t[2] = 0.0f; t[3] = sy;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetSkewX(float* t, float a)
{
    t[0] = 1.0f; t[1] = 0.0f;
    t[2] = tanf(a); t[3] = 1.0f;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetSkewY(float* t, float a)
{
    t[0] = 1.0f; t[1] = tanf(a);
    t[2] = 0.0f; t[3] = 1.0f;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformSetRotation(float* t, float a)
{
    float cs = cosf(a), sn = sinf(a);
    t[0] = cs; t[1] = sn;
    t[2] = -sn; t[3] = cs;
    t[4] = 0.0f; t[5] = 0.0f;
}

static void nsvg__xformMultiply(float* t, float* s)
{
    float t0 = t[0] * s[0] + t[1] * s[2];
    float t2 = t[2] * s[0] + t[3] * s[2];
    float t4 = t[4] * s[0] + t[5] * s[2] + s[4];
    t[1] = t[0] * s[1] + t[1] * s[3];
    t[3] = t[2] * s[1] + t[3] * s[3];
    t[5] = t[4] * s[1] + t[5] * s[3] + s[5];
    t[0] = t0;
    t[2] = t2;
    t[4] = t4;
}

static void nsvg__xformInverse(float* inv, float* t)
{
    double invdet, det = (double)t[0] * t[3] - (double)t[2] * t[1];
    if (det > -1e-6 && det < 1e-6) {
        nsvg__xformIdentity(t);
        return;
    }
    invdet = 1.0 / det;
    inv[0] = (float)(t[3] * invdet);
    inv[2] = (float)(-t[2] * invdet);
    inv[4] = (float)(((double)t[2] * t[5] - (double)t[3] * t[4]) * invdet);
    inv[1] = (float)(-t[1] * invdet);
    inv[3] = (float)(t[0] * invdet);
    inv[5] = (float)(((double)t[1] * t[4] - (double)t[0] * t[5]) * invdet);
}

static void nsvg__xformPremultiply(float* t, float* s)
{
    float s2[6];
    memcpy(s2, s, sizeof(float)*6);
    nsvg__xformMultiply(s2, t);
    memcpy(t, s2, sizeof(float)*6);
}

static void nsvg__xformPoint(float* dx, float* dy, float x, float y, float* t)
{
    *dx = x*t[0] + y*t[2] + t[4];
    *dy = x*t[1] + y*t[3] + t[5];
}

static void nsvg__xformVec(float* dx, float* dy, float x, float y, float* t)
{
    *dx = x*t[0] + y*t[2];
    *dy = x*t[1] + y*t[3];
}

#define NSVG_EPSILON (1e-12)

static int nsvg__ptInBounds(float* pt, float* bounds)
{
    return pt[0] >= bounds[0] && pt[0] <= bounds[2] && pt[1] >= bounds[1] && pt[1] <= bounds[3];
}


static double nsvg__evalBezier(double t, double p0, double p1, double p2, double p3)
{
    double it = 1.0-t;
    return it*it*it*p0 + 3.0*it*it*t*p1 + 3.0*it*t*t*p2 + t*t*t*p3;
}

static void nsvg__curveBounds(float* bounds, float* curve)
{
    int i, j, count;
    double roots[2], a, b, c, b2ac, t, v;
    float* v0 = &curve[0];
    float* v1 = &curve[2];
    float* v2 = &curve[4];
    float* v3 = &curve[6];

    // Start the bounding box by end points
    bounds[0] = nsvg__minf(v0[0], v3[0]);
    bounds[1] = nsvg__minf(v0[1], v3[1]);
    bounds[2] = nsvg__maxf(v0[0], v3[0]);
    bounds[3] = nsvg__maxf(v0[1], v3[1]);

    // Bezier curve fits inside the convex hull of it's control points.
    // If control points are inside the bounds, we're done.
    if (nsvg__ptInBounds(v1, bounds) && nsvg__ptInBounds(v2, bounds))
        return;

    // Add bezier curve inflection points in X and Y.
    for (i = 0; i < 2; i++) {
        a = -3.0 * v0[i] + 9.0 * v1[i] - 9.0 * v2[i] + 3.0 * v3[i];
        b = 6.0 * v0[i] - 12.0 * v1[i] + 6.0 * v2[i];
        c = 3.0 * v1[i] - 3.0 * v0[i];
        count = 0;
        if (fabs(a) < NSVG_EPSILON) {
            if (fabs(b) > NSVG_EPSILON) {
                t = -c / b;
                if (t > NSVG_EPSILON && t < 1.0-NSVG_EPSILON)
                    roots[count++] = t;
            }
        } else {
            b2ac = b*b - 4.0*c*a;
            if (b2ac > NSVG_EPSILON) {
                t = (-b + sqrt(b2ac)) / (2.0 * a);
                if (t > NSVG_EPSILON && t < 1.0-NSVG_EPSILON)
                    roots[count++] = t;
                t = (-b - sqrt(b2ac)) / (2.0 * a);
                if (t > NSVG_EPSILON && t < 1.0-NSVG_EPSILON)
                    roots[count++] = t;
            }
        }
        for (j = 0; j < count; j++) {
            v = nsvg__evalBezier(roots[j], v0[i], v1[i], v2[i], v3[i]);
            bounds[0+i] = nsvg__minf(bounds[0+i], (float)v);
            bounds[2+i] = nsvg__maxf(bounds[2+i], (float)v);
        }
    }
}

static NSVGparser* nsvg__createParser(void)
{
    NSVGparser* p;
    p = (NSVGparser*)NANOSVG_MALLOC(sizeof(NSVGparser));
    if (p == NULL) goto error;
    memset(p, 0, sizeof(NSVGparser));

    p->image = (NSVGimage*)NANOSVG_MALLOC(sizeof(NSVGimage));
    if (p->image == NULL) goto error;
    memset(p->image, 0, sizeof(NSVGimage));

    // Init style
    nsvg__xformIdentity(p->attr[0].xform);
    memset(p->attr[0].id, 0, sizeof p->attr[0].id);
    p->attr[0].fillColor = NSVG_RGB(0,0,0);
    p->attr[0].strokeColor = NSVG_RGB(0,0,0);
    p->attr[0].opacity = 1;
    p->attr[0].fillOpacity = 1;
    p->attr[0].strokeOpacity = 1;
    p->attr[0].stopOpacity = 1;
    p->attr[0].strokeWidth = 1;
    p->attr[0].strokeLineJoin = NSVG_JOIN_MITER;
    p->attr[0].strokeLineCap = NSVG_CAP_BUTT;
    p->attr[0].miterLimit = 4;
    p->attr[0].fillRule = NSVG_FILLRULE_NONZERO;
    p->attr[0].hasFill = 1;
    p->attr[0].visible = 1;

    return p;

error:
    if (p) {
        if (p->image) NANOSVG_FREE(p->image);
        NANOSVG_FREE(p);
    }
    return NULL;
}

static void nsvg__deletePaths(NSVGpath* path)
{
    while (path) {
        NSVGpath *next = path->next;
        if (path->pts != NULL)
        {
            NANOSVG_FREE(path->pts);
        }
        NANOSVG_FREE(path);
        path = next;
    }
}

static void nsvg__deleteDefsPaths(NSVGdefspath* defspath)
{
    while (defspath) {
        NSVGdefspath* next = defspath->next;
        NANOSVG_FREE(defspath);
        defspath = next;
    }
}

static void nsvg__deleteVGLitePaths(VGLITEpath* path)
{
    while (path) {
        VGLITEpath* next = path->next;
        if (path->vgpath)
            NANOSVG_FREE(path->vgpath);
        if (path->opcode)
            NANOSVG_FREE(path->opcode);
        NANOSVG_FREE(path);
        path = next;
    }
}

static void nsvg__deletePaint(NSVGpaint* paint)
{
    if (paint->text)
        NANOSVG_FREE(paint->text);

    if (paint->image)
        NANOSVG_FREE(paint->image);

    if (paint->type == NSVG_PAINT_LINEAR_GRADIENT || paint->type == NSVG_PAINT_RADIAL_GRADIENT)
        NANOSVG_FREE(paint->gradient);

    if (paint->pattern) {
        if (paint->pattern->image)
            NANOSVG_FREE(paint->pattern->image);
        NANOSVG_FREE(paint->pattern);
    }
}


static void nsvg__deleteAnimate(NSVGanimate* animate)
{
    if (animate)
        NANOSVG_FREE(animate);
}

static void nsvg__deleteClipPath(NSVGshape* shape, NSVGclipPathData* clipPath, char use_flag)
{
    if (use_flag)
    {
        if (clipPath != NULL && (shape->next == NULL || shape->next->clipPath != clipPath))
        {
            if (clipPath->path != NULL)
            {
                NSVGpath* path = clipPath->path;
                while (path) {
                    NSVGpath* next = path->next;
                    if (path->pts != NULL)
                    {
                        NANOSVG_FREE(path->pts);
                        path->pts = NULL;
                    }
                    NANOSVG_FREE(path);
                    path = next;
                }

                NANOSVG_FREE(clipPath);
            }
        }
    }
    else
    {
        NSVGclipPathData* tem_clipPath = clipPath;
        while (tem_clipPath) {
            if (tem_clipPath->use_flag == 0)
            {
                NSVGpath* path = tem_clipPath->path;
                while (path) {
                    NSVGpath* next = path->next;
                    if (path->pts != NULL)
                    {
                        NANOSVG_FREE(path->pts);
                        path->pts = NULL;
                    }
                    NANOSVG_FREE(path);
                    path = next;
                }

                NSVGclipPathData* next_clippath = tem_clipPath->next;
                NANOSVG_FREE(tem_clipPath);
                tem_clipPath = next_clippath;
            }
            else
            {
                tem_clipPath = tem_clipPath->next;
            }
        }
    }
}

static void nsvg__deleteGradientData(NSVGgradientData* grad)
{
    NSVGgradientData* next;
    while (grad != NULL) {
        next = grad->next;
        NANOSVG_FREE(grad->stops);
        NANOSVG_FREE(grad);
        grad = next;
    }
}

static void nsvg__deletePatternData(NSVGpatternData* pattern)
{
    NSVGpatternData* next;
    while (pattern != NULL) {
        next = pattern->next;
        NANOSVG_FREE(pattern);
        pattern = next;
    }
}

static void nsvg__deleteSymbols(NSVGsymbolData* symb)
{
    while (symb) {
        NSVGsymbolData *next = symb->next;
        NANOSVG_FREE(symb->content);
        NANOSVG_FREE(symb);
        symb = next;
    }
}

static void nsvg__deleteDefsTags(NSVGdefsTagData* tag)
{
    while (tag) {
        NSVGdefsTagData *next = tag->next;
        NANOSVG_FREE(tag->content);
        NANOSVG_FREE(tag);
        tag = next;
    }
}

static void nsvg__deleteStyle(NSVGstyle* style)
{
    while (style) {
        NSVGstyle* next = style->next;
        NANOSVG_FREE(style);
        style = next;
    }
}

static void nsvg__deleteDefsImage(NSVGdefsImage* image)
{
    while (image) {
        NSVGdefsImage* next = image->next;
        NANOSVG_FREE(image);
        image = next;
    }
}

static void nsvg__deleteParser(NSVGparser* p)
{
    if (p != NULL) {
        nsvg__deletePaths(p->plist);
        nsvg__deleteDefsPaths(p->defspath);
        nsvg__deleteGradientData(p->gradients);
        nsvg__deletePatternData(p->patterns);
        nsvg__deleteSymbols(p->symbols);
        nsvg__deleteDefsTags(p->defTags);
        nsvg__deleteStyle(p->style);
        nsvg__deleteDefsImage(p->defsimage);
        nsvg__deleteClipPath(NULL, p->clipPaths, 0);
        nsvgDelete(p->image);

        if (p->defsString)
            NANOSVG_FREE(p->defsString);
        if (p->text)
            NANOSVG_FREE(p->text);
        if (p->textpath)
            NANOSVG_FREE(p->textpath);

        NANOSVG_FREE(p->pts);
        NANOSVG_FREE(p);
    }
}

static void nsvg__resetPath(NSVGparser* p)
{
    p->npts = 0;
}

static void nsvg__addPoint(NSVGparser* p, float x, float y)
{
    if (p->npts+1 > p->cpts) {
        p->cpts = p->cpts ? p->cpts*2 : 8;
        p->pts = (float*)NANOSVG_REALLOC(p->pts, p->cpts*2*sizeof(float));
        if (!p->pts) return;
    }
    p->pts[p->npts*2+0] = x;
    p->pts[p->npts*2+1] = y;
    p->npts++;
}

static void nsvg__moveTo(NSVGparser* p, float x, float y)
{
    if (p->npts > 0) {
        p->pts[(p->npts-1)*2+0] = x;
        p->pts[(p->npts-1)*2+1] = y;
    } else {
        nsvg__addPoint(p, x, y);
    }
}

static void nsvg__lineTo(NSVGparser* p, float x, float y)
{
    float px,py, dx,dy;
    if (p->npts > 0) {
        px = p->pts[(p->npts-1)*2+0];
        py = p->pts[(p->npts-1)*2+1];
        dx = x - px;
        dy = y - py;
        nsvg__addPoint(p, px + dx/3.0f, py + dy/3.0f);
        nsvg__addPoint(p, x - dx/3.0f, y - dy/3.0f);
        nsvg__addPoint(p, x, y);
    }
}

static void nsvg__cubicBezTo(NSVGparser* p, float cpx1, float cpy1, float cpx2, float cpy2, float x, float y)
{
    if (p->npts > 0) {
        nsvg__addPoint(p, cpx1, cpy1);
        nsvg__addPoint(p, cpx2, cpy2);
        nsvg__addPoint(p, x, y);
    }
}

static NSVGattrib* nsvg__getAttr(NSVGparser* p)
{
    return &p->attr[p->attrHead];
}

static void nsvg__pushAttr(NSVGparser* p)
{
    if (p->attrHead < NSVG_MAX_ATTR-1) {
        p->attrHead++;
        memcpy(&p->attr[p->attrHead], &p->attr[p->attrHead-1], sizeof(NSVGattrib));
    }
}

static void nsvg__popAttr(NSVGparser* p)
{
    if (p->attrHead > 0)
        p->attrHead--;
}

static float nsvg__actualOrigX(NSVGparser* p)
{
    return p->viewMinx;
}

static float nsvg__actualOrigY(NSVGparser* p)
{
    return p->viewMiny;
}

static float nsvg__actualWidth(NSVGparser* p)
{
    return p->viewWidth;
}

static float nsvg__actualHeight(NSVGparser* p)
{
    return p->viewHeight;
}

static float nsvg__actualLength(NSVGparser* p)
{
    float w = nsvg__actualWidth(p), h = nsvg__actualHeight(p);
    return sqrtf(w*w + h*h) / sqrtf(2.0f);
}

static float nsvg__convertToPixels(NSVGparser* p, NSVGcoordinate c, float orig, float length)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    switch (c.units) {
        case NSVG_UNITS_USER:        return c.value;
        case NSVG_UNITS_PX:            return c.value;
        case NSVG_UNITS_PT:            return c.value / 72.0f * p->dpi;
        case NSVG_UNITS_PC:            return c.value / 6.0f * p->dpi;
        case NSVG_UNITS_MM:            return c.value / 25.4f * p->dpi;
        case NSVG_UNITS_CM:            return c.value / 2.54f * p->dpi;
        case NSVG_UNITS_IN:            return c.value * p->dpi;
        case NSVG_UNITS_EM:            return c.value * attr->fontSize;
        case NSVG_UNITS_EX:            return c.value * attr->fontSize * 0.52f; // x-height of Helvetica.
        case NSVG_UNITS_PERCENT:    return orig + c.value / 100.0f * length;
        default:                    return c.value;
    }
    return c.value;
}

static NSVGsymbolData* nsvg__findSymbolData(NSVGparser* p, const char* id)
{
    NSVGsymbolData* symb = p->symbols;
    if (id == NULL || *id == '\0')
        return NULL;
    while (symb != NULL) {
        if (strcmp(symb->id, id) == 0)
            return symb;
        symb = symb->next;
    }
    return NULL;
}

static NSVGdefsTagData* nsvg__findDefsTagData(NSVGparser* p, const char* id)
{
    NSVGdefsTagData* tag = p->defTags;
    if (id == NULL || *id == '\0')
        return NULL;
    while (tag != NULL) {
        if (strcmp(tag->id, id) == 0)
            return tag;
        tag = tag->next;
    }
    return NULL;
}

static NSVGdefsImage* nasvg__findDefsImage(NSVGparser* p, const char* id)
{
    NSVGdefsImage* image = p->defsimage;
    while (image) {
        if (strcmp(image->name, id) == 0)
            return image;
        image = image->next;
    }

    return NULL;
}

static NSVGgradientData* nsvg__findGradientData(NSVGparser* p, const char* id)
{
    NSVGgradientData* grad = p->gradients;
    if (id == NULL || *id == '\0')
        return NULL;
    while (grad != NULL) {
        if (strcmp(grad->id, id) == 0)
            return grad;
        grad = grad->next;
    }
    return NULL;
}

static NSVGpatternData* nsvg__findPatternData(NSVGparser* p, const char* id)
{
    NSVGpatternData* pattern = p->patterns;
    if (id == NULL || *id == '\0')
        return NULL;
    while (pattern != NULL) {
        if (strcmp(pattern->id, id) == 0)
            return pattern;
        pattern = pattern->next;
    }
    return NULL;
}

static NSVGclipPathData* nsvg__findClipPathData(NSVGparser* p, const char* id)
{
    NSVGclipPathData* clipPath = p->clipPaths;
    if (id == NULL || *id == '\0')
        return NULL;
    while (clipPath != NULL) {
        if (strcmp(clipPath->id, id) == 0) {
            clipPath->use_flag = 1;
            return clipPath;
        }
        clipPath = clipPath->next;
    }
    return NULL;
}

static NSVGpattern* nsvg__createPattern(NSVGparser* p, const char* id, signed char* paintType)
{
    NSVGpatternData* data = NULL;
    NSVGpattern* pattern = NULL;

    data = nsvg__findPatternData(p, id);
    if (data == NULL) return NULL;

    pattern = (NSVGpattern*)NANOSVG_MALLOC(sizeof(NSVGpattern));  //Only one pattern
    if (pattern == NULL) return NULL;
    pattern->x = data->x.value;
    pattern->y = data->y.value;
    pattern->height = data->height.value;
    pattern->width = data->width.value;
    pattern->shape = data->shape;
    pattern->units = data->units;

    if (data->image) {
        NSVGdefsImage* shapeImage = (NSVGdefsImage*)NANOSVG_MALLOC(sizeof(NSVGdefsImage));
        memcpy(shapeImage, data->image, sizeof(NSVGdefsImage));
        pattern->image = shapeImage;
    } else {
        pattern->image = NULL;
    }

    *paintType = data->type;

    return pattern;
}

static NSVGgradient* nsvg__createGradient(NSVGparser* p, const char* id, const float* localBounds, float *xform, signed char* paintType)
{
    NSVGgradientData* data = NULL;
    NSVGgradientData* ref = NULL;
    NSVGgradientStop* stops = NULL;
    NSVGgradient* grad;
    float ox, oy, sw, sh, sl;
    int nstops = 0;
    int refIter;

    data = nsvg__findGradientData(p, id);
    if (data == NULL) return NULL;

    // TODO: use ref to fill in all unset values too.
    ref = data;
    refIter = 0;
    while (ref != NULL) {
        NSVGgradientData* nextRef = NULL;
        if (!data->xformFlag && ref->xformFlag) {
            for (int i = 0; i < 6; i++)
                data->xform[i] = ref->xform[i];
        }
        if (!data->unitsFlag && ref->unitsFlag) {
            data->unitsFlag = 1;
            data->units = ref->units;
        }
        if (stops == NULL && ref->stops != NULL) {
            stops = ref->stops;
            nstops = ref->nstops;
            break;
        }
        nextRef = nsvg__findGradientData(p, ref->ref);
        if (nextRef == ref) break; // prevent infite loops on malformed data
        ref = nextRef;
        refIter++;
        if (refIter > 32) break; // prevent infite loops on malformed data
    }
    if (stops == NULL) return NULL;

    grad = (NSVGgradient*)NANOSVG_MALLOC(sizeof(NSVGgradient) + sizeof(NSVGgradientStop)*(nstops-1));
    if (grad == NULL) return NULL;

    // The shape width and height.
    if (data->units == NSVG_OBJECT_SPACE) {
        ox = localBounds[0];
        oy = localBounds[1];
        sw = localBounds[2] - localBounds[0];
        sh = localBounds[3] - localBounds[1];
    } else {
        ox = nsvg__actualOrigX(p);
        oy = nsvg__actualOrigY(p);
        sw = nsvg__actualWidth(p);
        sh = nsvg__actualHeight(p);
    }
    sl = sqrtf(sw*sw + sh*sh) / sqrtf(2.0f);

    grad->units = data->units;
    for (int i = 0; i < 6; i++)
        grad->xform[i] = data->xform[i];

    if (data->type == NSVG_PAINT_LINEAR_GRADIENT) {
        /* Convert NSVG_UNITS_PERCENT unit to NSVG_UNITS_USER */
        grad->param[0] = data->linear.x1.units ? data->linear.x1.value * 0.01 : data->linear.x1.value;
        grad->param[1] = data->linear.y1.units ? data->linear.y1.value * 0.01 : data->linear.y1.value;
        grad->param[2] = data->linear.x2.units ? data->linear.x2.value * 0.01 : data->linear.x2.value;
        grad->param[3] = data->linear.y2.units ? data->linear.y2.value * 0.01 : data->linear.y2.value;

    } else {
        /* Convert NSVG_UNITS_PERCENT unit to NSVG_UNITS_USER */
        grad->param[0] = data->radial.cx.units ? data->radial.cx.value * 0.01 : data->radial.cx.value;
        grad->param[1] = data->radial.cy.units ? data->radial.cy.value * 0.01 : data->radial.cy.value;
        grad->param[2] = data->radial.r.units  ? data->radial.r.value  * 0.01 : data->radial.r.value;
        grad->param[3] = data->radial.fx.units ? data->radial.fx.value * 0.01 : data->radial.fx.value;
        grad->param[4] = data->radial.fy.units ? data->radial.fy.value * 0.01 : data->radial.fy.value;
    }

    if(!data->units)
        nsvg__xformMultiply(grad->xform, xform);

    grad->spread = data->spread;
    memcpy(grad->stops, stops, nstops*sizeof(NSVGgradientStop));
    grad->nstops = nstops;

    *paintType = data->type;

    return grad;
}

static float nsvg__getAverageScale(float* t)
{
    float sx = sqrtf(t[0]*t[0] + t[2]*t[2]);
    float sy = sqrtf(t[1]*t[1] + t[3]*t[3]);
    return (sx + sy) * 0.5f;
}

static void nsvg__getLocalBounds(float* bounds, NSVGshape *shape, float* xform)
{
    NSVGpath* path;
    float curve[4*2], curveBounds[4];
    int i, first = 1;
    for (path = shape->paths; path != NULL; path = path->next) {
        nsvg__xformPoint(&curve[0], &curve[1], path->pts[0], path->pts[1], xform);
        for (i = 0; i < path->npts-1; i += 3) {
            nsvg__xformPoint(&curve[2], &curve[3], path->pts[(i+1)*2], path->pts[(i+1)*2+1], xform);
            nsvg__xformPoint(&curve[4], &curve[5], path->pts[(i+2)*2], path->pts[(i+2)*2+1], xform);
            nsvg__xformPoint(&curve[6], &curve[7], path->pts[(i+3)*2], path->pts[(i+3)*2+1], xform);
            nsvg__curveBounds(curveBounds, curve);
            if (first) {
                bounds[0] = curveBounds[0];
                bounds[1] = curveBounds[1];
                bounds[2] = curveBounds[2];
                bounds[3] = curveBounds[3];
                first = 0;
            } else {
                bounds[0] = nsvg__minf(bounds[0], curveBounds[0]);
                bounds[1] = nsvg__minf(bounds[1], curveBounds[1]);
                bounds[2] = nsvg__maxf(bounds[2], curveBounds[2]);
                bounds[3] = nsvg__maxf(bounds[3], curveBounds[3]);
            }
            curve[0] = curve[6];
            curve[1] = curve[7];
        }
    }
}

static void nsvg__addClassShape(NSVGparser* p)
{
    if (p->plist == NULL)
        return;

    NSVGattrib* attr = nsvg__getAttr(p);
    float scale = 1.0f;
    NSVGshape* shape;
    NSVGpath* path;
    char shape_fill_flag = 0;
    char shape_stroke_flag = 0;

    shape = (NSVGshape*)NANOSVG_MALLOC(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    memset(shape, 0, sizeof(NSVGshape));

    shape->fill.color = shape->fill.color | 0xFF000000;
    shape->opacity = 1;
    shape->fill_opacity = 1;
    shape->stroke_opacity = 1;

    NSVGstyle* style = NULL, * cur = p->style;

    memcpy(shape->id, attr->id, sizeof shape->id);
    memcpy(shape->xform, attr->xform, sizeof shape->xform);
    scale = nsvg__getAverageScale(attr->xform);

    //Set attribute, not link "class" tag.
    // to do
    if (attr->clipPath[0] != 0) {
        NSVGclipPathData* temp_clip_path = nsvg__findClipPathData(p, attr->clipPath);
        shape->clipPath = temp_clip_path;
    }
    if (attr->fillGradient[0] != 0)
        memcpy(shape->fillGradient, attr->fillGradient, sizeof shape->fillGradient);
    if (attr->fillPattern[0] != 0)
        memcpy(shape->fillPattern, attr->fillPattern, sizeof shape->fillPattern);
    if (attr->strokeGradient[0] != 0)
        memcpy(shape->strokeGradient, attr->strokeGradient, sizeof shape->strokeGradient);

    if (attr->fillFlag) {
        shape_fill_flag = 1;

        if (attr->fillRule)
            shape->fillRule = attr->fillRule;
        if (attr->opacity)
            shape->opacity = attr->opacity;
        if (attr->fillOpacity)
            shape->fill_opacity = attr->fillOpacity;

        if (attr->hasFill == 0) {
            shape->fill.type = NSVG_PAINT_NONE;
        }
        else if (attr->hasFill == 1) {
            shape->fill.type = NSVG_PAINT_COLOR;
            shape->fill.color = attr->fillColor;
            shape->fill.color |= (unsigned int)(attr->fillOpacity * 255) << 24;
        }
        else if (attr->hasFill == 2) {
            shape->fill.type = NSVG_PAINT_UNDEF;
        }
    }

    if (attr->strokeFlag) {
        shape_stroke_flag = 1;

        if (attr->strokeWidth)
            shape->strokeWidth = attr->strokeWidth * scale;
        if (attr->strokeDashOffset)
            shape->strokeDashOffset = attr->strokeDashOffset * scale;
        if (attr->strokeLineJoin)
            shape->strokeLineJoin = attr->strokeLineJoin;
        if (attr->strokeLineCap)
            shape->strokeLineCap = attr->strokeLineCap;
        if (attr->miterLimit)
            shape->miterLimit = attr->miterLimit;
        if (attr->strokeOpacity)
            shape->stroke_opacity = attr->strokeOpacity;
        if (attr->strokeDashCount) {
            shape->strokeDashCount = (char)attr->strokeDashCount;
            for (int i = 0; i < attr->strokeDashCount; i++)
                shape->strokeDashArray[i] = attr->strokeDashArray[i] * scale;
        }

        if (attr->hasStroke == 0) {
            shape->stroke.type = NSVG_PAINT_NONE;
        }
        else if (attr->hasStroke == 1) {
            shape->stroke.type = NSVG_PAINT_COLOR;
            shape->stroke.color = attr->strokeColor;
            shape->stroke.color |= (unsigned int)(attr->strokeOpacity * 255) << 24;
        }
        else if (attr->hasStroke == 2) {
            shape->stroke.type = NSVG_PAINT_UNDEF;
        }
    }

    //Set animate
    if (p->animateFlag) {
        NSVGanimate* animate;
        animate = (NSVGanimate*)NANOSVG_MALLOC(sizeof(NSVGanimate));
        if (animate != NULL) {
            memset(animate, 0, sizeof(NSVGanimate));

            memcpy(animate->attributeName, p->animateData->attributeName, sizeof animate->attributeName);
            memcpy(animate->attributeType, p->animateData->attributeType, sizeof animate->attributeType);
            memcpy(animate->transformBox, attr->transformBox, sizeof animate->transformBox);
            memcpy(animate->transformOrigin, attr->transformOrigin, sizeof animate->transformOrigin);

            if (strcmp(p->animateData->type, "translate") == 0){
                animate->type = TRANSLATE;
            } else if (strcmp(p->animateData->type, "rotate") == 0){
                animate->type = ROTATE;
            } else if (strcmp(p->animateData->type, "scale") == 0){
                animate->type = SCALE;
            }

            for (int i = 0; i < 3; i++)
                animate->form[i] = p->animateData->form[i];
            for (int i = 0; i < 3; i++)
                animate->to[i] = p->animateData->to[i];

            animate->begin = p->animateData->begin;
            animate->dur = p->animateData->dur;
            animate->repeatCount = p->animateData->repeatCount;

            shape->animateFlag = 1;
            shape->animate = animate;
        }
    }

    //Set attribute, link "class" tag.
    for (int i = class_nums_count - 1; i >= 0; i--)
    {
        while (cur) {
            if (strcmp(cur->name, p->class_name[i]) == 0) {
                style = cur;
                break;
            }
            cur = cur->next;
        }
        if (!style)
            continue;

        if (!shape->fillGradient[0] && style->fillGradient[0] != 0)
            memcpy(shape->fillGradient, style->fillGradient, sizeof shape->fillGradient);
        if (!shape->fillPattern[0] && style->fillPattern[0] != 0)
            memcpy(shape->fillPattern, style->fillPattern, sizeof shape->fillPattern);

        if (!shape->strokeWidth && style->strokeWidth != 0)
            shape->strokeWidth = style->strokeWidth * scale;
        if (!shape->strokeDashOffset && style->strokeDashOffset != 0)
            shape->strokeDashOffset = style->strokeDashOffset * scale;
        if (!shape->strokeLineJoin && style->strokeLineJoin != 0)
            shape->strokeLineJoin = style->strokeLineJoin;
        if (!shape->strokeLineCap && style->strokeLineCap != 0)
            shape->strokeLineCap = style->strokeLineCap;
        if (!shape->miterLimit && style->miterLimit != 0)
            shape->miterLimit = style->miterLimit;
        if (!shape->fillRule && style->fillRule != 0)
            shape->fillRule = style->fillRule;

        if (shape->opacity == 1 && style->opacity != 0)
            shape->opacity = style->opacity;
        if (shape->fill_opacity == 1 && style->fillOpacity != 0)
            shape->fill_opacity = style->fillOpacity;
        if (shape->stroke_opacity == 1 && style->strokeOpacity != 0)
            shape->stroke_opacity = style->strokeOpacity;

        if (!shape->mediaFlag && style->mediaFlag != 0) {
            shape->mediaFlag = style->mediaFlag;
            shape->minWidth = style->minWidth;
            shape->minHeight = style->minHeight;
            shape->maxWidth = style->maxWidth;
            shape->maxHeight = style->maxHeight;
        }

        if (!shape->blend_flag && style->mixBlendMode[0] != 0) {
            shape->blend_flag = 1;
            if (strncmp(style->mixBlendMode, "multiply", 8) == 0)
                shape->blendMode = VG_MIXBLENDMODE_MULTIPLY;
            else if (strncmp(style->mixBlendMode, "screen", 8) == 0)
                shape->blendMode = VG_MIXBLENDMODE_SCREEN;
        }

        if (!shape_fill_flag && style->fillFlag) {
            shape_fill_flag = 1;
            if (style->hasFill == 0) {
                shape->fill.type = NSVG_PAINT_NONE;
            }
            else if (style->hasFill == 1) {
                shape->fill.type = NSVG_PAINT_COLOR;
                shape->fill.color = style->fillColor;
                shape->fill.color |= (unsigned int)(style->fillOpacity * 255) << 24;
            }
            else if (style->hasFill == 2) {
                shape->fill.type = NSVG_PAINT_UNDEF;
            }
        }

        if (!shape_stroke_flag && style->strokeFlag) {
            shape_stroke_flag = 1;
            if (shape->stroke.type != NSVG_PAINT_COLOR && style->hasStroke) {
                if (style->hasStroke == 1) {
                    shape->stroke.type = NSVG_PAINT_COLOR;
                    shape->stroke.color = style->strokeColor;
                    shape->stroke.color |= (unsigned int)(style->strokeOpacity * 255) << 24;
                }
                else if (style->hasStroke == 2)
                    shape->stroke.type = NSVG_PAINT_UNDEF;
            }
        }

        style = NULL;
        cur = p->style;
    }

    if (shape_fill_flag == 0)
        shape->fill.type = NSVG_PAINT_COLOR;


    shape->paths = p->plist;
    p->plist = NULL;

    // Calculate shape bounds
    shape->bounds[0] = shape->paths->bounds[0];
    shape->bounds[1] = shape->paths->bounds[1];
    shape->bounds[2] = shape->paths->bounds[2];
    shape->bounds[3] = shape->paths->bounds[3];
    for (path = shape->paths->next; path != NULL; path = path->next) {
        shape->bounds[0] = nsvg__minf(shape->bounds[0], path->bounds[0]);
        shape->bounds[1] = nsvg__minf(shape->bounds[1], path->bounds[1]);
        shape->bounds[2] = nsvg__maxf(shape->bounds[2], path->bounds[2]);
        shape->bounds[3] = nsvg__maxf(shape->bounds[3], path->bounds[3]);
    }

    // Set flags
    shape->flags = 1;

    if (!p->patternFlag) {
        // Add to tail
        if (p->image->shapes == NULL)
            p->image->shapes = shape;
        else
            p->shapesTail->next = shape;
        p->shapesTail = shape;
    }
    else {
        if (p->patterns->shape == NULL)
            p->patterns->shape = shape;
    }

    return;

error:
    if (shape) NANOSVG_FREE(shape);
}

static void nsvg__addDefsImageShape(NSVGparser* p, NSVGdefsImage* image)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    NSVGshape* shape;

    shape = (NSVGshape*)NANOSVG_MALLOC(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    memset(shape, 0, sizeof(NSVGshape));

    memcpy(shape->id, attr->id, sizeof shape->id);
    shape->fillRule = attr->fillRule;
    shape->opacity = attr->opacity == 0 ? 1 : attr->opacity;
    shape->fill_opacity = attr->fillOpacity == 0 ? 1 : attr->fillOpacity;
    shape->stroke_opacity = attr->strokeOpacity == 0 ? 1 : attr->strokeOpacity;
    shape->fill.type = NSVG_PAINT_IMAGE;

    NSVGdefsImage* shapeImage = (NSVGdefsImage*)NANOSVG_MALLOC(sizeof(NSVGdefsImage));
    memcpy(shapeImage, image, sizeof(NSVGdefsImage));
    shape->fill.image = shapeImage;

    // Set flags
    shape->flags = 1;

    if (!p->patternFlag) {
        // Add to tail
        if (p->image->shapes == NULL)
            p->image->shapes = shape;
        else
            p->shapesTail->next = shape;
        p->shapesTail = shape;
    }
    else {
        if (p->patterns->shape == NULL)
            p->patterns->shape = shape;
    }

    return;

error:
    if (shape) NANOSVG_FREE(shape);
}

static void nsvg__addShape(NSVGparser* p)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    float scale = 1.0f;
    NSVGshape* shape;
    NSVGpath* path;
    int i;

    if (p->plist == NULL)
        return;

    if (p->clipPathFlag)
    {
        if (p->clipPaths->path == NULL)
            p->clipPaths->path = p->plist;
        else {
            NSVGpath* tem_path = p->clipPaths->path;
            while (tem_path->next)
                tem_path = tem_path->next;
            tem_path->next = p->plist;
        }
        p->plist = NULL;
        return;
    }

    shape = (NSVGshape*)NANOSVG_MALLOC(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    memset(shape, 0, sizeof(NSVGshape));

    shape->paths = p->plist;
    p->plist = NULL;

    if (attr->clipPath[0] != 0) {
        NSVGclipPathData* temp_clip_path = nsvg__findClipPathData(p, attr->clipPath);
        shape->clipPath = temp_clip_path;
        shape->pathType = NSVG_CLIP_PATH;
    }

    memcpy(shape->id, attr->id, sizeof shape->id);
    memcpy(shape->fillGradient, attr->fillGradient, sizeof shape->fillGradient);
    memcpy(shape->fillPattern, attr->fillPattern, sizeof shape->fillPattern);
    memcpy(shape->strokeGradient, attr->strokeGradient, sizeof shape->strokeGradient);
    memcpy(shape->xform, attr->xform, sizeof shape->xform);
    scale = nsvg__getAverageScale(attr->xform);
    shape->strokeWidth = attr->strokeWidth * scale;
    shape->strokeDashOffset = attr->strokeDashOffset * scale;
    shape->strokeDashCount = (char)attr->strokeDashCount;
    for (i = 0; i < attr->strokeDashCount; i++)
        shape->strokeDashArray[i] = attr->strokeDashArray[i] * scale;
    shape->strokeLineJoin = attr->strokeLineJoin;
    shape->strokeLineCap = attr->strokeLineCap;
    shape->miterLimit = attr->miterLimit;
    shape->fillRule = attr->fillRule;
    shape->opacity = attr->opacity == 0 ? 1 : attr->opacity;
    shape->fill_opacity = attr->fillOpacity == 0 ? 1 : attr->fillOpacity;
    shape->stroke_opacity = attr->strokeOpacity == 0 ? 1 : attr->strokeOpacity;

    // Calculate shape bounds
    shape->bounds[0] = shape->paths->bounds[0];
    shape->bounds[1] = shape->paths->bounds[1];
    shape->bounds[2] = shape->paths->bounds[2];
    shape->bounds[3] = shape->paths->bounds[3];
    for (path = shape->paths->next; path != NULL; path = path->next) {
        shape->bounds[0] = nsvg__minf(shape->bounds[0], path->bounds[0]);
        shape->bounds[1] = nsvg__minf(shape->bounds[1], path->bounds[1]);
        shape->bounds[2] = nsvg__maxf(shape->bounds[2], path->bounds[2]);
        shape->bounds[3] = nsvg__maxf(shape->bounds[3], path->bounds[3]);
    }

    // Set fill
    if (attr->hasFill == 0) {
        shape->fill.type = NSVG_PAINT_NONE;
    } else if (attr->hasFill == 1 && attr->fillOpacity != 0) {
        shape->fill.type = NSVG_PAINT_COLOR;
        shape->fill.color = attr->fillColor;
        shape->fill.color |= (unsigned int)(attr->fillOpacity*255) << 24;
    } else if (attr->hasFill == 2) {
        shape->fill.type = NSVG_PAINT_UNDEF;
    }

    // Set animate
    if (p->animateFlag)
    {
        NSVGanimate* animate;
        animate = (NSVGanimate*)NANOSVG_MALLOC(sizeof(NSVGanimate));
        memset(animate, 0, sizeof(NSVGanimate));

        memcpy(animate->attributeName, p->animateData->attributeName, sizeof animate->attributeName);
        memcpy(animate->attributeType, p->animateData->attributeType, sizeof animate->attributeType);
        memcpy(animate->transformBox, attr->transformBox, sizeof animate->transformBox);
        memcpy(animate->transformOrigin, attr->transformOrigin, sizeof animate->transformOrigin);

        if (strcmp(p->animateData->type, "translate") == 0) {
            animate->type = TRANSLATE;
        } else if (strcmp(p->animateData->type, "rotate") == 0) {
            animate->type = ROTATE;
        } else if (strcmp(p->animateData->type, "scale") == 0) {
            animate->type = SCALE;
        }

        for (int i = 0; i < 3; i++)
            animate->form[i] = p->animateData->form[i];
        for (int i = 0; i < 3; i++)
            animate->to[i] = p->animateData->to[i];

        animate->begin = p->animateData->begin;
        animate->dur = p->animateData->dur;
        animate->repeatCount = p->animateData->repeatCount;

        shape->animateFlag = 1;
        shape->animate = animate;
    }

    // Set stroke
    if (attr->hasStroke == 0) {
        shape->stroke.type = NSVG_PAINT_NONE;
    } else if (attr->hasStroke == 1) {
        shape->stroke.type = NSVG_PAINT_COLOR;
        shape->stroke.color = attr->strokeColor;
        shape->stroke.color |= (unsigned int)(attr->strokeOpacity*255) << 24;
    } else if (attr->hasStroke == 2) {
        shape->stroke.type = NSVG_PAINT_UNDEF;
    }

    // Set flags
    shape->flags = (attr->visible ? NSVG_FLAGS_VISIBLE : 0x00);

    if (!p->patternFlag) {
        shape->pathType = NSVG_PATH;
        // Add to tail
        if (p->image->shapes == NULL)
            p->image->shapes = shape;
        else
            p->shapesTail->next = shape;
        p->shapesTail = shape;
    }
    else {
        shape->pathType = NSVG_PATTERN_PATH;
        if (p->patterns->shape == NULL)
            p->patterns->shape = shape;
        else {
            NSVGshape* tem_shape = p->patterns->shape;
            while (tem_shape->next)
                tem_shape = tem_shape->next;
            tem_shape->next = shape;
        }
    }

    return;

error:
    if (shape) NANOSVG_FREE(shape);
}

static void nsvg__addTextShape(NSVGparser* p)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    float scale = 1.0f;
    NSVGshape* shape;
    NSVGtextData* data = p->text;
    NSVGtext* text;
    int i;

    shape = (NSVGshape*)NANOSVG_MALLOC(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    memset(shape, 0, sizeof(NSVGshape));

    text = (NSVGtext*)NANOSVG_MALLOC(sizeof(NSVGtext));
    if (text == NULL) goto error;

    scale = nsvg__getAverageScale(attr->xform);
    shape->opacity = attr->opacity;
    shape->strokeWidth = attr->strokeWidth * scale;
    shape->strokeDashOffset = attr->strokeDashOffset * scale;
    shape->strokeDashCount = (char)attr->strokeDashCount;
    for (i = 0; i < attr->strokeDashCount; i++)
        shape->strokeDashArray[i] = attr->strokeDashArray[i] * scale;
    shape->strokeLineJoin = attr->strokeLineJoin;
    shape->strokeLineCap = attr->strokeLineCap;
    shape->miterLimit = attr->miterLimit;
    shape->fillRule = attr->fillRule;
    shape->fill_opacity = attr->fillOpacity;
    shape->fill.type = NSVG_PAINT_TEXT;
    shape->fontSize = attr->fontSize;
    shape->fill.color = attr->fillColor | 0xFF000000;
    shape->fill.color |= (unsigned int)(shape->fill_opacity * 255) << 24;
    text->x = data->x.value;
    text->y = data->y.value;
    text->dx = data->dx.value;
    text->dy = data->dy.value;
    text->rotate = data->rotate.value;
    text->textLength = data->textLength.value;
    text->fontSize = data->fontSize.value == 0 ? shape->fontSize : data->fontSize.value;
    strncpy(text->content, data->content, 128);
    shape->fill.text = text;

    shape->flags = (attr->visible ? NSVG_FLAGS_VISIBLE : 0x00);

    if (p->image->shapes == NULL)
        p->image->shapes = shape;
    else
        p->shapesTail->next = shape;
    p->shapesTail = shape;

    return;

error:
    if (shape) NANOSVG_FREE(shape);
}

static void nsvg__addTextpathShape(NSVGparser* p)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    float scale = 1.0f;
    NSVGshape* shape;
    //NSVGtextData* data = p->text;
    NSVGtext* text;

    shape = (NSVGshape*)NANOSVG_MALLOC(sizeof(NSVGshape));
    if (shape == NULL) goto error;
    memset(shape, 0, sizeof(NSVGshape));

    text = (NSVGtext*)NANOSVG_MALLOC(sizeof(NSVGtext));
    if (text == NULL) goto error;

    NSVGtextpathData* textpath = p->textpath;
    NSVGdefspath* cur = p->defspath;

    /* To find xlink:href atrribute. */
    while (cur) {
        if (strcmp(cur->name, textpath->ref) == 0) {
            text->x = cur->x;
            text->y = cur->y;
            break;
        }
        cur = cur->next;
    }
    if (!cur) {
        NANOSVG_FREE(text);
        goto error;
    }

    /* To find class atrribute. */
    NSVGstyle* style = NULL;
    char classname[64];
    if (p->textFlag.textClassFlag)
    {
        strcpy(classname, p->text->class_name);
        if (classname[0]) {
            NSVGstyle* cur = p->style;
            while (cur) {
                if (strcmp(cur->name, classname) == 0) {
                    style = cur;
                    break;
                }
                cur = cur->next;
            }
            if (style) {
                text->fontSize = style->fontsize.value;
                shape->fill.color = style->fillColor;
                shape->fill_opacity = style->fillOpacity;
                if (style->hasStroke == 1) {
                    text->hasStroke = 1;
                    shape->stroke_opacity = style->strokeOpacity == 0 ? 1 : style->strokeOpacity;
                    shape->strokeWidth = style->strokeWidth;
                    shape->stroke.color = style->strokeColor;
                    shape->stroke.color |= (unsigned int)(shape->stroke_opacity * 255) << 24;
                    shape->miterLimit = style->miterLimit;
                }
            }
        }
    }

    if (p->textFlag.textpathClassFlag)
        strcpy(classname, textpath->class_name);
    else
        strcpy(classname, p->text->class_name);

    if (classname[0]) {
        NSVGstyle* cur = p->style;
        while (cur) {
            if (strcmp(cur->name, classname) == 0) {
                style = cur;
                break;
            }
            cur = cur->next;
        }
        if (style) {
            text->fontSize = style->fontsize.value > 0? style->fontsize.value : text->fontSize;
            shape->fill.color = style->fillColor > 0 ? style->fillColor : shape->fill.color;
            shape->fill_opacity = style->fillOpacity > 0 ? style->fillOpacity: shape->fill_opacity;
            if (style->hasStroke == 1) {
                text->hasStroke = 1;
                shape->stroke_opacity = style->strokeOpacity > 0 ? style->strokeOpacity : shape->stroke_opacity;
                shape->strokeWidth = style->strokeWidth > 0 ? style->strokeWidth: shape->strokeWidth;
                shape->stroke.color = style->strokeColor > 0 ? style->strokeColor: shape->stroke.color;
                shape->stroke.color |= (unsigned int)(shape->stroke_opacity * 255) << 24;
                shape->miterLimit = style->miterLimit > 0 ? style->miterLimit: style->miterLimit;
            }
        }
    }

    shape->fill.type = NSVG_PAINT_TEXT;
    scale = nsvg__getAverageScale(attr->xform);
    shape->strokeWidth = shape->strokeWidth != 0 ? shape->strokeWidth : attr->strokeWidth * scale;
    shape->fillRule = attr->fillRule;
    shape->opacity = shape->opacity ? shape->opacity : attr->opacity;
    shape->fontSize = shape->fontSize ? shape->fontSize : attr->fontSize;
    shape->fill.color = shape->fill.color ? shape->fill.color : attr->fillColor | 0xFF000000;
    shape->fill.color |= (unsigned int)(shape->fill_opacity * 255) << 24;
    text->textLength = textpath->textLength.value;
    strncpy(text->content, textpath->content, 128);
    shape->flags = 1;

    shape->fill.text = text;

    if (p->image->shapes == NULL)
        p->image->shapes = shape;
    else
        p->shapesTail->next = shape;
    p->shapesTail = shape;

    return;

error:
    if (shape) NANOSVG_FREE(shape);
}

static void nsvg__addPath(NSVGparser* p, char closed)
{
    NSVGattrib* attr = nsvg__getAttr(p);
    NSVGpath* path = NULL;
    float bounds[4];
    float* curve;
    int i;

    if (p->npts < 4)
        return;

    if (closed)
        nsvg__lineTo(p, p->pts[0], p->pts[1]);

    // Expect 1 + N*3 points (N = number of cubic bezier segments).
    if ((p->npts % 3) != 1)
        return;

    path = (NSVGpath*)NANOSVG_MALLOC(sizeof(NSVGpath));
    if (path == NULL) goto error;
    memset(path, 0, sizeof(NSVGpath));

    path->pts = (float*)NANOSVG_MALLOC(p->npts*2*sizeof(float));
    if (path->pts == NULL) goto error;
    path->closed = closed;
    path->npts = p->npts;

    // Transform path.
    for (i = 0; i < p->npts; ++i)
        nsvg__xformPoint(&path->pts[i*2], &path->pts[i*2+1], p->pts[i*2], p->pts[i*2+1], attr->xform);

    // Find bounds
    for (i = 0; i < path->npts-1; i += 3) {
        curve = &path->pts[i*2];
        nsvg__curveBounds(bounds, curve);
        if (i == 0) {
            path->bounds[0] = bounds[0];
            path->bounds[1] = bounds[1];
            path->bounds[2] = bounds[2];
            path->bounds[3] = bounds[3];
        } else {
            path->bounds[0] = nsvg__minf(path->bounds[0], bounds[0]);
            path->bounds[1] = nsvg__minf(path->bounds[1], bounds[1]);
            path->bounds[2] = nsvg__maxf(path->bounds[2], bounds[2]);
            path->bounds[3] = nsvg__maxf(path->bounds[3], bounds[3]);
        }
    }

    path->next = p->plist;
    p->plist = path;

    return;

error:
    if (path != NULL) {
        if (path->pts != NULL) NANOSVG_FREE(path->pts);
        NANOSVG_FREE(path);
    }
}

static double nsvg_pow10(long num)
{
    double x = 1.0;
    while (num-- > 0)
        x *= 10.0;

    return x;
}

// We roll our own string to float because the std library one uses locale and messes things up.
static double nsvg__atof(const char* s)
{
    char* cur = (char*)s;
    char* end = NULL;
    double res = 0.0, sign = 1.0;
    long long intPart = 0, fracPart = 0;
    char hasIntPart = 0, hasFracPart = 0;

    // Parse optional sign
    if (*cur == '+') {
        cur++;
    } else if (*cur == '-') {
        sign = -1;
        cur++;
    }

    // Parse integer part
    if (nsvg__isdigit(*cur)) {
        // Parse digit sequence
        intPart = strtoll(cur, &end, 10);
        if (cur != end) {
            res = (double)intPart;
            hasIntPart = 1;
            cur = end;
        }
    }

    // Parse fractional part.
    if (*cur == '.') {
        cur++; // Skip '.'
        if (nsvg__isdigit(*cur)) {
            // Parse digit sequence
            fracPart = strtoll(cur, &end, 10);
            if (cur != end) {
                res += (double)fracPart / nsvg_pow10(end - cur); // pow(10.0, (double)(end - cur));
                hasFracPart = 1;
                cur = end;
            }
        }
    }

    // A valid number should have integer or fractional part.
    if (!hasIntPart && !hasFracPart)
        return 0.0;

    // Parse optional exponent
    if (*cur == 'e' || *cur == 'E') {
        long expPart = 0;
        cur++; // skip 'E'
        expPart = strtol(cur, &end, 10); // Parse digit sequence with sign
        if (cur != end) {
            res *= nsvg_pow10(expPart); //pow(10.0, (double)expPart);
        }
    }

    return res * sign;
}


static const char* nsvg__parseNumber(const char* s, char* it, const int size)
{
    const int last = size-1;
    int i = 0;

    // sign
    if (*s == '-' || *s == '+') {
        if (i < last) it[i++] = *s;
        s++;
    }
    // integer part
    while (*s && nsvg__isdigit(*s)) {
        if (i < last) it[i++] = *s;
        s++;
    }
    if (*s == '.') {
        // decimal point
        if (i < last) it[i++] = *s;
        s++;
        // fraction part
        while (*s && nsvg__isdigit(*s)) {
            if (i < last) it[i++] = *s;
            s++;
        }
    }
    // exponent
    if ((*s == 'e' || *s == 'E') && (s[1] != 'm' && s[1] != 'x')) {
        if (i < last) it[i++] = *s;
        s++;
        if (*s == '-' || *s == '+') {
            if (i < last) it[i++] = *s;
            s++;
        }
        while (*s && nsvg__isdigit(*s)) {
            if (i < last) it[i++] = *s;
            s++;
        }
    }
    it[i] = '\0';

    return s;
}

static const char* nsvg__getNextPathItemWhenArcFlag(const char* s, char* it)
{
    it[0] = '\0';
    while (*s && (nsvg__isspace(*s) || *s == ',')) s++;
    if (!*s) return s;
    if (*s == '0' || *s == '1') {
        it[0] = *s++;
        it[1] = '\0';
        return s;
    }
    return s;
}

static const char* nsvg__getNextPathItem(const char* s, char* it)
{
    it[0] = '\0';
    // Skip white spaces and commas
    while (*s && (nsvg__isspace(*s) || *s == ',')) s++;
    if (!*s) return s;
    if (*s == '-' || *s == '+' || *s == '.' || nsvg__isdigit(*s)) {
        s = nsvg__parseNumber(s, it, 64);
    } else {
        // Parse command
        it[0] = *s++;
        it[1] = '\0';
        return s;
    }

    return s;
}

static unsigned int nsvg__parseColorHex(const char* str)
{
    unsigned int r=0, g=0, b=0;
    if (sscanf(str, "#%2x%2x%2x", &r, &g, &b) == 3 )        // 2 digit hex
        return NSVG_RGB(r, g, b);
    if (sscanf(str, "#%1x%1x%1x", &r, &g, &b) == 3 )        // 1 digit hex, e.g. #abc -> 0xccbbaa
        return NSVG_RGB(r*17, g*17, b*17);            // same effect as (r<<4|r), (g<<4|g), ..
    return NSVG_RGB(128, 128, 128);
}

static unsigned int nsvg__paraseColorStyle(const char* str)
{
    unsigned int r = 0, g = 0, b = 0;
    if (sscanf(str, "rgba(%u,%u,%u", &r, &g, &b) == 3)
        return NSVG_RGB(r, g, b);
    return NSVG_RGB(128, 128, 128);
}

static float nsvg__paraseOpacityStyle(const char* str)
{
    unsigned int r = 0, g = 0, b = 0;
    float a;
    if (sscanf(str, "rgba(%u,%u,%u,%f", &r, &g, &b, &a) == 4)
        return a;
    return 1.0;
}

// Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
// This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
// for backwards compatibility. Note: other image viewers return black instead.

static unsigned int nsvg__parseColorRGB(const char* str)
{
    int i;
    unsigned int rgbi[3];
    float rgbf[3];
    // try decimal integers first
    if (sscanf(str, "rgb(%u, %u, %u)", &rgbi[0], &rgbi[1], &rgbi[2]) != 3) {
        // integers failed, try percent values (float, locale independent)
        const char delimiter[3] = {',', ',', ')'};
        str += 4; // skip "rgb("
        for (i = 0; i < 3; i++) {
            while (*str && (nsvg__isspace(*str))) str++;     // skip leading spaces
            if (*str == '+') str++;                // skip '+' (don't allow '-')
            if (!*str) break;
            rgbf[i] = nsvg__atof(str);

            // Note 1: it would be great if nsvg__atof() returned how many
            // bytes it consumed but it doesn't. We need to skip the number,
            // the '%' character, spaces, and the delimiter ',' or ')'.

            // Note 2: The following code does not allow values like "33.%",
            // i.e. a decimal point w/o fractional part, but this is consistent
            // with other image viewers, e.g. firefox, chrome, eog, gimp.

            while (*str && nsvg__isdigit(*str)) str++;        // skip integer part
            if (*str == '.') {
                str++;
                if (!nsvg__isdigit(*str)) break;        // error: no digit after '.'
                while (*str && nsvg__isdigit(*str)) str++;    // skip fractional part
            }
            if (*str == '%') str++; else break;
            while (nsvg__isspace(*str)) str++;
            if (*str == delimiter[i]) str++;
            else break;
        }
        if (i == 3) {
            rgbi[0] = roundf(rgbf[0] * 2.55f);
            rgbi[1] = roundf(rgbf[1] * 2.55f);
            rgbi[2] = roundf(rgbf[2] * 2.55f);
        } else {
            rgbi[0] = rgbi[1] = rgbi[2] = 128;
        }
    }
    // clip values as the CSS spec requires
    for (i = 0; i < 3; i++) {
        if (rgbi[i] > 255) rgbi[i] = 255;
    }
    return NSVG_RGB(rgbi[0], rgbi[1], rgbi[2]);
}

typedef struct NSVGNamedColor {
    const char* name;
    unsigned int color;
} NSVGNamedColor;

static const NSVGNamedColor nsvg__colors[] = {

    { "red", NSVG_RGB(255, 0, 0) },
    { "green", NSVG_RGB( 0, 128, 0) },
    { "blue", NSVG_RGB( 0, 0, 255) },
    { "yellow", NSVG_RGB(255, 255, 0) },
    { "cyan", NSVG_RGB( 0, 255, 255) },
    { "magenta", NSVG_RGB(255, 0, 255) },
    { "black", NSVG_RGB( 0, 0, 0) },
    { "grey", NSVG_RGB(128, 128, 128) },
    { "gray", NSVG_RGB(128, 128, 128) },
    { "white", NSVG_RGB(255, 255, 255) },

#ifdef NANOSVG_ALL_COLOR_KEYWORDS
    { "aliceblue", NSVG_RGB(240, 248, 255) },
    { "antiquewhite", NSVG_RGB(250, 235, 215) },
    { "aqua", NSVG_RGB( 0, 255, 255) },
    { "aquamarine", NSVG_RGB(127, 255, 212) },
    { "azure", NSVG_RGB(240, 255, 255) },
    { "beige", NSVG_RGB(245, 245, 220) },
    { "bisque", NSVG_RGB(255, 228, 196) },
    { "blanchedalmond", NSVG_RGB(255, 235, 205) },
    { "blueviolet", NSVG_RGB(138, 43, 226) },
    { "brown", NSVG_RGB(165, 42, 42) },
    { "burlywood", NSVG_RGB(222, 184, 135) },
    { "cadetblue", NSVG_RGB( 95, 158, 160) },
    { "chartreuse", NSVG_RGB(127, 255, 0) },
    { "chocolate", NSVG_RGB(210, 105, 30) },
    { "coral", NSVG_RGB(255, 127, 80) },
    { "cornflowerblue", NSVG_RGB(100, 149, 237) },
    { "cornsilk", NSVG_RGB(255, 248, 220) },
    { "crimson", NSVG_RGB(220, 20, 60) },
    { "darkblue", NSVG_RGB( 0, 0, 139) },
    { "darkcyan", NSVG_RGB( 0, 139, 139) },
    { "darkgoldenrod", NSVG_RGB(184, 134, 11) },
    { "darkgray", NSVG_RGB(169, 169, 169) },
    { "darkgreen", NSVG_RGB( 0, 100, 0) },
    { "darkgrey", NSVG_RGB(169, 169, 169) },
    { "darkkhaki", NSVG_RGB(189, 183, 107) },
    { "darkmagenta", NSVG_RGB(139, 0, 139) },
    { "darkolivegreen", NSVG_RGB( 85, 107, 47) },
    { "darkorange", NSVG_RGB(255, 140, 0) },
    { "darkorchid", NSVG_RGB(153, 50, 204) },
    { "darkred", NSVG_RGB(139, 0, 0) },
    { "darksalmon", NSVG_RGB(233, 150, 122) },
    { "darkseagreen", NSVG_RGB(143, 188, 143) },
    { "darkslateblue", NSVG_RGB( 72, 61, 139) },
    { "darkslategray", NSVG_RGB( 47, 79, 79) },
    { "darkslategrey", NSVG_RGB( 47, 79, 79) },
    { "darkturquoise", NSVG_RGB( 0, 206, 209) },
    { "darkviolet", NSVG_RGB(148, 0, 211) },
    { "deeppink", NSVG_RGB(255, 20, 147) },
    { "deepskyblue", NSVG_RGB( 0, 191, 255) },
    { "dimgray", NSVG_RGB(105, 105, 105) },
    { "dimgrey", NSVG_RGB(105, 105, 105) },
    { "dodgerblue", NSVG_RGB( 30, 144, 255) },
    { "firebrick", NSVG_RGB(178, 34, 34) },
    { "floralwhite", NSVG_RGB(255, 250, 240) },
    { "forestgreen", NSVG_RGB( 34, 139, 34) },
    { "fuchsia", NSVG_RGB(255, 0, 255) },
    { "gainsboro", NSVG_RGB(220, 220, 220) },
    { "ghostwhite", NSVG_RGB(248, 248, 255) },
    { "gold", NSVG_RGB(255, 215, 0) },
    { "goldenrod", NSVG_RGB(218, 165, 32) },
    { "greenyellow", NSVG_RGB(173, 255, 47) },
    { "honeydew", NSVG_RGB(240, 255, 240) },
    { "hotpink", NSVG_RGB(255, 105, 180) },
    { "indianred", NSVG_RGB(205, 92, 92) },
    { "indigo", NSVG_RGB( 75, 0, 130) },
    { "ivory", NSVG_RGB(255, 255, 240) },
    { "khaki", NSVG_RGB(240, 230, 140) },
    { "lavender", NSVG_RGB(230, 230, 250) },
    { "lavenderblush", NSVG_RGB(255, 240, 245) },
    { "lawngreen", NSVG_RGB(124, 252, 0) },
    { "lemonchiffon", NSVG_RGB(255, 250, 205) },
    { "lightblue", NSVG_RGB(173, 216, 230) },
    { "lightcoral", NSVG_RGB(240, 128, 128) },
    { "lightcyan", NSVG_RGB(224, 255, 255) },
    { "lightgoldenrodyellow", NSVG_RGB(250, 250, 210) },
    { "lightgray", NSVG_RGB(211, 211, 211) },
    { "lightgreen", NSVG_RGB(144, 238, 144) },
    { "lightgrey", NSVG_RGB(211, 211, 211) },
    { "lightpink", NSVG_RGB(255, 182, 193) },
    { "lightsalmon", NSVG_RGB(255, 160, 122) },
    { "lightseagreen", NSVG_RGB( 32, 178, 170) },
    { "lightskyblue", NSVG_RGB(135, 206, 250) },
    { "lightslategray", NSVG_RGB(119, 136, 153) },
    { "lightslategrey", NSVG_RGB(119, 136, 153) },
    { "lightsteelblue", NSVG_RGB(176, 196, 222) },
    { "lightyellow", NSVG_RGB(255, 255, 224) },
    { "lime", NSVG_RGB( 0, 255, 0) },
    { "limegreen", NSVG_RGB( 50, 205, 50) },
    { "linen", NSVG_RGB(250, 240, 230) },
    { "maroon", NSVG_RGB(128, 0, 0) },
    { "mediumaquamarine", NSVG_RGB(102, 205, 170) },
    { "mediumblue", NSVG_RGB( 0, 0, 205) },
    { "mediumorchid", NSVG_RGB(186, 85, 211) },
    { "mediumpurple", NSVG_RGB(147, 112, 219) },
    { "mediumseagreen", NSVG_RGB( 60, 179, 113) },
    { "mediumslateblue", NSVG_RGB(123, 104, 238) },
    { "mediumspringgreen", NSVG_RGB( 0, 250, 154) },
    { "mediumturquoise", NSVG_RGB( 72, 209, 204) },
    { "mediumvioletred", NSVG_RGB(199, 21, 133) },
    { "midnightblue", NSVG_RGB( 25, 25, 112) },
    { "mintcream", NSVG_RGB(245, 255, 250) },
    { "mistyrose", NSVG_RGB(255, 228, 225) },
    { "moccasin", NSVG_RGB(255, 228, 181) },
    { "navajowhite", NSVG_RGB(255, 222, 173) },
    { "navy", NSVG_RGB( 0, 0, 128) },
    { "oldlace", NSVG_RGB(253, 245, 230) },
    { "olive", NSVG_RGB(128, 128, 0) },
    { "olivedrab", NSVG_RGB(107, 142, 35) },
    { "orange", NSVG_RGB(255, 165, 0) },
    { "orangered", NSVG_RGB(255, 69, 0) },
    { "orchid", NSVG_RGB(218, 112, 214) },
    { "palegoldenrod", NSVG_RGB(238, 232, 170) },
    { "palegreen", NSVG_RGB(152, 251, 152) },
    { "paleturquoise", NSVG_RGB(175, 238, 238) },
    { "palevioletred", NSVG_RGB(219, 112, 147) },
    { "papayawhip", NSVG_RGB(255, 239, 213) },
    { "peachpuff", NSVG_RGB(255, 218, 185) },
    { "peru", NSVG_RGB(205, 133, 63) },
    { "pink", NSVG_RGB(255, 192, 203) },
    { "plum", NSVG_RGB(221, 160, 221) },
    { "powderblue", NSVG_RGB(176, 224, 230) },
    { "purple", NSVG_RGB(128, 0, 128) },
    { "rosybrown", NSVG_RGB(188, 143, 143) },
    { "royalblue", NSVG_RGB( 65, 105, 225) },
    { "saddlebrown", NSVG_RGB(139, 69, 19) },
    { "salmon", NSVG_RGB(250, 128, 114) },
    { "sandybrown", NSVG_RGB(244, 164, 96) },
    { "seagreen", NSVG_RGB( 46, 139, 87) },
    { "seashell", NSVG_RGB(255, 245, 238) },
    { "sienna", NSVG_RGB(160, 82, 45) },
    { "silver", NSVG_RGB(192, 192, 192) },
    { "skyblue", NSVG_RGB(135, 206, 235) },
    { "slateblue", NSVG_RGB(106, 90, 205) },
    { "slategray", NSVG_RGB(112, 128, 144) },
    { "slategrey", NSVG_RGB(112, 128, 144) },
    { "snow", NSVG_RGB(255, 250, 250) },
    { "springgreen", NSVG_RGB( 0, 255, 127) },
    { "steelblue", NSVG_RGB( 70, 130, 180) },
    { "tan", NSVG_RGB(210, 180, 140) },
    { "teal", NSVG_RGB( 0, 128, 128) },
    { "thistle", NSVG_RGB(216, 191, 216) },
    { "tomato", NSVG_RGB(255, 99, 71) },
    { "turquoise", NSVG_RGB( 64, 224, 208) },
    { "violet", NSVG_RGB(238, 130, 238) },
    { "wheat", NSVG_RGB(245, 222, 179) },
    { "whitesmoke", NSVG_RGB(245, 245, 245) },
    { "yellowgreen", NSVG_RGB(154, 205, 50) },
#endif
};

static unsigned int nsvg__parseColorName(const char* str)
{
    int i, ncolors = sizeof(nsvg__colors) / sizeof(NSVGNamedColor);

    for (i = 0; i < ncolors; i++) {
        if (strcmp(nsvg__colors[i].name, str) == 0) {
            return nsvg__colors[i].color;
        }
    }

    return NSVG_RGB(128, 128, 128);
}

static unsigned int nsvg__parseColor(const char* str)
{
    size_t len = 0;
    while(*str == ' ') ++str;
    len = strlen(str);
    if (len >= 1 && *str == '#')
        return nsvg__parseColorHex(str);
    else if (len >= 4 && str[0] == 'r' && str[1] == 'g' && str[2] == 'b' && str[3] == '(')
        return nsvg__parseColorRGB(str);
    else if (strstr(str, "rgba("))
        return nsvg__paraseColorStyle(str);
    return nsvg__parseColorName(str);
}

static float nsvg__parseOpacity(const char* str)
{
    if (!strcmp(str, "null"))
        return 1;
    float val = nsvg__atof(str);
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;
    return val;
}

static float nsvg__parseMiterLimit(const char* str)
{
    float val =  nsvg__atof(str) * (svg_scale_image_width / svg_image_width);
    if (val < 0.0f) val = 0.0f;
    return val;
}

static int nsvg__parseUnits(const char* units)
{
    if (units[0] == 'p' && units[1] == 'x')
        return NSVG_UNITS_PX;
    else if (units[0] == 'p' && units[1] == 't')
        return NSVG_UNITS_PT;
    else if (units[0] == 'p' && units[1] == 'c')
        return NSVG_UNITS_PC;
    else if (units[0] == 'm' && units[1] == 'm')
        return NSVG_UNITS_MM;
    else if (units[0] == 'c' && units[1] == 'm')
        return NSVG_UNITS_CM;
    else if (units[0] == 'i' && units[1] == 'n')
        return NSVG_UNITS_IN;
    else if (units[0] == '%')
        return NSVG_UNITS_PERCENT;
    else if (units[0] == 'e' && units[1] == 'm')
        return NSVG_UNITS_EM;
    else if (units[0] == 'e' && units[1] == 'x')
        return NSVG_UNITS_EX;
    return NSVG_UNITS_USER;
}

static int nsvg__isCoordinate(const char* s)
{
    // optional sign
    if (*s == '-' || *s == '+')
        s++;
    // must have at least one digit, or start by a dot
    return (nsvg__isdigit(*s) || *s == '.');
}

static NSVGcoordinate nsvg__parseCoordinateRaw(const char* str)
{
    NSVGcoordinate coord = {0, NSVG_UNITS_USER};
    char buf[64];
    coord.units = nsvg__parseUnits(nsvg__parseNumber(str, buf, 64));
    coord.value = nsvg__atof(buf);
    return coord;
}

static NSVGcoordinate nsvg__coord(float v, int units)
{
    NSVGcoordinate coord = {v, units};
    return coord;
}

static float nsvg__parseCoordinate(NSVGparser* p, const char* str, float orig, float length)
{
    NSVGcoordinate coord = nsvg__parseCoordinateRaw(str);
    return nsvg__convertToPixels(p, coord, orig, length);
}

static int nsvg__parseTransformArgs(const char* str, float* args, int maxNa, int* na)
{
    const char* end;
    const char* ptr;
    char it[64];

    *na = 0;
    ptr = str;
    while (*ptr && *ptr != '(') ++ptr;
    if (*ptr == 0)
        return 1;
    end = ptr;
    while (*end && *end != ')') ++end;
    if (*end == 0)
        return 1;

    while (ptr < end) {
        if (*ptr == '-' || *ptr == '+' || *ptr == '.' || nsvg__isdigit(*ptr)) {
            if (*na >= maxNa) return 0;
            ptr = nsvg__parseNumber(ptr, it, 64);
            args[(*na)++] = (float)nsvg__atof(it);
        } else {
            ++ptr;
        }
    }
    return (int)(end - str);
}


static int nsvg__parseMatrix(float* xform, const char* str)
{
    float t[6];
    int na = 0;
    int len = nsvg__parseTransformArgs(str, t, 6, &na);
    if (na != 6) return len;
    memcpy(xform, t, sizeof(float)*6);
    return len;
}

static int nsvg__parseTranslate(float* xform, const char* str)
{
    float args[2];
    float t[6];
    int na = 0;
    int len = nsvg__parseTransformArgs(str, args, 2, &na);
    if (na == 1) args[1] = 0.0;

    nsvg__xformSetTranslation(t, args[0], args[1]);
    memcpy(xform, t, sizeof(float)*6);
    return len;
}

static int nsvg__parseScale(float* xform, const char* str)
{
    float args[2];
    int na = 0;
    float t[6];
    int len = nsvg__parseTransformArgs(str, args, 2, &na);
    if (na == 1) args[1] = args[0];
    nsvg__xformSetScale(t, args[0], args[1]);
    memcpy(xform, t, sizeof(float)*6);
    return len;
}

static int nsvg__parseSkewX(float* xform, const char* str)
{
    float args[1];
    int na = 0;
    float t[6];
    int len = nsvg__parseTransformArgs(str, args, 1, &na);
    nsvg__xformSetSkewX(t, args[0]/180.0f*NSVG_PI);
    memcpy(xform, t, sizeof(float)*6);
    return len;
}

static int nsvg__parseSkewY(float* xform, const char* str)
{
    float args[1];
    int na = 0;
    float t[6];
    int len = nsvg__parseTransformArgs(str, args, 1, &na);
    nsvg__xformSetSkewY(t, args[0]/180.0f*NSVG_PI);
    memcpy(xform, t, sizeof(float)*6);
    return len;
}

static int nsvg__parseRotate(float* xform, const char* str)
{
    float args[3];
    int na = 0;
    float m[6];
    float t[6];
    int len = nsvg__parseTransformArgs(str, args, 3, &na);
    if (na == 1)
        args[1] = args[2] = 0.0f;
    nsvg__xformIdentity(m);

    if (na > 1) {
        nsvg__xformSetTranslation(t, -args[1], -args[2]);
        nsvg__xformMultiply(m, t);
    }

    nsvg__xformSetRotation(t, args[0]/180.0f*NSVG_PI);
    nsvg__xformMultiply(m, t);

    if (na > 1) {
        nsvg__xformSetTranslation(t, args[1], args[2]);
        nsvg__xformMultiply(m, t);
    }

    memcpy(xform, m, sizeof(float)*6);

    return len;
}

static void nsvg__parseTransform(float* xform, const char* str)
{
    float t[6];
    int len;
    nsvg__xformIdentity(xform);
    while (*str)
    {
        if (strncmp(str, "matrix", 6) == 0)
            len = nsvg__parseMatrix(t, str);
        else if (strncmp(str, "translate", 9) == 0)
            len = nsvg__parseTranslate(t, str);
        else if (strncmp(str, "scale", 5) == 0)
            len = nsvg__parseScale(t, str);
        else if (strncmp(str, "rotate", 6) == 0)
            len = nsvg__parseRotate(t, str);
        else if (strncmp(str, "skewX", 5) == 0)
            len = nsvg__parseSkewX(t, str);
        else if (strncmp(str, "skewY", 5) == 0)
            len = nsvg__parseSkewY(t, str);
        else{
            ++str;
            continue;
        }
        if (len != 0) {
            str += len;
        } else {
            ++str;
            continue;
        }

        nsvg__xformPremultiply(xform, t);
    }
}

static void nsvg__parseUrl(char* id, const char* str)
{
    int i = 0;
    str += 4; // "url(";
    if (*str && *str == '\'')
        str++;
    if (*str && *str == '#')
        str++;
    while (i < 63 && *str && *str != ')' && *str != '\'' ) {
        id[i] = *str++;
        i++;
    }
    id[i] = '\0';
}

static char nsvg__parseLineCap(const char* str)
{
    if (strcmp(str, "butt") == 0)
        return NSVG_CAP_BUTT;
    else if (strcmp(str, "round") == 0)
        return NSVG_CAP_ROUND;
    else if (strcmp(str, "square") == 0)
        return NSVG_CAP_SQUARE;
    // TODO: handle inherit.
    return NSVG_CAP_BUTT;
}

static char nsvg__parseLineJoin(const char* str)
{
    if (strcmp(str, "miter") == 0)
        return NSVG_JOIN_MITER;
    else if (strcmp(str, "round") == 0)
        return NSVG_JOIN_ROUND;
    else if (strcmp(str, "bevel") == 0)
        return NSVG_JOIN_BEVEL;
    // TODO: handle inherit.
    return NSVG_JOIN_MITER;
}

static char nsvg__parseFillRule(const char* str)
{
    if (strcmp(str, "nonzero") == 0)
        return NSVG_FILLRULE_NONZERO;
    else if (strcmp(str, "evenodd") == 0)
        return NSVG_FILLRULE_EVENODD;
    // TODO: handle inherit.
    return NSVG_FILLRULE_NONZERO;
}

static const char* nsvg__getNextDashItem(const char* s, char* it)
{
    int n = 0;
    it[0] = '\0';
    // Skip white spaces and commas
    while (*s && (nsvg__isspace(*s) || *s == ',')) s++;
    // Advance until whitespace, comma or end.
    while (*s && (!nsvg__isspace(*s) && *s != ',')) {
        if (n < 63)
            it[n++] = *s;
        s++;
    }
    it[n++] = '\0';
    return s;
}

static int nsvg__parseStrokeDashArray(NSVGparser* p, const char* str, float* strokeDashArray)
{
    char item[64];
    int count = 0, i;
    float sum = 0.0f;

    // Handle "none"
    if (str[0] == 'n')
        return 0;

    // Parse dashes
    while (*str) {
        str = nsvg__getNextDashItem(str, item);
        if (!*item) break;
        if (count < NSVG_MAX_DASHES)
            strokeDashArray[count++] = fabsf(nsvg__parseCoordinate(p, item, 0.0f, nsvg__actualLength(p)));
    }

    for (i = 0; i < count; i++)
        sum += strokeDashArray[i];
    if (sum <= 1e-6f)
        count = 0;

    if (count == 1) {
        strokeDashArray[count] = strokeDashArray[count - 1];
        count++;
    }

    return count;
}

static void nsvg__parseStyle(NSVGparser* p, const char* str);

static int nsvg__parseStyleAttr(NSVGparser* p, NSVGstyle* style, const char* name, const char* value)
{
    //NSVGattrib* attr = nsvg__getAttr(p);

    if (strcmp(name, "stroke") == 0) {
        style->strokeFlag = 1;
        if (strcmp(value, "none") == 0) {
            style->hasStroke = 0;
        }
        else if (strncmp(value, "url(", 4) == 0) {
            //todo
            //attr->hasStroke = 2;
            //nsvg__parseUrl(attr->strokeGradient, value);
        }
        else if (strstr(value, "rgba(")) {
            style->hasStroke = 1;
            style->strokeColor = nsvg__parseColor(value);
            style->strokeOpacity = nsvg__paraseOpacityStyle(value);
        }
        else{
            style->hasStroke = 1;
            style->strokeColor = nsvg__parseColor(value);
        }
    }
    else if (strcmp(name, "opacity") == 0) {
        style->opacity = nsvg__parseOpacity(value);
    }
    else if (strcmp(name, "stroke-width") == 0) {
        style->strokeWidth = nsvg__parseCoordinate(p, value, 0.0f, nsvg__actualLength(p));
    }
    else if (strcmp(name, "stroke-linecap") == 0) {
        style->strokeLineCap = nsvg__parseLineCap(value);
    }
    else if (strcmp(name, "stroke-linejoin") == 0) {
        style->strokeLineJoin = nsvg__parseLineJoin(value);
    }
    else if (strcmp(name, "stroke-opacity") == 0) {
        style->strokeOpacity = nsvg__parseOpacity(value);
    }
    else if (strcmp(name, "stroke-miterlimit") == 0) {
        style->miterLimit = nsvg__parseMiterLimit(value);
    }
    else if (strcmp(name, "fill-opacity") == 0) {
        style->fillOpacity = nsvg__parseOpacity(value);
    }
    else if (strcmp(name, "mix-blend-mode") == 0) {
        strncpy(style->mixBlendMode, value, 63);
        style->mixBlendMode[63] = '\0';
    }
    else if (strcmp(name, "fill") == 0) {
        style->fillFlag = 1;
        if (strcmp(value, "none") == 0) {
            style->hasFill = 0;
        }
        else if (strncmp(value, "url(", 4) == 0) {
            style->hasFill = 2;
            char fillname[64];
            nsvg__parseUrl(fillname, value);
            NSVGpatternData* pattern = nsvg__findPatternData(p, fillname);
            if (pattern != NULL) {
                memcpy(style->fillPattern, fillname, 64);
            }
            else {
                memcpy(style->fillGradient, fillname, 64);
            }
        }
        else {
            style->hasFill = 1;
            style->fillColor = nsvg__parseColor(value);
            if (strstr(value, "rgba(")){
                style->fillOpacity = nsvg__paraseOpacityStyle(value);
            }
        }
    }
    else if (strcmp(name, "stroke") == 0) {
        style->strokeColor = nsvg__parseColor(value);
    }
    else if (strcmp(name, "font-size") == 0) {
        style->fontsize = nsvg__parseCoordinateRaw(value);
    }

    return 1;
}

static int nsvg__parseAttr(NSVGparser* p, const char* name, const char* value)
{
    float xform[6];
    NSVGattrib* attr = nsvg__getAttr(p);
    if (!attr) return 0;

    if (strcmp(name, "style") == 0) {
        nsvg__parseStyle(p, value);
    } else if (strcmp(name, "class") == 0) {
        strncpy(p->class_name[class_nums_count], value, 63);
        class_nums_count++;
    } else if (strcmp(name, "display") == 0) {
        if (strcmp(value, "none") == 0)
            attr->visible = 0;
        // Don't reset ->visible on display:inline, one display:none hides the whole subtree

    } else if (strcmp(name, "fill") == 0) {
        attr->fillFlag = 1;
        if (strcmp(value, "none") == 0) {
            attr->hasFill = 0;
        } else if (strncmp(value, "url(", 4) == 0) {
            attr->hasFill = 2;
            char fillname[64];
            nsvg__parseUrl(fillname, value);
            NSVGpatternData* pattern = nsvg__findPatternData(p, fillname);
            if (pattern != NULL) {
                memcpy(attr->fillPattern, fillname, 64);
            } else {
                memcpy(attr->fillGradient, fillname, 64);
            }
        } else {
            attr->hasFill = 1;
            attr->fillColor = nsvg__parseColor(value);
        }
    } else if (strcmp(name, "opacity") == 0) {
        float tempOpacity = nsvg__parseOpacity(value);
        attr->opacity = attr->opacity < tempOpacity ? attr->opacity : tempOpacity;
    } else if (strcmp(name, "fill-opacity") == 0) {
        attr->fillOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "stroke") == 0) {
        attr->strokeFlag = 1;
        if (strcmp(value, "none") == 0) {
            attr->hasStroke = 0;
        } else if (strncmp(value, "url(", 4) == 0) {
            attr->hasStroke = 2;
            nsvg__parseUrl(attr->strokeGradient, value);
        } else {
            attr->hasStroke = 1;
            attr->strokeColor = nsvg__parseColor(value);
        }
    } else if (strcmp(name, "clip-path") == 0) {
        nsvg__parseUrl(attr->clipPath, value);
    } else if (strcmp(name, "stroke-width") == 0) {
        attr->strokeWidth = nsvg__parseCoordinate(p, value, 0.0f, nsvg__actualLength(p));
    } else if (strcmp(name, "stroke-dasharray") == 0) {
        attr->strokeDashCount = nsvg__parseStrokeDashArray(p, value, attr->strokeDashArray);
    } else if (strcmp(name, "stroke-dashoffset") == 0) {
        attr->strokeDashOffset = nsvg__parseCoordinate(p, value, 0.0f, nsvg__actualLength(p));
    } else if (strcmp(name, "stroke-opacity") == 0) {
        attr->strokeOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "stroke-linecap") == 0) {
        attr->strokeLineCap = nsvg__parseLineCap(value);
    } else if (strcmp(name, "stroke-linejoin") == 0) {
        attr->strokeLineJoin = nsvg__parseLineJoin(value);
    } else if (strcmp(name, "stroke-miterlimit") == 0) {
        attr->miterLimit = nsvg__parseMiterLimit(value);
    } else if (strcmp(name, "fill-rule") == 0) {
        attr->fillRule = nsvg__parseFillRule(value);
    } else if (strcmp(name, "font-size") == 0) {
        attr->fontSize = nsvg__parseCoordinate(p, value, 0.0f, nsvg__actualLength(p));
    } else if (strcmp(name, "transform") == 0) {
        nsvg__parseTransform(xform, value);
        nsvg__xformPremultiply(attr->xform, xform);
    } else if (strcmp(name, "stop-color") == 0) {
        attr->stopColor = nsvg__parseColor(value);
    } else if (strcmp(name, "stop-opacity") == 0) {
        attr->stopOpacity = nsvg__parseOpacity(value);
    } else if (strcmp(name, "offset") == 0) {
        attr->stopOffset = nsvg__parseCoordinate(p, value, 0.0f, 1.0f);
    } else if (strcmp(name, "id") == 0) {
        strncpy(attr->id, value, 63);
        attr->id[63] = '\0';
    } else if (strcmp(name, "transform-origin") == 0) {
            strncpy(attr->transformOrigin, value, 15);
            attr->transformOrigin[15] = '\0';
    } else if (strcmp(name, "transform-box") == 0) {
            strncpy(attr->transformBox, value, 15);
            attr->transformOrigin[15] = '\0';
    } else {
        return 0;
    }
    return 1;
}

static int nsvg__parseNameValue(NSVGparser* p, const char* start, const char* end)
{
    const char* str;
    const char* val;
    char name[512];
    char value[512];
    int n;

    str = start;
    while (str < end && *str != ':') ++str;

    val = str;

    // Right Trim
    while (str > start &&  (*str == ':' || nsvg__isspace(*str))) --str;
    ++str;

    n = (int)(str - start);
    if (n > 511) n = 511;
    if (n) memcpy(name, start, n);
    name[n] = 0;

    while (val < end && (*val == ':' || nsvg__isspace(*val))) ++val;

    n = (int)(end - val);
    if (n > 511) n = 511;
    if (n) memcpy(value, val, n);
    value[n] = 0;

    return nsvg__parseAttr(p, name, value);
}

static void nsvg__parseStyle(NSVGparser* p, const char* str)
{
    const char* start;
    const char* end;

    while (*str) {
        // Left Trim
        while(*str && nsvg__isspace(*str)) ++str;
        start = str;
        while(*str && *str != ';') ++str;
        end = str;

        // Right Trim
        while (end > start &&  (*end == ';' || nsvg__isspace(*end))) --end;
        ++end;

        nsvg__parseNameValue(p, start, end);
        if (*str) ++str;
    }
}

static void nsvg__parseStyleAttribs(NSVGparser* p, NSVGstyle* style, const char** attr)
{
    int i;
    for(i = 0; attr[i]; i += 2)
        nsvg__parseStyleAttr(p, style, attr[i], attr[i + 1]);
}

static void nsvg__parseAttribs(NSVGparser* p, const char** attr, const char* el)
{
    int i;
    int is_g = 0;
    if (strcmp(el, "g") == 0)
    {
        is_g = 1;
        g_index++;
    }

    for (i = 0; attr[i]; i += 2)
    {
        if (strcmp(attr[i], "style") == 0)
            nsvg__parseStyle(p, attr[i + 1]);
        else if (strcmp(attr[i], "class") == 0) {
            strncpy(p->class_name[class_nums_count], attr[i + 1], 63);
            class_nums_count++;
            if (is_g)
                g_class_flag[g_index - 1] = 1;
        } else
            nsvg__parseAttr(p, attr[i], attr[i + 1]);
    }
}

void nsvg__parseAnimate(NSVGparser* p, const char** attr)
{
    NSVGanimateData* animate = NANOSVG_MALLOC(sizeof(NSVGanimateData));
    if (animate == NULL) return;
    memset(animate, 0, sizeof(NSVGanimateData));
    p->animateData = animate;

    for (int i = 0; attr[i]; i += 2)
    {
        if (strcmp(attr[i], "attributeName") == 0) {
            strncpy(p->animateData->attributeName, attr[i+1], 15);
            p->animateData->attributeName[15] = '\0';
        } else if (strcmp(attr[i], "attributeType") == 0) {
            strncpy(p->animateData->attributeType, attr[i + 1], 15);
            p->animateData->attributeType[15] = '\0';
        } else if (strcmp(attr[i], "type") == 0) {
            strncpy(p->animateData->type, attr[i + 1], 9);
            p->animateData->type[9] = '\0';
        } else if (strcmp(attr[i], "begin") == 0) {
            p->animateData->begin = nsvg__atof(attr[i + 1]);
        } else if (strcmp(attr[i], "dur") == 0) {
            p->animateData->dur = nsvg__atof(attr[i + 1]);
        } else if (strcmp(attr[i], "from") == 0) {
            char* token;
            char* value = (char*)attr[i+1];
            int i = 0;
            token = strtok(value, " ");
            while (token != NULL && i < 3)
            {
                p->animateData->form[i] = nsvg__atof(token);
                i++;
                token = strtok(NULL, " ");
            }
        } else if (strcmp(attr[i], "to") == 0) {
            char* token;
            char* value = (char*)attr[i + 1];
            int i = 0;
            token = strtok(value, " ");
            while (token != NULL && i < 3)
            {
                p->animateData->to[i] = nsvg__atof(token);
                i++;
                token = strtok(NULL, " ");
            }
        } else if (strcmp(attr[i], "repeatCount") == 0) {
            if (strcmp(attr[i + 1], "indefinite") == 0)
                p->animateData->repeatCount = -1;
            else
                p->animateData->repeatCount = nsvg__atof(attr[i + 1]);
        }
    }
};

static int nsvg__getArgsPerElement(char cmd)
{
    switch (cmd) {
        case 'v':
        case 'V':
        case 'h':
        case 'H':
            return 1;
        case 'm':
        case 'M':
        case 'l':
        case 'L':
        case 't':
        case 'T':
            return 2;
        case 'q':
        case 'Q':
        case 's':
        case 'S':
            return 4;
        case 'c':
        case 'C':
            return 6;
        case 'a':
        case 'A':
            return 7;
        case 'z':
        case 'Z':
            return 0;
    }
    return -1;
}

static void nsvg__pathMoveTo(NSVGparser* p, float* cpx, float* cpy, float* args, int rel)
{
    if (rel) {
        *cpx += args[0];
        *cpy += args[1];
    } else {
        *cpx = args[0];
        *cpy = args[1];
    }
    nsvg__moveTo(p, *cpx, *cpy);
}

static void nsvg__pathLineTo(NSVGparser* p, float* cpx, float* cpy, float* args, int rel)
{
    if (rel) {
        *cpx += args[0];
        *cpy += args[1];
    } else {
        *cpx = args[0];
        *cpy = args[1];
    }
    nsvg__lineTo(p, *cpx, *cpy);
}

static void nsvg__pathHLineTo(NSVGparser* p, float* cpx, float* cpy, float* args, int rel)
{
    if (rel)
        *cpx += args[0];
    else
        *cpx = args[0];
    nsvg__lineTo(p, *cpx, *cpy);
}

static void nsvg__pathVLineTo(NSVGparser* p, float* cpx, float* cpy, float* args, int rel)
{
    if (rel)
        *cpy += args[0];
    else
        *cpy = args[0];
    nsvg__lineTo(p, *cpx, *cpy);
}

static void nsvg__pathCubicBezTo(NSVGparser* p, float* cpx, float* cpy,
                                 float* cpx2, float* cpy2, float* args, int rel)
{
    float x2, y2, cx1, cy1, cx2, cy2;

    if (rel) {
        cx1 = *cpx + args[0];
        cy1 = *cpy + args[1];
        cx2 = *cpx + args[2];
        cy2 = *cpy + args[3];
        x2 = *cpx + args[4];
        y2 = *cpy + args[5];
    } else {
        cx1 = args[0];
        cy1 = args[1];
        cx2 = args[2];
        cy2 = args[3];
        x2 = args[4];
        y2 = args[5];
    }

    nsvg__cubicBezTo(p, cx1,cy1, cx2,cy2, x2,y2);

    *cpx2 = cx2;
    *cpy2 = cy2;
    *cpx = x2;
    *cpy = y2;
}

static void nsvg__pathCubicBezShortTo(NSVGparser* p, float* cpx, float* cpy,
                                      float* cpx2, float* cpy2, float* args, int rel)
{
    float x1, y1, x2, y2, cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        cx2 = *cpx + args[0];
        cy2 = *cpy + args[1];
        x2 = *cpx + args[2];
        y2 = *cpy + args[3];
    } else {
        cx2 = args[0];
        cy2 = args[1];
        x2 = args[2];
        y2 = args[3];
    }

    cx1 = 2*x1 - *cpx2;
    cy1 = 2*y1 - *cpy2;

    nsvg__cubicBezTo(p, cx1,cy1, cx2,cy2, x2,y2);

    *cpx2 = cx2;
    *cpy2 = cy2;
    *cpx = x2;
    *cpy = y2;
}

static void nsvg__pathQuadBezTo(NSVGparser* p, float* cpx, float* cpy,
                                float* cpx2, float* cpy2, float* args, int rel)
{
    float x1, y1, x2, y2, cx, cy;
    float cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        cx = *cpx + args[0];
        cy = *cpy + args[1];
        x2 = *cpx + args[2];
        y2 = *cpy + args[3];
    } else {
        cx = args[0];
        cy = args[1];
        x2 = args[2];
        y2 = args[3];
    }

    // Convert to cubic bezier
    cx1 = x1 + 2.0f/3.0f*(cx - x1);
    cy1 = y1 + 2.0f/3.0f*(cy - y1);
    cx2 = x2 + 2.0f/3.0f*(cx - x2);
    cy2 = y2 + 2.0f/3.0f*(cy - y2);

    nsvg__cubicBezTo(p, cx1,cy1, cx2,cy2, x2,y2);

    *cpx2 = cx;
    *cpy2 = cy;
    *cpx = x2;
    *cpy = y2;
}

static void nsvg__pathQuadBezShortTo(NSVGparser* p, float* cpx, float* cpy,
                                     float* cpx2, float* cpy2, float* args, int rel)
{
    float x1, y1, x2, y2, cx, cy;
    float cx1, cy1, cx2, cy2;

    x1 = *cpx;
    y1 = *cpy;
    if (rel) {
        x2 = *cpx + args[0];
        y2 = *cpy + args[1];
    } else {
        x2 = args[0];
        y2 = args[1];
    }

    cx = 2*x1 - *cpx2;
    cy = 2*y1 - *cpy2;

    // Convert to cubix bezier
    cx1 = x1 + 2.0f/3.0f*(cx - x1);
    cy1 = y1 + 2.0f/3.0f*(cy - y1);
    cx2 = x2 + 2.0f/3.0f*(cx - x2);
    cy2 = y2 + 2.0f/3.0f*(cy - y2);

    nsvg__cubicBezTo(p, cx1,cy1, cx2,cy2, x2,y2);

    *cpx2 = cx;
    *cpy2 = cy;
    *cpx = x2;
    *cpy = y2;
}

static float nsvg__sqr(float x) { return x*x; }
static float nsvg__vmag(float x, float y) { return sqrtf(x*x + y*y); }

static float nsvg__vecrat(float ux, float uy, float vx, float vy)
{
    return (ux*vx + uy*vy) / (nsvg__vmag(ux,uy) * nsvg__vmag(vx,vy));
}

static float nsvg__vecang(float ux, float uy, float vx, float vy)
{
    float r = nsvg__vecrat(ux,uy, vx,vy);
    if (r < -1.0f) r = -1.0f;
    if (r > 1.0f) r = 1.0f;
    return ((ux*vy < uy*vx) ? -1.0f : 1.0f) * acosf(r);
}

static void nsvg__pathArcTo(NSVGparser* p, float* cpx, float* cpy, float* args, int rel)
{
    // Ported from canvg (https://code.google.com/p/canvg/)
    float rx, ry, rotx;
    float x1, y1, x2, y2, cx, cy, dx, dy, d;
    float x1p, y1p, cxp, cyp, s, sa, sb;
    float ux, uy, vx, vy, a1, da;
    float x, y, tanx, tany, a, px = 0, py = 0, ptanx = 0, ptany = 0, t[6];
    float sinrx, cosrx;
    int fa, fs;
    int i, ndivs;
    float hda, kappa;

    rx = fabsf(args[0]);                // y radius
    ry = fabsf(args[1]);                // x radius
    rotx = args[2] / 180.0f * NSVG_PI;        // x rotation angle
    fa = fabsf(args[3]) > 1e-6 ? 1 : 0;    // Large arc
    fs = fabsf(args[4]) > 1e-6 ? 1 : 0;    // Sweep direction
    x1 = *cpx;                            // start point
    y1 = *cpy;
    if (rel) {                            // end point
        x2 = *cpx + args[5];
        y2 = *cpy + args[6];
    } else {
        x2 = args[5];
        y2 = args[6];
    }

    dx = x1 - x2;
    dy = y1 - y2;
    d = sqrtf(dx*dx + dy*dy);
    if (d < 1e-6f || rx < 1e-6f || ry < 1e-6f) {
        // The arc degenerates to a line
        nsvg__lineTo(p, x2, y2);
        *cpx = x2;
        *cpy = y2;
        return;
    }

    sinrx = sinf(rotx);
    cosrx = cosf(rotx);

    // Convert to center point parameterization.
    // http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes
    // 1) Compute x1', y1'
    x1p = cosrx * dx / 2.0f + sinrx * dy / 2.0f;
    y1p = -sinrx * dx / 2.0f + cosrx * dy / 2.0f;
    d = nsvg__sqr(x1p)/nsvg__sqr(rx) + nsvg__sqr(y1p)/nsvg__sqr(ry);
    if (d > 1) {
        d = sqrtf(d);
        rx *= d;
        ry *= d;
    }
    // 2) Compute cx', cy'
    s = 0.0f;
    sa = nsvg__sqr(rx)*nsvg__sqr(ry) - nsvg__sqr(rx)*nsvg__sqr(y1p) - nsvg__sqr(ry)*nsvg__sqr(x1p);
    sb = nsvg__sqr(rx)*nsvg__sqr(y1p) + nsvg__sqr(ry)*nsvg__sqr(x1p);
    if (sa < 0.0f) sa = 0.0f;
    if (sb > 0.0f)
        s = sqrtf(sa / sb);
    if (fa == fs)
        s = -s;
    cxp = s * rx * y1p / ry;
    cyp = s * -ry * x1p / rx;

    // 3) Compute cx,cy from cx',cy'
    cx = (x1 + x2)/2.0f + cosrx*cxp - sinrx*cyp;
    cy = (y1 + y2)/2.0f + sinrx*cxp + cosrx*cyp;

    // 4) Calculate theta1, and delta theta.
    ux = (x1p - cxp) / rx;
    uy = (y1p - cyp) / ry;
    vx = (-x1p - cxp) / rx;
    vy = (-y1p - cyp) / ry;
    a1 = nsvg__vecang(1.0f,0.0f, ux,uy);    // Initial angle
    da = nsvg__vecang(ux,uy, vx,vy);        // Delta angle

//    if (vecrat(ux,uy,vx,vy) <= -1.0f) da = NSVG_PI;
//    if (vecrat(ux,uy,vx,vy) >= 1.0f) da = 0;

    if (fs == 0 && da > 0)
        da -= 2 * NSVG_PI;
    else if (fs == 1 && da < 0)
        da += 2 * NSVG_PI;

    // Approximate the arc using cubic spline segments.
    t[0] = cosrx; t[1] = sinrx;
    t[2] = -sinrx; t[3] = cosrx;
    t[4] = cx; t[5] = cy;

    // Split arc into max 90 degree segments.
    // The loop assumes an iteration per end point (including start and end), this +1.
    ndivs = (int)(fabsf(da) / (NSVG_PI*0.5f) + 1.0f);
    hda = (da / (float)ndivs) / 2.0f;
    // Fix for ticket #179: division by 0: avoid cotangens around 0 (infinite)
    if ((hda < 1e-3f) && (hda > -1e-3f))
        hda *= 0.5f;
    else
        hda = (1.0f - cosf(hda)) / sinf(hda);
    kappa = fabsf(4.0f / 3.0f * hda);
    if (da < 0.0f)
        kappa = -kappa;

    for (i = 0; i <= ndivs; i++) {
        a = a1 + da * ((float)i/(float)ndivs);
        dx = cosf(a);
        dy = sinf(a);
        nsvg__xformPoint(&x, &y, dx*rx, dy*ry, t); // position
        nsvg__xformVec(&tanx, &tany, -dy*rx * kappa, dx*ry * kappa, t); // tangent
        if (i > 0)
            nsvg__cubicBezTo(p, px+ptanx,py+ptany, x-tanx, y-tany, x, y);
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    *cpx = x2;
    *cpy = y2;
}

static void nsvg__parseDefsPath(NSVGparser* p, const char** attr)
{
    const char* s = NULL;
    int i;
    char name[64];
    double x = 0, y = 0;
    char *char_x, *char_y;
    char *saveptr;

    name[0] = 0;

    /* Parse path content. */
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strcpy(name, attr[i + 1]);
        } else if(strcmp(attr[i], "d") == 0) {
            s = attr[i + 1];
        }
    }

    if (s) {
        s++;
        char_x = strtok_s((char*)s, " ", &saveptr);
        char_y = strtok_s(NULL, " ", &saveptr);

        x = nsvg__atof(char_x);
        y = nsvg__atof(char_y);
    }

    /* Add the path to parser. */
    NSVGdefspath* path = NANOSVG_MALLOC(sizeof(NSVGdefspath));
    if (path == NULL) return;
    memset(path, 0, sizeof(NSVGdefspath));
    strcpy(path->name, name);
    path->x = x;
    path->y = y;

    if (p->defspath) {
        path->next = p->defspath;
        p->defspath = path;
    } else {
        p->defspath = path;
    }

}

static void nsvg__parseDefsimage(NSVGparser* p, const char** attr)
{
    const char* ref = NULL;
    int i;
    char name[64];
    int width = 0, height = 0;
    double x = 0, y = 0;
    char* s, *imgdata = NULL, *encoding = NULL, *format = NULL;
    char* saveptr;
    char* saveptr1;

    name[0] = 0;

    /* Parse path content. */
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strcpy(name, attr[i + 1]);
        }
        else if (strcmp(attr[i], "width") == 0) {
            width = nsvg__atof(attr[i + 1]) * (svg_scale_image_width / svg_image_width);
        }
        else if (strcmp(attr[i], "height") == 0) {
            height = nsvg__atof(attr[i + 1]) * (svg_scale_image_height / svg_image_height);
        }
        else if (strcmp(attr[i], "x") == 0) {
            x = nsvg__atof(attr[i + 1]) * (svg_scale_image_width / svg_image_width);
        }
        else if (strcmp(attr[i], "y") == 0) {
            y = nsvg__atof(attr[i + 1]) * (svg_scale_image_height / svg_image_height);
        }
        else if (strcmp(attr[i], "xlink:href") == 0) {
            ref = attr[i + 1];
        }
    }

    NSVGdefsImage* image = (NSVGdefsImage*)NANOSVG_MALLOC(sizeof(NSVGdefsImage));
    if (image == NULL) return;
    memset(image, 0, sizeof(NSVGdefsImage));
    image->drawFlag = 1;

    if (ref) {
        s = strtok_s((char*)ref, " ;", &saveptr);
        encoding = strtok_s(NULL, " ,", &saveptr);
        imgdata = saveptr;

        if (imgdata && imgdata[0]) {
            s = strtok_s(s, "/", &saveptr1);
            format = saveptr1;
        } else {
            imgdata = s;
            format = (char *)NSVG_IMAGE_FORMAT_URL;
        }
    }
    if (format && format[0]) {
        strncpy(image->format, format, sizeof(image->format));
        image->format[sizeof(image->format) - 1] = '\0';
    }
    if (encoding && encoding[0]) {
        strncpy(image->encoding, encoding, sizeof(image->encoding));
        image->encoding[sizeof(image->encoding) - 1] = '\0';
    }
    if (imgdata && imgdata[0]) {
        int i = strlen(imgdata);
        if (i >= IMGDATA_SIZE)
            image->drawFlag = 0;

        if (image->drawFlag)
            strcpy(image->imgdata, imgdata);
        else
            NANOSVG_PRINT("Need to increase the size of IMGDATA_SIZE to %d.\n", i + 1);
    }
    if (name[0])
        strcpy(image->name, name);
    image->width = width;
    image->height = height;
    image->x = x;
    image->y = y;

    /* Add the image to parser. */
    if (p->defsimage) {
        image->next = p->defsimage;
        p->defsimage = image;
    }
    else {
        p->defsimage = image;
    }

    if (p->patternFlag) {
        if (p->patterns->image == NULL)
            p->patterns->image = image;
        else {
            NSVGdefsImage* tem_image = p->patterns->image;
            while (tem_image->next)
                tem_image = tem_image->next;
            tem_image->next = image;
        }
    }
}

static void nsvg__parsePath(NSVGparser* p, const char** attr)
{
    const char* s = NULL;
    char cmd = '\0';
    float args[10];
    int nargs;
    int rargs = 0;
    char initPoint;
    float cpx, cpy, cpx2, cpy2;
    const char* tmp[4];
    char closedFlag;
    int i;
    char item[64];
    int class_flag = 0;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "d") == 0) {
            s = attr[i + 1];
        } else {
            tmp[0] = attr[i];
            tmp[1] = attr[i + 1];
            tmp[2] = 0;
            tmp[3] = 0;
            if (strcmp(attr[i], "class") == 0)
                class_flag = 1;
            nsvg__parseAttribs(p, tmp, "path");
        }
    }

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
                                cpx = args[nargs-2];
                                cpy = args[nargs-1];
                                cpx2 = cpx; cpy2 = cpy;
                            }
                            break;
                    }
                    nargs = 0;
                }
            } else {
                cmd = item[0];
                if (cmd == 'M' || cmd == 'm') {
                    // Commit path.
                    if (p->npts > 0)
                        nsvg__addPath(p, closedFlag);
                    // Start new subpath.
                    nsvg__resetPath(p);
                    closedFlag = 0;
                    nargs = 0;
                } else if (initPoint == 0) {
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
    if (class_flag || class_nums_count) {
        nsvg__addClassShape(p);
        if (class_flag)
            class_nums_count--;
    }
    else
        nsvg__addShape(p);

}

static void nsvg__parseRect(NSVGparser* p, const char** attr)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float rx = -1.0f; // marks not set
    float ry = -1.0f;
    int class_flag = 0;
    int i;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0)
            class_flag = 1;
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "x") == 0) x = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
            if (strcmp(attr[i], "y") == 0) y = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
            if (strcmp(attr[i], "width") == 0) w = nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualWidth(p));
            if (strcmp(attr[i], "height") == 0) h = nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualHeight(p));
            if (strcmp(attr[i], "rx") == 0) rx = fabsf(nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualWidth(p)));
            if (strcmp(attr[i], "ry") == 0) ry = fabsf(nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualHeight(p)));
        }
    }

    if (rx < 0.0f && ry > 0.0f) rx = ry;
    if (ry < 0.0f && rx > 0.0f) ry = rx;
    if (rx < 0.0f) rx = 0.0f;
    if (ry < 0.0f) ry = 0.0f;
    if (rx > w/2.0f) rx = w/2.0f;
    if (ry > h/2.0f) ry = h/2.0f;

    if (w != 0.0f && h != 0.0f) {
        nsvg__resetPath(p);

        if (rx < 0.00001f || ry < 0.0001f) {
            nsvg__moveTo(p, x, y);
            nsvg__lineTo(p, x+w, y);
            nsvg__lineTo(p, x+w, y+h);
            nsvg__lineTo(p, x, y+h);
        } else {
            // Rounded rectangle
            nsvg__moveTo(p, x+rx, y);
            nsvg__lineTo(p, x+w-rx, y);
            nsvg__cubicBezTo(p, x+w-rx*(1-NSVG_KAPPA90), y, x+w, y+ry*(1-NSVG_KAPPA90), x+w, y+ry);
            nsvg__lineTo(p, x+w, y+h-ry);
            nsvg__cubicBezTo(p, x+w, y+h-ry*(1-NSVG_KAPPA90), x+w-rx*(1-NSVG_KAPPA90), y+h, x+w-rx, y+h);
            nsvg__lineTo(p, x+rx, y+h);
            nsvg__cubicBezTo(p, x+rx*(1-NSVG_KAPPA90), y+h, x, y+h-ry*(1-NSVG_KAPPA90), x, y+h-ry);
            nsvg__lineTo(p, x, y+ry);
            nsvg__cubicBezTo(p, x, y+ry*(1-NSVG_KAPPA90), x+rx*(1-NSVG_KAPPA90), y, x+rx, y);
        }

        nsvg__addPath(p, 1);

        if (class_nums_count || class_flag) {
            nsvg__addClassShape(p);
            if (class_flag)
                class_nums_count--;
        }
        else
            nsvg__addShape(p);
    }
}

static void nsvg__parseCircle(NSVGparser* p, const char** attr)
{
    float cx = 0.0f;
    float cy = 0.0f;
    float r = 0.0f;
    int class_flag = 0;
    int i;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0)
            class_flag = 1;
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "cx") == 0) cx = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
            if (strcmp(attr[i], "cy") == 0) cy = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
            if (strcmp(attr[i], "r") == 0) r = fabsf(nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualLength(p)));
        }
    }

    if (r > 0.0f) {
        nsvg__resetPath(p);

        nsvg__moveTo(p, cx+r, cy);
        nsvg__cubicBezTo(p, cx+r, cy+r*NSVG_KAPPA90, cx+r*NSVG_KAPPA90, cy+r, cx, cy+r);
        nsvg__cubicBezTo(p, cx-r*NSVG_KAPPA90, cy+r, cx-r, cy+r*NSVG_KAPPA90, cx-r, cy);
        nsvg__cubicBezTo(p, cx-r, cy-r*NSVG_KAPPA90, cx-r*NSVG_KAPPA90, cy-r, cx, cy-r);
        nsvg__cubicBezTo(p, cx+r*NSVG_KAPPA90, cy-r, cx+r, cy-r*NSVG_KAPPA90, cx+r, cy);

        nsvg__addPath(p, 1);

        if (class_nums_count || class_flag) {
            nsvg__addClassShape(p);
            if (class_flag)
                class_nums_count--;
        }
        else
            nsvg__addShape(p);
    }
}

static void nsvg__parseEllipse(NSVGparser* p, const char** attr)
{
    float cx = 0.0f;
    float cy = 0.0f;
    float rx = 0.0f;
    float ry = 0.0f;
    int class_flag = 0;
    int i;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0)
            class_flag = 1;
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "cx") == 0) cx = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
            if (strcmp(attr[i], "cy") == 0) cy = nsvg__parseCoordinate(p, attr[i+1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
            if (strcmp(attr[i], "rx") == 0) rx = fabsf(nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualWidth(p)));
            if (strcmp(attr[i], "ry") == 0) ry = fabsf(nsvg__parseCoordinate(p, attr[i+1], 0.0f, nsvg__actualHeight(p)));
        }
    }

    if (rx > 0.0f && ry > 0.0f) {

        nsvg__resetPath(p);

        nsvg__moveTo(p, cx+rx, cy);
        nsvg__cubicBezTo(p, cx+rx, cy+ry*NSVG_KAPPA90, cx+rx*NSVG_KAPPA90, cy+ry, cx, cy+ry);
        nsvg__cubicBezTo(p, cx-rx*NSVG_KAPPA90, cy+ry, cx-rx, cy+ry*NSVG_KAPPA90, cx-rx, cy);
        nsvg__cubicBezTo(p, cx-rx, cy-ry*NSVG_KAPPA90, cx-rx*NSVG_KAPPA90, cy-ry, cx, cy-ry);
        nsvg__cubicBezTo(p, cx+rx*NSVG_KAPPA90, cy-ry, cx+rx, cy-ry*NSVG_KAPPA90, cx+rx, cy);

        nsvg__addPath(p, 1);

        if (class_nums_count || class_flag) {
            nsvg__addClassShape(p);
            if (class_flag)
                class_nums_count--;
        }
        else
            nsvg__addShape(p);
    }
}

static void nsvg__parseLine(NSVGparser* p, const char** attr)
{
    float x1 = 0.0;
    float y1 = 0.0;
    float x2 = 0.0;
    float y2 = 0.0;
    int class_flag = 0;
    int i;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0)
            class_flag = 1;
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "x1") == 0) x1 = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
            if (strcmp(attr[i], "y1") == 0) y1 = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
            if (strcmp(attr[i], "x2") == 0) x2 = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
            if (strcmp(attr[i], "y2") == 0) y2 = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
        }
    }

    nsvg__resetPath(p);

    nsvg__moveTo(p, x1, y1);
    nsvg__lineTo(p, x2, y2);

    nsvg__addPath(p, 0);

    if (class_nums_count || class_flag) {
        nsvg__addClassShape(p);
        if (class_flag)
            class_nums_count--;
    }
    else
        nsvg__addShape(p);
}

static void nsvg__parsePoly(NSVGparser* p, const char** attr, int closeFlag)
{
    int i;
    const char* s;
    float args[2];
    int nargs, npts = 0;
    int class_flag = 0;
    char item[64];

    nsvg__resetPath(p);

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0)
            class_flag = 1;
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "points") == 0) {
                s = attr[i + 1];
                nargs = 0;
                while (*s) {
                    s = nsvg__getNextPathItem(s, item);
                    args[nargs++] = (float)nsvg__atof(item);
                    if (nargs >= 2) {
                        if (npts == 0)
                            nsvg__moveTo(p, args[0], args[1]);
                        else
                            nsvg__lineTo(p, args[0], args[1]);
                        nargs = 0;
                        npts++;
                    }
                }
            }
        }
    }

    nsvg__addPath(p, (char)closeFlag);

    if (class_nums_count || class_flag) {
        nsvg__addClassShape(p);
        if (class_flag)
            class_nums_count--;
    }
    else
        nsvg__addShape(p);
}

static void nsvg__parseSymbol(NSVGparser* p, const char** attr)
{
    char* start = content_start;
    char* end;
    int i;

    NSVGsymbolData* symb = (NSVGsymbolData*)NANOSVG_MALLOC(sizeof(NSVGsymbolData));
    if (symb == NULL) return;
    memset(symb, 0, sizeof(NSVGsymbolData));

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strncpy(symb->id, attr[i+1], 63);
            symb->id[63] = '\0';
        }
    }

    while (*start != '<') start++;
    end = start;
    while (memcmp(end++, "</symbol", 8) != 0);
    end--;
    symb->length = end - start + 1;
    symb->content = (char*)NANOSVG_MALLOC(((symb->length + 3) >> 2) << 2);
    if (symb->content) {
        memcpy(symb->content, start, symb->length);
        symb->content[symb->length - 1] = '\0';

        symb->next = p->symbols;
        p->symbols = symb;
    } else {
        NANOSVG_FREE(symb);
    }

    // Skip "</symbol>" string
    content_end = end + 9;
}

static void nsvg__parseDefsTag(NSVGparser* p, const char *el, const char** attr)
{
    char* start = content_start;
    char* end = NULL;
    int i;

    NSVGdefsTagData* elem = (NSVGdefsTagData*)NANOSVG_MALLOC(sizeof(NSVGdefsTagData));
    if (elem == NULL) return;
    memset(elem, 0, sizeof(NSVGdefsTagData));

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strncpy(elem->id, attr[i + 1], 63);
            elem->id[63] = '\0';
        }
    }
    strncpy(elem->tag, el, 63);
    elem->tag[63] = '\0';

    if (strcmp(el, "g") == 0) {
        while (*start != '<') start++;
        end = start;
        if (memcmp(start, "<symbol", 6) == 0) {
            NANOSVG_FREE(elem);
            content_end = end;
            return;
        }
        while (memcmp(end++, "</g", 3) != 0);
        end--;
        content_end = end + 4;
    }
    else {
        end = start;
        while (*end++ != '/');
        end--;
        content_end = end + 2;
    }

    elem->length = end - start + 1;
    elem->content = (char*)NANOSVG_MALLOC(((elem->length + 3) >> 2) << 2);
    if (elem->content == NULL) {
        NANOSVG_FREE(elem);
        return;
    }

    memcpy(elem->content, p->defsString + (start - p->defsStart), elem->length);
    elem->content[elem->length - 1] = '\0';

    elem->next = p->defTags;
    p->defTags = elem;
}

static void nsvg__parseUse(NSVGparser* p, const char** attr)
{
    NSVGsymbolData* symData;
    NSVGdefsTagData* tagData;
    NSVGdefsImage* image;
    NSVGattrib* svAttr;
    float usex = 0.0f;
    float usey = 0.0f;
    char* buffer;
    char* st;
    char* nt;
    int i;

    // Process all Use attributes first
    for (i = 0; attr[i]; i += 2) {
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "x") == 0) {
                usex = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigX(p), nsvg__actualWidth(p));
                /* Apply translate(usex, usery) to the */
                svAttr = nsvg__getAttr(p);
                svAttr->xform[4] += usex;
            }
            else if (strcmp(attr[i], "y") == 0) {
                usey = nsvg__parseCoordinate(p, attr[i + 1], nsvg__actualOrigY(p), nsvg__actualHeight(p));
                /* Apply translate(usex, usery) to the */
                svAttr = nsvg__getAttr(p);
                svAttr->xform[5] += usey;
            }
            else if (strcmp(attr[i], "width") == 0) {
                p->image->width = nsvg__parseCoordinate(p, attr[i + 1], 0.0f, nsvg__actualWidth(p));
            }
            else if (strcmp(attr[i], "height") == 0) {
                p->image->height = nsvg__parseCoordinate(p, attr[i + 1], 0.0f, nsvg__actualHeight(p));
            }
        }
    }

    // Then process Use reference content
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "xlink:href") == 0 || strcmp(attr[i], "href") == 0) {
            const char *hrefId = attr[i+1] + 1;

            symData = nsvg__findSymbolData(p, hrefId);
            if (symData) {
                buffer = (char*)NANOSVG_MALLOC(((symData->length+3)>>2) << 2);
                if (buffer) {
                    memcpy(buffer, symData->content, symData->length);
                    st = buffer;
                    while (*st) {
                        // Skip white space and '<'
                        while (*st && (nsvg__isspace(*st) || *st == '<')) st++;
                        nt = st;
                        while (*nt && (*nt != '>')) nt++;
                        nsvg__parseElement(st, nsvg__startElement, nsvg__endElement, p);
                        // Continue to next tag
                        st = (*nt) ? nt + 1 : nt;
                    }
                    NANOSVG_FREE(buffer);
                }
            }

            tagData = nsvg__findDefsTagData(p, hrefId);
            if (tagData) {
                buffer = (char*)NANOSVG_MALLOC(((tagData->length+3)>>2) << 2);
                if (buffer) {
                    memcpy(buffer, tagData->content, tagData->length);
                    st = buffer;
                    while (*st) {
                        // Skip white space and '<'
                        while (*st && (nsvg__isspace(*st) || *st == '<')) st++;
                        nt = st;
                        while (*nt && (*nt != '>')) nt++;
                        nsvg__parseElement(st, nsvg__startElement, nsvg__endElement, p);
                        // Continue to next tag
                        st = (*nt) ? nt + 1 : nt;
                    }
                    NANOSVG_FREE(buffer);
                }
            }

            image = nasvg__findDefsImage(p, hrefId);
            if (image) {
                image->x = usex;
                image->y = usey;
                nsvg__addDefsImageShape(p, image);
            }

        }
    }
}

static void nsvg__parseSVG(NSVGparser* p, const char** attr)
{
    int i;
    int width_flag = 0;
    int height_flag = 0;

    for (i = 0; attr[i]; i += 2) {
        if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "width") == 0) {
                svg_image_width = atoi(attr[i + 1]);
                svg_scale_image_width = p->image->width = nsvg__parseCoordinate(p, attr[i + 1], 0.0f, 0.0f);
                width_flag = 1;
            } else if (strcmp(attr[i], "height") == 0) {
                svg_image_height = atoi(attr[i + 1]);
                svg_scale_image_height = p->image->height = nsvg__parseCoordinate(p, attr[i + 1], 0.0f, 0.0f);
                height_flag = 1;
            } else if (strcmp(attr[i], "viewBox") == 0) {
                const char* s = attr[i + 1];
                char buf[64];
                s = nsvg__parseNumber(s, buf, 64);
                p->viewMinx = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                p->viewMiny = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                p->viewWidth = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                p->viewHeight = nsvg__atof(buf);
            } else if (strcmp(attr[i], "preserveAspectRatio") == 0) {
                if (strstr(attr[i + 1], "none") != 0) {
                    // No uniform scaling
                    p->alignType = NSVG_ALIGN_NONE;
                } else {
                    // Parse X align
                    if (strstr(attr[i + 1], "xMin") != 0)
                        p->alignX = NSVG_ALIGN_MIN;
                    else if (strstr(attr[i + 1], "xMid") != 0)
                        p->alignX = NSVG_ALIGN_MID;
                    else if (strstr(attr[i + 1], "xMax") != 0)
                        p->alignX = NSVG_ALIGN_MAX;
                    // Parse X align
                    if (strstr(attr[i + 1], "yMin") != 0)
                        p->alignY = NSVG_ALIGN_MIN;
                    else if (strstr(attr[i + 1], "yMid") != 0)
                        p->alignY = NSVG_ALIGN_MID;
                    else if (strstr(attr[i + 1], "yMax") != 0)
                        p->alignY = NSVG_ALIGN_MAX;
                    // Parse meet/slice
                    p->alignType = NSVG_ALIGN_MEET;
                    if (strstr(attr[i + 1], "slice") != 0)
                        p->alignType = NSVG_ALIGN_SLICE;
                }
            }
        }
    }

    if (!width_flag || !height_flag) {
        svg_image_width = svg_scale_image_width = p->image->width = 400;
        svg_image_height = svg_scale_image_height = p->image->height = 400;
        NANOSVG_PRINT("The svg source file does not set the width and heigh, set width and heigh to 400.\n");
    }
}

static void nsvg__parseGradient(NSVGparser* p, const char** attr, signed char type)
{
    int i;
    NSVGgradientData* grad = (NSVGgradientData*)NANOSVG_MALLOC(sizeof(NSVGgradientData));
    if (grad == NULL) return;
    memset(grad, 0, sizeof(NSVGgradientData));
    grad->units = NSVG_OBJECT_SPACE;
    grad->unitsFlag = 0;
    grad->type = type;
    if (grad->type == NSVG_PAINT_LINEAR_GRADIENT) {
        grad->linear.x1 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
        grad->linear.y1 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
        grad->linear.x2 = nsvg__coord(100.0f, NSVG_UNITS_PERCENT);
        grad->linear.y2 = nsvg__coord(0.0f, NSVG_UNITS_PERCENT);
    } else if (grad->type == NSVG_PAINT_RADIAL_GRADIENT) {
        grad->radial.cx = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
        grad->radial.cy = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
        grad->radial.r = nsvg__coord(50.0f, NSVG_UNITS_PERCENT);
    }

    nsvg__xformIdentity(grad->xform);

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strncpy(grad->id, attr[i+1], 63);
            grad->id[63] = '\0';
        } else if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "gradientUnits") == 0) {
                grad->unitsFlag = 1;
                if (strcmp(attr[i+1], "objectBoundingBox") == 0)
                    grad->units = NSVG_OBJECT_SPACE;
                else
                    grad->units = NSVG_USER_SPACE;
            } else if (strcmp(attr[i], "gradientTransform") == 0) {
                grad->xformFlag = 1;
                nsvg__parseTransform(grad->xform, attr[i + 1]);
            } else if (strcmp(attr[i], "cx") == 0) {
                grad->radial.cx = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "cy") == 0) {
                grad->radial.cy = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "r") == 0) {
                grad->radial.r = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "fx") == 0) {
                grad->radial.fx = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "fy") == 0) {
                grad->radial.fy = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "x1") == 0) {
                grad->linear.x1 = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "y1") == 0) {
                grad->linear.y1 = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "x2") == 0) {
                grad->linear.x2 = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "y2") == 0) {
                grad->linear.y2 = nsvg__parseCoordinateRaw(attr[i + 1]);
            } else if (strcmp(attr[i], "spreadMethod") == 0) {
                if (strcmp(attr[i+1], "pad") == 0)
                    grad->spread = NSVG_SPREAD_PAD;
                else if (strcmp(attr[i+1], "reflect") == 0)
                    grad->spread = NSVG_SPREAD_REFLECT;
                else if (strcmp(attr[i+1], "repeat") == 0)
                    grad->spread = NSVG_SPREAD_REPEAT;
            } else if (strcmp(attr[i], "xlink:href") == 0) {
                const char *href = attr[i+1];
                strncpy(grad->ref, href+1, 62);
                grad->ref[62] = '\0';
            }
        }
    }

    grad->next = p->gradients;
    p->gradients = grad;
}

static void nsvg__parseGradientStop(NSVGparser* p, const char** attr)
{
    NSVGattrib* curAttr = nsvg__getAttr(p);
    NSVGgradientData* grad;
    NSVGgradientStop* stop;
    int i, idx;

    curAttr->stopOffset = 0;
    curAttr->stopColor = 0;
    curAttr->stopOpacity = 1.0f;

    for (i = 0; attr[i]; i += 2) {
        nsvg__parseAttr(p, attr[i], attr[i + 1]);
    }

    // Add stop to the last gradient.
    grad = p->gradients;
    if (grad == NULL) return;

    grad->nstops++;
    grad->stops = (NSVGgradientStop*)NANOSVG_REALLOC(grad->stops, sizeof(NSVGgradientStop)*grad->nstops);
    if (grad->stops == NULL) return;

    // Insert
    idx = grad->nstops-1;
    for (i = 0; i < grad->nstops-1; i++) {
        if (curAttr->stopOffset < grad->stops[i].offset) {
            idx = i;
            break;
        }
    }
    if (idx != grad->nstops-1) {
        for (i = grad->nstops-1; i > idx; i--)
            grad->stops[i] = grad->stops[i-1];
    }

    stop = &grad->stops[idx];
    stop->color = curAttr->stopColor;
    stop->color |= (unsigned int)(curAttr->stopOpacity*255) << 24;
    stop->offset = curAttr->stopOffset;
}


static void nsvg__saveDefsString(NSVGparser* p, const char* el)
{
    char* start = (char*)el + 5;
    char* end = start;
    int deflen;

    // Save Defs string start pointer
    p->defsStart = start;

    while (memcmp(end++, "</defs>", 7) != 0);
    end--;
    deflen = end - start + 1;
    p->defsString = (char*)NANOSVG_MALLOC(((deflen + 3)>> 2) << 2);
    if (p->defsString == NULL) return;

    // Save entire Defs content
    memcpy(p->defsString, start, deflen);
    p->defsString[deflen - 1] = '\0';
}

static void nsvg_parseClipPath(NSVGparser* p, const char** attr)
{

    NSVGclipPathData* clipPath = (NSVGclipPathData*)NANOSVG_MALLOC(sizeof(NSVGclipPathData));
    if (clipPath == NULL) return;
    memset(clipPath, 0, sizeof(NSVGclipPathData));
    clipPath->units = NSVG_OBJECT_SPACE;

    for (int i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strncpy(clipPath->id, attr[i + 1], 63);
            clipPath->id[63] = '\0';
        }
    }

    clipPath->next = p->clipPaths;
    p->clipPaths = clipPath;
}

static void nsvg_parsePattern(NSVGparser* p, const char** attr, signed char type)
{
    int i;
    NSVGpatternData* pattern = (NSVGpatternData*)NANOSVG_MALLOC(sizeof(NSVGpatternData));
    if (pattern == NULL) return;
    memset(pattern, 0, sizeof(NSVGpatternData));
    pattern->units = NSVG_OBJECT_SPACE; // need to refine the enum
    pattern->type = type;

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "id") == 0) {
            strncpy(pattern->id, attr[i + 1], 63);
            pattern->id[63] = '\0';
        }
        else if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
           // char* test = (char*)attr[i];
            if (strcmp(attr[i], "patternUnits") == 0) {
                if (strcmp(attr[i + 1], "objectBoundingBox") == 0)
                    pattern->units = NSVG_OBJECT_SPACE;
                else
                    pattern->units = NSVG_USER_SPACE;
            }
            else if (strcmp(attr[i], "x") == 0) {
                pattern->x = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "y") == 0) {
                pattern->y = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "width") == 0) {
                pattern->width = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "height") == 0) {
                pattern->height = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "patternContentUnits") == 0) {
                //to do
            }
            else if (strcmp(attr[i], "patternTransform") == 0) {
                //to do
            }
            else if (strcmp(attr[i], "preserveAspectRatio") == 0) {
                //to do
            }
            else if (strcmp(attr[i], "viewBox") == 0) {
                const char* s = attr[i + 1];
                char buf[64];
                s = nsvg__parseNumber(s, buf, 64);
                pattern->viewMinx = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                pattern->viewMiny = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                pattern->viewWidth = nsvg__atof(buf);
                while (*s && (nsvg__isspace(*s) || *s == '%' || *s == ',')) s++;
                if (!*s) return;
                s = nsvg__parseNumber(s, buf, 64);
                pattern->viewHeight = nsvg__atof(buf);
            }
            else if (strcmp(attr[i], "xlink:href") == 0) {
                const char* href = attr[i + 1];
                strncpy(pattern->ref, href + 1, 62);
                pattern->ref[62] = '\0';
            }
        }
    }

    pattern->next = p->patterns;
    p->patterns = pattern;
}

static void nsvg__parseText(NSVGparser* p, const char** attr)
{
    int i;
    NSVGtextData* text = (NSVGtextData*)NANOSVG_MALLOC(sizeof(NSVGtextData));
    if (text == NULL) return;
    memset(text, 0, sizeof(NSVGtextData));

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0) {
            p->textFlag.textClassFlag = 1;
            char* str = (char*)attr[i + 1];
            char* out = strtok(str, " ");;
            strcpy(text->class_name, out);
        }
        else if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "x") == 0) {
                text->x = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "y") == 0) {
                text->y = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "width") == 0) {
                text->dx = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "height") == 0) {
                text->dy = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "rotate") == 0) {
                text->rotate = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
            else if (strcmp(attr[i], "font-family") == 0) {
                strncpy(text->font_family, attr[i + 1], 63);
                text->font_family[63] = '\0';
            }
            else if (strcmp(attr[i], "font-size") == 0) {
                text->fontSize = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
        }
    }
    if (p->text)
        NANOSVG_FREE(p->text);
    p->text = text;
}

static void nsvg__parseTextpath(NSVGparser* p, const char** attr)
{
    int i;
    NSVGtextpathData* textpath = (NSVGtextpathData*)NANOSVG_MALLOC(sizeof(NSVGtextpathData));
    if (textpath == NULL) return;
    memset(textpath, 0, sizeof(NSVGtextpathData));

    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "class") == 0) {
            p->textFlag.textpathClassFlag = 1;
            char* str = (char*)attr[i + 1];
            char* out = strtok(str, " ");;
            strcpy(textpath->class_name, out);
        }
        else if (!nsvg__parseAttr(p, attr[i], attr[i + 1])) {
            if (strcmp(attr[i], "spacing") == 0) {
                strcpy(textpath->spacing, attr[i + 1]);
            }
            else if (strcmp(attr[i], "method") == 0) {
                strcpy(textpath->method, attr[i + 1]);
            }
            else if (strcmp(attr[i], "xlink:href") == 0) {
                const char* href = attr[i + 1];
                strncpy(textpath->ref, href + 1, 62);
                textpath->ref[63] = '\0';
            }
            else if (strcmp(attr[i], "textLength") == 0) {
                textpath->textLength = nsvg__parseCoordinateRaw(attr[i + 1]);
            }
        }
    }
    if (p->textpath)
        NANOSVG_FREE(p->textpath);
    p->textpath = textpath;
}

static void nsvg_parseText_content(NSVGparser* p, const char* s) {
    strncpy(p->text->content, s, 128);
    p->text->content[127] = '\0';
    nsvg__addTextShape(p);
}

static void nsvg_parseTextpath_content(NSVGparser* p, const char* s) {
    strncpy(p->textpath->content, s, 128);
    p->text->content[127] = '\0';
    nsvg__addTextpathShape(p);
}

static void nsvg__startElement(void* ud, const char* el, const char** attr)
{
    NSVGparser* p = (NSVGparser*)ud;

    if (p->styleFlag) {
        NSVGstyle* style = (NSVGstyle*)NANOSVG_MALLOC(sizeof(NSVGstyle));
        if (style == NULL) return;
        memset(style, 0, sizeof(NSVGstyle));
        style->opacity = style->fillOpacity = style->strokeOpacity = 1;
        strcpy(style->name, el);
        nsvg__parseStyleAttribs(p, style, attr);

        if (p->style) {
            style->next = p->style;
            p->style = style;
        }
        else
            p->style = style;

        return;
    }

    if (p->defsFlag) {
        // Skip everything but gradients in defs
        if (strcmp(el, "linearGradient") == 0) {
            nsvg__parseGradient(p, attr, NSVG_PAINT_LINEAR_GRADIENT);
        } else if (strcmp(el, "radialGradient") == 0) {
            nsvg__parseGradient(p, attr, NSVG_PAINT_RADIAL_GRADIENT);
        } else if (strcmp(el, "stop") == 0) {
            nsvg__parseGradientStop(p, attr);
        } else if (strcmp(el, "symbol") == 0) {
            nsvg__parseSymbol(p, attr);
        } else if (strcmp(el, "pattern") == 0) {
            nsvg_parsePattern(p, attr, NSVG_PAINT_PATTERN);
            p->patternFlag = 1;
        } else if(strcmp(el, "path") == 0) {
            if(!p->patternFlag && !p->clipPathFlag)
                nsvg__parseDefsPath(p, attr);
        } else if (strcmp(el, "image") == 0) {
            nsvg__parseDefsimage(p, attr);
        } else if (strcmp(el, "style") == 0) {
            p->styleFlag = 1;
        }
        else if (strcmp(el, "clipPath") == 0) {
            nsvg_parseClipPath(p, attr);
            p->clipPathFlag = 1;
        }
        else {
            nsvg__parseDefsTag(p, el, attr);
        }

        if (!p->patternFlag && !p->clipPathFlag)
            return;
    }

    if (strcmp(el, "g") == 0) {
        nsvg__pushAttr(p);
        nsvg__parseAttribs(p, attr, el);
    } else if (strcmp(el, "animateTransform") == 0) {
        p->animateFlag = 1;
        nsvg__parseAnimate(p, attr);
    } else if (strcmp(el, "path") == 0) {
        if (p->pathFlag)    // Do not allow nested paths.
            return;
        nsvg__pushAttr(p);
        nsvg__parsePath(p, attr);
        nsvg__popAttr(p);
    } else if (strcmp(el, "rect") == 0) {
        nsvg__pushAttr(p);
        nsvg__parseRect(p, attr);
        nsvg__popAttr(p);
    } else if (strcmp(el, "circle") == 0) {
        nsvg__pushAttr(p);
        nsvg__parseCircle(p, attr);
        nsvg__popAttr(p);
    } else if (strcmp(el, "ellipse") == 0) {
        nsvg__pushAttr(p);
        nsvg__parseEllipse(p, attr);
        nsvg__popAttr(p);
    } else if (strcmp(el, "line") == 0)  {
        nsvg__pushAttr(p);
        nsvg__parseLine(p, attr);
        nsvg__popAttr(p);
    } else if (strcmp(el, "polyline") == 0)  {
        nsvg__pushAttr(p);
        nsvg__parsePoly(p, attr, 0);
        nsvg__popAttr(p);
    } else if (strcmp(el, "polygon") == 0)  {
        nsvg__pushAttr(p);
        nsvg__parsePoly(p, attr, 1);
        nsvg__popAttr(p);
    } else if (strcmp(el, "symbol") == 0)  {
        nsvg__parseSymbol(p, attr);
    } else if (strcmp(el, "use") == 0)  {
        nsvg__pushAttr(p);
        nsvg__parseUse(p, attr);
        nsvg__popAttr(p);
    } else  if (strcmp(el, "linearGradient") == 0) {
        nsvg__parseGradient(p, attr, NSVG_PAINT_LINEAR_GRADIENT);
    } else if (strcmp(el, "radialGradient") == 0) {
        nsvg__parseGradient(p, attr, NSVG_PAINT_RADIAL_GRADIENT);
    } else if (strcmp(el, "stop") == 0) {
        nsvg__parseGradientStop(p, attr);
    } else if (strcmp(el, "defs") == 0) {
        p->defsFlag = 1;
        insideDefs = 1;
        if (!emptyDefs) {
            nsvg__saveDefsString(p, el);
        }
    } else if (strcmp(el, "svg") == 0) {
        nsvg__parseSVG(p, attr);
    } else if (strcmp(el, "text") == 0) {
        p->textFlag.textTegFlag = 1;
        nsvg__pushAttr(p);
        nsvg__parseText(p, attr);
    } else if (strcmp(el, "textPath") == 0) {
        p->textFlag.textpathTegFlag = 1;
        nsvg__pushAttr(p);
        nsvg__parseTextpath(p, attr);
    } else if (strcmp(el, "style") == 0) {
        p->styleFlag = 1;
    }
}

static void nsvg__endElement(void* ud, const char* el)
{
    NSVGparser* p = (NSVGparser*)ud;

    if (strcmp(el, "g") == 0) {
        nsvg__popAttr(p);
        if (p->animateFlag) {
            NANOSVG_FREE(p->animateData);
            p->animateFlag = 0;
        }
    } else if (strcmp(el, "path") == 0) {
        p->pathFlag = 0;
    } else if (strcmp(el, "defs") == 0) {
        p->defsFlag = 0;
        insideDefs = 0;
    } else if (strcmp(el, "pattern") == 0) {
        p->patternFlag = 0;
    } else if (strcmp(el, "clipPath") == 0){
        p->clipPathFlag = 0;
    } else if (strcmp(el, "text") == 0) {
        p->textFlag.textTegFlag = 0;
        p->textFlag.textClassFlag = 0;
    } else if (strcmp(el, "textPath") == 0) {
        p->textFlag.textpathTegFlag = 0;
        p->textFlag.textpathClassFlag = 0;
    } else if (strcmp(el, "style") == 0) {
        p->styleFlag = 0;
    }
}

static void nsvg__content(void* ud, const char* s)
{
    NSVG_NOTUSED(ud);
    NSVG_NOTUSED(s);
    NSVGparser* p = (NSVGparser*)ud;

    if (p->textFlag.textpathTegFlag) {
        nsvg_parseTextpath_content(p, s);
        nsvg__popAttr(p);
    }
    else {
        if (p->textFlag.textTegFlag) {
            nsvg_parseText_content(p, s);
            nsvg__popAttr(p);
        }
    }
}

static void nsvg__imageBounds(NSVGparser* p, float* bounds)
{
    NSVGshape* shape;
    shape = p->image->shapes;
    if (shape == NULL) {
        bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0;
        return;
    }
    bounds[0] = shape->bounds[0];
    bounds[1] = shape->bounds[1];
    bounds[2] = shape->bounds[2];
    bounds[3] = shape->bounds[3];
    for (shape = shape->next; shape != NULL; shape = shape->next) {
        bounds[0] = nsvg__minf(bounds[0], shape->bounds[0]);
        bounds[1] = nsvg__minf(bounds[1], shape->bounds[1]);
        bounds[2] = nsvg__maxf(bounds[2], shape->bounds[2]);
        bounds[3] = nsvg__maxf(bounds[3], shape->bounds[3]);
    }
}

static float nsvg__viewAlign(float content, float container, int type)
{
    if (type == NSVG_ALIGN_MIN)
        return 0;
    else if (type == NSVG_ALIGN_MAX)
        return container - content;
    // mid
    return (container - content) * 0.5f;
}

/*
static void nsvg__scaleGradient(NSVGgradient* grad, float tx, float ty, float sx, float sy)
{
    float t[6];
    nsvg__xformSetTranslation(t, tx, ty);
    nsvg__xformMultiply (grad->xform, t);

    nsvg__xformSetScale(t, sx, sy);
    nsvg__xformMultiply (grad->xform, t);
}
*/

static void nsvg__scaleToViewbox(NSVGparser* p, const char* units)
{
    NSVGshape* shape;
    NSVGpath* path;
    float tx, ty, sx, sy, us, bounds[4], avgs;
    int i;
    float* pt;

    // Guess image size if not set completely.
    nsvg__imageBounds(p, bounds);

    if (p->viewWidth == 0) {
        if (p->image->width > 0) {
            p->viewWidth = p->image->width;
        } else {
            p->viewMinx = bounds[0];
            p->viewWidth = bounds[2] - bounds[0];
        }
    }
    if (p->viewHeight == 0) {
        if (p->image->height > 0) {
            p->viewHeight = p->image->height;
        } else {
            p->viewMiny = bounds[1];
            p->viewHeight = bounds[3] - bounds[1];
        }
    }
    if (p->image->width == 0)
        p->image->width = p->viewWidth;
    if (p->image->height == 0)
        p->image->height = p->viewHeight;

    tx = -p->viewMinx;
    ty = -p->viewMiny;
    sx = p->viewWidth > 0 ? p->image->width / p->viewWidth : 0;
    sy = p->viewHeight > 0 ? p->image->height / p->viewHeight : 0;
    // Unit scaling
    us = 1.0f / nsvg__convertToPixels(p, nsvg__coord(1.0f, nsvg__parseUnits(units)), 0.0f, 1.0f);

    // Fix aspect ratio
    if (p->alignType == NSVG_ALIGN_MEET || p->alignType == NSVG_ALIGN_NONE) {
        // fit whole image into viewbox
        sx = sy = nsvg__minf(sx, sy);
        tx += nsvg__viewAlign(p->viewWidth*sx, p->image->width, p->alignX) / sx;
        ty += nsvg__viewAlign(p->viewHeight*sy, p->image->height, p->alignY) / sy;
    } else if (p->alignType == NSVG_ALIGN_SLICE) {
        // fill whole viewbox with image
        sx = sy = nsvg__maxf(sx, sy);
        tx += nsvg__viewAlign(p->viewWidth*sx, p->image->width, p->alignX) / sx;
        ty += nsvg__viewAlign(p->viewHeight*sy, p->image->height, p->alignY) / sy;
    }

    // Transform
    sx *= us;
    sy *= us;
    avgs = (sx+sy) / 2.0f;
    for (shape = p->image->shapes; shape != NULL; shape = shape->next) {
        shape->bounds[0] = (shape->bounds[0] + tx) * sx;
        shape->bounds[1] = (shape->bounds[1] + ty) * sy;
        shape->bounds[2] = (shape->bounds[2] + tx) * sx;
        shape->bounds[3] = (shape->bounds[3] + ty) * sy;
        for (path = shape->paths; path != NULL; path = path->next) {
            path->bounds[0] = (path->bounds[0] + tx) * sx;
            path->bounds[1] = (path->bounds[1] + ty) * sy;
            path->bounds[2] = (path->bounds[2] + tx) * sx;
            path->bounds[3] = (path->bounds[3] + ty) * sy;
            for (i =0; i < path->npts; i++) {
                pt = &path->pts[i*2];
                pt[0] = (pt[0] + tx) * sx;
                pt[1] = (pt[1] + ty) * sy;
            }
        }
        if (shape->clipPath != NULL) {
            if (shape->clipPath->premul_flag == 0)
            {
                shape->clipPath->premul_flag = 1;
                for (path = shape->clipPath->path; path != NULL; path = path->next) {
                    path->bounds[0] = (path->bounds[0] + tx) * sx;
                    path->bounds[1] = (path->bounds[1] + ty) * sy;
                    path->bounds[2] = (path->bounds[2] + tx) * sx;
                    path->bounds[3] = (path->bounds[3] + ty) * sy;
                    for (i = 0; i < path->npts; i++) {
                        pt = &path->pts[i * 2];
                        pt[0] = (pt[0] + tx) * sx;
                        pt[1] = (pt[1] + ty) * sy;
                    }
                }
            }
        }
        if (shape->fill.text)
        {
            shape->fill.text->x = shape->fill.text->x * sx;
            shape->fill.text->y = shape->fill.text->y * sy;
        }
        if (shape->fill.image)
        {
            shape->fill.image->x = shape->fill.image->x * sx;
            shape->fill.image->y = shape->fill.image->y * sy;
        }

        shape->strokeWidth *= avgs;
        shape->strokeDashOffset *= avgs;
        for (i = 0; i < shape->strokeDashCount; i++)
            shape->strokeDashArray[i] *= avgs;
    }
}

static void nsvg__createGradients(NSVGparser* p)
{
    NSVGshape* shape;

    for (shape = p->image->shapes; shape != NULL; shape = shape->next) {
        if (shape->fill.type == NSVG_PAINT_UNDEF) {
            if (shape->fillGradient[0] != '\0') {
                float inv[6], localBounds[4];
                nsvg__xformInverse(inv, shape->xform);
                nsvg__getLocalBounds(localBounds, shape, inv);
                shape->fill.gradient = nsvg__createGradient(p, shape->fillGradient, localBounds, shape->xform, &shape->fill.type);
            }
            if (shape->fill.type == NSVG_PAINT_UNDEF) {
                shape->fill.type = NSVG_PAINT_NONE;
            }
        }
        if (shape->stroke.type == NSVG_PAINT_UNDEF) {
            if (shape->strokeGradient[0] != '\0') {
                float inv[6], localBounds[4];
                nsvg__xformInverse(inv, shape->xform);
                nsvg__getLocalBounds(localBounds, shape, inv);
                shape->stroke.gradient = nsvg__createGradient(p, shape->strokeGradient, localBounds, shape->xform, &shape->stroke.type);
            }
            if (shape->stroke.type == NSVG_PAINT_UNDEF) {
                shape->stroke.type = NSVG_PAINT_NONE;
            }
        }
    }
}

static void nsvg__createPatterns(NSVGparser* p)
{
    NSVGshape* shape;

    for (shape = p->image->shapes; shape != NULL; shape = shape->next) {
        if (shape->fill.type == NSVG_PAINT_UNDEF) {
            if (shape->fillPattern[0] != '\0') {
                shape->fill.pattern = nsvg__createPattern(p, shape->fillPattern, &shape->fill.type);
            }
        }
    }
}

NSVGimage* nsvgParse(char* input, const char* units, float dpi)
{
    NSVGparser* p;
    NSVGimage* ret = 0;

    p = nsvg__createParser();
    if (p == NULL) {
        return NULL;
    }
    p->dpi = dpi;

    nsvg__parseXML(input, nsvg__startElement, nsvg__endElement, nsvg__content, p);

    // Create patterns after all definitions have been parsed
    nsvg__createPatterns(p);

    // Create gradients after all definitions have been parsed
    nsvg__createGradients(p);

    // Scale to viewBox
    nsvg__scaleToViewbox(p, units);

    ret = p->image;
    p->image = NULL;

    nsvg__deleteParser(p);

    return ret;
}

#if 0
NSVGimage* nsvgParseFromFile(const char* filename, const char* units, float dpi)
{
    FILE* fp = NULL;
    size_t size;
    char* data = NULL;
    NSVGimage* image = NULL;

    fp = fopen(filename, "rb");
    if (!fp) goto error;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (char*)NANOSVG_MALLOC(size+1);
    if (data == NULL) goto error;
    if (fread(data, 1, size, fp) != size) goto error;
    data[size] = '\0';    // Must be null terminated.
    fclose(fp);
    image = nsvgParse(data, units, dpi);
    NANOSVG_FREE(data);

    return image;

error:
    if (fp) fclose(fp);
    if (data) NANOSVG_FREE(data);
    if (image) nsvgDelete(image);
    return NULL;
}
#endif

NSVGpath* nsvgDuplicatePath(NSVGpath* p)
{
    NSVGpath* res = NULL;

    if (p == NULL)
        return NULL;

    res = (NSVGpath*)NANOSVG_MALLOC(sizeof(NSVGpath));
    if (res == NULL) goto error;
    memset(res, 0, sizeof(NSVGpath));

    res->pts = (float*)NANOSVG_MALLOC(p->npts*2*sizeof(float));
    if (res->pts == NULL) goto error;
    memcpy(res->pts, p->pts, p->npts * sizeof(float) * 2);
    res->npts = p->npts;

    memcpy(res->bounds, p->bounds, sizeof(p->bounds));

    res->closed = p->closed;

    return res;

error:
    if (res != NULL) {
        NANOSVG_FREE(res->pts);
        NANOSVG_FREE(res);
    }
    return NULL;
}

void nsvgDelete(NSVGimage* image)
{
    NSVGshape *snext, *shape;
    if (image == NULL) return;
    shape = image->shapes;
    while (shape != NULL) {
        snext = shape->next;
        nsvg__deletePaths(shape->paths);
        nsvg__deleteVGLitePaths(shape->vgPathOfCharacter);
        nsvg__deletePaint(&shape->fill);
        nsvg__deletePaint(&shape->stroke);
        nsvg__deleteAnimate(shape->animate);
        nsvg__deleteClipPath(shape, shape->clipPath, 1);
        NANOSVG_FREE(shape);
        shape = snext;
    }
    NANOSVG_FREE(image);
}

void nsvg_parseStyleContent(char* s, char* attr[])
{
    char *name, *value;
    int nattr = 0;
    char* cur = s;
    int end_flag = 0;

    while (*cur)
    {
        //Split the string to obtain name and value
        name = cur;
        while (*cur != ':')
            cur++;
        *cur = '\0';
        cur++;

        value = cur;

        char* tem_end = cur;
        while (*tem_end  && *tem_end != ';')
            tem_end++;

        if (!*tem_end)
            end_flag = 1;

        if(*tem_end && *tem_end == ';')
            *tem_end = '\0';

        if (name && value)
        {
            attr[nattr++] = name;
            attr[nattr++] = value;
        }

        if (end_flag)
            break;

        tem_end++;
        cur = tem_end;
    }
    attr[nattr++] = 0;
    attr[nattr++] = 0;
}


static void nsvg__parseStyleElement(char* s,
                                    void (*startelCb)(void* ud, const char* el, const char** attr),
                                    void* ud)
{
    char *attr[NSVG_XML_MAX_ATTRIBS];
    char *name, *parse_content, *saveptr;
    char *cur = s;
    char* media_start = NULL;
    char* media_end = NULL;

    // Skip white space
    while (*s && nsvg__isspace(*s)) s++;

    while (*cur)
    {
        if (*cur == '.')
        {
            //Split the string to obtain the substrings in symbol '{}'
            char* tem_start = cur;
            while (*tem_start != '{')
                tem_start++;

            char* tem_end = tem_start;
            while (*tem_end != '}')
                tem_end++;

            *tem_start = '\0';
            tem_start++;
            parse_content = tem_start;
            *tem_end = '\0';

            name = cur;
            name++;

            int j = sizeof(name);
            for (int i = 0; i < j; i++)
            {
                if(name[i] == ' ')
                    name[i] = '\0';
            }

            nsvg_parseStyleContent(parse_content, attr);

            tem_end++;
            cur = tem_end;

            (*startelCb)(ud, name, (const char **)attr);
        }
        else if (*cur == '@' && *(cur + 1) == 'm' && *(cur + 2) == 'e' && *(cur + 3) == 'd' && *(cur + 4) == 'i' && *(cur + 5) == 'a')
        {
            media_start = cur;
            int brackets = 0;
            while (*cur != '{')
                cur++;
            brackets++;
            while (brackets)
            {
                cur++;
                if (*cur == '}')
                    brackets--;
                else if (*cur == '{')
                    brackets++;
            }
            media_end = cur;
            cur++;
        }
        else
        {
            name = strtok_s(cur, " {", &saveptr);
            parse_content = strtok_s(NULL, "}", &saveptr);
            nsvg_parseStyleContent(parse_content, attr);

            cur = saveptr;
            (*startelCb)(ud, name, (const char **)attr);
        }

        while (*cur && nsvg__isspace(*cur)) cur++;
    }

    if (media_start)
    {
        NSVGparser* p = (NSVGparser*)ud;
        NSVGstyle* style_tem = p->style;
        char* media_tem = media_start;

        strtok_s(media_tem, "(", &saveptr);
        saveptr = strtok_s(NULL, ")", &saveptr);
        saveptr = strtok_s(saveptr, ":", &parse_content);

        while (*parse_content && nsvg__isspace(*parse_content)) parse_content++;
        char nums[64];
        int j = 0;
        while (*parse_content < ':' && *parse_content >',')
        {
            nums[j] = *parse_content;
            parse_content++;
            j++;
        }
        nums[j] = '\0';

        float value = nsvg__atof(nums);

        while (style_tem)
        {
            char* cur_tem = media_start;
            for (int i = 0; cur_tem+i < media_end; i++)
            {
                int j = 0;
                if (*(cur_tem + i) == style_tem->name[j])
                {
                    while (style_tem->name[j] != '\0')
                    {
                        if (*(cur_tem + i) != style_tem->name[j])
                            break;
                        j++;
                        i++;
                    }
                    if (style_tem->name[j] == '\0')
                    {
                        style_tem->mediaFlag = 1;
                        if (strcmp(saveptr, "min-width") == 0)
                            style_tem->minWidth = value;
                        else if (strcmp(saveptr, "min-height") == 0)
                            style_tem->minHeight = value;
                        else if (strcmp(saveptr, "max-width") == 0)
                            style_tem->maxWidth = value;
                        else if (strcmp(saveptr, "max-height") == 0)
                            style_tem->maxHeight = value;
                        break;
                    }
                }
            }
            style_tem = style_tem->next;
        }
    }

}

void nsvgDetectStyle(char* s,
                     void (*startelCb)(void* ud, const char* el, const char** attr),
                     void* ud)
{
    NSVGparser* p = (NSVGparser*)ud;
    if (p->styleFlag == 1)
        nsvg__parseStyleElement(s, startelCb, ud);
}

int nsvgBase64Decode(const char* input, unsigned char** output, size_t* output_len)
{
    size_t len = strlen(input);
    if (len % 4 != 0) {
        return -1;
    }

    size_t padding = 0;
    if (input[len - 1] == '=') padding++;
    if (input[len - 2] == '=') padding++;

    *output_len = (len / 4) * 3 - padding;
    *output = (unsigned char*)NANOSVG_MALLOC(*output_len);
    if (*output == NULL) {
        return -1;
    }

    static const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char temp[4];
    int j = 0;
    for (size_t i = 0; i < len; i += 4) {
        temp[0] = strchr(base64chars, input[i]) - base64chars;
        temp[1] = strchr(base64chars, input[i + 1]) - base64chars;
        temp[2] = strchr(base64chars, input[i + 2]) - base64chars;
        temp[3] = strchr(base64chars, input[i + 3]) - base64chars;

        (*output)[j++] = (temp[0] << 2) | (temp[1] >> 4);
        if (temp[2] < 64) {
            (*output)[j++] = (temp[1] << 4) | (temp[2] >> 2);
            if (temp[3] < 64) {
                (*output)[j++] = (temp[2] << 6) | temp[3];
            }
        }
    }

    return 0;
}

void nsvgBase64IHDR(unsigned char* input, int* weight, int *height)
{
    for (int i = 0; i < 32; i++)
    {
        if (input[i] == 'I' && input[i + 1] == 'H' && input[i + 2] == 'D' && input[i + 3] == 'R')
        {
            *weight = input[i + 7];
            *height = input[i + 11];
            break;
        }
    }
    return;
}

#endif // NANOSVG_IMPLEMENTATION

#endif // NANOSVG_H
