/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <power_manager.h>
#include <app_manager.h>
#include <sys_event.h>
#include <mem_manager.h>
#include "bt_manager_inner.h"
#include <assert.h>
#include "btservice_api.h"
#include <alarm_manager.h>

#ifdef CONFIG_MEDIA
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>
#endif

#define USE_SWITCH_APP		0
#if USE_SWITCH_APP
#include "app_switch.h"
#endif

#define HFP_SIMULATE_CALL_THRESHOLD			(300)		/* 300ms */
#define HFP_ONGOING_ALLOW_SCO_TIME			(2000)		/* 2s */

enum {
	SIRI_INIT_STATE,
	SIRI_START_STATE,
	SIRI_RUNNING_STATE,
};

enum {
	HFP_APP_ALLOW_SCO = 0x1,
	HFP_MGR_ALLOW_SCO = 0x2,
	HFP_ALLOW_SCO     = (HFP_APP_ALLOW_SCO | HFP_MGR_ALLOW_SCO),
};

struct bt_manager_hfp_info_t {
	uint8_t connected:1;
	uint8_t siri_mode:3;
	uint8_t only_sco:1;
	uint8_t allow_sco:2;
	uint8_t phone_num:1;
	uint8_t accept_call:1;
	uint8_t simulate_outgoing:1;		/* Just like WeCha call out */
	uint8_t ongoing_before_connected:1;
	uint8_t siri_sco_connected:1;
	uint8_t pending_incoming:1;
	uint8_t hprec_battery:1;
	uint32_t hfp_status;
	uint32_t sco_connect_time;
	uint32_t send_atcmd_time;
	uint32_t outgoing_time;
	uint32_t ongoing_time;
	os_delayed_work hfp_active_source_switch_work;
};

static void btmgr_hfp_call_ongoing_proc(void);

static struct bt_manager_hfp_info_t hfp_manager;
static struct btmgr_map_time cclk_tm;

static void btmgr_hfp_allow_sco(uint8_t type, bool allow)
{
	uint8_t old_allow = hfp_manager.allow_sco;

#ifdef CONFIG_BT_AUDIO_TEST
	hfp_manager.allow_sco = HFP_ALLOW_SCO;
#else
	if (bt_manager_config_pts_test()) {
		hfp_manager.allow_sco = HFP_ALLOW_SCO;
	} else {
		if (allow) {
			hfp_manager.allow_sco |= type;
		} else {
			hfp_manager.allow_sco &= (~type);
		}
	}
#endif

	SYS_LOG_INF("allow sco %d->%d (%d, %d)", old_allow, hfp_manager.allow_sco, type, allow);
	if ((old_allow == HFP_ALLOW_SCO) && (hfp_manager.allow_sco != HFP_ALLOW_SCO)) {
		btif_br_allow_sco_connect(false);
	} else if ((old_allow != HFP_ALLOW_SCO) && (hfp_manager.allow_sco == HFP_ALLOW_SCO)) {
		btif_br_allow_sco_connect(true);
	}
}

static bool btmgr_hfp_is_sco_allow(void)
{
	return (hfp_manager.allow_sco == HFP_ALLOW_SCO) ? true : false;
}

static uint32_t _bt_manager_call_to_hfp_volume(uint32_t call_vol)
{
	uint32_t  hfp_vol = 0;
#ifdef CONFIG_AUDIO
	if (call_vol == 0) {
		hfp_vol = 0;
	} else if (call_vol >= audio_policy_get_volume_level()) {
		hfp_vol = 0x0F;
	} else {
		hfp_vol = (call_vol * 0x10 / audio_policy_get_volume_level());
		if (hfp_vol == 0) {
			hfp_vol = 1;
		}
	}
#endif
	return hfp_vol;
}

#ifdef CONFIG_VOLUME_MANAGER
static uint32_t _bt_manager_hfp_to_call_volume(uint32_t hfp_vol)
{
	uint32_t  call_vol = 0;
#ifdef CONFIG_AUDIO
	if (hfp_vol == 0) {
		call_vol = 0;
	} else if (hfp_vol >= 0x0F) {
		call_vol = audio_policy_get_volume_level();
	} else {
		call_vol = (hfp_vol + 1) * audio_policy_get_volume_level() / 0x10;

		if (call_vol == 0) {
	        call_vol = 1;
		}
	}
#endif
	return call_vol;
}
#endif

int bt_manager_hfp_set_status(uint32_t state)
{
	SYS_LOG_INF("Set hfp state 0x%x -> 0x%x", hfp_manager.hfp_status, state);
	hfp_manager.hfp_status = state;
	return 0;
}

