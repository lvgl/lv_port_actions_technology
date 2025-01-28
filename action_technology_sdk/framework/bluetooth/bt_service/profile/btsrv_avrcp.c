/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice avrcp
 */
#define SYS_LOG_DOMAIN "btsrv_avrcp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define AVRCP_CTRL_PASS_CHECK_INTERVAL			5		/* 5ms */
#define AVRCP_WAIT_ACCEPT_PUSH_TIMEOUT			500		/* 500ms */
#define AVRCP_CTRL_PASS_DELAY_RELEASE_TIME		5		/* 5ms */
#define AVRCP_GET_POS_TIMEOUT					2000	/* 2000ms */

enum {
	AVRCP_PASS_STATE_IDLE,
	AVRCP_PASS_STATE_WAIT_ACCEPT_PUSH,
	AVRCP_PASS_STATE_WAIT_DELAY_RELEASE,
	AVRCP_PASS_STATE_CONTINUE_START,
};

static btsrv_avrcp_callback_t *avrcp_user_callback;
static struct thread_timer sync_volume_timer;
static struct thread_timer ctrl_pass_timer;

struct btsrv_avrcp_stack_cb {
	struct bt_conn *conn;
	union {
		uint8_t event_id;
		uint8_t op_id;
		uint8_t cmd;
	};
	uint8_t status;
	uint32_t pos;
	uint32_t len;
};

static void _btsrv_avrcp_ctrl_connected_cb(struct bt_conn *conn)
{
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_AVRCP_CONNECTED, conn);
}

static void _btsrv_avrcp_ctrl_disconnected_cb(struct bt_conn *conn)
{
	/* TODO: Disconnected process order: btsrv_tws->btsrv_avrcp->btsrv_connect */
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_AVRCP_DISCONNECTED, conn);
}

static void _btsrv_avrcp_ctrl_event_notify_cb(struct bt_conn *conn, uint8_t event_id, uint8_t status)
{
	struct btsrv_avrcp_stack_cb param;

	param.conn = conn;
	param.event_id = event_id;
	param.status = status;
	btsrv_event_notify_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_NOTIFY_CB, (uint8_t *)&param, sizeof(param), 0);
}

static void btsrv_avrcp_ctrl_event_notify_proc(void *in_param)
{
	int cb_ev = -1;
	struct btsrv_avrcp_stack_cb *param = in_param;
	struct bt_conn *conn = param->conn;
	uint8_t event_id = param->event_id;
	uint8_t status = param->status;
#ifdef CONFIG_BT_A2DP
	uint32_t start_time = 0;
	uint32_t delay_time = btsrv_volume_sync_delay_ms();
#endif

	SYS_LOG_INF("Avrcp event notify 0x%x %d %d", hostif_bt_conn_get_handle(conn), event_id, status);

	if (event_id != BT_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED &&
		event_id != BT_AVRCP_EVENT_VOLUME_CHANGED &&
		event_id != BT_AVRCP_EVENT_TRACK_CHANGED) {
		SYS_LOG_WRN("avrcp is NULL or event_id %d not care\n", event_id);
		return;
	}

#ifdef CONFIG_BT_A2DP
	if (event_id == BT_AVRCP_EVENT_VOLUME_CHANGED &&
		((btsrv_rdm_a2dp_get_actived() == conn &&
		btsrv_rdm_is_a2dp_stream_open(conn))
#ifdef CONFIG_BT_A2DP_TRS
		||	btsrv_rdm_get_trs_mode(conn)
#endif
		)) {
#ifdef CONFIG_BT_A2DP_TRS
		if (btsrv_rdm_get_trs_mode(conn)) {
			if (avrcp_user_callback->trs_set_volume_cb)
				avrcp_user_callback->trs_set_volume_cb(hostif_bt_conn_get_handle(conn), status);
		} 
		else
#endif
		{
			btsrv_rdm_get_a2dp_start_time(conn, &start_time);
			if (start_time && (os_uptime_get_32() - start_time) > delay_time
				 && !thread_timer_is_running(&sync_volume_timer)) {
				avrcp_user_callback->set_volume_cb(hostif_bt_conn_get_handle(conn), status);
			}
		}
		return;
	}
#else
	if (event_id == BT_AVRCP_EVENT_VOLUME_CHANGED &&
		btsrv_rdm_avrcp_get_actived() == conn){
		avrcp_user_callback->set_volume_cb(hostif_bt_conn_get_handle(conn), status);
		return;
	}
#endif
	else if (event_id == BT_AVRCP_EVENT_TRACK_CHANGED){
		cb_ev = BTSRV_AVRCP_TRACK_CHANGE;
	}
	else if(event_id == BT_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED) {
		switch (status) {
		case BT_AVRCP_PLAYBACK_STATUS_STOPPED:
			cb_ev = BTSRV_AVRCP_STOPED;
			break;
		case BT_AVRCP_PLAYBACK_STATUS_PLAYING:
			cb_ev = BTSRV_AVRCP_PLAYING;
			break;
		case BT_AVRCP_PLAYBACK_STATUS_PAUSED:
			cb_ev = BTSRV_AVRCP_PAUSED;
			break;
		case BT_AVRCP_PLAYBACK_STATUS_FWD_SEEK:
		case BT_AVRCP_PLAYBACK_STATUS_REV_SEEK:
		case BT_AVRCP_PLAYBACK_STATUS_ERROR:
			break;
		}
	}

	if (cb_ev > 0 && btsrv_rdm_avrcp_get_actived() == conn) {
		avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(conn), cb_ev, NULL);
	}
}

