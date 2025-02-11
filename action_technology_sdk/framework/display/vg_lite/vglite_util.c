/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_VG_LITE

/*********************
 *      INCLUDES
 *********************/
#include <errno.h>
#include <math.h>
#include <string.h>
#include <display/display_hal.h>
#include "vglite_util.h"

/*********************
 *      DEFINES
 *********************/
#ifdef _WIN32
#  define VGLITE_MEM_ALIGNMENT 1
#else
#  define VGLITE_MEM_ALIGNMENT 64
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef void (*matrix_rotate_fn_t)(float, float, float, vg_lite_matrix_t *);

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool vglite_buf_format_is_opaque(vg_lite_buffer_format_t format);

static void matrix_rotate_zyx(float rx, float ry, float rz, vg_lite_matrix_t *matrix);
static void matrix_rotate_zxy(float rx, float ry, float rz, vg_lite_matrix_t *matrix);
static void matrix_rotate_yzx(float rx, float ry, float rz, vg_lite_matrix_t *matrix);
static void matrix_rotate_yxz(float rx, float ry, float rz, vg_lite_matrix_t *matrix);
static void matrix_rotate_xyz(float rx, float ry, float rz, vg_lite_matrix_t *matrix);
static void matrix_rotate_xzy(float rx, float ry, float rz, vg_lite_matrix_t *matrix);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 * Buffer functions
 **********************/
int vglite_buf_format_from_hal(uint32_t hal_format, uint8_t *bits_per_pixel)
{
	int format = -1;
	uint8_t bpp = 0;

	switch(hal_format) {
	case HAL_PIXEL_FORMAT_RGB_565:
		format = VG_LITE_BGR565;
		bpp = 16;
		break;
	case HAL_PIXEL_FORMAT_ARGB_8888:
		format = VG_LITE_BGRA8888;
		bpp = 32;
		break;
	case HAL_PIXEL_FORMAT_XRGB_8888:
		format = VG_LITE_BGRX8888;
		bpp = 32;
		break;
	case HAL_PIXEL_FORMAT_RGB_888:
		format = VG_LITE_BGR888;
		bpp = 24;
		break;
	case HAL_PIXEL_FORMAT_BGR_888:
		format = VG_LITE_RGB888;
		bpp = 24;
		break;
	case HAL_PIXEL_FORMAT_ARGB_8565:
		format = VG_LITE_BGRA5658;
		bpp = 24;
		break;
	case HAL_PIXEL_FORMAT_ARGB_1555:
		format = VG_LITE_BGRA5551;
		bpp = 16;
		break;
	case HAL_PIXEL_FORMAT_A8:
		format = VG_LITE_A8;
		bpp = 8;
		break;
	case HAL_PIXEL_FORMAT_A4_LE:
		format = VG_LITE_A4;
		bpp = 4;
		break;
	case HAL_PIXEL_FORMAT_I8:
		format = VG_LITE_INDEX_8;
		bpp = 8;
		break;
	default:
		break;
	}

	if (bits_per_pixel) {
		*bits_per_pixel = bpp;
	}

	return format;
}

uint32_t vglite_buf_format_to_hal(vg_lite_buffer_format_t format, uint8_t *bits_per_pixel)
{
	uint32_t hal_format = 0;
	uint8_t bpp = 0;

	switch (format) {
	case VG_LITE_BGR565:
		hal_format = HAL_PIXEL_FORMAT_RGB_565;
		bpp = 16;
		break;
	case VG_LITE_BGRA8888:
		hal_format = HAL_PIXEL_FORMAT_ARGB_8888;
		bpp = 32;
		break;
	case VG_LITE_BGRX8888:
		hal_format = HAL_PIXEL_FORMAT_XRGB_8888;
		bpp = 32;
		break;
	case VG_LITE_RGB888:
		hal_format = HAL_PIXEL_FORMAT_BGR_888;
		bpp = 24;
		break;
	case VG_LITE_BGR888:
		hal_format = HAL_PIXEL_FORMAT_RGB_888;
		bpp = 24;
		break;
	case VG_LITE_BGRA5658:
		hal_format = HAL_PIXEL_FORMAT_ARGB_8565;
		bpp = 24;
		break;
	case VG_LITE_BGRA5551:
		hal_format = HAL_PIXEL_FORMAT_ARGB_1555;
		bpp = 16;
		break;
	case VG_LITE_A8:
		hal_format = HAL_PIXEL_FORMAT_A8;
		bpp = 8;
		break;
	case VG_LITE_A4:
		hal_format = HAL_PIXEL_FORMAT_A4_LE;
		bpp = 4;
		break;
	case VG_LITE_INDEX_8:
		hal_format = HAL_PIXEL_FORMAT_I8;
		bpp = 8;
		break;
	case VG_LITE_INDEX_4:
	case VG_LITE_INDEX_2:
	case VG_LITE_INDEX_1:
	default:
		break;
	}

	if (bits_per_pixel) {
		*bits_per_pixel = bpp;
	}

	return hal_format;
}

