/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice pbap
 */
#define SYS_LOG_DOMAIN "btsrv_map"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

struct btsrv_map_priv {
	struct bt_conn *conn;
	uint8_t app_id;	/* ID from bt manager */
	uint8_t user_id;	/* ID from map client */
	uint8_t folder_flags;
    uint16_t max_list_count;
    uint32_t parameter_mask;    	
	char *path;	
	uint8_t connected:1;
	btsrv_map_callback cb;
};

static struct btsrv_map_priv map_priv[CONFIG_MAX_MAP_CONNECT];

static void *btsrv_map_find_free_priv(void)
{
	uint8_t i;

	for (i = 0; i < CONFIG_MAX_MAP_CONNECT; i++) {
		if (map_priv[i].app_id == 0) {
			return &map_priv[i];
		}
	}

	return NULL;
}

static void btsrv_map_free_priv(struct btsrv_map_priv *info)
{
	memset(info, 0, sizeof(struct btsrv_map_priv));
}

static void *btsrv_map_find_priv_by_app_id(uint8_t app_id)
{
	uint8_t i;

	for (i = 0; i < CONFIG_MAX_MAP_CONNECT; i++) {
		if (map_priv[i].app_id == app_id) {
			return &map_priv[i];
		}
	}

	return NULL;
}

static void *btsrv_map_find_priv_by_user_id(struct bt_conn *conn, uint8_t user_id)
{
	uint8_t i;

	for (i = 0; i < CONFIG_MAX_MAP_CONNECT; i++) {
		if (map_priv[i].user_id == user_id && map_priv[i].conn == conn) {
			return &map_priv[i];
		}
	}

	return NULL;
}

static void btsrv_map_connect_failed_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_MAP, MSG_BTSRV_MAP_CONNECT_FAILED, conn, user_id);
}

static void btsrv_map_connected_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_MAP, MSG_BTSRV_MAP_CONNECTED, conn, user_id);
}

void btsrv_map_disconnected_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_MAP, MSG_BTSRV_MAP_DISCONNECTED, conn, user_id);
}

static void btsrv_map_set_path_finised_cb(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_user_id(conn, user_id);

	if (!info || !info->cb) {
		return;
	}

	info->cb(BTSRV_MAP_SET_PATH_FINISHED, info->app_id, NULL, 0);
}

void btsrv_map_recv_cb(struct bt_conn *conn, uint8_t user_id, struct parse_messages_result *result, uint8_t array_size)
{
	/* Callback by hci_rx thread, in negative priority,
	 * is better send message to bt service, process by btservice??
	 */
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	info->cb(BTSRV_MAP_MESSAGES_RESULT, info->app_id, result, array_size);
}

static const struct bt_map_client_user_cb map_client_cb = {
	.connect_failed = btsrv_map_connect_failed_cb,
	.connected = btsrv_map_connected_cb,
	.disconnected = btsrv_map_disconnected_cb,
	.set_path_finished = btsrv_map_set_path_finised_cb,
	.recv = btsrv_map_recv_cb,
};

static void btsrv_map_connect_server(struct bt_map_connect_param *param)
{
	struct btsrv_map_priv *info = btsrv_map_find_free_priv();
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&param->bd);

	if (!info || !conn) {
		param->cb(BTSRV_MAP_CONNECT_FAILED, param->app_id, NULL, 0);
		return;
	}

	info->conn = conn;
	info->app_id = param->app_id;
	info->path = param->map_path;
	info->cb = param->cb;
	info->user_id = hostif_bt_map_client_connect(info->conn,info->path,
									(struct bt_map_client_user_cb *)&map_client_cb);
	if (!info->user_id) {
		info->cb(BTSRV_MAP_CONNECT_FAILED, param->app_id, NULL, 0);
		btsrv_map_free_priv(info);
	}
}