static void _btsrv_avrcp_ctrl_get_volume_cb(struct bt_conn *conn, uint8_t *volume)
{
	avrcp_user_callback->get_volume_cb(hostif_bt_conn_get_handle(conn), volume);
	SYS_LOG_INF("Avrcp remote reg notify get vol %d", (*volume));
}

static void _btsrv_avrcp_ctrl_pass_ctrl_cb(struct bt_conn *conn, uint8_t op_id, uint8_t state)
{
	struct btsrv_avrcp_stack_cb param;

	param.conn = conn;
	param.op_id = op_id;
	param.status = state;
	btsrv_event_notify_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_PASS_CTRL_CB, (uint8_t *)&param, sizeof(param), 0);
}

static void _btsrv_avrcp_get_play_state_cb(struct bt_conn *conn, uint8_t cmd,
						uint32_t *song_len, uint32_t *song_pos, uint8_t *play_state)
{
	struct btsrv_avrcp_stack_cb param;

	if (cmd) {
		/* Receive get play state command, set output value. */
		//*song_len = 0x1234;
		//*song_pos = 0x1111;
		//*play_state = 1;
		return;
	}

	param.conn = conn;
	param.cmd = cmd,
	param.status = (*play_state);
	param.len = (*song_len);
	param.pos = (*song_pos);
	btsrv_event_notify_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_PLAY_STATE_CB, (uint8_t *)&param, sizeof(param), 0);
}

static void btsrv_avrcp_play_state_proc(void *in_param)
{
	struct btsrv_avrcp_stack_cb *param = in_param;
	struct bt_conn *conn = param->conn;
	uint32_t pos = param->pos;

	if (btsrv_rdm_avrcp_get_getting_pos_time(conn) == 1) {
		btsrv_rdm_avrcp_set_getting_pos_time(conn, 0);
		if (btsrv_rdm_avrcp_get_actived() == conn) {
			avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(conn), BTSRV_AVRCP_PLAYBACK_POS, &pos);
		}
	}

}