int bt_manager_hfp_get_status(void)
{
	return hfp_manager.hfp_status;
}

static uint8_t string_to_num(uint8_t c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else {
		return 0;
	}
}

//format like "20/11/30, 18:58:09"
//https://m2msupport.net/m2msupport/atcclk-clock/
static int _bt_hfp_set_time(uint8_t *time)
{
	cclk_tm.tm_year = 2000 + string_to_num(time[0])*10 + string_to_num(time[1]);
	cclk_tm.tm_mon = string_to_num(time[3])*10 + string_to_num(time[4]);
	cclk_tm.tm_mday = string_to_num(time[6])*10 + string_to_num(time[7]);

	if (time[9] == ' ') {
		cclk_tm.tm_hour = string_to_num(time[10])*10 + string_to_num(time[11]);
		cclk_tm.tm_min = string_to_num(time[13])*10 + string_to_num(time[14]);
		cclk_tm.tm_sec = string_to_num(time[16])*10 + string_to_num(time[17]);
	} else if(time[9] >= '0' && time[9] <= '9') {
		cclk_tm.tm_hour = string_to_num(time[9])*10 + string_to_num(time[10]);
		cclk_tm.tm_min = string_to_num(time[12])*10 + string_to_num(time[13]);
		cclk_tm.tm_sec = string_to_num(time[15])*10 + string_to_num(time[16]);
	}

	struct app_msg msg = {0};
	msg.type = MSG_BT_MGR_EVENT;
	msg.cmd = BT_MAP_SET_TIME_EVENT;
	msg.ptr = &cclk_tm;
	send_async_msg(app_manager_get_current_app(), &msg);
	return 0;
}

static void btmgr_hfp_clcc_info_proc(uint8_t *param, int param_size)
{
	uint32_t state;
	int ret;
	uint8_t call_state, held_single;
	struct btsrv_hfp_clcc_info* clcc = (struct btsrv_hfp_clcc_info*)param;
	size_t number_size = strlen(clcc->number);

	if (hfp_manager.simulate_outgoing) {
		SYS_LOG_INF("hfp clcc simu return");
		return;
	}

	ret = bt_manager_hfp_get_call_state(1, &call_state);
	held_single = ((call_state & 0x4) && (!(call_state & 0x10)));
	SYS_LOG_INF("ret %d call_state %d held_single %d.", ret, call_state, held_single);
	SYS_LOG_INF("hfp clcc phone num %s size %d\n", clcc->number, number_size);
	if (number_size) {
		state = bt_manager_hfp_get_status();
		if ((state == BT_STATUS_OUTGOING) ||
			(state == BT_STATUS_ONGOING && clcc->status == 0) ||
			(state == BT_STATUS_MULTIPARTY && (clcc->status == 0 || (held_single && clcc->status == 1))) ||
			(state == BT_STATUS_3WAYIN && clcc->status == 0) ||
			(state == BT_STATUS_INCOMING && clcc->status == 4)) {
			hfp_manager.phone_num = 1;
			bt_manager_event_notify(BT_HFP_CALL_CLCC_INFO, param, param_size);
		}

		if (clcc->status == 0) {
			if ((state == BT_STATUS_MULTIPARTY || state == BT_STATUS_3WAYIN) && (!hfp_manager.sco_connect_time)) {
				/* Sco not in device, phone operate 3way, not allow sco connect untill ongoing */
			} else {
				/* Call in active state, allow sco connect,
				 * in outgoing/alerted state, some phone will try connect sco several times,
				 * so in outgoing/alerted state, can't allow sco connect.
				 */
				if ((state == BT_STATUS_ONGOING) &&
					((os_uptime_get_32() - hfp_manager.ongoing_time) < HFP_ONGOING_ALLOW_SCO_TIME)) {
					/* Ongoing not too long, allow sco connect later by hfp_active_source_switch_work */
				} else {
					btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
				}
			}
		}
	}
}

static void btmgr_hfp_connected_proc(void)
{
	hfp_manager.connected = 1;

#ifdef CONFIG_POWER
	if (false == hfp_manager.hprec_battery) {
		bt_manager_hfp_battery_report(BT_BATTERY_REPORT_INIT, 0);
		bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, power_manager_get_battery_capacity());
	} else {
		bt_manager_hfp_battery_hprec_report(power_manager_get_battery_capacity());
	}
#endif
#ifdef CONFIG_VOLUME_MANAGER
	bt_manager_hfp_sync_vol_to_remote(audio_system_get_stream_volume(AUDIO_STREAM_VOICE));
