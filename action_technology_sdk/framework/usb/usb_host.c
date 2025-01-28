/*
 * USB host core
 *
 * Copyright (c) 2020 Actions Corporation
 * Author: Jinang Lv <lvjinang@actions-semi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB host core layer
 *
 * NOTE:
 * 1. usb hub is NOT supported
 * 2. only ONE usb device is supported
 */
#include <sys/byteorder.h>
#include <drivers/gpio.h>
#include <usb/usb_host.h>

#include <logging/log.h>
#define	LOG_LEVEL CONFIG_SYS_LOG_USB_HOST_LEVEL
LOG_MODULE_REGISTER(usb_host);

#define DEVNUM_DEFAULT	1

#define NO_OF_RH_PORTS	1

#define PORT_RESET_TRIES	5
#define SET_ADDRESS_TRIES	2
#define GET_DESCRIPTOR_TRIES	2

#define HUB_ROOT_RESET_TIME	50	/* times are in msec */
#define HUB_SHORT_RESET_TIME	10
#define HUB_BH_RESET_TIME	50
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	800

static struct usb_device usbh_dev;

static sys_slist_t usb_drivers;

static void usbh_set_device_state(struct usb_device *udev,
				  enum usb_device_state new_state)
{
	unsigned int key;

	key = irq_lock();
	if (udev->state == USB_STATE_NOTATTACHED) {
		;	/* do nothing */
	} else {
		udev->state = new_state;
	}
	irq_unlock(key);
}

//should be defined in the file: board.h, Pay attention to the
//conflict of pin use, for example, the pin may be used for key detection
#define BOARD_USB_HOST_VBUS_EN_GPIO_NAME	"GPIOB"
#define BOARD_USB_HOST_VBUS_EN_GPIO		22	/* Vbus power en gpio for USB host */
#define BOARD_USB_HOST_VBUS_EN_GPIO_VALUE	1	/* Value to enable */

int usbh_vbus_set(bool on)
{
#ifdef BOARD_USB_HOST_VBUS_EN_GPIO_NAME
	static const struct device *usbh_gpio_dev;
	static u8_t usbh_gpio_value;
	u32_t value;
	int ret;

	if (usbh_gpio_dev == NULL) {
		usbh_gpio_dev = device_get_binding(BOARD_USB_HOST_VBUS_EN_GPIO_NAME);
		gpio_pin_configure(usbh_gpio_dev, BOARD_USB_HOST_VBUS_EN_GPIO,
				   GPIO_OUTPUT);
	}

	value = BOARD_USB_HOST_VBUS_EN_GPIO_VALUE;
	if (!on) {
		value = !value;
	}

	/* already */
	if (value == usbh_gpio_value) {
		return 0;
	}
	usbh_gpio_value = value;

	ret = gpio_pin_set(usbh_gpio_dev, BOARD_USB_HOST_VBUS_EN_GPIO,
			     value);
	if (ret) {
		return ret;
	}
#endif

	return 0;
}

/* returns 0 if no match, 1 if match */
static inline int usb_match_one_id_intf(const struct usb_if_descriptor *desc,
					const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
	    (id->bInterfaceClass != desc->bInterfaceClass)) {
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
	    (id->bInterfaceSubClass != desc->bInterfaceSubClass)) {
		return 0;
	}

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
	    (id->bInterfaceProtocol != desc->bInterfaceProtocol)) {
		return 0;
	}

	return 1;
}

/* returns 0 if no match, 1 if match */
static const inline struct usb_device_id *usb_match_id(
						       struct usb_if_descriptor *desc,
						       const struct usb_device_id *id)
{
	for (; id->idVendor || id->idProduct || id->bDeviceClass ||
	       id->bInterfaceClass || id->driver_info; id++) {
		if (usb_match_one_id_intf(desc, id)) {
			return id;
		}
	}

	return NULL;
}

