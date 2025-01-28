/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call view
 */

#define LOG_MODULE_CUSTOMER

#include "btcall.h"
#include <assert.h>
#include <input_manager.h>
#include <app_ui.h>
#ifdef CONFIG_UI_MANAGER
#include <msgbox_cache.h>
#include <widgets/anim_img.h>
#endif
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

LOG_MODULE_DECLARE(btcall, LOG_LEVEL_INF);

/**
 * A：表示当前通话
 * B：表示新的来电
 * Rules:
 * 1、A在通话在手表上（界面显示挂断按键），B来电，显示来电号码（界面显示挂断按键，不显示其它按键），
 *   点击界面的挂断按键功能为拒接B来电；手机上三方通话的任何操作，通话声音还是保持从手表出；
 * 2、A通话不在手表上，B来电，机子上不提示来电，不显示来电号码，手机上进行三方通话操作，手表上不做任何显示；
 */
#define CONFIG_GT2_WATCH_3WAY_POLICY 1

/*btcall view state*/
enum {
	BTCALL_NULL_STATE = 0,
	BTCALL_INCOMING_STATE,
	BTCALL_ONGOING_STATE,
	BTCALL_OUTGOING_STATE,
	BTCALL_SIRI_STATE,
	BTCALL_3W_STATE,

	BTCALL_NUM_STATES,
};

/*incoming state*/
enum {
	BTN_REJECT = 0,
	BTN_ACCEPT,

	NUM_INCOMING_BTNS,
};

enum {
	BMP_INCOMING_HEAD = 0,

	NUM_INCOMING_IMGS,
};

enum {
	TXT_CALL_NUM = 0,
	TXT_REJECT,
	TXT_ACCEPT,
	NUM_INCOMING_TXTS,
};

/*ongoing*/
enum {
	BTN_ONGOING_VOL_DOWN = 0,
	BTN_ONGOING_VOL_UP,
	BTN_ONGOING_MUTE,
	BTN_ONGOING_KEYB,
	BTN_ONGOING_HAND_UP,

	NUM_ONGOING_BTNS,
};

enum {
	BMP_ONGOING_HEAD = 0,
	BMP_ONGOING_VOL_BG,
	BMP_ONGOING_VOL_SYMBOL,

	NUM_ONGOING_IMGS,
};

enum {
	TXT_ONGOING_NUM = 0,
	TXT_ONGOING_TIME,
	NUM_ONGOING_TXTS,
};

/*outgoing*/
enum {
	BTN_OUTGOING_HAND_UP = 0,

	NUM_OUTGOING_BTNS,
};

enum {
	BMP_OUTGOING_SYMBOL = 0,

	NUM_OUTGOING_IMGS,
};

enum {
	TXT_OUTGOING_NUM = 0,
	NUM_OUTGOING_TXTS,
};

/*3way in view*/
enum {
	BTN_3W_HUP_ANOTH = 0,

#if CONFIG_GT2_WATCH_3WAY_POLICY == 0
	BTN_3W_HUP_CUR,
	BTN_3W_HUP_ANS,
	BTN_3W_HOLD_CUR,
	BTN_3W_ANS_ALL,
#endif

	NUM_3W_BTNS,
};

enum {
	TXT_3W_HUP_ANS = 0,
	TXT_3W_HOLD_ANS,
	TXT_3W_HOLD_NUM,
	TXT_3W_ANOTH_NUM,

	NUM_3W_TXTS,
};

const static uint32_t incoming_bmp_ids[] = {
	PIC_COMING_HEAD,
};

const static uint32_t incoming_btn_ids[] = {
	PIC_BTN_REJECT,
	PIC_BTN_ACCEPT,
};

const static uint32_t incoming_txt_ids[] = {
	STR_CALL_NUM,
	STR_REJECT,
	STR_ACCEPT,
};

const static uint32_t ongoing_bmp_ids[] = {
	PIC_CALLING_VOL_BG,
	PIC_CALLING_VOL_SYMBOL,
	PIC_CALLING_HEAD,
};

const static uint32_t ongoing_btn_ids[] = {
	PIC_BTN_CALLING_VOL_DOWN,
	PIC_BTN_CALLING_VOL_UP,
	PIC_BTN_CALLING_MUTE,
	PIC_BTN_CALLING_KEYB,
	PIC_BTN_CALLING_HANG_UP,
};

const static uint32_t ongoing_txt_ids[] = {
	STR_CALLING_NUM,
	STR_CALLING_TIME,
};

const static uint32_t outgoing_bmp_ids[] = {
	PIC_OUTGOING_SYMBOL,
};
const static uint32_t outgoing_btn_ids[] = {
	PIC_BTN_OUTGOING_HANG_UP,
};
const static uint32_t outgoing_txt_ids[] = {
	STR_OUTGOING_NUM,
};

const static uint32_t thr_way_btn_ids[] = {
	PIC_BTN_3W_HUP,

#if CONFIG_GT2_WATCH_3WAY_POLICY == 0
	PIC_BTN_3W_HUP_CUR,
	PIC_BTN_3W_ANS_ANOTH,
	PIC_BTN_3W_HOLD_CUR,
	PIC_BTN_3W_ANS_ALL,
#endif
};

const static uint32_t thr_way_txt_ids[] = {
	STR_3W_HUP_ANS,
	STR_3W_HOLD_ANS,
	STR_3W_HOLD_NUM,
	STR_3W_ANOTH_NUM,
};

typedef struct incoming_view_data {
	lv_obj_t *incoming_btn[NUM_INCOMING_BTNS];
	lv_obj_t *incoming_bmp[NUM_INCOMING_IMGS];
	lv_obj_t *incoming_lbl[NUM_INCOMING_TXTS];

	lvgl_res_string_t res_incoming_txt[NUM_INCOMING_TXTS];
	lv_image_dsc_t incoming_img_dsc_btn[NUM_INCOMING_BTNS];
	lv_image_dsc_t incoming_img_dsc_bmp[NUM_INCOMING_IMGS];

	lv_point_t incoming_pt_btn[NUM_INCOMING_BTNS];
	lv_point_t incoming_pt_bmp[NUM_INCOMING_IMGS];
	lv_point_t incoming_pt_txt[NUM_INCOMING_TXTS];

	lv_style_t incoming_style[NUM_INCOMING_TXTS];
} incoming_view_data_t;

