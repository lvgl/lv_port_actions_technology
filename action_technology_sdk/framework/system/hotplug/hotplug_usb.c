/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug usb interface
 */

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "hotpug_manager"

#include <os_common_api.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <sys_manager.h>
#include <string.h>
#include <fs_manager.h>
#include <hotplug_manager.h>

#include <init.h>
#include <stdio.h>
#include <device.h>
#include <stdio.h>

#include <usb/usb_otg.h>
#include <usb/usb_device.h>
#ifdef CONFIG_USB_HOST
#include <usb/usb_host.h>
#endif
#include <usb/class/usb_msc.h>

#ifdef CONFIG_USB_DEVICE
#ifdef CONFIG_PM_DEVICE
#include <pm/device.h>
#include <sys_wakelock.h>
static bool usb_wake_unlock;
#endif
#endif

#define USB_HOTPLUG_NONE	0
/* host mode: peripheral device connected */
#define USB_HOTPLUG_A_IN	1
/* host mode: peripheral device disconnected */
#define USB_HOTPLUG_A_OUT	2
/* device mode: connected to host */
#define USB_HOTPLUG_B_IN	3
/* device mode: disconnected from host */
#define USB_HOTPLUG_B_OUT	4
/* device mode: connected to charger */
#define USB_HOTPLUG_C_IN	5
/* device mode: disconnected from charger */
#define USB_HOTPLUG_C_OUT	6
/* usb hotplug suspend: stop detection */
#define USB_HOTPLUG_SUSPEND	7

static uint8_t usb_hotplug_state;
#if CONFIG_LOG
static char *state_string[] = {
	"undefined",
	"b_idle",
	"b_srp_init",
	"b_peripheral",
	"b_wait_acon",
	"b_host",
	"a_idle",
	"a_wait_vrise",
	"a_wait_bcon",
	"a_host",
	"a_suspend",
	"a_peripheral",
	"a_wait_vfall",
	"a_vbus_err"
};
#endif
static uint8_t otg_state;

#ifdef CONFIG_USB_DEVICE
/* distinguish pc/charger: nearly 10s */
#define USB_CONNECT_COUNT_MAX	(10000/ CONFIG_MONITOR_PERIOD)

static uint16_t usb_connect_count;

/* optimize: if no_plugin */
#define KEEP_IN_B_IDLE_RETRY	10
static uint8_t keep_in_b_idle;

/* doing b_peripheral exit process */
static uint8_t otg_b_peripheral_exiting;
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
/* timeout: nearly 1s */
#define OTG_A_WAIT_BCON_MAX	(K_SECONDS(1) / CONFIG_MONITOR_PERIOD)
static uint8_t otg_a_wait_bcon_count;

/* timeout: nearly 4s */
#define OTG_A_IDLE_MAX	(K_SECONDS(4) / CONFIG_MONITOR_PERIOD)
static uint8_t otg_a_idle_count;

/* doing a_host exit process */
static uint8_t otg_a_host_exiting;
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_HOTPLUG_THREAD_ENABLED
#define STACK_SZ	CONFIG_USB_HOTPLUG_STACKSIZE
#define THREAD_PRIO	CONFIG_USB_HOTPLUG_PRIORITY

static uint8_t usb_hotplug_stack[CONFIG_USB_HOTPLUG_STACKSIZE];

#ifdef CONFIG_USB_HOST
static void host_scan_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	usbh_scan_device();
}

static inline int usb_thread_init_host_scan(void)
{
	k_tid_t tid;

	usbh_prepare_scan();

	/* Start a thread to offload USB scan/enumeration */
	tid = os_thread_create(usb_hotplug_stack, STACK_SZ,
			host_scan_thread,
			NULL, NULL, NULL,
			THREAD_PRIO, 0, 0);

	os_thread_name_set(tid, "host_scan");

	return 0;
}

