/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
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

K_MSGQ_DEFINE(encoder_scan_msgq, sizeof(input_dev_data_t), 10, 4);
#ifdef CONFIG_INPUT_DEV_ACTS_ADC_SR
static void encoder_scan_callback(struct device *dev, uint32_t row,
					uint32_t col, bool pressed)
{
	input_dev_data_t data = {
		.point.x = col,
		.point.y = row,
		.state = pressed ? INPUT_DEV_STATE_PR : INPUT_DEV_STATE_REL,
	};

	if (k_msgq_put(&encoder_scan_msgq, &data, K_NO_WAIT) != 0) {
		SYS_LOG_ERR("Could put input data into queue");
	}
}

static bool encoder_scan_read(input_drv_t *drv, input_dev_data_t *data)
{
	static input_dev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = INPUT_DEV_STATE_REL,
	};

	input_dev_data_t curr;

	if (k_msgq_get(&encoder_scan_msgq, &curr, K_NO_WAIT) == 0) {
		prev = curr;
	}

	*data = prev;

	return k_msgq_num_used_get(&encoder_scan_msgq) > 0;
}
#endif
int input_encoder_device_init(void)
{
#ifdef CONFIG_INPUT_DEV_ACTS_ADC_SR
	input_drv_t input_drv;
	struct device *input_dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADC_SR_NAME);
	if (!input_dev) {
		SYS_LOG_ERR("cannot found key dev adckey\n");
		return -ENODEV;
	}

	input_dev_register_notify(input_dev, encoder_scan_callback);

	input_drv.type = INPUT_DEV_TYPE_ENCODER;
	input_drv.read_cb = encoder_scan_read;

	input_driver_register(&input_drv);
#endif
	return 0;
}