static inline int usbh_probe_interface(struct usb_interface *intf)
{
	const struct usb_device_id *id;
	struct usb_driver *driver;
	sys_snode_t *node;
	int ret;

	LOG_DBG("");

	if (sys_slist_is_empty(&usb_drivers)) {
		return -EINVAL;
	}

	node = sys_slist_peek_head(&usb_drivers);

	do {
		driver = CONTAINER_OF(node, struct usb_driver, node);
		id = usb_match_id(&intf->desc, driver->id_table);
		if (id) {
			intf->driver = driver;	/* bind first */
			ret = driver->probe(intf, driver->id_table);
			if (ret) {
				intf->driver = NULL;
				return ret;
			}
		}

		node = sys_slist_peek_next(node);
	} while (node);

	return 0;
}

int usbh_submit_urb(struct usb_request *urb)
{
	if (!urb || !urb->complete) {
		return -EINVAL;
	}

	if (usbh_dev.state < USB_STATE_UNAUTHENTICATED) {
		LOG_INF("%d", usbh_dev.state);
		return -ENODEV;
	}

	urb->status = -EBUSY;
	urb->actual = 0;

	return usb_hc_submit_urb(urb);
}

int usbh_cancel_urb(struct usb_request *urb)
{
	unsigned int key = irq_lock();
	int ret = 0;

	if (urb->status != -EBUSY) {
		goto done;
	}

	/* canceled or timeout */
	urb->status = -ECONNRESET;

	ret = usb_hc_cancel_urb(urb);

done:
	irq_unlock(key);

	return ret;
}

static void usbh_blocking_completion(struct usb_request *urb)
{
	struct k_sem *sem = urb->context;

	k_sem_give(sem);
}

static inline int usbh_start_wait_urb(struct usb_request *urb, int timeout,
				      u32_t *actual)
{
	struct k_sem sem;
	int ret;

	k_sem_init(&sem, 0, 1);

	urb->complete = usbh_blocking_completion;
	urb->context = &sem;

	if (timeout == 0) {
		timeout = K_TICKS_FOREVER;
	}

	ret = usbh_submit_urb(urb);
	if (ret) {
		return ret;
	}

	/* Semaphore will be released by the transfer completion callback */
	ret = k_sem_take(&sem, K_MSEC(timeout));
	if (ret == -EAGAIN) {
		usbh_cancel_urb(urb);
	}
	if (actual) {
		*actual = urb->actual;
	}

	return urb->status;
}

int usbh_transfer_urb_sync(struct usb_request *urb)
{
	return usbh_start_wait_urb(urb, K_TICKS_FOREVER, NULL);
}

int usbh_control_msg(unsigned char request, unsigned char requesttype,
		     unsigned short value, unsigned short index,
		     void *data, unsigned short size, int timeout)
{
	struct usb_setup_packet setup_packet __aligned(2);
	struct usb_request urb;
	int length;
	int ret;

	/* set setup command */
	setup_packet.bmRequestType = requesttype;
	setup_packet.bRequest = request;
	setup_packet.wValue = sys_cpu_to_le16(value);
	setup_packet.wIndex = sys_cpu_to_le16(index);
	setup_packet.wLength = sys_cpu_to_le16(size);
	LOG_DBG("request: 0x%X, requesttype: 0x%X, " \
		    "value 0x%X index 0x%X length 0x%X",
		    request, requesttype, value, index, size);

	/* for data phase */
	if (USB_REQ_DIR_IN(requesttype)) {
		urb.ep = USB_CONTROL_IN_EP0;
	} else {
		urb.ep = USB_CONTROL_OUT_EP0;
	}

	urb.ep_type = USB_EP_CONTROL;
	urb.buf = data;
	urb.len = size;
	urb.setup_packet = (u8_t *)&setup_packet;

	ret = usbh_start_wait_urb(&urb, timeout, &length);
	if (ret < 0) {
		return ret;
	} else {
		return length;
	}
}

