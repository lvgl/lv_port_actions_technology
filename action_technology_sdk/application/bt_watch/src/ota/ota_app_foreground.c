/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <thread_timer.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <sys_wakelock.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>
#include <ota_backend_bt.h>
#include <ota_storage.h>
#include "ota_app.h"
#ifdef CONFIG_TASK_WDT
#include <task_wdt_manager.h>
#endif
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <drivers/flash.h>
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#ifdef CONFIG_FILE_SYSTEM
#include <fs_manager.h>
#include <fs/fs.h>
#endif
#include <soc_pstore.h>
#include <partition/partition.h>
#include <sdfs.h>
#include "../launcher/clock_selector/clock_selector.h"
#include "app_ui.h"

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
#define OTA_STORAGE_EXT_DEVICE_NAME "spi_flash"
#endif

#ifdef CONFIG_OTA_RECOVERY
#define OTA_STORAGE_DEVICE_NAME		CONFIG_OTA_TEMP_PART_DEV_NAME
#else
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#endif
#ifdef CONFIG_OTA_BACKEND_SDCARD
#define SDCARD_OTA_FILE				CONFIG_APP_FAT_DISK"/ota.bin"
#endif

#define OS_OTA_READ_TIMEOUT (60*1000) //60s by read timeout

static struct ota_upgrade_info *g_ota;
#ifdef CONFIG_OTA_BACKEND_SDCARD
static struct ota_backend *backend_sdcard;
#endif
#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
static struct ota_backend *backend_bt;
#endif
#ifdef CONFIG_OTA_RECOVERY
#ifdef CONFIG_FILE_SYSTEM
#ifdef CONFIG_OTA_STORAGE_FS
static struct fs_file_t *fs = NULL;
static void ota_app_close_file_handle(void);
#endif
#endif
#endif

static bool is_need_start;
struct res_inc_file_info_t {
	uint8_t file_valid;
	uint8_t id1;
	uint8_t id2;
	uint8_t id3;
};
static struct res_inc_file_info_t g_file_info;
bool ota_is_already_done(void);

static void ota_app_start(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ota app start");

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	is_need_start = true;

	if (memcmp(app_manager_get_current_app(), "ota", strlen("ota")) == 0) {
		msg.type = MSG_START_APP;
		send_async_msg("ota", &msg);
	} else {
		msg.type = MSG_START_APP;
		msg.ptr = APP_ID_OTA;
		msg.reserve = APP_SWITCH_CURR;
		send_async_msg(APP_ID_MAIN, &msg);
	}

	app_switch_lock(1);
}

static void ota_app_stop(void)
{
	struct app_msg msg = {0};

	SYS_LOG_INF("ok");

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, true);

#ifdef CONFIG_OTA_RECOVERY
#ifdef CONFIG_FILE_SYSTEM
#ifdef CONFIG_OTA_STORAGE_FS
	ota_storage_unbind_fs(fs);
	ota_app_close_file_handle();
#endif
#endif
#endif

	msg.type = MSG_START_APP;
	msg.ptr = NULL;
	msg.reserve = APP_SWITCH_LAST;
	send_async_msg(APP_ID_MAIN, &msg);
}

#ifdef CONFIG_OTA_RECOVERY
#ifdef CONFIG_FILE_SYSTEM
#ifdef CONFIG_OTA_STORAGE_FS
static struct fs_file_t *ota_app_create_file_handle(const char *fpath)
{
	struct fs_file_t *ff = app_mem_malloc(sizeof(struct fs_file_t));

	if (ff == NULL)
		return NULL;

	if (fs_open(ff, fpath, FA_READ | FA_WRITE | FA_OPEN_ALWAYS)) {
		app_mem_free(ff);
		return NULL;
	}
	return ff;
}
static void ota_app_close_file_handle(void)
{
	if (fs) {
		fs_close(fs);
		app_mem_free(fs);
		fs = NULL;
	}
}
#endif
#endif
#endif
extern void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p cmd %d state %d", backend, cmd, state);

	if (!strcmp(APP_ID_BTCALL, app_manager_get_current_app())) {
		SYS_LOG_WRN("btcall unsupport ota, skip...");
		return;
	}
	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			SYS_LOG_INF("bt_manager_allow_sco_connect false\n");
#ifdef CONFIG_BT_HFP_HF
			bt_manager_allow_sco_connect(false);
