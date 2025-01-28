/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file anim_img.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <assert.h>
#include <sys/util.h>
#include "anim_img.h"
#include <lvgl/src/lvgl_private.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &anim_img_class

/**********************
 *      TYPEDEFS
 **********************/
/** Data of anim image */
typedef struct {
	lv_image_t img;
	lv_anim_t anim;

	uint8_t started : 1;
	uint8_t preload_en : 1;

	int16_t start_idx; /* anim start src index */
	int16_t src_idx;   /* src index */
	int16_t src_count; /* src count */
	lv_image_dsc_t src_dsc;

	/* resource loader */
	lvgl_img_loader_t loader;

	anim_img_ready_cb_t ready_cb;
} anim_img_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void anim_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void anim_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static void anim_img_exec_cb(void *obj, int32_t value);

static void prepare_anim(lv_obj_t *obj);
static bool load_image(lv_obj_t *obj, uint16_t index);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t anim_img_class = {
	.constructor_cb = anim_img_constructor,
	.destructor_cb = anim_img_destructor,
	.instance_size = sizeof(anim_img_t),
	.base_class = &lv_image_class,
	.name = "anim-img",
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * anim_img_create(lv_obj_t * parent)
{
	LV_LOG_INFO("begin");
	lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
	lv_obj_class_init_obj(obj);
	return obj;
}

void anim_img_clean(lv_obj_t * obj)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	/* stop current animation */
	anim_img_stop(obj);

	/* unload resource */
	if (animimg->src_dsc.data != NULL) {
		lv_image_cache_drop(&animimg->src_dsc);
		lvgl_img_loader_unload(&animimg->loader, &animimg->src_dsc);
		animimg->src_dsc.data = NULL;
	}
}

/*=====================
 * Setter functions
 *====================*/

void anim_img_set_src_picarray(lv_obj_t *obj, const lv_image_dsc_t * src, const lv_point_t * offset, uint16_t cnt)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	/* cleanup loader */
	anim_img_clean(obj);
	/* initialize loader */
	lvgl_img_loader_set_picarray(&animimg->loader, src, offset, cnt);
	/* prepare the animation */
	prepare_anim(obj);
}

void anim_img_set_src_picregion(lv_obj_t *obj, uint32_t scene_id, lvgl_res_picregion_t *picregion)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	/* cleanup loader */
	anim_img_clean(obj);
	/* initialize loader */
	lvgl_img_loader_set_picregion(&animimg->loader, scene_id, picregion);
	/* prepare the animation */
	prepare_anim(obj);
}

void anim_img_set_src_picgroup(lv_obj_t *obj, uint32_t scene_id,
		lvgl_res_group_t *picgroup, const uint32_t *ids, uint16_t cnt)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	/* cleanup loader */
	anim_img_clean(obj);
	/* initialize loader */
	lvgl_img_loader_set_picgroup(&animimg->loader, scene_id, picgroup, ids, cnt);
	/* prepare the animation */
	prepare_anim(obj);
}

void anim_img_set_duration(lv_obj_t *obj, uint32_t delay, uint32_t duration)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	lv_anim_set_delay(&animimg->anim, delay);
	lv_anim_set_duration(&animimg->anim, duration);
}

void anim_img_set_repeat(lv_obj_t *obj, uint32_t delay, uint32_t count)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	lv_anim_set_repeat_delay(&animimg->anim, delay);
	lv_anim_set_repeat_count(&animimg->anim, count);
}

void anim_img_set_preload(lv_obj_t *obj, bool en)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	animimg->preload_en = en;
}

void anim_img_set_ready_cb(lv_obj_t *obj, anim_img_ready_cb_t ready_cb)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	animimg->ready_cb = ready_cb;
}

/*=====================
 * Getter functions
 *====================*/

uint16_t anim_img_get_src_count(lv_obj_t *obj)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	return animimg->src_count;
}

bool anim_img_get_running(lv_obj_t *obj)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	return animimg->started;
}

