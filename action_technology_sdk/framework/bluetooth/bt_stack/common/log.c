/* log.c - logging helpers */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr.h>
#include <sys/util.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/hci.h>

#define LOG_DIRECT_OUTPUT	1

const char *bt_hex_real(const void *buf, size_t len)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_hex";
	const uint8_t *data = buf;
	size_t i;

	printk("%s: ", retStr);
	for (i=0; i<len; i++) {
		printk("%02x", data[i]);
	}
	printk("\n");

	return retStr;
#else
	static const char hex[] = "0123456789abcdef";
	static char str[129];
	const uint8_t *b = buf;
	size_t i;

	len = MIN(len, (sizeof(str) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
#endif
}

const char *bt_addr_str_real(const bt_addr_t *addr)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_addr";
	int i;

	printk("%s: ", retStr);
	for (i=0; i<6; i++) {
		printk("%02x%s", addr->val[5-i], (5 - i)? ":" : "");
	}
	printk("\n");

	return retStr;
#else
	static char str[BT_ADDR_STR_LEN];

	bt_addr_to_str(addr, str, sizeof(str));

	return str;
#endif
}

const char *bt_addr_le_str_real(const bt_addr_le_t *addr)
{
#if LOG_DIRECT_OUTPUT
	static const char *retStr = "bt_addr_le";
	int i;

	printk("%s: ", retStr);

	switch (addr->type) {
	case BT_ADDR_LE_PUBLIC:
		printk("public ");
		break;
	case BT_ADDR_LE_RANDOM:
		printk("random ");
		break;
	default:
		printk("type(0x%02x) ", addr->type);
		break;
	}

	for (i=0; i<6; i++) {
		printk("%02x%s", addr->a.val[5-i], (5 - i)? ":" : "");
	}
	printk("\n");

	return retStr;
#else
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
#endif
}

const char *bt_uuid_str_real(const struct bt_uuid *uuid)
{
	static char str[BT_UUID_STR_LEN];

	bt_uuid_to_str(uuid, str, sizeof(str));

	return str;
}
