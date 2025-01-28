/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#ifdef CONFIG_SPI_FLASH_ACTS
#include <flash/spi_flash.h>
#endif
#include "de_device.h"

void nor_xip_set_locked(bool locked)
{
#ifdef CONFIG_SPI_FLASH_ACTS
	if (locked) {
		spi0_nor_xip_lock();
	} else {
		spi0_nor_xip_unlock();
	}
#endif
}

DEVICE_DECLARE(de);

static int de_drv_init(const struct device *dev)
{
	int res = de_init(dev);

	if (res == 0) {
		IRQ_CONNECT(IRQ_ID_DE, 0, de_isr, DEVICE_GET(de), 0);
		irq_enable(IRQ_ID_DE);
	}

	clk_set_rate(CLOCK_ID_DE, KHZ(CONFIG_DISPLAY_ENGINE_CLOCK_KHZ));

	return res;
}

#if IS_ENABLED(CONFIG_DISPLAY_ENGINE_DEV)
DEVICE_DEFINE(de, CONFIG_DISPLAY_ENGINE_DEV_NAME, &de_drv_init,
		de_pm_control, &de_drv_data, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &de_drv_api);
#endif
