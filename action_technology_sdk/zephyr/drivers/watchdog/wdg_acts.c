/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/watchdog.h>
#include <soc.h>
#include <board_cfg.h>

#define WD_CTL_EN				(1 << 4)
#define WD_CTL_CLKSEL_MASK		(0x7 << 1)
#define WD_CTL_CLKSEL_SHIFT		(1)
#define WD_CTL_CLKSEL(n)		((n) << WD_CTL_CLKSEL_SHIFT)
#define WD_CTL_CLR				(1 << 0)
#define WD_CTL_IRQ_PD			(1 << 6)
#define WD_CTL_IRQ_EN			(1 << 5)

/* timeout: 176ms * 2^n */
#define WD_TIMEOUT_CLKSEL_TO_MS(n)	(176 << (n))
#define WD_TIMEOUT_MS_TO_CLKSEL(ms)	(find_msb_set(((ms) + 1) / 176))

static wdt_callback_t g_irq_handler_wdg;

DEVICE_DECLARE(wdg0);

static void wdg_acts_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	sys_write32(sys_read32(WD_CTL) | WD_CTL_EN, WD_CTL);
}

static int wdg_acts_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	sys_write32(sys_read32(WD_CTL) & ~WD_CTL_EN, WD_CTL);

	return 0;
}

static void wdg_acts_clear_irq_pd(void)
{
	sys_write32(sys_read32(WD_CTL), WD_CTL);
}

static void wdg_irq_handler(void *arg)
{
	wdg_acts_clear_irq_pd();

	if (g_irq_handler_wdg)
		g_irq_handler_wdg(arg, 0);
}

static int wdg_acts_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	sys_write32(sys_read32(WD_CTL) | WD_CTL_CLR, WD_CTL);

	return 0;
}

static int wdg_acts_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(options);

	wdg_acts_enable(dev);

	return 0;
}

static int wdg_acts_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *cfg)
{
	int clksel;

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.max == 0U)
		return -EINVAL;

	ARG_UNUSED(dev);

	clksel = WD_TIMEOUT_MS_TO_CLKSEL(cfg->window.max);

	if (cfg->callback == NULL) {
		g_irq_handler_wdg = NULL;
		irq_disable(IRQ_ID_WD2HZ);

		sys_write32((sys_read32(WD_CTL) & ~WD_CTL_CLKSEL_MASK)
			| WD_CTL_CLKSEL(clksel), WD_CTL);

		sys_write32((sys_read32(WD_CTL) & ~WD_CTL_IRQ_EN), WD_CTL);
	} else {
		g_irq_handler_wdg = cfg->callback;
		IRQ_CONNECT(IRQ_ID_WD2HZ, CONFIG_WDT_0_IRQ_PRI, wdg_irq_handler, DEVICE_GET(wdg0), 0);
		irq_enable(IRQ_ID_WD2HZ);

		sys_write32((sys_read32(WD_CTL) & ~WD_CTL_CLKSEL_MASK)
			| WD_CTL_CLKSEL(clksel) | WD_CTL_IRQ_EN, WD_CTL);
	}

	return 0;
}

static const struct wdt_driver_api wdg_acts_driver_api = {
	.setup = wdg_acts_setup,
	.disable = wdg_acts_disable,
	.install_timeout = wdg_acts_install_timeout,
	.feed = wdg_acts_feed,
};

static int wdg_acts_init(const struct device *dev)
{
	sys_write32(0x0, WD_CTL);
	wd_clear_wdreset_cnt();
#ifdef CONFIG_WDT_ACTS_START_AT_BOOT
	wdg_acts_enable(dev);
#endif

	return 0;
}

DEVICE_DEFINE(wdg0, CONFIG_WDT_ACTS_NAME, wdg_acts_init, NULL, NULL, NULL,
		    APPLICATION, 91,
		    &wdg_acts_driver_api);