typedef struct ongoing_view_data {
	lv_obj_t *ongoing_btn[NUM_ONGOING_BTNS];
	lv_obj_t *ongoing_bmp[NUM_ONGOING_IMGS];
	lv_obj_t *ongoing_lbl[NUM_ONGOING_TXTS];

	lvgl_res_string_t res_ongoing_txt[NUM_ONGOING_TXTS];

	lv_image_dsc_t ongoing_img_dsc_btn[NUM_ONGOING_BTNS];
	lv_image_dsc_t ongoing_img_dsc_bmp[NUM_ONGOING_IMGS];

	lv_point_t ongoing_pt_btn[NUM_ONGOING_BTNS];
	lv_point_t ongoing_pt_bmp[NUM_ONGOING_IMGS];
	lv_point_t ongoing_pt_txt[NUM_ONGOING_TXTS];

	lv_style_t ongoing_style[NUM_ONGOING_TXTS];
	lv_timer_t *timer_ls;
} ongoing_view_data_t;

typedef struct outgoing_view_data {
	lv_obj_t *outgoing_btn[NUM_OUTGOING_BTNS];
	lv_obj_t *outgoing_bmp[NUM_OUTGOING_IMGS];
	lv_obj_t *outgoing_lbl[NUM_OUTGOING_TXTS];
	lvgl_res_string_t res_outgoing_txt[NUM_OUTGOING_TXTS];
	lv_image_dsc_t outgoing_img_dsc_btn[NUM_OUTGOING_BTNS];
	lv_image_dsc_t outgoing_img_dsc_bmp[NUM_OUTGOING_IMGS];
	lv_point_t outgoing_pt_btn[NUM_OUTGOING_BTNS];
	lv_point_t outgoing_pt_bmp[NUM_OUTGOING_IMGS];
	lv_point_t outgoing_pt_txt[NUM_OUTGOING_TXTS];
	lv_style_t outgoing_style[NUM_OUTGOING_TXTS];
} outgoing_view_data_t;

typedef struct siri_view_data {
	lv_obj_t *siri_txt;
	lv_obj_t *siri_anim;
	lvgl_res_picregion_t siri_prg_anim;
} siri_view_data_t;

typedef struct thr_way_view_data {
	lv_obj_t *thr_way_btn[NUM_3W_BTNS];
	lv_obj_t *thr_way_lbl[NUM_3W_TXTS];
	lvgl_res_string_t res_3w_txt[NUM_3W_TXTS];
	lv_image_dsc_t thr_way_img_dsc_btn[NUM_3W_BTNS];
	lv_point_t thr_way_pt_btn[NUM_3W_BTNS];
	lv_point_t thr_way_pt_txt[NUM_3W_TXTS];
	lv_style_t thr_way_style[NUM_3W_TXTS];
} thr_way_view_data_t;

typedef struct btcall_view_data {
	/* lvgl object */
	lv_obj_t *btcall_bg;

	union {
		incoming_view_data_t incoming_view;
		ongoing_view_data_t ongoing_view;
		outgoing_view_data_t outgoing_view;
		siri_view_data_t siri_view;
		thr_way_view_data_t thr_way_view;
	};

	/* ui-editor resource */
	lvgl_res_scene_t res_scene;
	lv_font_t font;
	lv_font_t font_28;
	uint8_t view_state;/*1--in coming, 2--ongoing, 3--outgoing, 4--siri_mode,5--3w*/
	uint8_t calling_state;/*1--in coming, 2--ongoing, 3--outgoing, 4--siri_mode,5--3w*/
	uint8_t calling_pre_state;/*1--in coming, 2--ongoing, 3--outgoing, 4--siri_mode,5--3w*/
	uint8_t need_sync_calling_numb : 1;/*0--no, 1--need*/
	uint64_t ongoing_start_time;/*unit :ms*/

	const char *cur_phone_number;
	char *phone_number;
	char *thr_way_phone_number;
} btcall_view_data_t;

typedef struct btcall_view_state_transition {
	int (*enter)(btcall_view_data_t *data);
	void (*exit)(btcall_view_data_t *data);
} btcall_view_state_transition_t;

static int _display_incoming_view(btcall_view_data_t *data);
static void _exit_incoming_view(btcall_view_data_t *data);
static void _display_incoming_label(btcall_view_data_t *data,
		const char *coming_str, uint8_t txt_num);
static void _handle_incoming_btn_event(lv_event_t * e);

static int _display_ongoing_view(btcall_view_data_t *data);
static void _exit_ongoing_view(btcall_view_data_t *data);
static void _display_ongoing_label(btcall_view_data_t *data,
		const char *calling_str, uint8_t txt_num);
static void _display_ongoing_time_cb(lv_timer_t *timer);
static void _handle_ongoing_btn_event(lv_event_t * e);

static int _display_outgoing_view(btcall_view_data_t *data);
static void _exit_outgoing_view(btcall_view_data_t *data);
static void _display_outgoing_label(btcall_view_data_t *data,
		const char *outgoing_str, uint8_t txt_num);
static void _handle_outgoing_btn_event(lv_event_t * e);

static int _display_3way_view(btcall_view_data_t *data);
static void _exit_3way_view(btcall_view_data_t *data);
static void _display_3way_label(btcall_view_data_t *data,
		const char *thr_way_str, uint8_t txt_num);
static void _handle_3way_btn_event(lv_event_t * e);

static int _display_siri_view(btcall_view_data_t *data);
static void _exit_siri_view(btcall_view_data_t *data);

static int _btcall_view_apply_state_transition(btcall_view_data_t *data, uint8_t new_state);

static void _btcall_view_layout(void);
static void _btcall_view_delete(btcall_view_data_t *data);

static btcall_view_data_t *p_btcall_view_data;

static const btcall_view_state_transition_t btcall_view_transition_table[] = {
	[BTCALL_NULL_STATE] = { NULL, NULL },
	[BTCALL_INCOMING_STATE] = { _display_incoming_view, _exit_incoming_view },
	[BTCALL_ONGOING_STATE] = { _display_ongoing_view, _exit_ongoing_view },
	[BTCALL_OUTGOING_STATE] = { _display_outgoing_view, _exit_outgoing_view },
	[BTCALL_SIRI_STATE] = { _display_siri_view, _exit_siri_view },
	[BTCALL_3W_STATE] = { _display_3way_view, _exit_3way_view },
};

