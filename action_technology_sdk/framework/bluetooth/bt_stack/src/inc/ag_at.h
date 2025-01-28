/** @file ag_at.h
 *  @brief Internal APIs for AG AT command handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_AG_AT_H
#define __BT_AG_AT_H

#ifdef __cplusplus
	extern "C" {
#endif

#define HFP_AG_MAX_AT_PARAM			5

enum ag_at_cme {
	AG_CME_ERROR_AG_FAILURE                    = 0,
	AG_CME_ERROR_NO_CONNECTION_TO_PHONE        = 1,
	AG_CME_ERROR_OPERATION_NOT_ALLOWED         = 3,
	AG_CME_ERROR_OPERATION_NOT_SUPPORTED       = 4,
	AG_CME_ERROR_PH_SIM_PIN_REQUIRED           = 5,
	AG_CME_ERROR_SIM_NOT_INSERTED              = 10,
	AG_CME_ERROR_SIM_PIN_REQUIRED              = 11,
	AG_CME_ERROR_SIM_PUK_REQUIRED              = 12,
	AG_CME_ERROR_SIM_FAILURE                   = 13,
	AG_CME_ERROR_SIM_BUSY                      = 14,
	AG_CME_ERROR_INCORRECT_PASSWORD            = 16,
	AG_CME_ERROR_SIM_PIN2_REQUIRED             = 17,
	AG_CME_ERROR_SIM_PUK2_REQUIRED             = 18,
	AG_CME_ERROR_MEMORY_FULL                   = 20,
	AG_CME_ERROR_INVALID_INDEX                 = 21,
	AG_CME_ERROR_MEMORY_FAILURE                = 23,
	AG_CME_ERROR_TEXT_STRING_TOO_LONG          = 24,
	AG_CME_ERROR_INVALID_CHARS_IN_TEXT_STRING  = 25,
	AG_CME_ERROR_DIAL_STRING_TO_LONG           = 26,
	AG_CME_ERROR_INVALID_CHARS_IN_DIAL_STRING  = 27,
	AG_CME_ERROR_NO_NETWORK_SERVICE            = 30,
	AG_CME_ERROR_NETWORK_TIMEOUT               = 31,
	AG_CME_ERROR_NETWORK_NOT_ALLOWED           = 32,
	AG_CME_ERROR_UNKNOWN                       = 33,
};

enum AG_CODEC_BIT {
	BT_CODEC_ID_CVSD_BIT = (0x01 << 1),
	BT_CODEC_ID_MSBC_BIT = (0x01 << 2),
};

enum HFP_AG_AT_CMD_REQ_TYPE {
	AT_CMD_REQ_UNKNOW,
	AT_CMD_REQ_DIRECT,			/* AT comand(ATA, ATD10086) */
	AT_CMD_REQ_SET,				/* AT comand=value (AT+BRSF=59) */
	AT_CMD_REQ_TEST,			/* AT comand=? (AT+CIND=?) */
	AT_CMD_REQ_READ,			/* AT comand? (AT+CIND?) */
};

struct ag_at_parse_result;
typedef void (*ag_at_parse_cb)(struct ag_at_parse_result *result, void *param);

struct ag_at_parse_result {
	uint8_t at_cmd_type;
	uint8_t cmd_req_type;
	uint8_t cmd_len;
	uint8_t cmd_param_len;
	uint8_t *cmd;
	uint8_t *cmd_param;
	ag_at_parse_cb cb;
	void *cb_param;
};

uint16_t bt_ag_at_parse(struct ag_at_parse_result *result, uint8_t *data, uint16_t len);
int bt_ag_at_get_number(struct ag_at_parse_result *result, uint32_t *val);
int bt_ag_at_get_char(struct ag_at_parse_result *result, char *val);
int bt_ag_at_phone_number(struct ag_at_parse_result *result, char **pPhoneNum, uint16_t *len, uint8_t *is_index);

#ifdef __cplusplus
}
#endif

#endif /* __BT_AG_AT_H */
