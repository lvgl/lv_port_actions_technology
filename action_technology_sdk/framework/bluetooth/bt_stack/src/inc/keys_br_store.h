/** @file keys_br_store.h
 * @brief Bluetooth store br keys.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __KEYS_BR_STORE_H__
#define __KEYS_BR_STORE_H__

int bt_store_read_linkkey(const bt_addr_t *addr, void *key);
void bt_store_write_linkkey(const bt_addr_t *addr, const void *key);
void bt_store_clear_linkkey(const bt_addr_t *addr);

int bt_store_br_init(void);
int bt_store_br_deinit(void);

int bt_store_get_linkkey(struct br_linkkey_info *info, uint8_t cnt);
int bt_store_find_linkkey(const bt_addr_t *addr, void *key);
int bt_store_update_linkkey(struct br_linkkey_info *info, uint8_t cnt);
void bt_store_write_ori_linkkey(bt_addr_t *addr, uint8_t *link_key);

#endif /* __KEYS_BR_STORE_H__ */
