/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
 */


#ifndef __INPUT_MANGER_INNER_H__
#define __INPUT_MANGER_INNER_H__
#include<input_manager.h>

int input_driver_register(input_drv_t *input_drv);

int input_pointer_device_init(void);

int input_keypad_device_init(event_trigger event_cb);

int input_encoder_device_init(void);

int input_knob_device_init(void);

#endif

