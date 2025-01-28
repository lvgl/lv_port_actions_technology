/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief watchdog hal interface
 */

#include <kernel.h>
#include <os_common_api.h>
#include <drivers/watchdog.h>
#include <watchdog_hal.h>
#include <soc.h>

const struct device *system_wdg_dev;
#ifndef CONFIG_WDT_MODE_RESET
void watchdog_overflow(const struct device *wdt_dev, int channel_id)
{
	printk("watchdog overflow");
	k_panic();
}
#endif
void watchdog_start(int timeout_ms)
{
	int err;
	struct wdt_timeout_cfg m_cfg_wdt0 = { 0 };

	system_wdg_dev = device_get_binding(CONFIG_WDT_ACTS_NAME);
	if (!system_wdg_dev) {
		SYS_LOG_ERR("cannot found watchdog device");
		return;
	}

#ifdef CONFIG_WDT_MODE_RESET
	m_cfg_wdt0.callback = NULL;
#else
	m_cfg_wdt0.callback = watchdog_overflow;
#endif

	m_cfg_wdt0.flags = WDT_FLAG_RESET_SOC;
	m_cfg_wdt0.window.max = timeout_ms;

	wdt_install_timeout(system_wdg_dev, &m_cfg_wdt0);

	err = wdt_setup(system_wdg_dev, 0);
	if (err < 0) {
		SYS_LOG_ERR("Watchdog setup error\n");
		return;
	}

	SYS_LOG_INF("enable watchdog (%d ms)", m_cfg_wdt0.window.max);

}

void watchdog_clear(void)
{
	if (system_wdg_dev) {
		wdt_feed(system_wdg_dev, 0);
	} else {
		soc_watchdog_clear();
	}
}

void watchdog_stop(void)
{
	SYS_LOG_INF("disable watchdog");

	if (system_wdg_dev) {
		wdt_disable(system_wdg_dev);
		system_wdg_dev = NULL;
	}
}
