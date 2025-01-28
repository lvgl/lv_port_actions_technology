/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file property manager interface
 */
#include <os_common_api.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/util.h>
#include <property_inner.h>
#define SYS_LOG_DOMAIN "property"
#ifndef SYS_LOG_LEVEL
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DEFAULT_LEVEL
#endif

int property_get(const char *key, char *value, int value_len)
{
	int ret = 0;
#ifdef CONFIG_PROPERTY_CACHE
	ret = property_cache_get(key, value, value_len);
#else
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_get(key, value, value_len);
	if (ret < 0) {
		return -ENOENT;
	}
#endif
#endif
	return ret;
}

int property_set(const char *key, char *value, int value_len)
{
	int ret = -ENOENT;
#ifdef CONFIG_PROPERTY_CACHE
	ret = property_cache_set(key, value, value_len);
#else
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_set(key, value, value_len);
#endif
#endif
	return ret;
}

int property_set_factory(const char *key, char *value, int value_len)
{
	int ret = -ENOENT;
#ifdef CONFIG_NVRAM_CONFIG
	ret = nvram_config_set_factory(key, value, value_len);
#endif
	return ret;
}

int property_get_int(const char *key, int default_value)
{
	uint32_t property = 0;
	char temp_data[16] = {0};

	if (property_get(key, temp_data, sizeof(temp_data)) > 0) {
		property = atoi(temp_data);
	} else {
		property = default_value;
	}

	return property;
}

int property_set_int(const char *key, int value)
{
	int ret = 0;
	char temp_data[16] = {0};

	snprintf(temp_data, sizeof(temp_data), "%d", value);

	if (!property_set(key, temp_data, strlen(temp_data))) {
		SYS_LOG_ERR("key %s value %d\n", key, value);
		ret = -EACCES;
	}

	return ret;
}

int property_get_byte_array(const char *key, char *value, int value_len, const char *default_value)
{
	char temp_data[16] = {0};
	int len = property_get(key, temp_data, sizeof(temp_data));

	if (len > 0) {
		if (len > (value_len*2)) {
			len = value_len*2;
		}
		hex2bin(temp_data, len, value, value_len);
	} else {
		memcpy(value, default_value, value_len);
	}

	return 0;
}

int property_set_byte_array(const char *key, char *value, int value_len)
{
	int ret = 0;
	char temp_data[16] = {0};
	int off = 0;

	for (int i = 0 ; i < value_len; i++) {
		if (!off) {
			off += snprintf(&temp_data[off], sizeof(temp_data) - off, ",");
		}
		off += snprintf(&temp_data[off], sizeof(temp_data) - off, "%d", value[i]);
	}

	if (!property_set(key, temp_data, sizeof(temp_data))) {
		SYS_LOG_ERR("key %s value %s\n", key, value);
		ret = -EACCES;
	}

	return ret;
}

int property_get_int_array(const char *key, int *value, int value_len,  const short *default_value)
{
	char temp_data[16] = {0};
	char *str_begin = NULL;
	char *str_end = NULL;
	int index = 0;

	if (property_get(key, temp_data, sizeof(temp_data)) > 0) {
		str_begin = temp_data;
		while (!str_begin) {
			str_end = strstr(str_begin, ",");
			if (str_end) {
				*str_end = 0;
			}
			value[index++] = atoi(str_begin);
			str_begin = str_end + 1;
		}
	} else {
		memcpy(value, default_value, value_len);
	}

	return 0;
}

int property_set_int_array(const char *key, int *value, int value_len)
{
	int ret = 0;
	char temp_data[16] = {0};
	int off = 0;

	for (int i = 0 ; i < value_len; i++) {
		if (!off) {
			off += snprintf(&temp_data[off], sizeof(temp_data) - off, ",");
		}
		off += snprintf(&temp_data[off], sizeof(temp_data) - off, "%d", value[i]);
	}

	if (!property_set(key, temp_data, sizeof(temp_data))) {
		SYS_LOG_ERR("key %s error\n", key);
		ret = -EACCES;
	}

	return ret;
}

int property_flush(const char *key)
{
#ifdef CONFIG_PROPERTY_CACHE
	property_cache_flush(key);
#endif
	return 0;
}

int property_flush_req(const char *key)
{
#ifdef CONFIG_PROPERTY_CACHE
	property_cache_flush_req(key);
#endif
	return 0;
}

int property_flush_req_deal(void)
{
#ifdef CONFIG_PROPERTY_CACHE
	property_cache_flush_req_deal();
#endif
	return 0;
}


int property_manager_init(void)
{
#ifdef CONFIG_PROPERTY_CACHE
	property_cache_init();
#endif
	return 0;
}