void btcall_view_resume(void)
{
#if 0
	uint8_t call_state = 0;
	int ret = bt_manager_hfp_get_call_state(1, &call_state);

	SYS_LOG_INF("ret: %d call_state:%d", ret, call_state);

	if (!btcall_sco_is_established())
		return;

	if (ret != BTSRV_HFP_STATE_INIT){
		if (ret == BTSRV_HFP_STATE_CALL_INCOMING) {
			btcall_set_incoming_view();
		} else if (ret == BTSRV_HFP_STATE_CALL_OUTCOMING) {
			btcall_set_outgoing_view();
		} else if (ret == BTSRV_HFP_STATE_CALL_ONGOING || ret == BTSRV_HFP_STATE_CALL_MULTIPARTY) {
			btcall_sync_state_to_view(false);
		} else if (ret == BTSRV_HFP_STATE_CALL_3WAYIN) {
			if (p_btcall_view_data && p_btcall_view_data->calling_state != BTCALL_3W_STATE) {
				p_btcall_view_data->calling_state = BTCALL_3W_STATE;
				_btcall_view_layout();
			}
		}
	}
#else
	SYS_LOG_INF("btcall view resume");

	if (!p_btcall_view_data)
		return;

	if (p_btcall_view_data->calling_state != BTCALL_NULL_STATE) {
		p_btcall_view_data->calling_pre_state = p_btcall_view_data->view_state;
		_btcall_view_layout();
	}
#endif
}

void btcall_switch_voice_source(bool switch_to_phone)
{
	if (!p_btcall_view_data)
		return;

	SYS_LOG_INF("switch_to_phone %d, view_state %d, call_state %d\n",
			switch_to_phone, p_btcall_view_data->view_state, p_btcall_view_data->calling_state);

	if (switch_to_phone) {
		switch (p_btcall_view_data->calling_state) {
		case BTCALL_INCOMING_STATE:
		case BTCALL_ONGOING_STATE:
		case BTCALL_OUTGOING_STATE:
		case BTCALL_3W_STATE:
			p_btcall_view_data->calling_state = BTCALL_NULL_STATE;
			_btcall_view_layout();
			break;
		default:
			break;
		}
	} else {
		if (p_btcall_view_data->calling_state == BTCALL_ONGOING_STATE)
			return;

	#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_lock(FULL_WAKE_LOCK);
	#endif

	#ifdef CONFIG_BLUETOOTH
	#ifdef CONFIG_BT_HFP_HF
		uint8_t call_state = 0;
		int ret = bt_manager_hfp_get_call_state(1, &call_state);

		SYS_LOG_INF("ret: %d call_state:%d", ret, call_state);
		if (ret != BTSRV_HFP_STATE_INIT){
			if (ret == BTSRV_HFP_STATE_CALL_ONGOING || ret == BTSRV_HFP_STATE_CALL_MULTIPARTY) {
				btcall_sync_state_to_view(false);
			}
		}
	#endif
	#endif

	#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(FULL_WAKE_LOCK);
	#endif
	}
}

void btcall_sync_state_to_view(bool siri_mode)
{
	if (!p_btcall_view_data)
		return;

	SYS_LOG_INF("siri %d, view_state %d, call_state %d\n", siri_mode,
		 p_btcall_view_data->view_state, p_btcall_view_data->calling_state);

	if (!siri_mode) {/*in btcall mode*/
		if (p_btcall_view_data->calling_state == BTCALL_ONGOING_STATE)
			return;

		p_btcall_view_data->calling_state = BTCALL_ONGOING_STATE;/*system reset need to display ongoing view*/
		_btcall_view_layout();
	} else {/*in siri mode*/
		if (p_btcall_view_data->calling_state == BTCALL_NULL_STATE) {
			p_btcall_view_data->calling_state = BTCALL_SIRI_STATE;/*system reset need to display ongoing view*/
			_btcall_view_layout();
		}
	}
}

void btcall_set_ongoing_start_time(void)
{
	if (p_btcall_view_data && p_btcall_view_data->ongoing_start_time == 0) {
		p_btcall_view_data->ongoing_start_time = os_uptime_get();
	}
}

void btcall_set_phone_num(const char *phone_num, bool in_3way_view)
{
	btcall_view_data_t *data = p_btcall_view_data;

	if (!data)
		return;

	/* view may access the phone_number */
	data->cur_phone_number = NULL;

	if (data->phone_number) {
		app_mem_free(data->phone_number);
		data->phone_number = NULL;
	}

	if (phone_num) {
		SYS_LOG_INF("phone_number %s (in_3way %d)\n", phone_num, in_3way_view);

		data->phone_number = app_mem_malloc(strlen(phone_num) + 1);
		if (data->phone_number) {
			strcpy(data->phone_number, phone_num);
		} else {
			SYS_LOG_ERR("alloc failed");
		}
	}

	data->cur_phone_number = data->phone_number;

	if (in_3way_view == false) {
		switch (data->calling_state) {
		case BTCALL_INCOMING_STATE:
		case BTCALL_OUTGOING_STATE:
		case BTCALL_ONGOING_STATE:
			data->need_sync_calling_numb = 1;
			_btcall_view_layout();
			break;
		default:
			break;
		}
	}
}

void btcall_set_3way_phone_num(const char *phone_num)
{
	btcall_view_data_t *data = p_btcall_view_data;
	char *tmp_phone_num;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

	if (!data)
		return;

	if (data->phone_number && !strcmp(data->phone_number, phone_num)) {
		SYS_LOG_INF("ignore 3way which has the same phone number %s\n", phone_num);
		return;
	}

	/* view may access the phone_number */
	tmp_phone_num = data->thr_way_phone_number;
	data->thr_way_phone_number = NULL;

	if (tmp_phone_num) {
		app_mem_free(tmp_phone_num);
		tmp_phone_num = NULL;
	}

	if (phone_num) {
		SYS_LOG_INF("3way phone_number %s\n", phone_num);

		tmp_phone_num = app_mem_malloc(strlen(phone_num) + 1);
		if (tmp_phone_num) {
			strcpy(tmp_phone_num, phone_num);
		} else {
			SYS_LOG_ERR("alloc failed");
		}
	}

	data->thr_way_phone_number = tmp_phone_num;

	if (data->calling_state != BTCALL_3W_STATE) {
		data->calling_state = BTCALL_3W_STATE;
		_btcall_view_layout();
	}
}

void btcall_set_incoming_view(void)
{
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

	if (!p_btcall_view_data)
		return;

	SYS_LOG_INF("calling_state %d\n", p_btcall_view_data->calling_state);

	if (p_btcall_view_data->calling_state == BTCALL_NULL_STATE ||
		p_btcall_view_data->calling_state == BTCALL_SIRI_STATE) {
		p_btcall_view_data->calling_state = BTCALL_INCOMING_STATE;
		_btcall_view_layout();
	}
}

void btcall_set_outgoing_view(void)
{
	if (!p_btcall_view_data)
		return;

	SYS_LOG_INF("calling_state %d\n", p_btcall_view_data->calling_state);

	if (p_btcall_view_data->calling_state == BTCALL_NULL_STATE ||
		p_btcall_view_data->calling_state == BTCALL_SIRI_STATE) {
		p_btcall_view_data->calling_state = BTCALL_OUTGOING_STATE;
		_btcall_view_layout();
	}
}

