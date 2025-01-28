/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LAUNCHER_MEFFECT_EFFECT_WHEEL_EVENT_H_
#define LAUNCHER_MEFFECT_EFFECT_WHEEL_EVENT_H_

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl/lvgl.h>

/**********************
 *      TYPEDEFS
 **********************/
enum {
	NULL_SLIDE = 0,
	LEFT_SLIDE,
	RIGHT_SLIDE
};

#define ICON_NUM 6		//icon num
#define SURROUND_RADIUS 200		//radius(z)
#define D_ICON_SLANT_MIN 0		//min slant
#define D_ICON_SLANT_MAX (-SURROUND_RADIUS*5)	//max slant
#define MOVE_COEFFICIENT 110	//move coord to angle coefficient 255 = 1
#define D_ICON_ACCELERATED_SPEED 40 //auto roll rate of decay angle
#define D_ICON_TIMER_SPEED	4	//auto roll time extend rate
#define D_ICON_CALIBRATION_TIMER 100	//homing time
#define D_ICON_ZOOM_MIN 520		//min icon zoom
#define D_ICON_ZOOM_MAX LV_SCALE_NONE	//max icon zoom
#define D_ICON_ZOOM_TIME 300	//min zoom to max zoom time
typedef struct {
	lv_obj_t *cont;
	lv_obj_t *effect_wheel[ICON_NUM];
	lv_image_dsc_t effect_wheel_imgs[ICON_NUM];

	lv_coord_t angle_y;
	uint32_t move_time;
	lv_coord_t move_angle;
	lv_coord_t move_x;
	lv_coord_t time_angle;
	uint8_t direction;

	lv_coord_t anim_observe_y;
	lv_coord_t anim_zoom;
	uint32_t effect_wheel_id;

	uint32_t clock_zoom;

	void (*anim_observe_y_cb)(void *, int32_t);
	void (*anim_scroll_cb)(void *, int32_t);
	void (*anim_zoom_cb)(void *, int32_t);
	void (*anim_out_cb)(struct _lv_anim_t *);
} effect_wheel_wheel_scene_data_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief effect wheel exit anim create
 */
void _effect_wheel_out_anim_create(effect_wheel_wheel_scene_data_t *data);


/**
 * @brief effect wheel in anim create
 */
void _effect_wheel_in_anim_create(effect_wheel_wheel_scene_data_t *data);

/**
 * @brief effect wheel permutation
 * @return lately id
 */
uint32_t set_effect_wheel_permutation(lv_obj_t **obj,uint32_t num);

/**
 * @brief effect wheel icon rotation 
 * @param angle y rotation
 */
void _effect_wheel_wheel_angle(effect_wheel_wheel_scene_data_t * data,lv_coord_t angle);

/**
 * @brief effect wheel event manage
 */
void _effect_wheel_touch_event_cb(lv_event_t * e);

static inline int32_t effect_wheel_get_num(void)
{
    return ICON_NUM;
}
static inline int32_t effect_wheel_get_slant_min(void)
{
    return D_ICON_SLANT_MIN;
}
static inline int32_t effect_wheel_get_slant_max(void)
{
    return D_ICON_SLANT_MAX;
}
static inline int32_t effect_wheel_get_move_coefficient(void)
{
    return MOVE_COEFFICIENT;
}
static inline int32_t effect_wheel_get_accelerated_speed(void)
{
    return D_ICON_ACCELERATED_SPEED;
}
static inline int32_t effect_wheel_get_timer_speed(void)
{
    return D_ICON_TIMER_SPEED;
}
static inline int32_t effect_wheel_get_calibration_timer(void)
{
    return D_ICON_CALIBRATION_TIMER;
}
static inline int32_t effect_wheel_get_zoom_min(void)
{
    return D_ICON_ZOOM_MIN;
}
static inline int32_t effect_wheel_get_zoom_max(void)
{
    return D_ICON_ZOOM_MAX;
}
static inline int32_t effect_wheel_get_zoom_time(void)
{
    return D_ICON_ZOOM_TIME;
}

#ifdef __cplusplus
}
#endif
#endif /*LAUNCHER_MEFFECT_EFFECT_WHEEL_EVENT_H_*/
