/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager a2dp profile.
 */
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <bt_manager.h>
#include <sys_event.h>
#include <app_manager.h>
#include <power_manager.h>
#include "bt_manager_inner.h"
#include "audio_policy.h"
#include "app_switch.h"
#include "btservice_api.h"
#include "media_player.h"
#include <volume_manager.h>
#include <audio_system.h>
#include <input_manager.h>
#include "bt_porting_inner.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

#define TWS_PAIR_TYR_TIMES		3
#define TWS_TIMETO_BT_CLOCK(a)			  ((a) * 10000 / 3125)

static tws_runtime_observer_t *tws_observer;

struct tws_sync_event_t {
	sys_snode_t node;
	uint32_t event;
	uint32_t bt_clock;
};

struct bt_manager_tws_context_t {
	sys_slist_t tws_sync_event_list;
	os_work tws_irq_work;
	uint8_t irq_req_flag:1;
	uint8_t slave_actived:1;
	uint8_t record_low_latency:1;
	uint8_t peer_version;
	uint32_t peer_feature;
	uint8_t source_stream_type;
	uint8_t sink_tws_mode;
	uint8_t sink_drc_mode;
	uint8_t sink_volume;
};

struct bt_manager_tws_context_t tws_context;

static OS_MUTEX_DEFINE(bt_manager_tws_mutex);

static void bt_manager_tws_process_command(uint8_t *data, uint8_t len);
#ifdef CONFIG_BT_TWS_US281B
static void bt_manager_tws_process_us281b_command(uint8_t *data, uint8_t len);
#endif

static void _bt_manager_irq_work(os_work *work)
{
	struct tws_sync_event_t *temp, *event_item;

	os_mutex_lock(&bt_manager_tws_mutex, OS_FOREVER);

	if (!sys_slist_is_empty(&tws_context.tws_sync_event_list)) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tws_context.tws_sync_event_list, event_item, temp, node) {
			if (event_item && event_item->bt_clock <= btif_tws_get_bt_clock()) {
				struct app_msg  msg = {0};

				msg.type = MSG_SYS_EVENT;
				msg.cmd = event_item->event;
				send_async_msg("main", &msg);
				sys_slist_find_and_remove(&tws_context.tws_sync_event_list, &event_item->node);
				SYS_LOG_INF("event %d bt_clock %d\n", event_item->event, event_item->bt_clock);
				bt_mem_free(event_item);
			}
		}
	}

	os_mutex_unlock(&bt_manager_tws_mutex);
}

static void _bt_manager_tws_add_event(uint32_t event_id, uint32_t bt_clock)
{
	struct tws_sync_event_t *tws_sync_event = NULL;
	struct app_msg  msg = {0};

	if (!bt_manager_config_enable_tws_sync_event()) {
		msg.type = MSG_SYS_EVENT;
		msg.cmd = event_id;
		send_async_msg("main", &msg);
		return;
	}

	SYS_LOG_INF("event %d bt_clock %d\n", event_id, bt_clock);
	os_mutex_lock(&bt_manager_tws_mutex, OS_FOREVER);

	tws_sync_event = bt_mem_malloc(sizeof(struct tws_sync_event_t));
	if (!tws_sync_event) {
		goto exit;
	}

	tws_sync_event->event = event_id;
	tws_sync_event->bt_clock = bt_clock;

	sys_slist_append(&tws_context.tws_sync_event_list, (sys_snode_t *)tws_sync_event);
	SYS_LOG_INF("event %d bt_clock %d\n", event_id, bt_clock);
exit:
	os_mutex_unlock(&bt_manager_tws_mutex);
}

