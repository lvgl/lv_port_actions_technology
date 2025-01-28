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

#define ALIPAY_THREAD_STACKSIZE 1024
#ifdef CONFIG_SIMULATOR
char alipay_thread_stack[ALIPAY_THREAD_STACKSIZE];
#else
static OS_THREAD_STACK_DEFINE(alipay_thread_stack, ALIPAY_THREAD_STACKSIZE);
#endif

typedef struct alipay_tansit_CardBaseVO
{
    char cardNo[40];
    char cardType[40];
    char title[40];
	bool is_loaded;
	char transitcode[512];
}alipay_tansit_CardBaseVO_t;
alipay_tansit_CardBaseVO_t card_list[10];
static int8_t cur_card_num = -1;
static int8_t cur_card_idx = -1;
static int cardlist_status = STATE_LOAD_IDLE;
static int card_status = STATE_LOAD_IDLE;
alipay_tansit_CardBaseVO_t *last_card;
static bool is_cardlist_error = true;
static bool is_cardlist_exit = false;
static bool is_card_exit = false;
static bool is_card_error = true;

static bool is_bind_inited = false;
static bool is_screen_locked = false;
static bool is_barcode_mode = false;
static int status_cnt = 0;
static int refresh_cnt = 0;
static uint32_t support_features;

static void alipay_thread(void *p1, void *p2, void *p3)
{
	while(1) {
		SYS_LOG_DBG("check cardlist status: %d", cardlist_status);
		if(cardlist_status == STATE_LOADING) {
			int i,j;
			os_sleep(3000);
			cur_card_num = 6;
			uint8_t card_title_str[] = "A城乘车码";
			uint8_t card_transitcode_str[] = "13546765878660538318166775968769345465676245234356467657234523425";
			for(i=0; i<cur_card_num; i++) {
				card_list[i].is_loaded = false;
				memset(card_list[i].title, 0, sizeof(card_list[i].title));
				strcpy(card_list[i].title, card_title_str);
				memcpy(card_list[i].transitcode, card_transitcode_str, sizeof(card_transitcode_str));
				card_title_str[0] +=1;
				for(j=0; j<sizeof(card_transitcode_str); j++) {
					card_transitcode_str[j] += i;
				}
			}
			
			if(is_cardlist_error) {
				SYS_LOG_INF("_alipay_get_card_list_fail");
				cardlist_status = STATE_LOAD_FAIL;
				is_cardlist_error = false;
			} else {
				SYS_LOG_INF("_alipay_get_card_list_ok, card_num: %d", cur_card_num);
				cardlist_status = STATE_LOAD_OK;
				if(is_cardlist_exit == false)
					is_cardlist_exit = true;
			}
		}

		SYS_LOG_DBG("check card status: %d", card_status);
		if(card_status == STATE_LOADING) {
			if(cur_card_idx < 0) {
				SYS_LOG_INF("alipay get last card OK");
			} else {
				if(!card_list[cur_card_idx].is_loaded) {
					os_sleep(3000);
					card_list[cur_card_idx].is_loaded = true;
				}
				SYS_LOG_INF("alipay get card [%d] OK", cur_card_idx);
				if(cur_card_idx >= 0)
					last_card = &card_list[cur_card_idx];
			}
			
			if(is_card_error) {
				card_status = STATE_LOAD_FAIL;
				is_card_error = false;
			} else {
				card_status = STATE_LOAD_OK;
				if (is_card_exit == false) {
					is_card_exit = true;
				}
			}
		}

		os_sleep(100);
	}
}


static void _alipay_init(void)
{
	static int tid;
	if(!tid) {
		tid = (int)os_thread_create((char *)alipay_thread_stack, ALIPAY_THREAD_STACKSIZE, alipay_thread,
										NULL, NULL, NULL, 3, 0, OS_NO_WAIT);
	}
	support_features |= ALIPAY_FEATURE_PAY;
	support_features |= ALIPAY_FEATURE_TRANSPORT;
}

static void _alipay_start_adv(void)
{
}

static void _alipay_stop_adv(void)
{
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
	status_cnt = 0;
	refresh_cnt = 0;
	cardlist_status = STATE_LOAD_IDLE;
	card_status = STATE_LOAD_IDLE;
	is_cardlist_error = true;
	is_cardlist_exit = false;
	is_card_exit = false;
}

static void _alipay_sync_time(void)
{
}

static int _alipay_get_binding_status(void)
{
	int status = STATE_BINDING_OK;
	// fake status for simulator
	status_cnt ++;
	if (status_cnt < 3) {
		status = STATE_WAIT_SCAN;
	} else if (status_cnt < 6) {
		status = STATE_BINDING;
	} else {
		status = STATE_BINDING_OK;
	}
	SYS_LOG_INF("ui binding status: %d", status);

	return status;
}

static int _alipay_get_binding_string(uint8_t *buf, uint32_t *out_len)
{
	strcpy(buf, "https://lvgl.io");
	*out_len = strlen(buf);

	return 0;
}

static int _alipay_get_paycode_string(uint8_t *buf, uint32_t *out_len)
{
	strcpy(buf, "286605383181667759");
	*out_len = strlen(buf);

	return 0;
}

static int _alipay_get_userinfo(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len)
{
	strcpy(name, "alipay@163.com");
	*name_len = strlen(name);
	strcpy(id, "alipay");
	*id_len = strlen(id);

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

static int _alipay_get_cardlist_status(void)
{
	return cardlist_status;
}

static int _alipay_load_cardlist(uint8_t online)
{
	SYS_LOG_INF("alipay load cardlist %d", online);
	if(!online) {
		if(is_cardlist_exit == false) {
			cardlist_status = STATE_LOAD_IDLE;
			return -1;
		}
	}
	
	if(cardlist_status != STATE_LOADING) {
		cardlist_status = STATE_LOADING;
	}

	return 0;
}

static int _alipay_get_card_num(void)
{
	refresh_cnt++;
	if(refresh_cnt < 10) {
		return 0;
	} else {
		SYS_LOG_INF("alipay get card num, current card num: %d", cur_card_num);
		return cur_card_num;
	}
}

static int _alipay_get_card_title(uint8_t index, uint8_t *buf, int len)
{
	if(len < sizeof(card_list[index].title)) {
		return -1;
	}
	strcpy(buf, card_list[index].title);
	return 0;
}

static int _alipay_get_card_status(void)
{
	return card_status;
}

static int _alipay_load_card(int index)
{
	SYS_LOG_INF("alipay load card %d", index);
	if (index < 0) {
		if(is_card_exit == false) {
			return -1;
		}
	}
	
	if(card_status != STATE_LOADING) {
		cur_card_idx = index;
		card_status = STATE_LOADING;
	}

	return 0;
}

static void _alipay_load_card_retry(void)
{
	SYS_LOG_INF("alipay load card retry");
	if(card_status != STATE_LOADING) {
		card_status = STATE_LOADING;
	}
}

static int _alipay_get_transitcode(char* title, uint8_t* transitcode, uint32_t* len_transitcode)
{
	if(last_card == NULL) {
		return -1;
	}

	strcpy(title, last_card->title);
	*len_transitcode = strlen(last_card->transitcode);
	strcpy(transitcode, last_card->transitcode);
	return 0;
}

static uint32_t _alipay_get_support_function(void) {
	return support_features;
}

static void _alipay_get_transit_guide_string(uint8_t *buf, uint32_t *out_len) {
	strcpy(buf, "https://lvgl.io");
	*out_len = strlen(buf);
}

static char *_alipay_get_transit_error_string(void) {
	return "错误，请返回重试";
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
};

