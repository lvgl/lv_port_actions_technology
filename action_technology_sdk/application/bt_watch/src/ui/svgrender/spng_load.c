/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vg_lite.h>
#include <fs/fs.h>
#include <memory/mem_cache.h>
#include <zephyr.h>

#include "spng.h"

#ifndef ROUND_UP
#  define ROUND_UP(x, align) \
		(((x) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef ROUND_DOWN
#  define ROUND_DOWN(x, align) \
		((x) & ~((align) - 1))
#endif

#ifdef CONFIG_UI_MANAGER
#  include <ui_mem.h>
#  define SPNG_MALLOC(size)                   ui_mem_alloc(MEM_RES, size, __func__)
#  define SPNG_REALLOC(ptr, size)             ui_mem_realloc(MEM_RES, ptr, size, __func__)
#  define SPNG_ALIGNED_ALLOC(alignment, size) ui_mem_aligned_alloc(MEM_RES, alignment, ROUND_UP(size, alignment), __func__)
#  define SPNG_FREE(ptr)                      ui_mem_free(MEM_RES, ptr)

#else
#  define SPNG_MALLOC(size)                   malloc(size)
#  define SPNG_REALLOC(ptr, size)             realloc(ptr, size)
#  define SPNG_ALIGNED_ALLOC(alignment, size) aligned_alloc(alignment, ROUND_UP(size, alignment))
#  define SPNG_FREE(ptr)                      free(ptr)
#endif

static void * _spng_malloc(size_t size);
static void * _spng_realloc(void * ptr, size_t size);
static void * _spng_calloc(size_t count, size_t size);
static void _spng_free(void * ptr);

static void * _spng_open_file(const char * path, bool read_only);
static void _spng_close_file(void * fp);
static int _spng_read_file(spng_ctx * ctx, void * user, void * dst_src, size_t length);
static int _spng_write_file(spng_ctx * ctx, void * user, void * dst_src, size_t length);

static struct spng_alloc g_spng_alloc = {
	.malloc_fn = _spng_malloc,
	.realloc_fn = _spng_realloc,
	.calloc_fn = _spng_calloc,
	.free_fn = _spng_free,
};

int spng_load_file(vg_lite_buffer_t * buffer, const char * path)
{
	int rc = 0;

	memset(buffer, 0, sizeof(vg_lite_buffer_t));

	spng_ctx *ctx = spng_ctx_new2(&g_spng_alloc, 0);
	if (ctx == NULL)
		return -ENOMEM;

	void * fp = _spng_open_file(path, true);
	if (fp == NULL) {
		rc = -ENOENT;
		goto out_free_ctx;
	}

	rc = spng_set_png_stream(ctx, _spng_read_file, fp);
	if (rc)
		goto out_close_file;

	struct spng_ihdr ihdr = { 0 };
	int fmt = SPNG_FMT_RGBA8;

	spng_get_ihdr(ctx, &ihdr);
	if (ihdr.width == 0 || ihdr.height == 0) {
		rc = -EINVAL;
		goto out_free_ctx;
	}

	if (ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth <= 8) {
		buffer->format = VG_LITE_L8;
		buffer->stride = ihdr.width;
		fmt = SPNG_FMT_G8;
	} else {
		buffer->format = VG_LITE_RGBA8888;
		buffer->stride = ihdr.width * 4;
	}

	buffer->width  = ihdr.width;
	buffer->height = ihdr.height;
	buffer->memory = SPNG_ALIGNED_ALLOC(64, buffer->stride * buffer->height);
	if (buffer->memory == NULL) {
		rc = -ENOMEM;
		goto out_close_file;
	}

	rc = spng_decode_image(ctx, buffer->memory, buffer->stride * buffer->height, fmt, 0);
	if (rc) {
		SPNG_FREE(buffer->memory);
		buffer->memory = NULL;
		goto out_close_file;
	}

	mem_dcache_clean(buffer->memory, buffer->stride * buffer->height);
	vg_lite_map(buffer, VG_LITE_MAP_USER_MEMORY, -1);

out_close_file:
	_spng_close_file(fp);
out_free_ctx:
	spng_ctx_free(ctx);
	return rc;
}

int spng_load_memory(vg_lite_buffer_t * buffer, const void * png_bytes, size_t png_len)
{
	int rc = 0;

	memset(buffer, 0, sizeof(vg_lite_buffer_t));

	spng_ctx *ctx = spng_ctx_new2(&g_spng_alloc, 0);
	if (ctx == NULL)
		return -ENOMEM;

	rc = spng_set_png_buffer(ctx, png_bytes, png_len);
	if (rc)
		goto out_free_ctx;

	struct spng_ihdr ihdr = { 0 };
	int fmt = SPNG_FMT_RGBA8;

	spng_get_ihdr(ctx, &ihdr);
	if (ihdr.width == 0 || ihdr.height == 0) {
		rc = -EINVAL;
		goto out_free_ctx;
	}

	if (ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE && ihdr.bit_depth <= 8) {
		buffer->format = VG_LITE_L8;
		buffer->stride = ihdr.width;
		fmt = SPNG_FMT_G8;
	} else {
		buffer->format = VG_LITE_RGBA8888;
		buffer->stride = ihdr.width * 4;
	}

	buffer->width  = ihdr.width;
	buffer->height = ihdr.height;
	buffer->memory = SPNG_ALIGNED_ALLOC(64, buffer->stride * buffer->height);
	if (buffer->memory == NULL) {
		rc = -ENOMEM;
		goto out_free_ctx;
	}

	rc = spng_decode_image(ctx, buffer->memory, buffer->stride * buffer->height, fmt, 0);
	if (rc) {
		SPNG_FREE(buffer->memory);
		buffer->memory = NULL;
		goto out_free_ctx;
	}

	mem_dcache_clean(buffer->memory, buffer->stride * buffer->height);
	vg_lite_map(buffer, VG_LITE_MAP_USER_MEMORY, -1);

out_free_ctx:
	spng_ctx_free(ctx);
	return rc;
}

int spng_save_file(vg_lite_buffer_t * buffer, const char * path)
{
	int rc = 0;

	struct spng_ihdr ihdr = {
		.width = buffer->width,
		.height = buffer->height,
		.bit_depth = 8,
	};

	switch (buffer->format) {
	case VG_LITE_RGBA8888:
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
		break;
	case VG_LITE_RGB888:
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
		break;
	case VG_LITE_L8:
		ihdr.color_type = SPNG_COLOR_TYPE_GRAYSCALE;
		break;
	default:
		return -EINVAL;
	}

	void * fp = _spng_open_file(path, false);
	if (fp == NULL)
		return -ENOENT;

	spng_ctx *ctx = spng_ctx_new2(&g_spng_alloc, SPNG_CTX_ENCODER);
	if (ctx == NULL) {
		rc = -ENOMEM;
		goto out_close_file;
	}

	rc = spng_set_ihdr(ctx, &ihdr);
	if (rc)
		goto out_free_ctx;

	rc = spng_set_png_stream(ctx, _spng_write_file, fp);
	if (rc)
		goto out_free_ctx;

	rc = spng_encode_image(ctx, buffer->memory, buffer->stride * buffer->height, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

out_free_ctx:
	spng_ctx_free(ctx);
out_close_file:
	_spng_close_file(fp);
	return rc;
}

int spng_free_buffer(vg_lite_buffer_t * buffer)
{
	if (buffer && buffer->memory) {
		vg_lite_unmap(buffer);
		SPNG_FREE(buffer->memory);
		buffer->memory = NULL;
	}

	return 0;
}

static void * _spng_malloc(size_t size)
{
	return SPNG_MALLOC(size);
}

static void * _spng_realloc(void * ptr, size_t size)
{
	return SPNG_REALLOC(ptr, size);
}

static void * _spng_calloc(size_t count, size_t size)
{
	void *ptr = _spng_malloc(count * size);
	if (ptr) {
		memset(ptr, 0, count * size);
	}

	return ptr;
}

static void _spng_free(void * ptr)
{
	return SPNG_FREE(ptr);
}

static void * _spng_open_file(const char * path, bool read_only)
{
	struct fs_file_t *fp = _spng_malloc(sizeof(*fp));
	if (fp == NULL)
		return NULL;

	fs_file_t_init(fp);

	if (fs_open(fp, path, read_only ? FS_O_READ : (FS_O_WRITE | FS_O_CREATE))) {
		_spng_free(fp);
		return NULL;
	}

	if (read_only == false)
		fs_truncate(fp, 0);

	return fp;
}

static void _spng_close_file(void * fp)
{
	if (fp)
		_spng_free(fp);
}

static int _spng_read_file(spng_ctx * ctx, void * user, void * dst_src, size_t length)
{
	return fs_read(user, dst_src, length);
}

static int _spng_write_file(spng_ctx * ctx, void * user, void * dst_src, size_t length)
{
	return fs_write(user, dst_src, length);
}