void btcall_view_init(void)
{
	if (p_btcall_view_data) {
		SYS_LOG_WRN("view data exist\n");
		return;
	}

	p_btcall_view_data = app_mem_malloc(sizeof(*p_btcall_view_data));
	if (!p_btcall_view_data) {
		return;
	}

	memset(p_btcall_view_data, 0, sizeof(*p_btcall_view_data));

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif

	SYS_LOG_INF("ok\n");
}

void btcall_view_deinit(void)
{
	if (p_btcall_view_data == NULL)
		return;

	btcall_exit_view();

	if (p_btcall_view_data->phone_number) {
		app_mem_free(p_btcall_view_data->phone_number);
	}

	if (p_btcall_view_data->thr_way_phone_number) {
		app_mem_free(p_btcall_view_data->thr_way_phone_number);
	}

	app_mem_free(p_btcall_view_data);
	p_btcall_view_data = NULL;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(PARTIAL_WAKE_LOCK);
#endif

	SYS_LOG_INF("ok\n");
}

void btcall_exit_view(void)
{
	if (p_btcall_view_data) {
		p_btcall_view_data->calling_state = BTCALL_NULL_STATE;
		_btcall_view_layout();
	}
}

static void _delete_obj_array(lv_obj_t **pobj, uint32_t num)
{
	for (int i = num - 1; i >= 0; i--) {
		if (pobj[i]) {
			lv_obj_delete(pobj[i]);
			pobj[i] = NULL;
		}
	}
}

static void _cvt_txt_array(lv_point_t *pt, lv_style_t *sty, lv_font_t *font, lvgl_res_string_t* txt, uint32_t num)
{
	for (int i = num - 1; i >= 0; i--) {
		pt[i].x = txt[i].x;
		pt[i].y = txt[i].y;

		lv_style_init(&sty[i]);
		lv_style_set_text_font(&sty[i], font);
		lv_style_set_text_color(&sty[i], txt[i].color);
		lv_style_set_text_align(&sty[i], LV_TEXT_ALIGN_CENTER);
	}
}

static void _create_btn_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
				lv_image_dsc_t *def, lv_image_dsc_t *sel, uint32_t num,
				void (*handler)(lv_event_t *), void *event_data)
{
	for (int i = num - 1; i >= 0; i--) {
		pobj[i] = lv_imagebutton_create(par);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
		lv_obj_set_size(pobj[i], def[i].header.w, def[i].header.h);
		lv_obj_add_flag(pobj[i], LV_OBJ_FLAG_CHECKABLE);
		lv_obj_set_user_data(pobj[i], (void*)i);
		lv_obj_add_event_cb(pobj[i], handler, LV_EVENT_CLICKED, event_data);

		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_RELEASED, NULL, &def[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_PRESSED, NULL, &sel[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, &sel[i], NULL);
		lv_imagebutton_set_src(pobj[i], LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, &def[i], NULL);
	}
}

static void _create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = num - 1; i >= 0; i--) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}

static void _create_btcall_label_array(lv_obj_t *par, lv_obj_t **pobj, lvgl_res_string_t *txt,
										lv_style_t *sty, uint32_t num)
{
	int i;

	for (i = 0; i < num; i++) {
		pobj[i] = lv_label_create(par);
		lv_label_set_long_mode(pobj[i], LV_LABEL_LONG_WRAP);
		lv_obj_set_pos(pobj[i], txt[i].x, txt[i].y);
		lv_obj_set_size(pobj[i], txt[i].width, txt[i].height);
		lv_obj_add_style(pobj[i], &sty[i], LV_PART_MAIN);
		lv_label_set_text(pobj[i], "");
	}
}

static void _unload_resource(btcall_view_data_t *data)
{
	LVGL_FONT_CLOSE(&data->font);
#if DEF_UI_WIDTH >= 454
	LVGL_FONT_CLOSE(&data->font_28);
#endif

	lvgl_res_unload_scene(&data->res_scene);
}

static int _load_resource(btcall_view_data_t *data)
{
	int ret;

	/* load scene */
	ret = lvgl_res_load_scene(SCENE_BTCALL_VIEW, &data->res_scene, DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_HEART_VIEW not found");
		return -ENOENT;
	}

	/* open font */
	if (LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL) < 0) {
		SYS_LOG_ERR("font not found");
		goto error_exit;
	}

#if DEF_UI_WIDTH >= 454
	if (LVGL_FONT_OPEN_DEFAULT(&data->font_28, DEF_FONT_SIZE_SMALL) < 0) {
		SYS_LOG_ERR("font28 not found");
		goto error_exit;
	}
#endif

	SYS_LOG_INF("load resource succeed");
	return 0;
error_exit:
	_unload_resource(data);
	return -ENOENT;
}

