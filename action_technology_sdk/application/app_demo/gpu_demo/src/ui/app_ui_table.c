/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_ui_table.c
 */

/*********************
 *      INCLUDES
 *********************/
#include <os_common_api.h>
#include <app_ui.h>
#include "../launcher/launcher_ui.h"

/*********************
 *      DEFINES
 *********************/
#define SCENE_DEFINE(name, scene_id, scene_proc) \
	const app_ui_scene_info_t name = { \
		.id = scene_id, \
		.proc = scene_proc, \
	}

#define UI_ENTRY_DEFINE(ui_id, scene_ptr) \
	[ui_id] = { \
		.id = ui_id, \
		.scenes = scene_ptr, \
	}

/**********************
 *   GLOBAL PROTOTYPES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static SCENE_DEFINE(s_main_scene, MAIN_SCENE, main_scene_proc);
static SCENE_DEFINE(s_svg_scene, SVG_SCENE, svg_scene_proc);
static SCENE_DEFINE(s_cube_scene, CUBE_SCENE, cube_scene_proc);
static SCENE_DEFINE(s_font_scene, FONT_SCENE, font_scene_proc);

static const app_ui_info_t s_ui_table[] = {
	UI_ENTRY_DEFINE(MAIN_UI, &s_main_scene),
	UI_ENTRY_DEFINE(SVG_UI, &s_svg_scene),
	UI_ENTRY_DEFINE(CUBE_UI, &s_cube_scene),
	UI_ENTRY_DEFINE(FONT_UI, &s_font_scene),
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

const app_ui_info_t * app_ui_get_info(uint16_t id)
{
	if (id >= ARRAY_SIZE(s_ui_table))
		return NULL;

	if (s_ui_table[id].scenes == NULL)
		return NULL;

	return &s_ui_table[id];
}

const app_ui_scene_info_t * app_ui_get_scene_info(const app_ui_info_t * info, uint16_t scene_id)
{
	if (info) {
		int idx = app_ui_get_scene_count(info) - 1;

		for (; idx >= 0; idx--) {
			if (info->scenes[idx].id == scene_id)
				return &info->scenes[idx];
		}
	}

	return NULL;
}

uint16_t app_ui_get_scene_count(const app_ui_info_t *info)
{
	return info ? (info->n_scenes[0] + info->n_scenes[1] +
	               info->n_scenes[2] + info->n_scenes[3] + 1) : 0;
}

uint8_t app_ui_get_scroll_fps(const app_ui_info_t *info, bool vertical)
{
	if (info) {
		uint8_t fps = UINT8_MAX;
		int idx = app_ui_get_scene_count(info) - 1;

		for (; idx >= 0; idx--) {
			uint8_t scene_fps = app_ui_get_scene_fps(&info->scenes[idx]);
			if (fps > scene_fps)
				fps = scene_fps;
		}

		return fps;
	}

	return DEF_UI_FPS;
}

uint8_t app_ui_get_transition_fps(const app_ui_info_t * to, const app_ui_info_t * from, uint8_t anim_type)
{
	uint8_t to_fps = app_ui_get_scroll_fps(to, false);
	uint8_t from_fps = app_ui_get_scroll_fps(from, false);

	return LV_MIN(to_fps, from_fps);
}