#endif
			err = ota_upgrade_attach_backend(g_ota, backend);
			if (err) {
				SYS_LOG_INF("unable attach backend");
				return;
			}
		#ifdef CONFIG_OTA_RECOVERY
		#ifdef CONFIG_FILE_SYSTEM
		#ifdef CONFIG_OTA_STORAGE_FS
			if (fs) {
				SYS_LOG_WRN("already run\n");
				return;
			}

			fs = ota_app_create_file_handle(SDCARD_OTA_FILE);
			if (!fs) {
				SYS_LOG_ERR("create failed\n");
				return;
			}
			err = ota_storage_bind_fs(fs);
			if (err) {
				ota_app_close_file_handle();
				SYS_LOG_INF("unable attach fs");
				return;
			}
		#endif
		#endif
		#endif
			ota_app_start();
		} else {
			ota_upgrade_detach_backend(g_ota, backend);
			SYS_LOG_INF("bt_manager_allow_sco_connect true\n");
#ifdef CONFIG_BT_HFP_HF
			bt_manager_allow_sco_connect(true);
#endif
		}
	} else if (cmd == OTA_BACKEND_UPGRADE_PROGRESS) {
#ifdef CONFIG_UI_MANAGER
		ota_view_show_upgrade_progress(state);
#endif
	} else if (cmd == OTA_BACKEND_UPGRADE_EXIT) {
		if (bt_manager_ble_is_connected()) {
			bt_manager_ble_disconnect();
		}
	}
}

#ifdef CONFIG_OTA_BACKEND_SDCARD
struct ota_backend_sdcard_init_param sdcard_init_param = {
	.fpath = SDCARD_OTA_FILE,
};

int ota_app_init_sdcard(void)
{
	backend_sdcard = ota_backend_sdcard_init(ota_app_backend_callback, &sdcard_init_param);
	if (!backend_sdcard) {
		SYS_LOG_INF("failed to init sdcard ota");
		return -ENODEV;
	}

	return 0;
}
#endif

#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
/* UUID order using BLE format */
/*static const uint8_t ota_spp_uuid[16] = {0x00,0x00,0x66,0x66, \
	0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};*/

/* "00006666-0000-1000-8000-00805F9B34FB"  ota uuid spp */
static const uint8_t ota_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00};


/* BLE */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

static struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(OTA_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

static const struct ota_backend_bt_init_param bt_init_param = {
	.spp_uuid = ota_spp_uuid,
	.gatt_attr = &ota_gatt_attr[0],
	.attr_size = ARRAY_SIZE(ota_gatt_attr),
	.tx_chrc_attr = &ota_gatt_attr[3],
	.tx_attr = &ota_gatt_attr[4],
	.tx_ccc_attr = &ota_gatt_attr[5],
	.rx_attr = &ota_gatt_attr[2],
	.read_timeout = OS_OTA_READ_TIMEOUT, //OS_FOREVER,	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	.write_timeout = OS_FOREVER,
};

int ota_app_init_bluetooth(void)
{
	SYS_LOG_INF("spp uuid\n");
	//print_buffer(bt_init_param.spp_uuid, 1, 16, 16, 0);

	backend_bt = ota_backend_bt_init(ota_app_backend_callback,
		(struct ota_backend_bt_init_param *)&bt_init_param);
	if (!backend_bt) {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}

	return 0;
}
#endif

static void sys_clear_remain_msg(void)
{
	struct app_msg  msg;
	/*clear all remain message */
	while (receive_msg(&msg, 0)) {
		if (msg.callback) {
			msg.callback(&msg, 0, NULL);
		}
	}
#ifdef CONFIG_TASK_WDT
	task_wdt_stop();
#endif
}

static void sys_reboot_by_ota(void)
{
	struct app_msg  msg = {0};
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_OTA_FINISHED;
	send_async_msg("main", &msg);
}
static void ota_app_start_ota_upgrade(void)
{
	struct app_msg msg = {0};

	msg.type = MSG_START_APP;
	send_async_msg(APP_ID_OTA, &msg);
}
static int get_res_inc_file_info(struct res_inc_file_info_t *file_info)
{
#ifdef CONFIG_PROPERTY
	return property_get("RES_FILE_INFO", (char *)file_info, sizeof(struct res_inc_file_info_t));
#endif
	return -EINVAL;
}
static void set_res_inc_file_info(struct res_inc_file_info_t *file_info)
{
#ifdef CONFIG_PROPERTY
	property_set("RES_FILE_INFO", (char *)file_info, sizeof(struct res_inc_file_info_t));
	property_flush(NULL);
#endif
}

int res_inc_file_check(void)
{
	int ret = 0;

	if (get_res_inc_file_info(&g_file_info) > 0) {
		SYS_LOG_INF("file_valid:%d,id1:%d\n", g_file_info.file_valid, g_file_info.id1);
		if (g_file_info.file_valid) {
			if (g_file_info.id1 == PARTITION_FILE_ID_SDFS_PART1) {
				ret = sdfs_verify(CONFIG_APP_UI_DISK_B);
			}
		}
	}

	return ret;
}

void ota_app_file_cb(uint8_t file_id)
{
	if (file_id == PARTITION_FILE_ID_SDFS_PART0) {
		memset(&g_file_info, 0, sizeof(struct res_inc_file_info_t));
		set_res_inc_file_info(&g_file_info);
	} else if (file_id == PARTITION_FILE_ID_SDFS_PART1) {
		g_file_info.file_valid = 1;
		g_file_info.id1 = PARTITION_FILE_ID_SDFS_PART1;
		set_res_inc_file_info(&g_file_info);
	}
}

void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);

	if (old_state != OTA_RUNNING && state == OTA_RUNNING) {
		sys_clear_remain_msg();
	#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	#ifdef CONFIG_PROPERTY
		property_set("OTA_UPG_FLAG", "run", 5);
		property_flush(NULL);
	#endif
	} else if (old_state == OTA_RUNNING && state != OTA_RUNNING) {
	#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	}
	if (state == OTA_DONE) {
		sys_clear_remain_msg();
	#ifdef CONFIG_UI_MANAGER
		ota_view_show_upgrade_result("Succ", false);
	#endif
	#ifdef CONFIG_PROPERTY
		property_set("REC_OTA_FLAG", "yes", 4);
		property_set("OTA_UPG_FLAG", "done", 5);
		property_flush(NULL);
	#endif
		/*Set hardware flags to ensure enter ota upgrade durring bootloader*/
		soc_pstore_set(SOC_PSTORE_TAG_OTA_UPGRADE, 1);
#ifdef CONFIG_UI_MANAGER
		ota_view_deinit();
#endif
		app_switch_unlock(1);

		sys_reboot_by_ota();
	}
}