static void btsrv_avrcp_ctrl_rsp_pass_proc(struct bt_conn *conn, uint8_t op_id, uint8_t state)
{
	struct btsrv_rdm_avrcp_pass_info *info;

	if (state != BT_AVRCP_RSP_STATE_PASS_THROUGH_PUSHED) {
		return;
	}

	info = btsrv_rdm_avrcp_get_pass_info(conn);
	if (!info) {
		return;
	}

	if (info->op_id == op_id && info->pass_state == AVRCP_PASS_STATE_WAIT_ACCEPT_PUSH) {
		info->pass_state = AVRCP_PASS_STATE_WAIT_DELAY_RELEASE;
		info->op_time = os_uptime_get_32();
	}
}

static void btsrv_avrcp_ctrl_pass_ctrl_proc(void *in_param)
{
	struct btsrv_avrcp_stack_cb *param = in_param;
	struct bt_conn *conn = param->conn;
	uint8_t op_id = param->op_id;
	uint8_t state = param->status;

#ifdef CONFIG_BT_A2DP_TRS
	if (btsrv_trs_avrcp_ctrl_pass_ctrl_cb(conn, op_id, state) == true) {
		SYS_LOG_INF("trs op_id 0x%x state 0x%x", op_id, state);
		return;
	}
#endif

	SYS_LOG_INF("op_id 0x%x state 0x%x", op_id, state);

	if (state == BT_AVRCP_RSP_STATE_PASS_THROUGH_PUSHED ||
		state == BT_AVRCP_RSP_STATE_PASS_THROUGH_RELEASED) {
		btsrv_avrcp_ctrl_rsp_pass_proc(conn, op_id, state);
	} else if (btsrv_rdm_avrcp_get_actived() == conn) {
		uint8_t cmd;
		switch(op_id){
		case AVRCP_OPERATION_ID_PLAY:
			cmd = BTSRV_AVRCP_CMD_PLAY;
			break;
		case AVRCP_OPERATION_ID_PAUSE:
			cmd = BTSRV_AVRCP_CMD_PAUSE;
			break;
		case AVRCP_OPERATION_ID_VOLUME_UP:
			cmd = BTSRV_AVRCP_CMD_VOLUMEUP;
			break;
		case AVRCP_OPERATION_ID_VOLUME_DOWN:
			cmd = BTSRV_AVRCP_CMD_VOLUMEDOWN;
			break;
		case AVRCP_OPERATION_ID_MUTE:
			cmd = BTSRV_AVRCP_CMD_MUTE;
			break;
		default:
			SYS_LOG_ERR("op_id 0x%x not support", op_id);
			return;
		}
		avrcp_user_callback->pass_ctrl_cb(hostif_bt_conn_get_handle(conn), cmd, state);
	}
}

static void _btsrv_avrcp_ctrl_update_id3_info_cb(struct bt_conn *conn, struct id3_info * info)
{
    if (btsrv_rdm_avrcp_get_actived() == conn) {
        avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(conn), BTSRV_AVRCP_UPDATE_ID3_INFO, info);
    }
}

static void _btsrv_avrcp_ctrl_playback_pos_cb(struct bt_conn *conn, uint32_t pos)
{
	struct btsrv_avrcp_stack_cb param;

	param.conn = conn;
	param.pos = pos;
	btsrv_event_notify_malloc(MSG_BTSRV_AVRCP, MSG_BTSRV_AVRCP_PLAYBACK_POS_CB, (uint8_t *)&param, sizeof(param), 0);
}

static void btsrv_avrcp_ctrl_playback_pos_proc(void *in_param)
{
	struct btsrv_avrcp_stack_cb *param = in_param;
	struct bt_conn *conn = param->conn;
	uint32_t pos = param->pos;

	if (btsrv_rdm_avrcp_get_getting_pos_time(conn) == 1) {
		btsrv_rdm_avrcp_set_getting_pos_time(conn, 0);
		if (btsrv_rdm_avrcp_get_actived() == conn) {
			avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(conn), BTSRV_AVRCP_PLAYBACK_POS, &pos);
		}
	}
}

