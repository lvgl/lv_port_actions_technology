/*
 * Copyright (c) 2020 Actions Corporation.
 * Author: Jinang Lv <lvjinang@actions-semi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * usb-storage hotplug specification:
 * It is different from usb hotplug. In usb card reader case,
 * it means card could be removed but card reader can't.
 *
 * Design: delayed work(upper layer implements it).
 *
 * The policy for u-disk disconnect/connect detection is discribed
 * as follows.
 * 1. We suppose card reader is attaced default, which means we
 *     don't need to get pipes and max_luns again.
 * 2. Start detection at init time.
 */

#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <board.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <usb/usb_host.h>
#include <usb/usb_common.h>
#include <usb/storage.h>

#include "protocol.h"

#define LOG_LEVEL CONFIG_SYS_LOG_USB_HOST_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_storage);

/*
 * setting for bulk transfers timeout(unit: ms)
 * Some devices may need 600ms to reply for Test Unit Ready.
 *
 * Some devices take 4500ms for READ, some may need more.
 * Considering about compatibility, we set 5s by default.
 */
#define US_BULK_TIMEOUT	5000

#define DIR_TO_DEVICE	1
#define DIR_FROM_DEVICE	2

#define USB_DISK_NOT_INITED	0
#define USB_DISK_INITIALIZING	1
#define USB_DISK_INITED	2
#define USB_DISK_NOT_ACCESSED	3

#define US_MAX_LUN_DEFAULT 0

#define US_IOBUF_SIZE	32

/* WARNNING: please add member by appending */
struct us_data {
	/* CBW or DATA */
	union {
		u8_t iobuf[US_IOBUF_SIZE];
		struct bulk_cb_wrap cbw;
		u8_t inquiry[INQUIRY_LENGTH];
		read_cap_data_t capacity;
		u8_t requeset_sense[REQUEST_SENSE_LENGTH];
		u8_t mode_sense[MODE_SENSE_LENGTH];
	};
	/* CSW */
	struct bulk_cs_wrap csw;

	u8_t max_lun;
	u8_t state;
	u8_t reserved;

	u32_t cbw_tag;

	/* Data Phase if it has */
	u8_t *data;
	u32_t length;
	u8_t dir;

	/* usb info */
	u8_t ifnum;
	u8_t epin;
	u8_t epout;

	/* Logical Block Address, number of sectors = lba + 1 */
	u32_t lba;
	/* Block Length */
	u32_t blksz;

	struct k_mutex dev_mutex;
} __aligned(4);

static struct us_data us_data;

static int usb_stor_request_sense(struct us_data *us, u8_t lun);

void usb_stor_set_access(bool accessed)
{
	struct us_data *us = &us_data;
	u8_t state;

	k_mutex_lock(&us->dev_mutex, K_FOREVER);

	state = us->state;

	if (state == USB_DISK_INITED && !accessed) {
		us->state = USB_DISK_NOT_ACCESSED;
		usbh_fifo_control(false);
	} else if (state == USB_DISK_NOT_ACCESSED && accessed) {
		us->state = USB_DISK_INITED;
		usbh_fifo_control(true);
	}

	LOG_DBG("%d->%d", state, us->state);

	k_mutex_unlock(&us->dev_mutex);
}

static inline int usb_stor_get_max_lun(struct us_data *us)
{
	u8_t lun;
	int len;

	len = usbh_control_msg(US_BULK_GET_MAX_LUN,
				USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,
				0, us->ifnum, &lun, 1, USB_CTRL_TIMEOUT);
	if (len < 0) {
		return len;
	}

	LOG_DBG("len: %i, lun: %d\n", len, (int)lun);

	/*
	 * Some devices don't like GetMaxLUN.  They may STALL the control
	 * pipe, they may return a zero-length result, they may do nothing at
	 * all and timeout, or they may fail in even more bizarrely creative
	 * ways.  In these cases the best approach is to use the default
	 * value: only one LUN.
	 */
	return (len > 0) ? lun : 0;
}

