/*
 * USB Stub Device (Bulk-Transfer) class core.
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

#include "usb_stub_descriptor.h"

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#define LOG_LEVEL CONFIG_SYS_LOG_USB_STUB_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_stub_dev);

#define USB_STUB_ENABLED	BIT(0)
#define USB_STUB_CONFIGURED	BIT(1)

static u8_t usb_stub_state;
static struct k_sem stub_write_sem;
static struct k_sem stub_read_sem;

static void usb_stub_ep_write_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	LOG_DBG("stub_ep_write_cb");
	k_sem_give(&stub_write_sem);
}

static void usb_stub_ep_read_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	k_sem_give(&stub_read_sem);
}

/* USB endpoint configuration */
static const struct usb_ep_cfg_data stub_ep_cfg[] = {
	{
		.ep_cb	= usb_stub_ep_write_cb,
		.ep_addr = CONFIG_STUB_IN_EP_ADDR
	},
	{
		.ep_cb	= usb_stub_ep_read_cb,
		.ep_addr = CONFIG_STUB_OUT_EP_ADDR
	},
};

bool usb_stub_enabled(void)
{
	return usb_stub_state & USB_STUB_ENABLED;
}

bool usb_stub_configured(void)
{
	return usb_stub_state & USB_STUB_CONFIGURED;
}

static void usb_stub_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		usb_stub_state |= USB_STUB_CONFIGURED;
		LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_HIGHSPEED:
		LOG_DBG("USB device high-speed");
		break;
	case USB_DC_SOF:
		LOG_DBG("USB device sof-inter");
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

static int usb_stub_class_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("class request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return 0;
}

static int usb_stub_custom_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("custom request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return -1;
}

static int usb_stub_vendor_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("vendor request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return 0;
}


static const struct usb_cfg_data usb_stub_config = {
	.usb_device_description = NULL,
	.cb_usb_status = usb_stub_status_cb,
	.interface = {
		.class_handler  = usb_stub_class_handle_req,
		.custom_handler = usb_stub_custom_handle_req,
		.vendor_handler = usb_stub_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(stub_ep_cfg),
	.endpoint = stub_ep_cfg,
};

int usb_stub_ep_read(u8_t *data_buffer, u32_t data_len, k_timeout_t timeout)
{
	int read_bytes;
	int ret;

	while (data_len) {
		if (k_sem_take(&stub_read_sem, timeout) != 0) {
			usb_dc_ep_flush(CONFIG_STUB_OUT_EP_ADDR);
			LOG_ERR("timeout");
			return -ETIME;
		}
		ret = usb_read(CONFIG_STUB_OUT_EP_ADDR, data_buffer,
				data_len, &read_bytes);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_OUT_EP_ADDR);
			return ret;
		}

		data_buffer += read_bytes;
		data_len -= read_bytes;
	}

	return 0;
}

int usb_stub_ep_write(u8_t *data_buffer, u32_t data_len, k_timeout_t timeout)
{
	int write_bytes;
	int ret;
	u8_t need_zero = 0;
	u8_t zero;

	switch (usb_device_speed()) {
	case USB_SPEED_FULL:
		if (data_len % CONFIG_STUB_EP_MPS == 0) {
			need_zero = 1;
		}
		break;
	case USB_SPEED_HIGH:
		if (data_len % USB_MAX_HS_BULK_MPS == 0) {
			need_zero = 1;
		}
		break;
	default:
		break;
	}

	while (data_len) {
		ret = usb_write(CONFIG_STUB_IN_EP_ADDR, data_buffer, data_len,
				&write_bytes);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return ret;
		}
		if (k_sem_take(&stub_write_sem, timeout) != 0) {
			LOG_ERR("timeout");
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return -ETIME;
		}
		data_buffer += write_bytes;
		data_len -= write_bytes;
	}

	if (need_zero) {
		ret = usb_write(CONFIG_STUB_IN_EP_ADDR, &zero, 0, NULL);
		if (ret < 0) {
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return ret;
		}
		if (k_sem_take(&stub_write_sem, timeout) != 0) {
			printk("zero timeout");
			usb_dc_ep_flush(CONFIG_STUB_IN_EP_ADDR);
			return -ETIME;
		}
	}

	return 0;
}

static int usb_stub_fix_dev_sn(void)
{
	int ret;

#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_STUB_DEVICE_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_STUB_DEVICE_SN, strlen(CONFIG_USB_STUB_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_STUB_DEVICE_SN, strlen(CONFIG_USB_STUB_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

/*
 * API: initialize USB STUB
 */
int usb_stub_init(struct device *dev)
{
	int ret;

	k_sem_init(&stub_write_sem, 0, 1);
	k_sem_init(&stub_read_sem, 0, 1);

	/* register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_STUB_DEVICE_MANUFACTURER, strlen(CONFIG_USB_STUB_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_STUB_DEVICE_PRODUCT, strlen(CONFIG_USB_STUB_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_stub_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* register device descriptor */
	usb_device_register_descriptors(usb_stub_fs_descriptor, usb_stub_hs_descriptor);

	/* initialize the USB driver with the right configuration */
	ret = usb_set_config(&usb_stub_config);
	if (ret < 0) {
		return ret;
	}

	/* enable USB driver */
	ret = usb_enable(&usb_stub_config);
	if (ret < 0) {
		return ret;
	}

	usb_stub_state = USB_STUB_ENABLED;

	return 0;
}

/*
 * API: deinitialize USB Stub
 */
int usb_stub_exit(void)
{
	int ret;

	usb_stub_state = 0;

	ret = usb_disable();
	if (ret) {
		LOG_ERR("failed to disable USB Stub device");
		return ret;
	}
	usb_deconfig();
	return 0;
}
