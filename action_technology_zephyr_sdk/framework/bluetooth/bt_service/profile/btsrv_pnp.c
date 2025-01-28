/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice
 */
#define SYS_LOG_DOMAIN "btsrv_pnp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"
#include <acts_bluetooth/sdp.h>

struct btsrv_pnp_info_context_info {
	btsrv_pnp_info_callback pnp_user_callback;
};

static struct btsrv_pnp_info_context_info pnp_info_context;

static int _btsrv_pnp_info_get_cb(struct bt_conn *conn, uint16_t type, void *param)
{
	SYS_LOG_INF("type 0x%x.\n",type);

	btsrv_pnp_info_event_e event = BTSRV_PNP_VENDOR_ID;
	int param_size = 0;

	if (BT_SDP_ATTR_VENDOR_ID == type) {
		param_size = sizeof(uint16_t);
		event = BTSRV_PNP_VENDOR_ID;
	} else if (BT_SDP_ATTR_PRODUCT_ID == type) {
		param_size = sizeof(uint16_t);
		event = BTSRV_PNP_PRODUCT_ID;
	}

	if (pnp_info_context.pnp_user_callback) {
		pnp_info_context.pnp_user_callback(event, param, param_size);
	}

	return 0;
}

static const struct bt_pnp_info_cb btsrv_pnp_cb = {
	.info_get = _btsrv_pnp_info_get_cb
};

int btsrv_pnp_info_search(struct bt_conn *conn)
{
	if (!hostif_bt_pnp_info_search(conn)) {
		SYS_LOG_DBG("btsrv_pnp_info_search conn:%p\n", conn);
	} else {
		SYS_LOG_ERR("btsrv_pnp_info_search failed\n");
	}

	return 0;
}

int btsrv_pnp_info_search_init(btsrv_pnp_info_callback cb)
{
	SYS_LOG_INF("cb %p", cb);
	memset(&pnp_info_context, 0, sizeof(struct btsrv_pnp_info_context_info));
	hostif_bt_pnp_info_register_cb((struct bt_pnp_info_cb *)&btsrv_pnp_cb);
	pnp_info_context.pnp_user_callback = cb;

	return 0;
}

int btsrv_pnp_info_search_deinit(void)
{
	hostif_bt_pnp_info_register_cb(NULL);
	pnp_info_context.pnp_user_callback = NULL;
	return 0;
}

int btsrv_pnp_info_process(struct app_msg *msg)
{
	if (_btsrv_get_msg_param_type(msg) != MSG_BTSRV_PNP) {
		return -ENOTSUP;
	}

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_PNP_INFO_START:
		SYS_LOG_INF("MSG_BTSRV_PNP_INFO_START\n");
		btsrv_pnp_info_search_init(msg->ptr);
		break;
	case MSG_BTSRV_PNP_INFO_STOP:
		SYS_LOG_INF("MSG_BTSRV_PNP_INFO_STOP\n");
		btsrv_pnp_info_search_deinit();
		break;
	}
	return 0;
}

