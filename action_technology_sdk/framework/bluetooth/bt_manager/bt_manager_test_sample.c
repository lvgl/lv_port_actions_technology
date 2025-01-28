/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include <btservice_api.h>
#include <shell/shell.h>
#include <acts_bluetooth/host_interface.h>
#include <property_manager.h>

#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>

#define test_wake_lock()		sys_wake_lock(PARTIAL_WAKE_LOCK)
#define test_wake_unlock() 		sys_wake_unlock(PARTIAL_WAKE_LOCK)
#else
#define test_wake_lock()
#define test_wake_unlock()
#endif

#ifdef CONFIG_BT_SPP
//#define MGR_SPP_TEST_SHELL		1
#endif

#ifdef CONFIG_BT_PBAP_CLIENT
//#define MGR_PBAP_TEST_SHELL		1
#endif

#ifdef CONFIG_BT_MAP_CLIENT
//#define MGR_MAP_TEST_SHELL		1
#endif

#ifdef CONFIG_BT_BLE
// #define MGR_BLE_TEST_SHELL		1
#endif

#ifdef CONFIG_BT_PROPERTY
//#define MGR_BT_LINKKEY_TEST_SHELL    1
#endif

#if (defined CONFIG_BT_BLE) || (defined CONFIG_BT_SPP)
//#define MGR_SPPBLE_STREAM_TEST_SHELL	1
#endif

#ifdef CONFIG_BT_HIDS
//#define MGR_BLE_HID_TEST_SHELL	1
#endif

#ifdef CONFIG_BT_HID
// #define MGR_BR_HID_TEST_SHELL	1
#endif

#if CONFIG_BT_BR_ACTS
static int mgr_char2hex(const char *c, uint8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int mgr_str2bt_addr(const char *str, bd_address_t *addr)
{
	int i, j;
	uint8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->val[i] = addr->val[i] << 4;

		if (mgr_char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->val[i] |= tmp;
	}

	return 0;
}

static void btmgr_discover_result(void *result)
{
	uint8_t i;
	struct btsrv_discover_result *cb_result = result;

	if (cb_result->discover_finish) {
		SYS_LOG_INF("Discover finish\n");
	} else {
		SYS_LOG_INF("Mac %02x:%02x:%02x:%02x:%02x:%02x, rssi %i\n",
			cb_result->addr.val[5], cb_result->addr.val[4], cb_result->addr.val[3],
			cb_result->addr.val[2], cb_result->addr.val[1], cb_result->addr.val[0],
			cb_result->rssi);
		SYS_LOG_INF("COD 0x%02x%02x%02x", cb_result->cod[2], cb_result->cod[1], cb_result->cod[0]);
		if (cb_result->len) {
			SYS_LOG_INF("Name: ");
			for (i = 0; i < cb_result->len; i++) {
				printk("%c", cb_result->name[i]);
			}
			printk("\n");
			SYS_LOG_INF("Device id: 0x%x, 0x%x, 0x%x, 0x%x\n", cb_result->device_id[0],
					cb_result->device_id[1], cb_result->device_id[2], cb_result->device_id[3]);
		}
	}
}

static int shell_cmd_btmgr_br_discover(const struct shell *shell, size_t argc, char *argv[])
{
	struct btsrv_discover_param param;

	if (argc < 2) {
		SYS_LOG_INF("Used: btmgr discover start/stop\n");
		return -EINVAL;
	}

	if (!strcmp(argv[1], "start")) {
		param.cb = &btmgr_discover_result;
		param.length = 4;
		param.num_responses = 0;
		if (bt_manager_br_start_discover(&param)) {
			SYS_LOG_ERR("Failed to start discovery\n");
		}
	} else if (!strcmp(argv[1], "stop")) {
		if (bt_manager_br_stop_discover()) {
			SYS_LOG_ERR("Failed to stop discovery\n");
		}
	} else {
		SYS_LOG_INF("Used: btmgr discover start/stop\n");
	}

	return 0;
}

static int shell_cmd_btmgr_br_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btmgr br_connect F4:4E:FD:xx:xx:xx\n");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	return bt_manager_br_connect(&addr);
}

static int shell_cmd_btmgr_br_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btmgr br_connect F4:4E:FD:xx:xx:xx\n");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	return bt_manager_br_disconnect(&addr);
}
#endif

#if MGR_SPP_TEST_SHELL
/* UUID: "00007777-0000-1000-8000-00805F9B34FB" */
static const uint8_t test_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,	\
										0x00, 0x10, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00};
static uint8_t test_spp_channel;
static uint8_t test_spp_start_tx;

#ifdef CONFIG_BT_3M
#define SPP_TEST_SEND_SIZE			990
#else
#define SPP_TEST_SEND_SIZE			650
#endif
#define SPP_TEST_WORK_INTERVAL		1		/* 1ms */

static uint8_t spp_test_buf[SPP_TEST_SEND_SIZE];
static const char spp_tx_trigger[] = "1122334455";
static const char spp_tx_sample[] = "Test spp send data";
static os_delayed_work spp_test_work;

static void test_spp_connect_failed_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d\n", channel);
	if (test_spp_channel == channel) {
		test_spp_channel = 0;
	}
}

static void test_spp_connected_cb(uint8_t channel, uint8_t *uuid)
{
	SYS_LOG_INF("channel:%d\n", channel);
	test_spp_channel = channel;
}

static void test_spp_disconnected_cb(uint8_t channel)
{
	SYS_LOG_INF("channel:%d\n", channel);
	if (test_spp_channel == channel) {
		test_spp_channel = 0;
	}
}

static void spp_test_delaywork(os_work *work)
{
	static uint32_t curr_time;
	static uint32_t pre_time;
	static uint32_t count;
	int ret, i;

	if (test_spp_start_tx && test_spp_channel) {
		for (i=0; i<10; i++) {
			ret = bt_manager_spp_send_data(test_spp_channel, spp_test_buf, SPP_TEST_SEND_SIZE);
			if (ret > 0) {
				count += SPP_TEST_SEND_SIZE;
				curr_time = k_uptime_get_32();
				if ((curr_time - pre_time) >= 1000) {
					printk("Tx: %d byte\n", count);
					count = 0;
					pre_time = curr_time;
				}
				os_yield();
			} else {
				break;
			}
		}

		os_delayed_work_submit(&spp_test_work, SPP_TEST_WORK_INTERVAL);
	}
}

static void test_spp_start_stop_send_delaywork(void)
{
	uint8_t value = 0;
	int i;

	if (test_spp_start_tx == 0) {
		for (i = 0; i < SPP_TEST_SEND_SIZE; i++) {
			spp_test_buf[i] = value++;
		}

		bt_manager_spp_send_data(test_spp_channel, spp_test_buf, SPP_TEST_SEND_SIZE);
		os_sleep(1000);		/* Wait for exit sniff mode */

		test_spp_start_tx = 1;
		os_delayed_work_submit(&spp_test_work, SPP_TEST_WORK_INTERVAL);
		SYS_LOG_INF("SPP tx start\n");
	} else {
		test_spp_start_tx = 0;
		os_delayed_work_cancel(&spp_test_work);
		SYS_LOG_INF("SPP tx stop\n");
	}
}

static void test_spp_receive_data_cb(uint8_t channel, uint8_t *data, uint32_t len)
{
	static uint32_t curr_time;
	static uint32_t pre_time;
	static uint32_t count;

	/* SYS_LOG_INF("channel:%d, rx: %d\n", channel, len); */
	if (len == strlen(spp_tx_trigger)) {
		if (memcmp(data, spp_tx_trigger, len) == 0) {
			test_spp_start_stop_send_delaywork();
		}
	}

	count += len;
	curr_time = k_uptime_get_32();
	if ((curr_time - pre_time) >= 1000) {
		printk("Rx: %d byte\n", count);
		count = 0;
		pre_time = curr_time;
	}
}

static const struct btmgr_spp_cb test_spp_cb = {
	.connect_failed = test_spp_connect_failed_cb,
	.connected = test_spp_connected_cb,
	.disconnected = test_spp_disconnected_cb,
	.receive_data = test_spp_receive_data_cb,
};

static int shell_cmd_btspp_reg(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t reg_flag = 0;

	if (reg_flag) {
		SYS_LOG_INF("Already register\n");
		return 0;
	}

	if (!bt_manager_spp_reg_uuid((uint8_t *)test_spp_uuid, (struct btmgr_spp_cb *)&test_spp_cb)) {
		os_delayed_work_init(&spp_test_work, spp_test_delaywork);
		reg_flag = 1;
	}
	return 0;
}

static int shell_cmd_btspp_spp_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;

	if (test_spp_channel) {
		SYS_LOG_INF("Already connect channel %d\n", test_spp_channel);
		return 0;
	}

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btspp spp_connect F4:4E:FD:xx:xx:xx\n");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)\n", err);
		return err;
	}

	test_spp_channel = bt_manager_spp_connect(&addr, (uint8_t *)test_spp_uuid, (struct btmgr_spp_cb *)&test_spp_cb);
	if (test_spp_channel == 0) {
		SYS_LOG_INF("Failed to do spp connect\n");
	} else {
		SYS_LOG_INF("SPP connect channel %d\n", test_spp_channel);
	}

	return 0;
}

static int shell_cmd_btspp_spp_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	if (test_spp_channel == 0) {
		SYS_LOG_INF("SPP not connected\n");
	} else {
		bt_manager_spp_disconnect(test_spp_channel);
	}

	return 0;
}

static int shell_cmd_btspp_send(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t *data;
	uint16_t len;

	if (test_spp_channel == 0) {
		SYS_LOG_INF("SPP not connected\n");
	} else {
		if (argc >= 2) {
			data = argv[1];
			len = strlen(argv[1]);

			if (!strcmp(argv[1], "continue")) {
				test_spp_start_stop_send_delaywork();
				return 0;
			}
		} else {
			data = (uint8_t *)spp_tx_sample;
			len = strlen(spp_tx_sample);
		}
		bt_manager_spp_send_data(test_spp_channel, data, len);
		SYS_LOG_INF("Send data len %d\n", len);
	}

	return 0;
}
#endif

