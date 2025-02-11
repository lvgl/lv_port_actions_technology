/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl_port.h"
#include "../src/lvgl_private.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void _set_display_ready(void);
static void _pause_display(void);
static void _resume_display(void);
static void _update_display_period(uint32_t period);

static void _set_indev_ready(void);
static void _pause_indev(void);
static void _resume_indev(void);
static void _update_indev_period(uint32_t period);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lvx_refr_set_manual(lv_display_t * disp)
{
    if(disp) {
        lv_display_delete_refr_timer(disp);
    }
    else {
        disp = lv_display_get_next(NULL);
        while(disp) {
            lv_display_delete_refr_timer(disp);
            disp = lv_display_get_next(disp);
        }

        /* Make anim timer pause */
        lv_timer_pause(lv_anim_get_timer());

        /* Make indev timer pause */
        _pause_indev();
    }
}

void lvx_refr_all(void)
{
    lvx_refr_indev(NULL);
    lvx_refr_display(NULL);
}

void lvx_refr_display(lv_display_t * disp)
{
    lv_anim_refr_now();

    if(disp) {
        lv_display_t *disp_def = lv_display_get_default();
        lv_timer_t *refr_timer = lv_display_get_refr_timer(disp);
        if(refr_timer) {
            lv_display_refr_timer(refr_timer);
        }
        else {
            lv_display_set_default(disp);
            lv_display_refr_timer(NULL);
            lv_display_set_default(disp_def);
        }
    }
    else {
        lv_display_t *disp_def = lv_display_get_default();
        disp = lv_display_get_next(NULL);
        while(disp) {
            lv_timer_t *refr_timer = lv_display_get_refr_timer(disp);
            if(refr_timer) {
                lv_display_refr_timer(refr_timer);
            }
            else {
                lv_display_set_default(disp);
                lv_display_refr_timer(NULL);
                lv_display_set_default(disp_def);
            }

            disp = lv_display_get_next(disp);
        }
    }
}

void lvx_refr_indev(lv_indev_t * indev)
{
    if(indev) {
        lv_indev_read_timer_cb(lv_indev_get_read_timer(indev));
    }
    else {
        indev = lv_indev_get_next(NULL);
        while(indev) {
            lv_indev_read_timer_cb(lv_indev_get_read_timer(indev));
            indev = lv_indev_get_next(indev);
        }
    }
}

void lvx_refr_finish_layer(lv_display_t * disp, lv_layer_t * layer)
{
    if(disp == NULL)
        disp = lv_refr_get_disp_refreshing();

    while(layer->draw_task_head) {
        lv_draw_dispatch_wait_for_request();

        bool task_dispatched = lv_draw_dispatch_layer(disp, layer);
        if(!task_dispatched) {
            lv_draw_wait_for_finish();
            lv_draw_dispatch_request();
        }
    }
}

void lvx_refr_ready(void)
{
    _set_display_ready();
    _set_indev_ready();
}

void lvx_refr_pause(void)
{
    _pause_display();
    _pause_indev();
}

void lvx_refr_resume(void)
{
    _resume_display();
    _resume_indev();
}

void lvx_refr_set_period(uint32_t period)
{
    _update_display_period(period);
    _update_indev_period(period);
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void _set_display_ready(void)
{
    lv_display_t *disp = lv_display_get_next(NULL);
    while(disp) {
        lv_timer_ready(lv_display_get_refr_timer(disp));
        disp = lv_display_get_next(disp);
    }

    /* Make anim timer ready */
    lv_timer_ready(lv_anim_get_timer());
}

static void _pause_display(void)
{
    lv_display_t *disp = lv_display_get_next(NULL);
    while(disp) {
        lv_timer_t *timer = lv_display_get_refr_timer(disp);
        if(timer) lv_timer_pause(timer);

        disp = lv_display_get_next(disp);
    }

    /* Make anim timer pause */
    lv_timer_pause(lv_anim_get_timer());
}

static void _resume_display(void)
{
    lv_display_t *disp = lv_display_get_next(NULL);
    while(disp) {
        lv_timer_t *timer = lv_display_get_refr_timer(disp);
        if(timer) lv_timer_resume(timer);

        disp = lv_display_get_next(disp);
    }

    /* Make anim timer resume */
    lv_timer_resume(lv_anim_get_timer());
}

static void _update_display_period(uint32_t period)
{
    lv_display_t *disp = lv_display_get_next(NULL);
    while(disp) {
        lv_timer_t *timer = lv_display_get_refr_timer(disp);
        if(timer) lv_timer_set_period(timer, period);

        disp = lv_display_get_next(disp);
    }

    /* Update anim timer period */
    lv_timer_set_period(lv_anim_get_timer(), period);
}

static void _set_indev_ready(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while(indev) {
        lv_timer_ready(lv_indev_get_read_timer(indev));
        indev = lv_indev_get_next(indev);
    }
}

static void _pause_indev(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while(indev) {
        lv_timer_pause(lv_indev_get_read_timer(indev));
        indev = lv_indev_get_next(indev);
    }
}

static void _resume_indev(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while(indev) {
        lv_timer_resume(lv_indev_get_read_timer(indev));
        indev = lv_indev_get_next(indev);
    }
}

static void _update_indev_period(uint32_t period)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while(indev) {
        lv_timer_set_period(lv_indev_get_read_timer(indev), period);
        indev = lv_indev_get_next(indev);
    }
}