int usbh_bulk_msg(u8_t ep, void *data, int len, u32_t *actual, int timeout)
{
	struct usb_request urb;

	urb.ep = ep;
	urb.ep_type = USB_EP_BULK;
	urb.buf = data;
	urb.len = len;

	return usbh_start_wait_urb(&urb, timeout, actual);
}

/* clear endpoint halt/stall condition */
int usbh_clear_halt(u8_t ep)
{
	int ret;

	LOG_DBG("0x%x", ep);

	ret = usbh_control_msg(USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
			       USB_ENDPOINT_HALT, ep, NULL, 0,
			       USB_CTRL_TIMEOUT);
	if (ret) {
		return ret;
	}

	return usb_hc_ep_reset(ep);
}

static inline int usbh_root_control(unsigned char request,
				    unsigned char requesttype,
				    unsigned short value, unsigned short index,
				    void *data, unsigned short size,
				    int timeout)
{
	struct usb_setup_packet setup_packet __aligned(2);

	if (usbh_dev.state == USB_STATE_NOTATTACHED) {
		return -ENODEV;
	}

	/* set setup command */
	setup_packet.bmRequestType = requesttype;
	setup_packet.bRequest = request;
	setup_packet.wValue = sys_cpu_to_le16(value);
	setup_packet.wIndex = sys_cpu_to_le16(index);
	setup_packet.wLength = sys_cpu_to_le16(size);
	LOG_DBG("request: 0x%X, requesttype: 0x%X, " \
		    "value 0x%X index 0x%X length 0x%X",
		    request, requesttype, value, index, size);

	return usb_hc_root_control(data, size, &setup_packet, timeout);
}

static inline int clear_port_feature(int port, int feature)
{
	return usbh_root_control(USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature,
				 port, NULL, 0, USB_CTRL_TIMEOUT);
}

static inline int set_port_feature(int port, int feature)
{
	return usbh_root_control(USB_REQ_SET_FEATURE, USB_RT_PORT, feature,
				 port, NULL, 0, USB_CTRL_TIMEOUT);
}

static inline int hub_port_status(int port1, u16_t *status, u16_t *change)
{
	int ret;
	struct usb_port_status portsts;

	ret = usbh_root_control(USB_REQ_GET_STATUS,
				USB_DIR_IN | USB_RT_PORT, 0, port1,
				&portsts, sizeof(portsts), USB_CTRL_TIMEOUT);
	if (ret < 4) {
		return ret;
	}

	*status = sys_le16_to_cpu(portsts.wPortStatus);
	*change = sys_le16_to_cpu(portsts.wPortChange);

	LOG_DBG("portstatus %x, change %x", *status, *change);

	return 0;
}

static inline int hub_port_wait_reset(int port1, int delay)
{
	unsigned short portstatus = 0, portchange = 0;
	int delay_time, ret;

	for (delay_time = 0;
	     delay_time < HUB_RESET_TIMEOUT;
	     delay_time += delay) {
		/* wait to give the device a chance to reset */
		k_sleep(K_MSEC(delay));

		/* read and decode port status */
		ret = hub_port_status(port1, &portstatus, &portchange);
		if (ret < 0) {
			return ret;
		}

		/* The port state is unknown until the reset completes. */
		if (!(portstatus & USB_PORT_STAT_RESET)) {
			break;
		}
	}

	if ((portstatus & USB_PORT_STAT_RESET)) {
		return -EBUSY;
	}

	/* Device went away? */
	if (!(portstatus & USB_PORT_STAT_CONNECTION)) {
		return -ENOTCONN;
	}

	if (!(portstatus & USB_PORT_STAT_ENABLE)) {
		return -EBUSY;
	}

	if (portstatus & USB_PORT_STAT_HIGH_SPEED) {
		usbh_dev.speed = USB_SPEED_HIGH;
	} else if (portstatus & USB_PORT_STAT_LOW_SPEED) {
		usbh_dev.speed = USB_SPEED_LOW;
	} else {
		usbh_dev.speed = USB_SPEED_FULL;
	}

	return 0;
}

