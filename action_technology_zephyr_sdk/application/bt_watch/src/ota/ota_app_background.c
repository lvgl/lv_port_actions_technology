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
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>
#include <ota_backend_bt.h>
#include <ota_storage.h>
#include <drivers/flash.h>
#include "ota_app.h"
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#ifdef CONFIG_BLUETOOTH
#include "mem_manager.h"
#endif
#ifdef CONFIG_FILE_SYSTEM
#include <fs_manager.h>
#include <fs/fs.h>
#endif
#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"

#ifdef CONFIG_OTA_RECOVERY
#define OTA_STORAGE_DEVICE_NAME		CONFIG_OTA_TEMP_PART_DEV_NAME
#else
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#endif

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

static bool is_sd_ota;

static void ota_app_start(void)
{
	struct app_msg  msg = {0};
	uint8_t reserve = 0;

	if (!srv_manager_check_service_is_actived(APP_ID_OTA)) {
		srv_manager_active_service(APP_ID_OTA);
	}

	SYS_LOG_INF("ok");
	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
	 	flash_write_protection_set(flash_device, false);

	msg.type = MSG_START_APP;
	msg.reserve = reserve;

	send_async_msg(APP_ID_OTA, &msg);
}

static void ota_app_stop(void)
{
	SYS_LOG_INF("ok");
	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
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
}
static bool check_sd_ota_restart(void)
{
#ifdef CONFIG_PROPERTY
	char sd_ota_flag[4] = { 0 };

	property_get("SD_OTA_FLAG", sd_ota_flag, 4);
	if (!memcmp(sd_ota_flag, "yes", strlen("yes"))) {
		property_set("SD_OTA_FLAG", "no", 4);
		property_flush(NULL);
		return true;
	}
#endif
	return false;
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
	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			if (backend != backend_bt) {
				if (check_sd_ota_restart()) {
					SYS_LOG_INF("sd ota restart\n");
					return;
				}
				is_sd_ota = true;
			} else {
				is_sd_ota = false;
			}
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

			fs = ota_app_create_file_handle("/SD:/ota.bin");
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
		}
	}
}

#ifdef CONFIG_OTA_BACKEND_SDCARD
struct ota_backend_sdcard_init_param sdcard_init_param = {
	.fpath = "/SD:/ota.bin",
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
	.read_timeout = OS_FOREVER,	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
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

static void sys_reboot_by_ota(void)
{
	struct app_msg  msg = {0};
	msg.type = MSG_REBOOT;
	msg.cmd = REBOOT_REASON_OTA_FINISHED;
	send_async_msg("main", &msg);
}

void ota_install_start(void)
{
#ifdef CONFIG_PROPERTY
	property_set("REC_OTA_FLAG", "yes", 4);
#endif
	ota_view_deinit();

	sys_reboot_by_ota();
}
void ota_view_exit(void)
{
	ota_view_deinit();
}

void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);

	if (old_state != OTA_RUNNING && state == OTA_RUNNING) {
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
#endif

	} else if (old_state == OTA_RUNNING && state != OTA_RUNNING) {
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
#endif
	}
	if (state == OTA_DONE) {
		if (is_sd_ota) {
		#ifdef CONFIG_PROPERTY
			property_set("SD_OTA_FLAG", "yes", 4);
		#endif
		}
		ota_view_init();
		//os_sleep(1000);
		//sys_reboot_by_ota();
	}
}

int ota_app_init(void)
{
	struct ota_upgrade_param param;

	SYS_LOG_INF("device name %s ", OTA_STORAGE_DEVICE_NAME);

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = OTA_STORAGE_DEVICE_NAME;
	param.notify = ota_app_notify;
#ifdef CONFIG_OTA_RECOVERY
	param.upg_res_and_temp = 1;
	param.flag_use_recovery_app = 0;
#endif
	param.no_version_control = 1;

	struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
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
	return 0;
}

static void ota_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;

	SYS_LOG_INF("enter");

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_START_APP:
			#ifdef CONFIG_SYS_WAKELOCK
				sys_wake_lock(PARTIAL_WAKE_LOCK);
			#endif
				bt_manager_stop_auto_reconnect();		/* Stop reconnect when start ota  */
				ota_upgrade_check(g_ota);
				ota_app_stop();
			#ifdef CONFIG_SYS_WAKELOCK
				sys_wake_unlock(PARTIAL_WAKE_LOCK);
			#endif
				break;

			case MSG_EXIT_APP:
				terminaltion = true;
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
}

//static char __stack_noinit __aligned(ARCH_STACK_PTR_ALIGN) ota_stack[2048];
#define OTA_STACK_SIZE	2048
static OS_THREAD_STACK_DEFINE(ota_stack, OTA_STACK_SIZE);

SERVICE_DEFINE(ota, ota_stack, OTA_STACK_SIZE, 14,
	   BACKGROUND_APP, NULL, NULL, NULL, ota_app_main);

