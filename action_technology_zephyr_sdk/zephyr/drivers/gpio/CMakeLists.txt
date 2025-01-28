# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_GPIO_ACTS	gpio_acts.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_SHELL	gpio_shell.c)
zephyr_library_sources_ifdef(CONFIG_EXTEND_GPIO_ET6416	gpio_et6416.c)
