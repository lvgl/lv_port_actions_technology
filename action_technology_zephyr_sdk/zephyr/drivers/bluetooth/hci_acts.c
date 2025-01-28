/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth hci driver
 */
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#ifdef CONFIG_ACTS_BT
#include <acts_bluetooth/buf.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#else
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#endif
#include <drivers/bluetooth/hci_driver.h>
#include <drivers/bluetooth/bt_drv.h>

static struct net_buf *buf;

static uint8_t *hci_get_buf(uint8_t type, uint8_t evt, uint16_t exp_len)
{
	switch (type) {
	case HCI_EVT:
		if (exp_len) {
			buf = bt_buf_get_evt_len(evt, false, K_NO_WAIT, exp_len);
		} else {
			buf = bt_buf_get_evt(evt, false, K_NO_WAIT);
		}
		break;
	case HCI_ACL:
		if (exp_len) {
			buf = bt_buf_get_rx_len(BT_BUF_ACL_IN, K_NO_WAIT, exp_len);
		} else {
			buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		}
		break;
	case HCI_SCO:
		if (exp_len) {
			buf = bt_buf_get_rx_len(BT_BUF_SCO_IN, K_NO_WAIT, exp_len);
		} else {
			buf = bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT);
		}
		break;
	case HCI_ISO:
		if (exp_len) {
			buf = bt_buf_get_rx_len(BT_BUF_ISO_IN, K_NO_WAIT, exp_len);
		} else {
			buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
		}
		break;
	default:
		buf = NULL;
		break;
	}

	if (!buf) {
		printk("Error to get buf!\n");
		return NULL;
	}

	return buf->data;
}

/* process hci rx buf */
static int hci_recv(uint16_t len)
{
	if (buf) {
		net_buf_add(buf, len);
		return bt_recv(buf);
	} else {
		printk("hci_recv buf NULL!\n");
		return -ENOMEM;
	}
}

static int acts_hci_send(struct net_buf *buf)
{
    int err = 0;
    uint8_t type = 0;

    if (buf == NULL) {
        return -EINVAL;
    }

    switch (bt_buf_get_type(buf)) {
    case BT_BUF_CMD:
        type = HCI_CMD;
        break;
    case BT_BUF_ACL_OUT:
        type = HCI_ACL;
        break;
    case BT_BUF_SCO_OUT:
        type = HCI_SCO;
        break;
    case BT_BUF_ISO_OUT:
        type = HCI_ISO;
        break;
    default:
        printk("unknown buf type\n");
        break;
    }

    err = btdrv_send(type, buf->data, buf->len);

    net_buf_unref(buf);  //should unref buf

    return err;
}

static const btdrv_hci_cb_t cb = {
    .get_buf = hci_get_buf,
    .recv = hci_recv,
};

static int acts_hci_open(void)
{
    return btdrv_init((btdrv_hci_cb_t *)&cb);
}

static const struct bt_hci_driver drv = {
    .name = "H:ACTS",
    .bus = BT_HCI_DRIVER_BUS_VIRTUAL,
    .quirks = BT_QUIRK_NO_AUTO_DLE,		/* Controller does not auto-initiate a DLE procedure */
    .open = acts_hci_open,
    .send = acts_hci_send,
};

static int bt_acts_init(const struct device *unused)
{
    ARG_UNUSED(unused);

    printk("bt hci drv register\n");
    bt_hci_driver_register(&drv);

    return 0;
}

SYS_INIT(bt_acts_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
