/*
 * Human Interface Device (HID) USB class core
 *
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_hid.h>

#ifdef CONFIG_NVRAM_CONFIG
#include <drivers/nvram_config.h>
#endif

#define LOG_LEVEL CONFIG_SYS_LOG_USB_HID_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(core);

#define HID_INT_IN_EP_IDX			0
#define HID_INT_OUT_EP_IDX			1

#ifndef min
#define min(a, b) ((a) < (b)) ? (a) : (b)
#endif

static struct hid_device_info {
	const u8_t *report_desc;
	size_t report_size;
	const struct hid_ops *ops;
} hid_device;

static struct usb_hid_descriptor hid_descriptor = {
	.bLength = sizeof(struct usb_hid_descriptor),
	.bDescriptorType = USB_HID_DESC,
	.bcdHID = sys_cpu_to_le16(USB_1_1),
	.bCountryCode = 0,
	.bNumDescriptors = 1,
	.subdesc[0] = {
		.bDescriptorType = USB_HID_REPORT_DESC,
		.wDescriptorLength = 0,
	},
};

static void usb_hid_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
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
		LOG_DBG("USB device stack work in high-speed mode");
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

static int usb_hid_class_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("class request bRequest: 0x%x, bmRequestType: 0x%x, len: %d",
		    setup->bRequest, setup->bmRequestType, *len);
	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST) {
		switch (setup->bRequest) {
		case HID_GET_REPORT:
			if (hid_device.ops->get_report) {
				return hid_device.ops->get_report(setup, len,
								  data);
			} else {
				LOG_ERR("mandatory request not supported");
				return -EINVAL;
			}
			break;
		default:
			LOG_ERR("unhandled request: 0x%x", setup->bRequest);
			break;
		}
	} else {
		switch (setup->bRequest) {
		case HID_SET_IDLE:
			LOG_DBG("set idel");
			if (hid_device.ops->set_idle) {
				return hid_device.ops->set_idle(setup, len,
								data);
			}
			break;
		case HID_SET_REPORT:
			if (hid_device.ops->set_report == NULL) {
				LOG_ERR("set report not implemented");
				return -EINVAL;
			}
			return hid_device.ops->set_report(setup, len, data);
		default:
			LOG_ERR("unhandled request: 0x%x", setup->bRequest);
			break;
		}
	}

	return -ENOTSUP;
}

static int usb_hid_custom_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("custom request bRequest: 0x%x, bmRequestType: 0x%x, len: %d",
		    setup->bRequest, setup->bmRequestType, *len);
	if (REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_HOST &&
	    REQTYPE_GET_RECIP(setup->bmRequestType) == REQTYPE_RECIP_INTERFACE &&
					setup->bRequest == REQ_GET_DESCRIPTOR) {
		switch (sys_le16_to_cpu(setup->wValue)) {
		case 0x2200:
			LOG_DBG("get report descriptor");
			/* Some buggy system may be pass a larger wLength when
			 * it try read HID report descriptor, although we had
			 * already tell it the right descriptor size.
			 * So truncated wLength if it doesn't match.
			 */
			if (*len != hid_device.report_size) {
				LOG_DBG("len %d doesn't match"
					    "Report Descriptor size", *len);
				*len = min(*len, hid_device.report_size);
			}
			*data = (u8_t *)hid_device.report_desc;
			break;
		case 0x2100:
			LOG_DBG("get HID descriptor");
			*len = min(*len, hid_descriptor.bLength);
			*data = (u8_t *)&hid_descriptor;
			break;
		default:
			return -ENOTSUP;
		}
		return 0;
	}

	return -ENOTSUP;
}

static int usb_hid_vendor_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	LOG_DBG("vendor request bRequest: 0x%x, bmRequestType: 0x%x, len: %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static void usb_hid_inter_in_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_IN ||
	    hid_device.ops->int_in_ready == NULL) {
		return;
	}
	hid_device.ops->int_in_ready();
}

