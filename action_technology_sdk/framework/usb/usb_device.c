/*
 *  LPCUSB, an USB device driver for LPC microcontrollers
 *  Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)
 *  Copyright (c) 2016 Intel Corporation
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief USB device core layer
 *
 * This module handles control transfer handler, standard request handler and
 * USB Interface for customer application.
 *
 * Control transfers handler is normally installed on the
 * endpoint 0 callback.
 *
 * Control transfers can be of the following type:
 * 0 Standard;
 * 1 Class;
 * 2 Vendor;
 * 3 Reserved.
 *
 * A callback can be installed for each of these control transfers using
 * usb_register_request_handler.
 * When an OUT request arrives, data is collected in the data store provided
 * with the usb_register_request_handler call. When the transfer is done, the
 * callback is called.
 * When an IN request arrives, the callback is called immediately to either
 * put the control transfer data in the data store, or to get a pointer to
 * control transfer data. The data is then packetized and sent to the host.
 *
 * Standard request handler handles the 'chapter 9' processing, specifically
 * the standard device requests in table 9-3 from the universal serial bus
 * specification revision 2.0
 */

#include <errno.h>
#include <stddef.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <init.h>
#if defined(USB_VUSB_EN_GPIO)
#include <gpio.h>
#endif

#include <usb/usb_device.h>
#include <drivers/usb/usb_dc.h>
#include <drivers/usb/usb.h>
#include <usb/usbstruct.h>
#include <usb/usb_common.h>
#include <usb/bos.h>
#include <string.h>

#include <logging/log.h>
#define	LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
LOG_MODULE_REGISTER(usb_device);

#define MAX_DESC_HANDLERS           4 /** Device, interface, endpoint, other */

/* general descriptor field offsets */
#define DESC_bLength                0 /** Length offset */
#define DESC_bDescriptorType        1 /** Descriptor type offset */

/* config descriptor field offsets */
#define CONF_DESC_wTotalLength      2 /** Total length offset */
#define CONF_DESC_bConfigurationValue 5 /** Configuration value offset */
#define CONF_DESC_bmAttributes      7 /** configuration characteristics */

/* interface descriptor field offsets */
#define INTF_DESC_bInterfaceNumber  2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting 3 /** Alternate setting offset */

/* endpoint descriptor field offsets */
#define ENDP_DESC_bEndpointAddress  2 /** Endpoint address offset */
#define ENDP_DESC_bmAttributes      3 /** Bulk or interrupt? */
#define ENDP_DESC_wMaxPacketSize    4 /** Maximum packet size offset */

#define MAX_NUM_REQ_HANDLERS        4
#define MAX_STD_REQ_MSG_SIZE        8

#define MAX_NUM_TRANSFERS           4 /** Max number of parallel transfers */

/* Default USB control EP, always 0 and 0x80 */
#define USB_CONTROL_OUT_EP0         0
#define USB_CONTROL_IN_EP0          0x80

/* bound the USB descriptor structs */
static const struct usb_cfg_data *usb_cfg_data_buf[CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM];

struct usb_transfer_data {
	/** endpoint associated to the transfer */
	u8_t ep;
	/** Transfer status */
	int status;
	/** Transfer read/write buffer */
	u8_t *buffer;
	/** Transfer buffer size */
	size_t bsize;
	/** Transferred size */
	size_t tsize;
	/** Transfer callback */
	usb_transfer_callback cb;
	/** Transfer caller private data */
	void *priv;
	/** Transfer synchronization semaphore */
	struct k_sem sem;
	/** Transfer read/write work */
	struct k_work work;
	/** Transfer flags */
	unsigned int flags;
};

#ifdef CONFIG_USB_DEVICE_TRANSFER
static void usb_transfer_work(struct k_work *item);
#endif

static struct usb_dev_priv {
	/** Setup packet */
	struct usb_setup_packet setup;
	/** Pointer to data buffer */
	u8_t *data_buf;
	/** Eemaining bytes in buffer */
	s32_t data_buf_residue;
	/** Total length of control transfer */
	s32_t data_buf_len;
	/** Installed custom request handler */
	usb_request_handler custom_req_handler;
	/** USB stack status clalback */
	usb_status_callback status_callback;
	/** Pointer to registered descriptors */
	const u8_t *descriptors;
	/** Pointer to registered full-speed descriptors */
	const u8_t *fs_descriptors;
	/** Pointer to registered high-speed descriptors */
	const u8_t *hs_descriptors;
	/** Array of installed request handler callbacks */
	usb_request_handler req_handlers[MAX_NUM_REQ_HANDLERS];
	/* Buffer used for storing standard, class and vendor request data */
	u8_t req_data[CONFIG_USB_REQUEST_BUFFER_SIZE];
	/** Variable to check whether the usb has been enabled */
	bool enabled;
	/** Currently selected configuration */
	u8_t configuration;
	/** Remote wakeup feature status */
	bool remote_wakeup;
	/** Zero-length packet for control-transfer */
	bool zero;
	/*
	 * HACK: Add to let the upper layer take care of control transfer
	 * to support extend function.
	 *
	 * Restriction:
	 * 1. data length = setup->wLength
	 * 2. data length <= MAX_PACKET_SIZE0.
	 */
	bool upper_ctrl;
#ifdef CONFIG_USB_DEVICE_TRANSFER
	/** Transfer list */
	struct usb_transfer_data transfer[MAX_NUM_TRANSFERS];
#endif
	u8_t unconfigured;	/* 1: if set configuration 0 */
} usb_dev;

/*
 * @brief print the contents of a setup packet
 *
 * @param [in] setup The setup packet
 *
 */
static void usb_print_setup(struct usb_setup_packet *setup)
{
	/* avoid compiler warning if LOG_DBG is not defined */
	ARG_UNUSED(setup);

	LOG_DBG("SETUP\n");
	LOG_DBG("%x %x %x %x %x\n",
	    setup->bmRequestType,
	    setup->bRequest,
	    sys_le16_to_cpu(setup->wValue),
	    sys_le16_to_cpu(setup->wIndex),
	    sys_le16_to_cpu(setup->wLength));
}

/*
 * @brief handle a request by calling one of the installed request handlers
 *
 * Local function to handle a request by calling one of the installed request
 * handlers. In case of data going from host to device, the data is at *ppbData.
 * In case of data going from device to host, the handler can either choose to
 * write its data at *ppbData or update the data pointer.
 *
 * @param [in]     setup The setup packet
 * @param [in,out] len   Pointer to data length
 * @param [in,out] data  Data buffer
 *
 * @return true if the request was handles successfully
 */
static bool usb_handle_request(struct usb_setup_packet *setup,
		s32_t *len, u8_t **data)
{
	u32_t type = REQTYPE_GET_TYPE(setup->bmRequestType);
	usb_request_handler handler = usb_dev.req_handlers[type];

	LOG_DBG("** %d **\n", type);

	if (type >= MAX_NUM_REQ_HANDLERS) {
		LOG_DBG("Error Incorrect iType %d\n", type);
		return false;
	}

	if (handler == NULL) {
		LOG_DBG("No handler for reqtype %d\n", type);
		return false;
	}

	if ((*handler)(setup, len, data) < 0) {
		LOG_DBG("Handler Error %d\n", type);
		usb_print_setup(setup);
		return false;
	}

	return true;
}