#endif
	bt_manager_hfp_get_time();

	if (hfp_manager.ongoing_before_connected) {
		hfp_manager.ongoing_before_connected = 0;
		SYS_LOG_INF("Proc pending ongoing");
		btmgr_hfp_call_ongoing_proc();
	}
}

static void btmgr_hfp_disconnected_proc(void)
{
	bt_manager_hfp_set_status(BT_STATUS_HFP_NONE);
	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
	hfp_manager.connected = 0;
	hfp_manager.siri_mode = SIRI_INIT_STATE;
	//hfp_manager.only_sco = 0;	/* Clear by BTSRV_SCO_DISCONNECTED event */
	hfp_manager.phone_num = 0;
	hfp_manager.accept_call = 0;
	hfp_manager.simulate_outgoing = 0;
	hfp_manager.ongoing_before_connected = 0;
	hfp_manager.siri_sco_connected = 0;
	hfp_manager.pending_incoming = 0;
	hfp_manager.sco_connect_time = 0;
	hfp_manager.hprec_battery = 0;
}

static void btmgr_hft_query_curr_call(bool force)
{
	static uint32_t pre_time;
	uint32_t curr_time = os_uptime_get_32();

	if (force) {
		pre_time = curr_time;
		bt_manager_hfp_send_at_command("AT+CLCC", 1);
	} else if ((curr_time - pre_time) > 600) {
		pre_time = curr_time;
		bt_manager_hfp_send_at_command("AT+CLCC", 1);
	}
}

static void btmgr_hfp_call_incoming_proc(void)
{
	int ret;
	uint8_t call_state;

	if ((bt_manager_hfp_get_status() == BT_STATUS_SIRI) ||
		(hfp_manager.siri_sco_connected == 1)) {
		/* Incoming break siri, process imcoming after siri exit and sco disconnected. */
		hfp_manager.pending_incoming = 1;
		SYS_LOG_INF("hfp incoming break siri");
		return;
	} else {
		hfp_manager.pending_incoming = 0;
	}

	if (bt_manager_hfp_get_status() != BT_STATUS_3WAYIN) {
		/* Incoming in 3wayin state, still keep in 3wayin state untill goto ongoing state */
		bt_manager_hfp_set_status(BT_STATUS_INCOMING);
	} else {
		/* Incoming in not 3wayin state */
		ret = bt_manager_hfp_get_call_state(1, &call_state);
		SYS_LOG_INF("ret %d call_state %d", ret, call_state);
		if ((ret == BTSRV_HFP_STATE_CALL_INCOMING) && (0x02 == call_state)) {
			bt_manager_hfp_set_status(BT_STATUS_INCOMING);
		}
	}

#if USE_SWITCH_APP
	if (btmgr_hfp_is_sco_allow() && strcmp("tws", app_manager_get_current_app())) {
		app_switch("btcall", APP_SWITCH_CURR, true);
		app_switch_lock(1);
	}
#else
	bt_manager_event_notify(BT_HFP_CALL_STATE_START, NULL, 0);
#endif

	hfp_manager.only_sco = 0;
	hfp_manager.simulate_outgoing = 0;
	if ((bt_manager_hfp_get_status() == BT_STATUS_3WAYIN) && (!hfp_manager.sco_connect_time)) {
		/* 3Waycall state, soc not on device,
		 * Not display incoming view,
		 * not allow sco connect untill ongoing swith allow sco.
		 */
	} else {
		/* only support local ringtone.*/
		// btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
		bt_manager_event_notify(BT_HFP_CALL_INCOMING, NULL, 0);
	}

	btmgr_hft_query_curr_call(false);
}

static void btmgr_hfp_check_start_pending_incoming(void)
{
	/* Some phone siri exit before sco disconnected,
	 * some phone sco disconnected before siri exit,
	 * Have pending incoming, siri exit, sco diconected.
	 */
	if (hfp_manager.pending_incoming &&
		(bt_manager_hfp_get_status() == BT_STATUS_HFP_NONE) &&
		(!hfp_manager.siri_sco_connected)) {
		/* Resume incoming after siri exit, and siri sco discnnected */
		SYS_LOG_INF("hfp resume incoming");
		btmgr_hfp_call_incoming_proc();
	}
}

