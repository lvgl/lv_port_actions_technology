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
#include <alipay.h>
#include <vendor_ble.h>
#include <vendor_ble_alipay.h>
#include "alipay_ui.h"
#include <drivers/rtc.h>
#include <drivers/nvram_config.h>
#include <alipay_net_srv.h>
#include <alipay_service.h>

static bool is_bind_inited = false;
static bool is_barcode_mode = false;
static bool is_screen_locked = false;
static uint32_t support_features;
#if CONFIG_TRANSPORT_LIB
static bool is_cardlist_exit = false;
static bool is_card_exit = false;
static int cardlist_status;
static int card_status;
static int last_card_index;
static char *transit_err_string;

static int _alipay_get_cardlist_status(void)
{
	return cardlist_status;
}

static void load_cardlist_callback(int ret, int param)
{
	if (ret == 0) {
		cardlist_status = STATE_LOAD_OK;
		if(is_cardlist_exit == false) {
			is_cardlist_exit = true;
		}
	} else {
		cardlist_status = STATE_LOAD_FAIL;
		if (ret == TRANSIT_CARD_LIST_NET_ERROR) {
			transit_err_string = "网络异常,请打开手机“乘车测试应用”app并确保服务已连接后再重试";
		} else {
			transit_err_string = "未知错误，请联系设备厂商解决";
		}
	}
}

static int _alipay_load_cardlist(uint8_t online)
{
	SYS_LOG_INF("alipay load cardlist %d", online);
	if (!online) {
		if(is_cardlist_exit == false) {
			cardlist_status = STATE_LOAD_IDLE;
			return -1;
		}
	}
	
	if (cardlist_status != STATE_LOADING) {
		cardlist_status = STATE_LOADING;
		alipay_load_cardlist(online, load_cardlist_callback);
	}

	return 0;
}

static int _alipay_get_card_num(void)
{
	transitcode_manager_t *cache = _alipay_get_transitcode_cache();
	
	return cache->cur_card_num;
}

static int _alipay_get_card_title(uint8_t index, uint8_t *buf, int len)
{
	transitcode_manager_t *cache = _alipay_get_transitcode_cache();
	
	if (len < sizeof(cache->card_list[index].title)) {
		return -1;
	}
	strcpy(buf, cache->card_list[index].title);
	return 0;
}

static int _alipay_get_card_status(void)
{
	return card_status;
}

static void load_card_callback(int ret, int param)
{
	if (card_status == STATE_LOADING && param == last_card_index) {
		if (ret == 0) {
			card_status = STATE_LOAD_OK;
			if (is_card_exit == false) {
				is_card_exit = true;
			}
		} else {
			card_status = STATE_LOAD_FAIL;
			if (ret == TRANSIT_CODE_NET_ERROR) {
				transit_err_string = "网络异常,请打开手机“乘车测试应用”app并确保服务已连接后再重试";
			} else {
				transit_err_string = "未知错误，请联系设备厂商解决";
			}
		}
	}
}

static int _alipay_load_card(int index)
{
	SYS_LOG_INF("alipay load card %d", index);
	if (index < 0) {
		if(is_card_exit == false) {
			return -1;
		}
	}
	
	if (card_status != STATE_LOADING || last_card_index != index) {
		card_status = STATE_LOADING;
		last_card_index = index;
		alipay_load_card(index, load_card_callback);
	}

	return 0;
}

static void _alipay_load_card_retry(void)
{
	SYS_LOG_INF("alipay load card retry");
	if (card_status != STATE_LOADING) {
		card_status = STATE_LOADING;
		alipay_load_card(last_card_index, load_card_callback);
	}
}

static int _alipay_get_transitcode(char* title, uint8_t* transitcode, uint32_t* len_transitcode)
{
	transitcode_manager_t *cache = _alipay_get_transitcode_cache();
	
	strcpy(title, cache->cur_card_info.title);
	*len_transitcode = cache->cur_card_info.len_transitcode;
	memcpy(transitcode, cache->cur_card_info.transitcode, *len_transitcode);
#if 0
	int i;
	SYS_LOG_INF("_alipay_get_transitcode, title: %s, len: %u", cur_card_info.title, *len_transitcode);
	for(i=0; i<*len_transitcode; i++) {
		printk("%02x ", transitcode[i]);
	}
	printk("\n");
#endif
	return 0;
}

static void _alipay_get_transit_guide_string(uint8_t *buf, uint32_t *out_len) {
	strcpy(buf, "alipays://platformapi/startapp?appId=20002047&scene=bus");
	*out_len = strlen(buf);
}

