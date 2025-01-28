/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <string.h>
#include <fw_version.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_temp_part.h>
#include <ota_backend_sdcard.h>
#include <soc_pm.h>
#include <soc.h>
#include <sys_event.h>

#ifdef CONFIG_SDFS_MANAGER
#include <sdfs_manager.h>
#endif

#include <drivers/nvram_config.h>

#include "sys_manager.h"
#include <drivers/flash.h>

#ifdef CONFIG_USB_HOTPLUG
extern int usb_hotplug_host_check(void);
#endif

static struct ota_upgrade_info *g_ota;

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
#define OTA_STORAGE_EXT_DEVICE_NAME CONFIG_OTA_TEMP_PART_DEV_NAME
#endif

#ifdef CONFIG_OTA_BACKEND_TEMP_PART
#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)
#define OTA_STORAGE_DEVICE_NAME		CONFIG_OTA_TEMP_PART_DEV_NAME
#else
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#endif
static struct ota_backend *backend_spinor;
#endif

#ifdef CONFIG_OTA_BACKEND_SDCARD
#ifdef CONFIG_BOARD_NANDBOOT
#define OTA_STORAGE_DEVICE_NAME		CONFIG_OTA_TEMP_PART_DEV_NAME
#else
#define OTA_STORAGE_DEVICE_NAME		CONFIG_XSPI_NOR_ACTS_DEV_NAME
#endif
static struct ota_backend *backend_sdcard;
#endif

static void ota_app_start(void)
{
	SYS_LOG_INF("ota app start");
	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);
}

static void ota_app_stop(void)
{
	SYS_LOG_INF("ota app stop");
	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, true);
}

static void ota_app_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);
}
static void ota_app_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p state %d", backend, state);
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
			ota_app_stop();

		}
	}
}

#ifdef CONFIG_OTA_BACKEND_TEMP_PART
int ota_app_init_spinor(void)
{
	struct ota_backend_temp_part_init_param temp_part_init_param;

	memset(&temp_part_init_param, 0x0, sizeof(struct ota_backend_temp_part_init_param));
	temp_part_init_param.dev_name = CONFIG_OTA_TEMP_PART_DEV_NAME;

	backend_spinor = ota_backend_temp_part_init(ota_app_backend_callback, &temp_part_init_param);
	if (!backend_spinor) {
		SYS_LOG_ERR("failed to init temp part for ota");
		return -ENODEV;
	}

	return 0;
}
#endif

#ifdef CONFIG_OTA_BACKEND_SDCARD
struct ota_backend_sdcard_init_param sdcard_init_param = {
	.fpath = "/SD:/ota.bin",
};

struct ota_backend_sdcard_init_param usb_init_param = {
	.fpath = "/USB:/ota.bin",
};


int ota_app_init_sdcard(void)
{
	backend_sdcard = ota_backend_sdcard_init(ota_app_backend_callback, &sdcard_init_param);
	if (!backend_sdcard) {
		backend_sdcard = ota_backend_sdcard_init(ota_app_backend_callback, &usb_init_param);
		if (!backend_sdcard) {
			SYS_LOG_INF("failed to init sdcard ota");
			return -ENODEV;
		}
	}

	SYS_LOG_INF("%s %d ok\n",__FUNCTION__,__LINE__);
	return 0;
}
#endif

int ota_app_init(void)
{
	struct ota_upgrade_param param;

	SYS_LOG_INF("ota app init");

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = OTA_STORAGE_DEVICE_NAME;
#ifdef CONFIG_OTA_MUTIPLE_STORAGE
	param.storage_ext_name = OTA_STORAGE_EXT_DEVICE_NAME;
#endif
	param.notify = ota_app_notify;
	param.flag_use_recovery = 1;
	param.flag_use_recovery_app = 1;
	param.no_version_control = 1;

#ifndef CONFIG_BOARD_NANDBOOT
	const struct device *flash_device = device_get_binding(CONFIG_XSPI_NOR_ACTS_DEV_NAME);
	if (flash_device)
		flash_write_protection_set(flash_device, false);
#endif

	g_ota = ota_upgrade_init(&param);
	if (!g_ota) {
		SYS_LOG_INF("ota app init error");
#ifndef CONFIG_BOARD_NANDBOOT
		if (flash_device)
			flash_write_protection_set(flash_device, true);
#endif
		return -1;
	}

#ifndef CONFIG_BOARD_NANDBOOT
	if (flash_device)
		flash_write_protection_set(flash_device, true);
#endif

	return 0;
}
static bool ota_upgrade_is_allowed(void)
{
	char ota_flag[4] = { 0 };

	nvram_config_get("REC_OTA_FLAG", ota_flag, 4);
	if (!memcmp(ota_flag, "yes", strlen("yes"))) {
		return true;
	}
	return false;
}

extern void trace_init(void);
int main(void)
{
	uint8_t reason = REBOOT_REASON_OTA_FINISHED;
	
#ifdef CONFIG_WATCHDOG
	soc_watchdog_stop();
#endif

#ifdef CONFIG_USB_HOTPLUG
	usb_hotplug_host_check();
#endif
#ifdef CONFIG_ACTIONS_TRACE
	trace_init();
#endif

	SYS_LOG_INF("ota recovery main");

	ota_app_init();

	int need_run = ota_upgrade_is_in_progress(g_ota);
	if (!need_run) {
		SYS_LOG_INF("skip ota recovery");
		reason = 0;
		goto exit;
	}

#ifdef CONFIG_OTA_BACKEND_TEMP_PART
	ota_app_init_spinor();
#endif

#ifdef CONFIG_OTA_BACKEND_SDCARD
#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
	sdfs_manager_init();
#endif

	if (ota_app_init_sdcard()) {
		SYS_LOG_INF("init sdcard failed");
		reason = 0;
		goto exit;
	}
#endif

	if (!ota_upgrade_is_allowed()) {
		SYS_LOG_INF("upgrade not allowed\n");
		reason = 0;
		goto exit;
	}
	SYS_LOG_INF("do ota recovery");

	if (ota_upgrade_check(g_ota)) {
		SYS_LOG_INF("ota upgrade failed, wait check ota file \n");
		os_sleep(1000);
	} else {
	#ifdef CONFIG_OTA_BACKEND_SDCARD
	#ifdef CONFIG_MUTIPLE_VOLUME_MANAGER
		fs_unlink(sdcard_init_param.fpath);
	#endif
	#endif
		nvram_config_set("REC_OTA_FLAG", "no", 4);
	}

exit:
	SYS_LOG_INF("REBOOT_TYPE_GOTO_SYSTEM");
	sys_pm_reboot(REBOOT_TYPE_GOTO_SYSTEM | reason);
}