static void bt_manager_tws_stream_type_to_tws_mode(uint8_t stream_type, uint8_t *tws_mode, uint8_t *drc_mode)
{
	switch (stream_type) {
	case AUDIO_STREAM_LOCAL_MUSIC:
		*tws_mode = TWS_MODE_MUSIC_TWS;
		*drc_mode = DRC_MODE_OFF;
		break;
	case AUDIO_STREAM_LINEIN:
		*tws_mode = TWS_MODE_AUX_TWS;
		*drc_mode = DRC_MODE_AUX;
		break;
	default:
		*tws_mode = TWS_MODE_BT_TWS;
		*drc_mode = DRC_MODE_NORMAL;
		break;
	}
}

static void bt_manager_tws_tws_mode_to_stream_type(uint8_t tws_mode, uint8_t drc_mode, uint8_t *stream_type)
{
	switch (tws_mode) {
	case TWS_MODE_AUX_TWS:
		*stream_type = AUDIO_STREAM_LINEIN;
		break;
	case TWS_MODE_MUSIC_TWS:
		*stream_type = AUDIO_STREAM_LOCAL_MUSIC;
		break;
	default:
		*stream_type = AUDIO_STREAM_MUSIC;
		break;
	}
}

static void us281b_sink_start_play(uint8_t *data, int len)
{
	uint8_t cmd[6], stream_type;

	bt_manager_tws_tws_mode_to_stream_type(tws_context.sink_tws_mode, tws_context.sink_drc_mode, &stream_type);
	cmd[0] = TWS_STATUS_EVENT;
	cmd[1] = BT_TWS_START_PLAY;
	cmd[2] = stream_type;                /* exf_stream_type */
	cmd[3] = data[0];
	cmd[4] = data[1];
	cmd[5] = tws_context.sink_volume;
	bt_manager_tws_process_command(cmd, 6);
}

static void bt_manager_set_sink_volume(void)
{
	uint8_t cmd[3];
	uint16_t volume_limit = 0;

	cmd[0] = TWS_SYNC_M2S_UI_SET_VOL_LIMIT_CMD;
	memcpy(&cmd[1], &volume_limit, sizeof(volume_limit));
	btif_tws_send_command(cmd, 3);

#ifdef CONFIG_AUDIO
	int volume = audio_system_get_stream_volume(tws_context.source_stream_type);
	if (volume >= 0) {
		cmd[0] = TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD;
		cmd[1] = (uint8_t)volume;
		btif_tws_send_command(cmd, 2);
	}
#endif

}

static void bt_manager_set_sink_spk_pos(uint8_t pos)
{
       uint8_t cmd[2];

       cmd[0] = TWS_SYNC_M2S_UI_SWITCH_POS_CMD;
       cmd[1] = pos;
       btif_tws_send_command(cmd, 2);
}

/* return, 0: not pair tws device, other pair tws device */
static int bt_manager_tws_discover_check_device(void *param, int param_size)
{
	struct btsrv_discover_result *result = param;
	uint8_t pre_mac[3];

	if (result->name == NULL) {
		return 0;
	}

	if (bt_manager_config_get_tws_compare_high_mac()) {
		bt_manager_config_set_pre_bt_mac(pre_mac);
		if (result->addr.val[5] != pre_mac[0] ||
			result->addr.val[4] != pre_mac[1] ||
			result->addr.val[3] != pre_mac[2]) {
			return 0;
		}
	}

#ifdef CONFIG_PROPERTY
	uint8_t name[33];

	memset(name, 0, sizeof(name));
	property_get(CFG_BT_NAME, name, 32);
	if (strlen(name) != result->len || memcmp(result->name, name, result->len)) {
		return 0;
	}
#endif

	if (bt_manager_config_get_tws_compare_device_id() &&
		memcmp(result->device_id, bt_manager_config_get_device_id(), sizeof(result->device_id))) {
		return 0;
	}

	return 1;
}

