/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_LAUNCHER_UI_H_
#define SRC_LAUNCHER_UI_H_

/*********************
 *      INCLUDES
 *********************/
#include <app_ui.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/

int main_scene_proc(void * scr, uint8_t msg_id, void * msg_data);
int svg_scene_proc(void * scr, uint8_t msg_id, void * msg_data);
int cube_scene_proc(void * scr, uint8_t msg_id, void * msg_data);
int font_scene_proc(void * scr, uint8_t msg_id, void * msg_data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* SRC_LAUNCHER_UI_H_ */