static inline int usb_thread_exit_host_scan(void)
{
	return usbh_disconnect();
}
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_MASS_STORAGE_SHARE_THREAD
int usb_mass_storage_start(void)
{
	k_tid_t tid;

	SYS_LOG_INF("shared");

	tid = os_thread_create(usb_hotplug_stack, STACK_SZ,
			usb_mass_storage_thread,
			NULL, NULL, NULL,
			CONFIG_MASS_STORAGE_PRIORITY, 0, 0);

	os_thread_name_set(tid, "mass_storage");

	return 0;
}
#endif /* CONFIG_USB_MASS_STORAGE_SHARE_THREAD */
#endif /* CONFIG_USB_HOTPLUG_THREAD_ENABLED */

static int usb_hotplug_dpdm_switch(bool host_mode)
{
#ifdef BOARD_USB_SWITCH_GPIO_NAME
	static struct device *usb_switch_gpio_dev;
	static uint8_t usb_switch_gpio_value;
	uint32_t value;
	int ret;

	if (usb_switch_gpio_dev == NULL) {
		usb_switch_gpio_dev = device_get_binding(BOARD_USB_SWITCH_GPIO_NAME);
		gpio_pin_configure(usb_switch_gpio_dev, BOARD_USB_SWITCH_EN_GPIO,
				   GPIO_DIR_OUT);
	}

	value = BOARD_USB_SWITCH_HOST_EN_GPIO_VALUE;
	if (!host_mode) {
		value = !value;
	}

	/* already */
	if (value == usb_switch_gpio_value) {
		return 0;
	}
	usb_switch_gpio_value = value;

	ret = gpio_pin_write(usb_switch_gpio_dev, BOARD_USB_SWITCH_EN_GPIO,
				 value);
	if (ret) {
		return ret;
	}
#endif

	return 0;
}

static void usb_hotplug_status_cb(enum usb_dc_status_code status, uint8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_HIGHSPEED:
		SYS_LOG_DBG("USB HS detected");
		usb_disable();
		break;
	case USB_DC_SOF:
		SYS_LOG_DBG("USB SOF detected");
		usb_disable();
		break;
	default:
		break;
	}
}

u8_t usb_hotplug_get_otgstate(void)
{
	return otg_state;
}

static const struct usb_cfg_data usb_hotplug_config = {
	.cb_usb_status = usb_hotplug_status_cb,
};

static inline int usb_hotplug_detect_device(void)
{
	int ret;

	ret = usb_enable((struct usb_cfg_data *)&usb_hotplug_config);
	if (ret < 0) {
		SYS_LOG_ERR("enable");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_USB_HOST
static inline int usb_hotplug_host_plugin(void)
{
	usb_thread_init_host_scan();

	return 0;
}

static inline int usb_hotplug_host_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_A_OUT;

	return usb_thread_exit_host_scan();
}

#ifdef CONFIG_USB_HOST_STORAGE
extern void usb_stor_set_access(bool accessed);
extern bool usb_host_storage_enabled(void);

int usb_hotplug_disk_accessed(bool access)
{
	usb_stor_set_access(access);
#ifdef CONFIG_FS_MANAGER
	if (access) {
		fs_manager_disk_init("USB:");
	} else {
		fs_manager_disk_uninit("USB:");
	}
#endif

	return 0;
}

static inline int usb_hotplug_host_storage_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_A_IN;

	return 0;
}
#endif /* CONFIG_USB_HOST_STORAGE */
#endif /* CONFIG_USB_HOST */

#ifdef CONFIG_USB_DEVICE
static inline int usb_hotplug_device_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_B_IN;

	return 0;
}

static inline int usb_hotplug_device_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_B_OUT;

	return 0;
}

static inline int usb_hotplug_charger_plugin(void)
{
	usb_hotplug_state = USB_HOTPLUG_C_IN;

	return 0;
}

static inline int usb_hotplug_charger_plugout(void)
{
	usb_hotplug_state = USB_HOTPLUG_C_OUT;

	return 0;
}
#endif /* CONFIG_USB_DEVICE */

static inline uint8_t usb_hotplug_get_vbus(void)
{
#ifdef CONFIG_USB_DEVICE
	return usb_phy_get_vbus();
#else
	return USB_VBUS_LOW;	/* Host-only mode: never enter b_idle */
#endif
}