static void btmgr_hfp_call_outgoing_alerted_proc(uint8_t event)
{
	if (!hfp_manager.phone_num) {
		/* Just for get outgoing phone number */
		btmgr_hft_query_curr_call(false);
	}
	bt_manager_hfp_set_status(BT_STATUS_OUTGOING);

#if USE_SWITCH_APP
	if (btmgr_hfp_is_sco_allow() && strcmp("tws", app_manager_get_current_app())) {
		app_switch("btcall", APP_SWITCH_CURR, true);
		app_switch_lock(1);
	}
#else
	bt_manager_event_notify(BT_HFP_CALL_STATE_START, NULL, 0);
#endif

	hfp_manager.only_sco = 0;
	hfp_manager.simulate_outgoing = 0;
	if (event == BTSRV_HFP_CALL_OUTGOING) {
		/* Can update time in alerted, alerted too close to ongoing. */
		hfp_manager.outgoing_time = os_uptime_get_32();
	}
	bt_manager_event_notify(BT_HFP_CALL_OUTGOING, NULL, 0);
}

static void btmgr_hfp_call_ongoing_proc(void)
{
	uint32_t pre_state = bt_manager_hfp_get_status();

	if (!hfp_manager.connected) {
		SYS_LOG_INF("Pending ongoing");
		hfp_manager.ongoing_before_connected = 1;
		return;
	}

	if (pre_state == BT_STATUS_3WAYIN || pre_state == BT_STATUS_MULTIPARTY) {
		/* Operate 3way in phoone
		 * from 3wayin state to ongoing state, phone number may change, query again
		 */
		btmgr_hft_query_curr_call(true);
	}
	bt_manager_hfp_set_status(BT_STATUS_ONGOING);

#if USE_SWITCH_APP
	if (btmgr_hfp_is_sco_allow() && strcmp("tws", app_manager_get_current_app())) {
		app_switch("btcall", APP_SWITCH_CURR, true);
		app_switch_lock(1);
	}
#else
	bt_manager_event_notify(BT_HFP_CALL_STATE_START, NULL, 0);
#endif

	if ((pre_state == BT_STATUS_INCOMING) && (hfp_manager.accept_call == 0)) {
		/* Incoming,  accepte on phone, allow sco connect 2s later. */
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);

		/* if in CONFIG_BT_AUDIO_TEST mode, not exit call view, not disconnect sco. */
		if (!btmgr_hfp_is_sco_allow()) {
			/* Sco connected or not, need notify app exit call */
			bt_manager_event_notify(BT_HFP_ESCO_RELEASED_EVENT, NULL, 0);
			if (hfp_manager.sco_connect_time) {
				btif_hfp_hf_switch_sound_source();
			}
		}
	}

	if ((os_uptime_get_32() - hfp_manager.outgoing_time) < HFP_SIMULATE_CALL_THRESHOLD) {
		hfp_manager.simulate_outgoing = 1;
		SYS_LOG_INF("hfp simu outgoing");
	}

	hfp_manager.only_sco = 0;
	hfp_manager.accept_call = 0;
	hfp_manager.ongoing_time = os_uptime_get_32();
	bt_manager_event_notify(BT_HFP_CALL_ONGOING, NULL, 0);
	os_delayed_work_submit(&hfp_manager.hfp_active_source_switch_work, HFP_ONGOING_ALLOW_SCO_TIME);
}

static void btmgr_hfp_call_3wayin_proc(void)
{
	bt_manager_hfp_set_status(BT_STATUS_3WAYIN);
	hfp_manager.only_sco = 0;

	if (!hfp_manager.sco_connect_time) {
		/* Sco not in device, phone operate 3way, not allow sco connect untill ongoing */
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
	}
}

static void btmgr_hfp_call_multiparty_proc(void)
{
	uint8_t call_state;
	int ret;

	bt_manager_hfp_set_status(BT_STATUS_MULTIPARTY);
	hfp_manager.only_sco = 0;

	if (!hfp_manager.sco_connect_time) {
		/* Sco not in device, phone operate 3way, not allow sco connect untill ongoing or 2s later */
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
		os_delayed_work_submit(&hfp_manager.hfp_active_source_switch_work, HFP_ONGOING_ALLOW_SCO_TIME);
		btmgr_hft_query_curr_call(true);
	} else {
		btmgr_hft_query_curr_call(true);	/* Phone swith active and hold call, query and update phone number */
	}

	ret = bt_manager_hfp_get_call_state(1, &call_state);
	SYS_LOG_INF("call_state %d %d", ret, call_state);
	if ((ret != BTSRV_HFP_STATE_INIT) && (call_state & 0xF0)) {
		bt_manager_event_notify(BT_HFP_CALL_ONGOING, NULL, 0);
	}
}

static void btmgr_hfp_call_exit_proc(void)
{
	bt_manager_hfp_set_status(BT_STATUS_HFP_NONE);
	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);

#if USE_SWITCH_APP
	if (!strcmp("btcall", app_manager_get_current_app())) {
		bt_manager_event_notify(BT_HFP_CALL_HUNGUP, NULL, 0);
		app_switch_unlock(1);
		app_switch(NULL, APP_SWITCH_LAST, false);
	}
