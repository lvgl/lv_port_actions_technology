/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt srv hfp api interface
 */

#define SYS_LOG_DOMAIN "btif_hfp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_hfp_register_processer(void)
{
	int ret = 0;

	ret |= btsrv_register_msg_processer(MSG_BTSRV_HFP, &btsrv_hfp_process);
	ret |= btsrv_register_msg_processer(MSG_BTSRV_SCO, &btsrv_sco_process);
	return ret;
}

int btif_hfp_start(btsrv_hfp_callback cb)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_START, cb);
}

int btif_hfp_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_STOP, NULL);
}

int btif_sco_start(btsrv_sco_callback cb)
{
	return btsrv_function_call(MSG_BTSRV_SCO, MSG_BTSRV_SCO_START, cb);
}

int btif_sco_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_SCO, MSG_BTSRV_SCO_STOP, NULL);
}

int btif_hfp_hf_dial_number(uint8_t *number)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_DIAL_NUM, number);
}

int btif_hfp_hf_dial_last_number(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_DIAL_LAST_NUM, NULL);
}

int btif_hfp_hf_switch_sound_source(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_SWITCH_SOUND_SOURCE, NULL);
}

int btif_hfp_hf_dial_memory(int location)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_DIAL_MEMORY, (void *)location);
}

int btif_hfp_hf_volume_control(uint8_t type, uint8_t volume)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_VOLUME_CONTROL, (void *)((type << 16) | volume));
}

int btif_hfp_hf_accept_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_ACCEPT_CALL, NULL);
}

int btif_hfp_hf_reject_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_REJECT_CALL, NULL);
}

int btif_hfp_hf_hangup_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_HANGUP_CALL, NULL);
}

int btif_hfp_hf_battery_report(uint8_t mode, uint8_t bat_val)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_BATTERY_REPORT, (void *)((mode << 16) | bat_val));
}

int btif_hfp_hf_battery_hprec_report(uint8_t bat_val)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_BATTERY_HPREC_REPORT, (void *)(0xFF & bat_val));
}

int btif_hfp_hf_hangup_another_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_HANGUP_ANOTHER_CALL, NULL);
}

int btif_hfp_hf_holdcur_answer_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_HOLDCUR_ANSWER_CALL, NULL);
}

int btif_hfp_hf_hangupcur_answer_call(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_HANGUPCUR_ANSWER_CALL, NULL);
}

int btif_hfp_hf_voice_recognition_start(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_START, NULL);
}

int btif_hfp_hf_voice_recognition_stop(void)
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_VOICE_RECOGNITION_STOP, NULL);
}

int btif_hfp_hf_send_at_command(uint8_t *command,uint8_t active_call)
{
	return btsrv_function_call_malloc(MSG_BTSRV_HFP, MSG_BTSRV_HFP_HF_VOICE_SEND_AT_COMMAND, command,strlen(command),active_call);
}

int btif_hfp_hf_get_time()
{
	return btsrv_function_call(MSG_BTSRV_HFP, MSG_BTSRV_HFP_GET_TIME, NULL);
}

int btif_hfp_hf_get_call_state(uint8_t active_call,uint8_t *call_state)
{
	int state, flags;

	flags = btsrv_set_negative_prio();
	state = btsrv_hfp_get_call_state(active_call,call_state);
	btsrv_revert_prio(flags);
	return state;
}

void btif_hfp_get_active_mac(bd_address_t *addr)
{
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_rdm_get_hfp_acitve_mac(addr);
	btsrv_revert_prio(flags);
}

bool btif_hfp_active_phone_service_ready(void)
{
	int flags;
	uint8_t service;

	flags = btsrv_set_negative_prio();
	service = btsrv_rdm_get_hfp_service();
	btsrv_revert_prio(flags);

	return service ? true : false;
}
