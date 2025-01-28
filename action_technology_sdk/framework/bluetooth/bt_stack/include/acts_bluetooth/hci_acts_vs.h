/** @file hci_acts_vs.h
 * @brief Bluetooth actions hci vendor command/event.
 *
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HCI_ACTS_VS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HCI_ACTS_VS_H_

#include <acts_bluetooth/hci.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Vendor commands */

#define BT_HCI_OP_VS_SET_APLL_TEMP_COMP				BT_OP(BT_OGF_VS, 0x0030)
struct bt_hci_cp_vs_set_apll_temp_comp {
	uint8_t enable;
} __packed;

#define BT_HCI_OP_VS_DO_APLL_TEMP_COMP				BT_OP(BT_OGF_VS, 0x0031)

#define BT_HCI_OP_VS_READ_BT_BUILD_INFO				BT_OP(BT_OGF_VS, 0x0032)
struct bt_hci_op_vs_read_bt_build_info {
	uint8_t status;
	uint16_t rf_version;
	uint16_t bt_version;
	uint16_t board_type;
	uint16_t svn_version;
	uint8_t str_data[128];
} __packed;

#define BT_HCI_OP_VS_LE_WRITE_LOCAL_FEATURES		BT_OP(BT_OGF_VS, 0x0033)
struct bt_hci_cp_vs_le_write_local_features {
	uint8_t features[8];
} __packed;

#define BT_HCI_OP_ACTS_VS_ADJUST_LINK_TIME		BT_OP(BT_OGF_VS, 0x0080)
struct bt_hci_cp_acts_vs_adjust_link_time {
	uint16_t handle;
	int16_t value;
} __packed;

#define BT_HCI_OP_ACTS_VS_READ_BT_CLOCK			BT_OP(BT_OGF_VS, 0x0081)
struct bt_hci_cp_acts_vs_read_bt_clock {
	uint16_t handle;
} __packed;

struct bt_hci_rp_acts_vs_read_bt_clock {
	uint8_t  status;
	uint32_t bt_clk;
	uint16_t bt_intraslot;
} __packed;

#define BT_HCI_OP_ACTS_VS_SET_TWS_INT_CLOCK		BT_OP(BT_OGF_VS, 0x0082)
struct bt_hci_cp_acts_vs_set_tws_int_clock {
	uint16_t handle;
	uint32_t bt_clk;
	uint16_t bt_intraslot;
} __packed;

#define BT_HCI_OP_ACTS_VS_ENABLE_TWS_INT		BT_OP(BT_OGF_VS, 0x0083)
struct bt_hci_cp_acts_vs_enable_tws_int {
	uint8_t enable;
} __packed;

#define BT_HCI_OP_ACTS_VS_ENABLE_ACL_FLOW_CTRL 	BT_OP(BT_OGF_VS, 0x0084)
struct bt_hci_cp_acts_vs_enable_acl_flow_ctrl {
	uint16_t handle;
	uint8_t  enable;
} __packed;

#define BT_HCI_OP_ACTS_VS_READ_ADD_NULL_CNT		BT_OP(BT_OGF_VS, 0x0085)
struct bt_hci_cp_acts_vs_read_add_null_cnt {
	uint16_t handle;
} __packed;
struct bt_hci_rp_acts_vs_read_add_null_cnt {
	uint8_t  status;
	uint16_t null_cnt;
} __packed;

#define BT_HCI_OP_ACTS_VS_SET_TWS_SYNC_INT_TIME	BT_OP(BT_OGF_VS, 0x0086)
struct bt_hci_cp_acts_vs_set_tws_sync_int_time {
	uint16_t handle;
	uint8_t  tws_index;
	uint8_t  time_mode;
	uint32_t rx_bt_clk;
	uint16_t rx_bt_slot;
} __packed;

#define BT_HCI_OP_ACTS_VS_SET_TWS_INT_DELAY_PLAY_TIME	BT_OP(BT_OGF_VS, 0x0087)
struct bt_hci_cp_acts_vs_set_tws_int_delay_play_time {
	uint16_t handle;
	uint8_t  tws_index;
	uint8_t  time_mode;
	uint32_t per_int;
	uint16_t delay_bt_clk;
	uint16_t delay_bt_slot;
} __packed;

#define BT_HCI_OP_ACTS_VS_SET_TWS_INT_CLOCK_NEW		BT_OP(BT_OGF_VS, 0x0088)
struct bt_hci_cp_acts_vs_set_tws_int_clock_new {
	uint16_t handle;
	uint8_t  tws_index;
	uint8_t  time_mode;
	uint32_t bt_clk;
	uint16_t bt_intraslot;
	uint32_t per_bt_clk;
	uint16_t per_bt_intraslot;
} __packed;
#define BT_TWS_NATIVE_TIMER_MODE 0
#define BT_TWS_US_TIMER_MODE	 1

#define BT_HCI_OP_ACTS_VS_ENABLE_TWS_INT_NEW		BT_OP(BT_OGF_VS, 0x0089)
struct bt_hci_cp_acts_vs_enable_tws_int_new {
	uint8_t enable;
	uint8_t tws_index;
} __packed;
#define BT_TWS_INT_ENABLE 1
#define BT_TWS_INT_DISABLE 0

#define BT_HCI_OP_VS_WRITE_BB_REG				BT_OP(BT_OGF_VS, 0x008a)
struct bt_hci_cp_vs_write_bb_reg {
	uint32_t base_addr;
	uint32_t value;
} __packed;

#define BT_HCI_OP_VS_READ_BB_REG				BT_OP(BT_OGF_VS, 0x008b)
struct bt_hci_cp_vs_read_bb_reg {
	uint32_t base_addr;
	uint8_t  size;
} __packed;

#define BT_HCI_OP_VS_WRITE_RF_REG				BT_OP(BT_OGF_VS, 0x008c)
struct bt_hci_cp_vs_write_rf_reg {
	uint16_t base_addr;
	uint16_t value;
} __packed;

#define BT_HCI_OP_VS_READ_RF_REG				BT_OP(BT_OGF_VS, 0x008d)
struct bt_hci_cp_vs_read_rf_reg {
	uint16_t base_addr;
	uint8_t  size;
} __packed;


#define BT_HCI_OP_ACTS_VS_READ_BT_US_CNT			BT_OP(BT_OGF_VS, 0x008e)
struct bt_hci_rp_acts_vs_read_bt_us_cnt {
	uint8_t  status;
	uint32_t cnt;
} __packed;

/* Events */

#define BT_HCI_EVT_VS_READ_BB_REG_REPORT		0x80
struct bt_hci_rp_vs_read_bb_reg_report {
	uint32_t base_addr;
	uint32_t size;
	uint32_t result[0];
} __packed;

#define BT_HCI_EVT_VS_READ_RF_REG_REPORT		0x81
struct bt_hci_rp_vs_read_rf_reg_report {
	uint16_t base_addr;
	uint16_t size;
	uint16_t result[0];
} __packed;


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HCI_ACTS_VS_H_ */
