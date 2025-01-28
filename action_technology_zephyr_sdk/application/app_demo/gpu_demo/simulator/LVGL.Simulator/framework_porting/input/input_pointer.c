/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <string.h>
#include <native_window.h>

#define POINTER_GESTURE_MIN_VEC  3
#define POINTER_GESTURE_LIMIT    50

#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : (-(x)))
#endif

extern int input_driver_register(input_drv_t* input_drv);

static void pointer_gesture_detect(input_dev_data_t* data)
{
    static point_t gesture_sum;
    static input_dev_data_t prev = {
        .point.x = 0,
        .point.y = 0,
        .state = INPUT_DEV_STATE_REL,
        .gesture = 0,
    };
    point_t gesture_vec;
    uint8_t gesture = 0;

    if (prev.state == INPUT_DEV_STATE_REL)
        goto out_exit;

    if (data->state == INPUT_DEV_STATE_PR) {
        gesture_vec.x = data->point.x - prev.point.x;
        gesture_vec.y = data->point.y - prev.point.y;

        if (ABS(gesture_vec.x) < POINTER_GESTURE_MIN_VEC &&
            ABS(gesture_vec.y) < POINTER_GESTURE_MIN_VEC) {
            gesture_sum.x = 0;
            gesture_sum.y = 0;
        }

        gesture_sum.x += gesture_vec.x;
        gesture_sum.y += gesture_vec.y;

        if (ABS(gesture_sum.x) > POINTER_GESTURE_LIMIT ||
            ABS(gesture_sum.y) > POINTER_GESTURE_LIMIT) {
            if (ABS(gesture_sum.x) >= ABS(gesture_sum.y)) {
                gesture = (gesture_sum.x > 0) ? GESTURE_DROP_RIGHT : GESTURE_DROP_LEFT;
            } else {
                gesture = (gesture_sum.y > 0) ? GESTURE_DROP_DOWN : GESTURE_DROP_UP;
            }
        }
    }
    else if (data->state == INPUT_DEV_STATE_REL) {
        gesture_sum.x = 0;
        gesture_sum.y = 0;
    }

out_exit:
    data->gesture = gesture;
    prev = *data;
}

static bool pointer_scan_read(input_drv_t* drv, input_dev_data_t* data)
{
    /* get the pointer location and pressed state */
    native_window_get_pointer_state(data);
    pointer_gesture_detect(data);
    return false;
}

static void pointer_scan_enable(input_drv_t* drv, bool enable)
{
}

int input_pointer_device_init(void)
{
    input_drv_t input_drv;

    input_drv.type = INPUT_DEV_TYPE_POINTER;
    input_drv.enable = pointer_scan_enable;
    input_drv.read_cb = pointer_scan_read;
    input_drv.input_dev = NULL;

    return input_driver_register(&input_drv);
}