int ota_app_init(void)
{
	struct ota_upgrade_param param;

#ifdef CONFIG_OTA_BACKEND_SDCARD
	struct fs_dirent entry;
	int err = fs_stat(SDCARD_OTA_FILE, &entry);
	if (!err) {
		SYS_LOG_INF("unlink file %s ", SDCARD_OTA_FILE);
		fs_unlink(SDCARD_OTA_FILE);
	}
#endif

	SYS_LOG_INF("device name %s ", OTA_STORAGE_DEVICE_NAME);

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = OTA_STORAGE_DEVICE_NAME;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
    param.storage_ext_name = OTA_STORAGE_EXT_DEVICE_NAME;
#endif
	param.notify = ota_app_notify;
	param.file_cb = ota_app_file_cb;
#ifdef CONFIG_OTA_RECOVERY
	param.flag_use_recovery = 1;
	param.flag_erase_part_for_upg = 1;

	param.flag_use_recovery_app = 0;
#endif
	param.no_version_control = 1;

	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	g_ota = ota_upgrade_init(&param);
	if (!g_ota) {
		SYS_LOG_INF("init failed");
		if (flash_device)
			flash_write_protection_set(flash_device, true);
		return -1;
	}

	if (flash_device)
		flash_write_protection_set(flash_device, true);

#ifdef CONFIG_PROPERTY
	if(ota_is_already_done()) {
		property_set("OTA_UPG_FLAG", "init", 5);
		property_flush(NULL);
	}
#endif

	return 0;
}

bool ota_is_already_running(void)
{
	char ota_flag[6] = { 0 };

	property_get("OTA_UPG_FLAG", ota_flag, 5);
	if (!memcmp(ota_flag, "run", strlen("run"))) {
		return true;
	}
	return false;
}

bool ota_is_already_done(void)
{
	char ota_flag[6] = { 0 };

	property_get("OTA_UPG_FLAG", ota_flag, 5);
	if (!memcmp(ota_flag, "done", strlen("done"))) {
		return true;
	}
	return false;
}

static int ota_type_process_allow(int type)
{
	SYS_LOG_INF("ota_type %d, is not factory?", type);
	return 0;
}

static void ota_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;

	SYS_LOG_INF("enter");
#ifdef CONFIG_UI_MANAGER
	ota_view_init();
#endif
	if (is_need_start)
		ota_app_start_ota_upgrade();

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif
	ota_backend_ota_type_cb_set(ota_type_process_allow);

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_START_APP:
#ifdef CONFIG_TASK_WDT
				task_wdt_stop();
#endif
#if CONFIG_BT_BR_ACTS
				bt_manager_stop_auto_reconnect();		/* Stop reconnect when start ota  */
#endif
#ifdef CONFIG_ACTLOG
				actlog_increase_log_thread_priority();
#endif				
				if (ota_upgrade_check(g_ota)) {
#ifdef CONFIG_UI_MANAGER
					ota_view_show_upgrade_result("Fail", true);
#endif
					if (!ota_is_already_running()) {
						app_switch_unlock(1);
						ota_app_stop();
					}
				} else {
#ifdef CONFIG_UI_MANAGER
					ota_view_show_upgrade_result("Succ", false);
#endif
				}
#ifdef CONFIG_ACTLOG
				actlog_decrease_log_thread_priority();
#endif				
				break;

			case MSG_EXIT_APP:
				terminaltion = true;
#ifdef CONFIG_UI_MANAGER
				ota_view_deinit();
#endif
				app_manager_thread_exit(APP_ID_OTA);
				break;

			default:
				SYS_LOG_ERR("unknown: 0x%x!", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}

		if (!terminaltion) {
			thread_timer_handle_expired();
		}
	}

	ota_backend_ota_type_cb_set(NULL);
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif
}

APP_DEFINE(ota, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, NULL, NULL, NULL,
	   ota_app_main, NULL);