/*
 * @brief send next chunk of data (possibly 0 bytes) to host
 *
 * @return N/A
 */
static void usb_data_to_host(void)
{
	u32_t chunk = MIN(MAX_PACKET_SIZE0, usb_dev.data_buf_residue);

	/*Always EP0 for control*/
	usb_dc_ep_write(0x80, usb_dev.data_buf, chunk, &chunk);
	usb_dev.data_buf += chunk;
	usb_dev.data_buf_residue -= chunk;
}

/*
 * @brief handle IN/OUT transfers on EP0
 *
 * @param [in] ep        Endpoint address
 * @param [in] ep_status Endpoint status
 *
 * @return N/A
 */
static void usb_handle_control_transfer(u8_t ep,
		enum usb_dc_ep_cb_status_code ep_status)
{
	u32_t chunk = 0;
	struct usb_setup_packet *setup = &usb_dev.setup;

	LOG_DBG("ep %x, status %x\n", ep, ep_status);

	if (ep == USB_CONTROL_OUT_EP0 && ep_status == USB_DC_EP_SETUP) {
		u16_t length;

		/*
		 * OUT transfer, Setup packet,
		 * reset request message state machine
		 */
		if (usb_dc_ep_read(ep,
		    (u8_t *)setup, sizeof(*setup), NULL) < 0) {
			LOG_DBG("Read Setup Packet failed\n");
			usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
			return;
		}

		length = sys_le16_to_cpu(setup->wLength);
		if (length > CONFIG_USB_REQUEST_BUFFER_SIZE) {
			if (REQTYPE_GET_DIR(setup->bmRequestType)
			    == REQTYPE_DIR_TO_HOST) {
				LOG_DBG("length 0x%x(0x%x) too small\n",
					    length,
					    CONFIG_USB_REQUEST_BUFFER_SIZE);
			} else {
				LOG_ERR("Request buffer too small\n");
				usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
				usb_dc_ep_set_stall(USB_CONTROL_OUT_EP0);
				return;
			}
		}

		usb_dev.data_buf = usb_dev.req_data;
		usb_dev.data_buf_residue = length;
		usb_dev.data_buf_len = length;

		if (length &&
		    REQTYPE_GET_DIR(setup->bmRequestType)
		    == REQTYPE_DIR_TO_DEVICE) {
			return;
		}

		/* Ask installed handler to process request */
		if (!usb_handle_request(setup,
		    &usb_dev.data_buf_len, &usb_dev.data_buf)) {
			LOG_DBG("usb_handle_request failed\n");
			usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
			return;
		}

		if ((usb_dev.data_buf_len == 0) &&
			(length != 0) &&
			(REQTYPE_GET_DIR(setup->bmRequestType)
			== REQTYPE_DIR_TO_HOST)) {
			usb_dev.upper_ctrl = true;
			return;
		}

		/* Send smallest of requested and offered length */
		usb_dev.data_buf_residue = MIN(usb_dev.data_buf_len, length);

		/* Check for zero-length packet */
		if ((usb_dev.data_buf_residue < length) &&
			(usb_dev.data_buf_residue % MAX_PACKET_SIZE0 == 0)) {
			usb_dev.zero = true;
		} else {
			usb_dev.zero = false;
		}

		/* Send first part (possibly a zero-length status message) */
		usb_data_to_host();
	} else if (ep == USB_CONTROL_OUT_EP0) {
		/* OUT transfer, data or status packets */
		if (usb_dev.data_buf_residue <= 0) {
			/* absorb zero-length status message */
			if (usb_dc_ep_read(USB_CONTROL_OUT_EP0,
			    usb_dev.data_buf, 0, &chunk) < 0) {
				LOG_DBG("Read DATA Packet failed\n");
				usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
			}
			return;
		}

		if (usb_dc_ep_read(USB_CONTROL_OUT_EP0,
		    usb_dev.data_buf,
		    usb_dev.data_buf_residue, &chunk) < 0) {
			LOG_DBG("Read DATA Packet failed\n");
			usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
			usb_dc_ep_set_stall(USB_CONTROL_OUT_EP0);
			return;
		}

		usb_dev.data_buf += chunk;
		usb_dev.data_buf_residue -= chunk;
		if (usb_dev.data_buf_residue == 0) {
			/* Received all, send data to handler */
			usb_dev.data_buf = usb_dev.req_data;
			if (!usb_handle_request(setup,
			    &usb_dev.data_buf_len,	&usb_dev.data_buf)) {
				LOG_DBG("usb_handle_request1 failed\n");
				usb_dc_ep_set_stall(USB_CONTROL_IN_EP0);
				return;
			}

			/*Send status to host*/
			LOG_DBG(">> usb_data_to_host(2)\n");
			usb_data_to_host();
		}
	} else if (ep == USB_CONTROL_IN_EP0) {
		/* Avoid redundant data transfer */
		if (usb_dev.upper_ctrl) {
			usb_dev.upper_ctrl = false;
			usb_dev.data_buf_residue = 0;
			return;
		}

		if (usb_dev.zero && usb_dev.data_buf_residue == 0) {
			LOG_DBG("send zero-length packet\n");
			usb_data_to_host();
			usb_dev.zero = false;
		}

		/* Send more data if available */
		if (usb_dev.data_buf_residue != 0) {
			usb_data_to_host();
		}
	} else {
		__ASSERT_NO_MSG(false);
	}
}


/*
 * @brief register a callback for handling requests
 *
 * @param [in] type       Type of request, e.g. REQTYPE_TYPE_STANDARD
 * @param [in] handler    Callback function pointer
 *
 * @return N/A
 */
static void usb_register_request_handler(s32_t type,
					 usb_request_handler handler)
{
	usb_dev.req_handlers[type] = handler;
}

/*
 * @brief register full-speed/high-speed USB descriptors
 *
 * @return N/A
 */
void usb_device_register_descriptors(const u8_t *usb_fs_descriptors,
				     const u8_t *usb_hs_descriptors)
{
	usb_dev.fs_descriptors = usb_fs_descriptors;
	usb_dev.hs_descriptors = usb_hs_descriptors;
}