#ifdef CONFIG_HID_INTERRUPT_OUT
static void usb_hid_inter_out_cb(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if (ep_status != USB_DC_EP_DATA_OUT ||
	    hid_device.ops->int_out_ready == NULL) {
		return;
	}
	hid_device.ops->int_out_ready();
}
#endif

/* USB endpoint configuration */
static const struct usb_ep_cfg_data usb_hid_ep_cfg[] = {
	{
		.ep_cb = usb_hid_inter_in_cb,
		.ep_addr = CONFIG_HID_INTERRUPT_IN_EP_ADDR
	},
#ifdef CONFIG_HID_INTERRUPT_OUT
	{
		.ep_cb = usb_hid_inter_out_cb,
		.ep_addr = CONFIG_HID_INTERRUPT_OUT_EP_ADDR
	}
#endif
};

static const struct usb_if_descriptor usb_hid_if = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_INTERFACE_DESC,
	.bInterfaceNumber = CONFIG_USB_HID_DEVICE_IF_NUM,
	.bAlternateSetting = 0,
#ifdef CONFIG_HID_INTERRUPT_OUT
	.bNumEndpoints = 2,
#else
	.bNumEndpoints = 1,
#endif
	.bInterfaceClass = HID_CLASS,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
};

static const struct usb_cfg_data usb_hid_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &usb_hid_if,
	.cb_usb_status = usb_hid_status_cb,
	.interface = {
		.class_handler = usb_hid_class_handle_req,
		.custom_handler = usb_hid_custom_handle_req,
		.vendor_handler = usb_hid_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(usb_hid_ep_cfg),
	.endpoint = usb_hid_ep_cfg,
};

static int usb_hid_fix_dev_sn(void)
{
	int ret;
#ifdef CONFIG_NVRAM_CONFIG
	int read_len;

	u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

	read_len = nvram_config_get(CONFIG_USB_HID_DEVICE_SN_NVRAM, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_HID_DEVICE_SN, strlen(CONFIG_USB_HID_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_HID_DEVICE_SN, strlen(CONFIG_USB_HID_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}


/*
 * API: initialize USB HID device
 */
int usb_hid_init(void)
{
	int ret;

	/* register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_HID_DEVICE_MANUFACTURER, strlen(CONFIG_USB_HID_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_HID_DEVICE_PRODUCT, strlen(CONFIG_USB_HID_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_hid_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* initialize the USB driver with the right configuration */
	ret = usb_set_config(&usb_hid_config);
	if (ret < 0) {
		LOG_ERR("Failed to config USB");
		return ret;
	}

	/* enable USB driver */
	ret = usb_enable(&usb_hid_config);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

/*
 * API: deinitialize USB HID device
 */
int usb_hid_exit(void)
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
 * API: initialize USB HID composite devie
 */
int usb_hid_composite_init(void)
{
	return usb_decice_composite_set_config(&usb_hid_config);
}

void usb_hid_register_device(const u8_t *desc, size_t size,
			     const struct hid_ops *ops)
{
	hid_device.report_desc = desc;
	hid_device.report_size = size;
	hid_device.ops = ops;
	hid_descriptor.subdesc[0].wDescriptorLength = size;
}

int usb_hid_in_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_HID_INTERRUPT_IN_EP_ADDR);
}

#ifdef CONFIG_HID_INTERRUPT_OUT
int usb_hid_out_ep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_HID_INTERRUPT_OUT_EP_ADDR);
}
#endif

int hid_int_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(usb_hid_ep_cfg[HID_INT_IN_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
}

int hid_int_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
#ifdef CONFIG_HID_INTERRUPT_OUT
	return usb_read(usb_hid_ep_cfg[HID_INT_OUT_EP_IDX].ep_addr, data,
			 data_len, bytes_ret);
#else
	return -EINVAL;
#endif
}