static inline int interpret_result(struct us_data *us, u8_t ep, int result,
				int length, int actual)
{
	switch (result) {
	case 0:
		if (actual != length) {
			LOG_ERR("len %d %d", actual, length);
			return USB_STOR_XFER_SHORT;
		}
		return USB_STOR_XFER_GOOD;

	/* stalled */
	case -EPIPE:
		/* clear stall */
		if (usbh_clear_halt(ep) < 0) {
			return USB_STOR_XFER_ERROR;
		}
		return USB_STOR_XFER_STALLED;

	/* disconnected */
	case -ENODEV:
		return USB_STOR_XFER_NODEV;

	/* timeout */
	case -ECONNRESET:
	default:
		LOG_ERR("%d", result);
		return USB_STOR_XFER_ERROR;
	}
}

static inline int interpret_csw_result(struct us_data *us, u8_t ep, int result,
				int length, int actual)
{
	switch (result) {
	case 0:
		if (actual != length) {
			LOG_ERR("len %d %d", actual, length);
			return USB_STOR_XFER_SHORT;
		}
		return USB_STOR_XFER_GOOD;

	/* stalled */
	case -EPIPE:
		/* clear stall */
		if (usbh_clear_halt(ep) < 0) {
			return USB_STOR_XFER_ERROR;
		}
		return USB_STOR_XFER_STALLED;

	/* disconnected */
	case -ENODEV:
		return USB_STOR_XFER_NODEV;

	/* timeout */
	case -ECONNRESET:
		/*
		 * some u-disks may timeout first and will be fine if retry CSW,
		 * so give it one more chance!
		 */
		return USB_STOR_XFER_STALLED;

	default:
		LOG_ERR("%d", result);
		return USB_STOR_XFER_ERROR;
	}
}

static inline int bulk_send_cbw(struct us_data *us)
{
	u8_t ep = us->epout;
	int result, actual;

	us->cbw.Signature = US_BULK_CB_SIGN;
	us->cbw.Tag = ++us->cbw_tag;

	LOG_DBG("");

	result = usbh_bulk_msg(ep, &us->cbw, US_BULK_CB_WRAP_LEN, &actual,
				US_BULK_TIMEOUT);
	return interpret_result(us, ep, result, US_BULK_CB_WRAP_LEN, actual);
}

static inline int bulk_receive_data(struct us_data *us)
{
	u8_t ep = us->epin;
	int result, actual;

	LOG_DBG("");

	result = usbh_bulk_msg(ep, us->data, us->length, &actual,
				US_BULK_TIMEOUT);
	return interpret_result(us, ep, result, us->length, actual);
}

static inline int bulk_send_data(struct us_data *us)
{
	u8_t ep = us->epout;
	int result, actual;

	LOG_DBG("");

	result = usbh_bulk_msg(ep, us->data, us->length, &actual,
				US_BULK_TIMEOUT);
	return interpret_result(us, ep, result, us->length, actual);
}

static inline int bulk_receive_csw(struct us_data *us)
{
	u8_t ep = us->epin;
	int result, actual;

	LOG_DBG("");

	result = usbh_bulk_msg(ep, &us->csw, US_BULK_CS_WRAP_LEN, &actual,
				US_BULK_TIMEOUT);
	result = interpret_csw_result(us, ep, result, US_BULK_CS_WRAP_LEN, actual);
	if (result != USB_STOR_XFER_STALLED) {
		return result;
	}

	result = usbh_bulk_msg(ep, &us->csw, US_BULK_CS_WRAP_LEN, &actual,
				US_BULK_TIMEOUT);
	return interpret_result(us, ep, result, US_BULK_CS_WRAP_LEN, actual);
}

static inline int usb_stor_error_handler(struct us_data *us)
{
	int ret;

	LOG_DBG("reset");

	ret = usbh_reset_device();
	if (ret) {
		goto error;
	}

	ret = usb_stor_request_sense(us, US_MAX_LUN_DEFAULT);
	if (ret) {
		goto error;
	}

	LOG_DBG("OK");

	return 0;

error:
	LOG_ERR("failed");
	us->state = USB_DISK_NOT_INITED;
	usbh_vbus_set(false);
	return ret;
}