#if MGR_PBAP_TEST_SHELL
/* PBAP shell operate
 * btmgr pbap_connect F4:4E:FD:xx:xx:xx
 * btmgr pbap_op size  -- get pb size
 * btmgr pbap_op pb   -- get pb default order[indexed]
 *
 * -- get pb by order start
 * btmgr pbap_op setpath  -- set pb path
 * btmgr pbap_op list order  -- list pb by order
 * btmgr pbap_op vcard [xx.vcf]  -- get vcard one by one 0.vcf/1.vcf/...
 * -- get pb by order end
 *
 * -- search name by number start
 * btmgr pbap_op setpath  -- set pb path
 * btmgr pbap_op list order search_number
 * -- search name by number end
 *
 * btmgr pbap_op abort
 * btmgr pbap_op stopget
 * btmgr pbap_disconnect
 */

/* PBAP path
 * telecom/pb.vcf
 * telecom/ich.vcf
 * telecom/och.vcf
 * telecom/mch.vcf
 * telecom/cch.vcf
 * telecom/spd.vcf
 * telecom/fav.vcf
 * SIM1/telecom/pb.vcf
 * SIM1/telecom/ich.vcf
 * SIM1/telecom/och.vcf
 * SIM1/telecom/mch.vcf
 * SIM1/telecom/cch.vcf
 */
#define TEST_PBAP_PB_NAME "telecom/pb.vcf"
#define TEST_PBAP_PB_PATH "telecom/pb"
#if 1
#define TEST_PABP_FILTER	(BT_PBAP_FILTER_VERSION | BT_PBAP_FILTER_FN | BT_PBAP_FILTER_TEL |	\
							BT_PBAP_FILTER_TZ | BT_PBAP_FILTER_X_IRMC_CALL_DATETIME)
#endif
#if 0
#define TEST_PABP_FILTER	(BT_PBAP_FILTER_VERSION |	\
							BT_PBAP_FILTER_FN |	\
							BT_PBAP_FILTER_N |	\
							BT_PBAP_FILTER_ADR |	\
							BT_PBAP_FILTER_TEL |	\
							BT_PBAP_FILTER_EMAIL|	\
							BT_PBAP_FILTER_TITLE |	\
							BT_PBAP_FILTER_ROLE |	\
							BT_PBAP_FILTER_ORG |	\
							BT_PBAP_FILTER_NOTE)
#endif
#if 0
#define TEST_PABP_FILTER	(	BT_PBAP_FILTER_VERSION |	\
	BT_PBAP_FILTER_FN |	\
	BT_PBAP_FILTER_N |	\
	BT_PBAP_FILTER_PHOTO |	\
	BT_PBAP_FILTER_BDAY |	\
	BT_PBAP_FILTER_ADR |	\
	BT_PBAP_FILTER_LABEL |	\
	BT_PBAP_FILTER_TEL |	\
	BT_PBAP_FILTER_EMAIL |	\
	BT_PBAP_FILTER_MAILER |	\
	BT_PBAP_FILTER_TZ |	\
	BT_PBAP_FILTER_GEO |	\
	BT_PBAP_FILTER_TITLE |	\
	BT_PBAP_FILTER_ROLE |	\
	BT_PBAP_FILTER_LOGO |	\
	BT_PBAP_FILTER_AGENT |	\
	BT_PBAP_FILTER_ORG |	\
	BT_PBAP_FILTER_NOTE |	\
	BT_PBAP_FILTER_REV |	\
	BT_PBAP_FILTER_SOUND |	\
	BT_PBAP_FILTER_URL |	\
	BT_PBAP_FILTER_UID |	\
	BT_PBAP_FILTER_KEY |	\
	BT_PBAP_FILTER_NICKNAME |	\
	BT_PBAP_FILTER_CATEGORIES |	\
	BT_PBAP_FILTER_PROID |	\
	BT_PBAP_FILTER_CLASS |	\
	BT_PBAP_FILTER_SORT_STRING |	\
	BT_PBAP_FILTER_X_IRMC_CALL_DATETIME |	\
	BT_PBAP_FILTER_X_BT_SPEEDDIALKEY |	\
	BT_PBAP_FILTER_X_BT_UCI |	\
	BT_PBAP_FILTER_TYPE_X_BT_UID)
#endif

static uint8_t test_app_id;
static uint8_t test_pbap_connected;
static uint8_t test_pbap_continue_get;

static void test_pbap_connect_failed_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap cfcb app id %d", app_id);
	test_pbap_connected = 0;
}

static void test_pbap_connected_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap connected app id %d", app_id);
	test_pbap_connected = 1;
}

static void test_pbap_disconnected_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap disconected app id %d", app_id);
	test_app_id = 0;
	test_pbap_connected = 0;
}

static void test_pbap_max_size_cb(uint8_t app_id, uint16_t max_size)
{
	SYS_LOG_INF("pbap app id %d max size %d", app_id, max_size);
}

/*
 * Result like this, app need parse name and phone number.
 * same vcard result will withou name and phone number(like vcard info)
 * "Type 0, len 11, value VERSION:2.1"
 * "Type 1, len xx, value FN;CHARSET=UTF-8:test663552"
 * "Type 1, len xx, value FN;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:=E5=BD=AD=E9=BE=99"
 * "Type 2, len xx, valueN;CHARSET=UTF-8;ENCODING=QUOTED-PRINTABLE:;=E5=BD=AD=E9=BE=99;;;"
 * "Type 7, len 9, value TEL:10086"
 * "Type 28, len 43, value X-IRMC-CALL-DATETIME;DIALED:20220609T151403"
 * TYPE: enum BT_PBAP_CARC_CONTEXT_TYPE
*/
static void test_pbap_result_cb(uint8_t app_id, struct mgr_pbap_result *result, uint8_t size)
{
	int i;

	SYS_LOG_INF("pbap result app id %d", app_id);

	for (i = 0; i < size; i++) {
		SYS_LOG_INF("Type %d, len %d, value %s", result[i].type, result[i].len, result[i].data);
	}
}

static void test_pbap_setpath_finish_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap set patch finish app id %d", app_id);
}

static void test_pbap_search_result_cb(uint8_t app_id, struct mgr_pbap_result *result, uint8_t size)
{
	int i;

	SYS_LOG_INF("pbap search result app id %d", app_id);

	for (i = 0; i < size; i++) {
		SYS_LOG_INF("Type %d, len %d, value %s", result[i].type, result[i].len, result[i].data);
	}
}

static void test_pbap_get_vcard_finish_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap get vc finish app id %d", app_id);
	if (test_pbap_continue_get) {
		/* Get next vcard */
		btmgr_pbap_get_vcard_continue(app_id);
	}
}

static void test_pbap_end_of_body_cb(uint8_t app_id)
{
	test_pbap_continue_get = 0;
	SYS_LOG_INF("pbap EOB app id %d", app_id);
}

static void test_pbap_abort_cb(uint8_t app_id)
{
	SYS_LOG_INF("pbap abort app id %d", app_id);
}

static const struct btmgr_pbap_cb test_pbap_cb = {
	.connect_failed = test_pbap_connect_failed_cb,
	.connected = test_pbap_connected_cb,
	.disconnected = test_pbap_disconnected_cb,
	.max_size = test_pbap_max_size_cb,
	.result = test_pbap_result_cb,
	.setpath_finish = test_pbap_setpath_finish_cb,
	.search_result = test_pbap_search_result_cb,
	.get_vcard_finish = test_pbap_get_vcard_finish_cb,
	.end_of_body = test_pbap_end_of_body_cb,
	.abort = test_pbap_abort_cb,
};

static int shell_cmd_pbap_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btmgr pbap_connect F4:4E:FD:xx:xx:xx");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)", err);
		return err;
	}

	test_app_id = btmgr_pbap_connect(&addr, (struct btmgr_pbap_cb *)&test_pbap_cb);
	if (!test_app_id) {
		SYS_LOG_INF("Failed to connect phonebook");
	} else {
		SYS_LOG_INF("test_app_id %d", test_app_id);
	}

	return 0;
}

static int shell_cmd_pbap_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	if ((test_pbap_connected == 0) || (test_app_id == 0)) {
		SYS_LOG_INF("Not connected");
		return -EINVAL;
	}

	btmgr_pbap_disconnect(test_app_id);
	return 0;
}

static int shell_cmd_pbap_op(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t order = 0;
	uint8_t len = 0;
	char *char_value = NULL;
	char *path;

	if ((test_pbap_connected == 0) || (test_app_id == 0)) {
		SYS_LOG_INF("Not connected");
		return -EIO;
	}

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btmgr pbap_op size/pb/setpath/list/vcard/abort/stopget");
		return -EINVAL;
	}

	if (!strcmp(argv[1], "size")) {
		path = (argc >= 3) ? argv[2] : TEST_PBAP_PB_NAME;
		SYS_LOG_INF("PB_NAME %s", path);
		btmgr_pbap_get_size(test_app_id, path);
	} else if (!strcmp(argv[1], "pb")) {
		test_pbap_continue_get = 1;
		path = (argc >= 3) ? argv[2] : TEST_PBAP_PB_NAME;
		SYS_LOG_INF("PB_NAME %s", path);
		btmgr_pbap_get_pb(test_app_id, path, TEST_PABP_FILTER);
	}  else if (!strcmp(argv[1], "setpath")) {
		path = (argc >= 3) ? argv[2] : TEST_PBAP_PB_PATH;
		SYS_LOG_INF("PB_PATH %s", path);
		btmgr_pbap_setpath(test_app_id, path);
	} else if (!strcmp(argv[1], "list")) {
		if (argc >= 3) {
			order = argv[2][0] - 0x30;
			order = (order > 2) ? 0 : order;
		}
		if (argc >= 4) {
			char_value = argv[3];
			len = strlen(char_value);
		}
		test_pbap_continue_get = 1;
		btmgr_pbap_listing(test_app_id, order, 1, char_value, len);
	} else if (!strcmp(argv[1], "vcard")) {
		if (argc >= 3) {
			char_value = argv[2];
		} else {
			char_value = "0.vcf";
		}
		test_pbap_continue_get = 1;
		btmgr_pbap_get_vcard(test_app_id, char_value, strlen(char_value), TEST_PABP_FILTER);
	} else if (!strcmp(argv[1], "abort")) {
		test_pbap_continue_get = 0;
		btmgr_pbap_abort(test_app_id);
	} else if (!strcmp(argv[1], "stopget")) {
		test_pbap_continue_get = 0;
	} else {
		SYS_LOG_INF("Unsupport cmd %s", argv[1]);
		return -EINVAL;
	}

	return 0;
}
#endif

