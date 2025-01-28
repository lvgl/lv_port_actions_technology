/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager hfp ag profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>
#include <media_type.h>
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
#include <audio_policy.h>
#include <audio_system.h>
#include <volume_manager.h>

struct bt_manager_hfp_ag_info_t {
	uint8_t hfp_codec_id;
	uint8_t hfp_sample_rate;
	uint32_t hfp_status;
	uint8_t connected:1;
	uint8_t call:1;
	uint8_t call_setup:1;
	uint8_t call_held:1;
};

static struct bt_manager_hfp_ag_info_t hfp_ag_manager;

int bt_manager_hfp_ag_send_event(uint8_t *event, uint16_t len)
{
	return btif_hfp_ag_send_event(event,len);
}

static void _bt_manager_hfp_ag_callback(btsrv_hfp_event_e event, uint8_t *param, int param_size)
{
	switch (event) {
	case BTSRV_HFP_CONNECTED:
	{
		hfp_ag_manager.connected = 1;
		SYS_LOG_INF("hfp ag connected\n");
		break;
	}
	case BTSRV_HFP_DISCONNECTED:
	{
		hfp_ag_manager.connected = 0;
		SYS_LOG_INF("hfp ag disconnected\n");
		break;
	}

	default:
		break;
	}
}

int bt_manager_hfp_ag_update_indicator(enum btsrv_hfp_hf_ag_indicators index, uint8_t value)
{
	if(index == BTSRV_HFP_AG_CALL_IND)
		hfp_ag_manager.call = value;
	else if(index == BTSRV_HFP_AG_CALL_SETUP_IND)
		hfp_ag_manager.call_setup = value;
	else if(index == BTSRV_HFP_AG_CALL_HELD_IND)
		hfp_ag_manager.call_held = value;
	return btif_hfp_ag_update_indicator(index,value);
}

int bt_manager_hfp_ag_start_call_out(void){
	if(!hfp_ag_manager.connected)
		return -EINVAL;
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,2);//out going
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,3);

	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,1);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);//on going

	//creat sco
	return 0;
}

int bt_manager_hfp_ag_profile_start(void)
{
	btif_hfp_ag_start(&_bt_manager_hfp_ag_callback);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SERVICE_IND,1);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_SETUP_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_CALL_HELD_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_SINGNAL_IND,4);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_ROAM_IND,0);
	bt_manager_hfp_ag_update_indicator(BTSRV_HFP_AG_BATTERY_IND,5);//to do
	return 0;
}

int bt_manager_hfp_ag_profile_stop(void)
{
	return btif_hfp_ag_stop();
}

int bt_manager_hfp_ag_init(void)
{
	memset(&hfp_ag_manager, 0, sizeof(struct bt_manager_hfp_ag_info_t));
	return 0;
}

