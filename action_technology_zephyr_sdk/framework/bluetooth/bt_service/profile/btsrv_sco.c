/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice sco
 */

#define SYS_LOG_DOMAIN "btsrv_sco"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

/* WAIT TODO: Not ready work with two sco link as the same time  */
//#define DECODE_PLAYBACK_MCU 1

#define BTSPEECH_SCO_PACKET_SIZE			(60)	/* bytes  */
#define SCO_EXCEPT_CACHE_SIZE				(60)
#define SCO_CHECK_DROP_MUTE_FRAME_CNT		(50)	/* Check drop mute frame number */

static btsrv_sco_callback sco_user_callback;

struct btsrv_sco_context_info {
	struct bt_conn *sco_conn;
	uint8_t last_seq_no : 3;
	uint8_t first_frame : 1;
	uint8_t mute_frame : 1;
	uint8_t check_mute_finish : 1;
	uint8_t drop_flag : 1;
	uint8_t sco_rx_last_pkg_status_flag;
	uint16_t sco_rx_pkt_cache_len;
	uint8_t *sco_rx_cache;
	uint16_t frame_cnt;
	uint16_t frame_cached_len;
	uint16_t drop_frame;
	uint16_t sco_except_cache_len;
	uint8_t *sco_except_cache;
};

static struct btsrv_sco_context_info sco_context_info;
static struct btsrv_sco_context_info *sco_context;

static void _btsrv_sco_connected_cb(struct bt_conn *conn, uint8_t err)
{
	if (hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_SCO)
		return;

	/* ref for bt servcie process MSG_BTSRV_SCO_CONNECTED,
	 * need unref conn after process MSG_BTSRV_SCO_CONNECTED.
	 */
	hostif_bt_conn_ref(conn);
	btsrv_event_notify_ext(MSG_BTSRV_SCO, MSG_BTSRV_SCO_CONNECTED, conn, err);
}

static void _btsrv_sco_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	if (hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_SCO)
		return;


	/* ref for bt servcie process MSG_BTSRV_SCO_DISCONNECTED,
	 * need unref conn after process MSG_BTSRV_SCO_DISCONNECTED.
	 */
	hostif_bt_conn_ref(conn);
	btsrv_event_notify_ext(MSG_BTSRV_SCO, MSG_BTSRV_SCO_DISCONNECTED, conn, reason);
}

static void _btsrv_sco_security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (hostif_bt_conn_get_type(conn) != BT_CONN_TYPE_SCO)
		return;
}

static int _btsrv_sco_mscb_seq_convert(uint8_t seq)
{
	const uint8_t msbc_head_flag[4] = {0x08, 0x38, 0xc8, 0xf8};
	int i;

	for (i = 0; i < ARRAY_SIZE(msbc_head_flag); i++) {
		if (seq == msbc_head_flag[i]) {
			return i;
		}
	}
	return -1;
}

#ifdef DECODE_PLAYBACK_MCU
static int _btsrv_sco_insert_package(uint8_t *data, uint8_t len, int num)
{
	SYS_LOG_INF("lost %d packages\n", num);

	while (num > 0) {
		data[0] = 0;
		data[1] = 1;
		sco_user_callback(BTSRV_SCO_DATA_INDICATED, data, BTSPEECH_SCO_PACKET_SIZE);
		sco_context->last_seq_no = (sco_context->last_seq_no & 0x03) + 1;
		num--;
	}
	return 0;
}
#endif

static int _btsrv_find_really_msbc_pkt(uint8_t *data_buf, uint16_t pkt_length)
{
    uint8_t i;
    uint8_t valid_len;
    uint8_t remain_len;

    for(i=3; i < (pkt_length - 2); i++)
    {
        if((data_buf[i] == 0xad) && (data_buf[i+1] == 0x00) && (data_buf[i+2] == 0x00))
        {
            remain_len = i - 2;
            valid_len = pkt_length - i + 2;
            memcpy(sco_context->sco_except_cache + sco_context->sco_except_cache_len, (uint8_t *)&data_buf[i-2], valid_len);
			sco_context->sco_except_cache_len += valid_len;
            //printk("get flag:%d_%d\n", remain_len, valid_len);
            return 0;
        }
    }

    return -1;
}