static inline int usb_stor_bulk_transport(struct us_data *us)
{
	int ret;

	ret = bulk_send_cbw(us);
	if (ret != USB_STOR_XFER_GOOD) {
		return ret;
	}

	if (us->length) {
		if (us->dir == DIR_FROM_DEVICE) {
			ret = bulk_receive_data(us);
		} else {
			ret = bulk_send_data(us);
		}

		if (ret == USB_STOR_XFER_STALLED) {
			;	/* continue */
		} else if (ret != USB_STOR_XFER_GOOD) {
			return ret;
		}
	}

	ret = bulk_receive_csw(us);
	if (ret != USB_STOR_XFER_GOOD) {
		return ret;
	}

	/* validity check */
	switch (us->csw.Status) {
	case US_BULK_STAT_FAIL:
		/* command failed */
		LOG_DBG("failed");
		return USB_STOR_XFER_FAILED;

	case US_BULK_STAT_PHASE:
		LOG_ERR("phase");
		return USB_STOR_XFER_ERROR;
	}

	return USB_STOR_XFER_GOOD;
}

/*
 * For a normal SCSI operation, it includes 2 phases at least:
 * cbw, (data), csw.
 * The old scheme: clear halt -> complete the remianing
 * op -> request sense -> retry. If STALL happened at cbw
 * phase, we will do csw anyway and return the "right" value
 * which belongs to csw phase, that will make upper layer
 * mistake.
 * The new scheme: only STALL happens at csw phase, we'll
 * retry, otherwise return the error number!
 */

static inline int usb_stor_do_inquiry(struct us_data *us, u8_t lun)
{
	u8_t retry = 5;
	int ret;

	LOG_DBG("");

	us->cbw.DataLength = INQUIRY_LENGTH;
	us->cbw.Flags = USB_DIR_IN;
	us->cbw.LUN = lun;
	us->cbw.CBLength = 6;

	memset(us->cbw.CB, 0, 16);

	us->cbw.CB[0] = INQUIRY;
	us->cbw.CB[1] = lun << 5;
	us->cbw.CB[4] = INQUIRY_LENGTH;

	us->data = us->inquiry;
	us->length = INQUIRY_LENGTH;
	us->dir = DIR_FROM_DEVICE;

	do {
		ret = usb_stor_bulk_transport(us);
		if (ret == USB_STOR_XFER_GOOD) {
			if (us->inquiry[0] != DIRECT_ACCESS_DEVICE) {
				LOG_WRN("type: %d", us->inquiry[0]);
			}
			break;
		} else if (ret == USB_STOR_XFER_FAILED) {
			;	/* retry */
		} else {
			break;
		}
	} while (--retry);

	return ret;
}

/*
 * Request Sense is special, we use it to check if u-disk recover
 * from STALL (for example: Read -> STALL -> Request Sense
 * -> retry Read) and after port reset.
 */
static inline int usb_stor_request_sense(struct us_data *us, u8_t lun)
{
	int ret;

	LOG_DBG("");

	us->cbw.DataLength = REQUEST_SENSE_LENGTH;
	us->cbw.Flags = USB_DIR_IN;
	us->cbw.LUN = lun;
	us->cbw.CBLength = 12;

	memset(us->cbw.CB, 0, 16);

	us->cbw.CB[0] = REQUEST_SENSE;
	us->cbw.CB[1] = lun << 5;
	us->cbw.CB[4] = REQUEST_SENSE_LENGTH;

	us->data = us->requeset_sense;
	us->length = REQUEST_SENSE_LENGTH;

	ret = bulk_send_cbw(us);
	if (ret != USB_STOR_XFER_GOOD) {
		return ret;
	}

	ret = bulk_receive_data(us);
	if (ret == USB_STOR_XFER_STALLED) {
		;	/* continue */
	} else if (ret != USB_STOR_XFER_GOOD) {
		return ret;
	}

	ret = bulk_receive_csw(us);
	if (ret != USB_STOR_XFER_GOOD) {
		return ret;
	}

	if (us->csw.Status != US_BULK_STAT_OK) {
		return USB_STOR_XFER_ERROR;
	}

	return USB_STOR_XFER_GOOD;
}

