/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr.h>
#include <sys/types.h>
#include <sys/util.h>
#include <stdint.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/conn.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SETTINGS)
#define LOG_MODULE_NAME bt_property
#include "common/log.h"

#include <property_manager.h>

#include "common_internal.h"
#include "hci_core.h"
#include "property.h"

#if defined(CONFIG_BT_PROPERTY_USE_PRINTK)
void bt_property_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key)
{
	/* format: BT_XXX_ADDR_ID */
	if (key) {
		snprintk(path, path_size,
			 "%s_%02x%02x%02x%02x%02x%02x%u_%s", subsys,
			 addr->a.val[5], addr->a.val[4], addr->a.val[3],
			 addr->a.val[2], addr->a.val[1], addr->a.val[0],
			 addr->type, key);
	} else {
		snprintk(path, path_size,
			 "%s_%02x%02x%02x%02x%02x%02x%u", subsys,
			 addr->a.val[5], addr->a.val[4], addr->a.val[3],
			 addr->a.val[2], addr->a.val[1], addr->a.val[0],
			 addr->type);
	}

	BT_DBG("Encoded path %s", log_strdup(path));
}
#else
void bt_property_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key)
{
	size_t len = strlen(subsys);

	/* Skip if path_size is less than 3; strlen("bt/") */
	if (len < path_size) {
		/* Key format:
		 *  "bt/<subsys>/<addr><type>/<key>", "/<key>" is optional
		 */
		strcpy(path, subsys);
		path[len] = '_';
		len++;
#if 0
		strncpy(&path[len], subsys, path_size - len);
		len = strlen(path);
		if (len < path_size) {
			path[len] = '/';
			len++;
		}
#endif

		for (int8_t i = 5; i >= 0 && len < path_size; i--) {
			len += bin2hex(&addr->a.val[i], 1, &path[len],
				       path_size - len);
		}

		if (len < path_size) {
			/* Type can be either BT_ADDR_LE_PUBLIC or
			 * BT_ADDR_LE_RANDOM (value 0 or 1)
			 */
			path[len] = '0' + addr->type;
			len++;
		}

		if (key && len < path_size) {
			path[len] = '_';
			len++;
			strcpy(&path[len], key);
			len += strlen(&path[len]);
		}

		if (len >= path_size) {
			/* Truncate string */
			path[path_size - 1] = '\0';
		}

		path[len] = '\0';
	} else if (path_size > 0) {
		*path = '\0';
	}

	BT_DBG("Encoded path %s", path);
}
#endif

int bt_property_name_next(const char *name, const char **next)
{
	int rc = 0;

	if (next) {
		*next = NULL;
	}

	if (!name) {
		return 0;
	}

	/* name might come from flash directly, in flash the name would end
	 * with '=' or '\0' depending how storage is done. Flash reading is
	 * limited to what can be read
	 */
	while ((*name != '\0') && (*name != '=') && (*name != '_')) {
		rc++;
		name++;
	}

	if (*name == '_') {
		if (next) {
			*next = name + 1;
		}
		return rc;
	}

	return rc;
}

