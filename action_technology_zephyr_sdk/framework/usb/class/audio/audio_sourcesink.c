/*
 * USB Audio Class -- Audio Source Sink driver
 *
 * Copyright (C) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NOTE: Implement the interfaces which support audio sourcesink and
 * composite device(HID + UAC), support UAC version 1.0 and works in
 * both Full-speed and High-speed modes.
 *
 * Function:
 * # support multi sample rate and multi channels.
 * # add buffer and timing statistics.
 * # more flexible configuration.
 * # support feature unit.
 * # support high-speed.
 * # support version 2.0.
 */
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_audio.h>
#include "audio_sourcesink_desc.h"

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SOURCESINK_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(audio_sourcesink);

#define CUR_DEV_VOLUME_VAL	0x1E00

#define GET_SAMPLERATE		0xA2
#define SET_ENDPOINT_CONTROL	0x22
#define SAMPLING_FREQ_CONTROL	0x0100

#define FEATURE_UNIT1_ID	0x09
#define FEATURE_UNIT_INDEX1	0x0901

#define AUDIO_STREAM_INTER2	2
#define AUDIO_STREAM_INTER3	3

static u16_t get_volume_val;

static int ch0_cur_vol_val = CUR_DEV_VOLUME_VAL;
static int ch1_cur_vol_val = CUR_DEV_VOLUME_VAL;
static int ch2_cur_vol_val = CUR_DEV_VOLUME_VAL;
static u32_t g_cur_sample_rate = CONFIG_USB_AUDIO_DEVICE_SINK_SAM_FREQ_DOWNLOAD;

static usb_ep_callback iso_in_ep_cb;
static usb_ep_callback iso_out_ep_cb;
static usb_audio_start audio_start_cb;
static usb_audio_pm audio_pm_cb;

static usb_audio_volume_sync vol_ctrl_sync_cb;

static bool audio_streaming_enabled;

static const struct usb_if_descriptor usb_audio_if  = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_INTERFACE_DESC,
	.bInterfaceNumber = CONFIG_USB_AUDIO_DEVICE_IF_NUM,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
};

static void usb_audio_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	static u8_t alt_setting, iface;

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_INTERFACE:
		/* iface: the higher byte, alt_setting: the lower byte */
		iface = *(u16_t *)param >> 8;
		alt_setting = *(u16_t *)param & 0xff;
		LOG_DBG("Set_Int: %d, alt_setting: %d", iface, alt_setting);
		switch (iface) {
		case AUDIO_STREAM_INTER2:
			break;

		case AUDIO_STREAM_INTER3:
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
			LOG_WRN("Unavailable interface number");
			break;
		}
		break;

	case USB_DC_ALTSETTING:
		/* iface: the higher byte, alt_setting: the lower byte */
		iface = *(u16_t *)param >> 8;
		*(u16_t *)param = (iface << 8) + alt_setting;
		LOG_DBG("Get_Int: %d, alt_setting: %d", iface, alt_setting);
		break;

	case USB_DC_ERROR:
		LOG_DBG("USB device error");
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
		LOG_DBG("USB configuration done");
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
		if (audio_pm_cb) {
			audio_pm_cb(true);
		}
		LOG_DBG("USB device suspended");
		break;

	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		if (audio_pm_cb) {
			audio_pm_cb(false);
		}
		break;

	case USB_DC_HIGHSPEED:
		LOG_DBG("USB device stack work in high-speed mode");
		break;

	case USB_DC_UNKNOWN:
		break;

	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