#else
	bt_manager_event_notify(BT_HFP_CALL_HUNGUP, NULL, 0);
	bt_manager_event_notify(BT_HFP_CALL_STATE_EXIT, NULL, 0);
#endif

	hfp_manager.phone_num = 0;
	hfp_manager.accept_call = 0;
	hfp_manager.simulate_outgoing = 0;
	hfp_manager.siri_sco_connected = 0;
	hfp_manager.pending_incoming = 0;
}

/* Callback when sco connected withou incoming/outgoing,
 * just like some device create siri sco without hfp notify,
 * BTSRV_SCO_CONNECTED callback will follow BTSRV_HFP_SCO callback.
 */
static void btmgr_hfp_sco_proc(void)
{
#if USE_SWITCH_APP
	if (btmgr_hfp_is_sco_allow() && strcmp("tws", app_manager_get_current_app())) {
		hfp_manager.only_sco = 1;
		app_switch("btcall", APP_SWITCH_CURR, true);
		app_switch_lock(1);
	}
#else
	hfp_manager.only_sco = 1;
	bt_manager_event_notify(BT_HFP_CALL_STATE_START, NULL, 0);
#endif

	if (hfp_manager.siri_mode == SIRI_START_STATE) {
		hfp_manager.siri_mode = SIRI_RUNNING_STATE;
	}

	if (hfp_manager.siri_mode == SIRI_RUNNING_STATE) {
		bt_manager_hfp_set_status(BT_STATUS_SIRI);
		bt_manager_event_notify(BT_HFP_CALL_SIRI_MODE, NULL, 0);
	}
}

static void btmgr_hfp_volume_change_proc(uint8_t *param)
{
	uint32_t volume = *(uint8_t *)(param);

	SYS_LOG_INF("volume %d", volume);
	uint32_t pass_time = os_uptime_get_32() - hfp_manager.sco_connect_time;
	/* some phone may send volume sync event after sco connect. */
	/* we must drop these event to avoid volume conflict */
	/* To Do: do we have better solution?? */
#if USE_SWITCH_APP
	if ((hfp_manager.sco_connect_time && (pass_time < 500))
	|| strcmp("btcall", app_manager_get_current_app())) {
		SYS_LOG_INF("drop volume sync event from phone\n");
		return;
	}
#else
	if(hfp_manager.sco_connect_time && (pass_time < 500)){
		SYS_LOG_INF("drop volume sync event from phone\n");
		return;
	}
#endif

#ifdef CONFIG_VOLUME_MANAGER
	uint32_t call_vol = _bt_manager_hfp_to_call_volume(volume);
	system_volume_sync_remote_to_device(AUDIO_STREAM_VOICE, call_vol);
#endif
}

static void btmgr_hfp_siri_state_change_proc(uint8_t *param)
{
	uint32_t state = *(uint8_t *)(param);

	SYS_LOG_INF("hfp siri change state %d", state);
	if (state == BTSRV_HFP_SIRI_CHANGE_ACTIVE) {
		/* Receive AT notify siri enable */
		hfp_manager.siri_mode = SIRI_RUNNING_STATE;
		bt_manager_hfp_set_status(BT_STATUS_SIRI);
		sys_event_notify(SYS_EVENT_SIRI_START);
	} else if (state == BTSRV_HFP_SIRI_CHANGE_START) {
		/* start siri cmd send OK,just play tts */
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
		hfp_manager.siri_mode = SIRI_START_STATE;
		sys_event_notify(SYS_EVENT_SIRI_START);
	} else if (state == BTSRV_HFP_SIRI_CHANGE_DEACTIVE) {
		/* Receive AT notify siri disable */
		hfp_manager.siri_mode = SIRI_INIT_STATE;
		if (bt_manager_hfp_get_status() == BT_STATUS_SIRI) {
			bt_manager_hfp_set_status(BT_STATUS_HFP_NONE);
		}

		sys_event_notify(SYS_EVENT_SIRI_STOP);
		if (!hfp_manager.only_sco) {
			btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
		}

		btmgr_hfp_check_start_pending_incoming();
	} else {
		SYS_LOG_ERR("Siri unknow change %d", state);
	}
}

static void btmgr_hfp_sco_connected_proc(void)
{
	uint32_t state = bt_manager_hfp_get_status();

	SYS_LOG_INF("allow_sco %d status 0x%x", btmgr_hfp_is_sco_allow(), state);
	bt_manager_event_notify(BT_HFP_CALL_STATE_START, NULL, 0);
	hfp_manager.sco_connect_time = os_uptime_get_32();
	bt_manager_event_notify(BT_HFP_ESCO_ESTABLISHED_EVENT, NULL, 0);

	if (state == BT_STATUS_INCOMING) {
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
	}

	if (state == BT_STATUS_SIRI) {
		hfp_manager.siri_sco_connected = 1;
	}
}

