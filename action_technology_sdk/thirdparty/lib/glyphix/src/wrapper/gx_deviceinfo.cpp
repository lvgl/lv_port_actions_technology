/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#include "gx_deviceinfo.h"
#include "soc_regs.h"
#include <fs/fs.h>
#include <bt_manager.h>

namespace gx {
JsValue device_info_get_brand() { return "Actions"; }

JsValue device_info_get_manufacturer() { return "manufacturer"; }

JsValue device_info_get_model() { return "model"; }

JsValue device_info_get_product() { return "product"; }

JsValue device_info_get_device_id() { 
	static char uid_str[17];
	uint32_t uid0 = sys_read32(UID0);
	uint32_t uid1 = sys_read32(UID1);
	snprintf(uid_str, sizeof(uid_str), "%08x%08x", uid0, uid1);
	return uid_str;
}

// TODO
JsValue device_info_get_serial_number() { return "sn123"; }

JsValue device_info_get_mac() {	
	static char bt_mac[20];
	bt_addr_le_t addr;
	uint8_t *mac = addr.a.val;
	int ret;

	memset(bt_mac, 0, sizeof(bt_mac));
	ret = bt_manager_get_ble_mac(&addr);
	if (!ret) {	
		sprintf(bt_mac, "%02x%02x%02x%02x%02x%02x", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
	}
	return bt_mac;
}

JsValue device_info_get_user_id() { return "234"; };

JsValue device_info_get_advertis_id() { return "345"; };

JsValue device_info_get_os_type() { return "rtos"; }

JsValue device_info_get_os_version_name() { return "rtos"; }

uint64_t device_info_get_totalstorage() { 
	struct fs_statvfs stat;
	uint64_t ret;

	ret = fs_statvfs(CONFIG_GX_STORAGE_ROOTFS_PATH, &stat);
	if (ret) {
		ret = 0;
	} else {
		ret = stat.f_frsize * stat.f_blocks;
	}
	return ret;
}

uint64_t device_info_get_availablestorage() {
	struct fs_statvfs stat;
	uint64_t ret;

	ret = fs_statvfs(CONFIG_GX_STORAGE_ROOTFS_PATH, &stat);
	if (ret) {
		ret = 0;
	} else {
		ret = stat.f_frsize * stat.f_bfree;
	}
	return ret;
}

} // namespace gx
