/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_vibrator.h"
#include <drivers/vibrator.h>
#include <board.h>
#include <os_common_api.h>

namespace gx {
const struct device *vib_dev = NULL;
unsigned int vib_pwm_chan;
os_delayed_work vib_delay_work;

void vib_delay_work_handler(struct k_work *work) {
	if (vib_dev)
		vibrat_stop(vib_dev, vib_pwm_chan);
}

static int vibrator_init(const struct device *dev) {
	ARG_UNUSED(dev);
	struct board_pwm_pinmux_info pinmux_info;

	vib_dev = device_get_binding(CONFIG_VIBRATOR_DEV_NAME);
	board_get_pwm_pinmux_info(&pinmux_info);
	vib_pwm_chan = pinmux_info.pins_config->pin_chan;
	if (vib_dev) {
		vibrat_set_freq_param(vib_dev, vib_pwm_chan, 200);
	}
	os_delayed_work_init(&vib_delay_work, vib_delay_work_handler);

	return 0;
}
SYS_INIT(vibrator_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

bool vibrator_set(VibratorMode mode) {
	if (vib_dev == NULL) {
		return 0;
	}

	vibrat_start(vib_dev, vib_pwm_chan, 60);
	if (mode == ShortVibrate) {
		os_delayed_work_submit(&vib_delay_work, 500);
	} else if (mode == LongVibrate) {
		os_delayed_work_submit(&vib_delay_work, 2000);
	}
	return 0;
}

} // namespace gx
