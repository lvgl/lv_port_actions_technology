/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice hfp ag
 */
#define SYS_LOG_DOMAIN "btsrv_hfp_ag"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

struct btsrv_hfp_ag_context_info {
	btsrv_hfp_ag_callback hfp_user_callback;
};

extern void btsrv_sco_disconnect(struct bt_conn *sco_conn);

static struct btsrv_hfp_ag_context_info hfp_ag_context;

static void _btsrv_hfp_ag_connect_failed_cb(struct bt_conn *conn)
{
	SYS_LOG_INF("AG connect failed\n");
}

static void _btsrv_hfp_ag_connected_cb(struct bt_conn *conn)
{
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_HFP_AG_CONNECTED, conn);
}

static void _btsrv_hfp_ag_disconnected_cb(struct bt_conn *conn)
{
	/* TODO: Disconnected process order: btsrv_tws(if need)->btsrv_hfp_ag->btsrv_connect */
	btsrv_event_notify(MSG_BTSRV_HFP_AG, MSG_BTSRV_HFP_AG_DISCONNECTED, conn);
}

static int _btsrv_hfp_ag_cmd_cb(struct bt_conn *conn, uint8_t at_type, void *param)
{
#if CONFIG_LOG
	struct at_cmd_param *in_param = param;
#endif
	switch (at_type) {
	case AT_CMD_CHLD:
		SYS_LOG_INF("CHLD %d", in_param->val_u32t);
		break;
	case AT_CMD_CCWA:
		break;
	case AT_CMD_BCC:
		if(conn == btsrv_rdm_hfp_get_actived() || 
			conn == btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER)){
			if (hostif_bt_conn_create_sco(conn)) {
				SYS_LOG_INF("Sco connection failed");
			} else {
				SYS_LOG_INF("Sco connection pending");
			}
		}
		break;
	case AT_CMD_VGS:
		SYS_LOG_INF("VGS %d", in_param->val_u32t);
		break;
	case AT_CMD_VGM:
		SYS_LOG_INF("VGM %d", in_param->val_u32t);
		break;
	case AT_CMD_ATA:
		SYS_LOG_INF("ATA");
		break;
	case AT_CMD_CHUP:
		SYS_LOG_INF("CHUP");
		break;
	case AT_CMD_ATD:
		SYS_LOG_INF("ATD %c, len %d, is_index %d", in_param->s_val_char[0],
					in_param->s_len_u16t, in_param->is_index);
		break;
	case AT_CMD_BLDN:
		SYS_LOG_INF("AT+BLDN");
		break;
	case AT_CMD_NREC:
		SYS_LOG_INF("NREC %d", in_param->val_u32t);
		break;
	case AT_CMD_BVRA:
		SYS_LOG_INF("BVRA %d", in_param->val_u32t);
		break;
	case AT_CMD_BINP:
		//last call
		SYS_LOG_INF("BINP %d", in_param->val_u32t);
		break;
	case AT_CMD_VTS:
		SYS_LOG_INF("VTS %c", in_param->val_char);
		break;
	case AT_CMD_CNUM:
		SYS_LOG_INF("CNUM");
		hostif_bt_hfp_ag_send_event(conn, "+CNUM: ,\"1234567\",129,,4", strlen("+CNUM: ,\"1234567\",129,,4"));
		break;
	case AT_CMD_CLCC:
		SYS_LOG_INF("CLCC");
		hostif_bt_hfp_ag_send_event(conn, "+CLCC: 1,1,0,0,0", strlen("+CLCC: 1,1,0,0,0"));
		break;
	case AT_CMD_BCS:
		SYS_LOG_INF("BCS %d", in_param->val_u32t);
		if(conn == btsrv_rdm_hfp_get_actived() || 
			conn == btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER)){
			if (hostif_bt_conn_create_sco(conn)) {
				SYS_LOG_INF("Sco connection failed");
			} else {
				SYS_LOG_INF("Sco connection pending");
			}
		}
		break;
	case AT_CMD_BTRH:
		SYS_LOG_INF("BTRH %d", in_param->val_u32t);
		break;
	default:
		SYS_LOG_INF("at_type %d\n", at_type);
		break;
	}

	return 0;
}

static const struct bt_hfp_ag_cb btsrv_ag_cb = {
	.connect_failed = _btsrv_hfp_ag_connect_failed_cb,
	.connected = _btsrv_hfp_ag_connected_cb,
	.disconnected = _btsrv_hfp_ag_disconnected_cb,
	.ag_at_cmd = _btsrv_hfp_ag_cmd_cb
};

static int _btsrv_hfp_ag_update_codec_info(void)
{
	uint8_t codec_info[2] = {BT_CODEC_ID_CVSD, 8};
	struct bt_conn *conn = btsrv_rdm_hfp_get_actived();

	btsrv_rdm_hfp_get_codec_info(conn, codec_info, &codec_info[1]);

	if(!conn)
		return -EINVAL;

#ifdef CONFIG_MEDIA	
	if (codec_info[0] == BT_CODEC_ID_CVSD) {
		codec_info[0] = CVSD_TYPE;
	} else if (codec_info[0] == BT_CODEC_ID_MSBC) {
		codec_info[0] = MSBC_TYPE;
	}
#endif

	hfp_ag_context.hfp_user_callback(BTSRV_HFP_CODEC_INFO, codec_info, sizeof(codec_info));

	return 0;
}