static int _bt_manager_tws_event_notify(int event_type, void *param, int param_size)
{
	int ret = 0;
	struct update_version_param *in_param;

	switch (event_type) {
	case BTSRV_TWS_DISCOVER_CHECK_DEVICE:
		ret = bt_manager_tws_discover_check_device(param, param_size);
		break;
	case BTSRV_TWS_DISCONNECTED:
		SYS_LOG_INF("tws disconnected");
		tws_observer = NULL;
		tws_context.peer_version = 0x0;
		system_set_low_latencey_mode((tws_context.record_low_latency ? true : false));
		bt_manager_set_status(BT_STATUS_TWS_UNPAIRED);
		if (!strcmp("tws", app_manager_get_current_app())) {
			sys_event_notify(SYS_EVENT_TWS_DISCONNECTED);
			app_switch_unlock(1);
			app_switch("btmusic", APP_SWITCH_CURR, true);
		}
		if (system_check_low_latencey_mode()) {
		#ifdef CONFIG_BT_BLE
			bt_manager_resume_ble();
		#endif
		}
		break;
	case BTSRV_TWS_CONNECTED:
		SYS_LOG_INF("tws connected");
		tws_observer = btif_tws_get_tws_observer();
		bt_manager_set_status(BT_STATUS_TWS_PAIRED);
		if (btif_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
			app_switch("tws", APP_SWITCH_CURR, true);
			app_switch_lock(1);
		#ifdef CONFIG_POWER
			power_manager_sync_slave_battery_state();
		#endif
		}
		if (system_check_low_latencey_mode()) {
		#ifdef CONFIG_BT_BLE
			bt_manager_halt_ble();
		#endif
		}
		break;
	case BTSRV_TWS_RESTART_PLAY:
		SYS_LOG_INF("tws restart");
		bt_manager_event_notify(BT_REQ_RESTART_PLAY, NULL, 0);
		break;
	case BTSRV_TWS_START_PLAY:
		SYS_LOG_INF("tws start");
		break;
	case BTSRV_TWS_PLAY_SUSPEND:
		SYS_LOG_INF("tws suspend");
		break;
	case BTSRV_TWS_READY_PLAY:
		SYS_LOG_INF("tws ready");
		break;
	case BTSRV_TWS_EVENT_SYNC:
#ifdef CONFIG_BT_TWS_US281B
		bt_manager_tws_process_us281b_command(param, param_size);
#else
		bt_manager_tws_process_command(param, param_size);
#endif
		break;
	case BTSRV_TWS_A2DP_CONNECTED:
		SYS_LOG_INF("a2dp connected");
		break;
	case BTSRV_TWS_HFP_CONNECTED:
		SYS_LOG_INF("hfp connected");
		break;
	case BTSRV_TWS_IRQ_CB:
	{
		/* Be carefull, call back in tws irq context */
		os_work_submit(&tws_context.tws_irq_work);
		break;
	}
	case BTSRV_TWS_UNPROC_PENDING_START_CB:
		SYS_LOG_INF("Pending start");
		bt_manager_event_notify(BT_REQ_RESTART_PLAY, NULL, 0);
		break;
	case BTSRV_TWS_UPDATE_BTPLAY_MODE:
		if (btif_tws_get_dev_role() != BTSRV_TWS_MASTER) {
			break;
		}
		tws_context.slave_actived = *((uint8_t *)param) ? 1: 0;
		if (tws_context.slave_actived) {
			if (!bt_manager_tws_check_is_woodpecker()) {
#ifdef CONFIG_PROPERTY
				int mode = property_get_int("TWS_MODE", MEDIA_OUTPUT_MODE_LEFT);
#else
				int mode = MEDIA_OUTPUT_MODE_LEFT;
#endif
				if(mode == MEDIA_OUTPUT_MODE_LEFT) {
					bt_manager_set_sink_spk_pos(TWS_SPK_RIGHT);
				} else {
					bt_manager_set_sink_spk_pos(TWS_SPK_LEFT);
				}
			}
			bt_manager_set_sink_volume();
			tws_observer = btif_tws_get_tws_observer();
			bt_manager_event_notify(BT_REQ_RESTART_PLAY, NULL, 0);
		} else {
			tws_observer = NULL;
		}
		SYS_LOG_INF("Slave %s actived mode\n", tws_context.slave_actived ? "enter" : "exit");
		break;
	case BTSRV_TWS_UPDATE_PEER_VERSION:
		in_param = param;
		tws_context.peer_version = in_param->versoin;
		tws_context.peer_feature = in_param->feature;
		SYS_LOG_INF("peer version 0x%x, feature 0x%x", tws_context.peer_version, tws_context.peer_feature);

		tws_context.record_low_latency = system_check_low_latencey_mode() ? 1 : 0;
		if (bt_manager_tws_check_support_feature(TWS_FEATURE_LOW_LATENCY)) {
			system_set_low_latencey_mode((tws_context.record_low_latency ? true : false));
		} else {
			system_set_low_latencey_mode(false);
		}
		break;
	case BTSRV_TWS_SINK_START_PLAY:
		us281b_sink_start_play((uint8_t *)param, param_size);
		break;
	case BTSRV_TWS_PAIR_FAILED:
		SYS_LOG_INF("Tws pair failed");
		break;
	case BTSRV_TWS_EXPECT_TWS_ROLE:
		ret = bt_manager_config_expect_tws_connect_role();
		break;
	case BTSRV_TWS_SCO_DATA:
		// TO do
		break;
	default:
		break;
	}

	return ret;
}