static void btmgr_hfp_sco_disconnected_proc(void)
{
	uint32_t pre_state = bt_manager_hfp_get_status();

	SYS_LOG_INF("hfp only_sco %d state 0x%x", hfp_manager.only_sco, pre_state);
	hfp_manager.sco_connect_time = 0;
	bt_manager_event_notify(BT_HFP_ESCO_RELEASED_EVENT, NULL, 0);

#if USE_SWITCH_APP
	/**hfp_manager.only_sco == 0 means sco disconnect by switch sound source, not exit btcall app */
	if (hfp_manager.only_sco && !strcmp("btcall", app_manager_get_current_app())) {
		hfp_manager.only_sco = 0;
		app_switch_unlock(1);
		app_switch(NULL, APP_SWITCH_LAST, false);
	}
#else
	if (hfp_manager.only_sco) {
		hfp_manager.only_sco = 0;
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
		bt_manager_hfp_set_status(BT_STATUS_HFP_NONE);
		bt_manager_event_notify(BT_HFP_CALL_STATE_EXIT, NULL, 0);
	}
#endif

	hfp_manager.siri_mode = SIRI_INIT_STATE;
	hfp_manager.siri_sco_connected = 0;
	if (pre_state == BT_STATUS_HFP_NONE) {
		/* Already exit siri and disconnected sco. */
		btmgr_hfp_check_start_pending_incoming();
	}
}

static void _bt_manager_hfp_callback(btsrv_hfp_event_e event, uint8_t *param, int param_size)
{
	// temp debug for testing
	bt_manager_hfp_dump_info();

	switch (event) {
	case BTSRV_HFP_CONNECTED:
		SYS_LOG_INF("hfp connected");
		btmgr_hfp_connected_proc();
		break;
	case BTSRV_HFP_DISCONNECTED:
		SYS_LOG_INF("hfp disconnected");
		btmgr_hfp_disconnected_proc();
		break;
	case BTSRV_HFP_PHONE_NUM:
		SYS_LOG_INF("hfp phone num %s size %d", param, param_size);
		hfp_manager.phone_num = 1;
		bt_manager_event_notify(BT_HFP_CALL_RING_STATR_EVENT, param, param_size);
		break;
	case BTSRV_HFP_PHONE_NUM_STOP:
		SYS_LOG_INF("hfp phone stop");
		bt_manager_event_notify(BT_HFP_CALL_RING_STOP_EVENT, NULL, 0);
		break;
	case BTSRV_HFP_PHONE_NAME:
		SYS_LOG_INF("hfp phone name %s size %d", param, param_size);
		break;
	case BTSRV_HFP_CCWA_PHONE_NUM:
		SYS_LOG_INF("hfp ccwa phone num %s size %d", param, param_size);
		if (hfp_manager.sco_connect_time) {
			/* Only notify CCWA phone number when sco connected */
			bt_manager_event_notify(BT_HFP_CALL_CCWA_EVENT, param, param_size);
		}
		break;
	case BTSRV_HFP_CLCC_INFO:
		SYS_LOG_INF("hfp clcc info");
		btmgr_hfp_clcc_info_proc(param, param_size);
		break;
	case BTSRV_HFP_CODEC_INFO:
		SYS_LOG_INF("codec_id %d sample_rate %d", param[0], param[1]);
		bt_manager_sco_set_codec_info(param[0], param[1]);
		break;
	case BTSRV_HFP_CALL_INCOMING:
		SYS_LOG_INF("hfp call incoming");
		btmgr_hfp_call_incoming_proc();
		break;
	case BTSRV_HFP_CALL_OUTGOING:
	case BTSRV_HFP_CALL_ALERTED:
		SYS_LOG_INF("hfp call %s", (event == BTSRV_HFP_CALL_OUTGOING) ? "outgoing" : "alerted");
		btmgr_hfp_call_outgoing_alerted_proc(event);
		break;
	case BTSRV_HFP_CALL_ONGOING:
		SYS_LOG_INF("hfp call ongoing\n");
		btmgr_hfp_call_ongoing_proc();
		break;
	case BTSRV_HFP_CALL_3WAYIN:
		SYS_LOG_INF("hfp 3wayin");
		btmgr_hfp_call_3wayin_proc();
		break;
	case BTSRV_HFP_CALL_MULTIPARTY:
		SYS_LOG_INF("hfp call multiparty");
		btmgr_hfp_call_multiparty_proc();
		break;
	case BTSRV_HFP_CALL_EXIT:
		SYS_LOG_INF("hfp call exit");
		btmgr_hfp_call_exit_proc();
		break;
	case BTSRV_HFP_SCO:
		SYS_LOG_INF("hfp sco");
		btmgr_hfp_sco_proc();
		break;
	case BTSRV_HFP_VOLUME_CHANGE:
		SYS_LOG_INF("hfp volume change");
		btmgr_hfp_volume_change_proc(param);
		break;
	case BTSRV_HFP_SIRI_STATE_CHANGE:
		btmgr_hfp_siri_state_change_proc(param);
		break;
	case BTSRV_SCO_CONNECTED:
		SYS_LOG_INF("hfp sco cnnected");
		btmgr_hfp_sco_connected_proc();
		break;
	case BTSRV_SCO_DISCONNECTED:
		SYS_LOG_INF("hfp sco discnnected");
		btmgr_hfp_sco_disconnected_proc();
		break;
	case BTSRV_HFP_ACTIVE_DEV_CHANGE:
		SYS_LOG_INF("hfp dev changed");
		bt_manager_event_notify(BT_HFP_ACTIVEDEV_CHANGE_EVENT, NULL, 0);
		break;
	case BTSRV_HFP_TIME_UPDATE:
		SYS_LOG_INF("time update %s", param);
		_bt_hfp_set_time(param);
		break;
	case BTSRV_HFP_SCO_REJECTED:
		SYS_LOG_INF("hfp sco rejected");
		if (!btmgr_hfp_is_sco_allow()) {
			btmgr_hft_query_curr_call(false);
		}
		break;
	case BTSRV_HFP_BATTERY_HPREC:
		SYS_LOG_INF("hfp battery Indicator enable.");
		hfp_manager.hprec_battery = true;
		//bt_manager_hfp_battery_hprec_report(power_manager_get_battery_capacity());
		break;
	default:
		break;
	}
}