static int _btsrv_read_msbc_pkt(uint8_t *data, uint16_t pkt_length)
{
    int ret = -1;
	if((data[2] == 0xad) && (data[3] == 0x00) && (data[4] == 0x00))
	{
		sco_context->sco_except_cache_len = 0;
		ret = 0;
		goto exit;
	}

	//printk("wrong head:%d_%d\n", pkt_length, sco_context->sco_except_cache_len);
	if (sco_context->sco_except_cache_len > 0) {
		uint8_t remain_len;
		uint8_t start_len;
		uint8_t temp_buf[120];

		memset(temp_buf, 0, 120);
		start_len = sco_context->sco_except_cache_len;
		remain_len = pkt_length - sco_context->sco_except_cache_len;

		memcpy(temp_buf, sco_context->sco_except_cache, sco_context->sco_except_cache_len);
		sco_context->sco_except_cache_len = 0;

		memcpy(&temp_buf[start_len], data, remain_len);
		//printf("get pkt:%d_%d\n", remain_len, start_len);
		_btsrv_find_really_msbc_pkt(data, pkt_length);
		memcpy(data, temp_buf, pkt_length);
		ret = 0;

		goto exit;
	}

	if (_btsrv_find_really_msbc_pkt(data, pkt_length) < 0) {
		//printk("wrong pkt:%d\n", pkt_length);
		memset(data, 0, pkt_length);
		ret = 0;
	}

exit:
	return ret;
}

static void _btsrv_sco_convert_data_to_media(uint8_t *data, uint8_t len, uint8_t pkg_flag, uint8_t codec_id)
{
	uint16_t seq_no = 0;

	if (len != BTSPEECH_SCO_PACKET_SIZE) {
		SYS_LOG_ERR("len error %d codec_id %d\n", len, codec_id);
		return;
	}

	if (codec_id == BT_CODEC_ID_MSBC) {
		if (pkg_flag) {
			seq_no = (sco_context->last_seq_no & 0x03) + 1;
		} else if (data[1] == 0) {
			pkg_flag = 1;
		} else {
			seq_no = _btsrv_sco_mscb_seq_convert(data[1]) + 1;
		}
	} else {
		seq_no = (sco_context->last_seq_no & 0x03) + 1;
	}
#ifdef DECODE_PLAYBACK_MCU
	if (codec_id == BT_CODEC_ID_MSBC) {
		if (system_check_low_latencey_mode()) {
			if (sco_context->first_frame) {
				_btsrv_sco_insert_package(data, len, 2);
				sco_context->first_frame = 0;
				sco_context->drop_frame = 2;
			} else if(sco_context->drop_frame != 0){
				if(sco_context->drop_flag) {
					sco_context->drop_frame--;
					sco_context->drop_flag = 0;
					return;
				} else {
					sco_context->drop_flag = 1;
				}
			}
		}
		data[0] = 0;
		data[1] = pkg_flag;
		/* print_buffer((void *)data, 1, BTSPEECH_SCO_PACKET_SIZE, 16, -1); */
		sco_user_callback(BTSRV_SCO_DATA_INDICATED, data, BTSPEECH_SCO_PACKET_SIZE);
	} else {
		uint8_t send_data[62];

		send_data[0] = 0;
		send_data[1] = pkg_flag;
		memcpy(&send_data[2], data, BTSPEECH_SCO_PACKET_SIZE);
		/* print_buffer((void *)send_data, 1, BTSPEECH_SCO_PACKET_SIZE + 2, 16, -1); */
		sco_user_callback(BTSRV_SCO_DATA_INDICATED, send_data, BTSPEECH_SCO_PACKET_SIZE + 2);
	}
#else
	if (codec_id == BT_CODEC_ID_MSBC) {
		uint8_t send_data[68];
		uint16_t offset = 0;

		*(uint16_t *)&send_data[offset] = pkg_flag;
		offset += 2;

		*(uint16_t *)&send_data[offset] = sco_context->frame_cnt;
		offset += 2;

		*(uint16_t *)&send_data[offset]= len - 2;
		offset += 2;


		*(uint16_t *)&send_data[offset]= 2;
		offset += 2;

		memcpy(&send_data[offset], &data[2], len - 2);
		/* print_buffer((void *)data, 1, BTSPEECH_SCO_PACKET_SIZE, 16, -1); */
		sco_user_callback(BTSRV_SCO_DATA_INDICATED, send_data, sizeof(send_data));
	} else {
		uint8_t send_data[68];
		uint16_t offset = 0;

		*(uint16_t *)&send_data[offset] = pkg_flag;
		offset += 2;

		*(uint16_t *)&send_data[offset] = sco_context->frame_cnt;
		offset += 2;

		*(uint16_t *)&send_data[offset]= len;
		offset += 2;


		*(uint16_t *)&send_data[offset]= 0;
		offset += 2;

		memcpy(&send_data[offset], data, len);
		//print_buffer((void *)send_data, 1, sizeof(send_data), 16, -1); 
		sco_user_callback(BTSRV_SCO_DATA_INDICATED, send_data, sizeof(send_data));
	}
#endif
	sco_context->last_seq_no = seq_no;
}

