/*
 * USB Audio Source Device (ISO-Transfer) class core.
 *
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_audio.h>

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SOURCE_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_audio_source);

#include "usb_audio_source_desc.h"

#define AUDIO_STREAM_INTER1	1

static usb_audio_start audio_start_cb;
static usb_ep_callback iso_in_ep_cb;

static bool audio_streaming_enabled;

static void usb_audio_isoc_in(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{
	LOG_DBG("**isoc_in_cb!**\n");
	if (iso_in_ep_cb) {
		iso_in_ep_cb(ep, cb_status);
	}
}

static void usb_audio_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	static u8_t alt_setting;
	u8_t iface;

	switch (status) {
	case USB_DC_INTERFACE:
			/* iface: the higher byte, alt_setting: the lower byte */
			iface = *(u16_t *)param >> 8;
			alt_setting = *(u16_t *)param & 0xff;
			LOG_DBG("Set_Int: %d, alt_setting: %d\n", iface, alt_setting);
			switch (iface) {
			case AUDIO_STREAM_INTER1:
				if (!alt_setting) {
					audio_streaming_enabled = false;
				} else {
					audio_streaming_enabled = true;
				}
				if (audio_start_cb) {
					audio_start_cb(audio_streaming_enabled);
				}
				break;

			default:
				LOG_ERR("Unavailable interface number");
				break;
			}
	break;

	case USB_DC_ERROR:
		LOG_ERR("USB device error");
		break;

	case USB_DC_RESET:
		audio_streaming_enabled = false;
		if (audio_start_cb) {
			audio_start_cb(audio_streaming_enabled);
		}
		LOG_DBG("USB device reset detected");
		break;

	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;

	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		break;

	case USB_DC_DISCONNECTED:
		audio_streaming_enabled = false;
		if (audio_start_cb) {
			audio_start_cb(audio_streaming_enabled);
		}
		LOG_DBG("USB device disconnected");
		break;

	case USB_DC_SUSPEND:
		audio_streaming_enabled = false;
		if (audio_start_cb) {
			audio_start_cb(audio_streaming_enabled);
		}
		LOG_DBG("USB device suspended");
		break;

	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;

	case USB_DC_HIGHSPEED:
		LOG_INF("High-Speed mode handshake package");
		break;

	case USB_DC_SOF:
		LOG_DBG("USB device sof inter");
		break;

	case USB_DC_UNKNOWN:

	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

static int usb_audio_class_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("class request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static int usb_audio_custom_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("custom request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static int usb_audio_vendor_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("vendor request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

/* USB endpoint configuration */
static const struct usb_ep_cfg_data usb_audio_ep_cfg[] = {
	{
		.ep_cb	= usb_audio_isoc_in,
		.ep_addr = CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
	},
};

static const struct usb_cfg_data usb_audio_cfg = {
	.usb_device_description = NULL,
	.cb_usb_status = usb_audio_status_cb,
	.interface = {
		.class_handler  = usb_audio_class_handle_req,
		.custom_handler = usb_audio_custom_handle_req,
		.vendor_handler = usb_audio_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(usb_audio_ep_cfg),
	.endpoint = usb_audio_ep_cfg,
};

static int usb_audio_source_fix_dev_sn(void)
{
	int ret;
#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_AUDIO_SOURCE_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SOURCE_SN, strlen(CONFIG_USB_AUDIO_SOURCE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SOURCE_SN, strlen(CONFIG_USB_AUDIO_SOURCE_SN));
	if (ret) {
		return ret;
	}
#endif
	return 0;
}

/*
 * API: initialize usb audio source device.
 */
int usb_audio_source_init(struct device *dev)
{
	int ret;

	/* register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_AUDIO_SOURCE_MANUFACTURER, strlen(CONFIG_USB_AUDIO_SOURCE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_AUDIO_SOURCE_PRODUCT, strlen(CONFIG_USB_AUDIO_SOURCE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_audio_source_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* register device descriptor */
	usb_device_register_descriptors(usb_audio_source_fs_descriptor, usb_audio_source_hs_descriptor);

	/* initialize the USB driver with the right configuration */
	ret = usb_set_config(&usb_audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to config USB");
		return ret;
	}

	/* enable USB driver */
	ret = usb_enable(&usb_audio_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

/*
 * API: deinitialize usb audio source device.
 */
int usb_audio_source_deinit(void)
{
	int ret;

	ret = usb_disable();
	if (ret) {
		LOG_ERR("Failed to disable USB");
		return ret;
	}

	usb_deconfig();

	return 0;
}

void usb_audio_source_register_start_cb(usb_audio_start cb)
{
	audio_start_cb = cb;
}

void usb_audio_source_register_inter_in_ep_cb(usb_ep_callback cb)
{
	iso_in_ep_cb = cb;
}

int usb_audio_source_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR);
}
