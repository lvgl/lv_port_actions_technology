/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug manager interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "hotpug_manager"

#include <os_common_api.h>
#include <msg_manager.h>
#include <string.h>
#include <hotplug_manager.h>

struct linein_detect_state_t {
	uint8_t prev_state;
	uint8_t stable_state;
	uint8_t detect_cnt;
};

static struct linein_detect_state_t linein_detect_state;

int _linein_get_state(void)
{
	struct device *dev;
	int state;
	int ret;

	dev = device_get_binding("linein_detect");
	if (!dev) {
		SYS_LOG_ERR("dev not found\n");
		state = HOTPLUG_NONE;
		goto exit;
	}

	ret = hotplog_detect_state(dev, &state);
	if (ret < 0) {
		SYS_LOG_ERR("state error %d\n", ret);
		state = HOTPLUG_NONE;
		goto exit;
	}

exit:
	return state;
}

int _linein_hotplug_detect(void)
{
	int report_state = HOTPLUG_NONE;
	int state = HOTPLUG_NONE;

	state = _linein_get_state();

	if (state <= HOTPLUG_NONE) {
		goto exit;
	}

	if (state == linein_detect_state.prev_state) {
		linein_detect_state.detect_cnt++;
		if (linein_detect_state.detect_cnt >= 3) {
			linein_detect_state.detect_cnt = 0;
			if (state != linein_detect_state.stable_state) {
				linein_detect_state.stable_state = state;
				report_state = linein_detect_state.stable_state;
			}
		}
	} else {
		linein_detect_state.detect_cnt = 0;
		linein_detect_state.prev_state = state;
	}

exit:
	return report_state;
}

static const struct hotplug_device_t linein_hotplug_device = {
	.type = HOTPLUG_LINEIN,
	.get_state = _linein_get_state,
	.hotplug_detect = _linein_hotplug_detect,
};

int hotplug_linein_init(void)
{
	struct device *dev;
	int state;
	int ret;

	dev = device_get_binding("linein_detect");
	if (!dev) {
		SYS_LOG_ERR("dev not found\n");
		state = HOTPLUG_NONE;
		return -ENODEV;
	}

	ret = hotplog_detect_state(dev, &state);
	if (ret < 0) {
		SYS_LOG_ERR("state error %d\n", ret);
		state = HOTPLUG_NONE;
		return -ENODEV;
	}

	memset(&linein_detect_state, 0, sizeof(struct linein_detect_state_t));

	if (state == LINEIN_IN) {
		linein_detect_state.stable_state = HOTPLUG_IN;
	} else if (state == LINEIN_OUT) {
		linein_detect_state.stable_state = HOTPLUG_OUT;
	} else {
		linein_detect_state.stable_state = HOTPLUG_NONE;
	}

	return hotplug_device_register(&linein_hotplug_device);
}
