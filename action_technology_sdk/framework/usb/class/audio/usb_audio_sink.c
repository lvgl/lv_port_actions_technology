/**
 * @file
 * @brief USB audio device class driver.
 *
 * driver for USB audio sink device.
 */

#include <kernel.h>
#include <init.h>
#include <usb/class/usb_audio.h>
#include "usb_audio_sink_desc.h"

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#define LOG_LEVEL CONFIG_SYS_LOG_USB_SINK_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_audio_sink);

#define CUR_DEV_VOLUME_VAL	0x1E00
static int g_cur_volume = CUR_DEV_VOLUME_VAL;
static int g_min_volume = MINIMUM_VOLUME;
static int g_max_volume = MAXIMUM_VOLUME;
static int g_res_volume = RESOTION_VOLUME;

static int sync_vol_dat;
static usb_ep_callback iso_out_ep_cb;
static usb_audio_volume_sync vol_ctrl_sync_cb;

static void usb_audio_class_set_mute_vol(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	LOG_DBG("*data[0]: %d", *data[0]);
	sync_vol_dat = *data[0];
	if (vol_ctrl_sync_cb) {
		vol_ctrl_sync_cb(USOUND_SYNC_HOST_MUTE, LOW_BYTE(setup->wValue), &sync_vol_dat);
	}
}

static void usb_audio_class_set_cur_vol(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	u8_t *temp_data = *data;
	sync_vol_dat  = (temp_data[1] << 8) | (temp_data[0]);
	LOG_DBG("host set channel_%d volume: 0x%04x", LOW_BYTE(setup->wValue), sync_vol_dat);
	if (sync_vol_dat == 0x8000) {
		sync_vol_dat = MINIMUM_VOLUME;
	}else if (sync_vol_dat == 0) {
		sync_vol_dat = 65536;
	}
	if (vol_ctrl_sync_cb) {
		vol_ctrl_sync_cb(USOUND_SYNC_HOST_VOL_TYPE, LOW_BYTE(setup->wValue), &sync_vol_dat);
	}
}

static int usb_audio_class_handle_req(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	switch (setup->bmRequestType) {
		case SPECIFIC_REQUEST_IN:
			switch (setup->bRequest) {
				case UAC_GET_CUR:
					if (setup->wIndex == FEATURE_UNIT_INDEX1) {
						if(HIGH_BYTE(setup->wValue) == MUTE_CONTROL){
							*data = (u8_t *)&sync_vol_dat;
							*len  = MUTE_LENGTH;
						}else if(HIGH_BYTE(setup->wValue) == VOLUME_CONTROL) {
							LOG_DBG("get_cur_vol_value_of_channel_%d", LOW_BYTE(setup->wValue));
							/* the current value of each channel is the same. */
							*data = (u8_t *) &g_cur_volume;
							*len  = VOLUME_LENGTH;
						}
					}
				break;

				case UAC_GET_MIN:
					if (setup->wIndex == FEATURE_UNIT_INDEX1) {
						if (HIGH_BYTE(setup->wValue) == VOLUME_CONTROL) {
							LOG_DBG("get_min_vol_value_of_channel_%d", LOW_BYTE(setup->wValue));
							/* the minimum value of each channel is the same. */
							*data = (u8_t *) &g_min_volume;
							*len  = VOLUME_LENGTH;
						}
					}
				break;

				case UAC_GET_MAX:
				if (setup->wIndex == FEATURE_UNIT_INDEX1) {
						if (HIGH_BYTE(setup->wValue) == VOLUME_CONTROL) {
							LOG_DBG("get_max_vol_value_of_channel_%d", LOW_BYTE(setup->wValue));
							/* the maximum value of each channel is the same. */
							*data = (u8_t *) &g_max_volume;
							*len  = VOLUME_LENGTH;
						}
					}
				break;

				case UAC_GET_RES:
					if (setup->wIndex == FEATURE_UNIT_INDEX1) {
						if (HIGH_BYTE(setup->wValue) == VOLUME_CONTROL) {
							LOG_DBG("get_resolution_value_of_channel_%d", LOW_BYTE(setup->wValue));
							/* the resolution value of each channel is the same. */
							*data = (u8_t *) &g_res_volume;
							*len  = VOLUME_LENGTH;
						}
					}
				break;
			}
		break;

		case SPECIFIC_REQUEST_OUT:
			switch (setup->bRequest) {
				case UAC_SET_CUR:
					if (setup->wIndex == FEATURE_UNIT_INDEX1) {
						if (HIGH_BYTE(setup->wValue) == MUTE_CONTROL) {
								usb_audio_class_set_mute_vol(setup, NULL, data);
						} else if (HIGH_BYTE(setup->wValue) == VOLUME_CONTROL) {
								usb_audio_class_set_cur_vol(setup, NULL, data);
						}
					}
				break;
			}
		break;
   }
	return 0;
}

