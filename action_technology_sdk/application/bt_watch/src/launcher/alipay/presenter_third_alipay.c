/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CONFIG_SIMULATOR
#include <soc.h>
#endif
#include <os_common_api.h>
#include <ui_manager.h>
#include <input_manager.h>
#include <property_manager.h>
#include <bt_manager.h>
#include <view_stack.h>
#include <sys_wakelock.h>
#include <app_ui.h>
#include <alipay_third.h>
#include <vendor_ble.h>
#include <vendor_ble_alipay.h>
#include "alipay_ui.h"

static bool is_bind_inited = false;
static bool is_barcode_mode = false;
static bool is_screen_locked = false;

static void _alipay_start_adv(void)
{
	vendor_ble_alipay_start_adv();
}

static void _alipay_stop_adv(void)
{
	vendor_ble_alipay_stop_adv();
}

static void _alipay_bind_init(void)
{
	if (!is_bind_inited) {
		is_bind_inited = true;
	}
}

static void _alipay_bind_deinit(void)
{
	if (is_bind_inited) {
		pay_lib_init(0);
		is_bind_inited = false;
	}
}

static void _alipay_do_unbind(void)
{
	alipay_detect_bind();
	pay_lib_init(0);
}

static int _alipay_get_binding_status(void)
{
	int status = STATE_BINDING_OK;
	int binding_status;
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	printk("ui binding sec: %d\n", unix_sec);

	binding_status = alipay_get_bind_status(unix_sec, connected, 0);
	switch (binding_status) {
		case ALIPAY_BIND_START:
			status = STATE_WAIT_SCAN;
			break;

		case ALIPAY_BIND_ING:
		case ALIPAY_BIND_FINISH:
			status = STATE_BINDING;
			break;

		case ALIPAY_BIND_FAIL:
			status = STATE_BINDING_FAIL;
			break;

		case ALIPAY_BIND_OK:
		case ALIPAY_PAY_ERROR:
			status = STATE_BINDING_OK;
			break;
	}
	printk("ui binding status: %d\n", status);

	return status;
}

static int _alipay_get_binding_string(uint8_t *buf, uint32_t *out_len)
{
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	alipay_get_bind_status(unix_sec, connected, 1); // update bindcode
	alipay_get_bind_code(buf, (uint8_t*)out_len);
	buf[*out_len] = '\0';

	return 0;
}

static int _alipay_get_paycode_string(uint8_t *buf, uint32_t *out_len)
{
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	printk("paycode sec: %d\n", unix_sec);
	alipay_get_bind_status(unix_sec, connected, 1); // update paycode
	alipay_get_pay_code128(buf, (uint16_t*)out_len);

	buf[*out_len] = '\0';
	printk("paycode: %s\n", buf);

	return 0;
}

static int _alipay_get_userinfo(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len)
{
	alipay_get_userinfo(name, (uint16_t*)name_len, id, (uint16_t*)id_len);
	name[*name_len] = '\0';
	id[*id_len] = '\0';

	return 0;
}

static bool _alipay_is_barcode_on(void)
{
	return is_barcode_mode;
}

static void _alipay_toggle_barcode(void)
{
	is_barcode_mode = !is_barcode_mode;
}

static void _alipay_lock_screen(void)
{
	if (!is_screen_locked) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_lock(FULL_WAKE_LOCK);
#endif
		is_screen_locked = true;
	}
}

static void _alipay_unlock_screen(void)
{
	if (is_screen_locked) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
		is_screen_locked = false;
	}
}

static void _alipay_init_cb(uint8_t *data, uint8_t len)
{
	uint8_t pay_type = data[0];
	int status;

	printk("pay_lib_init: %d\n", pay_type);
	pay_lib_init(pay_type);

	status = _alipay_get_binding_status();
	if (status != STATE_BINDING_OK) {
		_alipay_bind_init();
	}
}

static void _alipay_init(void)
{
	uint8_t pay_type = 0;

	// init ble
	vendor_ble_alipay_init();

	// run pay_lib_init in ui_srv to avoid stack overflow
	paylib_ble_recv_cb(&pay_type, sizeof(pay_type), _alipay_init_cb);
}

static void _alipay_sync_time_cb(uint8_t *data, uint8_t len)
{
	uint32_t unix_sec = *(uint32_t*)data;
	bool connected = bt_manager_ble_is_connected();

	alipay_get_bind_status(unix_sec, connected, 2); //sync time
}

static void _alipay_sync_time(void)
{
	uint32_t unix_sec = third_paylib_get_unix_second();

	// symc time in ui_srv to avoid stack overflow
	paylib_ble_recv_cb((uint8_t*)&unix_sec, sizeof(unix_sec), _alipay_sync_time_cb);
}

const alipay_view_presenter_t alipay_view_presenter = {
	.init = _alipay_init,
	.start_adv = _alipay_start_adv,
	.stop_adv = _alipay_stop_adv,
	.bind_init = _alipay_bind_init,
	.bind_deinit = _alipay_bind_deinit,
	.do_unbind = _alipay_do_unbind,
	.sync_time = _alipay_sync_time,
	.get_binding_status = _alipay_get_binding_status,
	.get_binding_string = _alipay_get_binding_string,
	.get_paycode_string = _alipay_get_paycode_string,
	.get_userinfo = _alipay_get_userinfo,
	.is_barcode_on = _alipay_is_barcode_on,
	.toggle_barcode = _alipay_toggle_barcode,
	.lock_screen = _alipay_lock_screen,
	.unlock_screen = _alipay_unlock_screen,
};

