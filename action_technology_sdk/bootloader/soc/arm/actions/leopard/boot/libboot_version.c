/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lib boot version interface
 */
#include <kernel.h>

#define LIBBOOT_VERSION_NUMBER     0x01000000
#define LIBBOOT_VERSION_STRING     "1.0.2"

uint32_t libboot_version_dump(void)
{
	printk("libboot: version %s ,release time: %s:%s\n",LIBBOOT_VERSION_STRING, __DATE__, __TIME__);
	return LIBBOOT_VERSION_NUMBER;
}