/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <acts_bluetooth/buf.h>
#include <acts_bluetooth/l2cap.h>

#include "hci_core.h"
#include "conn_internal.h"
#include "audio/iso_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_buf
#include "common/log.h"

extern struct net_buf_pool_continue host_rx_pool;
extern struct net_buf_pool_continue host_tx_pool;

BT_BUF_POOL_DEFINE(hci_rx_pool, CONFIG_BT_RX_BUF_COUNT,
			  BT_BUF_RX_SIZE, 4, NULL, &host_rx_pool);

#if defined(CONFIG_BT_CONN)
#define NUM_COMLETE_EVENT_SIZE BT_BUF_SIZE(\
	sizeof(struct bt_hci_evt_hdr) +                                \
	sizeof(struct bt_hci_cp_host_num_completed_packets) +          \
	CONFIG_BT_MAX_CONN * sizeof(struct bt_hci_handle_count))
/* Dedicated pool for HCI_Number_of_Completed_Packets. This event is always
 * consumed synchronously by bt_recv_prio() so a single buffer is enough.
 * Having a dedicated pool for it ensures that exhaustion of the RX pool
 * cannot block the delivery of this priority event.
 */
BT_BUF_POOL_DEFINE(num_complete_pool, 1, NUM_COMLETE_EVENT_SIZE, 4, NULL, &host_rx_pool);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
#define DISCARDABLE_EVENT_SIZE BT_BUF_SIZE(CONFIG_BT_DISCARDABLE_BUF_SIZE)
BT_BUF_POOL_DEFINE(discardable_pool, CONFIG_BT_DISCARDABLE_BUF_COUNT,
			  DISCARDABLE_EVENT_SIZE, 4, NULL, &host_tx_pool);
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
#define ACL_IN_SIZE BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_RX_MTU)
BT_BUF_POOL_DEFINE(acl_in_pool, CONFIG_BT_ACL_RX_COUNT, ACL_IN_SIZE,
		    sizeof(struct acl_data), bt_hci_host_num_completed_packets, &host_rx_pool);
#endif /* CONFIG_BT_HCI_ACL_FLOW_CONTROL */

struct net_buf *bt_buf_get_rx(enum bt_buf_type type, k_timeout_t timeout)
{
	struct net_buf *buf;

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN || type == BT_BUF_SCO_IN ||
		 type == BT_BUF_ISO_IN, "Invalid buffer type requested");

	if (IS_ENABLED(CONFIG_BT_ISO) && type == BT_BUF_ISO_IN) {
		return bt_iso_get_rx(timeout);
	}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc(&hci_rx_pool, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc(&hci_rx_pool, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_rx_len(enum bt_buf_type type, k_timeout_t timeout, int len)
{
	struct net_buf *buf;
	uint16_t alloc_len;
	uint16_t rx_max_len = L2CAP_BR_MAX_MTU_A2DP + BT_HCI_ACL_HDR_SIZE + BT_L2CAP_HDR_SIZE;

	if (len > rx_max_len) {
		BT_ERR("Too length %d(%d)\n", len, rx_max_len);
		/* For check, later just drop this packet */
		//__ASSERT(false, "Too larger!!");
		return NULL;
	}

	__ASSERT(type == BT_BUF_EVT || type == BT_BUF_ACL_IN || type == BT_BUF_SCO_IN ||
		type == BT_BUF_ISO_IN, "Invalid buffer type requested");

	alloc_len = len + BT_BUF_RESERVE;

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
	if (type == BT_BUF_EVT) {
		buf = net_buf_alloc_len(&hci_rx_pool, alloc_len, timeout);
	} else {
		buf = net_buf_alloc(&acl_in_pool, timeout);
	}
#else
	buf = net_buf_alloc_len(&hci_rx_pool, alloc_len, timeout);
#endif

	if (buf) {
		net_buf_reserve(buf, BT_BUF_RESERVE);
		bt_buf_set_type(buf, type);
	}

	return buf;
}

struct net_buf *bt_buf_get_cmd_complete(k_timeout_t timeout)
{
	struct net_buf *buf;
	unsigned int key;

	key = irq_lock();
	buf = bt_dev.sent_cmd;
	bt_dev.sent_cmd = NULL;
	irq_unlock(key);

	BT_DBG("sent_cmd %p", buf);

	if (buf) {
		bt_buf_set_type(buf, BT_BUF_EVT);
		buf->len = 0U;
		net_buf_reserve(buf, BT_BUF_RESERVE);

		return buf;
	}

	return bt_buf_get_rx(BT_BUF_EVT, timeout);
}

struct net_buf *bt_buf_get_evt(uint8_t evt, bool discardable,
			       k_timeout_t timeout)
{
	switch (evt) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
		{
			struct net_buf *buf;

			buf = net_buf_alloc(&num_complete_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_CONN */
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
		return bt_buf_get_cmd_complete(timeout);
	default:
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
		if (discardable) {
			struct net_buf *buf;

			buf = net_buf_alloc(&discardable_pool, timeout);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

		return bt_buf_get_rx(BT_BUF_EVT, timeout);
	}
}

struct net_buf *bt_buf_get_evt_len(uint8_t evt, bool discardable,
			       k_timeout_t timeout, int len)
{
	switch (evt) {
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
		{
			struct net_buf *buf;

			buf = net_buf_alloc_len(&num_complete_pool, len, timeout);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_CONN */
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
		return bt_buf_get_cmd_complete(timeout);
	default:
#if defined(CONFIG_BT_DISCARDABLE_BUF_COUNT)
		if (discardable) {
			struct net_buf *buf;

			buf = net_buf_alloc(&discardable_pool, timeout, len);
			if (buf) {
				net_buf_reserve(buf, BT_BUF_RESERVE);
				bt_buf_set_type(buf, BT_BUF_EVT);
			}

			return buf;
		}
#endif /* CONFIG_BT_DISCARDABLE_BUF_COUNT */

		return bt_buf_get_rx_len(BT_BUF_EVT, timeout, len);
	}
}