static void us281b_event_report_input(uint32_t key_event)
{
	static uint32_t pre_key_type = 0;
	static uint8_t pre_key_value = 0;
	static uint32_t duration = 0;
	uint32_t send_key, key_type, key_value;

	key_type = key_event & KEY_TYPE_ALL;
	key_value = (key_event >> 8) & 0xFF;

	/* ZS285A don't care short down */
	if (key_type == KEY_TYPE_SHORT_DOWN) {
		return;
	}
	/* ZS281B long up is short up, so 285A need convert short up after hold to long up */
	if (key_type == KEY_TYPE_SHORT_UP 
		&& pre_key_value == key_value
		&& pre_key_type == KEY_TYPE_HOLD) {
		key_type = KEY_TYPE_LONG_UP;
	}

	if(pre_key_value == key_value && pre_key_type == key_type 
		&& os_uptime_get_32() - duration < 300) {
		key_type = KEY_TYPE_DOUBLE_CLICK;
	}

	send_key = key_type | key_value;
	SYS_LOG_INF("key 0x%x convert to 0x%x\n", key_event, send_key);
	sys_event_report_input(send_key);
	pre_key_value = key_value;
	pre_key_type = key_type;
	duration = os_uptime_get_32();
}

static void bt_manager_tws_update_tws_mode(uint8_t stream_type)
{
	uint8_t tws_mode, drc_mode;

	bt_manager_tws_stream_type_to_tws_mode(stream_type, &tws_mode, &drc_mode);
	btif_tws_update_tws_mode(tws_mode, drc_mode);
}

static void bt_manager_tws_process_command(uint8_t *data, uint8_t len)
{
	uint8_t event = data[0];
	uint32_t event_param;

	memcpy(&event_param, &data[1], 4);
	SYS_LOG_INF("event %d, event_param 0x%x", event, event_param);

	switch (event) {
	case TWS_INPUT_EVENT:
	{
		if (!bt_manager_tws_check_is_woodpecker()) {
			us281b_event_report_input(event_param);
		} else {
			sys_event_report_input(event_param);
		}
		break;
	}
	case TWS_UI_EVENT:
	{
		uint32_t bt_clock;

		memcpy(&bt_clock, &data[5], 4);
		_bt_manager_tws_add_event(event_param, bt_clock);
		break;
	}
	case TWS_SYSTEM_EVENT:
	{
		sys_event_send_message(event_param);
		break;
	}
	case TWS_VOLUME_EVENT:
	{
		SYS_LOG_INF("media_type %d music_vol %d\n", data[1], data[2]);
#ifdef CONFIG_VOLUME_MANAGER
		system_volume_sync_remote_to_device(data[1], data[2]);
#endif
		break;
	}
	case TWS_BATTERY_EVENT:
	{
	#ifdef CONFIG_POWER
		power_manager_set_slave_battery_state((event_param >> 24) & 0xff, (event_param & 0xffffff));
	#endif
		break;
	}
	case TWS_STATUS_EVENT:
	{
		SYS_LOG_INF("media type %d, codec_id %d sample_rate %d volume %d\n", data[2], data[3], data[4], data[5]);
		bt_manager_event_notify(data[1], data, 6);
		break;
	}
	}
}

