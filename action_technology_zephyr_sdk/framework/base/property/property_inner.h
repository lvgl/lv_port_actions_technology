/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file property manager interface
 */

#ifndef __PROPERTY_INNER_H__
#define __PROPERTY_INNER_H__

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#ifdef CONFIG_PROPERTY_CACHE

int property_cache_get(const char *name, void *data, int len);

int property_cache_set(const char *name, const void *data, int len);

int property_cache_flush(const char *name);

int property_cache_flush_req(const char *name);

int property_cache_flush_req_deal(void);

int property_cache_init(void);

#endif

#endif