static inline void usb_hotplug_update_state(void)
{
	uint8_t vbus = usb_hotplug_get_vbus();
#ifdef CONFIG_USB_DEVICE
	bool dc_attached;
	uint8_t i;
#endif

#ifdef CONFIG_USB_DEVICE
#ifdef CONFIG_PM_DEVICE
	if (system_is_ready()) {
		if (vbus == USB_VBUS_HIGH) {
			if (!usb_wake_unlock) {
				sys_wake_lock_ext(PARTIAL_WAKE_LOCK,USB_WAKE_LOCK_USER);
				usb_wake_unlock = true;
			}
		} else {
			if (usb_wake_unlock) {
				sys_wake_unlock_ext(PARTIAL_WAKE_LOCK,USB_WAKE_LOCK_USER);
				usb_wake_unlock = false;
			}
		}
	}
#endif
#endif

	switch (otg_state) {
#ifdef CONFIG_USB_DEVICE
	case OTG_STATE_B_IDLE:
		if (vbus == USB_VBUS_LOW) {
			keep_in_b_idle = 0;
			otg_state = OTG_STATE_A_IDLE;
#ifndef CONFIG_USB_HOST
			usb_phy_reset();
#endif
			usb_hotplug_charger_plugout();
			break;
		}

		if (keep_in_b_idle >= KEEP_IN_B_IDLE_RETRY) {
			break;
		}

		usb_phy_reset();
		usb_phy_enter_b_idle();
		k_sleep(K_MSEC(3));	/* necessary */
repeat:
		dc_attached = usb_phy_dc_attached();
		for (i = 0; i < 4; i++) {
			k_busy_wait(1000);
			if (dc_attached != usb_phy_dc_attached()) {
				SYS_LOG_INF("dc_attached: %d, i: %d",
				       dc_attached, i);
				goto repeat;
			}
		}
		if (dc_attached) {
			keep_in_b_idle = 0;
			otg_state = OTG_STATE_B_WAIT_ACON;
			usb_hotplug_detect_device();
		} else {
			if (++keep_in_b_idle >= KEEP_IN_B_IDLE_RETRY) {
				SYS_LOG_INF("b_idle");
			}
			usb_phy_enter_a_idle();
			usb_hotplug_charger_plugin();
		}
		break;

	case OTG_STATE_B_WAIT_ACON:
		if (vbus == USB_VBUS_LOW) {
			usb_connect_count = 0;
			otg_state = OTG_STATE_A_IDLE;
			/* exit device mode */
			usb_disable();
			usb_hotplug_charger_plugout();
			break;
		}

		if (usb_phy_dc_connected()) {
			usb_connect_count = 0;
			otg_state = OTG_STATE_B_PERIPHERAL;
			/* enter device mode */
			usb_hotplug_device_plugin();
			break;
		}

		if (++usb_connect_count >= USB_CONNECT_COUNT_MAX) {
			SYS_LOG_DBG("connected to charger");

			keep_in_b_idle = KEEP_IN_B_IDLE_RETRY;
			usb_connect_count = 0;
			otg_state = OTG_STATE_B_IDLE;

			/* exit device mode */
			usb_disable();
			usb_hotplug_charger_plugin();
			break;
		}
		break;

	case OTG_STATE_B_PERIPHERAL:
		if (otg_b_peripheral_exiting) {
			/* wait for exit done */
			if (usb_phy_dc_detached()) {
				otg_state = OTG_STATE_A_IDLE;
				otg_b_peripheral_exiting = 0;
			}
			break;
		}

		if (vbus == USB_VBUS_LOW) {
			/* exit device mode */
			usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;

			/* wait for exit done */
			if (usb_phy_dc_detached()) {
				otg_state = OTG_STATE_A_IDLE;
				otg_b_peripheral_exiting = 0;
			}
			break;
		}

		if (usb_device_unconfigured()) {
			usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;
			SYS_LOG_INF("usb unconfigured");
			break;
		}
#ifdef CONFIG_USB_MASS_STORAGE
		if (usb_mass_storage_ejected()) {
			usb_hotplug_device_plugout();
			otg_b_peripheral_exiting = 1;
			SYS_LOG_INF("usb msc ejected");
			break;
		}
#endif
		break;
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
	case OTG_STATE_A_IDLE:
		if (vbus == USB_VBUS_HIGH) {
			otg_a_idle_count = 0;
			otg_state = OTG_STATE_B_IDLE;
			usbh_vbus_set(false);
			break;
		}

		/* Enable Vbus */
		usbh_vbus_set(true);

		if (usb_phy_hc_attached()) {
			otg_a_idle_count = 0;
			otg_state = OTG_STATE_A_WAIT_BCON;
			usb_phy_enter_a_wait_bcon();
			break;
		}

		/* reset Vbus */
		if (++otg_a_idle_count >= OTG_A_IDLE_MAX) {
			otg_a_idle_count = 0;
			usbh_vbus_set(false);
			break;
		}
		break;

	case OTG_STATE_A_WAIT_BCON:
		if (vbus == USB_VBUS_HIGH) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_B_IDLE;
			/* exit host mode */
			usbh_vbus_set(false);
			break;
		}

		if (usb_phy_hc_connected()) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_A_HOST;
			/* enter host mode */
			usb_hotplug_host_plugin();
			break;
		}

		if (++otg_a_wait_bcon_count >= OTG_A_WAIT_BCON_MAX) {
			otg_a_wait_bcon_count = 0;
			otg_state = OTG_STATE_A_IDLE;
			usbh_vbus_set(false);
			break;
		}
		break;

	case OTG_STATE_A_HOST:
		if (vbus == USB_VBUS_HIGH) {
			if (otg_a_host_exiting) {
				if (!usb_hotplug_host_plugout()) {
					otg_a_host_exiting = 0;
					otg_state = OTG_STATE_B_IDLE;
				}
				break;
			}

			/* exit host mode */
			usbh_vbus_set(false);
			if (usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 1;
			} else {
				otg_state = OTG_STATE_B_IDLE;
			}
			break;
		}

		/* handle disconnect and connnect quickly */
		if (otg_a_host_exiting) {
			if (!usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 0;
				otg_state = OTG_STATE_A_IDLE;
			}
			break;
		}

		if (usb_phy_hc_disconnected()) {
			/* exit host mode */
			usbh_vbus_set(false);
			if (usb_hotplug_host_plugout()) {
				otg_a_host_exiting = 1;
			} else {
				otg_state = OTG_STATE_A_IDLE;
			}
			break;
		}