static void btsrv_map_get_message(struct bt_map_get_param *param)
{
	struct btsrv_map_priv *info = btsrv_map_find_free_priv();
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&param->bd);

	if (!info || !conn) {
		param->cb(BTSRV_MAP_CONNECT_FAILED, param->app_id, NULL, 0);
		return;
	}

	info->conn = conn;
	info->app_id = param->app_id;
	info->path = param->map_path;
	info->cb = param->cb;
	info->user_id = hostif_bt_map_client_get_message(info->conn, info->path,
									(struct bt_map_client_user_cb *)&map_client_cb);
	if (!info->user_id) {
		info->cb(BTSRV_MAP_CONNECT_FAILED, param->app_id, NULL, 0);
		btsrv_map_free_priv(info);
	}
}

static void btsrv_map_abort_get(uint8_t app_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_app_id(app_id);

	if (!info) {
		return;
	}

	hostif_bt_map_client_abort_get(info->conn, info->user_id);
}

static void btsrv_map_client_disconnect(uint8_t app_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_app_id(app_id);

	if (!info) {
		return;
	}

	hostif_bt_map_client_disconnect(info->conn, info->user_id);
}

static void btsrv_map_client_set_folder(struct bt_map_set_folder_param *param)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}
    
	if (!info->connected) {
		return;
	}

	info->path = param->map_path;
	info->folder_flags = param->flags;
	
	hostif_bt_map_client_set_folder(info->conn,info->user_id,info->path,info->folder_flags);

}

static void btsrv_map_client_get_folder_listing(uint8_t app_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_app_id(app_id);

	if (!info) {
		return;
	}
    
	if (!info->connected) {
		return;
	}
		
	hostif_bt_map_client_get_folder_listing(info->conn,info->user_id);

}

static void btsrv_map_client_get_messages_listing(struct bt_map_get_messages_listing_param * param)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}
    
	if (!info->connected) {
		return;
	}
	
	info->max_list_count = param->max_list_count;
	info->parameter_mask = param->parameter_mask;
	
	hostif_bt_map_client_get_messages_listing(info->conn,info->user_id,info->max_list_count,info->parameter_mask);

}

static void btsrv_map_connect_failed(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	info->cb(BTSRV_MAP_CONNECT_FAILED, info->app_id, NULL, 0);
	btsrv_map_free_priv(info);
}

static void btsrv_map_connected(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	if (info->connected) {
		SYS_LOG_INF("Already connected\n");
		return;
	}

	info->connected = 1;
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_MAP_CONNECTED, conn);
	info->cb(BTSRV_MAP_CONNECTED, info->app_id, NULL, 0);
}

static void btsrv_map_disconnected(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_map_priv *info = btsrv_map_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	info->connected = 0;
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_MAP_DISCONNECTED, conn);
	info->cb(BTSRV_MAP_DISCONNECTED, info->app_id, NULL, 0);
	btsrv_map_free_priv(info);
}

int btsrv_map_process(struct app_msg *msg)
{   
	switch (_btsrv_get_msg_param_cmd(msg)) {

	case MSG_BTSRV_MAP_CONNECT:
		btsrv_map_connect_server(msg->ptr);
	    break;
	case MSG_BTSRV_MAP_SET_FOLDER:
	    btsrv_map_client_set_folder(msg->ptr);	    
	    break;	    
	case MSG_BTSRV_MAP_GET_FOLDERLISTING:
	    btsrv_map_client_get_folder_listing((uint8_t)_btsrv_get_msg_param_value(msg));	    
	    break;	    
	case MSG_BTSRV_MAP_GET_MESSAGESLISTING:
	    btsrv_map_client_get_messages_listing(msg->ptr);	    
	    break;
	case MSG_BTSRV_MAP_GET_MESSAGE:
		btsrv_map_get_message(msg->ptr);
		break;
	case MSG_BTSRV_MAP_ABORT_GET:
		btsrv_map_abort_get((uint8_t)_btsrv_get_msg_param_value(msg));
		break;
	case MSG_BTSRV_MAP_DISCONNECT:
		btsrv_map_client_disconnect((uint8_t)_btsrv_get_msg_param_value(msg));
		break;		
	case MSG_BTSRV_MAP_CONNECT_FAILED:
		btsrv_map_connect_failed(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_MAP_CONNECTED:
		btsrv_map_connected(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_MAP_DISCONNECTED:
		btsrv_map_disconnected(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	default:
		break;
	}

	return 0;
}