static inline int hub_port_reset(int port1, int delay)
{
	int i, status;

	LOG_DBG("");

	for (i = 0; i < PORT_RESET_TRIES; i++) {
		status = set_port_feature(port1, USB_PORT_FEAT_RESET);
		if (status < 0) {
			;
		} else {
			status = hub_port_wait_reset(port1, delay);
		}

		if (status == 0) {
			k_sleep(K_MSEC(50));

			usbh_set_device_state(&usbh_dev, USB_STATE_DEFAULT);
			break;
		} else if (status == -ENOTCONN || status == -ENODEV) {
			clear_port_feature(port1, USB_PORT_FEAT_C_RESET);
			usbh_set_device_state(&usbh_dev, USB_STATE_NOTATTACHED);
			break;
		}
	}

	LOG_DBG("done");

	return status;
}

static inline int usbh_get_descriptor(unsigned char type,
				      unsigned char index, void *buf, int size)
{
	return usbh_control_msg(USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			(type << 8) + index, 0,
			buf, size, USB_CTRL_TIMEOUT);
}

static inline int usbh_get_device_descriptor(int size)
{
	LOG_DBG("%d", size);

	return usbh_get_descriptor(USB_DEVICE_DESC, 0, &usbh_dev.desc, size);
}

static inline int usbh_set_address(u8_t addr)
{
	int ret;

	LOG_DBG("");

	usbh_dev.devnum = addr;

	ret = usbh_control_msg(USB_REQ_SET_ADDRESS, 0, addr, 0,
			       NULL, 0, USB_CTRL_TIMEOUT);
	if (ret) {
		return ret;
	}

	return usb_hc_set_address(DEVNUM_DEFAULT);
}

static inline int usbh_parse_configuration(u8_t *buf, int len, int cfgidx)
{
	struct usb_config *config = &usbh_dev.config;
	int index, ifno, epno, curr_if_num;
	struct usb_interface *intf = NULL;
	struct usb_desc_header *head;

	ARG_UNUSED(cfgidx);

	ifno = epno = curr_if_num = -1;
	config->no_of_if = 0;

	index = config->desc.bLength;
	head = (struct usb_desc_header *)&buf[index];
	while (index + 1 < len && head->bLength) {
		switch (head->bDescriptorType) {
		case USB_INTERFACE_DESC:
			if (head->bLength != USB_INTERFACE_DESC_SIZE) {
				LOG_ERR("intf %d %d", head->bDescriptorType,
					    head->bLength);
				break;
			}
			if (index + USB_INTERFACE_DESC_SIZE > len) {
				LOG_ERR("intf overflow");
				break;
			}
			if (((struct usb_if_descriptor *) \
			     head)->bInterfaceNumber != curr_if_num) {
				ifno = config->no_of_if;
				if (ifno >= USB_MAXINTERFACES) {
					LOG_ERR("Too many intf\n");
					/* try to go on with what we have */
					return -EINVAL;
				}
				intf = &config->intf[ifno];
				config->no_of_if++;
				memcpy(intf, head, USB_INTERFACE_DESC_SIZE);
				intf->no_of_ep = 0;
				intf->num_altsetting = 1;
				curr_if_num = intf->desc.bInterfaceNumber;
			} else {
				/* found alternate setting for the interface */
				if (ifno >= 0) {
					intf = &config->intf[ifno];
					intf->num_altsetting++;
				}
			}
			break;

		case USB_ENDPOINT_DESC:
			if (head->bLength != USB_ENDPOINT_DESC_SIZE) {
				LOG_ERR("endp %d %d", head->bDescriptorType,
					    head->bLength);
				break;
			}
			if (index + USB_ENDPOINT_DESC_SIZE > len) {
				LOG_ERR("endp overflow");
				break;
			}
			if (ifno < 0) {
				LOG_ERR("endp out of order");
				break;
			}
			epno = config->intf[ifno].no_of_ep;
			intf = &config->intf[ifno];
			if (epno >= USB_MAXENDPOINTS) {
				LOG_ERR("intf %d has too many endp", ifno);
				return -EINVAL;
			}
			/* found an endpoint */
			intf->no_of_ep++;
			memcpy(&intf->ep_desc[epno], head,
			       USB_ENDPOINT_DESC_SIZE);
			/* (u8_t *)(&((struct usb_ep_descriptor *)head->wMaxPacketSize)) */
			intf->ep_desc[epno].wMaxPacketSize = sys_get_le16(((u8_t *)head + 4));
			break;

		case USB_INTERFACE_ASSOC_DESC:
		default:
			if (head->bLength == 0) {
				return -EINVAL;
			}

			LOG_DBG("unknown : %x", head->bDescriptorType);
			break;
		}
		index += head->bLength;
		head = (struct usb_desc_header *)&buf[index];
	}

	return 0;
}