int bt_property_decode_key(const char *key, bt_addr_le_t *addr)
{
	if (bt_property_name_next(key, NULL) != 13) {
		return -EINVAL;
	}

	if (key[12] == '0') {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (key[12] == '1') {
		addr->type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < 6; i++) {
		hex2bin(&key[i * 2], 2, &addr->a.val[5 - i], 1);
	}

	BT_DBG("Decoded %s as %s", log_strdup(key), bt_addr_le_str(addr));

	return 0;
}

static int set(char *name, property_read_cb read_cb, char *cb_arg)
{
	int len;
	//const char *next;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	//len = settings_name_next(name, &next);
	len = strlen(name);

	if (!strncmp(name, "id", len)) {
		/* Any previously provided identities supersede flash */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID)) {
			BT_WARN("Ignoring identities stored in flash");
			return 0;
		}

		len = read_cb(cb_arg, (char *)&bt_dev.id_addr, sizeof(bt_dev.id_addr));
		if (len < 0) {
			BT_DBG("Property %s not exist", cb_arg);
			return 0;
		}
		/* should not compare int with unsigned int */
		if (len < sizeof(bt_dev.id_addr[0])) {
			if (len < 0) {
				BT_ERR("Failed to read ID address from storage"
				       " (err %zd)", len);
			} else {
				BT_ERR("Invalid length ID address in storage");
				BT_HEXDUMP_DBG(&bt_dev.id_addr, len,
					       "data read");
			}
			(void)memset(bt_dev.id_addr, 0,
				     sizeof(bt_dev.id_addr));
			bt_dev.id_count = 0U;
		} else {
			int i;

			bt_dev.id_count = len / sizeof(bt_dev.id_addr[0]);
			for (i = 0; i < bt_dev.id_count; i++) {
				BT_DBG("ID[%d] %s", i,
				       bt_addr_le_str(&bt_dev.id_addr[i]));
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) || defined(CONFIG_BT_PROPERTY)
	if (!strncmp(name, "name", len)) {
		len = read_cb(cb_arg, bt_dev.name, sizeof(bt_dev.name) - 1);
		if (len < 0) {
			BT_DBG("Property %s not exist", cb_arg);
		} else {
			bt_dev.name[len] = '\0';

			BT_DBG("Name set to %s", log_strdup(bt_dev.name));
		}
		return 0;
	}
#endif

#if defined(CONFIG_BT_PRIVACY)
	if (!strncmp(name, "irk", len)) {
		len = read_cb(cb_arg, (char *)bt_dev.irk, sizeof(bt_dev.irk));
		if (len < sizeof(bt_dev.irk[0])) {
			if (len < 0) {
				BT_DBG("Property %s not exist", cb_arg);
			} else {
				BT_ERR("Invalid length IRK in storage");
				(void)memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
			}
		} else {
			int i, count;

			count = len / sizeof(bt_dev.irk[0]);
			for (i = 0; i < count; i++) {
				BT_DBG("IRK[%d] %s", i,
				       bt_hex(bt_dev.irk[i], 16));
			}
		}

		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return -ENOENT;
}

#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

static void save_id(struct k_work *work)
{
	int err;
	BT_INFO("Saving ID");
	err = bt_property_set(CFG_BT_ID, (char *)&bt_dev.id_addr,
				ID_DATA_LEN(bt_dev.id_addr));
	if (err) {
		BT_ERR("Failed to save ID (err %d)", err);
	}

#if defined(CONFIG_BT_PRIVACY)
	err = bt_property_set(CFG_BT_IRK, (char *)bt_dev.irk, sizeof(bt_dev.irk));
	if (err) {
		BT_ERR("Failed to save IRK (err %d)", err);
	}
#endif
}

K_WORK_DEFINE(save_id_work, save_id);

void bt_property_save_id(void)
{
	k_work_submit(&save_id_work);
}

static int commit(void)
{
	BT_DBG("");

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC) || defined(CONFIG_BT_PROPERTY)
	if (bt_dev.name[0] == '\0') {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}
#endif
	if (!bt_dev.id_count) {
		bt_setup_public_id_addr();
	}

	if (!bt_dev.id_count) {
		int err;

		err = bt_setup_random_id_addr();
		if (err) {
			BT_ERR("Unable to setup an identity address");
			return err;
		}
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_finalize_init();
	}

	/* If any part of the Identity Information of the device has been
	 * generated this Identity needs to be saved persistently.
	 */
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_STORE_ID)) {
		BT_DBG("Storing Identity Information");
		bt_property_save_id();
	}

#ifdef CONFIG_BT_BLE_IPS
	int id_count = CONFIG_BT_ID_MAX;
	bt_id_get(NULL, &id_count);
	if (id_count < CONFIG_BT_ID_MAX) {
		int id;

		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			BT_ERR("Create id failed (%d)\n", id);
		} else {
			BT_INFO("New id: %d\n", id);
		}
	}
#endif
	return 0;
}

#if 0
SETTINGS_STATIC_HANDLER_DEFINE(bt, "bt", NULL, set, commit, NULL);

int bt_settings_init(void)
{
	int err;

	BT_DBG("");

	err = settings_subsys_init();
	if (err) {
		BT_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	return 0;
}
#endif

/* Actions add start */
void bt_property_load(void)
{
	set("id", bt_property_get, CFG_BT_ID);
	set("name", bt_property_get, CFG_BLE_NAME);
	set("irk", bt_property_get, CFG_BT_IRK);

	commit();
}

int bt_property_delete(const char *name)
{
	return bt_property_set(name, NULL, 0);
}

int bt_property_set_le_name(char *value, int value_len)
{
	return bt_property_set(CFG_BLE_NAME, value, value_len);
}

int bt_property_get_le_name(char *value, int value_len)
{
	return bt_property_get(CFG_BLE_NAME, value, value_len);
}

int bt_property_get_br_name(char *value, int value_len)
{
	return bt_property_get(CFG_BT_NAME, value, value_len);
}
/* Actiosn add end */