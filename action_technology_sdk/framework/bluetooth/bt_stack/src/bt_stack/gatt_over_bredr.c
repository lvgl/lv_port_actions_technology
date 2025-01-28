/**
 * @file gatt_over_bredr.c
 * Generic GATT over BR/EDR
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if 0
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <net/buf.h>
#include <bluetooth/hci.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
//#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_GATT)
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"

#include "att_internal.h"
#include "bt_internal_variable.h"
#endif
#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/l2cap.h>
#include <drivers/bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ATT)
#define LOG_MODULE_NAME bt_gobr
#include "common/log.h"

#include "hci_core.h"
#include "conn_internal.h"
#include "l2cap_internal.h"
#include "att_internal.h"
#include "gatt_internal.h"

// #undef BT_DBG
// #define BT_DBG BT_INFO

#define BT_L2CAP_PSM_GOBR 0x001F
#define ATT_BR_CHAN(_ch) CONTAINER_OF(_ch, struct bt_att_chan, br_chan.chan)

static bt_gobr_connected_cb_t connect_cb_func;
static bt_gatt_connect_status_cb_t connect_status_cb;
struct bt_att *bredr_att;

static void bt_gobr_l2cap_connected(struct bt_l2cap_chan *chan)
{
	BT_INFO("Connected");

	if (!connect_status_cb) {
		BT_ERR("connect_status_cb is null.");
		return;
	}

	if (true == connect_status_cb()) {
		BT_ERR("gatt exist, chan %p", chan);
		return;
	}

	bt_gatt_over_bredr_connected(chan);
	if (connect_cb_func) {
		connect_cb_func(chan->conn, true);
	}
}

static void bt_gobr_l2cap_disconnected(struct bt_l2cap_chan *chan)
{
	BT_INFO("Disconnected");
	//struct bt_att_chan *att_chan = ATT_BR_CHAN(chan);

	/* Remove request from the list */
	//if (bredr_att)
	//	sys_slist_find_and_remove(&bredr_att->chans, &att_chan->node);
	if (!connect_status_cb) {
		BT_ERR("connect_status_cb is null.");
		return;
	}

	if (false == connect_status_cb()) {
		BT_ERR("gatt released, chan %p", chan);
		return;
	}

	bt_gatt_over_bredr_disconnected(chan);
	if (connect_cb_func) {
		connect_cb_func(chan->conn, false);
	}

	bredr_att = NULL;
}

static int bt_gobr_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	bt_gatt_over_bredr_recv(chan, buf);

	return 0;
}

static void bt_gobr_l2cap_released(struct bt_l2cap_chan *chan)
{

	//struct bt_att_chan *att_chan = ATT_CHAN(chan);
	/* Remove request from the list */
	//if (bredr_att)
	//	sys_slist_find_and_remove(&bredr_att->chans, &chan->node);
	BT_INFO("released.");
	bt_gatt_over_bredr_released(chan);

	return;
}

#if defined(CONFIG_BT_EATT)
#define ATT_CHAN_MAX				(CONFIG_BT_EATT_MAX + 1)
#else
#define ATT_CHAN_MAX				1
#endif /* CONFIG_BT_EATT */


extern struct k_mem_slab att_slab;

static int bt_gobr_l2cap_accept(struct bt_conn *conn, struct bt_l2cap_chan **ch)
{
	if (connect_status_cb && connect_status_cb()) {
		BT_ERR("bt_gobr_l2cap_accept conn %p", conn);
		return -ENOMEM;
	}

	static const struct bt_l2cap_chan_ops ops = {
		.connected = bt_gobr_l2cap_connected,
		.disconnected = bt_gobr_l2cap_disconnected,
		.recv = bt_gobr_l2cap_recv, 
		.released = bt_gobr_l2cap_released,
	};

	BT_DBG("conn %p", conn);

	struct bt_att *att;
	struct bt_att_chan *chan;

	BT_DBG("conn %p handle %u", conn, conn->handle);

	if (k_mem_slab_alloc(&att_slab, (void **)&att, K_NO_WAIT)) {
		BT_ERR("No available ATT context for conn %p", conn);
		return -ENOMEM;
	}

	(void)memset(att, 0, sizeof(*att));
	att->conn = conn;
	sys_slist_init(&att->reqs);
	sys_slist_init(&att->chans);

	chan = att_chan_new(att, 0);
	if (!chan) {
		return -ENOMEM;
	}

	chan->br_chan.chan.ops = (struct bt_l2cap_chan_ops *)&ops;
	chan->br_chan.rx.mtu = CONFIG_BT_L2CAP_RX_MTU;//BT_L2CAP_RX_MTU;
	chan->br_chan.tx.mtu = CONFIG_BT_L2CAP_TX_MTU;
	BT_INFO("chan %p tx.mtu %d rx.mtu %d.",chan,chan->br_chan.tx.mtu, chan->br_chan.rx.mtu);
	//atomic_set(att->flags, 0);
	//k_sem_init(&att->tx_sem, CONFIG_BT_ATT_TX_MAX,
	//		   CONFIG_BT_ATT_TX_MAX);;
	*ch = &chan->br_chan.chan;
	bredr_att = att;
	BT_DBG("att %p att_chan %p", att, chan);
	//sys_slist_append(&att->chans, &chan->node);

	return 0;
}

int bt_gobr_init(void)
{
	int err;
	static struct bt_l2cap_server gobr_l2cap = {
		.psm = BT_L2CAP_PSM_GOBR,
		.sec_level = BT_SECURITY_L0,//BT_SECURITY_MEDIUM,
		.accept = bt_gobr_l2cap_accept,
	};

	BT_DBG("");

	err = bt_l2cap_br_server_register(&gobr_l2cap);
	if (err < 0) {
		BT_ERR("GOBR L2CAP Registration failed %d", err);
	}

	return err;
}

void bt_gobr_reg_connected_cb(bt_gobr_connected_cb_t cb, bt_gatt_connect_status_cb_t status_cb)
{
	BT_DBG("");
	connect_cb_func = cb;
	connect_status_cb = status_cb;
}

struct bt_l2cap_chan *bt_att_lookup_br_chan(struct bt_conn *conn)
{
	//struct bt_l2cap_chan *chan;
	
	struct bt_att_chan *chan;

	if (!bredr_att)
	{
		BT_INFO("bredr_att is NULL.");
		return NULL;
	}

	BT_INFO("bredr_att %p %p.",bredr_att->conn,conn);
	if (bredr_att->conn == conn)
	{
		BT_DBG("bredr_att->chans %p", &bredr_att->chans);
		SYS_SLIST_FOR_EACH_CONTAINER(&bredr_att->chans, chan, node) {
			BT_INFO("chan att %p %p.",chan->att,bredr_att);
			if (chan->att == bredr_att) {
				return &chan->br_chan.chan;
			}
		}
	}

	return NULL;
}

struct bt_att_chan *bt_att_chan_lookup_br_chan(struct bt_conn *conn)
{
	struct bt_att_chan *chan;

	if (!bredr_att)
	{
		BT_INFO("bredr_att is NULL.");
		return NULL;
	}

	BT_INFO("bredr_att %p %p.",bredr_att->conn,conn);
	if (bredr_att->conn == conn)
	{
		SYS_SLIST_FOR_EACH_CONTAINER(&bredr_att->chans, chan, node) {
			BT_INFO("chan att %p %p.",chan->att,bredr_att);
			if (chan->att == bredr_att) {
				return chan;
			}
		}
	}

	return NULL;
}