#ifdef CONFIG_USB_HOST_STORAGE
		if (usb_host_storage_enabled()) {
			usb_hotplug_host_storage_plugin();
			break;
		}
#endif
		break;
#else /* CONFIG_USB_HOST */
	case OTG_STATE_A_IDLE:
		if (vbus == USB_VBUS_HIGH) {
			otg_state = OTG_STATE_B_IDLE;
			break;
		}
		break;
#endif

	default:
		break;
	}
}

static int usb_hotplug_loop(void)
{
	enum usb_otg_state old_state = otg_state;

	usb_hotplug_update_state();
	if (otg_state == old_state) {
		return 0;
	}
	SYS_LOG_INF("%s -> %s", state_string[old_state],
	       state_string[otg_state]);

	switch (otg_state) {
	case OTG_STATE_B_IDLE:
		usb_hotplug_dpdm_switch(false);
		break;

	case OTG_STATE_A_IDLE:
		usb_hotplug_dpdm_switch(true);
#ifdef CONFIG_USB_HOST
		usb_phy_reset();
		usb_phy_enter_a_idle();
#endif
		break;

	default:
		break;
	}

	return 0;
}

static inline int usb_hotplug_init(void)
{
	uint8_t vbus = usb_hotplug_get_vbus();

#ifdef CONFIG_USB_DEVICE
	/* suspend comes before hotplug and keep_in_b_idle maybe not reset */
	keep_in_b_idle = 0;
#endif

	if (vbus == USB_VBUS_HIGH) {
		usb_hotplug_dpdm_switch(false);
#ifdef CONFIG_USB_HOST
		usbh_vbus_set(false);
#endif
		otg_state = OTG_STATE_B_IDLE;
	} else {
		usb_hotplug_dpdm_switch(true);
#ifdef CONFIG_USB_HOST
		usb_phy_enter_a_idle();
#endif
		otg_state = OTG_STATE_A_IDLE;
	}

	SYS_LOG_INF("state %s", state_string[otg_state]);

	/* For a_idle, detect later */
	if (otg_state == OTG_STATE_B_IDLE) {
		usb_hotplug_loop();
	}

	return 0;
}