#define STRING_LENGTH(s)	(strlen(s) * 2)
static u8_t str_desc_buf[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN*2 + 2];
static u8_t manufacturer_str_ptr[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
static u8_t product_str_ptr[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
static u8_t dev_sn_str_ptr[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];

int usb_device_register_string_descriptor(enum usb_device_str_desc type,
					  u8_t *str_dat, u8_t str_len)
{
	u8_t *str_array[3] = {"manufacture_str", "product_str", "dev_sn_str"};

	if (str_len > CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN) {
		int index = type - 1;
		if(index < 0){
			index = 0;
		}
		LOG_ERR("%s : %s is more than %d bytes!\n", str_array[index],
			    str_dat, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
		return -EINVAL;
	}

	switch (type) {
	case MANUFACTURE_STR_DESC:
		memcpy(manufacturer_str_ptr, str_dat, strlen(str_dat));
		break;

	case PRODUCT_STR_DESC:
		memcpy(product_str_ptr, str_dat, strlen(str_dat));
		break;

	case DEV_SN_DESC:
		memcpy(dev_sn_str_ptr, str_dat, strlen(str_dat));
		break;

	default:
		LOG_ERR("Unknown_Type\n");
		return -EINVAL;
	}

	return 0;
}

static void change_strdesc_to_unicode(u8_t *str_desc, u8_t *unicode_buf)
{
	for (u8_t i = 2, j = 0; i < unicode_buf[0]; i++) {
		if (i%2 == 0) {
			unicode_buf[i] = str_desc[j++];
		} else {
			unicode_buf[i] = 0;
		}
	}
}

static void usb_process_str_des(u8_t *des_ptr)
{
	memset(str_desc_buf, 0, sizeof(str_desc_buf));
	str_desc_buf[0] = STRING_LENGTH(des_ptr) + 2;
	str_desc_buf[1] = USB_STRING_DESC;

	if(str_desc_buf[0] > sizeof(str_desc_buf)) {
		str_desc_buf[0] = sizeof(str_desc_buf);
	}

	change_strdesc_to_unicode(des_ptr, str_desc_buf);
}

static inline bool device_qual(u8_t **data)
{
	struct usb_qualifier_descriptor *qual =
		(struct usb_qualifier_descriptor *)*data;
	struct usb_device_descriptor *desc =
		(struct usb_device_descriptor *)(usb_dev.descriptors);

	if ((usb_dc_maxspeed() < USB_SPEED_HIGH) ||
	    (usb_dc_speed() >= USB_SPEED_SUPER)) {
		return false;
	}

	qual->bLength = sizeof(*qual);
	qual->bDescriptorType = DESC_DEVICE_QUALIFIER;
	/* POLICY: same bcdUSB and device type info at both speeds */
	qual->bcdUSB = sys_cpu_to_le16(USB_2_0),

	qual->bDeviceClass = desc->bDeviceClass;
	qual->bDeviceSubClass = desc->bDeviceSubClass;
	qual->bDeviceProtocol = desc->bDeviceProtocol;

	/* ASSUME same EP0 fifo size at both speeds */
	qual->bMaxPacketSize0 = MAX_PACKET_SIZE0;
	qual->bNumConfigurations = 1;
	qual->bRESERVED = 0;

	return true;
}

static inline bool other_speed(u8_t **data, s32_t *len, u8_t index)
{
	u8_t *p = NULL;
	enum usb_device_speed speed = usb_dc_speed();

	if (usb_dc_maxspeed() < USB_SPEED_HIGH) {
		return false;
	}

	switch (speed) {
	case USB_SPEED_HIGH:
		p = (u8_t *)usb_dev.hs_descriptors;
		break;

	case USB_SPEED_FULL:
		p = (u8_t *)usb_dev.fs_descriptors;
		break;

	default:
		return false;
	}

	/* FIXME: not implemented yet */
	return false;
}

/*
 * @brief get specified USB descriptor
 *
 * This function parses the list of installed USB descriptors and attempts
 * to find the specified USB descriptor.
 *
 * @param [in]  type_index Type and index of the descriptor
 * @param [in]  lang_id    Language ID of the descriptor (currently unused)
 * @param [out] len        Descriptor length
 * @param [out] data       Descriptor data
 *
 * @return true if the descriptor was found, false otherwise
 */
static bool usb_get_descriptor(u16_t type_index, u16_t lang_id,
		s32_t *len, u8_t **data)
{
	u8_t type = 0;
	u8_t index = 0;
	u8_t *p = NULL;
	s32_t cur_index = 0;
	bool found = false;

	/*Avoid compiler warning until this is used for something*/
	ARG_UNUSED(lang_id);

	type = GET_DESC_TYPE(type_index);
	index = GET_DESC_INDEX(type_index);

	if (type == DESC_STRING) {
		switch (index) {
		case LANGUAGE_ID_STR:
			memset(str_desc_buf, 0, sizeof(str_desc_buf));
			str_desc_buf[0] = 0x04;
			str_desc_buf[1] = 0x03;
			str_desc_buf[2] = 0x09;
			str_desc_buf[3] = 0x04;
			*data = str_desc_buf;
			*len = 4;
			break;

		case MANUFACTURE_STR_DESC:
			usb_process_str_des(manufacturer_str_ptr);
			*data = str_desc_buf;
			*len = str_desc_buf[0];
			break;

		case PRODUCT_STR_DESC:
			usb_process_str_des(product_str_ptr);
			*data = str_desc_buf;
			*len = str_desc_buf[0];
			break;

		case DEV_SN_DESC:
			usb_process_str_des(dev_sn_str_ptr);
			*data = str_desc_buf;
			*len = str_desc_buf[0];
			break;

		default:
			break;
		}
		return true;
	}

	/*
	 * Invalid types of descriptors,
	 * see USB Spec. Revision 2.0, 9.4.3 Get Descriptor
	 */
	if ((type == DESC_INTERFACE) || (type == DESC_ENDPOINT) ||
	    (type > DESC_OTHER_SPEED)) {
		return false;
	}

	switch (type) {
	case DESC_DEVICE_QUALIFIER:
		*len = sizeof(struct usb_qualifier_descriptor);
		return device_qual(data);

	case DESC_OTHER_SPEED:
		return other_speed(data, len, index);

	default:
		break;
	}

	p = (u8_t *)usb_dev.descriptors;
	cur_index = 0;

	while (p[DESC_bLength] != 0) {
		if (p[DESC_bDescriptorType] == type) {
			if (cur_index == index) {
				found = true;
				break;
			}
			cur_index++;
		}
		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	if (found) {
		/* set data pointer */
		*data = p;
		/* get length from structure */
		if (type == DESC_CONFIGURATION) {
			/* configuration descriptor is an
			 * exception, length is at offset
			 * 2 and 3
			 */
			*len = (p[CONF_DESC_wTotalLength]) |
			    (p[CONF_DESC_wTotalLength + 1] << 8);
		} else {
			/* normally length is at offset 0 */
			*len = p[DESC_bLength];
		}
	} else {
		/* nothing found */
		LOG_DBG("Desc %x not found!\n", type_index);
	}
	return found;
}

/*
 * @brief set USB configuration
 *
 * This function configures the device according to the specified configuration
 * index and alternate setting by parsing the installed USB descriptor list.
 * A configuration index of 0 unconfigures the device.
 *
 * @param [in] config_index Configuration index
 * @param [in] alt_setting  Alternate setting number
 *
 * @return true if successfully configured false if error or unconfigured
 */
static bool usb_set_configuration(u8_t config_index, u8_t alt_setting)
{
	u8_t *p = NULL;
	u8_t cur_config = 0;
	u8_t cur_alt_setting = 0;

	if (config_index == 0) {
		/* unconfigure device */
		if (usb_dev.configuration) {
			usb_dev.unconfigured = 1;
		}
		LOG_DBG("Device not configured - invalid configuration "
			    "offset\n");
		return true;
	}

	/* configure endpoints for this configuration/altsetting */
	p = (u8_t *)usb_dev.descriptors;
	cur_config = 0xFF;
	cur_alt_setting = 0xFF;

	while (p[DESC_bLength] != 0) {
		switch (p[DESC_bDescriptorType]) {
		case DESC_CONFIGURATION:
			/* remember current configuration index */
			cur_config = p[CONF_DESC_bConfigurationValue];
			break;

		case DESC_INTERFACE:
			/* remember current alternate setting */
			cur_alt_setting =
			    p[INTF_DESC_bAlternateSetting];
			break;

		case DESC_ENDPOINT:
			if ((cur_config == config_index) &&
			    (cur_alt_setting == alt_setting)) {
				struct usb_dc_ep_cfg_data ep_cfg;
				/* endpoint found for desired config
				 * and alternate setting
				 */
				ep_cfg.ep_type =
				    p[ENDP_DESC_bmAttributes];
				ep_cfg.ep_mps =
				    (p[ENDP_DESC_wMaxPacketSize]) |
				    (p[ENDP_DESC_wMaxPacketSize + 1]
					    << 8);
				ep_cfg.ep_addr =
				    p[ENDP_DESC_bEndpointAddress];
				usb_dc_ep_configure(&ep_cfg);
				usb_dc_ep_enable(ep_cfg.ep_addr);
			}
			break;

		default:
			break;
		}
		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	usb_dev.unconfigured = 0;
	if (usb_dev.status_callback) {
		usb_dev.status_callback(USB_DC_CONFIGURED, &config_index);
	}

	return true;
}

/*
 * @brief get USB interface altsetting
 *
 * @param [in] iface Interface index
 *
 * @return current altsetting
 */
static inline u8_t usb_get_interface(u8_t iface)
{
	u16_t intf_info;

	/*
	 * In order to let upper layer know what the interface number
	 * and retrun the current alternate setting.
	 *
	 * iface: the higher byte, alt_setting: the lower byte
	 */
	intf_info = (iface << 8) + 0;

	LOG_DBG("iface %u\n", iface);

	if (usb_dev.status_callback) {
		usb_dev.status_callback(USB_DC_ALTSETTING, (u8_t *)&intf_info);
	}

	return intf_info & 0xFF;
}

/*
 * @brief set USB interface
 *
 * @param [in] iface Interface index
 * @param [in] alt_setting  Alternate setting number
 *
 * @return true if successfully configured false if error or unconfigured
 */
static bool usb_set_interface(u8_t iface, u8_t alt_setting)
{
	const u8_t *p = usb_dev.descriptors;
	u8_t cur_iface = 0xFF;
	u8_t cur_alt_setting = 0xFF;
	struct usb_dc_ep_cfg_data ep_cfg;
	u16_t intf_info;

	/*
	 * In order to let upper layer know what the interface number
	 * and the alternate setting of "Set Interface" transfer is.
	 *
	 * iface: the higher byte, alt_setting: the lower byte
	 */
	intf_info = (iface << 8) + alt_setting;

	LOG_DBG("iface %u alt_setting %u\n", iface, alt_setting);

	while (p[DESC_bLength] != 0) {
		switch (p[DESC_bDescriptorType]) {
		case DESC_INTERFACE:
			/* remember current alternate setting */
			cur_alt_setting = p[INTF_DESC_bAlternateSetting];
			cur_iface = p[INTF_DESC_bInterfaceNumber];
			break;
		case DESC_ENDPOINT:
			if ((cur_iface != iface) ||
			    (cur_alt_setting != alt_setting)) {
				break;
			}

			/* Endpoint is found for desired interface and
			 * alternate setting
			 */
			ep_cfg.ep_type = p[ENDP_DESC_bmAttributes] &
				USB_DC_EP_TYPE_MASK;
			ep_cfg.ep_mps = (p[ENDP_DESC_wMaxPacketSize]) |
				(p[ENDP_DESC_wMaxPacketSize + 1] << 8);
			ep_cfg.ep_addr = p[ENDP_DESC_bEndpointAddress];
			usb_dc_ep_configure(&ep_cfg);
			usb_dc_ep_enable(ep_cfg.ep_addr);

			LOG_DBG("Found: ep_addr 0x%x\n", ep_cfg.ep_addr);
			break;
		default:
			break;
		}

		/* skip to next descriptor */
		p += p[DESC_bLength];
		LOG_DBG("p %p\n", p);
	}

	if (usb_dev.status_callback) {
		usb_dev.status_callback(USB_DC_INTERFACE, (u8_t *)&intf_info);
	}

	return true;
}

/*
 * @brief handle a standard device request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in,out] data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_device_req(struct usb_setup_packet *setup,
		s32_t *len, u8_t **data_buf)
{
	u16_t value = sys_le16_to_cpu(setup->wValue);
	u16_t index = sys_le16_to_cpu(setup->wIndex);
	bool ret = true;
	u8_t *data = *data_buf;

	switch (setup->bRequest) {
	case REQ_GET_STATUS:
		LOG_DBG("REQ_GET_STATUS\n");
		/* bit 0: self-powered */
		/* bit 1: remote wakeup = not supported */
		data[0] = 0;
		data[1] = 0;

#ifdef CONFIG_USB_DEVICE_SELF_POWERED
		data[0] |= BIT(USB_DEVICE_SELF_POWERED);
#endif

#ifdef CONFIG_USB_DEVICE_REMOTE_WAKEUP
		data[0] |= (usb_dev.remote_wakeup ? BIT(USB_DEVICE_REMOTE_WAKEUP) : 0);
#endif

		*len = 2;
		break;

	case REQ_SET_ADDRESS:
		LOG_DBG("REQ_SET_ADDRESS, addr 0x%x\n", value);
		usb_dc_set_address(value);
		break;

	case REQ_GET_DESCRIPTOR:
		LOG_DBG("REQ_GET_DESCRIPTOR\n");
		ret = usb_get_descriptor(value, index, len, data_buf);
		break;

	case REQ_GET_CONFIGURATION:
		LOG_DBG("REQ_GET_CONFIGURATION\n");
		/* indicate if we are configured */
		data[0] = usb_dev.configuration;
		*len = 1;
		break;

	case REQ_SET_CONFIGURATION:
		value &= 0xFF;
		LOG_DBG("REQ_SET_CONFIGURATION, conf 0x%x\n", value);
		if (!usb_set_configuration(value, 0)) {
			LOG_DBG("USBSetConfiguration failed!\n");
			ret = false;
		} else {
			/* configuration successful,
			 * update current configuration
			 */
			usb_dev.configuration = value;
		}
		break;

	case REQ_CLEAR_FEATURE:
		LOG_DBG("REQ_CLEAR_FEATURE\n");
		ret = false;

#ifdef CONFIG_USB_DEVICE_REMOTE_WAKEUP
		if (value == FEA_REMOTE_WAKEUP) {
			usb_dev.remote_wakeup = false;
			ret = true;
		}
#endif
		break;

	case REQ_SET_FEATURE:
		LOG_DBG("REQ_SET_FEATURE\n");
		ret = false;

#ifdef CONFIG_USB_DEVICE_REMOTE_WAKEUP
		if (value == FEA_REMOTE_WAKEUP) {
			usb_dev.remote_wakeup = true;
			ret = true;
		}
#endif

		if (value == FEA_TEST_MODE) {
			/* put TEST_MODE code here */
		}
		break;

	case REQ_SET_DESCRIPTOR:
		LOG_DBG("Device req %x not implemented\n", setup->bRequest);
		ret = false;
		break;

	default:
		LOG_DBG("Illegal device req %x\n", setup->bRequest);
		ret = false;
		break;
	}

	return ret;
}

/*
 * @brief handle a standard interface request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_interface_req(struct usb_setup_packet *setup,
		s32_t *len, u8_t **data_buf)
{
	u16_t value = sys_le16_to_cpu(setup->wValue);
	u16_t index = sys_le16_to_cpu(setup->wIndex);
	u8_t *data = *data_buf;

	switch (setup->bRequest) {
	case REQ_GET_STATUS:
		/* no bits specified */
		data[0] = 0;
		data[1] = 0;
		*len = 2;
		break;

	case REQ_CLEAR_FEATURE:
	case REQ_SET_FEATURE:
		/* not defined for interface */
		return false;

	case REQ_GET_INTERFACE:
		data[0] = usb_get_interface(index);
		*len = 1;
		break;

	case REQ_SET_INTERFACE:
		LOG_DBG("REQ_SET_INTERFACE\n");
		usb_set_interface(index, value);
		*len = 0;
		break;

	default:
		LOG_DBG("Illegal interface req %d\n", setup->bRequest);
		return false;
	}

	return true;
}

/*
 * @brief handle a standard endpoint request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_endpoint_req(struct usb_setup_packet *setup,
		s32_t *len, u8_t **data_buf)
{
	u16_t value = sys_le16_to_cpu(setup->wValue);
	u8_t ep = sys_le16_to_cpu(setup->wIndex);
	u8_t *data = *data_buf;

	switch (setup->bRequest) {
	case REQ_GET_STATUS:
		/* bit 0 = endpointed halted or not */
		usb_dc_ep_is_stalled(ep, &data[0]);
		data[1] = 0;
		*len = 2;
		break;

	case REQ_CLEAR_FEATURE:
		if (value == FEA_ENDPOINT_HALT) {
			/* clear HALT by unstalling */
			LOG_DBG("... EP clear halt %x\n", ep);
			usb_dc_ep_clear_stall(ep);
			break;
		}
		/* only ENDPOINT_HALT defined for endpoints */
		return false;

	case REQ_SET_FEATURE:
		if (value == FEA_ENDPOINT_HALT) {
			/* set HALT by stalling */
			LOG_DBG("--- EP SET halt %x\n", ep);
			usb_dc_ep_set_stall(ep);
			break;
		}
		/* only ENDPOINT_HALT defined for endpoints */
		return false;

	case REQ_SYNCH_FRAME:
		LOG_DBG("EP req %d not implemented\n", setup->bRequest);
		return false;

	default:
		LOG_DBG("Illegal EP req %d\n", setup->bRequest);
		return false;
	}

	return true;
}


