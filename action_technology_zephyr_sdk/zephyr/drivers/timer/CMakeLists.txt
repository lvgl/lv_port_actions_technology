# SPDX-License-Identifier: Apache-2.0

zephyr_sources(                         sys_clock_init.c)
zephyr_sources_ifdef(CONFIG_ACTS_TIMER  acts_timer.c)
zephyr_sources_ifdef(CONFIG_ACTS_HRTIMER  acts_hrtimer.c)
zephyr_sources_ifdef(CONFIG_CORTEX_M_SYSTICK cortex_m_systick.c)
