/*
 * Copyright (c) 2017	Actions	Semiconductor	Co., Ltd
 *
 * SPDX-License-Identifier:	Apache-2.0
 */

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pwm.h>
#include <drivers/vibrator.h>
#include <soc.h>
#include <board_cfg.h>

#define	LOG_LEVEL  CONFIG_VIBRATOR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(vibrator);

#define	VIBRATOR_DEFAULT_FREQ_PARAM_HZ    8000

struct vibrat_acts_data	{
	const struct device *pwm_dev;
	u32_t chan_freq[4];
};

static int vibrat_acts_start(const struct device *dev, u32_t chan, u8_t	dutycycle)
{
	struct vibrat_acts_data	*data =	dev->data;
	u32_t freq;

	if (data->chan_freq[chan] == 0)
		freq = VIBRATOR_DEFAULT_FREQ_PARAM_HZ;
	else
		freq = data->chan_freq[chan];

	return pwm_pin_set_freq_duty(data->pwm_dev,	chan, freq,	dutycycle, 1);
}

static int vibrat_acts_stop(const struct device	*dev, u32_t	chan)
{
	struct vibrat_acts_data	*data =	dev->data;

	return pwm_pin_stop(data->pwm_dev, chan);
}

static int vibrat_acts_set_freq_param(const	struct device *dev,	u32_t chan,	u32_t freq)
{
	struct vibrat_acts_data	*data =	dev->data;	

	data->chan_freq[chan] =	freq;

	return 0;
}

const struct vibrat_driver_api vibrat_acts_driver_api =	{
	.start = vibrat_acts_start,
	.stop  = vibrat_acts_stop,
	.set_freq_param	= vibrat_acts_set_freq_param,
};

int	vibrat_acts_init(const struct device *dev)
{
	struct vibrat_acts_data	*data =	dev->data;

	data->pwm_dev =	device_get_binding(CONFIG_PWM_NAME);
	if (!data->pwm_dev)	{
		LOG_ERR("Bind PWM device error");
		return -ENOENT;
	}

	return 0;
}

static struct vibrat_acts_data vibrat_acts_data;

#if	IS_ENABLED(CONFIG_VIBRATOR)
DEVICE_DEFINE(vibrator_acts, CONFIG_VIBRATOR_DEV_NAME,
				vibrat_acts_init, NULL,
				&vibrat_acts_data, NULL,
				POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
				&vibrat_acts_driver_api);
#endif