/**
 * SCSI Command: Test Unit Ready
 * @return: 0 on success, negative on Command Failed(Not Ready),
 * positive on STALL or ERROR.
 */
static inline int usb_stor_test_unit_ready(struct us_data *us, u8_t lun)
{
	u8_t retry = 10;
	int ret;

	LOG_DBG("");

	do {
		us->cbw.DataLength = 0;
		us->cbw.Flags = USB_DIR_OUT;
		us->cbw.LUN = lun;
		us->cbw.CBLength = 6;

		memset(us->cbw.CB, 0, 16);

		us->cbw.CB[0] = TEST_UNIT_READY;

		us->length = 0;

		ret = usb_stor_bulk_transport(us);
		if (ret == USB_STOR_XFER_GOOD) {
			break;
		} else if (ret == USB_STOR_XFER_FAILED) {
			;	/* retry */
		} else {
			break;
		}

		usb_stor_request_sense(us, lun);

		k_sleep(K_MSEC(100));
	} while (--retry);

	return ret;
}

static inline int usb_stor_read_capacity(struct us_data *us, u8_t lun)
{
	u8_t retry = 3;
	int ret;

	LOG_DBG("");

	us->cbw.DataLength = READ_CAPACITY_LENGTH;
	us->cbw.Flags = USB_DIR_IN;
	us->cbw.LUN = lun;
	us->cbw.CBLength = 10;

	memset(us->cbw.CB, 0, 16);

	us->cbw.CB[0] = READ_CAPACITY;

	us->data = (u8_t *)&us->capacity;
	us->length = READ_CAPACITY_LENGTH;
	us->dir = DIR_FROM_DEVICE;

	do {
		ret = usb_stor_bulk_transport(us);
		if (ret == USB_STOR_XFER_GOOD) {
			us->lba = sys_be32_to_cpu(us->capacity.lba);
			us->blksz = sys_be32_to_cpu(us->capacity.blksz);

			LOG_INF("lba: 0x%x, blksz: 0x%x", us->lba, us->blksz);
			break;
		} else if (ret == USB_STOR_XFER_FAILED) {
			;	/* retry */
		} else {
			break;
		}
	} while (--retry);

	return ret;
}

/**
 * @offset: current lba
 * @len: numbers of sectors
 */
static int usb_stor_read(struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct us_data *us = &us_data;
	u8_t retry = 3;
	int ret;

	k_mutex_lock(&us->dev_mutex, K_FOREVER);

	if (us->state != USB_DISK_INITED) {
		LOG_DBG("%d", us->state);
		ret = -ENODEV;
		goto exit;
	}

	do {
		us->cbw.DataLength = len * us->blksz;
		us->cbw.Flags = USB_DIR_IN;
		us->cbw.LUN = 0x0;
		us->cbw.CBLength = 0x0a;

		memset(us->cbw.CB, 0, 16);

		us->cbw.CB[0] = READ10;
		us->cbw.CB[1] = 0x0;

		/*
		 * Logical Block Addr
		 * NOTICE: cdb is big-endian
		 */
		us->cbw.CB[2] = (u8_t)(offset >> 24);
		us->cbw.CB[3] = (u8_t)(offset >> 16);
		us->cbw.CB[4] = (u8_t)(offset >> 8);
		us->cbw.CB[5] = (u8_t)(offset >> 0);

		/* Transfer Len */
		us->cbw.CB[7] = (u8_t)(len >> 8);
		us->cbw.CB[8] = (u8_t)len;

		us->data = data;
		us->length = us->cbw.DataLength;
		us->dir = DIR_FROM_DEVICE;

		ret = usb_stor_bulk_transport(us);
		switch (ret) {
		case USB_STOR_XFER_NODEV:
		case USB_STOR_XFER_GOOD:
			goto exit;

		case USB_STOR_XFER_FAILED:
			if (usb_stor_request_sense(us,
			    US_MAX_LUN_DEFAULT) == USB_STOR_XFER_GOOD) {
				break;
			}
			/* fall through */
		case USB_STOR_XFER_ERROR:
		default:
			if (usb_stor_error_handler(us)) {
				goto exit;
			}
			break;
		}
	} while (--retry);

exit:
	k_mutex_unlock(&us->dev_mutex);

	return ret;
}