static void _btsrv_sco_save_data_to_cache(uint8_t *data, uint8_t len, uint8_t pkg_status_flag, uint8_t codec_id)
{
	if (!sco_context->sco_rx_cache) {
		SYS_LOG_ERR("Drop sco data");
		return;
	}

#if 0
	if (codec_id == BT_CODEC_ID_MSBC) {
		if (sco_context->sco_rx_pkt_cache_len != 0 && _btsrv_sco_mscb_seq_convert(data[1]) >= 0) {
			sco_context->sco_rx_pkt_cache_len = 0;
		} else if (sco_context->sco_rx_pkt_cache_len == 0 &&  _btsrv_sco_mscb_seq_convert(data[1]) < 0) {
			return;
		}
	}
#endif

	if (sco_context->sco_rx_pkt_cache_len != 0) {
		if (pkg_status_flag == 0 && sco_context->sco_rx_last_pkg_status_flag == 0) {
			memcpy(sco_context->sco_rx_cache + sco_context->sco_rx_pkt_cache_len, data, len);
			sco_context->sco_rx_last_pkg_status_flag = pkg_status_flag;
		} else {
			if (sco_context->sco_rx_last_pkg_status_flag == 0) {
				memset(sco_context->sco_rx_cache, 0, sco_context->sco_rx_pkt_cache_len);
			}
			memset(sco_context->sco_rx_cache + sco_context->sco_rx_pkt_cache_len, 0, len);
			sco_context->sco_rx_last_pkg_status_flag = pkg_status_flag;
			pkg_status_flag = 1;
		}
	} else {
		if (pkg_status_flag == 0) {
			memcpy(sco_context->sco_rx_cache + sco_context->sco_rx_pkt_cache_len, data, len);
		} else {
			memset(sco_context->sco_rx_cache, 0, len);
		}
		sco_context->sco_rx_last_pkg_status_flag = pkg_status_flag;
	}

	sco_context->sco_rx_pkt_cache_len += len;

	while (sco_context->sco_rx_pkt_cache_len >= BTSPEECH_SCO_PACKET_SIZE) {
		int ret = 0;
		if (codec_id == BT_CODEC_ID_MSBC) {
			ret = _btsrv_read_msbc_pkt(sco_context->sco_rx_cache, BTSPEECH_SCO_PACKET_SIZE);
			if (ret) {
				memset(sco_context->sco_rx_cache, 0, BTSPEECH_SCO_PACKET_SIZE);
				pkg_status_flag = 1;
				ret = 0;
			}
		}

		if (ret == 0) {
			_btsrv_sco_convert_data_to_media(sco_context->sco_rx_cache, BTSPEECH_SCO_PACKET_SIZE, pkg_status_flag, codec_id);
		}
		sco_context->sco_rx_pkt_cache_len -= BTSPEECH_SCO_PACKET_SIZE;
		memcpy(sco_context->sco_rx_cache, sco_context->sco_rx_cache + BTSPEECH_SCO_PACKET_SIZE, sco_context->sco_rx_pkt_cache_len);
	}
}

#ifdef CONFIG_DEBUG_DATA_RATE
static uint32_t start_time_stamp;

static inline void _btsrv_sco_debug_reset(void)
{
	start_time_stamp = 0;
}

static inline void _btsrv_sco_debug_data_rate(uint8_t len, uint8_t pkg_flag, uint8_t fill_zero)
{
	static int received_data;
	static int frame_cnt;
	static int err_frame_cnt;

	if (!start_time_stamp) {
		frame_cnt = 0;
		err_frame_cnt = 0;
		received_data = 0;
		start_time_stamp = k_cycle_get_32();
	}
	received_data += len;
	frame_cnt++;
	if (pkg_flag && (fill_zero == 0)) {
		err_frame_cnt++;
	}
	if ((k_cycle_get_32() - start_time_stamp) > 5 * sys_clock_hw_cycles_per_sec()) {
		if (start_time_stamp != 0) {
			SYS_LOG_INF("sco data rate: %d b/s frame cnt (%d / %d) error rate %d %%\n",
					received_data / 5, err_frame_cnt, frame_cnt, err_frame_cnt * 100 / frame_cnt);
		}
		frame_cnt = 0;
		err_frame_cnt = 0;
		received_data = 0;
		start_time_stamp = k_cycle_get_32();
	}
}
#endif