/*
 * @brief default handler for standard ('chapter 9') requests
 *
 * If a custom request handler was installed, this handler is called first.
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static int usb_handle_standard_request(struct usb_setup_packet *setup,
		s32_t *len, u8_t **data_buf)
{
	int rc = 0;

	if (!usb_handle_bos(setup, len, data_buf)) {
		return 0;
	}

	/* try the custom request handler first */
	if (usb_dev.custom_req_handler &&
	    !usb_dev.custom_req_handler(setup, len, data_buf)) {
		return 0;
	}

	switch (REQTYPE_GET_RECIP(setup->bmRequestType)) {
	case REQTYPE_RECIP_DEVICE:
		if (usb_handle_std_device_req(setup, len, data_buf) == false)
			rc = -EINVAL;
		break;
	case REQTYPE_RECIP_INTERFACE:
		if (usb_handle_std_interface_req(setup, len, data_buf) == false)
			rc = -EINVAL;
		break;
	case REQTYPE_RECIP_ENDPOINT:
		if (usb_handle_std_endpoint_req(setup, len, data_buf) == false)
			rc = -EINVAL;
		break;
	default:
		rc = -EINVAL;
	}
	return rc;
}


/*
 * @brief Registers a callback for custom device requests
 *
 * In usb_register_custom_req_handler, the custom request handler gets a first
 * chance at handling the request before it is handed over to the 'chapter 9'
 * request handler.
 *
 * This can be used for example in HID devices, where a REQ_GET_DESCRIPTOR
 * request is sent to an interface, which is not covered by the 'chapter 9'
 * specification.
 *
 * @param [in] handler Callback function pointer
 */
