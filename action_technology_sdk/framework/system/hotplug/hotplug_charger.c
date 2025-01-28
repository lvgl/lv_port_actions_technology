/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug manager interface
 */

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "hotpug_manager"
#endif
#include <os_common_api.h>
#include <msg_manager.h>
#include <string.h>
#include <hotplug_manager.h>
#include <soc_pmu.h>

struct charger_detect_state_t {
	uint8_t prev_state;
	uint8_t stable_state;
	uint8_t detect_cnt;
};

static struct charger_detect_state_t charger_detect_state;

int _charger_get_state(void)
{
	int state;

	if (soc_pmu_get_dc5v_status()) {
		state = HOTPLUG_IN;
	} else {
		state = HOTPLUG_OUT;
	}

	return state;
}

int _charger_hotplug_detect(void)
{
	int report_state = HOTPLUG_NONE;
	int state = HOTPLUG_NONE;

	state = _charger_get_state();

	if (state <= HOTPLUG_NONE) {
		goto exit;
	}

	if (state == charger_detect_state.prev_state) {
		charger_detect_state.detect_cnt++;
		if (charger_detect_state.detect_cnt >= 1) {
			charger_detect_state.detect_cnt = 0;
			if (state != charger_detect_state.stable_state) {
				charger_detect_state.stable_state = state;
				report_state = charger_detect_state.stable_state;
			}
		}
	} else {
		charger_detect_state.detect_cnt = 0;
		charger_detect_state.prev_state = state;
	}

exit:
	return report_state;
}

static const struct hotplug_device_t charger_hotplug_device = {
	.type = HOTPLUG_CHARGER,
	.get_state = _charger_get_state,
	.hotplug_detect = _charger_hotplug_detect,
};

int hotplug_charger_init(void)
{
	memset(&charger_detect_state, 0, sizeof(struct charger_detect_state_t));

	if (soc_pmu_get_dc5v_status()) {
		charger_detect_state.stable_state = HOTPLUG_IN;
	} else {
		charger_detect_state.stable_state = HOTPLUG_NONE;
	}

	return hotplug_device_register(&charger_hotplug_device);
}