int btsrv_hfp_ag_set_status(struct bt_conn *conn, int state)
{
	int old_state = btsrv_rdm_hfp_get_state(conn);
	int new_state = -1;

	// ag only care sco status
	switch (state) {
	case BTSRV_HFP_STATE_SCO_ESTABLISHED:
		if (old_state == BTSRV_HFP_STATE_INIT ||
			old_state == BTSRV_HFP_STATE_LINKED){
			new_state = BTSRV_HFP_STATE_SCO_ESTABLISHED;
			if (conn == btsrv_rdm_hfp_get_actived()) {
				_btsrv_hfp_ag_update_codec_info();
				hfp_ag_context.hfp_user_callback(BTSRV_SCO_CONNECTED, NULL, 0);
			}
		}
		btsrv_rdm_hfp_set_sco_state(conn, BTSRV_SCO_STATE_HFP);
		break;
	case BTSRV_HFP_STATE_SCO_RELEASED:
		if (old_state == BTSRV_HFP_STATE_SCO_ESTABLISHED) {
			new_state = BTSRV_HFP_STATE_LINKED;
			btsrv_rdm_hfp_set_sco_state(conn, BTSRV_SCO_STATE_INIT);
			if (conn == btsrv_rdm_hfp_get_actived())
				hfp_ag_context.hfp_user_callback(BTSRV_SCO_DISCONNECTED, NULL, 0);
		}
		break;
	}

	SYS_LOG_INF("old state %d new state %d state %d\n", old_state, new_state, state);

	if (new_state >= 0) {
		btsrv_rdm_hfp_set_state(conn, new_state);
	}
	return 0;
}

int btsrv_hfp_ag_connect(struct bt_conn *conn)
{
	if (!hostif_bt_hfp_ag_connect(conn)) {
		SYS_LOG_INF("Connect ag hfp_conn:%p\n", conn);
	} else {
		SYS_LOG_ERR("cmd_bredr_hfp_connect failed\n");
	}

	return 0;
}

int btsrv_hfp_ag_init(btsrv_hfp_ag_callback cb)
{
	SYS_LOG_INF("cb %p", cb);
	memset(&hfp_ag_context, 0, sizeof(struct btsrv_hfp_ag_context_info));
	hostif_bt_hfp_ag_register_cb((struct bt_hfp_ag_cb *)&btsrv_ag_cb);
	hfp_ag_context.hfp_user_callback = cb;

	return 0;
}

int btsrv_hfp_ag_deinit(void)
{
	hostif_bt_hfp_ag_register_cb(NULL);
	return 0;
}

static int _btsrv_hfp_ag_connected(struct bt_conn *conn)
{
	bd_address_t *addr = NULL;

	addr = GET_CONN_BT_ADDR(conn);
	SYS_LOG_INF("hfp ag connected:%p addr %p\n", conn, addr);

	hfp_ag_context.hfp_user_callback(BTSRV_HFP_CONNECTED, NULL, 0);
	return 0;
}

static int _btsrv_hfp_ag_disconnected(struct bt_conn *conn)
{
	bd_address_t *addr = NULL;

	addr = GET_CONN_BT_ADDR(conn);
	SYS_LOG_INF("hfp disconnected:%p addr %p\n", conn, addr);

	hfp_ag_context.hfp_user_callback(BTSRV_HFP_DISCONNECTED, NULL, 0);

	return 0;
}

static int _btsrv_hfp_ag_update_indicator(int index, uint8_t value)
{
	return hostif_bt_hfp_ag_update_indicator(index,value);
}

static int _btsrv_hfp_ag_send_event( char *event, uint16_t len)
{
	struct bt_conn* conn = btsrv_rdm_hfp_get_actived();
	if(conn){
		return hostif_bt_hfp_ag_send_event(conn,event,len);
	}
	return 0;
}

int btsrv_hfp_ag_process(struct app_msg *msg)
{
	if (_btsrv_get_msg_param_type(msg) != MSG_BTSRV_HFP_AG) {
		return -ENOTSUP;
	}

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_HFP_AG_START:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_START\n");
		btsrv_hfp_ag_init(msg->ptr);
		break;
	case MSG_BTSRV_HFP_AG_STOP:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_STOP\n");
		btsrv_hfp_ag_deinit();
		break;
	case MSG_BTSRV_HFP_AG_CONNECTED:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_CONNECTED\n");
		_btsrv_hfp_ag_connected(msg->ptr);
		break;
	case MSG_BTSRV_HFP_AG_DISCONNECTED:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_DISCONNECTED\n");
		_btsrv_hfp_ag_disconnected(msg->ptr);
		btsrv_event_notify(MSG_BTSRV_CONNECT, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_HFP_AG_UPDATE_INDICATOR:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_UPDATE_INDICATOR\n");
		_btsrv_hfp_ag_update_indicator((int)msg->ptr,msg->reserve);
		break;
	case MSG_BTSRV_HFP_AG_SEND_EVENT:
		SYS_LOG_INF("MSG_BTSRV_HFP_AG_SEND_EVENT\n");
		_btsrv_hfp_ag_send_event(msg->ptr,strlen(msg->ptr)+1);
		break;
	}
	return 0;
}