static void usb_register_custom_req_handler(usb_request_handler handler)
{
	usb_dev.custom_req_handler = handler;
}

/*
 * @brief register a callback for device status
 *
 * This function registers a callback for device status. The registered callback
 * is used to report changes in the status of the device controller.
 *
 * @param [in] cb Callback function pointer
 */
static void usb_register_status_callback(usb_status_callback cb)
{
	usb_dev.status_callback = cb;
}

/**
 * @brief turn on/off USB VBUS voltage
 *
 * @param on Set to false to turn off and to true to turn on VBUS
 *
 * @return 0 on success, negative errno code on fail
 */
static int usb_vbus_set(bool on)
{
#if defined(USB_VUSB_EN_GPIO)
	int ret = 0;
	struct device *gpio_dev = device_get_binding(USB_GPIO_DRV_NAME);

	if (!gpio_dev) {
		LOG_DBG("USB requires GPIO. Cannot find %s!\n",
			    USB_GPIO_DRV_NAME);
		return -ENODEV;
	}

	/* Enable USB IO */
	ret = gpio_pin_configure(gpio_dev, USB_VUSB_EN_GPIO, GPIO_DIR_OUT);
	if (ret)
		return ret;

	ret = gpio_pin_write(gpio_dev, USB_VUSB_EN_GPIO, on == true ? 1 : 0);
	if (ret)
		return ret;
#endif

	return 0;
}