int bt_manager_hfp_profile_start(void)
{
	return btif_hfp_start(&_bt_manager_hfp_callback);
}

int bt_manager_hfp_profile_stop(void)
{
	return btif_hfp_stop();
}

int bt_manager_hfp_get_call_state(uint8_t active_call,uint8_t *call_state)
{
	return btif_hfp_hf_get_call_state(active_call, call_state);
}

int bt_manager_hfp_check_atcmd_time(void)
{
	uint32_t pass_time = os_uptime_get_32() - hfp_manager.send_atcmd_time;

	if (hfp_manager.send_atcmd_time && (pass_time < 600)) {
		return 0;
	}
	hfp_manager.send_atcmd_time = os_uptime_get_32();
	return 1;
}

int bt_manager_hfp_dial_number(uint8_t *number)
{
	if (!bt_manager_hfp_check_atcmd_time() ||
		!btif_hfp_active_phone_service_ready()) {
		return -1;
	}

	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
	return btif_hfp_hf_dial_number(number);
}

int bt_manager_hfp_dial_last_number(void)
{
	if (!bt_manager_hfp_check_atcmd_time() ||
		!btif_hfp_active_phone_service_ready()) {
		return -1;
	}

	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
	return btif_hfp_hf_dial_last_number();
}

int bt_manager_hfp_dial_memory(int location)
{
	if (!bt_manager_hfp_check_atcmd_time() ||
		!btif_hfp_active_phone_service_ready()) {
		return -1;
	}

	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
	return btif_hfp_hf_dial_memory(location);
}

int bt_manager_hfp_volume_control(uint8_t type, uint8_t volume)
{
#ifdef CONFIG_BT_HFP_HF_VOL_SYNC
	return btif_hfp_hf_volume_control(type, volume);
#else
	return 0;
#endif
}

int bt_manager_hfp_battery_report(uint8_t mode, uint8_t bat_val)
{
	if (mode == BT_BATTERY_REPORT_VAL) {
		bat_val = bat_val / 10;
		if (bat_val > 9) {
			bat_val = 9;
		}
	}
	SYS_LOG_INF("mode %d , bat_val %d\n", mode, bat_val);
	return btif_hfp_hf_battery_report(mode, bat_val);
}

int bt_manager_hfp_battery_hprec_report(uint8_t bat_val)
{
	SYS_LOG_INF("bat_val %d\n", bat_val);
	return btif_hfp_hf_battery_hprec_report(bat_val);
}

