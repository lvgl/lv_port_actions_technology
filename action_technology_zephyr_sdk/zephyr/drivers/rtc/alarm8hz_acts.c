/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Alarm 8HZ driver for Actions SoC
 */

#include <errno.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/alarm.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(alarm8hz0, CONFIG_LOG_DEFAULT_LEVEL);

struct acts_alarm8hz_data {
	void (*alarm_cb_fn)(const void *cb_data);
	const void *cb_data;
	uint8_t alarm_wakeup : 1; /* System wakeup from alarm8hz indicator */
};

static int alarm8hz_acts_set_alarm(const struct device *dev, struct alarm_config *config, bool enable)
{
	struct acts_alarm8hz_data *data = dev->data;
	int ret = 0;

	if ((!config) && enable) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	if (enable) {
		ret = soc_pmu_alarm8hz_enable(config->alarm_msec);
		if (ret)
			return ret;

		data->alarm_cb_fn = config->cb_fn;
		data->cb_data = config->cb_data;
	} else {
		soc_pmu_alarm8hz_disable();

		data->alarm_cb_fn = NULL;
		data->cb_data = NULL;
	}

	return ret;
}

static int alarm8hz_acts_get_alarm(const struct device *dev, struct alarm_status *sts)
{
	struct acts_alarm8hz_data *data = dev->data;

	if (!sts) {
		LOG_ERR("Invalid parameter");
		return -EINVAL;
	}

	if (data->alarm_cb_fn)
		sts->is_on = 1;
	else
		sts->is_on = 0;

	sts->alarm_msec = soc_pmu_get_alarm8hz() * 125;

	return 0;
}

static bool alarm8hz_acts_is_alarm_wakeup(const struct device *dev)
{
	struct acts_alarm8hz_data *data = dev->data;
	if (data->alarm_wakeup)
		return true;
	else
		return false;
}

static void alarm8hz_acts_pmu_notify(void *cb_data, int state)
{
	const struct device *dev = (const struct device *)cb_data;
	struct acts_alarm8hz_data *data = dev->data;

	LOG_DBG("on alarm8hz func:%p", data->alarm_cb_fn);
	LOG_INF("alarm8hz wakeup cur_8hz=%d\n", soc_pmu_get_counter8hz_cycles(false));
	if (data->alarm_cb_fn)
		data->alarm_cb_fn(data->cb_data);
}

const struct alarm_driver_api alarm8hz_acts_driver_api = {
	.set_alarm = alarm8hz_acts_set_alarm,
	.get_alarm = alarm8hz_acts_get_alarm,
	.is_alarm_wakeup = alarm8hz_acts_is_alarm_wakeup,
};

int alarm8hz_acts_init(const struct device *dev)
{
	struct acts_alarm8hz_data *data = dev->data;
	struct detect_param_t detect_param = {PMU_DETECT_DEV_ALARM8HZ, alarm8hz_acts_pmu_notify, (void *)dev};

	if (soc_pmu_is_alarm_wakeup()) {
		data->alarm_wakeup = true;
		LOG_INF("alarm8hz wakeup");
	}

	soc_pmu_counter8hz_enable();

	if (soc_pmu_register_notify(&detect_param)) {
		LOG_ERR("failed to register pmu notify");
		return -ENXIO;
	}

	printk("alarm8hz initialized\n");

	return 0;
}


struct acts_alarm8hz_data alarm8hz_acts_ddata;

DEVICE_DEFINE(alarm8hz0, CONFIG_ALARM8HZ_0_NAME, alarm8hz_acts_init, NULL,
		    &alarm8hz_acts_ddata, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &alarm8hz_acts_driver_api);