static void status_cb(enum usb_dc_status_code status, u8_t *param)
{
	if (status == USB_DC_HIGHSPEED) {
		LOG_DBG("High-speed status cb\n");
		usb_dev.descriptors = usb_dev.hs_descriptors;
	}

	if (usb_dev.status_callback != NULL) {
		usb_dev.status_callback(status, param);
	}
}

bool usb_remote_wakeup_enabled(void)
{
	return usb_dev.remote_wakeup;
}

int usb_remote_wakeup(void)
{
#ifdef CONFIG_USB_DEVICE_REMOTE_WAKEUP
	if (usb_dev.remote_wakeup) {
		return usb_dc_do_remote_wakeup();
	}
	return -EACCES;
#else
	return -ENOTSUP;
#endif
}

int usb_set_config(const struct usb_cfg_data *config)
{
	if (!config) {
		return -EINVAL;
	}

	/* register descriptors */
	usb_dev.descriptors = usb_dev.fs_descriptors;

	/* register standard request handler */
	usb_register_request_handler(REQTYPE_TYPE_STANDARD,
				     usb_handle_standard_request);

	/* register class request handlers for each interface*/
	if (config->interface.class_handler != NULL) {
		usb_register_request_handler(REQTYPE_TYPE_CLASS,
					     config->interface.class_handler);
	}
	/* register vendor request handlers */
	if (config->interface.vendor_handler) {
		usb_register_request_handler(REQTYPE_TYPE_VENDOR,
					     config->interface.vendor_handler);
	}
	/* register class request handlers for each interface*/
	if (config->interface.custom_handler != NULL) {
		usb_register_custom_req_handler(
		config->interface.custom_handler);
	}

	return 0;
}

static u8_t class_num;
int usb_decice_composite_set_config(const struct usb_cfg_data *config)
{
	if (class_num >= CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM) {
		LOG_ERR("%d\n", class_num);
		class_num = 0;
		return -EINVAL;
	}

	usb_cfg_data_buf[class_num] = config;
	class_num++;

	return 0;
}

int usb_deconfig(void)
{
	class_num = 0;

	/* unegister standard request handler */
	usb_register_request_handler(REQTYPE_TYPE_STANDARD, NULL);

	/* unregister class request handlers for each interface*/
	usb_register_request_handler(REQTYPE_TYPE_CLASS, NULL);

	/* unregister class request handlers for each interface*/
	usb_register_custom_req_handler(NULL);

	/* unregister status callback */
	usb_register_status_callback(NULL);

	/* Reset USB controller */
	usb_dc_reset();

	return 0;
}

int usb_enable(const struct usb_cfg_data *config)
{
	int ret;
	u32_t i;
	struct usb_dc_ep_cfg_data ep0_cfg;

	if (true == usb_dev.enabled) {
		return 0;
	}

	/* Enable VBUS if needed */
	ret = usb_vbus_set(true);
	if (ret < 0)
		return ret;

	/* register status callback */
	if (config->cb_usb_status != NULL) {
		usb_register_status_callback(config->cb_usb_status);
	}
	ret = usb_dc_set_status_callback(status_cb);
	if (ret < 0)
		return ret;

	ret = usb_dc_attach();
	if (ret < 0)
		return ret;

	/* Configure control EP */
	ep0_cfg.ep_mps = MAX_PACKET_SIZE0;
	ep0_cfg.ep_type = USB_DC_EP_CONTROL;

	ep0_cfg.ep_addr = USB_CONTROL_OUT_EP0;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0)
		return ret;

	ep0_cfg.ep_addr = USB_CONTROL_IN_EP0;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0)
		return ret;

	/*register endpoint 0 handlers*/
	ret = usb_dc_ep_set_callback(USB_CONTROL_OUT_EP0,
	    usb_handle_control_transfer);
	if (ret < 0)
		return ret;
	ret = usb_dc_ep_set_callback(USB_CONTROL_IN_EP0,
	    usb_handle_control_transfer);
	if (ret < 0)
		return ret;

	/*register endpoint handlers*/
	for (i = 0; i < config->num_endpoints; i++) {
		ret = usb_dc_ep_set_callback(config->endpoint[i].ep_addr,
		    config->endpoint[i].ep_cb);
		if (ret < 0)
			return ret;
	}

#ifdef CONFIG_USB_DEVICE_TRANSFER
	/* init transfer slots */
	for (i = 0; i < MAX_NUM_TRANSFERS; i++) {
		k_work_init(&usb_dev.transfer[i].work, usb_transfer_work);
		k_sem_init(&usb_dev.transfer[i].sem, 1, 1);
	}
#endif

	/* enable control EP */
	ret = usb_dc_ep_enable(USB_CONTROL_OUT_EP0);
	if (ret < 0)
		return ret;

	ret = usb_dc_ep_enable(USB_CONTROL_IN_EP0);
	if (ret < 0)
		return ret;

	usb_dev.unconfigured = 0;
	usb_dev.enabled = true;

	return 0;
}

int usb_disable(void)
{
	int ret;

	if (true != usb_dev.enabled) {
		/* Already disabled: go ahead! */
		LOG_DBG("already\n");
	}

	ret = usb_dc_detach();
	if (ret < 0) {
		return ret;
	}

	/* Disable VBUS if needed */
	usb_vbus_set(false);

	usb_dev.enabled = false;

	return 0;
}

int usb_write(u8_t ep, const u8_t *data, u32_t data_len,
		u32_t *bytes_ret)
{
	return usb_dc_ep_write(ep, data, data_len, bytes_ret);
}

int usb_read(u8_t ep, u8_t *data, u32_t max_data_len,
		u32_t *ret_bytes)
{
	return usb_dc_ep_read(ep, data, max_data_len, ret_bytes);
}

int usb_read_async(u8_t ep, u8_t *data, u32_t max_data_len,
		u32_t *ret_bytes)
{
	return usb_dc_ep_read_async(ep, data, max_data_len, ret_bytes);
}

int usb_read_actual(u8_t ep, u32_t *ret_bytes)
{
	return usb_dc_ep_read_actual(ep, ret_bytes);
}

int usb_ep_set_stall(u8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_ep_clear_stall(u8_t ep)
{
	return usb_dc_ep_clear_stall(ep);
}

int usb_ep_read_wait(u8_t ep, u8_t *data, u32_t max_data_len,
			u32_t *ret_bytes)
{
	return usb_dc_ep_read_wait(ep, data, max_data_len, ret_bytes);
}

int usb_ep_read_continue(u8_t ep)
{
	return usb_dc_ep_read_continue(ep);
}

#ifdef CONFIG_USB_DEVICE_TRANSFER
/* Transfer management */
static struct usb_transfer_data *usb_ep_get_transfer(u8_t ep)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(usb_dev.transfer); i++) {
		if (usb_dev.transfer[i].ep == ep) {
			return &usb_dev.transfer[i];
		}
	}

	return NULL;
}