#if MGR_MAP_TEST_SHELL
#define TEST_MAP_PATH             "telecom/msg/inbox"
#define TEST_MAP_SET_FOLDER       "telecom/msg/inbox"
#define TEST_MAP_APP_PARAMETER_DATETIME  1

static uint8_t map_test_app_id;

#if 0
static void btsrv_map_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	if (map_test_app_id) {
		btmgr_map_get_messages_listing(map_test_app_id,1,0x0A);
	}
}
#endif

static void test_map_connect_failed_cb(uint8_t app_id)
{
	SYS_LOG_INF("app id %d\n", app_id);
}

static void test_map_connected_cb(uint8_t app_id)
{
	SYS_LOG_INF("app id %d\n", app_id);
	map_test_app_id = app_id;
}

static void test_map_disconnected_cb(uint8_t app_id)
{
	SYS_LOG_INF("app id %d\n", app_id);
	map_test_app_id = 0;
}

static void test_map_result_cb(uint8_t app_id, struct mgr_map_result *result, uint8_t size)
{
	int i;
    uint8_t datetime_buf[24];

	SYS_LOG_INF("app id %d size:%d\n", app_id,size);

    if(size > 0){
        for (i = 0; i < size; i++) {
            if((result[i].type == TEST_MAP_APP_PARAMETER_DATETIME) && (result[i].len < 24)){
                memcpy(datetime_buf,result[i].data,result[i].len);
                datetime_buf[result[i].len] = 0;
                SYS_LOG_INF("Type %d, len %d, value %s\n", result[i].type, result[i].len, datetime_buf);
            }
        }
    }
}

static const struct btmgr_map_cb test_map_cb = {
	.connect_failed = test_map_connect_failed_cb,
	.connected = test_map_connected_cb,
	.disconnected = test_map_disconnected_cb,
	.result = test_map_result_cb,
};

static int cmd_test_map_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;
    uint8_t app_id;

	if (argc < 2) {
		SYS_LOG_INF("CMD link: btpbap get F4:4E:FD:xx:xx:xx");
		return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		SYS_LOG_INF("Invalid peer address (err %d)", err);
		return err;
	}

	app_id = btmgr_map_client_connect(&addr, TEST_MAP_PATH, (struct btmgr_map_cb *)&test_map_cb);
	if (!app_id) {
		SYS_LOG_INF("Failed to connect\n");
	} else {
		SYS_LOG_INF("map_test_app_id %d\n", app_id);
	}

	return 0;
}

static int cmd_test_map_set_folder(const struct shell *shell, size_t argc, char *argv[])
{
	if (map_test_app_id) {
	    btmgr_map_client_set_folder(map_test_app_id,TEST_MAP_SET_FOLDER,2);
    }
    return 0;
}

static int cmd_test_map_get_msg_list(const struct shell *shell, size_t argc, char *argv[])
{
	if (map_test_app_id) {
	    btmgr_map_get_messages_listing(map_test_app_id,1,0x0A);
    }
    return 0;
}

static int cmd_test_map_get_folder_list(const struct shell *shell, size_t argc, char *argv[])
{
	if (map_test_app_id) {
		btmgr_map_get_folder_listing(map_test_app_id);
    }
    return 0;
}

static int cmd_test_map_abort(const struct shell *shell, size_t argc, char *argv[])
{
    btmgr_map_abort_get(map_test_app_id);
	return 0;
}

static int cmd_test_map_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
    btmgr_map_client_disconnect(map_test_app_id);
	return 0;
}
#endif

#if MGR_BLE_TEST_SHELL
#if (!defined(CONFIG_GATT_OVER_BREDR) || !defined(CONFIG_BT_PTS_TEST))
#define SPEED_BLE_SERVICE_UUID		BT_UUID_DECLARE_16(0xFFC0)
#define SPEED_BLE_WRITE_UUID		BT_UUID_DECLARE_16(0xFFC1)
#define SPEED_BLE_READ_UUID			BT_UUID_DECLARE_16(0xFFC2)

static int ble_speed_rx_data(uint8_t *buf, uint16_t len);

#define BLE_TEST_SEND_SIZE			244
#define BLE_TEST_WORK_INTERVAL		1		/* 1ms */

static const char ble_speed_tx_trigger[] = "1122334455";
static uint8_t ble_connected_flag = 0;
static uint8_t ble_speed_tx_flag = 0;
static uint8_t ble_speed_notify_enable = 0;
static os_delayed_work ble_speed_test_work;
uint8_t ble_speed_send_buf[BLE_TEST_SEND_SIZE];

#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
#define BLE_SPEED_CHECK_PARAM_INTERVAL	2000		/* 2s */

enum {
	BLE_PHONE_TYPE_ANDROID,
	BLE_PHONE_TYPE_IOS,
    BLE_PHONE_TYPE_MAX,
};

enum {
	BLE_SPEED_LEVEL_FAST,
    BLE_SPEED_LEVEL_BUSY,
	BLE_SPEED_LEVEL_IDLE,
    BLE_SPEED_LEVEL_MAX,
};

