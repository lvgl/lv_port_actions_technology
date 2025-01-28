# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_PWM_ACTS pwm_acts.c)
zephyr_library_sources_ifdef(CONFIG_VIBRATOR_ACTS vibrator_acts.c)
