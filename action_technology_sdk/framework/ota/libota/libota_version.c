/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lib OTA version interface
 */
#include <os_common_api.h>
#include <kernel.h>

#define LIBOTA_VERSION_NUMBER     0x01000000
#define LIBOTA_VERSION_STRING     "1.0.0"

uint32_t libota_version_dump(void)
{
	os_printk("libota: version %s ,release time: %s:%s\n",LIBOTA_VERSION_STRING, __DATE__, __TIME__);
	return LIBOTA_VERSION_NUMBER;
}