static const struct bt_avrcp_app_cb btsrv_avrcp_ctrl_cb = {
	.connected = _btsrv_avrcp_ctrl_connected_cb,
	.disconnected = _btsrv_avrcp_ctrl_disconnected_cb,
	.notify = _btsrv_avrcp_ctrl_event_notify_cb,
	.pass_ctrl = _btsrv_avrcp_ctrl_pass_ctrl_cb,
	.get_play_status = _btsrv_avrcp_get_play_state_cb,
	.get_volume = _btsrv_avrcp_ctrl_get_volume_cb,
	.update_id3_info = _btsrv_avrcp_ctrl_update_id3_info_cb,
	.playback_pos = _btsrv_avrcp_ctrl_playback_pos_cb,
};

static void btsrv_avrcp_ctrl_pass_timer_start_stop(bool start)
{
	if (start) {
		if (!thread_timer_is_running(&ctrl_pass_timer)) {
			thread_timer_start(&ctrl_pass_timer, AVRCP_CTRL_PASS_CHECK_INTERVAL, AVRCP_CTRL_PASS_CHECK_INTERVAL);
		}
	} else {
		if (thread_timer_is_running(&ctrl_pass_timer)) {
			thread_timer_stop(&ctrl_pass_timer);
		}
	}
}

static void connected_dev_cb_check_ctrl_pass(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param)
{
	int *need_timer = cb_param;
	struct btsrv_rdm_avrcp_pass_info *info;
	uint32_t time, check_timeout = 0;

	if (tws_dev) {
		return;
	}

	info = btsrv_rdm_avrcp_get_pass_info(base_conn);
	if (!info) {
		SYS_LOG_ERR("not connected??\n");
		return;
	}

	if (info->op_id == 0 ||
		info->pass_state == AVRCP_PASS_STATE_IDLE ||
		info->pass_state == AVRCP_PASS_STATE_CONTINUE_START) {
		return;
	}

	*need_timer = 1;
	time = os_uptime_get_32();

	if (info->pass_state == AVRCP_PASS_STATE_WAIT_ACCEPT_PUSH) {
		check_timeout = AVRCP_WAIT_ACCEPT_PUSH_TIMEOUT;
	} else if (info->pass_state == AVRCP_PASS_STATE_WAIT_DELAY_RELEASE) {
		check_timeout = AVRCP_CTRL_PASS_DELAY_RELEASE_TIME;
	}

	if ((time - info->op_time) > check_timeout) {
		hostif_bt_avrcp_ct_pass_through_cmd(base_conn, info->op_id, false);
		info->pass_state = AVRCP_PASS_STATE_IDLE;
		info->op_id = 0;
	}
}

static void btsrv_avrcp_ctrl_pass_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	int need_timer = 0;

	btsrv_rdm_get_connected_dev(connected_dev_cb_check_ctrl_pass, &need_timer);
	if (!need_timer) {
		btsrv_avrcp_ctrl_pass_timer_start_stop(false);
	}
}

#define AVRCP_SYNC_TO_REMOTE_INTERVAL	300
static uint32_t avrcp_pre_sync_vol_time;

static void btsrv_avrcp_sync_volume_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	uint8_t volume;
	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	avrcp_user_callback->get_volume_cb(hostif_bt_conn_get_handle(avrcp_conn), &volume);
	SYS_LOG_INF("Avrcp sync vol local to remote vol %d", volume);
	avrcp_pre_sync_vol_time = os_uptime_get_32();

	//if (!btsrv_is_pts_test()) {
		hostif_bt_avrcp_tg_notify_change(avrcp_conn, volume);
	//}
}

int btsrv_avrcp_init(btsrv_avrcp_callback_t *cb)
{
	hostif_bt_avrcp_cttg_register_cb((struct bt_avrcp_app_cb *)&btsrv_avrcp_ctrl_cb);
	avrcp_user_callback = cb;

	thread_timer_init(&sync_volume_timer, btsrv_avrcp_sync_volume_timer_handler, NULL);
	thread_timer_init(&ctrl_pass_timer, btsrv_avrcp_ctrl_pass_timer_handler, NULL);

#ifdef CONFIG_BT_A2DP_TRS
	btsrv_trs_avrcp_init(cb);
#endif
	return 0;
}