#ifdef CONFIG_BT_TWS_US281B
static void bt_manager_tws_process_us281b_command(uint8_t *data, uint8_t len)
{
	uint8_t comvert_data[5], stream_type;
	uint32_t param;

	switch (data[0]) {
	case TWS_SYNC_S2M_UI_UPDATE_BAT_EV:
		comvert_data[0] = TWS_BATTERY_EVENT;
		param = (data[1]&0xF)*(4200000 - 3200000)/10 + 3200000;
		param |= (((data[1]&0xF)*10) << 24);
		memcpy(&comvert_data[1], &param, sizeof(param));
		bt_manager_tws_process_command(comvert_data, (sizeof(param) + 1));
		break;
	case TWS_SYNC_S2M_UI_UPDATE_KEY_EV:
		comvert_data[0] = TWS_INPUT_EVENT;
		memcpy(&comvert_data[1], &data[1], 4);
		bt_manager_tws_process_command(comvert_data, 5);
		break;

	case TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD:
		bt_manager_tws_tws_mode_to_stream_type(tws_context.sink_tws_mode, tws_context.sink_drc_mode, &stream_type);
		tws_context.sink_volume = data[1];
		comvert_data[0] = TWS_VOLUME_EVENT;
		comvert_data[1] = stream_type;
		comvert_data[2] = tws_context.sink_volume;
		bt_manager_tws_process_command(comvert_data, 3);
		break;

	case TWS_SYNC_M2S_UI_SET_VOL_LIMIT_CMD:
		SYS_LOG_INF("sink_vol_limit\n");
		break;

	case TWS_SYNC_M2S_UI_SWITCH_POS_CMD:
		SYS_LOG_INF("switch_pos %d\n", data[1]);
		bt_manager_event_notify(BT_TWS_CHANNEL_MODE_SWITCH, &data[1], 1);
		break;

	case TWS_SYNC_M2S_UI_POWEROFF_CMD:
		comvert_data[0] = TWS_UI_EVENT;
		param = SYS_EVENT_POWER_OFF;
		memcpy(&comvert_data[1], &param, sizeof(param));
		bt_manager_tws_process_command(comvert_data, (sizeof(param) + 1));
		break;

	case TWS_SYNC_M2S_UI_SWITCH_TWS_MODE_CMD:
		tws_context.sink_tws_mode = data[1];
		tws_context.sink_drc_mode = data[2];
		SYS_LOG_INF("sink tws/drc mode %d %d\n", tws_context.sink_tws_mode, tws_context.sink_drc_mode);
		break;
	case TWS_SYNC_M2S_START_PLAYER:
		SYS_LOG_INF("media type %d, codec_id %d sample_rate %d volume %d\n", data[2], data[3], data[4], data[5]);
		bt_manager_event_notify(data[1], data, 6);
		break;
	case TWS_SYNC_M2S_STOP_PLAYER:
		SYS_LOG_INF("stop player \n");
		bt_manager_event_notify(data[1], data, 2);
		break;
	default:
		SYS_LOG_INF("Wait todo cmd_id 0x%x\n", data[0]);
		break;
	}
}
#endif

void bt_manager_tws_send_event(uint8_t event, uint32_t event_param)
{
	uint8_t tws_event_data[5];

	memset(tws_event_data, 0, sizeof(tws_event_data));
	tws_event_data[0] = event;
	memcpy(&tws_event_data[1], &event_param, 4);

	bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
}

