# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_RTC_ACTS rtc_acts.c rtc_lib.c)
zephyr_library_sources_ifdef(CONFIG_ALARM8HZ_ACTS alarm8hz_acts.c)