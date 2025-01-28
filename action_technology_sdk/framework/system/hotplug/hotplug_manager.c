/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug manager interface
 */

#if defined(CONFIG_SYS_LOG)
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "hotpug_manager"
#endif
#include <os_common_api.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <sys_monitor.h>
#include <string.h>
#include <hotplug_manager.h>
#ifdef CONFIG_TWS
#include "bt_manager.h"
#endif

static struct hotplug_manager_context_t hotplug_manager_context;

static struct hotplug_manager_context_t  *hotplug_manager;

int hotplug_device_register(const struct hotplug_device_t *device)
{
	const struct hotplug_device_t *temp_device = NULL;
	int free_index = -1;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		temp_device = hotplug_manager->device[i];
		if (temp_device && temp_device->type == device->type) {
			return -EEXIST;
		} else if (!temp_device) {
			free_index = i;
			break;
		}
	}
	if (free_index != -1 && free_index < MAX_HOTPLUG_DEVICE_NUM) {
		hotplug_manager->device[free_index] = device;
		hotplug_manager->device_num++;
	}
	return 0;
}

int hotplug_device_unregister(const struct hotplug_device_t *device)
{
	const struct hotplug_device_t *temp_device = NULL;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		temp_device = hotplug_manager->device[i];
		if (temp_device && temp_device->type == device->type) {
			hotplug_manager->device[i] = NULL;
			hotplug_manager->device_num--;
		}
	}
	return 0;
}

int hotplug_event_report(int device_type, int device_state)
{
	struct app_msg  msg = {0};

	msg.type = MSG_HOTPLUG_EVENT;
	msg.cmd = device_type;
	msg.value = device_state;
	SYS_LOG_INF("type: %d state: %d\n", device_type, device_state);
	return send_async_msg("main", &msg);
}

static int _hotplug_manager_work_handle(void)
{
	int state = HOTPLUG_NONE;
	const struct hotplug_device_t *device = NULL;

	/**slave not report hot plug*/
#ifdef CONFIG_TWS
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
		return 0;
	}
#endif

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		device = hotplug_manager->device[i];
		if (!device)
			continue;

		if (device->hotplug_detect) {
			state = device->hotplug_detect();
		}

		if (state != HOTPLUG_NONE) {
			if (device->fs_process) {
				if (device->fs_process(state)) {
					continue;
				}
			}
			if (device->get_type) {
				hotplug_event_report(device->get_type(), state);
			} else {
				hotplug_event_report(device->type, state);
			}
		}
	}

	return 0;
}

int hotplug_manager_get_state(int hotplug_device_type)
{
	int state = HOTPLUG_NONE;
	const struct hotplug_device_t *device = NULL;

	for (int i = 0; i < MAX_HOTPLUG_DEVICE_NUM; i++) {
		device = hotplug_manager->device[i];
		if (device->type == hotplug_device_type) {
			state = device->get_state();
			break;
		}
	}

	return state;
}

int hotplug_manager_init(void)
{
	if (hotplug_manager)
		return -EEXIST;

	hotplug_manager = &hotplug_manager_context;

	memset(hotplug_manager, 0, sizeof(struct hotplug_manager_context_t));

#ifdef CONFIG_LINEIN_HOTPLUG
	hotplug_linein_init();
#endif

#ifdef CONFIG_CARD_HOTPLUG
	hotplug_sdcard_init();
#endif

#if defined(CONFIG_USB_HOTPLUG)
	hotplug_usb_init();
#endif

#ifdef CONFIG_CHARGER_HOTPLUG
	hotplug_charger_init();
#endif

	sys_monitor_add_work(_hotplug_manager_work_handle);
	return 0;
}
