/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for BT driver
 */

#ifndef __BT_DRV_H__
#define __BT_DRV_H__

#define HCI_CMD  0x01
#define HCI_ACL  0x02
#define HCI_SCO  0x03
#define HCI_EVT  0x04
#define HCI_ISO  0x05

#define HCI_CMD_HDR_SIZE 3
#define HCI_ACL_HDR_SIZE 4
#define HCI_SCO_HDR_SIZE 3
#define HCI_EVT_HDR_SIZE 2
#define HCI_ISO_HDR_SIZE 4
#define HCI_L2CAP_HEAD_SIZE		4
#define BT_ACL_HDL_FLAG_START	0x02
#ifdef CONFIG_BT_3M
#define L2CAP_BR_MAX_MTU_A2DP_AAC	1000			/* Value same as define in L2cap.h */
#else
#define L2CAP_BR_MAX_MTU_A2DP_AAC	895			/* Value same as define in L2cap.h */
#endif
#ifdef CONFIG_BT_RX_BUF_LEN
#define BT_MAX_RX_ACL_LEN		CONFIG_BT_RX_BUF_LEN
#else
#define BT_MAX_RX_ACL_LEN		0
#endif

#define BT_TWS_0 0
#define BT_TWS_1 1

#define BT_INFO_SHARE_SIZE      (128)

typedef struct btdrv_init_param {
	uint8_t set_hosc_cap:1;
	uint8_t set_max_rf_power:1;
	uint8_t set_ble_rf_power:1;
	uint8_t hosc_capacity;
	uint8_t bt_max_rf_tx_power;
	uint8_t ble_rf_tx_power;
} btdrv_init_param_t;

typedef void (*btdrv_tws_cb_t)(void);

typedef struct btdrv_hci_cb {
    /**
     * @brief get buffer to receive hci data.
     * @param type hci data type.
     * @param evt hci event code.
     * @param exp_len, 0 default buffer lengh, other: expect buffer length.
     * @return buffer to receive hci data.
     */
    uint8_t * (*get_buf)(uint8_t type, uint8_t evt, uint16_t exp_len);
    /**
     * @brief recv hci data callback.
     * @param len length of hci data.
     * @return 0 for success, non-zero otherwise.
     */
    int (*recv)(uint16_t len);
} btdrv_hci_cb_t;

/**
 * @brief initialize bt controller init parameter, call before btdrv_init.
 * @param param Initialize paramter.
 * @return 0 on success, no-zero otherwise.
 */
int btdrv_set_init_param(btdrv_init_param_t *param);

/**
 * @brief initialize bt controller.
 * @param cb callback structure to receive hci data.
 * @return 0 on success, no-zero otherwise.
 */
int btdrv_init(btdrv_hci_cb_t *cb);

/**
 * @brief reset bt controller.
 * @param N/A.
 * @return 0 on success, no-zero otherwise.
 */
int btdrv_reset(void);
/**
 * @brief send hci data to bt controller.
 * @param type hci data type.
 * @param data hci data.
 * @param len length of hci data.
 * @return 0 on success, no-zero otherwise.
 */
int btdrv_send(uint8_t type, uint8_t *data, uint16_t len);

int btdrv_tws_irq_enable(uint8_t index);
int btdrv_tws_irq_disable(uint8_t index);
int btdrv_tws_irq_cb_set(uint8_t index, btdrv_tws_cb_t cb);
int btdrv_tws_set_mode(uint8_t index, uint8_t mode);

void* btdrv_dump_btcpu_info(void);

#endif