int btsrv_avrcp_deinit(void)
{
	btsrv_avrcp_ctrl_pass_timer_start_stop(false);
	if (thread_timer_is_running(&sync_volume_timer)) {
		thread_timer_stop(&sync_volume_timer);
	}

#ifdef CONFIG_BT_A2DP_TRS
	btsrv_trs_avrcp_deinit();
#endif
	return 0;
}

int btsrv_avrcp_disconnect(struct bt_conn *conn)
{
	if (conn && btsrv_rdm_is_avrcp_connected(conn)) {
		SYS_LOG_INF("avrcp_disconnect\n");
		hostif_bt_avrcp_cttg_disconnect(conn);
	}

#ifdef CONFIG_BT_A2DP_TRS
	struct thread_timer *trs_discover_timer = btsrv_rdm_get_trs_discover_timer(conn);
	if(trs_discover_timer && thread_timer_is_running(trs_discover_timer))
		thread_timer_stop(trs_discover_timer);
#endif
	return 0;
}

int btsrv_avrcp_connect(struct bt_conn *conn)
{
	int ret = 0;

	if (!hostif_bt_avrcp_cttg_connect(conn)) {
		SYS_LOG_INF("Connect avrcp\n");
		ret = 0;
	} else {
		SYS_LOG_ERR("Connect avrcp failed\n");
		ret = -1;
	}

	return ret;
}

int btsrv_avrcp_sync_vol(void)
{
	uint32_t start_time = 0, diff, curr_time;
	uint32_t delay_time = btsrv_volume_sync_delay_ms();

	if (thread_timer_is_running(&sync_volume_timer)) {
		return -1;
	}

	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	curr_time = os_uptime_get_32();
	btsrv_rdm_get_a2dp_start_time(avrcp_conn, &start_time);
	diff = curr_time - start_time;
	if (diff >= delay_time) {
		diff = curr_time - avrcp_pre_sync_vol_time;
		if (diff >= AVRCP_SYNC_TO_REMOTE_INTERVAL) {
			btsrv_avrcp_sync_volume_timer_handler(NULL, NULL);
		} else {
			thread_timer_start(&sync_volume_timer, (AVRCP_SYNC_TO_REMOTE_INTERVAL - diff), 0);
		}
	}  else {
		thread_timer_start(&sync_volume_timer, (delay_time - diff), 0);
	}

	return 0;
}

static int btsrv_avrcp_ct_pass_through_cmd(struct bt_conn *conn,
										uint8_t opid, bool continue_cmd, bool push)
{
	int ret = 0;
	struct btsrv_rdm_avrcp_pass_info *info;

	info = btsrv_rdm_avrcp_get_pass_info(conn);
	if (!info) {
		return -EIO;
	}

	if (continue_cmd) {
		if ((push && info->pass_state != AVRCP_PASS_STATE_IDLE) ||
			(!push && info->pass_state != AVRCP_PASS_STATE_CONTINUE_START)) {
			SYS_LOG_INF("Pass busy %d op_id 0x%x", info->pass_state, info->op_id);
			ret = -EBUSY;
			goto pass_exit;
		}

		if (push) {
			info->pass_state = AVRCP_PASS_STATE_CONTINUE_START;
			info->op_id = opid;
		} else {
			info->pass_state = AVRCP_PASS_STATE_IDLE;
			info->op_id = 0;
		}

		ret = hostif_bt_avrcp_ct_pass_through_cmd(conn, opid, push);
	} else {
		if (info->pass_state != AVRCP_PASS_STATE_IDLE) {
			SYS_LOG_INF("Pass busy %d op_id 0x%x", info->pass_state, info->op_id);
			ret = -EBUSY;
			goto pass_exit;
		}

		info->pass_state = AVRCP_PASS_STATE_WAIT_ACCEPT_PUSH;
		info->op_id = opid;
		info->op_time = os_uptime_get_32();
		btsrv_avrcp_ctrl_pass_timer_start_stop(true);

		ret = hostif_bt_avrcp_ct_pass_through_cmd(conn, opid, push);
	}

pass_exit:
	return ret;
}

