/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice pbap
 */

#define SYS_LOG_DOMAIN "btsrv_pbap"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

struct btsrv_pbap_priv {
	struct bt_conn *conn;
	uint8_t app_id;	/* ID from bt manager */
	uint8_t user_id;	/* ID from pbap client */
	char *path;
	uint8_t connected:1;
	btsrv_pbap_callback cb;
};

static struct btsrv_pbap_priv pbap_priv[CONFIG_MAX_PBAP_CONNECT];

static void *btsrv_pbap_find_free_priv(void)
{
	int i;

	for (i = 0; i < CONFIG_MAX_PBAP_CONNECT; i++) {
		if (pbap_priv[i].app_id == 0) {
			return &pbap_priv[i];
		}
	}

	return NULL;
}

static void btsrv_pbap_free_priv(struct btsrv_pbap_priv *info)
{
	memset(info, 0, sizeof(struct btsrv_pbap_priv));
}

static void *btsrv_pbap_find_priv_by_app_id(uint8_t app_id)
{
	int i;

	for (i = 0; i < CONFIG_MAX_PBAP_CONNECT; i++) {
		if (pbap_priv[i].app_id == app_id) {
			return &pbap_priv[i];
		}
	}

	return NULL;
}

static void *btsrv_pbap_find_priv_by_user_id(struct bt_conn *conn, uint8_t user_id)
{
	int i;

	for (i = 0; i < CONFIG_MAX_PBAP_CONNECT; i++) {
		if (pbap_priv[i].user_id == user_id && pbap_priv[i].conn == conn) {
			return &pbap_priv[i];
		}
	}

	return NULL;
}

static void btsrv_pbap_connect_failed_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_PBAP, MSG_BTSRV_PBAP_CONNECT_FAILED, conn, user_id);
}

static void btsrv_pbap_connected_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_PBAP, MSG_BTSRV_PBAP_CONNECTED, conn, user_id);
}

void btsrv_pbap_disconnected_cb(struct bt_conn *conn, uint8_t user_id)
{
	btsrv_event_notify_ext(MSG_BTSRV_PBAP, MSG_BTSRV_PBAP_DISCONNECTED, conn, user_id);
}

void btsrv_pbap_recv_cb(struct bt_conn *conn, uint8_t user_id, struct stack_pbap_cb_result *result)
{
	/* Callback by hci_rx thread, in negative priority,
	 * is better send message to bt service, process by btservice??
	 */
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_user_id(conn, user_id);

	if (!info || !result) {
		return;
	}

	switch (result->cb_event) {
	case STACK_PBAP_CB_EVENT_MAX_SIZE:
		info->cb(BTSRV_PBAP_MAX_SIZE, info->app_id, &result->max_size, sizeof(result->max_size));
		break;
	case STACK_PBAP_CB_EVENT_VCARD:
		info->cb(BTSRV_PBAP_VCARD_RESULT, info->app_id, result->vcard_result, result->array_size);
		break;
	case STACK_PBAP_CB_EVENT_SETPATH_FINISH:
		info->cb(BTSRV_PBAP_SETPATH_FINISH, info->app_id, NULL, 0);
		break;
	case STACK_PBAP_CB_EVENT_SEARCH_RESULT:
		info->cb(BTSRV_PBAP_SEARCH_RESULT, info->app_id, result->vcard_result, result->array_size);
		break;
	case STACK_PBAP_CB_EVENT_GET_VCARD_FINISH:
		info->cb(BTSRV_PBAP_GET_VCARD_FINISH, info->app_id, NULL, 0);
		break;
	case STACK_PBAP_CB_EVENT_END_OF_BODY:
		info->cb(BTSRV_PBAP_END_OF_BODY, info->app_id, NULL, 0);
		break;
	case STACK_PBAP_CB_EVENT_ABORT_FINISH:
		info->cb(BTSRV_PBAP_ABORT_FINISH, info->app_id, NULL, 0);
		break;
	default:
		break;
	}
}

static const struct bt_pbap_client_user_cb pbap_client_cb = {
	.connect_failed = btsrv_pbap_connect_failed_cb,
	.connected = btsrv_pbap_connected_cb,
	.disconnected = btsrv_pbap_disconnected_cb,
	.recv = btsrv_pbap_recv_cb,
};

static void btsrv_pbap_connect_failed(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	info->cb(BTSRV_PBAP_CONNECT_FAILED, info->app_id, NULL, 0);
	btsrv_pbap_free_priv(info);
}

static void btsrv_pbap_connected(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	if (info->connected) {
		SYS_LOG_INF("Already connected\n");
		return;
	}

	info->connected = 1;
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_PBAP_CONNECTED, conn);
	info->cb(BTSRV_PBAP_CONNECTED, info->app_id, NULL, 0);
}