bool vglite_buf_is_valid(vg_lite_buffer_t *vgbuf)
{
	return (vgbuf == NULL || vgbuf->handle == NULL || vgbuf->memory == NULL) ? false : true;
}

int vglite_buf_map(vg_lite_buffer_t *vgbuf, void *data, uint16_t width, uint16_t height,
			uint16_t stride, vg_lite_buffer_format_t format)
{
	/* Some source buffer format does not require 64 bytes alignment */
	if ((uintptr_t)data & (VGLITE_MEM_ALIGNMENT - 1)) {
		SYS_LOG_DBG("vgbuf %p not %d aligned", data, VGLITE_MEM_ALIGNMENT);
	}

	if (format == VG_LITE_RGBA8888_ETC2_EAC && (stride & 0x3))
		return -EINVAL;

	memset(vgbuf, 0, sizeof(*vgbuf));
	vgbuf->format = format;
	vgbuf->memory = data;
	vgbuf->image_mode = VG_LITE_NORMAL_IMAGE_MODE;
	vgbuf->transparency_mode = vglite_buf_format_is_opaque(format) ?
			VG_LITE_IMAGE_OPAQUE : VG_LITE_IMAGE_TRANSPARENT;
	vgbuf->width = width;
	vgbuf->height = height;
	vgbuf->stride = stride;
	vgbuf->tiled = (format == VG_LITE_RGBA8888_ETC2_EAC) ?
			VG_LITE_TILED : VG_LITE_LINEAR;

	if (vg_lite_map(vgbuf, VG_LITE_MAP_USER_MEMORY, -1) != VG_LITE_SUCCESS)
		return -EINVAL;

	return 0;
}

int vglite_buf_map_graphic(vg_lite_buffer_t *vgbuf, const graphic_buffer_t *gbuf)
{
	int format = vglite_buf_format_from_hal(graphic_buffer_get_pixel_format(gbuf), NULL);
	if (format < 0)
		return -EINVAL;

	return vglite_buf_map(vgbuf, (void *)gbuf->data,
				graphic_buffer_get_width(gbuf), graphic_buffer_get_height(gbuf),
				graphic_buffer_get_bytes_per_line(gbuf), format);
}

int vglite_buf_unmap(vg_lite_buffer_t *vgbuf)
{
	if (!vglite_buf_is_valid(vgbuf))
		return -EINVAL;

	return (vg_lite_unmap(vgbuf) == VG_LITE_SUCCESS) ? 0 : -EINVAL;
}

/**********************
 * Matrix functions
 **********************/
void matrix_rotate_x(float rx, vg_lite_matrix_t *matrix)
{
	matrix->m[0][0] = 1;
	matrix->m[0][1] = 0;
	matrix->m[0][2] = 0;
	matrix->m[1][0] = 0;
	matrix->m[1][1] = cos(rx);
	matrix->m[1][2] = -sin(rx);
	matrix->m[2][0] = 0;
	matrix->m[2][1] = sin(rx);
	matrix->m[2][2] = cos(rx);
}

void matrix_rotate_y(float ry, vg_lite_matrix_t *matrix)
{
	matrix->m[0][0] = cos(ry);
	matrix->m[0][1] = 0;
	matrix->m[0][2] = sin(ry);
	matrix->m[1][0] = 0;
	matrix->m[1][1] = 1;
	matrix->m[1][2] = 0;
	matrix->m[2][0] = -sin(ry);
	matrix->m[2][1] = 0;
	matrix->m[2][2] = cos(ry);
}

