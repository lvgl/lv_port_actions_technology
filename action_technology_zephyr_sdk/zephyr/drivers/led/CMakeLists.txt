# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_ACT_LED act_led_display.c)
zephyr_sources_ifdef(CONFIG_ACT_DYNAMIC_LED act_dynamic_led_display.c)