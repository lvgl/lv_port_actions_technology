/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_UI_SERVICE

/*********************
 *      INCLUDES
 *********************/

#include <zephyr.h>
#include <drivers/input/input_dev.h>
#include <utils/acts_ringbuf.h>
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include <board.h>
#include "lvgl_inner.h"

#ifndef CONFIG_ACTS_RING_BUFFER
#  error "CONFIG_ACTS_RING_BUFFER is required"
#endif

/*********************
 *      DEFINES
 *********************/
#define CONFIG_LVGL_POINTER_COUNT 8

/**********************
 *      TYPEDEFS
 **********************/
typedef struct lvgl_pointer_user_data {
    const struct device *input_dev;    /* display device */
    struct k_work input_work;

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
    struct k_delayed_work boost_work;
    bool is_boosted;
    bool boost_requested;
#endif
} lvgl_pointer_user_data_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _pointer_input_work_handler(struct k_work *work);
static void _lvgl_pointer_read_cb(lv_indev_t * indev, lv_indev_data_t *data);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
static void _pointer_input_boost_work_handler(struct k_work *work);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_data_t s_pointer_scan_buf_mem[CONFIG_LVGL_POINTER_COUNT];
static ACTS_RINGBUF_DEFINE(s_pointer_scan_buf, s_pointer_scan_buf_mem, sizeof(s_pointer_scan_buf_mem));

static lvgl_pointer_user_data_t s_pointer_data;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_indev_pointer_scan(void)
{
    lvgl_pointer_user_data_t *pointer_data = &s_pointer_data;

    if (pointer_data->input_dev) {
        k_work_submit(&pointer_data->input_work);
    }
}

int lv_port_indev_pointer_init(void)
{
    lvgl_pointer_user_data_t *pointer_data = &s_pointer_data;
    pointer_data->input_dev = device_get_binding(CONFIG_TPKEY_DEV_NAME);
    if (pointer_data->input_dev == NULL) {
        LV_LOG_ERROR("Failed to get input device " CONFIG_TPKEY_DEV_NAME);
        return -ENODEV;
    }

    k_work_init(&pointer_data->input_work, _pointer_input_work_handler);
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
    k_delayed_work_init(&pointer_data->boost_work, _pointer_input_boost_work_handler);
#endif

    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL) {
        LV_LOG_ERROR("Failed to create input pointer.");
        return -ENOMEM;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _lvgl_pointer_read_cb);
    lv_indev_set_user_data(indev, pointer_data);

    input_dev_enable(pointer_data->input_dev);
    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
static void _pointer_input_boost_work_handler(struct k_work *work)
{
    lvgl_pointer_user_data_t *pointer_data = CONTAINER_OF(work, lvgl_pointer_user_data_t, boost_work);

    LV_LOG_INFO("input boost stop");

    pointer_data->is_boosted = false;
    dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "lv_input");
}

static void _pointer_input_boost(lvgl_pointer_user_data_t *pointer_data, bool boosted)
{
    if (boosted == pointer_data->boost_requested)
        return;

    pointer_data->boost_requested = boosted;

    if (boosted) {
        k_delayed_work_cancel(&pointer_data->boost_work);

        if (!pointer_data->is_boosted) {
            LV_LOG_INFO("input boost start");
            pointer_data->is_boosted = true;
            dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "lv_input");
        }
    }
    else if (pointer_data->is_boosted) {
        k_delayed_work_submit(&pointer_data->boost_work, K_MSEC(2000));
    }
}
#endif /* CONFIG_ACTS_DVFS_DYNAMIC_LEVEL */

static void _pointer_input_work_handler(struct k_work *work)
{
    lvgl_pointer_user_data_t *pointer_data =
            CONTAINER_OF(work, lvgl_pointer_user_data_t, input_work);

    static uint8_t put_cnt;
    static uint8_t drop_cnt;
    static lv_indev_state_t prev_state = LV_INDEV_STATE_RELEASED;

    struct input_value val;
    input_dev_inquiry(pointer_data->input_dev, &val);

    lv_indev_data_t indev_data;
    indev_data.point.x = val.point.loc_x;
    indev_data.point.y = val.point.loc_y;
    indev_data.state = (val.point.pessure_value == 1) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    /* drop redundant released point */
    if (indev_data.state == LV_INDEV_STATE_RELEASED && prev_state == LV_INDEV_STATE_RELEASED) {
        goto out_boost;
    }

    if (acts_ringbuf_put(&s_pointer_scan_buf, &indev_data, sizeof(indev_data)) != sizeof(indev_data)) {
        drop_cnt++;
    }
    else {
        prev_state = indev_data.state;
    }

    if (++put_cnt == 0) { /* about 4s for LCD refresh-rate 60 Hz */
        if (drop_cnt > 0) {
            LV_LOG_WARN("%u pointer dropped\n", drop_cnt);
            drop_cnt = 0;
        }
    }

out_boost:
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
    _pointer_input_boost(pointer_data, indev_data.state == LV_INDEV_STATE_PRESSED);
#endif

    return;
}

static void _lvgl_pointer_read_cb(lv_indev_t * indev, lv_indev_data_t *data)
{
    static lv_indev_data_t prev = {
        .point.x = 0,
        .point.y = 0,
        .state = LV_INDEV_STATE_RELEASED,
    };

    lv_indev_data_t curr;

    if (acts_ringbuf_get(&s_pointer_scan_buf, &curr, sizeof(curr)) == sizeof(curr)) {
        prev = curr;
    }

    *data = prev;
    data->continue_reading = (acts_ringbuf_is_empty(&s_pointer_scan_buf) == 0);
}

#endif /* CONFIG_UI_SERVICE */