static void btsrv_pbap_disconnected(struct bt_conn *conn, uint8_t user_id)
{
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_user_id(conn, user_id);

	if (!info) {
		return;
	}

	if (!info->connected) {
		SYS_LOG_INF("Connect failed\n");
	}

	info->connected = 0;
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_PBAP_DISCONNECTED, conn);
	info->cb(BTSRV_PBAP_DISCONNECTED, info->app_id, NULL, 0);
	btsrv_pbap_free_priv(info);
}

static void btsrv_pbap_op_connect(struct bt_pbap_op_param *param)
{
	struct btsrv_pbap_priv *info = btsrv_pbap_find_free_priv();
	struct bt_conn *conn = btsrv_rdm_find_conn_by_addr(&param->bd);

	if (!info || !conn) {
		param->cb(BTSRV_PBAP_CONNECT_FAILED, param->app_id, NULL, 0);
		return;
	}

	info->conn = conn;
	info->app_id = param->app_id;
	info->cb = param->cb;
	info->user_id = hostif_bt_pbap_client_connect(info->conn, (struct bt_pbap_client_user_cb *)&pbap_client_cb);
	if (!info->user_id) {
		info->cb(BTSRV_PBAP_CONNECT_FAILED, param->app_id, NULL, 0);
		btsrv_pbap_free_priv(info);
	}
}

static void btsrv_pbap_op_disconnect(struct bt_pbap_op_param *param)
{
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	hostif_bt_pbap_client_disconnect(info->conn, info->user_id);
}

static void btsrv_pbap_op_get_size(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_GET_SIZE;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	stack_param.path = param->pb_path;

	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_get_pb(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_GET_PB;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	stack_param.path = param->pb_path;
	stack_param.filter = param->pb_filter;

	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_set_path(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_SET_PATH;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	stack_param.path = param->pb_path;

	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_get_vcard(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_GET_VCARD;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	stack_param.vcard_name_len = param->vcard_name_len;
	stack_param.filter = param->vc_filter;
	if (stack_param.vcard_name_len) {
		stack_param.vcard_name = param->vcard_name;
	} else {
		stack_param.vcard_name = NULL;
	}
	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_get_continue(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_GET_CONTINUE;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_listing(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_LISTING;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	stack_param.order = param->order;
	stack_param.search_attr = param->search_attr;
	stack_param.search_len = param->search_len;
	if (stack_param.search_len) {
		stack_param.search_value = param->search_value;
	} else {
		stack_param.search_value = NULL;
	}
	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_op_abort(struct bt_pbap_op_param *param)
{
	struct stack_pbap_op_param stack_param;
	struct btsrv_pbap_priv *info = btsrv_pbap_find_priv_by_app_id(param->app_id);

	if (!info) {
		return;
	}

	stack_param.op_cmd = STACK_PBAP_OP_CMD_ABORT;
	stack_param.conn = info->conn;
	stack_param.user_id = info->user_id;
	hostif_bt_pbap_client_op(&stack_param);
}

static void btsrv_pbap_cmd_op(struct bt_pbap_op_param *param)
{
	SYS_LOG_INF("PBAP op %d", param->cmd);

	switch (param->cmd) {
	case BTSRV_PBAP_OP_CMD_CONNECT:
		btsrv_pbap_op_connect(param);
		break;
	case BTSRV_PBAP_OP_CMD_DISCONNECT:
		btsrv_pbap_op_disconnect(param);
		break;
	case BTSRV_PBAP_OP_CMD_GET_SIZE:
		btsrv_pbap_op_get_size(param);
		break;
	case BTSRV_PBAP_OP_CMD_GET_PB:
		btsrv_pbap_op_get_pb(param);
		break;
	case BTSRV_PBAP_OP_CMD_SET_PATH:
		btsrv_pbap_op_set_path(param);
		break;
	case BTSRV_PBAP_OP_CMD_GET_VCARD:
		btsrv_pbap_op_get_vcard(param);
		break;
	case BTSRV_PBAP_OP_CMD_GET_CONTINUE:
		btsrv_pbap_op_get_continue(param);
		break;
	case BTSRV_PBAP_OP_CMD_LISTING:
		btsrv_pbap_op_listing(param);
		break;
	case BTSRV_PBAP_OP_CMD_ABORT:
		btsrv_pbap_op_abort(param);
		break;
	default:
		break;
	}
}

int btsrv_pbap_process(struct app_msg *msg)
{
	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_PBAP_CONNECT_FAILED:
		btsrv_pbap_connect_failed(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_PBAP_CONNECTED:
		btsrv_pbap_connected(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_PBAP_DISCONNECTED:
		btsrv_pbap_disconnected(_btsrv_get_msg_param_ptr(msg), _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_PBAP_CMD_OP:
		btsrv_pbap_cmd_op(msg->ptr);
		break;
	default:
		break;
	}

	return 0;
}