static int  _btsrv_avrcp_controller_process(btsrv_avrcp_cmd_e cmd)
{
	uint8_t op_id = 0;
	bool push = true, continue_cmd = false;
	int status = 0;
	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	switch (cmd) {
	case BTSRV_AVRCP_CMD_PLAY:
		op_id = AVRCP_OPERATION_ID_PLAY;
		break;
	case BTSRV_AVRCP_CMD_PAUSE:
		op_id = AVRCP_OPERATION_ID_PAUSE;
		break;
	case BTSRV_AVRCP_CMD_STOP:
		op_id = AVRCP_OPERATION_ID_STOP;
		break;
	case BTSRV_AVRCP_CMD_FORWARD:
		op_id = AVRCP_OPERATION_ID_FORWARD;
		break;
	case BTSRV_AVRCP_CMD_BACKWARD:
		op_id = AVRCP_OPERATION_ID_BACKWARD;
		break;
	case BTSRV_AVRCP_CMD_VOLUMEUP:
		op_id = AVRCP_OPERATION_ID_VOLUME_UP;
		break;
	case BTSRV_AVRCP_CMD_VOLUMEDOWN:
		op_id = AVRCP_OPERATION_ID_VOLUME_DOWN;
		break;
	case BTSRV_AVRCP_CMD_MUTE:
		op_id = AVRCP_OPERATION_ID_MUTE;
		break;
	case BTSRV_AVRCP_CMD_FAST_FORWARD_START:
		op_id = AVRCP_OPERATION_ID_FAST_FORWARD;
		continue_cmd = true;
		push = true;
		break;
	case BTSRV_AVRCP_CMD_FAST_FORWARD_STOP:
		op_id = AVRCP_OPERATION_ID_FAST_FORWARD;
		continue_cmd = true;
		push = false;
		break;
	case BTSRV_AVRCP_CMD_FAST_BACKWARD_START:
		op_id = AVRCP_OPERATION_ID_REWIND;
		continue_cmd = true;
		push = true;
		break;
	case BTSRV_AVRCP_CMD_FAST_BACKWARD_STOP:
		op_id = AVRCP_OPERATION_ID_REWIND;
		continue_cmd = true;
		push = false;
		break;
	case BTSRV_AVRCP_CMD_REPEAT_SINGLE:
	case BTSRV_AVRCP_CMD_REPEAT_ALL_TRACK:
	case BTSRV_AVRCP_CMD_REPEAT_OFF:
	case BTSRV_AVRCP_CMD_SHUFFLE_ON:
	case BTSRV_AVRCP_CMD_SHUFFLE_OFF:
	default:
		SYS_LOG_ERR("cmd 0x%02x not support\n", cmd);
		return -EINVAL;
	}

	status = btsrv_avrcp_ct_pass_through_cmd(avrcp_conn, op_id, continue_cmd, push);
	if (status < 0) {
		SYS_LOG_ERR("0x%x failed status %d conn 0x%x",
					cmd, status, hostif_bt_conn_get_handle(avrcp_conn));
	} else {
		SYS_LOG_INF("0x%x ok conn 0x%x", cmd, hostif_bt_conn_get_handle(avrcp_conn));
	}

	return status;
}

static int _btsrv_avrcp_get_id3_info()
{
	int status = 0;
	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	if(!avrcp_conn) {
		return -EINVAL;
	}

	status = hostif_bt_avrcp_ct_get_id3_info(avrcp_conn);
	return status;
}

bool btsrv_avrcp_is_support_get_playback_pos(void)
{
	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	if(!avrcp_conn) {
		return false;
	}

	return hostif_bt_avrcp_ct_check_event_support(avrcp_conn, BT_AVRCP_EVENT_PLAYBACK_POS_CHANGED);
}