static inline int handle_set_sample_rate(struct usb_setup_packet *setup,
				s32_t *len, u8_t *buf)
{
	switch (setup->bRequest) {
	case UAC_SET_CUR:
	case UAC_SET_MIN:
	case UAC_SET_MAX:
	case UAC_SET_RES:
		*len = 3;
		LOG_DBG("buf[0]: 0x%02x", buf[0]);
		LOG_DBG("buf[1]: 0x%02x", buf[1]);
		LOG_DBG("buf[2]: 0x%02x", buf[2]);
		g_cur_sample_rate = (buf[2] << 16) | (buf[1] << 8) | buf[0];
		LOG_DBG("g_cur_sample_rate:%d ", g_cur_sample_rate);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static inline int handle_get_sample_rate(struct usb_setup_packet *setup,
				s32_t *len, u8_t *buf)
{
	switch (setup->bRequest) {
	case UAC_GET_CUR:
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
		buf[0] = (u8_t)CONFIG_USB_AUDIO_DEVICE_SINK_SAM_FREQ_DOWNLOAD;
		buf[1] = (u8_t)(CONFIG_USB_AUDIO_DEVICE_SINK_SAM_FREQ_DOWNLOAD >> 8);
		buf[2] = (u8_t)(CONFIG_USB_AUDIO_DEVICE_SINK_SAM_FREQ_DOWNLOAD >> 16);
		*len = 3;
		return 0;
	default:
		break;
	}
	return -ENOTSUP;
}

int usb_audio_source_inep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR);
}

int usb_audio_sink_outep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_DEVICE_SINK_OUT_EP_ADDR);
}

static int usb_audio_class_handle_req(struct usb_setup_packet *psetup,
	s32_t *len, u8_t **data)
{

	int ret = -ENOTSUP;
	u8_t *temp_data = *data;

	switch (psetup->bmRequestType) {
	case SPECIFIC_REQUEST_OUT:
	if (psetup->wIndex == FEATURE_UNIT_INDEX1) {
		if (psetup->bRequest == UAC_SET_CUR) {
			/* process cmd: set mute and unmute */
			if (psetup->wValue == ((MUTE_CONTROL << 8) | MAIN_CHANNEL_NUMBER0)) {
				LOG_DBG("mute_ch0:%d %d\n", ch0_cur_vol_val, temp_data[0]);
				if (ch0_cur_vol_val != temp_data[0]) {
					ch0_cur_vol_val = temp_data[0];
					if (vol_ctrl_sync_cb) {
						if (ch0_cur_vol_val == 1) {
							vol_ctrl_sync_cb(USOUND_SYNC_HOST_MUTE, LOW_BYTE(psetup->wValue), &ch0_cur_vol_val);
						} else {
							vol_ctrl_sync_cb(USOUND_SYNC_HOST_UNMUTE, LOW_BYTE(psetup->wValue), &ch0_cur_vol_val);
						}
					}
				}
			}
			/* process cmd: set current volume */
			else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER1)) {
				ch1_cur_vol_val = (temp_data[1] << 8) | (temp_data[0]);
				LOG_DBG("host set volume1:0x%04x", ch1_cur_vol_val);
				if (ch1_cur_vol_val == 0x8000) {
					ch1_cur_vol_val = MINIMUM_VOLUME;
				}
				if (ch1_cur_vol_val == 0) {
					ch1_cur_vol_val = 65536;
				}
				if (vol_ctrl_sync_cb) {
					vol_ctrl_sync_cb(USOUND_SYNC_HOST_VOL_TYPE, LOW_BYTE(psetup->wValue), &ch1_cur_vol_val);
				}
			}
		}
	}
	ret = 0;
		break;

	case SPECIFIC_REQUEST_IN:
	*data = (u8_t *) &get_volume_val;
	*len = VOLUME_LENGTH;
	if (psetup->wIndex == FEATURE_UNIT_INDEX1) {
		*len = VOLUME_LENGTH;
		if (psetup->bRequest == UAC_GET_CUR) {
			if (psetup->wValue == ((MUTE_CONTROL << 8) | MAIN_CHANNEL_NUMBER0)) {
				*data = (u8_t *) &ch0_cur_vol_val;
				*len = MUTE_LENGTH;
			} else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER1)) {
				LOG_DBG("get_volume1");
				*data = (u8_t *) &ch1_cur_vol_val;
			} else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER2)) {
				LOG_DBG("get_volume2");
				*data = (u8_t *) &ch2_cur_vol_val;
			}
		} else if (psetup->bRequest == UAC_GET_MIN) {
			get_volume_val = MINIMUM_VOLUME;
		} else if (psetup->bRequest == UAC_GET_MAX) {
			get_volume_val = MAXIMUM_VOLUME;
		} else if (psetup->bRequest == UAC_GET_RES) {
			get_volume_val = RESOTION_VOLUME;
		}
	}
	ret = 0;
		break;

	case SET_ENDPOINT_CONTROL:
	if (psetup->wValue == SAMPLING_FREQ_CONTROL) {
		ret = handle_set_sample_rate(psetup, len, *data);
	}
		break;

	case GET_SAMPLERATE:
	if (psetup->wValue == SAMPLING_FREQ_CONTROL) {
		ret = handle_get_sample_rate(psetup, len, *data);
	}
		break;
	default:
		break;
	}
	return ret;
}