static bool _btsrv_sco_check_mute_frame(uint8_t *data, uint8_t len)
{
	if (data[6] == 0 && data[7] == 0 && data[8] == 0 && data[9] == 0) {
		return true;
	}
	return false;
}

static void _btsrv_sco_data_avaliable_cb(struct bt_conn *conn, uint8_t *data, uint8_t len, uint8_t pkg_flag)
{
	uint8_t codec_id = 0, sample_rate = 0, fill_zero = 0;
	struct bt_conn *base_conn = btsrv_rdm_get_base_conn_by_sco(conn);

	if ((btsrv_rdm_hfp_get_actived_sco() != conn && base_conn != btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER)
		 && base_conn != btsrv_rdm_get_tws_by_role(BTSRV_TWS_SLAVE)) || !base_conn) {
		return;
	}

	if(base_conn == btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER) && sco_context->sco_conn){
		btsrv_event_notify_malloc(MSG_BTSRV_TWS, MSG_BTSRV_TWS_SCO_DATA, data, len,len);
		return;
	}

	btsrv_rdm_hfp_get_codec_info(base_conn, &codec_id, &sample_rate);

	if (codec_id != BT_CODEC_ID_MSBC && codec_id != BT_CODEC_ID_CVSD) {
		SYS_LOG_ERR("error codec id %d\n", codec_id);
		return;
	}

	if (conn == btsrv_rdm_hfp_get_actived_sco()) {
		sco_context->frame_cached_len += len;
		if (sco_context->frame_cached_len >= BTSPEECH_SCO_PACKET_SIZE) {
			sco_context->frame_cnt++;
			sco_context->frame_cached_len -= BTSPEECH_SCO_PACKET_SIZE;
		}

		if (sco_context->frame_cnt < SCO_CHECK_DROP_MUTE_FRAME_CNT && !sco_context->check_mute_finish) {
			if (!sco_context->mute_frame) {
				if (_btsrv_sco_check_mute_frame(data, len)) {
					sco_context->mute_frame = 1;
				} else {
					SYS_LOG_DBG("drop frame %d", sco_context->frame_cnt);
					return;
				}
			} else {
				if (!_btsrv_sco_check_mute_frame(data, len)) {
					pkg_flag = 1;
					fill_zero = 1;
					SYS_LOG_INF("fill zero  %d", sco_context->frame_cnt);
				}
			}
		} else {
			sco_context->check_mute_finish = 1;
		}
	}

#ifdef CONFIG_DEBUG_DATA_RATE
	_btsrv_sco_debug_data_rate(len, pkg_flag, fill_zero);
#endif

	if (len == BTSPEECH_SCO_PACKET_SIZE) {
		int ret = 0;

		if (codec_id == BT_CODEC_ID_MSBC) {
			ret = _btsrv_read_msbc_pkt(data, len);
			if (ret) {
				memset(data, 0, len);
				pkg_flag = 1;
				ret = 0;
			}
		}

		if (ret == 0) {
			_btsrv_sco_convert_data_to_media(data, len, pkg_flag, codec_id);
		}
	} else {
		_btsrv_sco_save_data_to_cache(data, len, pkg_flag, codec_id);
	}
}

static struct bt_conn_cb sco_conn_callbacks = {
	.connected = _btsrv_sco_connected_cb,
	.disconnected = _btsrv_sco_disconnected_cb,
	.security_changed = _btsrv_sco_security_changed_cb,
	.rx_sco_data = _btsrv_sco_data_avaliable_cb,
};