static int usb_stor_write(struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct us_data *us = &us_data;
	u8_t retry = 3;
	int ret;

	k_mutex_lock(&us->dev_mutex, K_FOREVER);

	if (us->state != USB_DISK_INITED) {
		LOG_DBG("%d", us->state);
		ret = -ENODEV;
		goto exit;
	}

	do {
		us->cbw.DataLength = len * us->blksz;
		us->cbw.Flags = USB_DIR_OUT;
		us->cbw.LUN = 0x0;
		us->cbw.CBLength = 0x0a;

		memset(us->cbw.CB, 0, 16);

		us->cbw.CB[0] = WRITE10;
		us->cbw.CB[1] = 0x0;

		/*
		 * Logical Block Addr
		 * NOTICE: cdb is big-endian
		 */
		us->cbw.CB[2] = (u8_t)(offset >> 24);
		us->cbw.CB[3] = (u8_t)(offset >> 16);
		us->cbw.CB[4] = (u8_t)(offset >> 8);
		us->cbw.CB[5] = (u8_t)(offset >> 0);

		/* Transfer Len */
		us->cbw.CB[7] = (u8_t)(len >> 8);
		us->cbw.CB[8] = (u8_t)len;

		us->data = (u8_t *)data;
		us->length = us->cbw.DataLength;
		us->dir = DIR_TO_DEVICE;

		ret = usb_stor_bulk_transport(us);
		switch (ret) {
		case USB_STOR_XFER_NODEV:
		case USB_STOR_XFER_GOOD:
			goto exit;

		case USB_STOR_XFER_FAILED:
			if (usb_stor_request_sense(us,
			    US_MAX_LUN_DEFAULT) == USB_STOR_XFER_GOOD) {
				break;
			}
			/* fall through */
		case USB_STOR_XFER_ERROR:
		default:
			if (usb_stor_error_handler(us)) {
				goto exit;
			}
			break;
		}
	} while (--retry);

exit:
	k_mutex_unlock(&us->dev_mutex);

	return ret;
}

/**
 *  @brief  Poll u-disk status: connected or disconnected.
 *
 *  If usb disk supports hotplug, it implys if the u-disk is
 *  present or not.
 *  If usb disk doesn't support hotplug, the return value
 *  is always USB_DISK_NOOP!
 *
 *  @return  USB_DISK_CONNECTED: connected;
 *               USB_DISK_DISCONNECTED: disconnected;
 *               USB_DISK_NOOP: nothing happened, ignore.
 */
static int usb_disk_hotplug_poll(struct us_data *us)
{
	if (us->state == USB_DISK_INITED) {
		return USB_DISK_CONNECTED;
	}

	return USB_DISK_DISCONNECTED;
}