void bt_manager_tws_send_event_sync(uint8_t event, uint32_t event_param)
{
	uint8_t tws_event_data[10];
	uint32_t bt_clock = btif_tws_get_bt_clock() + TWS_TIMETO_BT_CLOCK(100);

	memset(tws_event_data, 0, sizeof(tws_event_data));
	tws_event_data[0] = event;
	memcpy(&tws_event_data[1], &event_param, 4);
	memcpy(&tws_event_data[5], &bt_clock, 4);

	bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));

	_bt_manager_tws_add_event(event_param, bt_clock);
}

void bt_manager_tws_notify_start_play(uint8_t media_type, uint8_t codec_id, uint8_t sample_rate)
{
	uint8_t tws_event_data[6];
	bool bt_play;
	bool local_play;

	if (btif_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}

	memset(tws_event_data, 0, sizeof(tws_event_data));
	tws_event_data[0] = TWS_STATUS_EVENT;
	tws_event_data[1] = BT_TWS_START_PLAY;
	tws_event_data[2] = media_type;
	tws_event_data[3] = codec_id;
	tws_event_data[4] = sample_rate;

#ifdef CONFIG_AUDIO
	tws_event_data[5] = audio_system_get_stream_volume(media_type);
#endif

	bt_play = (media_type == AUDIO_STREAM_MUSIC)? true : false;
	local_play = (media_type == AUDIO_STREAM_LOCAL_MUSIC)? true : false;
	btif_tws_set_bt_local_play(bt_play, local_play);
	bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
}

void bt_manager_tws_notify_stop_play(void)
{
	uint8_t tws_event_data[5];

	if (btif_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}

	memset(tws_event_data, 0, sizeof(tws_event_data));
	tws_event_data[0] = TWS_STATUS_EVENT;
	tws_event_data[1] = BT_TWS_STOP_PLAY;

	bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
}

#ifdef CONFIG_BT_TWS_US281B
static void bt_manager_tws_system_event_to_us281b(uint8_t *data, int len)
{
	uint8_t cmd[2];
	uint32_t event;

	memcpy(&event, &data[1], sizeof(event));

	switch (event) {
	case MSG_POWER_OFF:
		cmd[0] = TWS_SYNC_M2S_UI_POWEROFF_CMD;
		cmd[1] = TWS_POWEROFF_KEY;
		btif_tws_send_command(cmd, 2);
		break;
	default:
		SYS_LOG_INF("Wait todo event %d\n", event);
	}
}

void bt_manager_tws_to_us281b_send_command(uint8_t *command, int command_len)
{
	uint8_t cmd[8], event;
	uint32_t param;

	memset(cmd, 0, sizeof(cmd));
	event = command[0];

	switch (event) {
	/* Tws master to slave event */
	case TWS_UI_EVENT:
		/* Current not send tts event to slave */
		break;

	case TWS_VOLUME_EVENT:
		cmd[0] = TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD;
		/* command[1]: media_type, not support send */
		cmd[1] = command[2];
		btif_tws_send_command(cmd, 2);
		break;

	case TWS_SYSTEM_EVENT:
		bt_manager_tws_system_event_to_us281b(command, command_len);
		break;

	case TWS_STATUS_EVENT:
		if (command[1] == BT_TWS_START_PLAY) {
			/* Sync tws mode and volume */
			SYS_LOG_INF("START_PLAY %d,%d,%d,%d\n", command[2], command[3], command[4], command[5]);
			bt_manager_tws_update_tws_mode(command[2]);
			if (!bt_manager_tws_check_support_feature(TWS_FEATURE_UI_STARTSTOP_CMD)) {
				cmd[0] = TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD;
				cmd[1] = command[5];		/* volume */
				btif_tws_send_command_sync(cmd, 2);
			} else {
				command[0] = TWS_SYNC_M2S_START_PLAYER;
				command[1] = BT_TWS_START_PLAY;
				btif_tws_send_command_sync(command, 6);
			}
		} else if (command[1] == BT_TWS_STOP_PLAY) {
			if (bt_manager_tws_check_support_feature(TWS_FEATURE_UI_STARTSTOP_CMD)) {
				command[0] = TWS_SYNC_M2S_STOP_PLAYER;
				command[1] = BT_TWS_STOP_PLAY;
				btif_tws_send_command_sync(command, 2);
			}
		}
		break;

	/* Tws slave to master event */
	case TWS_BATTERY_EVENT:
		memcpy(&param, &command[1], sizeof(param));
		cmd[0] = TWS_SYNC_S2M_UI_UPDATE_BAT_EV;
		cmd[1] = (uint8_t)param;
		btif_tws_send_command(cmd, 2);
		break;

	case TWS_INPUT_EVENT:
		cmd[0] = TWS_SYNC_S2M_UI_UPDATE_KEY_EV;
		memcpy(&cmd[1], &command[1], 4);
		btif_tws_send_command(cmd, 5);
		break;

	default:
		SYS_LOG_INF("Wait todo event: %d\n", event);
		break;
	}
}
#endif

