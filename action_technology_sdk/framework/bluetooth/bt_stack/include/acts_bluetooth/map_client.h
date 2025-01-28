/** @file
 *  @brief Bluetooth PBAP client profile
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_MAP_CLIENT_H
#define __BT_MAP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acts_bluetooth/parse_vcard.h>

enum {
	MAP_PARAM_ID_MAX_LIST_COUNT = 0x01,
	MAP_PARAM_ID_FILTER_MESSAGE_TYPE = 0x03,
	MAP_PARAM_ID_PARAMETER_MASK = 0x10,
	MAP_PARAM_ID_MESSAGES_LISTING_SIZE  = 0x12,	
	MAP_PARAM_ID_ATTACHMENT = 0x0A,
	MAP_PARAM_ID_NEW_MESSAGE = 0x0D,
	MAP_PARAM_ID_MSE_TIME = 0x19,
};

enum {
	MAP_PARAMETER_MASK_SUBJECT   	            = (0x1 << 0),
	MAP_PARAMETER_MASK_DATETIME 			        = (0x1 << 1),
	MAP_PARAMETER_MASK_SENDER_NAME 			    = (0x1 << 2),
	MAP_PARAMETER_MASK_SENDER_ADDRESS 		    = (0x1 << 3),
	MAP_PARAMETER_MASK_RECIPIENT_NAME 		    = (0x1 << 4),
	MAP_PARAMETER_MASK_RECIPIENT_ADDRESSING   = (0x1 << 5),
	MAP_PARAMETER_MASK_TYPE   	                = (0x1 << 6),
	MAP_PARAMETER_MASK_SIZE 			             = (0x1 << 7),
	MAP_PARAMETER_MASK_RECEPTION_STATUS         = (0x1 << 8),
	MAP_PARAMETER_MASK_TEXT 		                 = (0x1 << 9),
	MAP_PARAMETER_MASK_ATTACHMENT_SIZE 		     = (0x1 << 10),
	MAP_PARAMETER_MASK_PRIORITY                  = (0x1 << 11),
	MAP_PARAMETER_MASK_READ                      = (0x1 << 12),
	MAP_PARAMETER_MASK_SEND 		                 = (0x1 << 13),
	MAP_PARAMETER_MASK_PROTECTED 		         = (0x1 << 14),
	MAP_PARAMETER_MASK_REPLYTO_ADDRESSING       = (0x1 << 15),
};

enum {
	MAP_MESSAGE_RESULT_TYPE_DATETIME = 0x01,
	MAP_MESSAGE_RESULT_TYPE_BODY = 0x02,
};

#define MAP_GET_VALUE_16(addr)		sys_be16_to_cpu(UNALIGNED_GET((uint16_t *)(addr)))
#define MAP_PUT_VALUE_16(value, addr)	UNALIGNED_PUT(sys_cpu_to_be16(value), (uint16_t *)addr)
#define MAP_GET_VALUE_32(addr)		sys_be32_to_cpu(UNALIGNED_GET((uint32_t *)(addr)))
#define MAP_PUT_VALUE_32(value, addr)	UNALIGNED_PUT(sys_cpu_to_be32(value), (uint32_t *)addr)

struct parse_messages_result {
	uint8_t type;
	uint16_t len;
	uint8_t *data;
};

struct bt_map_client_user_cb {
	void (*connect_failed)(struct bt_conn *conn, uint8_t user_id);
	void (*connected)(struct bt_conn *conn, uint8_t user_id);
	void (*disconnected)(struct bt_conn *conn, uint8_t user_id);
	void (*set_path_finished)(struct bt_conn *conn, uint8_t user_id);
	void (*recv)(struct bt_conn *conn, uint8_t user_id, struct parse_messages_result *result, uint8_t array_size);
};

struct map_common_param {
	uint8_t id;
	uint8_t len;
	uint8_t data[0];
} __packed;

struct map_get_messages_listing_param {
	uint16_t max_list_count;
	uint8_t filter_message_type;
	uint8_t reserved;
	uint32_t parametr_mask;
};

uint8_t bt_map_client_get_message(struct bt_conn *conn, char *path, struct bt_map_client_user_cb *cb);
int bt_map_client_abort_get(struct bt_conn *conn, uint8_t user_id);
uint8_t bt_map_client_connect(struct bt_conn *conn,char *path,struct bt_map_client_user_cb *cb);
int bt_map_client_disconnect(struct bt_conn *conn, uint8_t user_id);
int bt_map_client_get_messages_listing(struct bt_conn *conn, uint8_t user_id ,uint16_t max_list_count,uint32_t parameter_mask);
int bt_map_client_get_folder_listing(struct bt_conn *conn, uint8_t user_id);
int bt_map_client_set_folder(struct bt_conn *conn, uint8_t user_id,char *path,uint8_t flags);
int bt_pts_map_client_disconnect(struct bt_conn *conn, uint8_t user_id);
int bt_pts_map_client_get_final(struct bt_conn *conn, uint8_t user_id);

#ifdef __cplusplus
}
#endif
#endif /* __BT_MAP_CLIENT_H */