void matrix_rotate_z(float rz, vg_lite_matrix_t *matrix)
{
	matrix->m[0][0] = cos(rz);
	matrix->m[0][1] = -sin(rz);
	matrix->m[0][2] = 0;
	matrix->m[1][0] = sin(rz);
	matrix->m[1][1] = cos(rz);
	matrix->m[1][2] = 0;
	matrix->m[2][0] = 0;
	matrix->m[2][1] = 0;
	matrix->m[2][2] = 1;
}

void matrix_rotate(float rx, float ry, float rz, uint8_t order, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in ZYX order.
	 *
	 * Point rotate matrix (Rx) around X axis:
	 *     [  1    0        0     ]
	 *     [  0  cos(rx) -sin(rx) ]
	 *     [  0  sin(rx)  cos(rx) ]
	 *
	 * Point rotate matrix (Ry) around Y axis:
	 *     [  cos(ry)  0  sin(ry) ]
	 *     [    0      1    0     ]
	 *     [ -sin(ry)  0  cos(ry) ]
	 *
	 * Point rotate matrix (Rz) around Z axis:
	 *     [  cos(rz) -sin(rz)  0 ]
	 *     [  sin(rz)  cos(rz)  0 ]
	 *     [    0        0      1 ]
	 *
	 */

	static const matrix_rotate_fn_t rotate_vtbl[] = {
		matrix_rotate_zyx,
		matrix_rotate_zxy,
		matrix_rotate_yzx,
		matrix_rotate_yxz,
		matrix_rotate_xyz,
		matrix_rotate_xzy,
	};

	if (order < NUM_ROT_ORDERS)
		rotate_vtbl[order](rx, ry, rz, matrix);
}

void matrix_rotate_axis(float x, float y, float z, float angle, vg_lite_matrix_t *matrix)
{
	float c = cos(angle);
	float s = sin(angle);

	matrix->m[0][0] = (1 - c) * x * x + c;
	matrix->m[0][1] = (1 - c) * x * y - s * z;
	matrix->m[0][2] = (1 - c) * x * z + s * y;
	matrix->m[1][0] = (1 - c) * x * y + s * z;
	matrix->m[1][1] = (1 - c) * y * y + c;
	matrix->m[1][2] = (1 - c) * y * z - s * x;
	matrix->m[2][0] = (1 - c) * x * z - s * y;
	matrix->m[2][1] = (1 - c) * y * z + s * x;
	matrix->m[2][2] = (1 - c) * z * z + c;
}

void matrix_multiply(const vg_lite_matrix_t *matrix_left, const vg_lite_matrix_t *matrix_right, vg_lite_matrix_t *result)
{
	int row, col;

	/* Process all rows. */
	for (row = 0; row < 3; row++) {
		/* Process all columns. */
		for (col = 0; col < 3; col++) {
			result->m[row][col] =  (matrix_left->m[row][0] * matrix_right->m[0][col])
				+ (matrix_left->m[row][1] * matrix_right->m[1][col])
				+ (matrix_left->m[row][2] * matrix_right->m[2][col]);
		}
	}
}

void matrix_translate_left(float tx, float ty,
		const vg_lite_matrix_t *matrix_right, vg_lite_matrix_t *matrix)
{
	matrix->m[0][0] = matrix_right->m[0][0] + tx * matrix_right->m[2][0];
	matrix->m[0][1] = matrix_right->m[0][1] + tx * matrix_right->m[2][1];
	matrix->m[0][2] = matrix_right->m[0][2] + tx * matrix_right->m[2][2];
	matrix->m[1][0] = matrix_right->m[1][0] + ty * matrix_right->m[2][0];
	matrix->m[1][1] = matrix_right->m[1][1] + ty * matrix_right->m[2][1];
	matrix->m[1][2] = matrix_right->m[1][2] + ty * matrix_right->m[2][2];
	matrix->m[2][0] = matrix_right->m[2][0];
	matrix->m[2][1] = matrix_right->m[2][1];
	matrix->m[2][2] = matrix_right->m[2][2];
}