static inline int usbh_get_configuration(void)
{
	u8_t *tmp = usbh_dev.config.rawdescriptors;
	u16_t length;
	int ret;

	LOG_DBG("");

	ret = usbh_get_descriptor(USB_CONFIGURATION_DESC, 0,
	      &usbh_dev.config.desc, USB_CONFIGURATION_DESC_SIZE);
	if (ret < USB_CONFIGURATION_DESC_SIZE) {
		LOG_ERR("config: %d", ret);
		return -EIO;
	}

	length = sys_le16_to_cpu(usbh_dev.config.desc.wTotalLength);
	if (length > USB_RAW_DESCRIPTORS_SIZE) {
		LOG_WRN("long: %d", length);
		length = USB_RAW_DESCRIPTORS_SIZE;
	}

	ret = usbh_get_descriptor(USB_CONFIGURATION_DESC, 0, tmp, length);
	if (ret < 0) {
		LOG_ERR("all: %d", ret);
		return ret;
	}
	if (ret < length) {
		LOG_WRN("short: %d", ret);
		length = ret;
	}

	return usbh_parse_configuration(tmp, length, 0);
}

static inline int usbh_set_configuration(int configuration)
{
	int ret;

	LOG_DBG("%d", configuration);

	ret = usbh_control_msg(USB_REQ_SET_CONFIGURATION, 0,
			       configuration, 0, NULL, 0, USB_CTRL_TIMEOUT);
	if (ret) {
		LOG_ERR("%d", ret);
		return ret;
	}

	usbh_set_device_state(&usbh_dev, USB_STATE_CONFIGURED);

	return 0;
}

int usbh_enable_endpoint(struct usb_ep_descriptor *desc)
{
	struct usb_hc_ep_cfg_data ep_cfg;
	int ret;

	LOG_DBG("0x%x", desc->bEndpointAddress);

	if (usbh_dev.state == USB_STATE_NOTATTACHED) {
		return -ENODEV;
	}

	ep_cfg.ep_addr = desc->bEndpointAddress;
	ep_cfg.ep_mps = desc->wMaxPacketSize;
	ep_cfg.ep_type = usb_endpoint_type(desc->bmAttributes);

	ret = usb_hc_ep_configure(&ep_cfg);
	if (ret) {
		LOG_ERR("configure ep%x: %d", ep_cfg.ep_addr, ret);
		return ret;
	}

	ret = usb_hc_ep_enable(ep_cfg.ep_addr);
	if (ret) {
		LOG_ERR("enable ep%x: %d", ep_cfg.ep_addr, ret);
		return ret;
	}

	return 0;
}

int usbh_disable_endpoint(struct usb_ep_descriptor *desc)
{
	unsigned int key;
	int ret;

	LOG_DBG("0x%x", desc->bEndpointAddress);

	key = irq_lock();

	ret = usb_hc_ep_disable(desc->bEndpointAddress);

	irq_unlock(key);

	return ret;
}

int usbh_enable_interface(struct usb_interface *intf)
{
	int i;

	for (i = 0; i < intf->no_of_ep; i++) {
		usbh_enable_endpoint(&intf->ep_desc[i]);
	}

	return 0;
}