/*
 * Check if USB is attached to charger or host
 */
bool usb_hotplug_device_mode(void)
{
	if ((otg_state == OTG_STATE_B_WAIT_ACON) ||
	    (otg_state == OTG_STATE_B_PERIPHERAL)) {
	    return true;
	}

	return false;
}

int usb_hotplug_suspend(void)
{
	usb_hotplug_state = USB_HOTPLUG_SUSPEND;

#ifdef CONFIG_USB_HOST
	usbh_vbus_set(false);

	if (otg_state == OTG_STATE_A_HOST) {
		/* should be fast */
		while (usbh_disconnect()) {
			k_sleep(K_MSEC(10));
			SYS_LOG_INF("");
		}
	}
#endif

	usb_phy_exit();

	return 0;
}

int usb_hotplug_resume(void)
{
	usb_hotplug_state = HOTPLUG_NONE;

#ifdef CONFIG_USB_HOST
	if (otg_state == OTG_STATE_A_HOST) {
		/* broadcast when resume */
		struct app_msg	msg = {0};

		msg.type = MSG_HOTPLUG_EVENT;
		msg.cmd = HOTPLUG_USB_HOST;
		msg.value = HOTPLUG_OUT;

		SYS_LOG_INF("host");

		return send_async_msg("main", &msg);
	}
#endif

	return usb_hotplug_init();
}

#ifdef CONFIG_USB_HOST
/* check and enumerate USB device directly */
int usb_hotplug_host_check(void)
{
	int timeout = 100;	/* 1s */

	if (otg_state != OTG_STATE_A_IDLE) {
		return -EINVAL;
	}

	do {
		usb_hotplug_loop();

		switch (otg_state) {
		case OTG_STATE_A_WAIT_BCON:
			goto attached;
		case OTG_STATE_A_IDLE:
			break;
		default:
			goto exit;
		}

		k_sleep(K_MSEC(10));
	} while (--timeout);

	/* timeout */
	if (timeout == 0) {
		return -ENODEV;
	}

attached:
	timeout = 10;	/* 1s */

	do {
		usb_hotplug_loop();

		switch (otg_state) {
		case OTG_STATE_A_HOST:
			usbh_prepare_scan();
			usbh_scan_device();
		/* fall through */
		default:
			goto exit;
		case OTG_STATE_A_WAIT_BCON:
			break;
		}

		/* should be same as CONFIG_MONITOR_PERIOD */
		k_sleep(K_MSEC(100));
	} while (--timeout);

	/* timeout */
	if (timeout == 0) {
		return -ENODEV;
	}

exit:
	return 0;
}
#endif
static int hotplug_usb_get_type(void)
{
	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
	case USB_HOTPLUG_A_OUT:
		return HOTPLUG_USB_HOST;

	case USB_HOTPLUG_B_IN:
	case USB_HOTPLUG_B_OUT:
		return HOTPLUG_USB_DEVICE;
	}

	/* never */
	return 0;
}

static int hotplug_usb_detect(void)
{
	uint8_t old_state = usb_hotplug_state;

	if (usb_hotplug_state == USB_HOTPLUG_SUSPEND) {
		return HOTPLUG_NONE;
	}

	usb_hotplug_loop();

	if (usb_hotplug_state == old_state) {
		return HOTPLUG_NONE;
	}

	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
	case USB_HOTPLUG_B_IN:
		return HOTPLUG_IN;

	case USB_HOTPLUG_A_OUT:
	case USB_HOTPLUG_B_OUT:
		return HOTPLUG_OUT;
	}

	return HOTPLUG_NONE;
}