void bt_manager_tws_send_command(uint8_t *command, int command_len)
{
#ifdef CONFIG_BT_TWS_US281B
	bt_manager_tws_to_us281b_send_command(command, command_len);
#else
	btif_tws_send_command(command, command_len);
#endif
}

void bt_manager_tws_send_sync_command(uint8_t *command, int command_len)
{
#ifdef CONFIG_BT_TWS_US281B
	/* Change to 281b command */
#else
	btif_tws_send_command_sync(command, command_len);
#endif
}

void bt_manager_tws_sync_volume_to_slave(uint32_t media_type, uint32_t music_vol)
{
	uint8_t tws_event_data[4];

	if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER) {
		memset(tws_event_data, 0, sizeof(tws_event_data));
		tws_event_data[0] = TWS_VOLUME_EVENT;
		tws_event_data[1] = media_type;
		tws_event_data[2] = music_vol;
		bt_manager_tws_send_command(tws_event_data, sizeof(tws_event_data));
		SYS_LOG_INF("media_type %d music_vol %d\n", media_type, music_vol);
	}
}

void bt_manager_tws_cancel_wait_pair(void)
{
	btif_tws_cancel_wait_pair();
}

void bt_manager_tws_wait_pair(void)
{
	uint8_t i;

	if (btif_tws_is_in_connecting()) {
		btif_br_auto_reconnect_stop();

		i = 10;
		while (i-- > 0) {
			os_sleep(20);
			if (!btif_tws_is_in_connecting()) {
				SYS_LOG_INF("Stop tws connecting\n");
				break;
			}
		}
	}

	if ((bt_manager_get_connected_dev_num() < BT_MANAGER_MAX_BR_NUM) &&
		btif_tws_can_do_pair()) {
		bt_manager_set_status(BT_STATUS_MASTER_WAIT_PAIR);
		btif_tws_wait_pair(TWS_PAIR_TYR_TIMES);
	} else {
		btif_tws_cancel_wait_pair();
	}
}

void bt_manager_tws_disconnect_and_wait_pair(void)
{
	uint16_t i;

	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	if (btif_tws_is_in_connecting()) {
		btif_br_auto_reconnect_stop();

		i = 10;
		while (i-- > 0) {
			os_sleep(20);
			if (!btif_tws_is_in_connecting()) {
				SYS_LOG_INF("Stop tws connecting\n");
				break;
			}
		}
	}

	i = 1000;
	while (i-- > 0) {
		os_sleep(2);
		if (!btif_br_get_connected_device_num()) {
			SYS_LOG_INF("all disconnected \n");
			break;
		}
	}

	bt_manager_set_status(BT_STATUS_MASTER_WAIT_PAIR);
	btif_tws_wait_pair(TWS_PAIR_TYR_TIMES);
}

