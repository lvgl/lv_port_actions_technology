/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <lvgl/lvgl_img_loader.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int picarray_load(lvgl_img_loader_t * loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset);
static uint16_t picarray_get_count(lvgl_img_loader_t *loader);

static int picregion_load(lvgl_img_loader_t * loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset);
static void picregion_unload(lvgl_img_loader_t * loader, lv_image_dsc_t * dsc);
static void picregion_preload(lvgl_img_loader_t * loader, uint16_t index);
static uint16_t picregion_get_count(lvgl_img_loader_t *loader);

static int picgroup_load(lvgl_img_loader_t * loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset);
static void picgroup_unload(lvgl_img_loader_t * loader, lv_image_dsc_t * dsc);
static void picgroup_preload(lvgl_img_loader_t * loader, uint16_t index);
static uint16_t picgroup_get_count(lvgl_img_loader_t * loader);

/**********************
 *  STATIC VARIABLES
 **********************/

static const lvgl_img_loader_fn_t picarray_loader_fn = {
	.load = picarray_load,
	.get_count = picarray_get_count,
};

static const lvgl_img_loader_fn_t picregion_loader_fn = {
	.load = picregion_load,
	.unload = picregion_unload,
	.preload = picregion_preload,
	.get_count = picregion_get_count,
};

static const lvgl_img_loader_fn_t picgroup_loader_fn = {
	.load = picgroup_load,
	.unload = picgroup_unload,
	.preload = picgroup_preload,
	.get_count = picgroup_get_count,
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int lvgl_img_loader_set_picarray(lvgl_img_loader_t * loader,
		const lv_image_dsc_t * dsc, const lv_point_t * offsets, uint16_t num)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picarray_loader_fn;
	loader->picarray.dsc = dsc;
	loader->picarray.offsets = offsets;
	loader->picarray.count = num;
	loader->last_preload_idx = -1;
	return 0;
}

int lvgl_img_loader_set_picregion(lvgl_img_loader_t * loader, uint32_t scene_id,
		lvgl_res_picregion_t * picregion)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picregion_loader_fn;
	loader->picregion.scene_id = scene_id;
	loader->picregion.node = picregion;
	loader->last_preload_idx = -1;
	return 0;
}

int lvgl_img_loader_set_picgroup(lvgl_img_loader_t * loader, uint32_t scene_id,
		lvgl_res_group_t * picgroup, const uint32_t * ids, uint16_t num)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = &picgroup_loader_fn;
	loader->picgroup.scene_id = scene_id;
	loader->picgroup.node = picgroup;
	loader->picgroup.ids = ids;
	loader->picgroup.count = num;
	loader->last_preload_idx = -1;
	return 0;
}

int lvgl_img_loader_set_custom(lvgl_img_loader_t *loader,
		const lvgl_img_loader_fn_t * fn, void * user_data)
{
	memset(loader, 0, sizeof(*loader));
	loader->fn = fn;
	loader->user_data = user_data;
	loader->last_preload_idx = -1;
	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int picarray_load(lvgl_img_loader_t * loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset)
{
	if (offset) {
		if (loader->picarray.offsets) {
			offset->x = loader->picarray.offsets[index].x;
			offset->y = loader->picarray.offsets[index].y;
		} else {
			offset->x = 0;
			offset->y = 0;
		}
	}

	memcpy(dsc, &loader->picarray.dsc[index], sizeof(*dsc));
	return 0;
}

static uint16_t picarray_get_count(lvgl_img_loader_t * loader)
{
	return loader->picarray.count;
}

static int picregion_load(lvgl_img_loader_t * loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset)
{
	int res = lvgl_res_load_pictures_from_picregion(loader->picregion.node, index, index, dsc);
	if (res == 0 && offset != NULL) {
		offset->x = 0;
		offset->y = 0;
	}

	return res;
}

static void picregion_unload(lvgl_img_loader_t *loader, lv_image_dsc_t * dsc)
{
	if (dsc->data) {
		lvgl_res_unload_pictures(dsc, 1);
		dsc->data = NULL;
	}
}

static void picregion_preload(lvgl_img_loader_t *loader, uint16_t index)
{
	lvgl_res_preload_pictures_from_picregion(loader->picregion.scene_id,
			loader->picregion.node, index, index);
}

static uint16_t picregion_get_count(lvgl_img_loader_t *loader)
{
	return loader->picregion.node->frames;
}

static int picgroup_load(lvgl_img_loader_t *loader, uint16_t index, lv_image_dsc_t * dsc, lv_point_t * offset)
{
	return lvgl_res_load_pictures_from_group(loader->picgroup.node,
					loader->picgroup.ids + index, dsc, offset, 1);
}

static void picgroup_unload(lvgl_img_loader_t *loader, lv_image_dsc_t * dsc)
{
	if (dsc->data) {
		lvgl_res_unload_pictures(dsc, 1);
		dsc->data = NULL;
	}
}

static void picgroup_preload(lvgl_img_loader_t *loader, uint16_t index)
{
	lvgl_res_preload_pictures_from_group(loader->picgroup.scene_id,
			loader->picgroup.node, loader->picgroup.ids + index, 1);
}

static uint16_t picgroup_get_count(lvgl_img_loader_t *loader)
{
	return loader->picgroup.count;
}