static int btsrv_avrcp_get_playback_pos(void)
{
	int status = 0;
	struct bt_conn *avrcp_conn = btsrv_rdm_avrcp_get_actived();

	if(!avrcp_conn) {
		return -EINVAL;
	}

	if (btsrv_rdm_avrcp_get_getting_pos_time(avrcp_conn)) {
		return -EBUSY;
	}

	status = hostif_bt_avrcp_ct_get_playback_pos(avrcp_conn);
	if (status == 0) {
		btsrv_rdm_avrcp_set_getting_pos_time(avrcp_conn, 1);
	}
	return status;
}

static int btsrv_avrcp_set_absolute_volume(uint32_t param)
{
	union {
		uint8_t c_param[4];		/* 0:dev_type, 1:len, 2~3:data */
		int32_t i_param;
	} value;
	struct bt_conn *avrcp_conn = NULL;

	value.i_param = (int32_t)param;
	if (value.c_param[0] == BTSRV_DEVICE_PLAYER) {
#ifdef CONFIG_BT_A2DP_TRS
		avrcp_conn = btsrv_rdm_trs_avrcp_get_actived();
#endif
	} else if (value.c_param[0] == BTSRV_DEVICE_PHONE) {
		avrcp_conn = btsrv_rdm_avrcp_get_actived();
	}

	if(!avrcp_conn) {
		return -EIO;
	}

	return hostif_bt_avrcp_ct_set_absolute_volume(avrcp_conn, (uint32_t)value.i_param);
}

static int btsrv_avrcp_get_play_status(void)
{
	int status;
	struct bt_conn *br_conn = btsrv_rdm_avrcp_get_connected_dev();

	if (br_conn == NULL) {
		return -EIO;
	}

	if (btsrv_rdm_avrcp_get_getting_pos_time(br_conn)) {
		return -EBUSY;
	}

	status = hostif_bt_avrcp_ct_get_play_status(br_conn);
	if (status == 0) {
		btsrv_rdm_avrcp_set_getting_pos_time(br_conn, 1);
	}

	return status;
}

#ifdef CONFIG_BT_A2DP_TRS
static void btsrv_a2dp_trs_discover_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct bt_conn *br_conn = (struct bt_conn *)expiry_fn_arg;
	int ret;

	SYS_LOG_INF("trs discover timer.");
	if ((!br_conn) && (!btsrv_rdm_get_trs_mode(br_conn))) {
		SYS_LOG_ERR("conn invalid.");
		return;
	}

	if (btsrv_rdm_is_a2dp_connected(br_conn)) {
		SYS_LOG_INF("a2dp already init.");
		return;
	}

	ret = bt_a2dp_discover(br_conn, BT_A2DP_CH_SOURCE);
	SYS_LOG_INF("ret %d.",ret);
}
#endif