void transfrom_rotate(vertex_t *result, vg_lite_matrix_t *rotate, const vertex_t *vertex, const vertex_t *scale, const vertex_t *translate)
{
	float x = rotate->m[0][0] * vertex->x + rotate->m[0][1] * vertex->y + rotate->m[0][2] * vertex->z;
	float y = rotate->m[1][0] * vertex->x + rotate->m[1][1] * vertex->y + rotate->m[1][2] * vertex->z;
	float z = rotate->m[2][0] * vertex->x + rotate->m[2][1] * vertex->y + rotate->m[2][2] * vertex->z;

	if (scale) {
		x *= scale->x;
		y *= scale->y;
		z *= scale->z;
	}

	if (translate) {
		x += translate->x;
		y += translate->y;
		z += translate->z;
	}

	result->x = x;
	result->y = y;
	result->z = z;
}

void transfrom_perspective(vertex_t *vertex, float camera_x, float camera_y, float camera_distance)
{
	vertex->x = camera_x + (vertex->x - camera_x) * camera_distance / (camera_distance + vertex->z);
	vertex->y = camera_y + (vertex->y - camera_y) * camera_distance / (camera_distance + vertex->z);
}

float transfrom_normal_z(vg_lite_matrix_t *rotate, const normal_t *normal)
{
	/* Compute the new normal Z coordinate transformed by the rotation matrix. */

	return rotate->m[2][0] * normal->x + rotate->m[2][1] * normal->y + rotate->m[2][2] * normal->z;
}

/*
   From 6.5.2 of OpenVG1.1 Spec: An affine transformation maps a point (x, y) into the point
   (x*sx + y*shx + tx, x*shy + y*sy + ty) using homogeneous matrix multiplication.
   To map a rectangle image (0,0),(w,0),(w,h),(0,h) to a parallelogram (x0,y0),(x1,y1),(x2,y2),(x3,y3).
   We get the following equations:
       x0 = tx;
       y0 = ty;
       x1 = w*sx + tx;
       y1 = w*shy + ty;
       x3 = h*shx + tx;
       y3 = h*sy + ty;

   So the homogeneous matrix sx, sy, shx, shy, tx, ty can be easily solved from above equations.
*/
void matrix_affine_blit(int w, int h, vertex_t *v0, vertex_t *v1, vertex_t *v2, vertex_t *v3, vg_lite_matrix_t *matrix)
{
	float sx, sy, shx, shy, tx, ty;

	/* Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise. */
	sx = (v1->x - v0->x) / w;
	sy = (v3->y - v0->y) / h;
	shx = (v3->x - v0->x) / h;
	shy = (v1->y - v0->y) / w;
	tx = v0->x;
	ty = v0->y;

	/* Set the Blit transformation matrix. */
	matrix->m[0][0] = sx;
	matrix->m[0][1] = shx;
	matrix->m[0][2] = tx;
	matrix->m[1][0] = shy;
	matrix->m[1][1] = sy;
	matrix->m[1][2] = ty;
	matrix->m[2][0] = 0.0;
	matrix->m[2][1] = 0.0;
	matrix->m[2][2] = 1.0;
}

void matrix_perspective_blit(int w, int h, vertex_t *v0, vertex_t *v1, vertex_t *v2, vertex_t *v3, vg_lite_matrix_t *matrix)
{
	vg_lite_float_point4_t src = { { 0, 0 }, { w, 0 }, { w, h ,}, { 0, h }, };
	vg_lite_float_point4_t dst = {
		{ v0->x, v0->y }, { v1->x, v1->y },
		{ v2->x, v2->y }, { v3->x, v3->y },
	};

	vg_lite_get_transform_matrix(src, dst, matrix);
}

void matrix_transform_rect(vg_lite_rectangle_t *rect,
		float pivots[3], float angles[3], float scale, float camera[3],
		vertex_t result[4], vg_lite_matrix_t *matrix)
{
	matrix_transform_rect2(rect, pivots, angles, ROT_ZYX, scale, camera, result, matrix);
}