static void btsrv_sco_disconnect_timer_handler(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct bt_conn *sco_conn = (struct bt_conn *)expiry_fn_arg;
	struct bt_conn *br_conn = hostif_bt_conn_get_acl_conn_by_sco(sco_conn);
	struct bt_conn *second_br_conn = btsrv_rdm_hfp_get_second_dev();
	int other_state = btsrv_rdm_hfp_get_state(second_br_conn);
	if(br_conn && (br_conn != btsrv_rdm_hfp_get_actived() || 
		second_br_conn == NULL || other_state > BTSRV_HFP_STATE_LINKED)){
		hostif_bt_conn_ref(sco_conn);
		int err = hostif_bt_conn_disconnect(sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			SYS_LOG_INF("Disconnection failed (err %d)\n", err);
		}else
			btsrv_rdm_hfp_set_sco_state(br_conn,BTSRV_SCO_STATE_DISCONNECT);
		hostif_bt_conn_unref(sco_conn);
		SYS_LOG_INF("");
	}
}

void btsrv_sco_disconnect(struct bt_conn *sco_conn){
	uint32_t creat_time = -1,diff;
	struct thread_timer *sco_disconnect_timer = NULL;
	struct bt_conn *br_conn = hostif_bt_conn_get_acl_conn_by_sco(sco_conn);

	if(br_conn){
		btsrv_rdm_get_sco_creat_time(br_conn, &creat_time);
		diff = os_uptime_get_32() - creat_time;
		sco_disconnect_timer = btsrv_rdm_get_sco_disconnect_timer(br_conn);
		if (creat_time > 0 && diff > 500){
			if(sco_disconnect_timer && thread_timer_is_running(sco_disconnect_timer))
				thread_timer_stop(sco_disconnect_timer);
			hostif_bt_conn_ref(sco_conn);
			int err = hostif_bt_conn_disconnect(sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (err) {
				SYS_LOG_INF("Disconnection failed (err %d)\n", err);
			}else
				btsrv_rdm_hfp_set_sco_state(br_conn,BTSRV_SCO_STATE_DISCONNECT);
			hostif_bt_conn_unref(sco_conn);
			SYS_LOG_INF("");
		}else if(sco_disconnect_timer){
			if(!thread_timer_is_running(sco_disconnect_timer)){
				thread_timer_init(sco_disconnect_timer, btsrv_sco_disconnect_timer_handler
					, (void *)sco_conn);
				thread_timer_start(sco_disconnect_timer, 500-diff, 0);
			}
		}
	}
}

static void _btsrv_sco_connected(struct bt_conn *conn)
{
	struct bt_conn *br_conn;
	int hfp_state;

	if (sco_context) {
		br_conn = hostif_bt_conn_get_acl_conn_by_sco(conn);
		if ((br_conn == NULL) || (btsrv_rdm_sco_connected(br_conn, conn) < 0)) {
			return;
		}

		hfp_state = btsrv_rdm_hfp_get_state(br_conn);
		SYS_LOG_INF("other_state %d\n", hfp_state);
		SYS_LOG_INF("allow %d\n", btsrv_allow_sco_connect());

		if (!btsrv_allow_sco_connect()/* || (hfp_state == BTSRV_HFP_STATE_CALL_INCOMING)*/) {
			/* disconnect sco if upper application(ota) not allow */
			hostif_bt_conn_ref(conn);
			btsrv_sco_disconnect(conn);
            btsrv_hfp_rejected(conn);
			btsrv_rdm_set_sco_connected_reject(br_conn, 1);
			return;
		} else {
			btsrv_rdm_set_sco_connected_reject(br_conn, 0);
		}

		if(btsrv_rdm_get_hfp_role(br_conn) == BTSRV_HFP_ROLE_HF)
			btsrv_hfp_set_status(br_conn, BTSRV_HFP_STATE_SCO_ESTABLISHED);
		else
			btsrv_hfp_ag_set_status(br_conn, BTSRV_HFP_STATE_SCO_ESTABLISHED);

		if (br_conn == btsrv_rdm_hfp_get_actived()
			|| br_conn == btsrv_rdm_get_tws_by_role(BTSRV_TWS_SLAVE)) {
			if (!sco_context->sco_conn) {
				hostif_bt_conn_ref(conn);
				sco_context->sco_conn = conn;
			} else {
				btsrv_sco_disconnect(sco_context->sco_conn);
				sco_context->sco_conn = conn;
				hostif_bt_conn_ref(conn);
			}
			sco_context->sco_rx_last_pkg_status_flag = 0;
			sco_context->sco_rx_pkt_cache_len = 0;
			sco_context->first_frame = 1;
			sco_context->drop_flag = 0;
			sco_context->mute_frame = 0;
			sco_context->check_mute_finish = 0;
			sco_context->frame_cnt = 0;
			sco_context->frame_cached_len = 0;
			sco_context->last_seq_no = 0;
			sco_context->sco_except_cache_len = 0;
#ifdef CONFIG_DEBUG_DATA_RATE
			_btsrv_sco_debug_reset();
#endif
		} else {
			/* disconnect sco for deactive dev */
			if (br_conn != btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER)) {
				hostif_bt_conn_ref(conn);
				btsrv_sco_disconnect(conn);
			} else {
				//just ref conn,this conn will be send to tws and save in tws_manager->tws_session->sco_conn
				hostif_bt_conn_ref(conn);
				btsrv_event_notify(MSG_BTSRV_TWS, MSG_BTSRV_SCO_CONNECTED, conn);
			}
		}
	}
}