static char *_alipay_get_transit_error_string(void) {
	return transit_err_string;
}

#endif

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
		is_bind_inited = false;
	}
}

static void _alipay_do_unbind(void)
{
	retval_e ret;
	ret = alipay_unbinding();
	if (ret == ALIPAY_RV_OK) {
#if CONFIG_TRANSPORT_LIB
		cardlist_status = STATE_LOAD_IDLE;
		card_status = STATE_LOAD_IDLE;
		is_cardlist_exit = false;
		is_card_exit = false;
#endif
	}
}

static int _alipay_get_binding_status(void)
{
	int status = STATE_BINDING_OK;
	binding_status_e query_status;
	bool is_binged = alipay_get_binding_status();
	SYS_LOG_INF("alipay_get_binding_status, is_binged: %d", is_binged);
	if (is_binged) {
		status = STATE_BINDING_OK;
	} else {
		status = STATE_UNBINDED;
		retval_e ret = alipay_query_binding_result(&query_status);
		SYS_LOG_INF("alipay_query_binding_result, ret: %d, query_status:%d", ret, query_status);
		switch (query_status) {
		case ALIPAY_STATUS_UNKNOWN:
			status = STATE_UNBINDED;
			break;

		case ALIPAY_STATUS_START_BINDING:
			status = STATE_BINDING;
			break;

		case ALIPAY_STATUS_BINDING_FAIL:
			status = STATE_BINDING_FAIL;
			break;
		}
	}

	SYS_LOG_INF("ui binding status: %d", status);

	return status;
}

static int _alipay_get_binding_string(uint8_t *buf, uint32_t *out_len)
{
	int ret = alipay_get_binding_code(buf, out_len);
	buf[*out_len] = '\0';
	SYS_LOG_INF("buf: %s, *out_len: %d",buf, *out_len);
	return ret;
}

static int _alipay_get_paycode_string(uint8_t *buf, uint32_t *out_len)
{
	int ret = alipay_get_paycode(buf, out_len);

	buf[*out_len] = '\0';
	return ret;
}

static int _alipay_get_userinfo(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len)
{
	int ret = 0;
	
	ret |= alipay_get_nick_name(name, name_len);
	ret |= alipay_get_logon_ID(id, id_len);
	name[*name_len] = '\0';
	id[*id_len] = '\0';

	return ret;
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

static void _alipay_init_cb(const uint8_t *data, uint32_t len)
{
	int ret;
	//just for actions test!!
	char buf_sn[20];
	ret = nvram_config_get("SN", (void*)buf_sn, sizeof(buf_sn));
    if (ret < 0) {
		char *test_sn = "TEST123456";
        nvram_config_set("SN", test_sn, strlen(test_sn));
    }

	ret = alipay_pre_init();
	SYS_LOG_INF("alipay_pre_init, ret:%d", ret);
#if CONFIG_TRANSPORT_LIB
	alipay_service_start();
	alipay_load_cardlist(0, load_cardlist_callback);
#endif
}

static void _alipay_init(void)
{
	uint8_t pay_type = 0;

	support_features |= ALIPAY_FEATURE_PAY;
#if CONFIG_TRANSPORT_LIB
	support_features |= ALIPAY_FEATURE_TRANSPORT;
#endif
	// init ble
	vendor_ble_alipay_init();

	// run pay_lib_init in ui_srv to avoid stack overflow
	paylib_ble_recv_cb(&pay_type, sizeof(pay_type), _alipay_init_cb);
}

static void _alipay_sync_time(void)
{
}

static uint32_t _alipay_get_support_function(void) {
	return support_features;
}

const alipay_view_presenter_t alipay_view_presenter = {
	.init = _alipay_init,
	.get_support_features = _alipay_get_support_function,
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
#if CONFIG_TRANSPORT_LIB
	.get_cardlist_status = _alipay_get_cardlist_status,
	.load_cardlist = _alipay_load_cardlist,
	.get_card_num = _alipay_get_card_num,
	.get_card_title = _alipay_get_card_title,
	.get_card_status = _alipay_get_card_status,
	.load_card = _alipay_load_card,
	.load_card_retry = _alipay_load_card_retry,
	.get_transitcode = _alipay_get_transitcode,
	.get_transit_guide_string = _alipay_get_transit_guide_string,
	.get_transit_error_string = _alipay_get_transit_error_string,
#endif
};

