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
#include <view_stack.h>
#include <sys_wakelock.h>
#include <app_ui.h>
#include "alipay_ui.h"

static bool is_bind_inited = false;
static bool is_screen_locked = false;
static bool is_barcode_mode = false;
static int status_cnt = 0;

static void _wxpay_init(void)
{
}

static void _wxpay_start_adv(void)
{
}

static void _wxpay_stop_adv(void)
{
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
		is_bind_inited = false;
	}
}

static void _wxpay_do_unbind(void)
{
	status_cnt = 0;
}

static void _wxpay_sync_time(void)
{
}

static int _wxpay_get_binding_status(void)
{
	int status = STATE_BINDING_OK;

	// fake status for simulator
	status_cnt ++;
	if (status_cnt < 10) {
		status = STATE_WAIT_SCAN;
	} else if (status_cnt < 15) {
		status = STATE_BINDING;
	} else {
		status = STATE_BINDING_OK;
	}
	SYS_LOG_INF("ui binding status: %d", status);

	return status;
}

static int _wxpay_get_binding_string(uint8_t *buf, uint32_t *out_len)
{
	strcpy(buf, "https://lvgl.io");
	*out_len = strlen(buf);

	return 0;
}

static int _wxpay_get_paycode_string(uint8_t *buf, uint32_t *out_len)
{
	strcpy(buf, "286605383181667759");
	*out_len = strlen(buf);

	return 0;
}

static int _wxpay_get_userinfo(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len)
{
	strcpy(name, "alipay@163.com");
	*name_len = strlen(name);
	strcpy(id, "alipay");
	*id_len = strlen(id);

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