static int _display_ongoing_view(btcall_view_data_t *data)
{
	ongoing_view_data_t *ongo_view = &data->ongoing_view;

	if (data->view_state != BTCALL_NULL_STATE) {
		SYS_LOG_ERR("in state %d\n", data->view_state);
		return -EINVAL;
	}

	memset(ongo_view, 0, sizeof(ongoing_view_data_t));

	/* display calling view*/
	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, ongoing_bmp_ids, ongo_view->ongoing_img_dsc_bmp,
			ongo_view->ongoing_pt_bmp, NUM_ONGOING_IMGS);
	lvgl_res_load_pictures_from_scene(&data->res_scene, ongoing_btn_ids, ongo_view->ongoing_img_dsc_btn,
			ongo_view->ongoing_pt_btn, NUM_ONGOING_BTNS);
	lvgl_res_load_strings_from_scene(&data->res_scene, ongoing_txt_ids, ongo_view->res_ongoing_txt, NUM_ONGOING_TXTS);

	/* convert resource */
	_cvt_txt_array(ongo_view->ongoing_pt_txt, ongo_view->ongoing_style, &data->font, ongo_view->res_ongoing_txt, NUM_ONGOING_TXTS);

	/* set bg color */
	lv_obj_set_style_bg_color(data->btcall_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);

	_create_img_array(data->btcall_bg, ongo_view->ongoing_bmp, ongo_view->ongoing_pt_bmp,
			ongo_view->ongoing_img_dsc_bmp, NUM_ONGOING_IMGS);

	/* create button */
	_create_btn_array(data->btcall_bg, ongo_view->ongoing_btn, ongo_view->ongoing_pt_btn,
			ongo_view->ongoing_img_dsc_btn, ongo_view->ongoing_img_dsc_btn, NUM_ONGOING_BTNS,
			_handle_ongoing_btn_event, data);
	/*set button ext click area to improve the recognition rate*/
	lv_obj_set_ext_click_area(ongo_view->ongoing_btn[BTN_ONGOING_VOL_DOWN],
			lv_obj_get_style_width(ongo_view->ongoing_btn[BTN_ONGOING_VOL_DOWN], LV_PART_MAIN));
	lv_obj_set_ext_click_area(ongo_view->ongoing_btn[BTN_ONGOING_VOL_UP],
			lv_obj_get_style_width(ongo_view->ongoing_btn[BTN_ONGOING_VOL_UP], LV_PART_MAIN));

	/* create coming label */
	_create_btcall_label_array(data->btcall_bg, ongo_view->ongoing_lbl, ongo_view->res_ongoing_txt,
			ongo_view->ongoing_style, NUM_ONGOING_TXTS);

	/* display coming info */
	if (data->cur_phone_number) {
		_display_ongoing_label(data, data->cur_phone_number, TXT_ONGOING_NUM);
	} else {
		_display_ongoing_label(data, "正在通话", TXT_ONGOING_NUM);
	}

	/*dont display calling time for three way calling*/
	if (data->view_state != BTCALL_3W_STATE) {
		/* smaller period to make sure the show seconds do not jump */
		ongo_view->timer_ls = lv_timer_create(_display_ongoing_time_cb, 300, data);
		if (data->ongoing_start_time == 0) {
			_display_ongoing_label(data, "00:00", TXT_ONGOING_TIME);
			data->ongoing_start_time = os_uptime_get();
		} else {
			/*display calling time*/
			data->view_state = BTCALL_ONGOING_STATE;
			_display_ongoing_time_cb(ongo_view->timer_ls);
		}
	}

	return 0;
}

static void _exit_ongoing_view(btcall_view_data_t *data)
{
	ongoing_view_data_t *ongo_view = &data->ongoing_view;

	if (data->view_state != BTCALL_ONGOING_STATE) {
		SYS_LOG_WRN("not ongoing state\n");
		return;
	}

	lvgl_res_unload_pictures(ongo_view->ongoing_img_dsc_bmp, NUM_ONGOING_IMGS);
	lvgl_res_unload_pictures(ongo_view->ongoing_img_dsc_btn, NUM_ONGOING_BTNS);
	lvgl_res_unload_strings(ongo_view->res_ongoing_txt, NUM_ONGOING_TXTS);

	_delete_obj_array(ongo_view->ongoing_bmp, NUM_ONGOING_IMGS);
	_delete_obj_array(ongo_view->ongoing_btn, NUM_ONGOING_BTNS);
	_delete_obj_array(ongo_view->ongoing_lbl, NUM_ONGOING_TXTS);
	/*free calling style memory*/
	lvgl_style_array_reset(ongo_view->ongoing_style, NUM_ONGOING_TXTS);

	if (ongo_view->timer_ls) {
		lv_timer_delete(ongo_view->timer_ls);
		ongo_view->timer_ls = NULL;
	}
}

static void _display_ongoing_label(btcall_view_data_t *data, const char *calling_str, uint8_t txt_num)
{
	ongoing_view_data_t *ongo_view = &data->ongoing_view;

	if (txt_num >= NUM_ONGOING_TXTS || !calling_str)
		return;

	lv_label_set_text(ongo_view->ongoing_lbl[txt_num], calling_str);

	SYS_LOG_INF("calling_str %s\n", calling_str);
}

static void _display_ongoing_time_cb(lv_timer_t *timer)
{
	btcall_view_data_t *data = timer->user_data;
	char time_buff[12] = { 0 };
	uint32_t second  = (os_uptime_get() - data->ongoing_start_time) / 1000;

	if (second >= 3600) {
		snprintf(time_buff, 12, "%02u:%02u:%02u", (uint16_t)(second / 3600), (uint16_t)(second % 3600 / 60), (uint16_t)(second % 60));
	} else {
		snprintf(time_buff, 9, "%02u:%02u", (uint16_t)(second / 60), (uint16_t)(second % 60));
	}

	_display_ongoing_label(data, time_buff, TXT_ONGOING_TIME);
}

static void _handle_ongoing_btn_event(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	btcall_view_data_t *data = lv_event_get_user_data(e);
	ongoing_view_data_t *ongo_view = &data->ongoing_view;

	if (obj == ongo_view->ongoing_btn[BTN_ONGOING_HAND_UP]) {
#if CONFIG_GT2_WATCH_3WAY_POLICY == 0
		uint8_t call_state = 0;
		int ret = bt_manager_hfp_get_call_state(1, &call_state);

		data->cur_phone_number = data->phone_number;

		if (ret == BTSRV_HFP_STATE_CALL_MULTIPARTY) {
			_display_ongoing_label(data, data->cur_phone_number, TXT_ONGOING_NUM);
			btcall_hangupcur_answer_another();
		} else {
			btcall_handup_call();
		}
#else
		btcall_handup_call();
#endif /* CONFIG_GT2_WATCH_3WAY_POLICY == 0 */
	} else if (obj == ongo_view->ongoing_btn[BTN_ONGOING_VOL_UP]) {
		btcall_vol_adjust(true);
	} else if (obj == ongo_view->ongoing_btn[BTN_ONGOING_VOL_DOWN]) {
		btcall_vol_adjust(false);
	} else if (obj == ongo_view->ongoing_btn[BTN_ONGOING_MUTE]) {
		btcall_switch_micmute();
	}
}