int usbh_disable_interface(struct usb_interface *intf)
{
	int i;

	for (i = 0; i < intf->no_of_ep; i++) {
		usbh_disable_endpoint(&intf->ep_desc[i]);
	}

	return 0;
}

/* Enable control EP */
static inline int enable_ep0(u8_t mps)
{
	struct usb_ep_descriptor desc;
	int ret;

	/* ep0-out */
	desc.bEndpointAddress = USB_CONTROL_OUT_EP0;
	desc.bmAttributes = USB_EP_CONTROL;
	desc.wMaxPacketSize = mps;
	ret = usbh_enable_endpoint(&desc);
	if (ret) {
		return ret;
	}

	/* ep0-in */
	desc.bEndpointAddress = USB_CONTROL_IN_EP0;
	usbh_enable_endpoint(&desc);
	if (ret) {
		return ret;
	}

	return ret;
}

/* Disable control EP */
static inline int disable_ep0(void)
{
	struct usb_ep_descriptor desc;

	desc.bEndpointAddress = USB_CONTROL_OUT_EP0;
	usbh_disable_endpoint(&desc);

	desc.bEndpointAddress = USB_CONTROL_IN_EP0;
	usbh_disable_endpoint(&desc);

	return 0;
}

static inline int usbh_enumerate_device(void)
{
	struct usb_interface *intf;
	int ret;
	u8_t i;

	ret = usbh_get_configuration();
	if (ret) {
		return ret;
	}

	/* FIXME: get string descriptors */
#if 0
	if (usbh_dev.desc.iManufacturer)
		usb_string(usbh_dev.desc.iManufacturer,
			   usbh_dev.mf, sizeof(usbh_dev.mf));
	if (usbh_dev.desc.iProduct)
		usb_string(usbh_dev.desc.iProduct,
			   usbh_dev.prod, sizeof(usbh_dev.prod));
	if (usbh_dev.desc.iSerialNumber)
		usb_string(usbh_dev.desc.iSerialNumber,
			   usbh_dev.serial, sizeof(usbh_dev.serial));
	LOG_DBG("mf: %s, prod: %s, sn: %s",
		    usbh_dev.mf, usbh_dev.prod, usbh_dev.serial);
#endif

	ret = usbh_set_configuration(usbh_dev.config.desc.bConfigurationValue);
	if (ret) {
		return ret;
	}

	for (i = 0; i < usbh_dev.config.no_of_if; i++) {
		intf = &usbh_dev.config.intf[i];
		ret = usbh_probe_interface(intf);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static inline int hub_port_init(int port1)
{
	int ret;

	LOG_DBG("");

	usbh_set_device_state(&usbh_dev, USB_STATE_POWERED);
	ret = hub_port_reset(port1, HUB_ROOT_RESET_TIME);
	if (ret) {
		LOG_ERR("reset: %d", ret);
		return ret;
	}

	LOG_INF("speed: %d", usbh_dev.speed);

	/* default bMaxPacketSize0: 64 */
	ret = enable_ep0(USB_MAX_CTRL_MPS);
	if (ret) {
		return ret;
	}

	ret = usbh_get_device_descriptor(64);
	if (ret < 8) {
		LOG_ERR("device desc(64): %d", ret);
		return -EIO;
	}

	if (usbh_dev.desc.bMaxPacketSize0 != USB_MAX_CTRL_MPS) {
		ret = enable_ep0(usbh_dev.desc.bMaxPacketSize0);
		if (ret) {
			return ret;
		}
	}

	ret = hub_port_reset(port1, HUB_ROOT_RESET_TIME);
	if (ret) {
		LOG_ERR("reset: %d", ret);
		return ret;
	}

	LOG_INF("speed: %d", usbh_dev.speed);

	ret = usbh_set_address(DEVNUM_DEFAULT);
	if (ret) {
		LOG_ERR("address: %d", ret);
		return ret;
	}

	usbh_set_device_state(&usbh_dev, USB_STATE_ADDRESS);

	/* Let the SET_ADDRESS settle */
	k_sleep(K_MSEC(10));

	ret = usbh_get_device_descriptor(USB_DEVICE_DESC_SIZE);
	if (ret < USB_DEVICE_DESC_SIZE) {
		LOG_ERR("device desc: %d", ret);
		return -EIO;
	}

	LOG_DBG("OK");

	return 0;
}

int usbh_prepare_scan(void)
{
	usbh_dev.state = USB_STATE_ATTACHED;
	usbh_dev.scanning = 1;

	return usb_hc_enable();
}

int usbh_scan_device(void)
{
	int ret;

	LOG_DBG("");

	ret = hub_port_init(NO_OF_RH_PORTS);
	if (ret) {
		goto error;
	}

	/* correct le values */
	usbh_dev.desc.bcdUSB = sys_le16_to_cpu(usbh_dev.desc.bcdUSB);
	usbh_dev.desc.idVendor = sys_le16_to_cpu(usbh_dev.desc.idVendor);
	usbh_dev.desc.idProduct = sys_le16_to_cpu(usbh_dev.desc.idProduct);
	usbh_dev.desc.bcdDevice = sys_le16_to_cpu(usbh_dev.desc.bcdDevice);

	LOG_DBG("bcdUSB: 0x%x, idVendor: 0x%x, idProduct: 0x%x\n",
		    usbh_dev.desc.bcdUSB,
		    usbh_dev.desc.idVendor,
		    usbh_dev.desc.idProduct);

	ret = usbh_enumerate_device();
	if (ret) {
		goto error;
	}

	LOG_INF("enum done");

	usbh_dev.scanning = 0;

	return 0;
error:
	usbh_vbus_set(false);
	LOG_ERR("failed");
	usbh_dev.scanning = 0;
	return ret;
}

int usbh_reset_device(void)
{
	int ret;
	u8_t i;

	if (usbh_dev.state != USB_STATE_CONFIGURED) {
		return -ENODEV;
	}

	LOG_INF("");

	ret = hub_port_init(NO_OF_RH_PORTS);
	if (ret) {
		goto error;
	}

	/* set configuration */
	ret = usbh_set_configuration(usbh_dev.config.desc.bConfigurationValue);
	if (ret) {
		goto error;
	}

	for (i = 0; i < usbh_dev.config.no_of_if; i++) {
		struct usb_interface *intf = &usbh_dev.config.intf[i];

		if (intf->desc.bAlternateSetting == 0) {
			usbh_disable_interface(intf);
			usbh_enable_interface(intf);
		}
	}

	LOG_INF("done");

	return 0;

error:
	/* upper layer should do disconnect or ... */
	return ret;
}

int usbh_disconnect(void)
{
	struct usb_interface *intf;
	u8_t i;

	LOG_DBG("");

	usbh_set_device_state(&usbh_dev, USB_STATE_NOTATTACHED);

	disable_ep0();

	for (i = 0; i < usbh_dev.config.no_of_if; i++) {
		intf = &usbh_dev.config.intf[i];
		if (intf->driver) {
			intf->driver->disconnect(intf);
			intf->driver = NULL;
		}
	}

	if (usbh_dev.scanning == 1) {
		LOG_WRN("scanning");
		return -EAGAIN;
	}

	usb_hc_disable();

	/* USB_STATE_NOTATTACHED */
	memset(&usbh_dev, 0, sizeof(usbh_dev));

	LOG_INF("done");

	return 0;
}

bool usbh_is_scanning(void)
{
	return usbh_dev.scanning;
}

int usbh_register_driver(struct usb_driver *driver)
{
	sys_slist_append(&usb_drivers, &driver->node);

	return 0;
}

void usbh_unregister_driver(struct usb_driver *driver)
{
	sys_slist_find_and_remove(&usb_drivers, &driver->node);
}

int usbh_fifo_control(bool enable)
{
	return usb_hc_fifo_control(enable);
}
