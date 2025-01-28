/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lccal player
 */

#define LOG_MODULE_CUSTOMER

#include <ringbuff_stream.h>
#include <os_common_api.h>

#include "bt_transmit.h"

LOG_MODULE_DECLARE(bt_transmit, LOG_LEVEL_INF);
#ifdef CONFIG_LCMUSIC_APP
extern void lcmusic_play_or_pause(void);
extern void lcmusic_play_next(void);
extern void lcmusic_play_prev(void);
extern void lcmusic_restart_play(void);
extern void lcmusic_set_volume(int volume);
#endif
void bt_transmit_bt_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("bt_transmit: bt cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case BT_TRS_INQUIRY_START_EVENT:
	{
		bt_transmit_inquiry_start(msg->ptr);
		break;
	}

	case BT_TRS_INQUIRY_RESTART_EVENT:
	{
		bt_transmit_inquiry_restart(msg->ptr);
		break;
	}

	case BT_TRS_INQUIRY_STOP_EVENT:
	{
		bt_transmit_inquiry_stop();
		break;
	}

	case BT_TRS_A2DP_STREAM_READY_EVENT: /* a2dp connected, 226 */
	{
		SYS_LOG_INF("STREAM_READY\n");
		bt_transmit_capture_set_ready(true);
		bt_transmit_capture_stop_inner();
		bt_transmit_capture_start_inner();
		break;
	}

	case BT_TRS_A2DP_STREAM_CLOSE_EVENT: /* a2dp disconnected, 236 */
	{
		SYS_LOG_INF("STREAM_CLOSE\n");
		bt_transmit_capture_set_ready(false);
		bt_transmit_capture_stop_inner();
		break;
	}

	case BT_TRS_A2DP_STREAM_START_EVENT: /* invoked by bt_manager_trs_stream_enable(true) ? */
	{
		SYS_LOG_INF("STREAM_START\n");
		//bt_transmit_capture_start_inner();
		break;
	}

	case BT_TRS_A2DP_STREAM_SUSPEND_EVENT: /* invoked by bt_manager_trs_stream_enable(false) ? */
	{
		SYS_LOG_INF("STREAM_SUSPEND\n");
		//lcmusic_restart_play();
		break;
	}

	case BT_TRS_AVRCP_PLAY_EVENT: /* 232 */
	case BT_TRS_AVRCP_PAUSE_EVENT: /* 233 */
	{
#ifdef CONFIG_LCMUSIC_APP
		lcmusic_play_or_pause();
#endif
		break;
	}

	case BT_TRS_AVRCP_FORWARD_EVENT:
	{
#ifdef CONFIG_LCMUSIC_APP
		lcmusic_play_next();
#endif
		break;
	}

	case BT_TRS_AVRCP_BACKWARD_EVENT:
	{
#ifdef CONFIG_LCMUSIC_APP
		lcmusic_play_prev();
#endif
		break;
	}

	case BT_RMT_VOL_SYNC_EVENT:
	{
#ifdef CONFIG_LCMUSIC_APP
		lcmusic_set_volume(msg->value);
#endif
		break;
	}

	default:
		break;
	}
}