static int _display_3way_view(btcall_view_data_t *data)
{
	thr_way_view_data_t *thr_way_view = &data->thr_way_view;

	if (data->view_state != BTCALL_NULL_STATE) {
		SYS_LOG_ERR("in state %d\n", data->view_state);
		return -EINVAL;
	}

	memset(thr_way_view , 0, sizeof(thr_way_view_data_t));

	/* display calling view*/
	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, thr_way_btn_ids, thr_way_view->thr_way_img_dsc_btn, thr_way_view->thr_way_pt_btn, NUM_3W_BTNS);
	lvgl_res_load_strings_from_scene(&data->res_scene, thr_way_txt_ids, thr_way_view->res_3w_txt, NUM_3W_TXTS);

	/* convert resource */
	_cvt_txt_array(thr_way_view->thr_way_pt_txt, thr_way_view->thr_way_style, &data->font,
			thr_way_view->res_3w_txt, NUM_3W_TXTS);

	/* set bg color */
	lv_obj_set_style_bg_color(data->btcall_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);

	/* create button */
	_create_btn_array(data->btcall_bg, thr_way_view->thr_way_btn, thr_way_view->thr_way_pt_btn,
			thr_way_view->thr_way_img_dsc_btn, thr_way_view->thr_way_img_dsc_btn, NUM_3W_BTNS,
			_handle_3way_btn_event, data);

	/* create coming label */
	_create_btcall_label_array(data->btcall_bg, thr_way_view->thr_way_lbl, thr_way_view->res_3w_txt,
			thr_way_view->thr_way_style, NUM_3W_TXTS);

	/* display coming info */
	if (data->cur_phone_number) {
		_display_3way_label(data, data->cur_phone_number, TXT_3W_HOLD_NUM);
	} else {
		_display_3way_label(data, "未知号码", TXT_3W_HOLD_NUM);
	}

	if (data->thr_way_phone_number) {
		_display_3way_label(data, data->thr_way_phone_number, TXT_3W_ANOTH_NUM);
	} else {
		_display_3way_label(data, "三方号码", TXT_3W_ANOTH_NUM);
	}

	//_display_3way_label(data, "结束并接听", TXT_3W_HUP_ANS);
	//_display_3way_label(data, "保持并接听", TXT_3W_HOLD_ANS);

	return 0;
}

static void _exit_3way_view(btcall_view_data_t *data)
{
	thr_way_view_data_t *thr_way_view = &data->thr_way_view;

	if (data->view_state != BTCALL_3W_STATE) {
		SYS_LOG_WRN("not 3way state\n");
		return;
	}

	lvgl_res_unload_pictures(thr_way_view->thr_way_img_dsc_btn, NUM_3W_BTNS);
	lvgl_res_unload_strings(thr_way_view->res_3w_txt, NUM_3W_TXTS);

	_delete_obj_array(thr_way_view->thr_way_btn, NUM_3W_BTNS);
	_delete_obj_array(thr_way_view->thr_way_lbl, NUM_3W_TXTS);
	/*free 3 way style memory*/
	lvgl_style_array_reset(thr_way_view->thr_way_style, NUM_3W_TXTS);

	if (data->thr_way_phone_number) {
		app_mem_free(data->thr_way_phone_number);
		data->thr_way_phone_number = NULL;
	}
}

static void _display_3way_label(btcall_view_data_t *data, const char *thr_way_str, uint8_t txt_num)
{
	thr_way_view_data_t *thr_way_view = &data->thr_way_view;

	if (txt_num >= NUM_3W_TXTS || !thr_way_str)
		return;

	lv_label_set_text(thr_way_view->thr_way_lbl[txt_num], thr_way_str);

	SYS_LOG_INF("outgoing_str(%d) :%s\n", txt_num, thr_way_str);
}

static void _handle_3way_btn_event(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	btcall_view_data_t *data = lv_event_get_user_data(e);
	thr_way_view_data_t *thr_way_view = &data->thr_way_view;

#if CONFIG_GT2_WATCH_3WAY_POLICY == 0
	if (obj == thr_way_view->thr_way_btn[BTN_3W_HUP_CUR] ||
		obj == thr_way_view->thr_way_btn[BTN_3W_HUP_ANS]) {
		btcall_set_phone_num(data->thr_way_phone_number, true);
		btcall_hangupcur_answer_another();
		return;
	}

	if (obj == thr_way_view->thr_way_btn[BTN_3W_HOLD_CUR] ||
		obj == thr_way_view->thr_way_btn[BTN_3W_ANS_ALL]) {
		data->cur_phone_number = data->thr_way_phone_number;
		btcall_holdcur_answer_another();
		return;
	}
#endif /* CONFIG_GT2_WATCH_3WAY_POLICY == 0 */

	if (obj == thr_way_view->thr_way_btn[BTN_3W_HUP_ANOTH]) {
		btcall_hangup_another();
	}
}

static int _display_incoming_view(btcall_view_data_t *data)
{
	incoming_view_data_t *inc_view = &data->incoming_view;

	if (data->view_state != BTCALL_NULL_STATE) {
		SYS_LOG_ERR("in state %d\n", data->view_state);
		return -EINVAL;
	}

	memset(inc_view , 0, sizeof(incoming_view_data_t));

	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, incoming_bmp_ids, inc_view->incoming_img_dsc_bmp,
			inc_view->incoming_pt_bmp, NUM_INCOMING_IMGS);
	lvgl_res_load_pictures_from_scene(&data->res_scene, incoming_btn_ids, inc_view->incoming_img_dsc_btn,
			inc_view->incoming_pt_btn, NUM_INCOMING_BTNS);
	lvgl_res_load_strings_from_scene(&data->res_scene, incoming_txt_ids, inc_view->res_incoming_txt, NUM_INCOMING_TXTS);
	/* convert resource */
#if DEF_UI_WIDTH < 454
	_cvt_txt_array(inc_view->incoming_pt_txt, inc_view->incoming_style, &data->font, inc_view->res_incoming_txt, NUM_INCOMING_TXTS);
#else
	_cvt_txt_array(inc_view->incoming_pt_txt, inc_view->incoming_style, &data->font, inc_view->res_incoming_txt, 1);
	_cvt_txt_array(&inc_view->incoming_pt_txt[1], &inc_view->incoming_style[1], &data->font_28, &inc_view->res_incoming_txt[1], NUM_INCOMING_TXTS - 1);
#endif

	/* create bg color */
	lv_obj_set_style_bg_color(data->btcall_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);

	/* create image */
	_create_img_array(data->btcall_bg, inc_view->incoming_bmp, inc_view->incoming_pt_bmp,
			inc_view->incoming_img_dsc_bmp, NUM_INCOMING_IMGS);
	/* create button */
	_create_btn_array(data->btcall_bg, inc_view->incoming_btn, inc_view->incoming_pt_btn,
			inc_view->incoming_img_dsc_btn, inc_view->incoming_img_dsc_btn, NUM_INCOMING_BTNS,
			_handle_incoming_btn_event, data);
	/* create coming label */
	_create_btcall_label_array(data->btcall_bg, inc_view->incoming_lbl, inc_view->res_incoming_txt,
			inc_view->incoming_style, NUM_INCOMING_TXTS);

	/* display coming info */
	_display_incoming_label(data, "拒绝", TXT_REJECT);
	_display_incoming_label(data, "接受", TXT_ACCEPT);

	if (data->cur_phone_number) {
		_display_incoming_label(data, data->cur_phone_number, TXT_CALL_NUM);
	} else {
		_display_incoming_label(data, /*"未知号码"*/ "", TXT_CALL_NUM);
	}

	return 0;
}

