/** @file
 *  @brief Handsfree AG Profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HFP_AG_H
#define __BT_HFP_AG_H

/**
 * @brief Hands Free Profile (HFP AG)
 * @defgroup bt_hfp Hands Free Profile (HFP AG)
 * @ingroup bluetooth
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <acts_bluetooth/rfcomm.h>

enum ag_at_cmd_type {
	AT_CMD_BRSF,
	AT_CMD_BAC,
	AT_CMD_CIND,
	AT_CMD_CMER,
	AT_CMD_CHLD,
	AT_CMD_BIND,
	AT_CMD_COPS,
	AT_CMD_CLIP,
	AT_CMD_CCWA,
	AT_CMD_CMEE,
	AT_CMD_BCC,
	AT_CMD_VGS,
	AT_CMD_VGM,
	AT_CMD_ATA,
	AT_CMD_CHUP,
	AT_CMD_ATD,
	AT_CMD_BLDN,
	AT_CMD_NREC,
	AT_CMD_BVRA,
	AT_CMD_BINP,
	AT_CMD_VTS,
	AT_CMD_CNUM,
	AT_CMD_CLCC,
	AT_CMD_BCS,
	AT_CMD_BTRH,
};

enum ag_ciev_index {		/* enum value must same as enum hfp_hf_ag_indicators  */
	CIEV_SERVICE_IND,
	CIEV_CALL_IND,
	CIEV_CALL_SETUP_IND,
	CIEV_CALL_HELD_IND,
	CIEV_SINGNAL_IND,
	CIEV_ROAM_IND,
	CIEV_BATTERY_IND,
};

struct at_cmd_param {
	union {
		char val_char;
		uint32_t val_u32t;
		struct {
			char *s_val_char;
			uint16_t s_len_u16t;
			uint8_t is_index;
		};
	};
};

struct bt_hfp_ag_cb {
	void (*connect_failed)(struct bt_conn *conn);
	void (*connected)(struct bt_conn *conn);
	void (*disconnected)(struct bt_conn *conn);
	int (*ag_at_cmd)(struct bt_conn *conn, uint8_t at_type, void *param);
};

int bt_hfp_ag_register_cb(struct bt_hfp_ag_cb *cb);
int bt_hfp_ag_update_indicator(uint8_t index, uint8_t value);
int bt_hfp_ag_get_indicator(uint8_t index);
int bt_hfp_ag_send_event(struct bt_conn *conn, char *event, uint16_t len);
int bt_hfp_ag_connect(struct bt_conn *conn);
int bt_hfp_ag_disconnect(struct bt_conn *conn);
void bt_hfp_ag_display_indicator(void);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* __BT_HFP_AG_H */