/*
 * NOTICE: In composite device case, this function will never be called,
 * the reason is same as class request as above.
 */
static int usb_audio_custom_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("custom request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static int usb_audio_vendor_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("vendor request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static void usb_audio_isoc_in_cb(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{

	LOG_DBG("**isoc_in_cb!**");
	if (iso_in_ep_cb) {
		iso_in_ep_cb(ep, cb_status);
	}
}

static void usb_audio_isoc_out_cb(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{
	LOG_DBG("**isoc_out_cb!**");
	if (iso_out_ep_cb) {
		iso_out_ep_cb(ep, cb_status);
	}
}

/* USB endpoint configuration */
static const struct usb_ep_cfg_data usb_audio_ep_cfg[] = {
	{
		.ep_cb = usb_audio_isoc_out_cb,
		.ep_addr = CONFIG_USB_AUDIO_DEVICE_SINK_OUT_EP_ADDR,
	},

	{
		.ep_cb = usb_audio_isoc_in_cb,
		.ep_addr = CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR,
	}
};

static const struct usb_cfg_data usb_audio_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &usb_audio_if,
	.cb_usb_status = usb_audio_status_cb,
	.interface = {
		.class_handler = usb_audio_class_handle_req,
		.custom_handler = usb_audio_custom_handle_req,
		.vendor_handler = usb_audio_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(usb_audio_ep_cfg),
	.endpoint = usb_audio_ep_cfg,
};

static int usb_audio_fix_dev_sn(void)
{
	int ret;
#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_AUDIO_SOURCESINK_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SOURCESINK_SN, strlen(CONFIG_USB_AUDIO_SOURCESINK_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SOURCESINK_SN, strlen(CONFIG_USB_AUDIO_SOURCESINK_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

/*
 * API: initialize USB audio dev
 */
int usb_audio_device_init(void)
{
	int ret;

	/* Register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_AUDIO_SOURCESINK_MANUFACTURER, strlen(CONFIG_USB_AUDIO_SOURCESINK_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_AUDIO_SOURCESINK_PRODUCT, strlen(CONFIG_USB_AUDIO_SOURCESINK_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_audio_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* Register device descriptors */
	usb_device_register_descriptors(usb_audio_sourcesink_fs_desc, usb_audio_sourcesink_fs_desc);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&usb_audio_config);
	if (ret < 0) {
		LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&usb_audio_config);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

/*
 * API: deinitialize USB audio dev
 */
int usb_audio_device_exit(void)
{
	int ret;

	ret = usb_disable();
	if (ret) {
		LOG_ERR("Failed to disable USB: %d", ret);
		return ret;
	}
	usb_deconfig();
	return 0;
}

/*
 * API: Initialize USB audio composite devie
 */
int usb_audio_composite_dev_init(void)
{
	return usb_decice_composite_set_config(&usb_audio_config);
}

void usb_audio_source_register_pm_cb(usb_audio_pm cb)
{
	audio_pm_cb = cb;
}

void usb_audio_device_register_start_cb(usb_audio_start cb)
{
	audio_start_cb = cb;
}

void usb_audio_device_register_inter_in_ep_cb(usb_ep_callback cb)
{
	iso_in_ep_cb = cb;
}

void usb_audio_device_register_inter_out_ep_cb(usb_ep_callback cb)
{
	iso_out_ep_cb = cb;
}

void usb_audio_device_register_volume_sync_cb(usb_audio_volume_sync cb)
{
	vol_ctrl_sync_cb = cb;
}

int usb_audio_device_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(CONFIG_USB_AUDIO_DEVICE_SOURCE_IN_EP_ADDR,
				data, data_len, bytes_ret);
}

int usb_audio_device_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_read(CONFIG_USB_AUDIO_DEVICE_SINK_OUT_EP_ADDR,
				data, data_len, bytes_ret);
}
