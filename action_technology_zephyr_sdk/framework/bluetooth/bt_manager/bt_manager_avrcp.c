/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager avrcp profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include "bt_manager_inner.h"
#include "btservice_api.h"
#include "bt_porting_inner.h"

#ifdef CONFIG_MEDIA
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>
#endif

#ifdef CONFIG_BT_AVRCP_GET_ID3
static void _bt_manager_avrcp_msg_cb(struct app_msg *msg, int result, void *not_used)
{
	if (msg && msg->ptr){
		struct bt_id3_info *info = msg->ptr;
		for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
			if(info->item[i].data){
				bt_mem_free(info->item[i].data);
			}
		}
		bt_mem_free(msg->ptr);
	}
}
#endif

static void _bt_manager_avrcp_callback(uint16_t hdl, btsrv_avrcp_event_e event, void *param)
{
	int status;

	switch (event) {
	/** notifying that avrcp connected */
	case BTSRV_AVRCP_CONNECTED:
	{
		SYS_LOG_INF("avrcp connected\n");
		if (bt_manager_get_connected_dev_num() == 1) {
			bt_manager_event_notify(BT_AVRCP_CONNECTION_EVENT, NULL, 0);
		}
	}
	break;
	/** notifying that avrcp disconnected*/
	case BTSRV_AVRCP_DISCONNECTED:
	{
		SYS_LOG_INF("avrcp disconnected\n");
		if (bt_manager_get_connected_dev_num() == 1) {
			bt_manager_event_notify(BT_AVRCP_DISCONNECTION_EVENT, NULL, 0);
		}
	}
	break;
	/** notifying that avrcp stoped */
	case BTSRV_AVRCP_STOPED:
	{
		SYS_LOG_INF("avrcp stoped\n");
		status = BT_STATUS_PAUSED;
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp paused */
	case BTSRV_AVRCP_PAUSED:
	{
		SYS_LOG_INF("avrcp paused\n");
		status = BT_STATUS_PAUSED;
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp playing */
	case BTSRV_AVRCP_PLAYING:
	{
		SYS_LOG_INF("avrcp playing\n");
		status = BT_STATUS_PLAYING;
		bt_manager_event_notify(BT_AVRCP_PLAYBACK_STATUS_CHANGED_EVENT, (void *)&status, 4);
	}
	break;
	/** notifying that avrcp cur track change */
	case BTSRV_AVRCP_TRACK_CHANGE:
	{
		SYS_LOG_INF("avrcp track change\n");
		bt_manager_event_notify(BT_AVRCP_TRACK_CHANGED_EVENT, NULL, 0);
	}
	break;
	/** notifying that id3 info */
#ifdef CONFIG_BT_AVRCP_GET_ID3
	case BTSRV_AVRCP_UPDATE_ID3_INFO:
	{
		SYS_LOG_INF("avrcp id3 info\n");
		struct bt_id3_info info;
		struct bt_id3_info *pinfo = (struct bt_id3_info *)param;
		memcpy(&info,pinfo,sizeof(info));
		for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
			if(info.item[i].len && pinfo->item[i].data){
				info.item[i].data = bt_mem_malloc(info.item[i].len + 1);
				memset(info.item[i].data, 0, info.item[i].len + 1);
				memcpy(info.item[i].data, pinfo->item[i].data, info.item[i].len);
			}
			SYS_LOG_INF("id3 %d %s\n",info.item[i].id,info.item[i].data);
		}
		int ret = bt_manager_event_notify_ext(BT_AVRCP_UPDATE_ID3_INFO_EVENT, (void *)&info,sizeof(info),_bt_manager_avrcp_msg_cb);
		if(ret <= 0){
			for(int i =0 ; i< BT_TOTAL_ATTRIBUTE_ITEM_NUM; i++){
				if(info.item[i].data){
					bt_mem_free(info.item[i].data);
				}
			}
		}

	}
	break;
	case BTSRV_AVRCP_PLAYBACK_POS:
	{
		uint32_t *pos = param;

		if ((*pos) == 0xFFFFFFFF) {
			SYS_LOG_INF("No track currently selected");
		} else {
			SYS_LOG_INF("Playback pos %d ms", *pos);
		}

		bt_manager_event_notify(BT_AVRCP_UPDATE_PLAYBACK_POS, param, sizeof(uint32_t));
	}
	break;
#endif
	default:
		break;
	}
}

#if (defined CONFIG_VOLUME_MANAGER) || (defined CONFIG_BT_AVRCP_VOL_SYNC)
static uint32_t _bt_manager_music_to_avrcp_volume(uint32_t music_vol)
{
	uint32_t  avrcp_vol = 0;
#ifdef CONFIG_AUDIO
	uint32_t  max_volume = audio_policy_get_volume_level();

	if (music_vol == 0) {
		avrcp_vol = 0;
	} else if (music_vol >= max_volume) {
		avrcp_vol = 0x7F;
	} else {
		avrcp_vol = (music_vol * 0x80 / max_volume);
	}
#endif
	return avrcp_vol;
}
#endif

#ifdef CONFIG_VOLUME_MANAGER
static uint32_t _bt_manager_avrcp_to_music_volume(uint32_t avrcp_vol)
{
	uint32_t  music_vol = 0;
	uint32_t  max_volume = audio_policy_get_volume_level();

	if (avrcp_vol == 0) {
		music_vol = 0;
	} else if (avrcp_vol >= 0x7F) {
		music_vol = max_volume;
	} else {
		music_vol = (avrcp_vol + 1) * max_volume / 0x80;
		if (music_vol == 0) {
			music_vol = 1;
		}
	}

	return music_vol;
}
#endif
void _bt_manager_avrcp_get_volume_callback(uint16_t hdl, uint8_t *volume)
{

#ifdef CONFIG_VOLUME_MANAGER
	uint32_t  music_vol = audio_system_get_stream_volume(AUDIO_STREAM_MUSIC);

	*volume = (uint8_t)_bt_manager_music_to_avrcp_volume(music_vol);
#endif

}

void _bt_manager_avrcp_set_volume_callback(uint16_t hdl, uint8_t volume)
{
#ifdef CONFIG_VOLUME_MANAGER
	uint32_t music_vol = (uint8_t)_bt_manager_avrcp_to_music_volume(volume);

	SYS_LOG_INF("avrcp remote set abs vol %d", volume);

	system_volume_sync_remote_to_device(AUDIO_STREAM_MUSIC, music_vol);
#endif
}

void _bt_manager_avrcp_pass_ctrl_callback(uint16_t hdl, uint8_t cmd, uint8_t state)
{
	SYS_LOG_INF("cmd %d\n", cmd);

	if (state) {
#ifdef CONFIG_VOLUME_MANAGER
		uint32_t  music_vol = audio_system_get_stream_volume(AUDIO_STREAM_MUSIC);
		uint32_t  max_volume = audio_policy_get_volume_level();
		if (cmd == BTSRV_AVRCP_CMD_VOLUMEUP) {
			if (music_vol == max_volume)
				return;
			system_volume_sync_remote_to_device(AUDIO_STREAM_MUSIC, music_vol + 1);
		} else if(cmd == BTSRV_AVRCP_CMD_VOLUMEDOWN) {
			if (music_vol == 0)
				return;
			system_volume_sync_remote_to_device(AUDIO_STREAM_MUSIC, music_vol - 1);
		}
#endif
	}
}

#ifdef CONFIG_BT_A2DP_TRS
void _bt_manager_trs_avrcp_set_volume_callback(uint16_t hdl, uint8_t volume)
{
#ifdef CONFIG_VOLUME_MANAGER
	uint32_t music_vol = (uint8_t)_bt_manager_avrcp_to_music_volume(volume);

	SYS_LOG_INF("trs avrcp remote set vol %d", volume);

	system_volume_sync_remote_to_device(AUDIO_STREAM_LOCAL_MUSIC, music_vol);
#endif
}
#endif

static const btsrv_avrcp_callback_t btm_avrcp_ctrl_cb = {
	.event_cb = _bt_manager_avrcp_callback,
	.get_volume_cb = _bt_manager_avrcp_get_volume_callback,
	.set_volume_cb = _bt_manager_avrcp_set_volume_callback,
	.pass_ctrl_cb = _bt_manager_avrcp_pass_ctrl_callback,
#ifdef CONFIG_BT_A2DP_TRS
	.trs_pass_ctrl_cb = bt_manager_trs_avrcp_pass_ctrl_callback,
	.trs_set_volume_cb = _bt_manager_trs_avrcp_set_volume_callback,
#endif
};

int bt_manager_avrcp_profile_start(void)
{
	return btif_avrcp_start((btsrv_avrcp_callback_t *)&btm_avrcp_ctrl_cb);
}

int bt_manager_avrcp_profile_stop(void)
{
	return btif_avrcp_stop();
}

int bt_manager_avrcp_play(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_PLAY);
}

