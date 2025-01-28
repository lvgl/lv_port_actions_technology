/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system app main
 */
#include <mem_manager.h>
#include <msg_manager.h>
#include <fw_version.h>
#include <sys_event.h>
#include "app_switch.h"
#include <hotplug_manager.h>
#include <input_manager.h>
#include <thread_timer.h>
#include <stream.h>
#include <property_manager.h>
#include <fs_manager.h>
#include <usb/usb_device.h>
#include <usb/class/usb_msc.h>
#include <soc.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#include <ui_mem.h>
#endif
#include <sys_wakelock.h>
#include <drivers/usb/usb_phy.h>

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif

#ifdef CONFIG_BLUETOOTH
#include <bt_manager.h>
#endif

#include "card_reader.h"

struct cardreader_app_t {
	struct thread_timer monitor_timer;
	u32_t card_plugin:1;
	u32_t card_write_flag:1;
	u32_t card_state_change_cnt:8;
};
static bool card_read_init_ok;

static void card_reader_moditor_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct cardreader_app_t *card_reader = (struct cardreader_app_t *)expiry_fn_arg;
	/**stub mode */
	if (usb_mass_storage_stub_mode()) {
		struct app_msg  msg = {0};
		msg.type = MSG_HOTPLUG_EVENT;
		msg.cmd = HOTPLUG_USB_STUB;
		msg.value = HOTPLUG_IN;
		send_async_msg("main", &msg);
	}

	if (!usb_hotplug_device_mode() || usb_mass_storage_ejected()) {
		struct app_msg  msg = {0};
		msg.type = MSG_HOTPLUG_EVENT;
		msg.cmd = HOTPLUG_USB_DEVICE;
		msg.value = HOTPLUG_OUT;
		send_async_msg("main", &msg);
	}

	if (usb_mass_storage_working()) {
		card_reader->card_write_flag = 1;
		card_reader->card_state_change_cnt = 0;
#ifdef CONFIG_UI_MANAGER
		card_reader_show_storage_working();
#endif
	} else {
		card_reader->card_state_change_cnt ++;
		if(card_reader->card_state_change_cnt >= 5) {
			card_reader->card_state_change_cnt = 0;
			card_reader->card_write_flag = 0;
			if(card_reader->card_plugin) {
#ifdef CONFIG_UI_MANAGER
				card_reader_show_card_plugin();
#endif
			}
		}
	}
}

int card_reader_init(void);
struct cardreader_app_t *card_reader_monitor = NULL;

static void card_reader_init_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if(usb_hotplug_get_otgstate()==OTG_STATE_B_PERIPHERAL) {
#ifdef CONFIG_UI_MANAGER
		card_reader_view_init();
#endif
		if (!card_reader_init()) {
			card_reader_monitor = app_mem_malloc(sizeof(struct cardreader_app_t));
			if (!card_reader_monitor) {
				return ;
			}
			card_read_init_ok = true;
#ifdef CONFIG_THREAD_TIMER
			thread_timer_init(&card_reader_monitor->monitor_timer, card_reader_moditor_handle, card_reader_monitor);
			thread_timer_start(&card_reader_monitor->monitor_timer, 400, 400);
#endif
		}
	} else {
		struct app_msg  msg = {0};
		msg.type = MSG_HOTPLUG_EVENT;
		msg.cmd = HOTPLUG_USB_DEVICE;
		msg.value = HOTPLUG_OUT;
		send_async_msg("main", &msg);
	}
}

#ifndef CONFIG_UI_MANAGER
#ifdef CONFIG_USB_MASS_STORAGE
#define PAGE_SIZE	CONFIG_MASS_STORAGE_BUF_SIZE
static u8_t data_buf[PAGE_SIZE*2] __aligned(4) ;
#endif
#else
static void *data_buf = NULL;
#endif

int card_reader_init(void)
{
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "card_reader");
#endif

#ifdef CONFIG_FS_MANAGER
	fs_manager_disk_uninit(CONFIG_APP_FAT_DISK);
#endif

#ifdef CONFIG_HOTPLUG
#ifdef CONFIG_USB_MASS_STORAGE
	usb_disable();
	usb_phy_enter_b_idle();
	usb_phy_init();
#ifdef CONFIG_UI_MANAGER
	data_buf = ui_mem_gui_alloc(CONFIG_MASS_STORAGE_BUF_SIZE * 2); 
#endif
	usb_mass_storage_init(NULL, data_buf, CONFIG_MASS_STORAGE_BUF_SIZE * 2);
#endif

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_enter_high_speed();
#endif
#endif

	return 0;
}

