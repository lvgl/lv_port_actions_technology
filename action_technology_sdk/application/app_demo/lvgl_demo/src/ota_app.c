/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bt_manager.h>
#include <sys_manager.h>
#include <sys_wakelock.h>
#include <ota_upgrade.h>
#include <ota_backend_bt.h>
#include <drivers/flash.h>
#include <board_cfg.h>
#ifdef CONFIG_TASK_WDT
#include <task_wdt_manager.h>
#endif
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include <soc_pstore.h>
#include "ota_app.h"
#include "app_msg.h"

static struct ota_upgrade_info *g_ota;

static void ota_app_start(void)
{
	SYS_LOG_INF("ota app start");

	const struct device *flash_device = device_get_binding(CONFIG_SPI_FLASH_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);

	// send msg to app
	app_msg_send(APP_NAME, MSG_OTA, 0);
}

static void ota_app_stop(void)
{
	SYS_LOG_INF("ota app stop");

	const struct device *flash_device = device_get_binding(CONFIG_SPI_FLASH_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, true);
}

static void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p cmd %d state %d", backend, cmd, state);

	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			err = ota_upgrade_attach_backend(g_ota, backend);
			if (err) {
				SYS_LOG_INF("unable attach backend");
				return;
			}
			ota_app_start();
		} else {
			ota_upgrade_detach_backend(g_ota, backend);
		}
	} else if (cmd == OTA_BACKEND_UPGRADE_PROGRESS) {
		SYS_LOG_INF("ota progress: %d", state);
	} else if (cmd == OTA_BACKEND_UPGRADE_EXIT) {
		if (bt_manager_ble_is_connected()) {
			bt_manager_ble_disconnect();
		}
	}
}

#ifdef CONFIG_OTA_BACKEND_BLUETOOTH
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
	.spp_uuid = NULL,
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
	struct ota_backend *backend_bt = ota_backend_bt_init(ota_app_backend_callback,
		(struct ota_backend_bt_init_param *)&bt_init_param);
	if (!backend_bt) {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}

	return 0;
}
#endif

void ota_app_file_cb(uint8_t file_id)
{
	SYS_LOG_INF("ota file_id: %d", file_id);
}

void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);

	if (old_state != OTA_RUNNING && state == OTA_RUNNING) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	#ifdef CONFIG_PROPERTY
		property_set("OTA_UPG_FLAG", "run", 5);
		property_flush(NULL);
	#endif
	} else if (old_state == OTA_RUNNING && state != OTA_RUNNING) {
	#ifdef CONFIG_DVFS_DYNAMIC_LEVEL
		dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "ota");
	#endif
	}
	if (state == OTA_DONE) {
	#ifdef CONFIG_PROPERTY
		property_set("REC_OTA_FLAG", "yes", 4);
		property_set("OTA_UPG_FLAG", "done", 5);
		property_flush(NULL);
	#endif
		ota_app_reboot(0);
	}
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

int ota_app_init(void)
{
	struct ota_upgrade_param param;

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));
	param.storage_name = CONFIG_SPI_FLASH_NAME;
	param.notify = ota_app_notify;
	param.file_cb = ota_app_file_cb;
	param.flag_use_recovery = 1;
	param.flag_erase_part_for_upg = 1;
	param.flag_use_recovery_app = 0;
	param.no_version_control = 1;

	/* keep temp-part on boot, erase temp-part when ota start */
	param.flag_keep_temp_part = 1;

	const struct device *flash_device = device_get_binding(CONFIG_SPI_FLASH_NAME);
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
	property_set("OTA_UPG_FLAG", "init", 5);
	property_flush(NULL);
#endif

	return 0;
}

int ota_app_process(void)
{
	int ret, prio;

	sys_wake_lock(FULL_WAKE_LOCK);

	prio = os_thread_priority_get(os_current_get());
	os_thread_priority_set(os_current_get(), 6);

#ifdef CONFIG_TASK_WDT
	// stop soft watchdog once
	task_wdt_stop();
#endif
	ret = ota_upgrade_check(g_ota);
	if (ret) {
		SYS_LOG_INF("ota_upgrade_check failed");
		ota_app_stop();
	} else {
		SYS_LOG_INF("ota_upgrade_check ok");
	}

	os_thread_priority_set(os_current_get(), prio);

	sys_wake_unlock(FULL_WAKE_LOCK);

	return ret;
}

int ota_app_reboot(uint8_t temp_file_id)
{
	/*Set temp file_id*/
	if (temp_file_id > 0) {
#ifdef CONFIG_PROPERTY
		property_set("OTA_FILE_ID", (char*)&temp_file_id, sizeof(uint8_t));
		property_flush(NULL);
#endif
	}

	/*Set upgrade state*/
	ota_upgrade_set_in_progress(g_ota);

	/*Set hardware flags to ensure enter ota upgrade durring bootloader*/
	soc_pstore_set(SOC_PSTORE_TAG_OTA_UPGRADE, 1);
	system_power_reboot(REBOOT_REASON_OTA_FINISHED);

	return 0;
}