const struct bt_le_conn_param le_param[BLE_PHONE_TYPE_MAX][BLE_SPEED_LEVEL_MAX] = {
    {
        {6,15,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
    {
        {12,28,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
};

static os_delayed_work ble_uparam_work;
static uint8_t fast_speed_mode;

static void ble_speed_param_set_wakelock(bool set)
{
	static uint8_t param_wake_lock = 0;

	if (set) {
		if (!param_wake_lock) {
			param_wake_lock = 1;
			test_wake_lock();
			SYS_LOG_INF("Le param lock");
		}
	} else {
		if (param_wake_lock) {
			param_wake_lock = 0;
			test_wake_unlock();
			SYS_LOG_INF("Le param unlock");
		}
	}
}

static void ble_speed_fast_mode_set_wakelock(bool set)
{
	static uint8_t fast_mode_wake_lock = 0;

	if (set) {
		if (!fast_mode_wake_lock) {
			fast_mode_wake_lock = 1;
			test_wake_lock();
			SYS_LOG_INF("Le fast lock");
		}
	} else {
		if (fast_mode_wake_lock) {
			fast_mode_wake_lock = 0;
			test_wake_unlock();
			SYS_LOG_INF("Le fast unlock");
		}
	}
}

/* Be careful: If have multi ble module need operate update ble parameter,  need only operate in one place. */
static void ble_speed_check_param_delaywork(os_work *work)
{
	int phone_type, br_busy;
	uint16_t curr_int = 0, curr_lat = 0;
	uint16_t need_min_int, need_max_int;
	uint8_t need_speed_leve = BLE_SPEED_LEVEL_IDLE;
	
#ifdef CONFIG_GATT_OVER_BREDR
	if (!bt_manager_ble_is_connected()) {
		ble_speed_param_set_wakelock(false);
		return;
	}
#endif

	if (!ble_connected_flag) {
		ble_speed_param_set_wakelock(false);
		return;
	}

	phone_type = bt_manager_ble_get_phone_type();
	br_busy = bt_manager_ble_is_br_busy();
	if (bt_manager_ble_get_param(&curr_int, &curr_lat, NULL)) {
		goto next_work;
	}

	if (curr_int == 0) {
		goto next_work;
	}

	if (fast_speed_mode) {
		need_speed_leve = br_busy ? BLE_SPEED_LEVEL_BUSY : BLE_SPEED_LEVEL_FAST;
	}

	curr_int += curr_int*curr_lat;
	need_min_int = le_param[phone_type][need_speed_leve].interval_min * (1 + le_param[phone_type][need_speed_leve].latency);
	need_max_int = le_param[phone_type][need_speed_leve].interval_max * (1 + le_param[phone_type][need_speed_leve].latency);

	if ((curr_int < need_min_int) || (curr_int > need_max_int)) {
		/* Be careful: If failed to update several times, refer to stop update again */
		SYS_LOG_INF("App set param %d %d", phone_type, need_speed_leve);
		bt_manager_ble_update_param(&le_param[phone_type][need_speed_leve]);
		goto next_work;
	} else {
		if (need_speed_leve != BLE_SPEED_LEVEL_IDLE) {
			goto next_work;
		} else {
			/* In BLE_SPEED_LEVEL_IDLE and not need update parameter,
			 * stop delaywork, clear wakelock.
			 */
			ble_speed_param_set_wakelock(false);
			return;
		}
	}

next_work:
	os_delayed_work_submit(&ble_uparam_work, BLE_SPEED_CHECK_PARAM_INTERVAL);
}
#endif

static ssize_t speed_write_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	//SYS_LOG_INF("len %d\n", len);

	ble_speed_rx_data((uint8_t *)buf, len);
	return len;
}

static void speed_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	SYS_LOG_INF("value: %d\n", value);
	ble_speed_notify_enable = (uint8_t)value;
}

static struct bt_gatt_attr ble_speed_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(SPEED_BLE_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(SPEED_BLE_WRITE_UUID, BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, speed_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(SPEED_BLE_READ_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(speed_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static void ble_speed_test_delaywork(os_work *work)
{
	static uint32_t curr_time;
	static uint32_t pre_time;
	static uint32_t TxCount;
	uint16_t mtu;
	static uint8_t data = 0;
	uint8_t i, j, repeat;
	int ret;

	if (ble_speed_tx_flag && ble_speed_notify_enable) {
		mtu = bt_manager_get_ble_mtu() - 3;
		mtu = (mtu > BLE_TEST_SEND_SIZE) ? BLE_TEST_SEND_SIZE : mtu;

#if 0
		if (mtu <= 23) {
			repeat = 10;
		} else if (mtu <=  64) {
			repeat = 4;
		} else {
			repeat = 1;
		}
#else
		if (!bt_manager_ble_is_connected()) {
			repeat = 1;
		} else {
			repeat = 10;
		}
#endif

		for (j = 0; j < repeat; j++) {
			for (i = 0; i < mtu; i++) {
				ble_speed_send_buf[i] = data++;
			}

			ret = bt_manager_ble_send_data(&ble_speed_attrs[3], &ble_speed_attrs[4], ble_speed_send_buf, mtu);
			if (ret < 0) {
				break;
			}

			TxCount += mtu;
			curr_time = k_uptime_get_32();
			if ((curr_time - pre_time) >= 1000) {
				printk("Tx: %d byte\n", TxCount);
				TxCount = 0;
				pre_time = curr_time;
			}

			os_yield();
		}

		os_delayed_work_submit(&ble_speed_test_work, BLE_TEST_WORK_INTERVAL);
	}
}

static void test_ble_speed_start_stop_delaywork(void)
{
	if (!ble_speed_tx_flag) {
		ble_speed_tx_flag = 1;
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		fast_speed_mode = 1;
		ble_speed_fast_mode_set_wakelock(true);
		ble_speed_param_set_wakelock(true);
		os_delayed_work_submit(&ble_uparam_work, 0);
#endif
		os_delayed_work_submit(&ble_speed_test_work, BLE_TEST_WORK_INTERVAL);
		SYS_LOG_INF("BLE tx start\n");
	} else {
		ble_speed_tx_flag = 0;
		os_delayed_work_cancel(&ble_speed_test_work);
		SYS_LOG_INF("BLE tx stop\n");
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		fast_speed_mode = 0;
		ble_speed_fast_mode_set_wakelock(false);
#endif
	}
}

static int ble_speed_rx_data(uint8_t *buf, uint16_t len)
{
	static uint32_t curr_time;
	static uint32_t pre_time;
	static uint32_t RxCount;

	if (len == strlen(ble_speed_tx_trigger)) {
		if (memcmp(buf, ble_speed_tx_trigger, len) == 0) {
			test_ble_speed_start_stop_delaywork();
		}
	}

	RxCount += len;
	curr_time = k_uptime_get_32();
	if ((curr_time - pre_time) >= 1000) {
		printk("Rx: %d byte\n", RxCount);
		RxCount = 0;
		pre_time = curr_time;
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		if (!fast_speed_mode)
		{
			fast_speed_mode = 1;
			os_delayed_work_submit(&ble_uparam_work, 0);
		}
#endif
	}

	return 0;
}

static void ble_speed_connect_cb(uint8_t *mac, uint8_t connected)
{
	SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
	SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if (connected) {
		ble_connected_flag = 1;
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		fast_speed_mode = 0;
		os_delayed_work_submit(&ble_uparam_work, BLE_SPEED_CHECK_PARAM_INTERVAL);
		ble_speed_param_set_wakelock(true);
#endif
	} else {
		ble_connected_flag = 0;
		ble_speed_tx_flag = 0;
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		fast_speed_mode = 0;
		ble_speed_fast_mode_set_wakelock(false);
		os_delayed_work_cancel(&ble_uparam_work);
		ble_speed_param_set_wakelock(false);
#endif
	}
}

static struct ble_reg_manager ble_speed_mgr = {
	.link_cb = ble_speed_connect_cb,
};

static int shell_cmd_btble_reg(const struct shell *shell, size_t argc, char *argv[])
{
	static uint8_t reg_flag = 0;

	if (reg_flag) {
		SYS_LOG_INF("Already register\n");
	} else {
		ble_speed_mgr.gatt_svc.attrs = ble_speed_attrs;
		ble_speed_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_speed_attrs);

		os_delayed_work_init(&ble_speed_test_work, ble_speed_test_delaywork);
#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
		os_delayed_work_init(&ble_uparam_work, ble_speed_check_param_delaywork);
#endif
#endif

#ifdef CONFIG_GATT_OVER_BREDR
		extern uint16_t bt_gobr_sdp_handle_get(void);
		ble_speed_mgr.gatt_svc.sdp_handle = bt_gobr_sdp_handle_get();
#endif
		bt_manager_ble_service_reg(&ble_speed_mgr);
		reg_flag = 1;

		SYS_LOG_INF("Register ble service\n");
	}
	return 0;
}

static int shell_cmd_btble_send(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t mtu, i;
	static uint8_t data = 0;

	if (ble_speed_notify_enable) {
		if (argc >= 2) {
			if (!strcmp(argv[1], "continue")) {
				test_ble_speed_start_stop_delaywork();
				return 0;
			}
		}

		mtu = bt_manager_get_ble_mtu() - 3;
		mtu = (mtu > BLE_TEST_SEND_SIZE) ? BLE_TEST_SEND_SIZE : mtu;
		SYS_LOG_INF("Ble send len %d", mtu);

		for (i = 0; i < mtu; i++) {
			ble_speed_send_buf[i] = data++;
		}
		bt_manager_ble_send_data(&ble_speed_attrs[3], &ble_speed_attrs[4], ble_speed_send_buf, mtu);
	} else {
		SYS_LOG_INF("Ble not connected or notify not enable!");
	}

	return 0;
}

static int shell_ble_set_security(const struct shell *shell, size_t argc, char *argv[])
{
	void bt_manager_set_security(void);
	bt_manager_set_security();

	return 0;
}
#else
#define TEST_BLE_SERVICE_UUID		BT_UUID_DECLARE_16(0xFFC0)
#define TEST_BLE_WRITE_UUID			BT_UUID_DECLARE_16(0xFFC1)
#define TEST_BLE_NOTIFY_UUID		BT_UUID_DECLARE_16(0xFFC2)
#define TEST_BLE_INDICATE_UUID		BT_UUID_DECLARE_16(0xFFC3)

#define TEST_RW_DATA_LEN	10

static u8_t ble_test_notify_enable = 0;
static u8_t ble_test_indicate_enable = 0;
//static struct bt_gatt_ccc_cfg g_test_ccc_cfg_notify[1];
//static struct bt_gatt_ccc_cfg g_test_ccc_cfg_indicate[1];
static u8_t test_rw_data[TEST_RW_DATA_LEN] = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39};

static ssize_t test_read_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	u16_t copy_len = min(len, TEST_RW_DATA_LEN);

	SYS_LOG_INF("copy_len %d\n", copy_len);
	memcpy(buf, test_rw_data, copy_len);
	return copy_len;
}

static ssize_t test_write_cb(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, u16_t len, u16_t offset,
			      u8_t flags)
{
	u16_t i;
	u8_t *pData = (u8_t *)buf;
	u16_t copy_len;

	SYS_LOG_INF("Rx data:");
	for (i = 0; i < len; i++) {
		printf(" %02x", pData[i]);
	}
	printf("\n");

	copy_len = min(len, TEST_RW_DATA_LEN);
	memcpy(test_rw_data, buf, copy_len);
	return len;
}
#if 1
static void test_ccc_cfg_notify(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d\n", value);
	ble_test_notify_enable = (u8_t)value;
}

static void test_ccc_cfg_indicate(const struct bt_gatt_attr *attr, u16_t value)
{
	SYS_LOG_INF("value: %d\n", value);
	ble_test_indicate_enable = (u8_t)value;
}
#endif
static struct bt_gatt_attr ble_test_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(TEST_BLE_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(TEST_BLE_WRITE_UUID, BT_GATT_CHRC_READ|BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
							BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, test_read_cb, test_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(TEST_BLE_NOTIFY_UUID, BT_GATT_CHRC_NOTIFY,
							BT_GATT_PERM_READ|BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	//BT_GATT_CCC(g_test_ccc_cfg_notify, test_ccc_cfg_notify),
	//BT_GATT_CCC(test_ccc_cfg_notify, 0),
	BT_GATT_CCC(test_ccc_cfg_notify, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	//BT_GATT_CCC(NULL, 0),

	BT_GATT_CHARACTERISTIC(TEST_BLE_INDICATE_UUID, BT_GATT_CHRC_INDICATE,
							BT_GATT_PERM_READ|BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	//BT_GATT_CCC(g_test_ccc_cfg_indicate, test_ccc_cfg_indicate)
	//BT_GATT_CCC(test_ccc_cfg_indicate, 0),
	BT_GATT_CCC(test_ccc_cfg_indicate, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
	//BT_GATT_CCC(NULL, 0)
};

static void ble_test_connect_cb(u8_t *mac, u8_t connected)
{
	SYS_LOG_INF("BLE %s\n", connected ? "connected" : "disconnected");
	SYS_LOG_INF("MAC %2x:%2x:%2x:%2x:%2x:%2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static struct ble_reg_manager ble_test_mgr = {
	.link_cb = ble_test_connect_cb,
};

static int shell_cmd_btble_reg(int argc, char *argv[])
{
	static u8_t reg_flag = 0;
	extern uint16_t bt_gobr_sdp_handle_get(void);

	if (reg_flag) {
		SYS_LOG_INF("Already register\n");
	} else {
		ble_test_mgr.gatt_svc.sdp_handle = bt_gobr_sdp_handle_get();
		ble_test_mgr.gatt_svc.attrs = ble_test_attrs;
		ble_test_mgr.gatt_svc.attr_count = ARRAY_SIZE(ble_test_attrs);

		bt_manager_ble_service_reg(&ble_test_mgr);
		reg_flag = 1;
		SYS_LOG_INF("Register ble service\n");
	}
	return 0;
}

static int shell_cmd_btble_notify(int argc, char *argv[])
{
	int ret = -EACCES;

	if (ble_test_notify_enable) 
	{
		ret = bt_manager_ble_send_data(&ble_test_attrs[3], &ble_test_attrs[4], test_rw_data, TEST_RW_DATA_LEN);
	}
	SYS_LOG_INF("ret %d\n", ret);
	return 0;
}

static int shell_cmd_btble_indicate(int argc, char *argv[])
{
	int ret = -EACCES;

	if (ble_test_indicate_enable) 
	{
		ret = bt_manager_ble_send_data(&ble_test_attrs[6], &ble_test_attrs[7], test_rw_data, TEST_RW_DATA_LEN);
	}
	SYS_LOG_INF("ret %d\n", ret);
	return 0;
}
#endif

#endif

#if MGR_SPPBLE_STREAM_TEST_SHELL
#define SPPBLE_TEST_STACKSIZE	(1024*2)

io_stream_t sppble_stream;
static uint8_t sppble_stream_opened = 0;

static os_work_q test_sppble_q;
static os_delayed_work sppble_connect_delaywork;
static os_delayed_work sppble_disconnect_delaywork;
static os_delayed_work sppble_run_delaywork;
static uint8_t sppble_test_stack[SPPBLE_TEST_STACKSIZE] __aligned(4);

/* SPP  */
/* UUID: "00001101-0000-1000-8000-00805F9B34FB" */
static const uint8_t sppble_ota_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00};

/* BLE */
/* UUID: "e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" */
#define BLE_OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* UUID: "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" */
#define BLE_OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* UUID: "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" */
#define BLE_OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

static struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(BLE_OTA_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(BLE_OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP|BT_GATT_CHRC_READ,
							BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BLE_OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY|BT_GATT_CHRC_READ,
							BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(NULL, 0)
};

static void test_connect_delaywork(os_work *work)
{
	int ret;

	if (sppble_stream) {
		ret = stream_open(sppble_stream, MODE_IN_OUT);
		if (ret) {
			SYS_LOG_ERR("stream_open Failed\n");
		} else {
			sppble_stream_opened = 1;
			os_delayed_work_submit_to_queue(&test_sppble_q, &sppble_run_delaywork, 0);
		}
	}
}

static void test_disconnect_delaywork(os_work *work)
{
	int ret;

	if (sppble_stream) {
		ret = stream_close(sppble_stream);
		if (ret) {
			SYS_LOG_ERR("stream_close Failed\n");
		} else {
			sppble_stream_opened = 0;
		}
	}
}

static uint8_t read_buf[512];

static void test_run_delaywork(os_work *work)
{
	int ret;

	if (sppble_stream && sppble_stream_opened) {
		ret = stream_read(sppble_stream, read_buf, 512);
		if (ret > 0) {
			SYS_LOG_INF("sppble_stream rx: %d\n", ret);
			ret = stream_write(sppble_stream, read_buf, ret);
			if (ret > 0) {
				SYS_LOG_INF("sppble_stream tx: %d\n", ret);
			}
		}

		os_delayed_work_submit_to_queue(&test_sppble_q, &sppble_run_delaywork, 0);
	}
}

static void test_sppble_connect(bool connected, uint8_t connect_type)
{
	SYS_LOG_INF("%s\n", (connected) ? "connected" : "disconnected");
	if (connected) {
		os_delayed_work_submit_to_queue(&test_sppble_q, &sppble_connect_delaywork, 0);
	} else {
		os_delayed_work_submit_to_queue(&test_sppble_q, &sppble_disconnect_delaywork, 0);
	}
}

static int shell_cmd_sppble_stream_reg(const struct shell *shell, size_t argc, char *argv[])
{
	struct sppble_stream_init_param init_param;
	static uint8_t reg_flag = 0;

	if (reg_flag) {
		SYS_LOG_INF("Already register\n");
		return -EIO;
	}

	os_work_q_start(&test_sppble_q, (os_thread_stack_t *)sppble_test_stack, SPPBLE_TEST_STACKSIZE, 11);
	os_delayed_work_init(&sppble_connect_delaywork, test_connect_delaywork);
	os_delayed_work_init(&sppble_disconnect_delaywork, test_disconnect_delaywork);
	os_delayed_work_init(&sppble_run_delaywork, test_run_delaywork);

	memset(&init_param, 0, sizeof(struct sppble_stream_init_param));
	init_param.spp_uuid = (uint8_t *)sppble_ota_spp_uuid;
	init_param.gatt_attr = ota_gatt_attr;
	init_param.attr_size = ARRAY_SIZE(ota_gatt_attr);
	init_param.tx_chrc_attr = &ota_gatt_attr[3];
	init_param.tx_attr = &ota_gatt_attr[4];
	init_param.tx_ccc_attr = &ota_gatt_attr[5];
	init_param.rx_attr = &ota_gatt_attr[2];
	init_param.connect_cb = test_sppble_connect;
	init_param.read_timeout = OS_FOREVER;	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.write_timeout = OS_FOREVER;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	sppble_stream = sppble_stream_create(&init_param);
	if (!sppble_stream) {
		SYS_LOG_ERR("stream_create failed\n");
	}

	reg_flag = 1;
	return 0;
}
#endif

#if MGR_BT_LINKKEY_TEST_SHELL
static int shell_cmd_br_get_linkkey(const struct shell *shell, size_t argc, char *argv[])
{
    struct bt_linkkey_info info[8];
    uint8_t cnt;

    cnt = bt_manager_get_linkkey(&info[0],8);
	SYS_LOG_INF("%d",cnt);
	return 0;
}

static int shell_cmd_br_clear_linkkey(const struct shell *shell, size_t argc, char *argv[])
{
	bd_address_t addr;
	int err;

	if (argc < 2) {
        bt_manager_clear_linkkey(NULL);
        return 0;
	}

	err = mgr_str2bt_addr(argv[1], &addr);
	if (err) {
		return err;
	}

	bt_manager_clear_linkkey(&addr);
	return 0;
}

static int shell_cmd_ble_clear_linkkey(const struct shell *shell, size_t argc, char *argv[])
{
    bt_addr_le_t le_addr;
	int err;

    if (argc < 2) {
        bt_manager_ble_clear_linkkey(NULL);
        return 0;
	}

	err = mgr_str2bt_addr(argv[1], (bd_address_t *)(&le_addr.a));
	if(err) {
		return err;
	}

    le_addr.type = 0;
    bt_manager_ble_clear_linkkey(&le_addr);

	return 0;
}

static int shell_cmd_ble_find_linkkey(const struct shell *shell, size_t argc, char *argv[])
{
    bt_addr_le_t le_addr;
	int err;

	if (argc < 2) {
        return -EINVAL;
	}

	err = mgr_str2bt_addr(argv[1],(bd_address_t *)(&le_addr.a));
	if (err) {
		return err;
	}

    le_addr.type = 0;
	err = bt_manager_ble_find_linkkey(&le_addr);

	SYS_LOG_INF("%d",err);
	return 0;
}
#endif

#if MGR_BLE_HID_TEST_SHELL
static int shell_cmd_ble_hid_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t button = strtoul(argv[1], NULL, 16);
    bt_manager_ble_hid_report(button);
	return 0;
}
#endif

#ifdef CONFIG_BR_SDP_ACTIVE_REGISTER
void bt_register_a2dp_sink_sdp(void);
void bt_register_a2dp_source_sdp(void);
void bt_register_avrcp_ct_sdp(void);
void bt_register_avrcp_tg_sdp(void);
void bt_register_hfp_hf_sdp(void);
void bt_register_hfp_ag_sdp(void);

#ifdef CONFIG_GATT_OVER_BREDR
void bt_register_gobr_sdp(void);
#endif

void bt_unregister_a2dp_sink_sdp(void);
void bt_unregister_a2dp_source_sdp(void);
void bt_unregister_avrcp_ct_sdp(void);
void bt_unregister_avrcp_tg_sdp(void);
void bt_unregister_hfp_hf_sdp(void);
void bt_unregister_hfp_ag_sdp(void);

#ifdef CONFIG_GATT_OVER_BREDR
void bt_unregister_gobr_sdp(void);
#endif

static int shell_cmd_sdp_register_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);

	switch (profile) {
	case 1:
		bt_register_a2dp_sink_sdp();
		break;
	case 2:
		bt_register_a2dp_source_sdp();
		break;
	case 3:
		bt_register_avrcp_ct_sdp();
		break;
	case 4:
		bt_register_avrcp_tg_sdp();
		break;
	case 5:
		bt_register_hfp_hf_sdp();
		break;
	case 6:
		bt_register_hfp_ag_sdp();
		break;
	case 7:
#ifdef CONFIG_GATT_OVER_BREDR
		bt_register_gobr_sdp();
#endif
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

static int shell_cmd_sdp_unregister_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);

	switch (profile) {
	case 1:
		bt_unregister_a2dp_sink_sdp();
		break;
	case 2:
		bt_unregister_a2dp_source_sdp();
		break;
	case 3:
		bt_unregister_avrcp_ct_sdp();
		break;
	case 4:
		bt_unregister_avrcp_tg_sdp();
		break;
	case 5:
		bt_unregister_hfp_hf_sdp();
		break;
	case 6:
		bt_unregister_hfp_ag_sdp();
		break;
	case 7:
#ifdef CONFIG_GATT_OVER_BREDR
		bt_unregister_gobr_sdp();
#endif
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

/**
* ??£§°ß???a1???°ß??°ß°„??|®¨°ßa°ßo??|®¨?°ß°È??3°ß?°ß°ß????®∫o
* 
* ???aA2dp o°ßaavrcp
* a)	?°ß°ß|®¨??°ß??bt_unregister_a2dp_sink_sdp?®∫?bt_unregister_avrcp_ct_sdp?®∫?bt_unregister_avrcp_tg_sdp
* b)	?°ß°‰|®¨??°ß??hostif_bt_a2dp_disable(0);o°ßahostif_bt_avrcp_disable(0);
* c)	?°ß°‰|®¨??°ß?? btif_a2dp_disconnect???a a2dp °ßa?°ßo?®§|®¨??°ß?? btif_avrcp_disconnect ???aavrcp??®∫

* °ß°È??°ß?A2dp o°ßaavrcp
* a)	?°ß°ß|®¨??°ß??bt_register_a2dp_sink_sdp?®∫?bt_register_avrcp_ct_sdp?®∫?bt_register_avrcp_tg_sdp
* b)	?°ß°‰|®¨??°ß??hostif_bt_a2dp_enable(0); o°ßahostif_bt_avrcp_enable(0);
* c)	?°ß°‰|®¨??°ß??btif_a2dp_connect ?®∫?°ßa?°ßo?®§|®¨??°ß??btif_avrcp_connect °ß°È?°ß|?avrcp??®¶
* 
* ??????D???®∫?°ß°„a?®¢?2s|®¨??°Ë°ß°Ë???
*/

static int shell_cmd_profile_register_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);

	switch (profile) {
	case 1:
		hostif_bt_a2dp_enable(0);
		break;
	case 2:
		hostif_bt_avrcp_enable(0);
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

static int shell_cmd_profile_unregister_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);

	switch (profile) {
	case 1:
		hostif_bt_a2dp_disable(0);
		break;
	case 2:
		hostif_bt_avrcp_disable(0);
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

int btsrv_a2dp_disconnect(struct bt_conn *conn);
int btsrv_a2dp_connect(struct bt_conn *conn, uint8_t role);
int btsrv_avrcp_disconnect(struct bt_conn *conn);
int btsrv_avrcp_connect(struct bt_conn *conn);
static int shell_cmd_a2dp_switch_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);
	uint32_t conn = strtoul(argv[2], NULL, 16);

	SYS_LOG_INF("conn 0x%p.",conn);
	switch (profile) {
	case 0:
		btsrv_a2dp_disconnect((struct bt_conn *)conn);
		break;
	case 1:
		btsrv_a2dp_connect((struct bt_conn *)conn, 0x02);
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

static int shell_cmd_avrcp_switch_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t profile = strtoul(argv[1], NULL, 16);
	uint32_t conn = strtoul(argv[2], NULL, 16);

	SYS_LOG_INF("conn 0x%p.",conn);
	switch (profile) {
	case 0:
		btsrv_avrcp_disconnect((struct bt_conn *)conn);
		break;
	case 1:
		btsrv_avrcp_connect((struct bt_conn *)conn);
		break;
	default:
		SYS_LOG_ERR("Unknown profile type %u",profile);
		return -EINVAL;
	}

	return 0;
}

static int shell_cmd_music_switch_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t enable = strtoul(argv[1], NULL, 16);

	SYS_LOG_INF("enable %d.",enable);
	if (enable) {
		bt_manager_a2dp_enable();
	} else {
		bt_manager_a2dp_disable();
	}

	return 0;
}
#endif

#if MGR_BR_HID_TEST_SHELL
static int shell_cmd_br_hid_send(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_hid_take_photo();
	return 0;
}

static int shell_cmd_br_hid_connect(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_hid_reconnect();
	return 0;
}

static int shell_cmd_br_hid_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_hid_disconnect();
	return 0;
}
#endif

#ifdef CONFIG_OTA_PRODUCT_SUPPORT
static int shell_br_ota_trans_start(const struct shell *shell, size_t argc, char *argv[])
{
	void ota_product_ota_start(void);

	ota_product_ota_start();
	return 0;
}

static int shell_br_ota_trans_stop(const struct shell *shell, size_t argc, char *argv[])
{
	void ota_product_ota_stop(void);

	ota_product_ota_stop();
	return 0;
}
#endif

#ifdef CONFIG_NSM_APP
#include <dvfs.h>

#define NSM_FCC_TEST 	 (1)
//#define SRAM_LOG_SUPPORT (1)
#endif

#ifdef NSM_FCC_TEST
struct ft_env_var {
		void (*ft_printf)(const char *fmt, ...);
		void (*ft_udelay)(unsigned int us);
		void (*ft_mdelay)(unsigned int ms);
		uint32_t (*ft_get_time_ms)(void);
		int (*ft_efuse_write_32bits)(uint32_t bits, uint32_t num);
		uint32_t (*ft_efuse_read_32bits)(uint32_t num, uint32_t* efuse_value);
		void (*ft_load_fcc_bin)(void);
};
extern struct ft_env_var global_ft_env_var;
int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);
void ft_load_fcc_bin(void);
void sys_pm_reboot(int type);
void bt_nsm_prepare_disconnect(void);

static void sram_log_print(void) 
{
#ifdef SRAM_LOG_SUPPORT
	extern char *sram_log_addr;
	int i,rd = 0;
	char print_buf[128];
	char *ptr = print_buf;

	memset(print_buf, 0, 128);
	for(i = 8; i < 2048; i++) {
		if (((128-2) == rd) ||
			('\n' == sram_log_addr[i])) {
			print_buf[rd] = '\n';
			rd++;
			print_buf[rd] = '\0';
			printk("%s",ptr);
			rd = 0;
			memset(print_buf, 0, 128);
		} else if ('\n' != sram_log_addr[i]) {
			print_buf[rd] = sram_log_addr[i];
			rd++;
		}
	}

	os_sleep(100);
#endif
}
static int shell_cmd_nsm_uart(const struct shell *shell, size_t argc, char *argv[])
{
#if 1//def UART_FCC_TEST
	uint32_t flags;

	// media_player_force_stop();
	bt_nsm_prepare_disconnect();
	os_sleep(100);

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");
	printk_dma_switch(0);
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	SYS_LOG_INF("fcc_test_main");
	fcc_test_main(0, NULL, NULL);
	//k_sched_unlock();
	//irq_unlock(flags);
	printk("Fcc test exit.\n");/* fcc_test_main parameter error will print this log */
	sram_log_print();

	printk_dma_switch(1);
	sys_pm_reboot(0);
	return 0;
#endif
}

static int shell_cmd_nsm_air_tx(const struct shell *shell, size_t argc, char *argv[])
{
#if 1//def AIR_TX_FCC_TEST
	int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);
	void ft_load_fcc_bin(void);
	uint32_t flags, result;
	extern struct ft_env_var global_ft_env_var;

	uint8_t bt_param[9];

	// media_player_force_stop();
	bt_nsm_prepare_disconnect();
	os_sleep(100);
	/** bt_param(For tx mode):
	*		byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
	*		byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
	*		byte2 : channel //tx channel	 (0-79)
	*		byte3 : tx_power_mode //tx power	   (0-43)
	*		byte4 : tx_mode //tx mode, DH1/DH3/DH5	  (9-19, !=12)
	*		byte5 : payload_mode //payload mode   (0-6)
	*		byte6 : excute mode // excute mode (0-2)
	*		byte7 : test time // unit : s
	*/
	bt_param[0] = 1;
	bt_param[1] = 0;
	bt_param[2] = 0;
	bt_param[3] = 38;
	bt_param[4] = 0x14;
	bt_param[5] = 0x10; // no use by BLE, to set invalid value.
	bt_param[6] = 1;
	bt_param[7] = 10;
	bt_param[8] = 0;

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");
	printk_dma_switch(0);
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	SYS_LOG_INF("fcc_test_main");
	result = fcc_test_main(1, bt_param, NULL);
	//k_sched_unlock();
	//irq_unlock(flags);
	printk("Fcc test result %d\n", result);	/* fcc_test_main parameter error will print this log */
	sram_log_print();

	printk_dma_switch(1);
	sys_pm_reboot(0);
	return 0;
#endif
}

static int shell_cmd_nsm_air_rx(const struct shell *shell, size_t argc, char *argv[])
{
#if 1//def AIR_RX_FCC_TEST
	int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);
	void ft_load_fcc_bin(void);
	uint32_t flags, result;
	extern struct ft_env_var global_ft_env_var;

	uint8_t rx_param[11], report[18];
	uint8_t save_test_index = 0, save_test_result = 1, save_report[16], i;

	memset(save_report, 0, sizeof(save_report));
	property_get("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_get("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_get("FCC_TEST_REPORT", (char *)&save_report, 16);

	SYS_LOG_INF("Pre test index %d result %d", save_test_index, save_test_result);
	for (i=0; i<16; i++) {
		SYS_LOG_INF("Pre test report %d = 0x%x", i, save_report[i]);
	}

	// media_player_force_stop();
	bt_nsm_prepare_disconnect();
	os_sleep(100);
	/* * rx_param(For rx mode):
		*	byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
		*	byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		*	byte[2-5]:	access code
		*	byte6 : channel //rx channel	 (0-79)
		*	byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
		*	byte8 : excute mode // excute mode, 0: one packet; 1:continue
		*	byte9 : test time // unit : s
		*	rx_report(Only for rx mode):
		*					buffer[16]: 16byte report.
		* Return:  0:success; 1:failed
	*/

	rx_param[0] = 1; //1: BLE TEST
	rx_param[1] = 0; //0: BLE 1M
	rx_param[2] = 0x29;
	rx_param[3] = 0x46;
	rx_param[4] = 0x76;
	rx_param[5] = 0x71;
	rx_param[6] = 0;	//rx channel
	rx_param[7] = 0x12; /* rx mode 0x12. K_RX_MODE_LE_01010101 */
	rx_param[8] = 1;	/* continue */
	rx_param[9] = 10;	/* timeout */
	rx_param[10] = 0;

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");
	printk_dma_switch(0);
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	SYS_LOG_INF("fcc_test_main");
	result = fcc_test_main(2, rx_param, report);
	//k_sched_unlock();
	//irq_unlock(flags);

	printk("result %d\n", result);
	for (int i=0; i<17; i++) {
		printk("test report %d = 0x%x\n", i, report[i]);
	}
	printk("Fcc test result %d\n", result);	/* fcc_test_main parameter error will print this log */

	memset(save_report, 0, sizeof(save_report));
	if (result == 0) {
		memcpy(save_report, report, sizeof(save_report));
	}
	
	save_test_index++;
	save_test_result = (uint8_t)result;

	property_set("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_set("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_set("FCC_TEST_REPORT", (char *)&save_report, 16);
	property_set("FCC_TEST_RX_RSSI", (char *)&report[16], 1);
	property_flush(NULL);

	sram_log_print();

	printk_dma_switch(1);
	sys_pm_reboot(0);
	return 0;
#endif
}
#endif

static int shell_cmd_bt_reset(const struct shell *shell, size_t argc, char *argv[])
{
	int bt_manager_bt_reset_test(void);
	bt_manager_bt_reset_test();
	return 0;
}

static int shell_cmd_bt_crash(const struct shell *shell, size_t argc, char *argv[])
{
	int bt_manager_bt_crash_test(void);
	bt_manager_bt_crash_test();
	return 0;
}

#ifdef CONFIG_BT_BLE_IPS
#include <thread_timer.h>
#include <bt_manager_ips.h>
//struct thread_timer ips_adv_timer;
os_delayed_work ips_work;
bool ips_mic_status = false;
static void *ips_handle = NULL;
static uint8_t g_pkt_num[CONFIG_BT_PER_ADV_SYNC_MAX];
static void *sync_handle[CONFIG_BT_PER_ADV_SYNC_MAX];

static OS_MUTEX_DEFINE(t_ips_mutex);

static int test_ips_add_pa(void *sync)
{
	int i;

	if (!sync)
		return -ESRCH;

	os_mutex_lock(&t_ips_mutex, OS_FOREVER);
	for (i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
		if (sync_handle[i] == NULL) {
			sync_handle[i] = sync;
			//SYS_LOG_INF("a sync (%p) %p\n", sync,&ips_st->pa_st[i]);
			break;
		}
	}
	os_mutex_unlock(&t_ips_mutex);

	if (i == CONFIG_BT_PER_ADV_SYNC_MAX) {
		SYS_LOG_ERR("Failed to add pa %p", sync);
		return -EIO;
	}

	return 0;
}

static int test_ips_remove_pa(void *sync)
{
	int i;

	if (!sync )
		return -ESRCH;

	os_mutex_lock(&t_ips_mutex, OS_FOREVER);
	for (i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
		if (sync_handle[i] == sync) {
			//ips_st->pa_st[i].adv_sync = NULL;
			//SYS_LOG_INF("r sync (%p) %p\n", sync,&ips_st->pa_st[i]);
			sync_handle[i] = NULL;
			break;
		}
	}
	os_mutex_unlock(&t_ips_mutex);

	if (i == CONFIG_BT_PER_ADV_SYNC_MAX) {
		SYS_LOG_ERR("Failed to remove pa %p", sync);
		return -EIO;
	}

	return 0;
}

static uint8_t test_ips_pa_st_get(void *sync)
{
	int i;

	if (!sync)
		return 0;

	os_mutex_lock(&t_ips_mutex, OS_FOREVER);

	for (i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
		//SYS_LOG_INF("g sync (%p) %p\n", adv_sync,&ips_st->pa_st[i]);
		if (sync_handle[i] == sync) {
			os_mutex_unlock(&t_ips_mutex);
			return i;
		}
	}

	os_mutex_unlock(&t_ips_mutex);
	return 0;
}

void test_ips_synced(void *sync_handle, struct bt_ips_sync_info *info)
{
	SYS_LOG_INF("synced %p %d %s.", sync_handle, info->role, info->name);
	test_ips_add_pa(sync_handle);
}

void test_ips_term(void *sync_handle)
{
	SYS_LOG_INF("synced lost %x.", sync_handle);
	test_ips_remove_pa(sync_handle);
}

void test_ips_recv_codec(void *sync_handle, struct bt_ips_recv_info *info, uint8_t *data)
{
	SYS_LOG_INF("sync_handle %x %d.", sync_handle, info->pkt_num);
	uint8_t index = test_ips_pa_st_get(sync_handle);

	if (info->pkt_num != g_pkt_num[index] + 1)
		SYS_LOG_ERR("index %d %d %d.", index, g_pkt_num[index], info->pkt_num);

	g_pkt_num[index] = info->pkt_num;
}

void test_ips_status(void *sync_handle, uint8_t status, uint8_t num_synced)
{
	SYS_LOG_INF("status %d %d.", status, num_synced);
}

static const struct bt_ips_cb test_ips_cb = {
	.ips_synced = test_ips_synced,
	.ips_term = test_ips_term,
	.ips_recv_codec = test_ips_recv_codec,
	.ips_status = test_ips_status,
};

#if 0
void ips_adv_loop_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct bt_ips_send_info info;
	uint8_t data[40];

	SYS_LOG_INF("hdl: (%p)", ips_handle);

	info.length = 40; //ÂèëÈÄÅÁºñÁ†ÅÊï∞ÊçÆÈïøÂ∫¶
	info.frame_number = 1; //ÂèëÈÄÅÂ∏ßÊï∞
	memset(data, 0x66, 40);
	if (true == ips_mic_status)
		bt_manager_ips_send_codec(ips_handle, &info, data);
	else
		bt_manager_ips_send_codec(ips_handle, NULL, NULL);
}
#endif
static void shell_ips_adv_loop_work(struct k_work *work)
{
	struct bt_ips_send_info info;
	uint8_t data[40];

	//SYS_LOG_INF("hdl: (%p)", ips_handle);

	info.length = 40; //ÂèëÈÄÅÁºñÁ†ÅÊï∞ÊçÆÈïøÂ∫¶
	info.frame_number = 1; //ÂèëÈÄÅÂ∏ßÊï∞
	memset(data, 0x66, 40);
	if (true == ips_mic_status) {
		info.status = BT_IPS_STATUS_TALK;
		bt_manager_ips_send_codec(ips_handle, &info, data);
	} else {
		info.status = BT_IPS_STATUS_READY;
		bt_manager_ips_send_codec(ips_handle, &info, NULL);
	}
	os_delayed_work_submit(&ips_work, 20);
}

#define TEST_IPS_NAME "ips_tele"

void shell_ips_search_cb(struct bt_ips_search_rt *rt)
{
	struct bt_ips_init_info init_info;

	SYS_LOG_INF("search t_num %d role %d %s.", rt->t_num, rt->role, rt->name);
	if (BT_IPS_ROLE_INITIATOR == rt->role) {

	} else if ((4) == rt->t_num) {

	} else {
		return;
	}

	bt_manager_ips_search_close();
	init_info.match_id = 0x31323334; // ÂåπÈÖçÁ†Å, 4bytes ÈöèÊú∫Êï∞
	init_info.role = BT_IPS_ROLE_SUBSCRIBER;
	memcpy(init_info.name, TEST_IPS_NAME ,strlen(TEST_IPS_NAME));
	init_info.ch_max = rt->t_num; //ÊúÄÂ§öÂÖÅËÆ∏Êé•Êî∂ÁöÑÈü≥È¢ëÈÄöÈÅìÊï∞Èáè
	init_info.tx_codec = BT_IPS_TX_CODEC_OPUS; //Êú¨Êú∫ÂπøÊí≠ÁöÑÁºñÁ†ÅÊ†ºÂºè
	init_info.comp_ratio = BT_IPS_COMP_RATIO_8; // ÁºñÁ†ÅÂéãÁº©ÊØî
	init_info.ch_mode = BT_IPS_TX_CH_SINGLE; //ÂçïÂ£∞ÈÅìÊàñÂèåÂ£∞ÈÅì
	init_info.samp_freq = BT_IPS_SAMPLE_FREQ_8; //ÈááÊ†∑Áéá8KHZ or 16KHZ
	init_info.bit_width = BT_IPS_BIT_WIDTH_16; // ‰ΩçÂÆΩ8bits or 16bits

	bt_manager_ips_search_close();
	if (bt_manager_ips_start(&ips_handle, &init_info, (struct bt_ips_cb *)&test_ips_cb))
		return;

	os_delayed_work_init(&ips_work, shell_ips_adv_loop_work);
	os_delayed_work_cancel(&ips_work);
	os_delayed_work_submit(&ips_work, 20);
	//thread_timer_init(&ips_adv_timer,ips_adv_loop_timer, NULL);
	//thread_timer_start(&ips_adv_timer, 100, 100);
	SYS_LOG_INF("2 timer start.");
}

static int shell_cmd_ips_search(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_ips_search_init info;
	uint16_t ch = 1, role = BT_IPS_ROLE_INITIATOR;
	uint32_t shell_id = 0x31323334;

	if (argc >= 2) {
		role = strtoul(argv[1], NULL, 16);
	}

	if (argc >= 3) {
		ch = strtoul(argv[2], NULL, 16);
	}

	if (argc == 4) {
		shell_id = strtoul(argv[3], NULL, 16);
	}

	SYS_LOG_INF("shell match id %x.", shell_id);
	SYS_LOG_INF("ch max %d.", ch);

	info.match_id = shell_id; // ÂåπÈÖçÁ†Å, 4bytes ÈöèÊú∫Êï∞
	info.role = role;
	info.s_cb = shell_ips_search_cb;
	memcpy(info.name, TEST_IPS_NAME ,strlen(TEST_IPS_NAME));
	if (bt_manager_ips_search_open(&info))
		return 0;

	SYS_LOG_INF("search start.");
	return 0;
}

static int shell_cmd_ips_search_stop(const struct shell *shell, size_t argc, char *argv[])
{
	bt_manager_ips_search_close();
	SYS_LOG_INF("search stop.");
	return 0;
}

static int shell_cmd_ips_start(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_ips_init_info init_info;
	uint16_t ch = 1, role = BT_IPS_ROLE_INITIATOR;
	uint32_t shell_id = 0x31323334;

	if (argc >= 2) {
		role = strtoul(argv[1], NULL, 16);
	}

	if (argc >= 3) {
		ch = strtoul(argv[2], NULL, 16);
	}

	if (argc == 4) {
		shell_id = strtoul(argv[3], NULL, 16);
	}

	SYS_LOG_INF("shell match id %x.", shell_id);
	SYS_LOG_INF("ch max %d.", ch);

	init_info.match_id = shell_id; // ÂåπÈÖçÁ†Å, 4bytes ÈöèÊú∫Êï∞
	init_info.role = role;
	memcpy(init_info.name, TEST_IPS_NAME ,strlen(TEST_IPS_NAME));
	init_info.ch_max = ch; //ÊúÄÂ§öÂÖÅËÆ∏Êé•Êî∂ÁöÑÈü≥È¢ëÈÄöÈÅìÊï∞Èáè
	init_info.tx_codec = BT_IPS_TX_CODEC_OPUS; //Êú¨Êú∫ÂπøÊí≠ÁöÑÁºñÁ†ÅÊ†ºÂºè
	init_info.comp_ratio = BT_IPS_COMP_RATIO_8; // ÁºñÁ†ÅÂéãÁº©ÊØî
	init_info.ch_mode = BT_IPS_TX_CH_SINGLE; //ÂçïÂ£∞ÈÅìÊàñÂèåÂ£∞ÈÅì
	init_info.samp_freq = BT_IPS_SAMPLE_FREQ_8; //ÈááÊ†∑Áéá8KHZ or 16KHZ
	init_info.bit_width = BT_IPS_BIT_WIDTH_16; // ‰ΩçÂÆΩ8bits or 16bits

	bt_manager_ips_search_close();
	if (bt_manager_ips_start(&ips_handle, &init_info, (struct bt_ips_cb *)&test_ips_cb))
		return 0;

	os_delayed_work_init(&ips_work, shell_ips_adv_loop_work);
	os_delayed_work_cancel(&ips_work);
	os_delayed_work_submit(&ips_work, 20);
	//thread_timer_init(&ips_adv_timer,ips_adv_loop_timer, NULL);
	//thread_timer_start(&ips_adv_timer, 100, 100);
	SYS_LOG_INF("timer start.");

	return 0;
}

static int shell_cmd_ips_stop(const struct shell *shell, size_t argc, char *argv[])
{
	int i;
	ips_mic_status = false;
	os_delayed_work_cancel(&ips_work);
	bt_manager_ips_stop(ips_handle);
	ips_handle = NULL;
	for (i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++)
		sync_handle[i] = NULL;

	for (i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++)
		g_pkt_num[i] = 0;

	//thread_timer_stop(&ips_adv_timer);

	return 0;
}

static int shell_cmd_ips_mic_enable(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t enable = strtoul(argv[1], NULL, 16);

	SYS_LOG_INF("mic enable %d.",enable);
	if (enable) {
		ips_mic_status = true;
	} else {
		ips_mic_status = false;
	}

	return 0;
}
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(bt_mgr_cmds,
#if CONFIG_BT_BR_ACTS
	SHELL_CMD(discover, NULL, "BR discover", shell_cmd_btmgr_br_discover),
	SHELL_CMD(br_connect, NULL, "BR connect", shell_cmd_btmgr_br_connect),
	SHELL_CMD(br_disconnect, NULL, "BR disconnect", shell_cmd_btmgr_br_disconnect),
#endif
#if MGR_SPP_TEST_SHELL
	SHELL_CMD(spp_reg, NULL, "Register spp uuid", shell_cmd_btspp_reg),
	SHELL_CMD(spp_connect, NULL, "SPP connect", shell_cmd_btspp_spp_connect),
	SHELL_CMD(spp_disconnect, NULL, "SPP disconnect", shell_cmd_btspp_spp_disconnect),
	SHELL_CMD(spp_send, NULL, "SPP send data", shell_cmd_btspp_send),
#endif
#if MGR_PBAP_TEST_SHELL
	SHELL_CMD(pbap_connect, NULL, "Connect phonebook", shell_cmd_pbap_connect),
	SHELL_CMD(pbap_disconnect, NULL, "Disconnect phonebook", shell_cmd_pbap_disconnect),
	SHELL_CMD(pbap_op, NULL, "Get size/vcard/search/abort/stopget", shell_cmd_pbap_op),
#endif
#if MGR_MAP_TEST_SHELL
	SHELL_CMD(map_connect, NULL, "Connect MSE", cmd_test_map_connect),
	SHELL_CMD(map_abort, NULL, "Abort get message", cmd_test_map_abort),
	SHELL_CMD(map_disconnect, NULL, "Disconnect MSE", cmd_test_map_disconnect),
	SHELL_CMD(map_setfolder, NULL, "Set Folder", cmd_test_map_set_folder),
	SHELL_CMD(map_getmsglist, NULL, "Get Messages Listing", cmd_test_map_get_msg_list),
	SHELL_CMD(map_getfolderlist, NULL, "Get Folder Listing", cmd_test_map_get_folder_list),
#endif
#if MGR_BLE_TEST_SHELL
	SHELL_CMD(ble_reg, NULL, "Register ble service", shell_cmd_btble_reg),
#if (!defined(CONFIG_GATT_OVER_BREDR) || !defined(CONFIG_BT_PTS_TEST))
	SHELL_CMD(ble_send, NULL, "BLE send data", shell_cmd_btble_send),
	SHELL_CMD(ble_security, NULL, "ble_security", shell_ble_set_security),
#else
	SHELL_CMD(ble_notify, NULL, "ble notify send", shell_cmd_btble_notify),
	SHELL_CMD(ble_indicate, NULL, "ble indicate send", shell_cmd_btble_indicate),
#endif
#endif
#if MGR_SPPBLE_STREAM_TEST_SHELL
	SHELL_CMD(sppble_stream_reg, NULL, "Register sppble stream", shell_cmd_sppble_stream_reg),
#endif
#if MGR_BT_LINKKEY_TEST_SHELL
	SHELL_CMD(br_getlk, NULL, "br get linkkey", shell_cmd_br_get_linkkey),
	SHELL_CMD(br_clrlk, NULL, "br clear linkkey", shell_cmd_br_clear_linkkey),
	SHELL_CMD(ble_getlk, NULL, "ble get linkkey", shell_cmd_ble_find_linkkey),
	SHELL_CMD(ble_clrlk, NULL, "ble clear linkkey", shell_cmd_ble_clear_linkkey),
#endif
#if MGR_BLE_HID_TEST_SHELL
	SHELL_CMD(ble_hid, NULL, "ble hid test", shell_cmd_ble_hid_test),
#endif

#ifdef CONFIG_BR_SDP_ACTIVE_REGISTER
	SHELL_CMD(br_sdp_register, NULL, "br sdp sevice register", shell_cmd_sdp_register_test),
	SHELL_CMD(br_sdp_unregister, NULL, "br sdp sevice unregister", shell_cmd_sdp_unregister_test),
	SHELL_CMD(br_profile_register, NULL, "br profile sevice register", shell_cmd_profile_register_test),
	SHELL_CMD(br_profile_unregister, NULL, "br profile sevice unregister", shell_cmd_profile_unregister_test),
	SHELL_CMD(br_a2dp_switch, NULL, "br_a2dp connct or disconnect", shell_cmd_a2dp_switch_test),
	SHELL_CMD(br_avrcp_switch, NULL, "br_avrcp_switch connct or disconnect", shell_cmd_avrcp_switch_test),
	SHELL_CMD(br_music_switch, NULL, "bt music disable/enable api test", shell_cmd_music_switch_test),
#endif

#ifdef MGR_BR_HID_TEST_SHELL
	SHELL_CMD(br_hid, NULL, "br hid send test", shell_cmd_br_hid_send),
	SHELL_CMD(br_hid_connect, NULL, "br hid connect", shell_cmd_br_hid_connect),
	SHELL_CMD(br_hid_disconnect, NULL, "br hid disconnect", shell_cmd_br_hid_disconnect),
#endif

#ifdef CONFIG_OTA_PRODUCT_SUPPORT
	SHELL_CMD(br_ota_trans_start, NULL, "br ota_trans_start", shell_br_ota_trans_start),
	SHELL_CMD(br_ota_trans_stop, NULL, "br ota_trans_stop", shell_br_ota_trans_stop),
#endif

#ifdef NSM_FCC_TEST
	SHELL_CMD(nsm_uart, NULL, "uart fcc", shell_cmd_nsm_uart),
	SHELL_CMD(nsm_air_tx, NULL, "air fcc tx", shell_cmd_nsm_air_tx),
	SHELL_CMD(nsm_air_rx, NULL, "air fcc rx", shell_cmd_nsm_air_rx),
#endif
	SHELL_CMD(bt_reset, NULL, "bt_reset", shell_cmd_bt_reset),
	SHELL_CMD(bt_crash, NULL, "bt core crash", shell_cmd_bt_crash),

#ifdef CONFIG_BT_BLE_IPS
	SHELL_CMD(bt_ips_search, NULL, "ips_start", shell_cmd_ips_search),
	SHELL_CMD(bt_ips_search_stop, NULL, "ips_stop", shell_cmd_ips_search_stop),
	SHELL_CMD(bt_ips_start, NULL, "ips_start", shell_cmd_ips_start),
	SHELL_CMD(bt_ips_stop, NULL, "ips_stop", shell_cmd_ips_stop),
	SHELL_CMD(bt_ips_mic, NULL, "mic enable", shell_cmd_ips_mic_enable),
#endif
	SHELL_SUBCMD_SET_END
);

static int cmd_bt_mgr(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_REGISTER(btmgr, &bt_mgr_cmds, "Bluetooth manager test shell commands", cmd_bt_mgr);