void matrix_transform_rect2(vg_lite_rectangle_t *rect,
		float pivots[3], float angles[3], uint8_t rotate_order,
		float scale, float camera[3],
		vertex_t result[4], vg_lite_matrix_t *matrix)
{
	const float camera_dist = camera ? camera[2] : 0.0f;
	const float pivot_x = pivots ? pivots[0] : (rect->width / 2.0f);
	const float pivot_y = pivots ? pivots[1] : (rect->height / 2.0f);
	const float pivot_z = pivots ? pivots[2] : 0.0f;

	vertex_t verts[4] = {
		[0] = {              -pivot_x,               -pivot_y, -pivot_z },
		[1] = { rect->width - pivot_x,               -pivot_y, -pivot_z },
		[2] = { rect->width - pivot_x, rect->height - pivot_y, -pivot_z },
		[3] = {              -pivot_x, rect->height - pivot_y, -pivot_z },
	};
	vertex_t scale_xyz = { scale, scale, scale };
	vertex_t translate_zyz = { pivot_x, pivot_y, pivot_z };

	/* Perspective transformation requires absolute coordinates */
	if (camera_dist > 0.0f) {
		translate_zyz.x += rect->x;
		translate_zyz.y += rect->y;
	}

	vg_lite_matrix_t rotate_m;
	if (angles) {
		matrix_rotate(angles[0], angles[1], angles[2], rotate_order, &rotate_m);
	} else {
		vg_lite_identity(&rotate_m);
	}

	transfrom_rotate(&verts[0], &rotate_m, &verts[0], &scale_xyz, &translate_zyz);
	transfrom_rotate(&verts[1], &rotate_m, &verts[1], &scale_xyz, &translate_zyz);
	transfrom_rotate(&verts[2], &rotate_m, &verts[2], &scale_xyz, &translate_zyz);
	transfrom_rotate(&verts[3], &rotate_m, &verts[3], &scale_xyz, &translate_zyz);

	if (camera && camera_dist > 0.0f) {
		transfrom_perspective(&verts[0], camera[0], camera[1], camera_dist);
		transfrom_perspective(&verts[1], camera[0], camera[1], camera_dist);
		transfrom_perspective(&verts[2], camera[0], camera[1], camera_dist);
		transfrom_perspective(&verts[3], camera[0], camera[1], camera_dist);

		matrix_perspective_blit(rect->width, rect->height, &verts[0], &verts[1], &verts[2], &verts[3], matrix);
	} else {
		matrix_affine_blit(rect->width, rect->height, &verts[0], &verts[1], &verts[2], &verts[3], matrix);
	}

	if (result) {
		memcpy(result, verts, sizeof(verts));
	}
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static bool vglite_buf_format_is_opaque(vg_lite_buffer_format_t format)
{
	switch (format) {
	case VG_LITE_RGB565:
	case VG_LITE_BGR565:
	case VG_LITE_RGB888:
	case VG_LITE_BGR888:
		return true;
	default:
		return false;
	}
}

static void matrix_rotate_zyx(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in ZYX order.
	 *
	 * matrix = Rz * Ry * Rx
	 */

	matrix->m[0][0] = cos(rz) * cos(ry);
	matrix->m[0][1] = cos(rz) * sin(ry) * sin(rx) - sin(rz) * cos(rx);
	matrix->m[0][2] = cos(rz) * sin(ry) * cos(rx) + sin(rz) * sin(rx);
	matrix->m[1][0] = sin(rz) * cos(ry);
	matrix->m[1][1] = sin(rz) * sin(ry) * sin(rx) + cos(rz) * cos(rx);
	matrix->m[1][2] = sin(rz) * sin(ry) * cos(rx) - cos(rz) * sin(rx);
	matrix->m[2][0] = -sin(ry);
	matrix->m[2][1] = cos(ry) * sin(rx);
	matrix->m[2][2] = cos(ry) * cos(rx);
}

static void matrix_rotate_zxy(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in ZXY order.
	 *
	 * matrix = Rz * Rx * Ry
	 */

	matrix->m[0][0] = cos(rz) * cos(ry) - sin(rz) * sin(rx) * sin(ry);
	matrix->m[0][1] = -sin(rz) * cos(rx);
	matrix->m[0][2] = cos(rz) * sin(ry) + sin(rz) * sin(rx) * cos(ry);
	matrix->m[1][0] = sin(rz) * cos(ry) + cos(rz) * sin(rx) * sin(ry);
	matrix->m[1][1] = cos(rz) * cos(rx);
	matrix->m[1][2] = sin(rz) * sin(ry) - cos(rz) * sin(rx) * cos(ry);
	matrix->m[2][0] = -cos(rx) * sin(ry);
	matrix->m[2][1] = sin(rx);
	matrix->m[2][2] = cos(rx) * cos(ry);
}

static void matrix_rotate_yzx(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in YZX order.
	 *
	 * matrix = Ry * Rz * Rx
	 */

	matrix->m[0][0] = cos(ry) * cos(rz);
	matrix->m[0][1] = -cos(ry) * sin(rz) * cos(rx) + sin(ry) * sin(rx);
	matrix->m[0][2] = cos(ry) * sin(rz) * sin(rx) + sin(ry) * cos(rx);
	matrix->m[1][0] = sin(rz);
	matrix->m[1][1] = cos(rz) * cos(rx);
	matrix->m[1][2] = -cos(rz) * sin(rx);
	matrix->m[2][0] = -sin(ry) * cos(rz);
	matrix->m[2][1] = sin(ry) * sin(rz) * cos(rx) + cos(ry) * sin(rx);
	matrix->m[2][2] = -sin(ry) * sin(rz) * sin(rx) + cos(ry) * cos(rx);
}

static void matrix_rotate_yxz(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in YXZ order.
	 *
	 * matrix = Ry * Rx * Rz
	 */

	matrix->m[0][0] = cos(ry) * cos(rz) + sin(ry) * sin(rx) * sin(rz);
	matrix->m[0][1] = -cos(ry) * sin(rz) + sin(ry) * sin(rx) * cos(rz);
	matrix->m[0][2] = sin(ry) * cos(rx);
	matrix->m[1][0] = cos(rx) * sin(rz);
	matrix->m[1][1] = cos(rx) * cos(rz);
	matrix->m[1][2] = -sin(rx);
	matrix->m[2][0] = -sin(ry) * cos(rz) + cos(ry) * sin(rx) * sin(rz);
	matrix->m[2][1] = sin(ry) * sin(rz) + cos(ry) * sin(rx) * cos(rz);
	matrix->m[2][2] = cos(ry) * cos(rx);
}

static void matrix_rotate_xyz(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in XYZ order.
	 *
	 * matrix = Rx * Ry * Rz
	 */

	matrix->m[0][0] = cos(ry) * cos(rz);
	matrix->m[0][1] = -cos(ry) * sin(rz);
	matrix->m[0][2] = sin(ry);
	matrix->m[1][0] = cos(rx) * sin(rz) + sin(rx) * sin(ry) * cos(rz);
	matrix->m[1][1] = cos(rx) * cos(rz) - sin(rx) * sin(ry) * sin(rz);
	matrix->m[1][2] = -sin(rx) * cos(ry);
	matrix->m[2][0] = sin(rx) * sin(rz) - cos(rx) * sin(ry) * cos(rz);
	matrix->m[2][1] = sin(rx) * cos(rz) + cos(rx) * sin(ry) * sin(rz);
	matrix->m[2][2] = cos(rx) * cos(ry);
}

static void matrix_rotate_xzy(float rx, float ry, float rz, vg_lite_matrix_t *matrix)
{
	/**
	 * Euler angles rx, ry, rz in XZY order.
	 *
	 * matrix = Rx * Rz * Ry
	 */

	matrix->m[0][0] = cos(rz) * cos(ry);
	matrix->m[0][1] = -sin(rz);
	matrix->m[0][2] = cos(rz) * sin(ry);
	matrix->m[1][0] = cos(rx) * sin(rz) * cos(ry) + sin(rx) * sin(ry);
	matrix->m[1][1] = cos(rx) * cos(rz);
	matrix->m[1][2] = cos(rx) * sin(rz) * sin(ry) - sin(rx) * cos(ry);
	matrix->m[2][0] = sin(rx) * sin(rz) * cos(ry) - cos(rx) * sin(ry);
	matrix->m[2][1] = sin(rx) * cos(rz);
	matrix->m[2][2] = sin(rx) * sin(rz) * sin(ry) + cos(rx) * cos(ry);
}

void vector_normalize(float *x, float *y, float *z)
{
	float s2 = (*x) * (*x) + (*y) * (*y) + (*z) * (*z);
	if (s2 > 0.0f && s2 != 1.0f) {
		float s = sqrtf(s2);
		*x = (*x) / s;
		*y = (*y) / s;
		*z = (*z) / s;
	}
}

#endif /* CONFIG_VG_LITE*/
