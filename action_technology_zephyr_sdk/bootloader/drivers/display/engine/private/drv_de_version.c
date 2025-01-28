/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief drv display engine version interface
 */
#include <kernel.h>

#define DRV_DE_VERSION_NUMBER     0x01000000
#define DRV_DE_VERSION_STRING     "1.0.0"

uint32_t drv_de_version_dump(void)
{
	printk("drv_display_engine: version %s ,release time: %s:%s\n", DRV_DE_VERSION_STRING, __DATE__, __TIME__);
	return DRV_DE_VERSION_NUMBER;
}