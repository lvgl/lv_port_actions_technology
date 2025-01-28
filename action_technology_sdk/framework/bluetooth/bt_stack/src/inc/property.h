/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <property_manager.h>

/* Max settings key length (with all components) */
#define BT_PROPERTY_KEY_MAX 36

/* Base64-encoded string buffer size of in_size bytes */
#define BT_SETTINGS_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

#if 0
/* Helpers for keys containing a bdaddr */
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key);
int bt_settings_decode_key(const char *key, bt_addr_le_t *addr);

void bt_settings_save_id(void);

int bt_settings_init(void);
#endif

/* Actions add start */
/**
 * @brief read the data from the storage by property.
 * @param[in] cb_arg key of key-value pair in the storage.
 * @param[out] data  the destination buffer
 * @param[in] len    length of read
 * @return positive: Number of bytes read.
 *                   On error returns -ERRNO code.
 */
typedef int (*property_read_cb)(const char *cb_arg, char *data, int len);

/* Helpers for encode bdaddr with id as string */
void bt_property_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key);
/* Helpers for decode string into bdaddr */
int bt_property_decode_key(const char *key, bt_addr_le_t *addr);
void bt_property_save_id(void);
int bt_property_name_next(const char *name, const char **next);
int bt_property_delete(const char *name);
int bt_property_set_le_name(char *value, int value_len);
int bt_property_get_le_name(char *value, int value_len);
int bt_property_get_br_name(char *value, int value_len);
void bt_property_load(void);
/* Actions add end */
