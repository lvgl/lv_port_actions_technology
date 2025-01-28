
/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file vglite_renderer.h
 *
 */

#ifndef __VGLITE_RENDERER_H__
#define __VGLITE_RENDERER_H__

#ifdef CONFIG_VG_LITE

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <vg_lite/vg_lite.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct NSVGimage;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Parses SVG file from a null terminated string
 *
 * Important note: string will be changed.
 *
 * @param input SVG file stream appended with a null terminated char as string format
 * @param pwidth store the width of SVG image
 * @param pheight store the height of SVG image
 *
 * @revtal SVG image as paths.
 */
struct NSVGimage* parseSVGImage(char* input, int *pwidth, int *pheight);

/**
 * @brief Parses SVG file
 *
 * Important note: string will be changed.
 *
 * @param filename SVG file path
 * @param pwidth store the width of SVG image
 * @param pheight store the height of SVG image
 *
 * @revtal SVG image as paths.
 */
struct NSVGimage* parseSVGImageFromFile(const char* filename, int *pwidth, int *pheight);

/**
 * @brief Render SVG image
 *
 * Important note: matrix will be changed.
 *
 * @param target target buffer to render
 * @param image pointer to structure NSVGimage
 * @param matrix_p transform matrix
 *
 * @revtal 0 on success else other codes.
 */
int renderSVGImage(vg_lite_buffer_t *target, struct NSVGimage *image, vg_lite_matrix_t *matrix_p);

/**
 * @brief Delete SVG image
 *
 * @param image pointer to structure NSVGimage
 *
 * @revtal N/A.
 */
void deleteSVGImage(struct NSVGimage *image);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* CONFIG_VG_LITE */
#endif /* __VGLITE_RENDERER_H__ */
