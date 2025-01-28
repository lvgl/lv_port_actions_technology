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
#include <vendor_ble_wxpay.h>
#include "alipay_ui.h"

static bool is_bind_inited = false;
static bool is_barcode_mode = false;
static bool is_screen_locked = false;

static void _wxpay_start_adv(void)
{
	vendor_ble_wxpay_start_adv();
}

static void _wxpay_stop_adv(void)
{
	vendor_ble_wxpay_stop_adv();
}

static void _wxpay_bind_init(void)
{
	if (!is_bind_inited) {
		is_bind_inited = true;
	}
}

static void _wxpay_bind_deinit(void)
{
	if (is_bind_inited) {
		pay_lib_init(1);
		is_bind_inited = false;
	}
}

static void _wxpay_do_unbind(void)
{
	wxpay_detect_bind();
	pay_lib_init(1);
}

static int _wxpay_get_binding_status(void)
{
	int status = STATE_BINDING_OK;
	int binding_status;
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	printk("ui binding sec: %d\n", unix_sec);

	binding_status = wxpay_get_bind_status(unix_sec, connected, 0);
	switch (binding_status) {
		case WXPAY_BIND_START:
			status = STATE_WAIT_SCAN;
			break;

		case WXPAY_BIND_ING:
		case WXPAY_BIND_FINISH:
			status = STATE_BINDING;
			break;

		case WXPAY_BIND_FAIL:
			status = STATE_BINDING_FAIL;
			break;

		case WXPAY_BIND_OK:
		case WXPAY_PAY_ERROR:
		case WXPAY_PAY_AGAIN_ERROR:
			status = STATE_BINDING_OK;
			break;
	}
	printk("ui binding status: %d\n", status);

	return status;
}

static int _wxpay_get_binding_string(uint8_t *buf, uint32_t *out_len)
{
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	wxpay_get_bind_status(unix_sec, connected, 1); // update bindcode
	wxpay_get_bind_code(buf, (uint8_t*)out_len);
	buf[*out_len] = '\0';

	return 0;
}

static int _wxpay_get_paycode_string(uint8_t *buf, uint32_t *out_len)
{
	uint32_t unix_sec = third_paylib_get_unix_second();
	bool connected = bt_manager_ble_is_connected();

	printk("paycode sec: %d\n", unix_sec);
	wxpay_get_bind_status(unix_sec, connected, 1); // update paycode
	wxpay_get_pay_code128(buf, (uint16_t*)out_len);
	buf[*out_len] = '\0';
	printk("paycode: %s\n", buf);

	return 0;
}

static int _wxpay_get_userinfo(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len)
{
	wxpay_get_userinfo(name, (uint16_t*)name_len, id, (uint16_t*)id_len);
	name[*name_len] = '\0';
	id[*id_len] = '\0';

	return 0;
}

static bool _wxpay_is_barcode_on(void)
{
	return is_barcode_mode;
}

static void _wxpay_toggle_barcode(void)
{
	is_barcode_mode = !is_barcode_mode;
}

static void _wxpay_lock_screen(void)
{
	if (!is_screen_locked) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_lock(FULL_WAKE_LOCK);
#endif
		is_screen_locked = true;
	}
}

static void _wxpay_unlock_screen(void)
{
	if (is_screen_locked) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
#endif
		is_screen_locked = false;
	}
}

static void _wxpay_init_cb(uint8_t *data, uint8_t len)
{
	uint8_t pay_type = data[0];
	int status;

	printk("pay_lib_init: %d\n", pay_type);
	pay_lib_init(pay_type);

	status = _wxpay_get_binding_status();
	if (status != STATE_BINDING_OK) {
		_wxpay_bind_init();
	}
}

static void _wxpay_init(void)
{
	uint8_t pay_type = 1;

	// init ble
	vendor_ble_wxpay_init();

	// run pay_lib_init in ui_srv to avoid stack overflow
	paylib_ble_recv_cb(&pay_type, sizeof(pay_type), _wxpay_init_cb);
}

static void _wxpay_sync_time(void)
{
//	uint32_t unix_sec = third_paylib_get_unix_second();
//	bool connected = bt_manager_ble_is_connected();

//	wxpay_get_bind_status(unix_sec, connected, 2); //sync time
}

const alipay_view_presenter_t wxpay_view_presenter = {
	.init = _wxpay_init,
	.start_adv = _wxpay_start_adv,
	.stop_adv = _wxpay_stop_adv,
	.bind_init = _wxpay_bind_init,
	.bind_deinit = _wxpay_bind_deinit,
	.do_unbind = _wxpay_do_unbind,
	.sync_time = _wxpay_sync_time,
	.get_binding_status = _wxpay_get_binding_status,
	.get_binding_string = _wxpay_get_binding_string,
	.get_paycode_string = _wxpay_get_paycode_string,
	.get_userinfo = _wxpay_get_userinfo,
	.is_barcode_on = _wxpay_is_barcode_on,
	.toggle_barcode = _wxpay_toggle_barcode,
	.lock_screen = _wxpay_lock_screen,
	.unlock_screen = _wxpay_unlock_screen,
};