static int hotplug_usb_process(int device_state)
{
	int res = -1;

	SYS_LOG_INF("%d", usb_hotplug_state);

	switch (usb_hotplug_state) {
	case USB_HOTPLUG_A_IN:
#ifdef CONFIG_FS_MANAGER
		res = fs_manager_disk_init("USB:");
#endif
		break;

	case USB_HOTPLUG_A_OUT:
#ifdef CONFIG_FS_MANAGER
		res = fs_manager_disk_uninit("USB:");
#endif
		break;

	case USB_HOTPLUG_B_IN:
	case USB_HOTPLUG_B_OUT:
		res = 0;
		break;

	default:
		break;
	}

	return res;
}

#ifdef CONFIG_USB_DEVICE
static int hotplug_usb_device_get_state(void)
{
	if (otg_b_peripheral_exiting == 1) {
		return HOTPLUG_OUT;
	}

	if (otg_state == OTG_STATE_B_PERIPHERAL) {
		return HOTPLUG_IN;
	}

	return HOTPLUG_OUT;
}

static const struct hotplug_device_t hotplug_usb_device = {
	.type = HOTPLUG_USB_DEVICE,
	.get_state = hotplug_usb_device_get_state,
	.get_type = hotplug_usb_get_type,
	.hotplug_detect = hotplug_usb_detect,
	.fs_process = hotplug_usb_process,
};
#endif /* CONFIG_USB_DEVICE */

#ifdef CONFIG_USB_HOST
static int hotplug_usb_host_get_state(void)
{
	if (otg_state == OTG_STATE_A_HOST) {
		return HOTPLUG_IN;
	}

	return HOTPLUG_OUT;
}

static int __unused hotplug_usb_host_detect(void)
{
	return HOTPLUG_NONE;
}

static const struct hotplug_device_t hotplug_usb_host = {
	.type = HOTPLUG_USB_HOST,
	.get_state = hotplug_usb_host_get_state,
#ifndef CONFIG_USB
	.get_type = hotplug_usb_get_type,
	.hotplug_detect = hotplug_usb_detect,
	.fs_process = hotplug_usb_process,
#else
	.hotplug_detect = hotplug_usb_host_detect,
#endif
};
#endif /* CONFIG_USB_HOST */

int hotplug_usb_init(void)
{
#ifdef CONFIG_USB_DEVICE
	hotplug_device_register(&hotplug_usb_device);
#endif

#ifdef CONFIG_USB_HOST
	hotplug_device_register(&hotplug_usb_host);
#endif

	return 0;
}

#ifdef CONFIG_USB_HOTPLUG
static struct k_timer usb_hotplug_timer;

static void usb_hotplug_expiry_fn(struct k_timer *timer)
{
	switch (otg_state) {
	case OTG_STATE_B_WAIT_ACON:
	case OTG_STATE_B_PERIPHERAL:
		if (!usb_phy_get_vbus() && !usb_phy_dc_detached()) {
			usb_phy_dc_disconnect();
		}
		break;

	default:
		break;
	}
}
#endif /* CONFIG_USB_HOTPLUG */

int usb_hotplug_pre_init(const struct device *dev)
{
	ARG_UNUSED(dev);
#ifdef CONFIG_USB_HOTPLUG
	k_timer_init(&usb_hotplug_timer, usb_hotplug_expiry_fn, NULL);
	k_timer_start(&usb_hotplug_timer, K_SECONDS(1), K_MSEC(400));
#endif
	usb_phy_init();

#ifdef CONFIG_USB_HOST
	/* turn off Vbus by default */
	usbh_vbus_set(false);
#endif

	usb_hotplug_init();

	return 0;
}

#define USB_HOTPLUG_PRE_INIT_PRIORITY CONFIG_APPLICATION_INIT_PRIORITY
SYS_INIT(usb_hotplug_pre_init, POST_KERNEL, USB_HOTPLUG_PRE_INIT_PRIORITY);

