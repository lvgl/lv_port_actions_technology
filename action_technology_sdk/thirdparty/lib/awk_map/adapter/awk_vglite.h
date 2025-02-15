/**
 * @file lv_vglite_utils.h
 *
 */

/**
 * MIT License
 *
 * Copyright 2022, 2023 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next paragraph)
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef LV_AWK_VGLITE_UTILS_H
#define LV_AWK_VGLITE_UTILS_H
#include "vg_lite.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

typedef struct vg_lite_buffer_addr_t{
        vg_lite_pointer handle;                 /*! The memory handle of the buffer's memory as allocated by the VGLite kernel. */
        vg_lite_pointer memory;                 /*! The logical pointer to the buffer's memory for the CPU. */
        vg_lite_uint32_t address;               /*! The address to the buffer's memory for the hardware. */
        vg_lite_yuvinfo_t yuv; 
        vg_lite_buffer_format_t format;
        vg_lite_int32_t width;                  /*! Width of the buffer in pixels. */
        vg_lite_int32_t height;                 /*! Height of the buffer in pixels. */
        vg_lite_int32_t stride;                 /*! The number of bytes to move from one line in the buffer to the next line. */
}vg_lite_buffer_addr_t;


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_VGLITE_UTILS_H*/