int card_reader_deinit(void)
{
#ifdef CONFIG_USB_MASS_STORAGE
	usb_mass_storage_exit();
#ifdef CONFIG_UI_MANAGER
	if (data_buf) {
		ui_mem_gui_free(data_buf);
	}
#endif
#endif

#ifdef CONFIG_FS_MANAGER
	fs_manager_disk_init(CONFIG_APP_FAT_DISK);
#endif

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_exit_high_speed();
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "card_reader");
#endif

	return 0;
}

int card_reader_mode_check(void)
{
	struct app_msg msg = {0};
	int result = 0;
	int res = 0;
	bool terminaltion = false;
	bool exit_by_stub = false;
	struct cardreader_app_t *card_reader = NULL;

	if (!usb_hotplug_device_mode()) {
		return 0;
	}

	card_reader = app_mem_malloc(sizeof(struct cardreader_app_t));
	if (!card_reader) {
		return -ENOMEM;
	}
	if (usb_hotplug_get_otgstate()==OTG_STATE_B_PERIPHERAL) {
#ifdef CONFIG_UI_MANAGER
		card_reader_view_init();
#endif
		if (!card_reader_init()) {
			card_read_init_ok = true;
		}
#ifdef CONFIG_THREAD_TIMER
		thread_timer_init(&card_reader->monitor_timer, card_reader_moditor_handle, card_reader);
		thread_timer_start(&card_reader->monitor_timer, 400, 400);
#endif
	} else if (usb_hotplug_get_otgstate()==OTG_STATE_B_WAIT_ACON){
#ifdef CONFIG_THREAD_TIMER
		thread_timer_init(&card_reader->monitor_timer, card_reader_init_handle, card_reader);
		thread_timer_start(&card_reader->monitor_timer, 1000, 0);
#endif
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_HOTPLUG
	if (hotplug_manager_get_state(HOTPLUG_SDCARD) == HOTPLUG_IN) {
		card_reader->card_plugin = 1;
	}
#endif

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			switch (msg.type) {
            case MSG_KEY_INPUT:
				if (msg.value == (KEY_POWER | KEY_TYPE_SHORT_UP)
					|| msg.value == (KEY_MENU | KEY_TYPE_SHORT_UP)) {
					if (!card_reader_monitor->card_write_flag) {
						terminaltion = true;
					}
				}
				break;
		#ifdef CONFIG_UI_MANAGER
			case MSG_UI_EVENT:
				//ui_message_dispatch(msg.sender, msg.cmd, msg.value);
               	break;
		#endif
		#ifdef CONFIG_PLAYTTS
			case MSG_TTS_EVENT:
				if (msg.cmd == TTS_EVENT_START_PLAY) {
					tts_manager_play_process();
				}
				break;
		#endif
			case MSG_HOTPLUG_EVENT:
				if (msg.value == HOTPLUG_OUT) {
					switch (msg.cmd) {
						case HOTPLUG_USB_DEVICE:
							terminaltion = true;
							break;
						case HOTPLUG_SDCARD:
						#ifdef CONFIG_USB_MASS_STORAGE
                           usb_mass_storage_remove_lun();
                  		#endif
						#ifdef CONFIG_SEG_LED_MANAGER
							if (card_read_init_ok) {
								seg_led_display_icon(SLED_SD, false);
							}
						#endif
							card_reader->card_plugin = 0;
							break;
					}
				} else if (msg.value == HOTPLUG_IN) {
					if (msg.cmd == HOTPLUG_SDCARD) {
					#ifdef CONFIG_SEG_LED_MANAGER
						if (card_read_init_ok) {
							seg_led_display_icon(SLED_SD, true);
						}
					#endif
					#ifdef CONFIG_USB_MASS_STORAGE
						usb_mass_storage_add_lun(MSC_DISK_SD);
					#endif
						card_reader->card_plugin = 1;
					} else if (msg.cmd == HOTPLUG_USB_STUB) {
						terminaltion = true;
						//app_switch_remove_app(APP_ID_USOUND);
						exit_by_stub = true;
					}
				}
				break;
			default:
				SYS_LOG_ERR("error type: 0x%x! \n", msg.type);
				continue;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}
		thread_timer_handle_expired();
	}

	if (card_read_init_ok) {
		card_reader_deinit();
	}

	thread_timer_stop(&card_reader->monitor_timer);
#ifdef CONFIG_UI_MANAGER
	card_reader_view_deinit();
#endif

	app_mem_free(card_reader);


#if defined(CONFIG_TOOL) && defined(CONFIG_STUB_USB)
	/* not init bt manager when connected att */
	if (exit_by_stub && tool_init() == STUB_PC_TOOL_ATT_MODE) {
		res = NO_NEED_INIT_BT;
	}
#endif

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif
	return res;

}
