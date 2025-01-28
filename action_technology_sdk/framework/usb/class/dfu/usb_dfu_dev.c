/**
 * @brief DFU class driver
 *
 * USB DFU device class driver
 *
 */
#include <kernel.h>
#include <init.h>
#include <string.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

#include "usb_dfu_descriptor.h"

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#define LOG_LEVEL CONFIG_SYS_LOG_USB_DFU_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dfu_dev);

static void usb_dfu_status_cb(enum usb_dc_status_code status, u8_t *param)
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

static int usb_dfu_class_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("class request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return 0;
}

static int usb_dfu_custom_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("custom request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return -1;
}

static int usb_dfu_vendor_handle_req(struct usb_setup_packet *setup, s32_t *len,
					u8_t **data)
{
	LOG_DBG("vendor request bRequest: 0x%x bmRequestType: 0x%x len: %d",
				setup->bRequest, setup->bmRequestType, *len);
	return 0;
}


static const struct usb_cfg_data usb_dfu_config = {
	.usb_device_description = NULL,
	.cb_usb_status = usb_dfu_status_cb,
	.interface = {
		.class_handler  = usb_dfu_class_handle_req,
		.custom_handler = usb_dfu_custom_handle_req,
		.vendor_handler = usb_dfu_vendor_handle_req,
	},
	.num_endpoints = 0,
	.endpoint = NULL,
};

static int usb_dfu_fix_dev_sn(void)
{
	int ret;

#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_DFU_DEVICE_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_DFU_DEVICE_SN, strlen(CONFIG_USB_DFU_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_DFU_DEVICE_SN, strlen(CONFIG_USB_DFU_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

/*
 * API: initialize USB STUB
 */
int usb_dfu_init(struct device *dev)
{
	int ret;

	/* register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_DFU_DEVICE_MANUFACTURER, strlen(CONFIG_USB_DFU_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_DFU_DEVICE_PRODUCT, strlen(CONFIG_USB_DFU_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_dfu_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* register device descriptor */
	usb_device_register_descriptors(usb_dfu_fs_descriptor, usb_dfu_hs_descriptor);

	/* initialize the USB driver with the right configuration */
	ret = usb_set_config(&usb_dfu_config);
	if (ret < 0) {
		return ret;
	}

	/* enable USB driver */
	ret = usb_enable(&usb_dfu_config);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/*
 * API: deinitialize USB Stub
 */
int usb_dfu_exit(void)
{
	int ret;
	ret = usb_disable();
	if (ret) {
		LOG_ERR("failed to disable USB Stub device");
		return ret;
	}
	usb_deconfig();
	return 0;
}