static void _exit_incoming_view(btcall_view_data_t *data)
{
	incoming_view_data_t *inc_view = &data->incoming_view;

	if (data->view_state != BTCALL_INCOMING_STATE) {
		SYS_LOG_WRN("not incoming state\n");
		return;
	}

	lvgl_res_unload_pictures(inc_view->incoming_img_dsc_bmp, NUM_INCOMING_IMGS);
	lvgl_res_unload_pictures(inc_view->incoming_img_dsc_btn, NUM_INCOMING_BTNS);
	lvgl_res_unload_strings(inc_view->res_incoming_txt, NUM_INCOMING_TXTS);

	_delete_obj_array(inc_view->incoming_btn, NUM_INCOMING_BTNS);
	_delete_obj_array(inc_view->incoming_lbl, NUM_INCOMING_TXTS);
	_delete_obj_array(inc_view->incoming_bmp, NUM_INCOMING_IMGS);

	/*free coming style memory*/
	lvgl_style_array_reset(inc_view->incoming_style, NUM_INCOMING_TXTS);
}

static void _display_incoming_label(btcall_view_data_t *data, const char *coming_str, uint8_t txt_num)
{
	incoming_view_data_t *inc_view = &data->incoming_view;

	if (txt_num >= NUM_INCOMING_TXTS || !coming_str)
		return;

	lv_label_set_text(inc_view ->incoming_lbl[txt_num], coming_str);

	SYS_LOG_INF("incoming_str :%s\n", coming_str);
}

static void _handle_incoming_btn_event(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	btcall_view_data_t *data = lv_event_get_user_data(e);
	incoming_view_data_t *inc_view = &data->incoming_view;

	if (obj == inc_view->incoming_btn[BTN_ACCEPT]) {
		_btcall_view_apply_state_transition(data, BTCALL_ONGOING_STATE);
		btcall_accept_call();
	} else if (obj == inc_view->incoming_btn[BTN_REJECT]) {
		btcall_reject_call();
	}
}

static int _display_outgoing_view(btcall_view_data_t *data)
{
	outgoing_view_data_t *outg_view = &data->outgoing_view;

	if (data->view_state != BTCALL_NULL_STATE) {
		SYS_LOG_ERR("in state %d\n", data->view_state);
		return -EINVAL;
	}

	memset(outg_view , 0, sizeof(outgoing_view_data_t));

	/* load resource */
	lvgl_res_load_pictures_from_scene(&data->res_scene, outgoing_bmp_ids, outg_view->outgoing_img_dsc_bmp,
			outg_view->outgoing_pt_bmp, NUM_OUTGOING_IMGS);
	lvgl_res_load_pictures_from_scene(&data->res_scene, outgoing_btn_ids, outg_view->outgoing_img_dsc_btn,
			outg_view->outgoing_pt_btn, NUM_OUTGOING_BTNS);
	lvgl_res_load_strings_from_scene(&data->res_scene, outgoing_txt_ids, outg_view->res_outgoing_txt, NUM_OUTGOING_TXTS);
	/* convert resource */
	_cvt_txt_array(outg_view->outgoing_pt_txt, outg_view->outgoing_style, &data->font, outg_view->res_outgoing_txt, NUM_OUTGOING_TXTS);

	/* set bg color */
	lv_obj_set_style_bg_color(data->btcall_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);

	_create_img_array(data->btcall_bg, outg_view->outgoing_bmp, outg_view->outgoing_pt_bmp,
			outg_view->outgoing_img_dsc_bmp, NUM_OUTGOING_IMGS);
	/* create button */
	_create_btn_array(data->btcall_bg, outg_view->outgoing_btn, outg_view->outgoing_pt_btn,
			outg_view->outgoing_img_dsc_btn, outg_view->outgoing_img_dsc_btn, NUM_OUTGOING_BTNS,
			_handle_outgoing_btn_event, outg_view);
	/* create coming label */
	_create_btcall_label_array(data->btcall_bg, outg_view->outgoing_lbl, outg_view->res_outgoing_txt,
			outg_view->outgoing_style, NUM_OUTGOING_TXTS);

	/* display coming info */
	if (data->cur_phone_number) {
		_display_outgoing_label(data, data->cur_phone_number, TXT_OUTGOING_NUM);
	} else {
		_display_outgoing_label(data, /*"未知号码"*/ "", TXT_OUTGOING_NUM);
	}

	return 0;
}

static void _exit_outgoing_view(btcall_view_data_t *data)
{
	outgoing_view_data_t *outg_view = &data->outgoing_view;

	if (data->view_state != BTCALL_OUTGOING_STATE) {
		SYS_LOG_WRN("not outgoing state\n");
		return;
	}

	lvgl_res_unload_pictures(outg_view->outgoing_img_dsc_bmp, NUM_OUTGOING_IMGS);
	lvgl_res_unload_pictures(outg_view->outgoing_img_dsc_btn, NUM_OUTGOING_BTNS);
	lvgl_res_unload_strings(outg_view->res_outgoing_txt, NUM_OUTGOING_TXTS);

	_delete_obj_array(outg_view->outgoing_btn, NUM_OUTGOING_BTNS);
	_delete_obj_array(outg_view->outgoing_lbl, NUM_OUTGOING_TXTS);
	_delete_obj_array(outg_view->outgoing_bmp, NUM_OUTGOING_IMGS);

	/*free outgoing style memory*/
	lvgl_style_array_reset(outg_view->outgoing_style, NUM_OUTGOING_TXTS);
}

static void _display_outgoing_label(btcall_view_data_t *data, const char *outgoing_str, uint8_t txt_num)
{
	outgoing_view_data_t *outg_view = &data->outgoing_view;

	if (txt_num >= NUM_OUTGOING_TXTS || !outgoing_str)
		return;

	lv_label_set_text(outg_view->outgoing_lbl[txt_num], outgoing_str);

	SYS_LOG_INF("outgoing_str :%s\n", outgoing_str);
}

static void _handle_outgoing_btn_event(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	outgoing_view_data_t *outg_view = lv_event_get_user_data(e);

	if (obj == outg_view->outgoing_btn[BTN_OUTGOING_HAND_UP]) {
		btcall_handup_call();
	}
}