static int usb_audio_custom_handle_req(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	LOG_DBG("custom request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static int usb_audio_vendor_handle_req(struct usb_setup_packet *setup, s32_t *len, u8_t **data)
{
	LOG_DBG("vendor request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static void usb_audio_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	switch (status) {
	case USB_DC_INTERFACE:
		LOG_DBG("usb device set interface");
		break;
	case USB_DC_ERROR:
		LOG_DBG("usb device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("usb device reset detected");
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("usb device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("usb device configured");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("usb device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("usb device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("usb device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("usb unknown state");
		break;
	}
}

int usb_audio_sink_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR);
}

static void usb_audio_isoc_out(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{
	LOG_DBG("audio_isoc_out");
	if (iso_out_ep_cb) {
		iso_out_ep_cb(ep, cb_status);
	}
}

static struct usb_ep_cfg_data usb_audio_ep_cfg[] = {
	{
		.ep_cb = usb_audio_isoc_out,
		.ep_addr = CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	},
};

static struct usb_cfg_data usb_audio_config = {
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


/*
 * API: initialize USB audio sound
 */
static int usb_audio_sink_fix_dev_sn(void)
{
	int ret;
#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_AUDIO_SINK_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SINK_SN, strlen(CONFIG_USB_AUDIO_SINK_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_SINK_SN, strlen(CONFIG_USB_AUDIO_SINK_SN));
	if (ret) {
			return ret;
	}
#endif
	return 0;
}


/*
 * API: initialize USB audio sink
 */
int usb_audio_sink_init(struct device *dev)
{
	int ret;

	/* register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_AUDIO_SINK_MANUFACTURER, strlen(CONFIG_USB_AUDIO_SINK_MANUFACTURER));
	if (ret) {
			return ret;
	}

	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_AUDIO_SINK_PRODUCT, strlen(CONFIG_USB_AUDIO_SINK_PRODUCT));
	if (ret) {
			return ret;
	}

	ret = usb_audio_sink_fix_dev_sn();
	if (ret) {
			return ret;
	}

	/* register device descriptor */
	usb_device_register_descriptors(usb_audio_sink_fs_descriptor, usb_audio_sink_hs_descriptor);

	/* initialize the usb driver with the right configuration */
	ret = usb_set_config(&usb_audio_config);
	if (ret < 0) {
			LOG_ERR("Failed to config USB");
			return ret;
	}

	/* enable usb driver */
	ret = usb_enable(&usb_audio_config);
	if (ret < 0) {
			LOG_ERR("Failed to enable USB");
			return ret;
	}

	return 0;
}

/*
 * API: deinitialize USB audio sink
 */
int usb_audio_sink_deinit(void)
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

int usb_audio_sink_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_read(CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
				data, data_len, bytes_ret);
}

void usb_audio_sink_register_inter_out_ep_cb(usb_ep_callback cb)
{
	iso_out_ep_cb = cb;
}

void usb_audio_sink_register_volume_sync_cb(usb_audio_volume_sync cb)
{
	vol_ctrl_sync_cb = cb;
}
