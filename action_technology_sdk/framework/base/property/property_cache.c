/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file property cache interface
 */
 #include <os_common_api.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <mem_manager.h>
#include <property_inner.h>
#define SYS_LOG_DOMAIN "property"
#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif

#define MAX_NVRAM_ITEM_CACHE_NUM 15

struct cahce_item_data {
	char *name;
	char *data;
	uint32_t data_len:16;
	uint32_t used_flag:2;
	uint32_t flush_req:1;
};

OS_MUTEX_DEFINE(nvram_cache_mutex);

static struct cahce_item_data globle_property_cache[MAX_NVRAM_ITEM_CACHE_NUM];


static struct cahce_item_data *find_property_cache(const char *name)
{
	int i;
	struct cahce_item_data *item = NULL;

	for (i = 0; i < MAX_NVRAM_ITEM_CACHE_NUM; i++) {
		item = &globle_property_cache[i];
		if (item->used_flag && !memcmp(item->name, name, strlen(name))) {
			SYS_LOG_INF(" %d %s\n", i, item->name);
			return item;
		}
	}
	return NULL;
}

static struct cahce_item_data *get_property_cache(const char *name, int len)
{
	int i;
	struct cahce_item_data *item = NULL;

	for (i = 0; i < MAX_NVRAM_ITEM_CACHE_NUM; i++) {
		item = &globle_property_cache[i];
		if (!item->used_flag) {
			item->name = mem_malloc(strlen(name) + 1);
			if (!item->name) {
				item = NULL;
				goto exit;
			}
			memset(item->name, 0, strlen(name) + 1);
			if (len > 0) {
				item->data = mem_malloc(len);
				if (!item->data) {
					mem_free(item->name);
					item = NULL;
					goto exit;
				}
				memset(item->data, 0, len);
			}
exit:
			return item;
		}
	}
	return NULL;
}

static int put_property_cache(struct cahce_item_data *item)
{
	if (item) {
		if (item->data) {
			mem_free(item->data);
		}
		if (item->name) {
			mem_free(item->name);
		}
		item->data = NULL;
		item->name = NULL;
		item->used_flag = 0;
		item->data_len = 0;
		item->flush_req = 0;
	}
	return 0;
}

int property_cache_get(const char *name, void *data, int len)
{
	int read_len = 0;
	struct cahce_item_data *item = NULL;

	os_mutex_lock(&nvram_cache_mutex, OS_FOREVER);

	item = find_property_cache(name);

	/**read from nvram cache */
	if (item) {
		if (item->data_len > len) {
			read_len = len;
		} else {
			read_len = item->data_len;
		}
		memcpy(data, item->data, read_len);
	} else {
#ifdef CONFIG_NVRAM_CONFIG
		/** read from nvram*/
		read_len = nvram_config_get(name, data, len);
#endif
	}

	os_mutex_unlock(&nvram_cache_mutex);

	return read_len;
}

int property_cache_set(const char *name, const void *data, int len)
{
	int ret = 0;
	struct cahce_item_data *item = NULL;

	os_mutex_lock(&nvram_cache_mutex, OS_FOREVER);

	item = find_property_cache(name);
	/**write to old nvram cache */
	if (item) {
		if (item->data_len == len) {
			memcpy(item->data, data, len);
			ret = 0;
		} else {
			void *old_data = item->data;
			if (len > 0) {
				item->data = mem_malloc(len);
				memcpy(item->data, data, len);
			}
			item->data_len = len;
			ret = 0;
			mem_free(old_data);
		}
		goto exit;
	}

	/**write to new nvram cache */
	item = get_property_cache(name, len);
	if (item) {
		/** new item */
		memcpy(item->name, name, strlen(name));
		memcpy(item->data, data, len);
		item->data_len = len;
		item->used_flag = 1;
		ret = 0;
		goto exit;
	}

	/** direct write to nvram*/
	SYS_LOG_INF("direct write to nvram\n");

#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_set(name, data, len);
#endif

exit:
	os_mutex_unlock(&nvram_cache_mutex);
	return ret;
}

int property_cache_flush(const char *name)
{
	int i;
	struct cahce_item_data *item = NULL;

	os_mutex_lock(&nvram_cache_mutex, OS_FOREVER);

	for (i = 0; i < MAX_NVRAM_ITEM_CACHE_NUM; i++) {
		item = &globle_property_cache[i];
		if (item->used_flag &&
	     ((!name) || memcmp(item->name, name, strlen(name)) == 0)) {
#ifdef CONFIG_NVRAM_CONFIG
			if (!nvram_config_set(item->name, item->data, item->data_len)) {
				put_property_cache(item);
				item->flush_req = false;
			}
#endif

		}
	}

	os_mutex_unlock(&nvram_cache_mutex);

	SYS_LOG_INF("ok\n");
	return 0;
}

int property_cache_flush_req(const char *name)
{
	int i;
	struct cahce_item_data *item = NULL;

	os_mutex_lock(&nvram_cache_mutex, OS_FOREVER);

	for (i = 0; i < MAX_NVRAM_ITEM_CACHE_NUM; i++) {
		item = &globle_property_cache[i];
		if (item->used_flag &&
	     ((!name) || memcmp(item->name, name, strlen(name)) == 0)) {
			item->flush_req = true;
		}
	}

	os_mutex_unlock(&nvram_cache_mutex);

	SYS_LOG_INF("ok\n");
	return 0;
}

int property_cache_flush_req_deal(void)
{
	int i;
	struct cahce_item_data *item = NULL;

	os_mutex_lock(&nvram_cache_mutex, OS_FOREVER);

	for (i = 0; i < MAX_NVRAM_ITEM_CACHE_NUM; i++) {
		item = &globle_property_cache[i];
		if (item->used_flag && item->flush_req) {
#ifdef CONFIG_NVRAM_CONFIG
			if (!nvram_config_set(item->name, item->data, item->data_len)) {
				put_property_cache(item);
			}
#endif
			item->flush_req = false;
		}
	}

	os_mutex_unlock(&nvram_cache_mutex);

	SYS_LOG_INF("flush ok\n");
	return 0;
}

int property_cache_init(void)
{
	memset(globle_property_cache, 0, sizeof(globle_property_cache));
	return 0;
}