int bt_manager_hfp_accept_call(void)
{
	SYS_LOG_INF("hfp accept call");
	if (bt_manager_hfp_get_status() == BT_STATUS_INCOMING) {
		hfp_manager.accept_call = 1;
	}
	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
	return btif_hfp_hf_accept_call();
}

int bt_manager_hfp_reject_call(void)
{
	SYS_LOG_INF("hfp reject call");
	return btif_hfp_hf_reject_call();
}

int bt_manager_hfp_hangup_call(void)
{
	SYS_LOG_INF("hfp hangup call");
	return btif_hfp_hf_hangup_call();
}

int bt_manager_hfp_hangup_another_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}

	SYS_LOG_INF("hfp hangup another call");
	return btif_hfp_hf_hangup_another_call();
}

int bt_manager_hfp_holdcur_answer_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}

	SYS_LOG_INF("hfp holdcur answer call");
	return btif_hfp_hf_holdcur_answer_call();
}

int bt_manager_hfp_hangupcur_answer_call(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}

	SYS_LOG_INF("hfp hangupcur answer call");
	return btif_hfp_hf_hangupcur_answer_call();
}

int bt_manager_hfp_start_siri(void)
{
	int ret = 0;

	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	if (hfp_manager.siri_mode != SIRI_RUNNING_STATE) {
		ret = btif_hfp_hf_voice_recognition_start();
	}
	return ret;
}

int bt_manager_hfp_stop_siri(void)
{
	int ret = 0;

	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	if (hfp_manager.siri_mode != SIRI_INIT_STATE) {
		ret = btif_hfp_hf_voice_recognition_stop();
		sys_event_notify(SYS_EVENT_SIRI_STOP);

		if (hfp_manager.only_sco == 0) {
			btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
			hfp_manager.siri_mode = SIRI_INIT_STATE;
		}
		/* When sco connected, BTSRV_SCO_DISCONNECTED will callback */
	}
	return ret;
}

int bt_manager_hfp_switch_sound_source(void)
{
	if (!bt_manager_hfp_check_atcmd_time()) {
		return -1;
	}
	return btif_hfp_hf_switch_sound_source();
}

int bt_manager_hfp_get_time(void)
{
	return btif_hfp_hf_get_time();
}

/* Just call by app */
int bt_manager_allow_sco_connect(bool allowed)
{
	btmgr_hfp_allow_sco(HFP_APP_ALLOW_SCO, allowed);
	return 0;
}

int bt_manager_hfp_send_at_command(uint8_t *command,uint8_t active_call)
{
	return btif_hfp_hf_send_at_command(command,active_call);
}

int bt_manager_hfp_sync_vol_to_remote(uint32_t call_vol)
{
	uint32_t  hfp_vol = _bt_manager_call_to_hfp_volume(call_vol);

	return bt_manager_hfp_volume_control(1, (uint8_t)hfp_vol);
}

static void bt_manager_hfp_active_source_switch(struct k_work *work)
{
	SYS_LOG_INF("status %d num %d\n", bt_manager_hfp_get_status(), hfp_manager.phone_num);

	if ((bt_manager_hfp_get_status() == BT_STATUS_ONGOING) && (!btmgr_hfp_is_sco_allow())) {
		if (hfp_manager.phone_num) {
			btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
		} else {
			/* Some phone, in calling state, disconnect and connect bt,
			 * will not create sco link, so need query current call again.
			 */
			btmgr_hft_query_curr_call(false);
		}
	} else if ((bt_manager_hfp_get_status() == BT_STATUS_MULTIPARTY) && (!hfp_manager.sco_connect_time)) {
		btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, true);
	}
}

int bt_manager_hfp_init(void)
{
	memset(&hfp_manager, 0, sizeof(struct bt_manager_hfp_info_t));
	hfp_manager.allow_sco = HFP_ALLOW_SCO;
	btmgr_hfp_allow_sco(HFP_MGR_ALLOW_SCO, false);
	os_delayed_work_init(&hfp_manager.hfp_active_source_switch_work, bt_manager_hfp_active_source_switch);
	return 0;
}

void bt_manager_hfp_dump_info(void)
{
	printk("hfp info\n");
	printk("\t connected %d state 0x%x siri %d sco %d %d PASO %d %d %d %d %d %d T %d\n",
			hfp_manager.connected, hfp_manager.hfp_status, hfp_manager.siri_mode, hfp_manager.only_sco,
			hfp_manager.allow_sco, hfp_manager.phone_num, hfp_manager.accept_call, hfp_manager.simulate_outgoing,
			hfp_manager.ongoing_before_connected, hfp_manager.pending_incoming, hfp_manager.siri_sco_connected, hfp_manager.sco_connect_time);
}
