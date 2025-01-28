/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_brightness.h"
#include <sys_manager.h>
#include <board_cfg.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include <drivers/display.h>

namespace gx {
static bool lock_flag = false;

int screen_brightness_get() {
	const struct device *display_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	if (display_dev == NULL) {
		return -1;
	}
	return (int)display_get_brightness(display_dev);
}

int screen_brightness_set(int brightness) {
	const struct device *display_dev = device_get_binding(CONFIG_LCD_DISPLAY_DEV_NAME);
	if (display_dev == NULL) {
		return -1;
	}
	display_set_brightness(display_dev, (uint8_t)brightness);
	return 0;
}

int screen_brightness_mode() {
	return 0;
}

int screen_brightness_set_mode(int mode) {
	(void)mode;
	return 0;
}

int screen_off() {
	system_request_fast_standby();
	return 0;
}

int screen_on() {
	system_clear_fast_standby();
	sys_wake_lock(FULL_WAKE_LOCK);
	sys_wake_unlock(FULL_WAKE_LOCK);
	return 0;
}

int screen_off_time_set(unsigned int time) {
	system_set_auto_sleep_time(time);
	return 0;
}

void screen_always_bright(int isAlways) {
	if (isAlways) {
		if (lock_flag == false) {
			sys_wake_lock(FULL_WAKE_LOCK);
			lock_flag = true;
		}
	} else {
		if (lock_flag == true) {
			sys_wake_unlock(FULL_WAKE_LOCK);
			lock_flag = false;
		}
	}
}

} // namespace gx