/*=====================
 * Other functions
 *====================*/

void anim_img_play_static(lv_obj_t *obj, uint16_t idx)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	if (animimg->started || !lvgl_img_loader_is_inited(&animimg->loader)) {
		return;
	}

	idx = LV_MIN(idx, animimg->src_count - 1);
	load_image(obj, idx);
}

void anim_img_start(lv_obj_t *obj, bool continued)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	if (animimg->started || !lvgl_img_loader_is_inited(&animimg->loader)) {
		return;
	}

	if (continued && animimg->src_idx > 0) {
		animimg->start_idx = LV_MIN(animimg->src_idx, animimg->src_count - 1);
	} else {
		animimg->start_idx = 0;
	}

	animimg->started = 1;
	lv_anim_set_values(&animimg->anim, animimg->start_idx,
			animimg->start_idx + animimg->src_count - 1);
	lv_anim_start(&animimg->anim);
}

void anim_img_stop(lv_obj_t *obj)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	if (animimg->started) {
		animimg->started = 0;
		lv_anim_delete(obj, anim_img_exec_cb);
	}
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void anim_img_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	LV_UNUSED(class_p);
	LV_TRACE_OBJ_CREATE("begin");

	anim_img_t *animimg = (anim_img_t *)obj;

	animimg->src_idx = -1;
	animimg->preload_en = 1;
	lv_anim_init(&animimg->anim);
	lv_anim_set_early_apply(&animimg->anim, true);
	lv_anim_set_var(&animimg->anim, obj);
	lv_anim_set_exec_cb(&animimg->anim, anim_img_exec_cb);
	lv_anim_set_repeat_count(&animimg->anim, LV_ANIM_REPEAT_INFINITE);

	lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);

	LV_TRACE_OBJ_CREATE("finished");
}

static void anim_img_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
	anim_img_clean(obj);
}

static void prepare_anim(lv_obj_t *obj)
{
	anim_img_t *animimg = (anim_img_t *)obj;

	if (animimg->preload_en) {
		lvgl_img_loader_preload(&animimg->loader, 0);
	}

	animimg->src_idx = -1;
	animimg->src_count = lvgl_img_loader_get_count(&animimg->loader);
}

static bool load_image(lv_obj_t *obj, uint16_t index)
{
	anim_img_t *animimg = (anim_img_t *)obj;
	lv_point_t offset = { 0, 0 };
	lv_image_dsc_t next_src;

	if (animimg->src_idx == index && animimg->src_dsc.data != NULL)
		return true;

	if (lvgl_img_loader_load(&animimg->loader, index, &next_src, &offset) == 0) {
		if (animimg->src_dsc.data != NULL)
			lvgl_img_loader_unload(&animimg->loader, &animimg->src_dsc);

		animimg->src_idx = index;
		lv_memcpy(&animimg->src_dsc, &next_src, sizeof(next_src));

		lv_image_cache_drop(&animimg->src_dsc);
		lv_image_set_src(obj, &animimg->src_dsc);

		lv_obj_set_style_translate_x(obj, offset.x, LV_PART_MAIN);
		lv_obj_set_style_translate_y(obj, offset.y, LV_PART_MAIN);
		return true;
	}

	return false;
}

static void anim_img_exec_cb(void *var, int32_t value)
{
	lv_obj_t *obj = var;
	anim_img_t *animimg = (anim_img_t *)obj;

	/* value range can be exceed src_count - 1 */
	if (value >= animimg->src_count) {
		value -= animimg->src_count;
	}

	/* load current image */
	load_image(obj, value);

	/* preload next image */
	if (animimg->preload_en) {
		uint16_t preload_idx = value + 1;

		if (preload_idx >= animimg->src_count)
			preload_idx = 0;

		lvgl_img_loader_preload(&animimg->loader, preload_idx);
	}

	if (animimg->ready_cb && value == animimg->start_idx + animimg->src_count - 1) {
		animimg->ready_cb(obj);
	}
}