int btsrv_avrcp_process(struct app_msg *msg)
{
	struct bt_conn *conn;

#ifdef CONFIG_BT_A2DP_TRS
	int trs_cmd = _btsrv_get_msg_param_cmd(msg);
	struct thread_timer *trs_discover_timer;

	if (MSG_BTSRV_AVRCP_CONNECTED == trs_cmd) {
		conn = (struct bt_conn *)(msg->ptr);
		if (conn && btsrv_rdm_get_trs_mode(conn)) {
			bt_pts_avrcp_ct_register_notification(conn);
			if (!btsrv_rdm_is_a2dp_connected(conn)) {
				trs_discover_timer = btsrv_rdm_get_trs_discover_timer(conn);
				if(trs_discover_timer){
					if(!thread_timer_is_running(trs_discover_timer)){
						thread_timer_init(trs_discover_timer, btsrv_a2dp_trs_discover_timer_handler
							, (void *)conn);
						thread_timer_start(trs_discover_timer, 1000, 0);
					}
				}
			}
		}
	} else if (MSG_BTSRV_AVRCP_DISCONNECTED == trs_cmd) {
		conn = (struct bt_conn *)(msg->ptr);
		if (conn && btsrv_rdm_get_trs_mode(conn)) {
			trs_discover_timer = btsrv_rdm_get_trs_discover_timer(conn);
			if (trs_discover_timer && thread_timer_is_running(trs_discover_timer))
				thread_timer_stop(trs_discover_timer);
		}
	}
#endif

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_AVRCP_START:
		SYS_LOG_INF("btsrv avrcp start");
		btsrv_avrcp_init(msg->ptr);
		break;
	case MSG_BTSRV_AVRCP_STOP:
		SYS_LOG_INF("btsrv avrcp stop");
		btsrv_avrcp_deinit();
		break;
	case MSG_BTSRV_AVRCP_CONNECT_TO:
		SYS_LOG_INF("btsrv avrcp connect");
		conn = btsrv_rdm_find_conn_by_addr(msg->ptr);
		if (conn) {
			btsrv_avrcp_connect(conn);
		}
		break;
	case MSG_BTSRV_AVRCP_DISCONNECT:
		SYS_LOG_INF("btsrv avrcp disconnect");
		conn = btsrv_rdm_find_conn_by_addr(msg->ptr);
		if (conn) {
			btsrv_avrcp_disconnect(conn);
		}
		break;

	case MSG_BTSRV_AVRCP_SEND_CMD:
		SYS_LOG_INF("btsrv avrcp send cmd %d\n", msg->value);
		_btsrv_avrcp_controller_process(msg->value);
		break;
	case MSG_BTSRV_AVRCP_GET_ID3_INFO:
		SYS_LOG_INF("btsrv avrcp ID3 info");
		_btsrv_avrcp_get_id3_info();
		break;
	case MSG_BTSRV_AVRCP_GET_PLAYBACK_POS:
		SYS_LOG_INF("btsrv avrcp get playback pos");
		btsrv_avrcp_get_playback_pos();
		break;
	case MSG_BTSRV_AVRCP_SET_ABSOLUTE_VOLUME:
		SYS_LOG_INF("btsrv avrcp set abs volume 0x%x\n", msg->value);
		btsrv_avrcp_set_absolute_volume((uint32_t)msg->value);
		break;
	case MSG_BTSRV_AVRCP_CONNECTED:
		avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(msg->ptr), BTSRV_AVRCP_CONNECTED, NULL);
		break;
	case MSG_BTSRV_AVRCP_DISCONNECTED:
		avrcp_user_callback->event_cb(hostif_bt_conn_get_handle(msg->ptr), BTSRV_AVRCP_DISCONNECTED, NULL);
		break;
	case MSG_BTSRV_AVRCP_NOTIFY_CB:
		btsrv_avrcp_ctrl_event_notify_proc(msg->ptr);
		break;
	case MSG_BTSRV_AVRCP_PASS_CTRL_CB:
		btsrv_avrcp_ctrl_pass_ctrl_proc(msg->ptr);
		break;
	case MSG_BTSRV_AVRCP_PLAYBACK_POS_CB:
		btsrv_avrcp_ctrl_playback_pos_proc(msg->ptr);
		break;
	case MSG_BTSRV_AVRCP_SYNC_VOLUME:
		SYS_LOG_INF("btsrv avrcp sync volume");
		if (btsrv_rdm_get_dev_role() != BTSRV_TWS_SLAVE) {
			btsrv_avrcp_sync_vol();
		}
		break;
	case MSG_BTSRV_AVRCP_GET_PLAY_STATE:
		SYS_LOG_INF("btsrv avrcp get play state");
		btsrv_avrcp_get_play_status();
		break;
	case MSG_BTSRV_AVRCP_PLAY_STATE_CB:
		btsrv_avrcp_play_state_proc(msg->ptr);
		break;
	default:
		break;
	}

	return 0;
}