int bt_manager_avrcp_stop(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_STOP);
}

int bt_manager_avrcp_pause(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_PAUSE);
}

int bt_manager_avrcp_play_next(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_FORWARD);
}

int bt_manager_avrcp_play_previous(void)
{
	return btif_avrcp_send_command(BTSRV_AVRCP_CMD_BACKWARD);
}

int bt_manager_avrcp_fast_forward(bool start)
{
	if (start) {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_FORWARD_START);
	} else {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_FORWARD_STOP);
	}
	return 0;
}

int bt_manager_avrcp_fast_backward(bool start)
{
	if (start) {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_BACKWARD_START);
	} else {
		btif_avrcp_send_command(BTSRV_AVRCP_CMD_FAST_BACKWARD_STOP);
	}
	return 0;
}

int bt_manager_avrcp_sync_vol_to_remote(uint32_t music_vol)
{
#ifdef CONFIG_BT_AVRCP
#ifdef CONFIG_BT_AVRCP_VOL_SYNC
	uint32_t  avrcp_vol = _bt_manager_music_to_avrcp_volume(music_vol);

	return btif_avrcp_sync_vol(avrcp_vol);
#else
	return 0;
#endif
#else
    return 0;
#endif
}

#ifdef CONFIG_BT_AVRCP_GET_ID3
int bt_manager_avrcp_get_id3_info(void)
{
	return btif_avrcp_get_id3_info();
}

void bt_manager_avrcp_notify_playback_pos(uint32_t pos)
{
	_bt_manager_avrcp_callback(0, BTSRV_AVRCP_PLAYBACK_POS, &pos);
}

int bt_manager_avrcp_get_playback_pos(void)
{
	/* Response play status with play position */
	return btif_avrcp_get_play_status();
}
#else
int bt_manager_avrcp_get_id3_info(void)
{
	return -EIO;
}

int bt_manager_avrcp_get_playback_pos(void)
{
	return -EIO;
}
#endif

int bt_manager_avrcp_set_absolute_volume(uint8_t dev_type, uint8_t *data, uint8_t len)
{
	return btif_avrcp_set_absolute_volume(dev_type, data, len);
}