static void _btsrv_sco_disconnected(struct bt_conn *conn)
{
	struct bt_conn *br_conn = btsrv_rdm_get_base_conn_by_sco(conn);

	if (br_conn == NULL) {
		SYS_LOG_WRN("conn not recorded\n");
		return;
	}

	struct thread_timer *sco_disconnect_timer = btsrv_rdm_get_sco_disconnect_timer(br_conn);
	if(sco_disconnect_timer && thread_timer_is_running(sco_disconnect_timer))
		thread_timer_stop(sco_disconnect_timer);

	if (!btsrv_rdm_is_sco_connected_reject(br_conn)) {
		if(btsrv_rdm_get_hfp_role(br_conn) == BTSRV_HFP_ROLE_HF)
			btsrv_hfp_set_status(br_conn, BTSRV_HFP_STATE_SCO_RELEASED);
		else
			btsrv_hfp_ag_set_status(br_conn, BTSRV_HFP_STATE_SCO_RELEASED);

		if(br_conn == btsrv_rdm_get_tws_by_role(BTSRV_TWS_MASTER)){
			btsrv_event_notify(MSG_BTSRV_TWS, MSG_BTSRV_SCO_DISCONNECTED, conn);
		}
	}

	btsrv_rdm_set_sco_connected_reject(br_conn, 0);
	btsrv_rdm_sco_disconnected(conn);
	hostif_bt_conn_unref(conn);

	if (sco_context && sco_context->sco_conn == conn) {
		sco_context->sco_conn = NULL;
	}
}


static int _btsrv_sco_init(btsrv_sco_callback cb)
{
	sco_context = &sco_context_info;
#if CONFIG_MEDIA
	sco_context->sco_rx_cache = media_mem_get_cache_pool(RX_SCO, AUDIO_STREAM_VOICE);
	sco_context->sco_except_cache = bt_mem_malloc(SCO_EXCEPT_CACHE_SIZE);
	if (!sco_context->sco_except_cache) {
		return -1;
	}
#endif
	hostif_bt_conn_cb_register((struct bt_conn_cb *)&sco_conn_callbacks);
	sco_user_callback = cb;

	return 0;
}

static int _btsrv_sco_deinit(void)
{
	if (sco_context->sco_except_cache) {
		bt_mem_free(sco_context->sco_except_cache);
	}
	sco_context = NULL;
	sco_user_callback = NULL;
	return 0;
}

int btsrv_sco_process(struct app_msg *msg)
{
	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_SCO_START:
		SYS_LOG_INF("MSG_BTSRV_SCO_START\n");
		_btsrv_sco_init(msg->ptr);
		break;
	case MSG_BTSRV_SCO_STOP:
		SYS_LOG_INF("MSG_BTSRV_SCO_STOP\n");
		_btsrv_sco_deinit();
		break;
	case MSG_BTSRV_SCO_CONNECTED:
		SYS_LOG_INF("MSG_BTSRV_SCO_CONNECTED\n");
		_btsrv_sco_connected(msg->ptr);
		hostif_bt_conn_unref(msg->ptr);		/* unref conn for ref in _btsrv_sco_connected_cb */
		break;
	case MSG_BTSRV_SCO_DISCONNECTED:
		SYS_LOG_INF("MSG_BTSRV_SCO_DISCONNECTED\n");
		_btsrv_sco_disconnected(msg->ptr);
		hostif_bt_conn_unref(msg->ptr);		/* unref conn for ref in _btsrv_sco_disconnected_cb */
		break;
	}
	return 0;
}

struct bt_conn *btsrv_sco_get_conn(void)
{
	if (sco_context)
		return sco_context->sco_conn;

	return NULL;
}
