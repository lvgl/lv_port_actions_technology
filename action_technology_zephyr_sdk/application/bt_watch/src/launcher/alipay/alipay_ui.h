/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_WATCH_SRC_LAUNCHER_ALIPAY_UI_H_
#define BT_WATCH_SRC_LAUNCHER_ALIPAY_UI_H_

#include <msg_manager.h>

enum alipay_bind_state {
	STATE_UNBINDED = 0,
	STATE_WAIT_SCAN,
	STATE_BINDING,
	STATE_BINDING_FAIL,
	STATE_BINDING_OK,
};

enum alipay_transport_view_state {
	STATE_LOAD_IDLE = 0,
	STATE_LOADING,
	STATE_LOAD_OK,
	STATE_LOAD_FAIL,
};

enum alipay_feature {
	ALIPAY_FEATURE_PAY = 0x0001,
	ALIPAY_FEATURE_TRANSPORT = 0x0002,
};

typedef struct alipay_view_presenter_s {
	void (*init)(void);
	uint32_t (*get_support_features)(void);
	void (*start_adv)(void);
	void (*stop_adv)(void);

	void (*bind_init)(void);
	void (*bind_deinit)(void);
	void (*do_unbind)(void);
	void (*sync_time)(void);

	int (*get_binding_status)(void);
	int (*get_binding_string)(uint8_t *buf, uint32_t *out_len);
	int (*get_paycode_string)(uint8_t *buf, uint32_t *out_len);
	int (*get_userinfo)(uint8_t *name, uint32_t *name_len, uint8_t *id, uint32_t *id_len);

	bool (*is_barcode_on)(void);
	void (*toggle_barcode)(void);

	void (*lock_screen)(void);
	void (*unlock_screen)(void);
	int (*get_cardlist_status)(void);
	int (*load_cardlist)(uint8_t online);
	void (*load_card_retry)(void);
	int (*get_card_num)(void);
	int (*get_card_title)(uint8_t index, uint8_t *buf, int len);
	int (*get_card_status)(void);
	int (*load_card)(int index);
	int (*set_card_index)(uint8_t index);
	int (*get_transitcode)(char* title, uint8_t* transitcode, uint32_t* len_transitcode);
	void (*get_transit_guide_string)(uint8_t *buf, uint32_t *out_len);
	char* (*get_transit_error_string)(void);
} alipay_view_presenter_t;

extern const alipay_view_presenter_t alipay_view_presenter;
extern const alipay_view_presenter_t wxpay_view_presenter;

void alipay_ui_sync_time(void);

void alipay_ui_init(void);
void alipay_ui_enter(void);
void alipay_ui_update(void);

void wxpay_ui_init(void);
void wxpay_ui_enter(void);
void wxpay_ui_update(void);

#endif /* BT_WATCH_SRC_LAUNCHER_ALIPAY_UI_H_ */

