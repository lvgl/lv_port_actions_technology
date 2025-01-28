/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NVRAM storage interface
 */

#ifndef __NVRAM_STORAGE_H__
#define __NVRAM_STORAGE_H__

#ifndef PC
#include <kernel.h>
#include <device.h>
#endif

#define NVRAM_MAX_NAME_SIZE		112
#define NVRAM_MAX_DATA_SIZE		512

#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)
extern void nvram_storage_flush(struct device *dev);
#else
#define nvram_storage_flush(dev)  do{}while(false)
#endif

extern struct device *nvram_storage_init(void);
extern int nvram_storage_read(struct device *dev, uint64_t addr,
			      void *buf, int32_t size);
extern int nvram_storage_write(struct device *dev, uint64_t addr,
			       const void *buf, int32_t size);
extern int nvram_storage_erase(struct device *dev, uint64_t addr, int32_t size);

#endif/* #define __NVRAM_STORAGE_H__ */