tws_runtime_observer_t *bt_manager_tws_get_runtime_observer(void)
{

	if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER
		|| btif_tws_get_dev_role() == BTSRV_TWS_SLAVE) {
		return tws_observer;
	}

	return NULL;
}


void bt_manager_tws_init(void)
{
	btif_tws_init(_bt_manager_tws_event_notify);

	memset(&tws_context, 0, sizeof(struct bt_manager_tws_context_t));

	sys_slist_init(&tws_context.tws_sync_event_list);

	os_work_init(&tws_context.tws_irq_work, _bt_manager_irq_work);

	tws_observer = NULL;
}

int bt_manager_tws_get_dev_role(void)
{
	return btif_tws_get_dev_role();
}

uint8_t bt_manager_tws_get_peer_version(void)
{
	return tws_context.peer_version;
}

bool bt_manager_tws_check_is_woodpecker(void)
{
	if (btif_tws_get_dev_role() != BTSRV_TWS_NONE) {
		return IS_WOODPECKER_VERSION(tws_context.peer_version);
	} else {
		return true;
	}
}

bool bt_manager_tws_check_support_feature(uint32_t feature)
{
	if (btif_tws_get_dev_role() != BTSRV_TWS_NONE) {
		if ((get_tws_current_feature() & feature) && (tws_context.peer_feature & feature)) {
			return true;
		} else {
			return false;
		}
	} else {
		return true;
	}
}

void bt_manager_tws_set_stream_type(uint8_t stream_type)
{
	uint8_t cmd[2];
	int volume = 0;

	tws_context.source_stream_type = stream_type;

	/* Event not connected, update to tws service */
	bt_manager_tws_update_tws_mode(stream_type);

	if (btif_tws_get_dev_role() == BTSRV_TWS_MASTER) {
	#ifdef CONFIG_AUDIO
		volume = audio_system_get_stream_volume(stream_type);
	#endif
		if (volume >= 0) {
			cmd[0] = TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD;
			cmd[1] = (uint8_t)volume;
			btif_tws_send_command(cmd, 2);
		}
	}
}

void bt_manager_tws_set_codec(uint8_t codec)
{
	btif_tws_set_codec(codec);
}
void bt_manager_tws_channel_mode_switch(void)
{
#if CONFIG_TWS_AUDIO_OUT_MODE == 0
	uint8_t cmd[2];
#ifdef CONFIG_PROPERTY
	int channel_mode = property_get_int("TWS_MODE", MEDIA_OUTPUT_MODE_LEFT);
#else
	int channel_mode = MEDIA_OUTPUT_MODE_LEFT;
#endif

	if (btif_tws_get_dev_role() != BTSRV_TWS_MASTER) {
		return;
	}

	cmd[0] = TWS_SYNC_M2S_UI_SWITCH_POS_CMD;
	if (channel_mode == MEDIA_OUTPUT_MODE_LEFT) {
		channel_mode = MEDIA_OUTPUT_MODE_RIGHT;
		cmd[1] = MEDIA_OUTPUT_MODE_LEFT;
	} else {
		channel_mode = MEDIA_OUTPUT_MODE_LEFT;
		cmd[1] = MEDIA_OUTPUT_MODE_RIGHT;
	}

	btif_tws_send_command(cmd, 2);

#ifdef CONFIG_PROPERTY
	property_set_int("TWS_MODE", channel_mode);
#endif
#ifdef CONFIG_MEDIA_PLAYER
	media_player_t *player = media_player_get_current_main_player();
	if (player) {
		if (channel_mode == MEDIA_OUTPUT_MODE_LEFT || channel_mode == MEDIA_OUTPUT_MODE_RIGHT) {
			media_player_set_output_mode(player, channel_mode);
		}
	}
	SYS_LOG_INF("%d\n",channel_mode);
#endif
#endif
}
