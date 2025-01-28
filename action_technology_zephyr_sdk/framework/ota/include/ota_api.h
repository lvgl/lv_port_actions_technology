/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA backend interface
 */

#ifndef __OTA_API_H__
#define __OTA_API_H__

/* support check data checksum in each ota unit */
#define CONFIG_OTA_BT_SUPPORT_UNIT_CRC

#define TLV_TYPE_OTA_SUPPORT_FEATURES		0x09
#define OTA_SUPPORT_FEATURE_UNIT_DATA_CRC	(1 << 0)

#define OTA_ERROR_CODE_SUCCESS		100000

#define SERVICE_ID_OTA		0x9

#define OTA_SVC_SEND_BUFFER_SIZE	0x80
#define OTA_UNIT_SIZE			0x100

#define OTA_SVC_TRANS_SEND_BUFFER_SIZE	1024

#define TLV_MAX_DATA_LENGTH	0x3fff
#define TLV_TYPE_ERROR_CODE	0x7f
#define TLV_TYPE_MAIN		0x80

#define OTA_CMD_H2D_REQUEST_UPGRADE		0x01
#define OTA_CMD_H2D_CONNECT_NEGOTIATION		0x02
#define OTA_CMD_D2H_REQUIRE_IMAGE_DATA		0x03
#define OTA_CMD_H2D_SEND_IMAGE_DATA		0x04
#define OTA_CMD_D2H_REPORT_RECEVIED_DATA_COUNT	0x05
#define OTA_CMD_D2H_VALIDATE_IMAGE		0x06
#define OTA_CMD_D2H_REPORT_STATUS		0x07
#define OTA_CMD_D2H_CANCEL_UPGRADE		0x08
#define OTA_CMD_H2D_NEGOTIATION_RESULT		0x09
#define OTA_CMD_D2H_REQUEST_UPGRADE		0x0A
#define OTA_CMD_H2D_SEND_IMAGE_DATA_WITH_CRC	0x0B

#define TLV_TYPE_OTA_WAIT_TIMEOUT           0x01
#define TLV_TYPE_OTA_RESTART_TIMEOUT        0x02
#define TLV_TYPE_OTA_UINT_SIZE               0x03
#define TLV_TYPE_OTA_INTERVAL                0x04

#define TLV_TYPE_OTA_FILE_OFFSET           0x01
#define TLV_TYPE_OTA_FILE_LEN               0x02
#define TLV_TYPE_OTA_APPLY_BITMAP          0x03

#define TLV_TYPE_OTA_VALID_REPORT           0x01

#define TLV_TYPE_OTA_STATUS_CODE           0x7F

enum cli_prot_state{
	PROT_STATE_IDLE,
	PROT_STATE_DATA,
};

struct cli_prot_context {
	u8_t state;
	int send_buf_size;
	u8_t *send_buf;

	u8_t *read_buf;
	int read_len;
	int read_done_len;
	u8_t last_psn;
	u8_t host_features;
	u16_t app_wait_timeout;
	u16_t device_restart_timeout;
	u16_t ota_unit_size;	
	u16_t ota_interval;
	u8_t ota_ack_enable;
	

	io_stream_t sppble_stream;
	int sppble_stream_opened;
	int negotiation_done;
};

#endif /* __OTA_API_H__ */
