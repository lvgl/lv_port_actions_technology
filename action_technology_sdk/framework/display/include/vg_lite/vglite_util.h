/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VG_LITE_VGLITE_UTIL_H_
#define VG_LITE_VGLITE_UTIL_H_

#ifdef CONFIG_VG_LITE

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <display/graphic_buffer.h>
#include "vg_lite.h"

/**********************
 *      TYPEDEFS
 **********************/
#define RAD(d)     ((float)(d) * 3.141592654/180.0)
#define RAD10(d)   ((float)(d) * 3.141592654/1800.0)

/**********************
 *      TYPEDEFS
 **********************/
/*
 * Euler angle rotation order around model's dynamic axis
 */
typedef enum rotate_order {
	ROT_ZYX = 0,
	ROT_ZXY,
	ROT_YZX,
	ROT_YXZ,
	ROT_XYZ,
	ROT_XZY,

	NUM_ROT_ORDERS,
} rotate_order_e;

typedef struct vertex_rec {
	float x;
	float y;
	float z;
} vertex_t;

typedef struct normal_rec {
	float x;
	float y;
	float z;
} normal_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 * Buffer functions
 **********************/
/**
 * @brief Convert display hal pixel format to vglite buffer format
 *
 * @param hal_format display hal pixel format
 * @param bits_per_pixel optional address to store the bits_per_pixel of format.
 *
 * @retval vglite buffer format
 */
int vglite_buf_format_from_hal(uint32_t hal_format, uint8_t *bits_per_pixel);

/**
 * @brief Convert vglite buffer format to display hal pixel format
 *
 * @param format vglite buffer cf.
 * @param bits_per_pixel optional address to store the bits_per_pixel of format.
 *
 * @retval display hal pixel format
 */
uint32_t vglite_buf_format_to_hal(vg_lite_buffer_format_t format, uint8_t *bits_per_pixel);

/*
 * Query vglite buffer is valid or not
 *
 * @param vgbuf pointer to structure vg_lite_buffer
 *
 * @retval query result
 */
bool vglite_buf_is_valid(vg_lite_buffer_t *vgbuf);

/*
 * Map a vglite buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer
 * @param data buffer address to map
 * @param width width of buffer in pixels
 * @param height height of buffer in pixels
 * @param stride number of bytes to move from one line in the buffer to the next line
 * @param format buffer format
 *
 * @retval 0 on success else negative code
 */
int vglite_buf_map(vg_lite_buffer_t *vgbuf, void *data, uint16_t width, uint16_t height,
			uint16_t stride, vg_lite_buffer_format_t format);

/*
 * Map a vglite buffer from graphic buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer
 * @param gbuf pointer to structure graphic_buffer_t
 *
 * @retval 0 on success else negative code
 */
int vglite_buf_map_graphic(vg_lite_buffer_t *vgbuf, const graphic_buffer_t *gbuf);

/*
 * Unmap a vglite buffer
 *
 * @param vgbuf pointer to structure vg_lite_buffer
 *
 * @retval 0 on success else negative code
 */
int vglite_buf_unmap(vg_lite_buffer_t *vgbuf);

/**********************
 * Matrix functions
 **********************/
/*
 * Compute rotate matrix around x-axis to the world coordinate
 *
 * @param rx Euler angle in radian around X axis
 * @param matrix store the rotate matrix result
 *
 * @retval N/A
 */
void matrix_rotate_x(float rx, vg_lite_matrix_t *matrix);

/*
 * Compute rotate matrix around x-axis to the world coordinate
 *
 * @param ry Euler angle in radian around Y axis
 * @param matrix store the rotate matrix result
 *
 * @retval N/A
 */
void matrix_rotate_y(float ry, vg_lite_matrix_t *matrix);

/*
 * Compute rotate matrix around x-axis to the world coordinate
 *
 * @param rz Euler angle in radian around Z axis
 * @param matrix store the rotate matrix result
 *
 * @retval N/A
 */
void matrix_rotate_z(float rz, vg_lite_matrix_t *matrix);

/*
 * Compute rotate matrix to the world coordinate
 *
 * The rotation from model coordinate to world coordinate around model's dynamic axis.
 *
 * @param rx Euler angle in radian around X axis
 * @param ry Euler angle in radian around Y axis
 * @param rz Euler angle in radian around Z axis
 * @param order Rotation order, see enum rotate_order
 * @param matrix store the rotate matrix result
 *
 * @retval N/A
 */
void matrix_rotate(float rx, float ry, float rz, uint8_t order, vg_lite_matrix_t *matrix);

