/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file task wdt manager interface
 */
#include <task_wdt_manager.h>
#include <sys/reboot.h>
#include <msg_manager.h>

extern void show_stack(void);
static void task_wdt_callback(int channel_id, void *user_data)
{
	os_printk("Task watchdog channel %d callback, thread: %s\n",
		channel_id, k_thread_name_get((k_tid_t)user_data));

	/*
	 * If the issue could be resolved, call task_wdt_feed(channel_id) here
	 * to continue operation.
	 *
	 * Otherwise we can perform some cleanup and reset the device.
	 */
	//show_stack();

	//os_printk("Resetting device...\n");

	//sys_reboot(SYS_REBOOT_COLD);
	k_panic();
}

int task_wdt_start(uint32_t reload_period)
{
	return task_wdt_add(reload_period, task_wdt_callback, os_current_get());
}

int task_wdt_stop(void)
{
	int channel_id = msg_manager_get_current_channel_id();
	
	if (channel_id >= 0) {
		return task_wdt_delete(channel_id);
	}

	return 0;
}

int task_wdt_user_feed(void)
{
	int channel_id = msg_manager_get_current_channel_id();
	if (channel_id >= 0) {
		return task_wdt_feed(channel_id);
	}

	return -EINVAL;
}

int task_wdt_manager_init(void)
{
	const struct device *hw_wdt_dev = NULL;

#ifdef CONFIG_WATCHDOG
	hw_wdt_dev = device_get_binding(CONFIG_WDT_ACTS_NAME);
	if (!hw_wdt_dev) {
		SYS_LOG_ERR("cannot found watchdog device");
		return -1;
	}
	if (!device_is_ready(hw_wdt_dev)) {
		os_printk("Hardware watchdog %s is not ready; ignoring it.\n",
		       hw_wdt_dev->name);
		hw_wdt_dev = NULL;
	}
#endif

	task_wdt_init(hw_wdt_dev);

	return 0;
}