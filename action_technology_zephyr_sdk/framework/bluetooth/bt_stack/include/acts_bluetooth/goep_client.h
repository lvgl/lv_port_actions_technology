/** @file
 *  @brief Bluetooth GOEP client profile
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_GOEP_CLIENT_H
#define __BT_GOEP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
	GOEP_SEND_HEADER_CONNECTING_ID	= (0x1 << 0),
	GOEP_SEND_HEADER_NAME 			= (0x1 << 1),
	GOEP_SEND_HEADER_TYPE 			= (0x1 << 2),
	GOEP_SEND_HEADER_APP_PARAM 		= (0x1 << 3),
	GOEP_SEND_HEADER_FLAGS 		    = (0x1 << 4),
};

enum {
	GOEP_RX_HEADER_APP_PARAM		= (0x1 << 0),
	GOEP_RX_HEADER_BODY				= (0x1 << 1),
	GOEP_RX_HEADER_END_OF_BODY		= (0x1 << 2),
	GOEP_RX_CONTINUE_WITHOUT_BODY	= (0x1 << 3),
};

struct bt_goep_header_param {
	uint16_t header_bit;
	uint16_t param_len;
	void *param;
};

struct bt_goep_client_cb {
	void (*connected)(struct bt_conn *conn, uint8_t goep_id);
	void (*aborted)(struct bt_conn *conn, uint8_t goep_id);
	void (*disconnected)(struct bt_conn *conn, uint8_t goep_id);
	void (*recv)(struct bt_conn *conn, uint8_t goep_id, uint16_t header_bit, uint8_t *data, uint16_t len);
	void (*setpath)(struct bt_conn *conn, uint8_t goep_id);
};

uint16_t bt_goep_client_connect(struct bt_conn *conn, uint8_t *target_uuid, uint8_t uuid_len, uint8_t server_channel,
									uint16_t connect_psm, struct bt_goep_client_cb *cb);
int bt_goep_client_get(struct bt_conn *conn, uint8_t goep_id, uint16_t headers,
					struct bt_goep_header_param *param, uint8_t array_size);
int bt_goep_client_disconnect(struct bt_conn *conn, uint8_t goep_id, uint16_t headers);
int bt_goep_client_aboot(struct bt_conn *conn, uint8_t goep_id, uint16_t headers);
int bt_goep_client_setpath(struct bt_conn *conn, uint8_t goep_id, uint16_t headers,
					struct bt_goep_header_param *param, uint8_t array_size);
#ifdef __cplusplus
}
#endif

#endif /* __BT_GOEP_CLIENT_H */
