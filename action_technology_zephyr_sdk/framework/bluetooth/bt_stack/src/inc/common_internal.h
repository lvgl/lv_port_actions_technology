/** @file common_internal.h
 * @brief Bluetooth internel variables.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_INTERNAL_H__
#define __COMMON_INTERNAL_H__

#define __IN_BT_SECTION	__in_section_unique(bthost_bss)

struct bt_inner_value_t {
	uint32_t max_conn:4;
	uint32_t br_max_conn:3;
	uint32_t le_max_conn:4;
	uint32_t br_reserve_pkts:3;
	uint32_t le_reserve_pkts:3;
	uint32_t acl_tx_max:5;
	uint32_t rfcomm_max_credits:4;
	uint32_t avrcp_vol_sync:1;
	uint32_t pts_test_mode:1;
	uint32_t debug_log:1;
	uint32_t le_addr_type:2;
	uint32_t disable_hf_signal:1;
	uint16_t l2cap_tx_mtu;
	uint16_t rfcomm_l2cap_mtu;
	uint16_t avdtp_rx_mtu;
	uint16_t hf_features;
	uint16_t ag_features;
};

extern const struct bt_inner_value_t bt_inner_value;
#define bti_max_conn()                  bt_inner_value.max_conn
#define bt_internal_is_pts_test()       bt_inner_value.pts_test_mode
#define bti_is_pts_test()               bt_internal_is_pts_test()
#define bt_internal_debug_log()         bt_inner_value.debug_log
#define bti_debug_log()                 bt_internal_debug_log()
#define bti_br_max_conn()               bt_inner_value.br_max_conn
#define bti_le_max_conn()               bt_inner_value.le_max_conn
#define bti_le_addr_type()              bt_inner_value.le_addr_type
#define bti_avrcp_vol_sync()            bt_inner_value.avrcp_vol_sync
#define bti_disable_hf_signal()         bt_inner_value.disable_hf_signal

void *bt_malloc(size_t size);
void bt_free(void *ptr);
/**
 * @brief save value in property.
 * @param key the key of item.
 * @param value the value of item.
 * @param value_len length to set.
 * @return 0 on success, non-zero on failure.
 */
int bt_property_set(const char *key, char *value, int value_len);
/**
 * @brief get value in property.
 * @param key the key of item.
 * @param value the destination buffer.
 * @param value_len length to get.
 * @return return the length of value, otherwise nagative on failure.
 */
int bt_property_get(const char *key, char *value, int value_len);

/**
 * @brief Register property flush callback function.
 * @param key the key of item.
 */
int bt_property_reg_flush_cb(void *cb);

void hci_data_log_init(void);
void hci_data_log_debug(bool send, struct net_buf *buf);

#endif /* __COMMON_INTERNAL_H__ */