static void usb_transfer_work(struct k_work *item)
{
	struct usb_transfer_data *trans;
	int ret = 0, bytes;
	u8_t ep;

	trans = CONTAINER_OF(item, struct usb_transfer_data, work);
	ep = trans->ep;

	if (trans->status != -EBUSY) {
		/* transfer cancelled or already completed */
		goto done;
	}

	if (trans->flags & USB_TRANS_WRITE) {
		if (!trans->bsize) {
			if (!(trans->flags & USB_TRANS_NO_ZLP)) {
				usb_dc_ep_write(ep, NULL, 0, NULL);
			}
			trans->status = 0;
			goto done;
		}

		ret = usb_dc_ep_write(ep, trans->buffer, trans->bsize, &bytes);
		if (ret) {
			/* transfer error */
			trans->status = -EINVAL;
			goto done;
		}

		trans->buffer += bytes;
		trans->bsize -= bytes;
		trans->tsize += bytes;
	} else {
		ret = usb_dc_ep_read_wait(ep, trans->buffer, trans->bsize,
					  &bytes);
		if (ret) {
			/* transfer error */
			trans->status = -EINVAL;
			goto done;
		}

		trans->buffer += bytes;
		trans->bsize -= bytes;
		trans->tsize += bytes;

		/* ZLP, short-pkt or buffer full */
		if (!bytes || (bytes % usb_dc_ep_mps(ep)) || !trans->bsize) {
			/* transfer complete */
			trans->status = 0;
			goto done;
		}

		/* we expect mote data, clear NAK */
		usb_dc_ep_read_continue(ep);
	}

done:
	if (trans->status != -EBUSY && trans->cb) { /* Transfer complete */
		usb_transfer_callback cb = trans->cb;
		int tsize = trans->tsize;
		void *priv = trans->priv;

		if (k_is_in_isr()) {
			/* reschedule completion in thread context */
			k_work_submit(&trans->work);
			return;
		}

		LOG_DBG("transfer done, ep=%02x, status=%d, size=%zu\n",
			    trans->ep, trans->status, trans->tsize);

		trans->cb = NULL;
		k_sem_give(&trans->sem);

		/* Transfer completion callback */
		cb(ep, tsize, priv);
	}
}

void usb_transfer_ep_callback(u8_t ep, enum usb_dc_ep_cb_status_code status)
{
	struct usb_transfer_data *trans = usb_ep_get_transfer(ep);

	if (status != USB_DC_EP_DATA_IN && status != USB_DC_EP_DATA_OUT) {
		return;
	}

	if (!trans) {
		if (status == USB_DC_EP_DATA_IN) {
			u32_t bytes;
			/* In the unlikely case we receive data while no
			 * transfer is ongoing, we have to consume the data
			 * anyway. This is to prevent stucking reception on
			 * other endpoints (e.g dw driver has only one rx-fifo,
			 * so drain it).
			 */
			do {
				u8_t data;

				usb_dc_ep_read_wait(ep, &data, 1, &bytes);
			} while (bytes);

			LOG_ERR("RX data lost, no transfer\n");
		}
		return;
	}

	if (!k_is_in_isr() || (status == USB_DC_EP_DATA_OUT)) {
		/* If we are not in IRQ context, no need to defer work */
		/* Read (out) needs to be done from ep_callback */
		usb_transfer_work(&trans->work);
	} else {
		k_work_submit(&trans->work);
	}
}

int usb_transfer(u8_t ep, u8_t *data, size_t dlen, unsigned int flags,
		 usb_transfer_callback cb, void *cb_data)
{
	struct usb_transfer_data *trans = NULL;
	int i, key, ret = 0;

	LOG_DBG("transfer start, ep=%02x, data=%p, dlen=%zd\n",
		    ep, data, dlen);

	key = irq_lock();

	for (i = 0; i < MAX_NUM_TRANSFERS; i++) {
		if (!k_sem_take(&usb_dev.transfer[i].sem, K_NO_WAIT)) {
			trans = &usb_dev.transfer[i];
			break;
		}
	}

	if (!trans) {
		LOG_ERR("no transfer slot available\n");
		ret = -ENOMEM;
		goto done;
	}

	if (trans->status == -EBUSY) {
		/* A transfer is already ongoing and not completed */
		k_sem_give(&trans->sem);
		ret = -EBUSY;
		goto done;
	}

	/* Configure new transfer */
	trans->ep = ep;
	trans->buffer = data;
	trans->bsize = dlen;
	trans->tsize = 0;
	trans->cb = cb;
	trans->flags = flags;
	trans->priv = cb_data;
	trans->status = -EBUSY;

	if (usb_dc_ep_mps(ep) && (dlen % usb_dc_ep_mps(ep))) {
		/* no need to send ZLP since last packet will be a short one */
		trans->flags |= USB_TRANS_NO_ZLP;
	}

	if (flags & USB_TRANS_WRITE) {
		/* start writing first chunk */
		k_work_submit(&trans->work);
	} else {
		/* ready to read, clear NAK */
		ret = usb_dc_ep_read_continue(ep);
	}

done:
	irq_unlock(key);
	return ret;
}

void usb_cancel_transfer(u8_t ep)
{
	struct usb_transfer_data *trans;
	int key;

	key = irq_lock();

	trans = usb_ep_get_transfer(ep);
	if (!trans) {
		goto done;
	}

	if (trans->status != -EBUSY) {
		goto done;
	}

	trans->status = -ECANCELED;
	k_work_submit(&trans->work);

done:
	irq_unlock(key);
}

struct usb_transfer_sync_priv {
	int tsize;
	struct k_sem sem;
};

static void usb_transfer_sync_cb(u8_t ep, int size, void *priv)
{
	struct usb_transfer_sync_priv *pdata = priv;

	pdata->tsize = size;
	k_sem_give(&pdata->sem);
}

int usb_transfer_sync(u8_t ep, u8_t *data, size_t dlen, unsigned int flags)
{
	struct usb_transfer_sync_priv pdata;
	int ret;

	k_sem_init(&pdata.sem, 0, 1);

	ret = usb_transfer(ep, data, dlen, flags, usb_transfer_sync_cb, &pdata);
	if (ret) {
		return ret;
	}

	/* Semaphore will be released by the transfer completion callback */
	k_sem_take(&pdata.sem, K_FOREVER);

	return pdata.tsize;
}
#endif

static void forward_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	size_t size = CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM;

	if (status == USB_DC_HIGHSPEED) {
		usb_dev.descriptors = usb_dev.hs_descriptors;
	}

	for (size_t i = 0; i < size; i++) {
		if (usb_cfg_data_buf[i]->cb_usb_status) {
			usb_cfg_data_buf[i]->cb_usb_status(status, param);
		}
	}
}

/*
 * The functions class_handler(), custom_handler() and vendor_handler()
 * go through the interfaces one after the other and compare the
 * bInterfaceNumber with the wIndex and and then call the appropriate
 * callback of the USB function.
 * Note, a USB function can have more than one interface and the
 * request does not have to be directed to the first interface (unlikely).
 * These functions can be simplified and moved to usb_handle_request()
 * when legacy initialization throgh the usb_set_config() and
 * usb_enable() is no longer needed.
 */

