/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file knob manager interface
 */

#include <os_common_api.h>
#include <string.h>
#include <key_hal.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <input_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include <property_manager.h>
#include <input_manager_inner.h>

struct k_work knob_work;
uint32_t knob_val = 0;


static void _knob_work_handler(struct k_work *work)
{

	sys_event_report_input(knob_val);
}


static void knob_notify_callback(struct device *dev,struct input_value *val)
{

	knob_val = val->keypad.value;
	k_work_submit(&knob_work);
}

int input_knob_device_init(void)
{
	// input_drv_t input_drv;
	const struct device *input_dev = device_get_binding(KNOB_ENCODER_DEV_NAME);
	if (!input_dev) {
		SYS_LOG_ERR("cannot found key dev knobencoder\n");
		return -ENODEV;
	}

	input_dev_register_notify(input_dev, knob_notify_callback);
	
    k_work_init(&knob_work, _knob_work_handler);
	return 0;
}