/* sector count, sector size and capacity */
static int usb_stor_ioctl(struct device *dev, u8_t cmd, void *buff)
{
	struct us_data *us = &us_data;

	switch (cmd) {
	case USB_DISK_IOCTL_GET_SECTOR_COUNT:
		*(u32_t *)buff = us_data.lba + 1;	/* is it OK? */
		break;

	/* 64KB at most for block length, should be enough */
	case USB_DISK_IOCTL_GET_SECTOR_SIZE:
		*(u16_t *)buff = us_data.blksz;
		break;

	/* 4GB at most, is it OK? */
	case USB_DISK_IOCTL_GET_DISK_SIZE:
		*(u32_t *)buff = us_data.blksz * (us_data.lba + 1);
		break;

	case USB_DISK_IOCTL_GET_STATUS:
		*(u8_t *)buff = usb_disk_hotplug_poll(us);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct storage_driver_api us_api_funcs = {
	.read = usb_stor_read,
	.write = usb_stor_write,
	.ioctl = usb_stor_ioctl,
};

static int usb_stor_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	struct usb_ep_descriptor *ep_desc;
	struct us_data *us;
	int ret;
	u8_t i;

	ARG_UNUSED(id);

	LOG_DBG("");

	us = &us_data;

	us->state = USB_DISK_NOT_INITED;
	us->cbw_tag = 0;

	for (i = 0; i < intf->no_of_ep; i++) {
		ep_desc = &intf->ep_desc[i];
		if (!usb_endpoint_xfer_bulk(ep_desc->bmAttributes)) {
			continue;
		}
		if (usb_ep_dir_in(ep_desc->bEndpointAddress)) {
			us->epin = ep_desc->bEndpointAddress;
		} else {
			us->epout = ep_desc->bEndpointAddress;
		}

		ret = usbh_enable_endpoint(ep_desc);
		if (ret) {
			return ret;
		}
	}

	us->ifnum = intf->desc.bInterfaceNumber;

	us->max_lun = usb_stor_get_max_lun(us);

	/* FIXME: support one LUN only */
	if (us->max_lun != US_MAX_LUN_DEFAULT) {
		LOG_ERR("max_lun %d", us->max_lun);
		return -EINVAL;
	}

	us->state = USB_DISK_INITIALIZING;

	/* for each lun */
	for (i = 0; i <= us->max_lun; i++) {
		ret = usb_stor_do_inquiry(us, i);
		if (ret) {
			LOG_ERR("inquiry %d", ret);
			return ret;
		}

		ret = usb_stor_test_unit_ready(us, i);
		if (ret) {
			LOG_ERR("TUR %d", ret);
			return ret;
		}

		ret = usb_stor_read_capacity(us, i);
		if (ret) {
			LOG_ERR("capacity %d", ret);
			return ret;
		}
	}

	us->state = USB_DISK_INITED;

	LOG_INF("done");

	return 0;
}

static void usb_stor_disconnect(struct usb_interface *intf)
{
	struct us_data *us = &us_data;

	LOG_INF("");

	us->state = USB_DISK_NOT_INITED;

	usbh_disable_interface(intf);

	/* wait for read/write done: should be fast */
	k_mutex_lock(&us->dev_mutex, K_FOREVER);
	k_mutex_unlock(&us->dev_mutex);

	LOG_INF("done");
}

static const struct usb_device_id usb_storage_ids[] = {
	{
		.match_flags = USB_DEVICE_ID_MATCH_INT_INFO,
		.bInterfaceClass = MASS_STORAGE_CLASS,
		.bInterfaceSubClass = SCSI_TRANSPARENT_SUBCLASS,
		.bInterfaceProtocol = BULK_ONLY_PROTOCOL
	},

	{ }		/* Terminating entry */
};

static struct usb_driver usb_storage_driver = {
	.probe = usb_stor_probe,
	.disconnect = usb_stor_disconnect,
	.id_table =	usb_storage_ids,
};

bool usb_host_storage_enabled(void)
{
	return us_data.state == USB_DISK_INITED;
}

static int usb_stor_init(struct device *dev)
{
	struct us_data *us = &us_data;

	ARG_UNUSED(dev);

	k_mutex_init(&us->dev_mutex);

	return usbh_register_driver(&usb_storage_driver);
}

DEVICE_AND_API_INIT(usb_storage, "usb-storage", usb_stor_init,
				NULL, NULL,
				POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
				&us_api_funcs);