static int _display_siri_view(btcall_view_data_t *data)
{
	siri_view_data_t *siri_view = &data->siri_view;

	if (data->view_state != BTCALL_NULL_STATE) {
		SYS_LOG_ERR("in state %d\n", data->view_state);
		return -EINVAL;
	}

	memset(siri_view , 0, sizeof(siri_view_data_t));

	/* animation */
	lvgl_res_load_picregion_from_scene(&data->res_scene, RES_SIRI_ANIMATION, &siri_view->siri_prg_anim);

	/* set bg color */
	lv_obj_set_style_bg_color(data->btcall_bg, lv_color_make(0, 2, 15), LV_PART_MAIN);

	siri_view->siri_txt = lv_label_create(data->btcall_bg);
	lv_obj_set_style_text_font(siri_view->siri_txt, &data->font, 0);
	lv_obj_set_style_text_color(siri_view->siri_txt, lv_color_white(), 0);
#if DEF_BG_WIDTH >= 454
	lv_obj_align(siri_view->siri_txt, LV_ALIGN_TOP_MID, 0, 30);
#else
	lv_obj_align(siri_view->siri_txt, LV_ALIGN_TOP_MID, 0, 22);
#endif
	lv_label_set_text_static(siri_view->siri_txt, "语音助手");

	/* create siri anim */
	siri_view->siri_anim = anim_img_create(data->btcall_bg);
	if (siri_view->siri_anim) {
		lv_obj_set_pos(siri_view->siri_anim, siri_view->siri_prg_anim.x, siri_view->siri_prg_anim.y);
		lv_obj_set_size(siri_view->siri_anim, siri_view->siri_prg_anim.width, siri_view->siri_prg_anim.height);
		anim_img_set_src_picregion(siri_view->siri_anim, SCENE_BTCALL_VIEW, &siri_view->siri_prg_anim);
		anim_img_set_duration(siri_view->siri_anim, 16, 64 * siri_view->siri_prg_anim.frames);
		anim_img_set_repeat(siri_view->siri_anim, 0, LV_ANIM_REPEAT_INFINITE);
		anim_img_start(siri_view->siri_anim, false);
	}

	return 0;
}

static void _exit_siri_view(btcall_view_data_t *data)
{
	siri_view_data_t *siri_view = &data->siri_view;

	if (data->view_state != BTCALL_SIRI_STATE) {
		SYS_LOG_WRN("not siri state\n");
		return;
	}

	lvgl_res_unload_picregion(&siri_view->siri_prg_anim);

	if (siri_view->siri_anim) {
		lv_obj_delete(siri_view->siri_anim);
	}

	if (siri_view->siri_txt) {
		lv_obj_delete(siri_view->siri_txt);
	}
}

static int _btcall_view_apply_state_transition(btcall_view_data_t *data, uint8_t new_state)
{
	const btcall_view_state_transition_t *state_transition;
	int res = 0;

	if (new_state >= BTCALL_NUM_STATES) {
		SYS_LOG_WRN("unknow new state %d\n", new_state);
		return -EINVAL;
	}

	if (new_state == data->view_state) {
		return 0;
	}

	SYS_LOG_INF("view_state %d -> %d\n", data->view_state, new_state);

	state_transition = &btcall_view_transition_table[data->view_state];
	if (state_transition->exit) {
		state_transition->exit(data);
		SYS_LOG_INF("view %d exit", data->view_state);
	}

	data->view_state = BTCALL_NULL_STATE;

	state_transition = &btcall_view_transition_table[new_state];
	if (state_transition->enter) {
		res = state_transition->enter(data);
		SYS_LOG_INF("view %d enter (res=%d)", new_state, res);
	}

	if (res == 0) {
		data->view_state = new_state;
	}

	return res;
}

static void _btcall_view_layout(void)
{
	SYS_LOG_INF("btcall_bg %p, view_state %d, calling_state %d->%d\n",
		p_btcall_view_data->btcall_bg, p_btcall_view_data->view_state,
		p_btcall_view_data->calling_pre_state, p_btcall_view_data->calling_state);

	if (p_btcall_view_data->calling_state == BTCALL_NULL_STATE) {
		msgbox_cache_close(BTCALL_MSGBOX_ID, true);
	} else {
		if (launcher_is_resumed() == 0)
			return;

		if (p_btcall_view_data->calling_pre_state == BTCALL_NULL_STATE) {
			msgbox_cache_popup(BTCALL_MSGBOX_ID, p_btcall_view_data);
		} else {
			msgbox_cache_paint(BTCALL_MSGBOX_ID, p_btcall_view_data);
		}
	}

	p_btcall_view_data->calling_pre_state = p_btcall_view_data->calling_state;
}

int btcall_msgbox_popup_cb(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * hwnd)
{
	btcall_view_data_t *data = p_btcall_view_data;
	int ret = 0;

	if (msgbox_id != BTCALL_MSGBOX_ID) {
		return -EINVAL;
	}

	if (msg_id == MSG_MSGBOX_CANCEL) {
		SYS_LOG_ERR("popup cancel\n");
		assert(data->view_state == BTCALL_NULL_STATE);
		return 0;
	}

	if (msg_id == MSG_MSGBOX_CLOSE) {
		_btcall_view_delete(data);
		return 0;
	}

	if (msg_id == MSG_MSGBOX_POPUP) {
		assert(data->view_state == BTCALL_NULL_STATE);

		if (_load_resource(data)) {
			SYS_LOG_ERR("load_resource failed\n");
			return -ENOENT;
		}

		/* create bg image */
		data->btcall_bg = lv_obj_create(hwnd);
		lv_obj_set_size(data->btcall_bg, data->res_scene.width, data->res_scene.height);
		lv_obj_center(data->btcall_bg);

		lv_obj_set_style_bg_color(hwnd, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(hwnd, LV_OPA_COVER, 0);
		/*set current background clickable to cut out click event send to front bg*/
		lv_obj_add_flag(data->btcall_bg, LV_OBJ_FLAG_CLICKABLE);
	}

	ret = _btcall_view_apply_state_transition(data, data->calling_state);
	if (ret) {
		_btcall_view_delete(data);
	}

	if (data->need_sync_calling_numb) {
		if (data->cur_phone_number) {
			switch (data->view_state) {
			case BTCALL_INCOMING_STATE:
				_display_incoming_label(data, data->cur_phone_number, TXT_CALL_NUM);
				break;
			case BTCALL_OUTGOING_STATE:
				_display_outgoing_label(data, data->cur_phone_number, TXT_OUTGOING_NUM);
				break;
			case BTCALL_ONGOING_STATE:
				_display_ongoing_label(data, data->cur_phone_number, TXT_ONGOING_NUM);
				break;
			default:
				break;
			}
		}

		data->need_sync_calling_numb = 0;
	}

	return 0;
}

static void _btcall_view_delete(btcall_view_data_t *data)
{
	_btcall_view_apply_state_transition(data, BTCALL_NULL_STATE);

	if (data->btcall_bg) {
		_unload_resource(data);
		lv_obj_delete(data->btcall_bg);
		data->btcall_bg = NULL;
	}

	SYS_LOG_INF("ok\n");
}