static int class_handler(struct usb_setup_packet *pSetup,
			 s32_t *len, u8_t **data)
{
	size_t size = CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM;
	u16_t index = sys_le16_to_cpu(pSetup->wIndex);
	const struct usb_if_descriptor *if_descr;
	const struct usb_interface_cfg_data *iface;
	const struct usb_ep_cfg_data *ep_data;

	LOG_DBG("bRequest 0x%x, wIndex 0x%x\n", pSetup->bRequest, index);

	for (size_t i = 0; i < size; i++) {
		iface = &(usb_cfg_data_buf[i]->interface);
		if_descr = usb_cfg_data_buf[i]->interface_descriptor;
		if (!iface->class_handler) {
			continue;
		}

		/* interface */
		if (if_descr->bInterfaceNumber == (index & 0xFF)) {
			return iface->class_handler(pSetup, len, data);
		}

		/* endpoint */
		for (size_t j = 0; j < usb_cfg_data_buf[i]->num_endpoints; j++) {
			ep_data = usb_cfg_data_buf[i]->endpoint;

			if (ep_data[j].ep_addr == index) {
				return iface->class_handler(pSetup, len, data);
			}
		}
	}

	return -ENOTSUP;
}

static int custom_handler(struct usb_setup_packet *pSetup,
			  s32_t *len, u8_t **data)
{
	size_t size = CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM;
	u16_t index = sys_le16_to_cpu(pSetup->wIndex);
	const struct usb_if_descriptor *if_descr;
	const struct usb_interface_cfg_data *iface;

	LOG_DBG("bRequest 0x%x, wIndex 0x%x\n", pSetup->bRequest, index);

	for (size_t i = 0; i < size; i++) {
		iface = &(usb_cfg_data_buf[i]->interface);
		if_descr = usb_cfg_data_buf[i]->interface_descriptor;
		if ((iface->custom_handler) &&
		    (if_descr->bInterfaceNumber == (index & 0xFF))) {
			return iface->custom_handler(pSetup, len, data);
		}
	}

	return -ENOTSUP;
}

static int vendor_handler(struct usb_setup_packet *pSetup,
			  s32_t *len, u8_t **data)
{
	size_t size = CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM;
	u16_t index = sys_le16_to_cpu(pSetup->wIndex);
	const struct usb_if_descriptor *if_descr;
	const struct usb_interface_cfg_data *iface;

	LOG_DBG("bRequest 0x%x, wIndex 0x%x\n", pSetup->bRequest, index);

	for (size_t i = 0; i < size; i++) {
		iface = &(usb_cfg_data_buf[i]->interface);
		if_descr = usb_cfg_data_buf[i]->interface_descriptor;
		if ((iface->vendor_handler) &&
		    (if_descr->bInterfaceNumber == (index & 0xFF))) {
			return iface->vendor_handler(pSetup, len, data);
		}
	}

	return -ENOTSUP;
}

static int composite_setup_ep_cb(void)
{
	size_t size = CONFIG_USB_COMPOSITE_DEVICE_CLASS_NUM;
	const struct usb_ep_cfg_data *ep_data;

	for (size_t i = 0; i < size; i++) {
		ep_data = usb_cfg_data_buf[i]->endpoint;
		for (u8_t n = 0; n < usb_cfg_data_buf[i]->num_endpoints; n++) {
			LOG_INF("set cb, ep: 0x%x\n", ep_data[n].ep_addr);
			if (usb_dc_ep_set_callback(ep_data[n].ep_addr,
						   ep_data[n].ep_cb)) {
				return -1;
			}
		}
	}

	return 0;
}

/*
 * API: initialize USB composite device
 */
int usb_device_composite_init(struct device *dev)
{
	int ret;
	struct usb_dc_ep_cfg_data ep0_cfg;

	if (true == usb_dev.enabled) {
		return 0;
	}

	/* register composite device descriptors */
	usb_dev.descriptors = usb_dev.fs_descriptors;

	/* register standard request handler */
	usb_register_request_handler(REQTYPE_TYPE_STANDARD,
				     usb_handle_standard_request);

	/* register class request handlers for each interface*/
	usb_register_request_handler(REQTYPE_TYPE_CLASS, class_handler);

	/* register vendor request handlers */
	usb_register_request_handler(REQTYPE_TYPE_VENDOR, vendor_handler);

	/* register class request handlers for each interface*/
	usb_register_custom_req_handler(custom_handler);

	usb_register_status_callback(forward_status_cb);
	ret = usb_dc_set_status_callback(forward_status_cb);
	if (ret < 0) {
		return ret;
	}

	/* Enable VBUS if needed */
	ret = usb_vbus_set(true);
	if (ret < 0) {
		return ret;
	}

	ret = usb_dc_attach();
	if (ret < 0) {
		return ret;
	}

	/* Configure control EP */
	ep0_cfg.ep_mps = MAX_PACKET_SIZE0;
	ep0_cfg.ep_type = USB_DC_EP_CONTROL;

	ep0_cfg.ep_addr = USB_CONTROL_OUT_EP0;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0) {
		return ret;
	}

	ep0_cfg.ep_addr = USB_CONTROL_IN_EP0;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0) {
		return ret;
	}

	/*register endpoint 0 handlers*/
	ret = usb_dc_ep_set_callback(USB_CONTROL_OUT_EP0,
				     usb_handle_control_transfer);
	if (ret < 0)
		return ret;

	ret = usb_dc_ep_set_callback(USB_CONTROL_IN_EP0,
				     usb_handle_control_transfer);
	if (ret < 0)
		return ret;

	if (composite_setup_ep_cb()) {
		return -1;
	}

#ifdef CONFIG_USB_DEVICE_TRANSFER
	/* init transfer slots */
	for (int i = 0; i < MAX_NUM_TRANSFERS; i++) {
		k_work_init(&usb_dev.transfer[i].work, usb_transfer_work);
		k_sem_init(&usb_dev.transfer[i].sem, 1, 1);
	}
#endif

	/* enable control EP */
	ret = usb_dc_ep_enable(USB_CONTROL_OUT_EP0);
	if (ret < 0) {
		return ret;
	}

	ret = usb_dc_ep_enable(USB_CONTROL_IN_EP0);
	if (ret < 0) {
		return ret;
	}

	usb_dev.unconfigured = 0;
	usb_dev.enabled = true;

	return 0;
}

/*
 * API: deinitialize USB composite device
 */
int usb_device_composite_exit(void)
{
	int ret;

	ret = usb_disable();
	if (ret) {
		LOG_ERR("Failed to disable USB: %d\n", ret);
		return ret;
	}
	usb_deconfig();

	LOG_INF("done\n");

	return ret;
}

enum usb_device_speed usb_device_speed(void)
{
	return usb_dc_speed();
}

bool usb_device_unconfigured(void)
{
	return usb_dev.unconfigured == 1;
}