/*
 * Compute matrix multiplication
 *
 * @param matrix_left pointer to the left multiplication matrix
 * @param matrix_right pointer to the right multiplication matrix
 * @param result store the multiplication result
 *
 * @retval N/A
 */
void matrix_multiply(const vg_lite_matrix_t *matrix_left, const vg_lite_matrix_t *matrix_right, vg_lite_matrix_t *result);

/*
 * Compute left multiply translate matrix
 *
 * @param tx X translate
 * @param ty Y translate
 * @param matrix_right right multiplied matrix
 * @param matrix store the matrix result
 *
 * @retval N/A
 */
void matrix_translate_left(float tx, float ty,
		const vg_lite_matrix_t *matrix_right, vg_lite_matrix_t *matrix);

/*
 * Compute affine transformation matrix for blit
 *
 * @param w image width
 * @param h image height
 * @param v0 1st vertex in counterclock wise
 * @param v1 2nd vertex in counterclock wise
 * @param v2 3rd vertex in counterclock wise
 * @param v3 4th vertex in counterclock wise
 * @param matrix store the result matrix
 *
 * @retval N/A
 */
void matrix_affine_blit(int w, int h, vertex_t *v0, vertex_t *v1, vertex_t *v2, vertex_t *v3, vg_lite_matrix_t *matrix);

/*
 * Compute perspective transformation matrix for blit
 *
 * @param w image width
 * @param h image height
 * @param v0 1st vertex in counterclock wise
 * @param v1 2nd vertex in counterclock wise
 * @param v2 3rd vertex in counterclock wise
 * @param v3 4th vertex in counterclock wise
 * @param matrix store the result matrix
 *
 * @retval N/A
 */
void matrix_perspective_blit(int w, int h, vertex_t *v0, vertex_t *v1, vertex_t *v2, vertex_t *v3, vg_lite_matrix_t *matrix);

/*
 * Compute full transformation matrix for blit
 *
 * @param rect image bounding rectangle
 * @param pivots image pivot coordinate in pixels relative to top-left corner of the image
 * @param angles image Euler angles around X/Y/Z axis respectively in radian in ZYX order
 * @param scale scaling factor
 * @param camera camera position in display coordinates, [0] x; [1] y; [2] distance (>= 0)
 * @param result store the result vertices
 * @param matrix store the result matrix
 *
 * @retval N/A
 */
void matrix_transform_rect(vg_lite_rectangle_t *rect,
		float pivots[3], float angles[3], float scale, float camera[3],
		vertex_t result[4], vg_lite_matrix_t *matrix);

/*
 * Compute full transformation matrix of given rotate order for blit
 *
 * @param rect image bounding rectangle
 * @param pivots image pivot coordinate in pixels relative to top-left corner of the image
 * @param angles image Euler angles around X/Y/Z axis respectively in radian
 * @param rotate_order axis rotation order, see @rotate_order_e
 * @param scale scaling factor
 * @param camera camera position in display coordinates, [0] x; [1] y; [2] distance (>= 0)
 * @param result store the result vertices
 * @param matrix store the result matrix
 *
 * @retval N/A
 */
void matrix_transform_rect2(vg_lite_rectangle_t *rect,
		float pivots[3], float angles[3], uint8_t rotate_order,
		float scale, float camera[3],
		vertex_t result[4], vg_lite_matrix_t *matrix);

/*
 * Compute rotated vertex
 *
 * @param result store the rotated vertex result
 * @param rotate rotate matrix
 * @param vertex the vertex to rotate
 * @param scale scale factor
 * @param translate translate factor
 *
 * @retval N/A
 */
void transfrom_rotate(vertex_t *result, vg_lite_matrix_t *rotate, const vertex_t *vertex, const vertex_t *scale, const vertex_t *translate);

/*
 * Compute perspective vertex
 *
 * @param vertex vertex to do perspective transformation
 * @param camera_x camera x coordinate
 * @param camera_y camera y coordinate
 * @param camera_distance camera distance
 *
 * @retval N/A
 */
void transfrom_perspective(vertex_t *vertex, float camera_x, float camera_y, float camera_distance);

/*
 * Compute rotated normal value
 *
 * @param rotate rotate matrix
 * @param nVec normal
 *
 * @retval the result
 */
float transfrom_normal_z(vg_lite_matrix_t *rotate, const normal_t *normal);

#endif /* CONFIG_VG_LITE */
#endif /* VG_LITE_VGLITE_UTIL_H_ */
