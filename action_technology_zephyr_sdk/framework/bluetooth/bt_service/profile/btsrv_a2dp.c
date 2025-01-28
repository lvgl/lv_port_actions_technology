/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrvice a2dp
 */

#define SYS_LOG_DOMAIN "btsrv_a2dp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

struct btsrv_a2dp_codec_cb {
	struct bt_conn *conn;
	struct bt_a2dp_media_codec codec;
	uint8_t cp_type;
};

static btsrv_a2dp_hdl_callback a2dp_user_callback;
static struct bt_a2dp_endpoint a2dp_sbc_endpoint[CONFIG_MAX_A2DP_ENDPOINT];
static struct bt_a2dp_endpoint a2dp_aac_endpoint[CONFIG_MAX_A2DP_ENDPOINT];
static uint8_t a2dp_register_aac_num;
static struct bt_conn *trs_conn = NULL;

#if 0
static const uint8_t a2dp_sbc_codec[] = {
	BT_A2DP_AUDIO << 4,
	BT_A2DP_SBC,
	0xFF,	/* (SNK mandatory)44100, 48000, mono, dual channel, stereo, join stereo */
			/* (SNK optional) 16000, 32000 */
	0xFF,	/* (SNK mandatory) Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
	0x02,	/* min bitpool */
	0x35	/* max bitpool */
};

static const uint8_t a2dp_aac_codec[] = {
	BT_A2DP_AUDIO << 4,
	BT_A2DP_MPEG2,
	0xF0,	/* MPEG2 AAC LC, MPEG4 AAC LC, MPEG AAC LTP, MPEG4 AAC Scalable */
	0x01,	/* Sampling Frequecy 44100 */
	0x8F,	/* Sampling Frequecy 48000, channels 1, channels 2 */
	0xFF,	/* VBR, bit rate */
	0xFF,	/* bit rate */
	0xFF	/* bit rate */
};
#endif

#define A2DP_AAC_FREQ_NUM		12
const static uint8_t a2dp_aac_freq_table[A2DP_AAC_FREQ_NUM] = {96, 88, 64, 48, 44, 32, 24, 22, 16, 12, 11, 8};

static void btsrv_a2dp_hdl_cb_user(struct bt_conn *conn, btsrv_a2dp_event_e event, void *packet, int size)
{
	if (a2dp_user_callback) {
		a2dp_user_callback(hostif_bt_conn_get_handle(conn), event, packet, size);
	}
}

static uint8_t btsrv_convert_sample_rate(struct bt_a2dp_media_codec *codec)
{
	uint8_t sample_rate = 44, i;
	uint16_t freq;

	if (codec->head.codec_type == BT_A2DP_SBC) {
		switch (codec->sbc.freq) {
		case BT_A2DP_SBC_48000:
			sample_rate = 48;
			break;
		case BT_A2DP_SBC_44100:
			sample_rate = 44;
			break;
		case BT_A2DP_SBC_32000:
			sample_rate = 32;
			break;
		case BT_A2DP_SBC_16000:
			sample_rate = 16;
			break;
		}
	} else if (codec->head.codec_type == BT_A2DP_MPEG2) {
		freq = (codec->aac.freq0 << 4) | codec->aac.freq1;
		for (i = 0; i < A2DP_AAC_FREQ_NUM; i++) {
			if (freq & (BT_A2DP_AAC_96000 << i)) {
				sample_rate = a2dp_aac_freq_table[i];
				break;
			}
		}
	}

	return sample_rate;
}

static uint8_t btsrv_parse_bitpool(struct bt_a2dp_media_codec *codec)
{
	if (codec->head.codec_type == BT_A2DP_SBC) {
		return codec->sbc.max_bitpool;
	} else {
		return 0;
	}
}

static void _btsrv_a2dp_connect_cb(struct bt_conn *conn)
{
	btsrv_event_notify(MSG_BTSRV_CONNECT, MSG_BTSRV_A2DP_CONNECTED, conn);
}

static void _btsrv_a2dp_disconnected_cb(struct bt_conn *conn)
{
	/* TODO: Disconnected process order: btsrv_tws->btsrv_a2dp->btsrv_connect */
	btsrv_event_notify(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_DISCONNECTED, conn);
}

#ifdef CONFIG_DEBUG_DATA_RATE
#define TEST_RECORD_NUM			10
static uint32_t pre_rx_time;
static uint8_t test_index;
static uint16_t test_rx_record[TEST_RECORD_NUM][2];

static inline void _btsrv_a2dp_debug_date_rate(uint16_t len)
{
	static int start_time_stamp;
	static int received_data;
	uint32_t curr_time;

	if (!start_time_stamp) {
		received_data = 0;
		start_time_stamp = k_cycle_get_32();
	}
	received_data += len;
	if ((k_cycle_get_32() - start_time_stamp) > 5 * sys_clock_hw_cycles_per_sec()) {
		if (start_time_stamp != 0) {
			SYS_LOG_INF("a2dp data rate: %d bytes/s\n", received_data / 5);
		}
		received_data = 0;
		start_time_stamp = k_cycle_get_32();
	}

	curr_time = os_uptime_get_32();
	test_rx_record[test_index][0] = curr_time - pre_rx_time;
	test_rx_record[test_index][1] = len;
	test_index++;
	if (test_index >= TEST_RECORD_NUM) {
		test_index = 0;
	}
	pre_rx_time = curr_time;
}

void debug_dump_media_rx_state(void)
{
	static uint32_t pre_dump_time;
	uint32_t curr_time;
	uint16_t time_cnt = 0, len_cnt = 0;
	int flag, i;

	curr_time = os_uptime_get_32();
	if ((curr_time - pre_dump_time) < 500) {
		return;
	}
	pre_dump_time = curr_time;

	flag = btsrv_set_negative_prio();

	printk("diff_time data_len\n");
	for (i = test_index; i < TEST_RECORD_NUM; i++) {
		time_cnt += test_rx_record[i][0];
		len_cnt += test_rx_record[i][1];
		printk("%d(%d)\t %d(%d)\n", test_rx_record[i][0], time_cnt, test_rx_record[i][1], len_cnt);
	}

	for (i = 0; i < test_index; i++) {
		time_cnt += test_rx_record[i][0];
		len_cnt += test_rx_record[i][1];
		printk("%d(%d)\t %d(%d)\n", test_rx_record[i][0], time_cnt, test_rx_record[i][1], len_cnt);
	}

	printk("%d(%d)\n", curr_time - pre_rx_time, time_cnt + (curr_time - pre_rx_time));
	btsrv_revert_prio(flag);
}
#else
void debug_dump_media_rx_state(void)
{
}
#endif

struct avdtp_data_header_t
{
    uint16_t frame_cnt;
    uint16_t seq_no;
    uint16_t frame_len;  //stream len
    uint16_t padding_len;
} __packed;

static uint8_t btsrv_a2dp_pack_date_header(struct bt_conn *conn, uint8_t *data, uint16_t media_len, uint8_t head_len, uint8_t format, uint8_t *padding_len)
{
	struct avdtp_data_header_t header;
	uint8_t pack_header_size = sizeof(struct avdtp_data_header_t);
	uint8_t start_pos;

	/* data start from RTTP */
	if (format == BT_A2DP_SBC) {
		header.frame_cnt = (uint16_t)(data[head_len - 1]&0x0F);
	} else {
		header.frame_cnt = 1;
	}

	header.seq_no = data[3] | ((uint16_t)(data[2]) << 8);
	header.frame_len = media_len;
	header.padding_len = media_len % 2;

	*padding_len = header.padding_len;
	start_pos = head_len - pack_header_size;
	memcpy(&data[start_pos], (uint8_t*)&header, pack_header_size);

#if 1
	static struct bt_conn *rec_seq_conn;
	static uint16_t rec_seq_no;
	static uint32_t rec_time;
	uint32_t curr_time;

	curr_time = os_uptime_get_32();
	if (rec_seq_conn != conn || ((curr_time - rec_time) > 300)) {
		rec_seq_conn = conn;
		rec_seq_no = header.seq_no;
	} else {
		rec_seq_no += 1;
		if (rec_seq_no != header.seq_no) {
			SYS_LOG_ERR("pkt miss %d %d\n", header.seq_no, (rec_seq_no - 1));
		}
		rec_seq_no = header.seq_no;
	}

	rec_time = curr_time;
#endif

	return start_pos;
}

/** this callback dircty call to app, will in bt stack context */
static void _btsrv_a2dp_media_handler_cb(struct bt_conn *conn, uint8_t *data, uint16_t len)
{

	uint8_t head_len, format, sample_rate, cp_type, padding_len;

	if (btsrv_rdm_get_a2dp_pending_ahead_start(conn)) {
		btsrv_a2dp_media_state_change(conn, BT_A2DP_MEDIA_STATE_START);
	}

#ifdef CONFIG_SUPPORT_TWS
	if (btsrv_tws_protocol_data_cb(conn, data, len)) {
		return;
	}
#endif

	/* Must get info after btsrv_tws_protocol_data_cb,
	 * tws slave update format in btsrv_tws_protocol_data_cb
	 */
	btsrv_rdm_a2dp_get_codec_info(conn, &format, &sample_rate, &cp_type);

	switch (format) {
	case BT_A2DP_SBC:
	case BT_A2DP_MPEG2:
		if (!(btsrv_rdm_a2dp_get_actived() == conn ||
			btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE)) {
			SYS_LOG_DBG("return for master role %d\n", btsrv_rdm_get_dev_role());
			break;
		}

		if (format == BT_A2DP_SBC) {
			head_len = AVDTP_SBC_HEADER_LEN;
		} else {
			head_len = AVDTP_AAC_HEADER_LEN;
		}

		if (cp_type == BT_AVDTP_AV_CP_TYPE_SCMS_T) {
			head_len++;
		}

#ifdef CONFIG_DEBUG_DATA_RATE
		_btsrv_a2dp_debug_date_rate(len - head_len);
#endif

		head_len = btsrv_a2dp_pack_date_header(conn, data, (len - head_len), head_len, format, &padding_len);
		btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_DATA_INDICATED, data + head_len, (len - head_len + padding_len));

		break;
	default:
		SYS_LOG_INF("A2dp not support type: 0x%x, 0x%x, %d\n", data[0], data[1], format);
		break;
	}

}

static int _btsrv_a2dp_media_state_req_cb(struct bt_conn *conn, uint8_t state)
{
	btsrv_event_notify_ext(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_MEDIA_STATE_CB, conn, state);
	return 0;
}

static int btsrv_a2dp_media_state_req_proc(struct bt_conn *conn, uint8_t state)
{
	int cmd = -1;
	uint8_t send_notify = 0;
	uint16_t delay_report;

#ifdef CONFIG_BT_A2DP_TRS
	if(btsrv_trs_a2dp_media_state_req_cb(conn, state) == true) {
		SYS_LOG_INF("trs a2dp media state req %d", state);
		if (state == BT_A2DP_MEDIA_STATE_START) {
			btsrv_rdm_trs_a2dp_stream_open(conn, true);
		} else if (state == BT_A2DP_MEDIA_STATE_CLOSE ||
					state == BT_A2DP_MEDIA_STATE_SUSPEND) {
			btsrv_rdm_trs_a2dp_stream_open(conn, false);
		}
		return 0;
	}
#endif

	SYS_LOG_INF("a2dp media state req %d", state);

	switch (state) {
	case BT_A2DP_MEDIA_STATE_OPEN:
		cmd = MSG_BTSRV_A2DP_MEDIA_STATE_OPEN;
		delay_report = 0;
		btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_GET_INIT_DELAY_REPORT, &delay_report, sizeof(delay_report));
		if (delay_report) {
			hostif_bt_a2dp_send_delay_report(conn, delay_report);
		}
		break;
	case BT_A2DP_MEDIA_STATE_START:
		cmd = MSG_BTSRV_A2DP_MEDIA_STATE_START;
		btsrv_rdm_a2dp_actived(conn, 1);
		btsrv_rdm_set_a2dp_pending_ahead_start(conn, 0);
		break;
	case BT_A2DP_MEDIA_STATE_CLOSE:
		cmd = MSG_BTSRV_A2DP_MEDIA_STATE_CLOSE;
		btsrv_rdm_a2dp_actived(conn, 0);
		btsrv_rdm_set_a2dp_pending_ahead_start(conn, 0);
		break;
	case BT_A2DP_MEDIA_STATE_SUSPEND:
		cmd = MSG_BTSRV_A2DP_MEDIA_STATE_SUSPEND;
		btsrv_rdm_a2dp_actived(conn, 0);
		btsrv_rdm_set_a2dp_pending_ahead_start(conn, 0);
		break;
	case BT_A2DP_MEDIA_STATE_PENDING_AHEAD_START:
		btsrv_rdm_set_a2dp_pending_ahead_start(conn, 1);
		break;
	}

	if (cmd > 0) {
		if (btsrv_rdm_a2dp_get_actived() == conn) {
			/* Phone state req */
			send_notify = 1;
		} else if (btsrv_rdm_a2dp_get_actived() &&
			(btsrv_rdm_get_dev_role() == BTSRV_TWS_MASTER)) {
			/* Tws source_restart state req,
			 * only notify when have a2dp active device.
			 */
			send_notify = 1;
		} else if (btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE) {
			/* Tws mater send to slave state req */
			send_notify = 1;
		}

		if ((btsrv_rdm_a2dp_get_actived() == conn || btsrv_rdm_get_dev_role() == BTSRV_TWS_SLAVE) &&
			(cmd == MSG_BTSRV_A2DP_MEDIA_STATE_START)) {
			uint8_t format, sample_rate;
			uint8_t codec_info[3];

			btsrv_rdm_a2dp_get_codec_info(conn, &format, &sample_rate, NULL);
			codec_info[0] = format;
			codec_info[1] = sample_rate;
			codec_info[2] = btsrv_rdm_a2dp_get_bitpool(conn);

			btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_CODEC_INFO, codec_info, sizeof(codec_info));
		}

		if (send_notify) {
			btsrv_event_notify(MSG_BTSRV_A2DP, cmd, conn);
		}
	}

	return 0;
}

static void _btsrv_a2dp_seted_codec_cb(struct bt_conn *conn, struct bt_a2dp_media_codec *codec, uint8_t cp_type)
{
	struct btsrv_a2dp_codec_cb param;

	memset(&param, 0, sizeof(param));
	param.conn = conn;
	memcpy(&param.codec, codec, sizeof(struct bt_a2dp_media_codec));
	param.cp_type = cp_type;
	btsrv_event_notify_malloc(MSG_BTSRV_A2DP, MSG_BTSRV_A2DP_SET_CODEC_CB, (uint8_t *)&param, sizeof(param), 0);
}

static void btsrv_a2dp_seted_codec_proc(void *in_parm)
{
	struct btsrv_a2dp_codec_cb *param = in_parm;
	uint8_t codec_info[3];

	codec_info[0] = param->codec.head.codec_type;
	codec_info[1] = btsrv_convert_sample_rate(&param->codec);
	codec_info[2] = btsrv_parse_bitpool(&param->codec);
	SYS_LOG_INF("media %d, codec %d, freq %d, cp %d\n", param->codec.head.media_type, codec_info[0], codec_info[1], param->cp_type);
	btsrv_rdm_a2dp_set_codec_info(param->conn, codec_info[0], codec_info[1], param->cp_type);
	btsrv_rdm_a2dp_set_bitpool(param->conn, codec_info[2]);

#ifdef CONFIG_BT_A2DP_TRS
	/* Wait TODO: Not ready support a2dp device and TRS concurrently */
	btsrv_a2dp_hdl_cb_user(param->conn, BTSRV_A2DP_CODEC_INFO, codec_info, sizeof(codec_info));
	btsrv_trs_a2dp_seted_codec_cb(param->conn, &param->codec);
#endif
}

static const struct bt_a2dp_app_cb btsrv_a2dp_cb = {
	.connected = _btsrv_a2dp_connect_cb,
	.disconnected = _btsrv_a2dp_disconnected_cb,
	.media_handler = _btsrv_a2dp_media_handler_cb,
	.media_state_req = _btsrv_a2dp_media_state_req_cb,
	.seted_codec = _btsrv_a2dp_seted_codec_cb,
};

int btsrv_a2dp_media_state_change(struct bt_conn *conn, uint8_t state)
{
	return _btsrv_a2dp_media_state_req_cb(conn, state);
}

static void _btsrv_a2dp_connected(struct bt_conn *conn)
{
	char addr_str[BT_ADDR_STR_LEN];
	bd_address_t *addr = GET_CONN_BT_ADDR(conn);

	hostif_bt_addr_to_str((const bt_addr_t *)addr, addr_str, BT_ADDR_STR_LEN);
	SYS_LOG_INF("A2dp connected:%p addr %s\n", conn, addr_str);
	btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_CONNECTED, NULL, 0);
}

static void _btsrv_a2dp_disconnected(struct bt_conn *conn)
{
	char addr_str[BT_ADDR_STR_LEN];
	bd_address_t *addr = GET_CONN_BT_ADDR(conn);

	hostif_bt_addr_to_str((const bt_addr_t *)addr, addr_str, BT_ADDR_STR_LEN);
	SYS_LOG_INF("A2dp disconnected:%p addr %s\n", conn, addr_str);
	btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_DISCONNECTED, NULL, 0);
}

static void _btsrv_a2dp_actived_dev_changed(struct bt_conn *conn)
{
	uint8_t format, sample_rate;
	uint8_t codec_info[3];
	struct bt_conn *second_conn = btsrv_rdm_a2dp_get_second_dev();

	btsrv_a2dp_hdl_cb_user(second_conn, BTSRV_A2DP_STREAM_SUSPEND, NULL, 0);

	btsrv_rdm_a2dp_get_codec_info(conn, &format, &sample_rate, NULL);
	codec_info[0] = format;
	codec_info[1] = sample_rate;
	codec_info[2] = btsrv_rdm_a2dp_get_bitpool(conn);

	btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_CODEC_INFO, codec_info, sizeof(codec_info));
	btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_STREAM_STARED, NULL, 0);
}

static void _btsrv_a2dp_check_state(void)
{
	struct bt_conn *conn = btsrv_rdm_a2dp_get_actived();
	struct bt_conn *second_conn = btsrv_rdm_a2dp_get_second_dev();

	if (btsrv_rdm_is_actived_a2dp_stream_open()) {
		SYS_LOG_INF("a2dp trigger start\n");
		uint8_t format, sample_rate;
		uint8_t codec_info[3];

		btsrv_rdm_a2dp_get_codec_info(conn, &format, &sample_rate, NULL);
		codec_info[0] = format;
		codec_info[1] = sample_rate;
		codec_info[2] = btsrv_rdm_a2dp_get_bitpool(conn);

		btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_CODEC_INFO, codec_info, sizeof(codec_info));
		btsrv_a2dp_hdl_cb_user(conn, BTSRV_A2DP_STREAM_STARED, NULL, 0);
	} else if (second_conn && btsrv_rdm_is_a2dp_stream_open(second_conn)) {
		/* active dev may not update during call,so after call exit,
		 * we should change active dev if current active dev is not
		 * stream opened.
		 */
		btsrv_rdm_a2dp_actived(second_conn, 1);
		SYS_LOG_INF("change active dev to another\n");
	}
}

static void pts_send_delay_report(struct bt_conn *base_conn, uint8_t tws_dev, void *cb_param)
{
	uint32_t delay_time = (uint32_t)cb_param;

	hostif_bt_a2dp_send_delay_report(base_conn, (uint16_t)delay_time);
}

static void btsrv_a2dp_send_delay_report(uint16_t delay_time)
{
	struct bt_conn *conn = btsrv_rdm_a2dp_get_actived();
	uint32_t cb_param = delay_time;

	if (btsrv_is_pts_test() && conn == NULL) {
		btsrv_rdm_get_connected_dev(pts_send_delay_report, (void *)cb_param);
		return;
	}

	if (conn) {
		hostif_bt_a2dp_send_delay_report(conn, delay_time);
	}
}

int btsrv_a2dp_init(struct btsrv_a2dp_start_param *param)
{
	int ret = 0;
	int i = 0, max_register;

	max_register = MIN(CONFIG_MAX_A2DP_ENDPOINT, param->sbc_endpoint_num);
	max_register = (max_register == 1) ? 2 : max_register;		/* Register two endpoint for 1 phone */
	for (i = 0; i < max_register; i++) {
		a2dp_sbc_endpoint[i].info.codec = (struct bt_a2dp_media_codec *)param->sbc_codec;
		a2dp_sbc_endpoint[i].info.a2dp_cp_scms_t = param->a2dp_cp_scms_t;
		a2dp_sbc_endpoint[i].info.a2dp_delay_report = param->a2dp_delay_report;
#ifdef CONFIG_BT_A2DP_SINK
		ret |= hostif_bt_a2dp_register_endpoint(&a2dp_sbc_endpoint[i], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);
#endif
	}

	max_register = MIN(CONFIG_MAX_A2DP_ENDPOINT, param->aac_endpoint_num);
	max_register = (max_register == 1) ? 2 : max_register;		/* Register two endpoint for 1 phone */
	a2dp_register_aac_num = max_register;
	for (i = 0; i < max_register; i++) {
		a2dp_aac_endpoint[i].info.codec = (struct bt_a2dp_media_codec *)param->aac_codec;
		a2dp_aac_endpoint[i].info.a2dp_cp_scms_t = param->a2dp_cp_scms_t;
		a2dp_aac_endpoint[i].info.a2dp_delay_report = param->a2dp_delay_report;
#ifdef CONFIG_BT_A2DP_SINK
		ret |= hostif_bt_a2dp_register_endpoint(&a2dp_aac_endpoint[i], BT_A2DP_AUDIO, BT_A2DP_EP_SINK);
#endif
	}

	if (ret) {
		SYS_LOG_ERR("bt br-a2dp-register failed\n");
		goto err;
	}

	a2dp_user_callback = param->cb;

#ifdef CONFIG_BT_A2DP_TRS
	btsrv_trs_a2dp_init(param);
#endif

	hostif_bt_a2dp_register_cb((struct bt_a2dp_app_cb *)&btsrv_a2dp_cb);

	return 0;
err:
	return -1;
}

int btsrv_a2dp_deinit(void)
{
	hostif_bt_a2dp_register_cb(NULL);
	a2dp_user_callback = NULL;

#ifdef CONFIG_BT_A2DP_TRS
	btsrv_trs_a2dp_deinit();
#endif

	return 0;
}

static int btsrv_a2dp_disable(void)
{
	struct bt_conn *conn;
/** step 1: unregister sdp list*****************************/
#ifdef CONFIG_BT_AVDTP
#ifdef CONFIG_BT_A2DP_SINK
	bt_unregister_a2dp_sink_sdp();
#endif
#if defined(CONFIG_TWS) || defined(CONFIG_BT_A2DP_TRS)
	bt_unregister_a2dp_source_sdp();
#endif
#endif
#ifdef CONFIG_BT_AVRCP
	bt_unregister_avrcp_ct_sdp();
	bt_unregister_avrcp_tg_sdp();
#endif

/*** step 2:  unregister avdtp psm, disable a2dp/avrcp********/
	hostif_bt_a2dp_disable(0);
	hostif_bt_avrcp_disable(0);

/*** step 3:  disconnect a2dp/avrcp***************************/
	conn = btsrv_rdm_a2dp_get_connected_dev();
	if (conn) {
		btsrv_a2dp_disconnect(conn);
	}
	conn = btsrv_rdm_avrcp_get_connected_dev();
	if (conn) {
		btsrv_avrcp_disconnect(conn);
	}
#ifdef CONFIG_BT_A2DP_TRS
	conn = btsrv_rdm_trs_a2dp_get_actived();
	if (conn) {
		btsrv_a2dp_disconnect(conn);
	}
	conn = btsrv_rdm_trs_avrcp_get_actived();
	if (conn) {
		btsrv_avrcp_disconnect(conn);
	}
#endif

	return 0;
}

static int btsrv_a2dp_enable(void)
{
	struct bt_conn *conn;
/** step 1: register sdp list*********************************/
#ifdef CONFIG_BT_AVDTP
#ifdef CONFIG_BT_A2DP_SINK
	bt_register_a2dp_sink_sdp();
#endif
#if defined(CONFIG_TWS) || defined(CONFIG_BT_A2DP_TRS)
	bt_register_a2dp_source_sdp();
#endif
#endif
#ifdef CONFIG_BT_AVRCP
	bt_register_avrcp_ct_sdp();
	bt_register_avrcp_tg_sdp();
#endif
/*** step 2:  register avdtp psm, enable a2dp/avrcp***********/
	hostif_bt_a2dp_enable(0);
	hostif_bt_avrcp_enable(0);
/*** step 3:  connect a2dp/avrcp******************************/
	conn = btsrv_rdm_acl_get_connected_dev();
	if (conn) {
		if (!btsrv_rdm_is_a2dp_connected(conn)) {
			btsrv_a2dp_connect(conn, BT_A2DP_CH_SINK);
		}
		if (!btsrv_rdm_is_avrcp_connected(conn)) {
			btsrv_avrcp_connect(conn);
		}
	}

	return 0;
}

int btsrv_a2dp_disconnect(struct bt_conn *conn)
{
	if (!conn) {
		SYS_LOG_ERR("conn is NULL\n");
		return -EINVAL;
	}

	hostif_bt_a2dp_disconnect(conn);
	SYS_LOG_INF("a2dp_disconnect\n");
	return 0;
}

int btsrv_a2dp_connect(struct bt_conn *conn, uint8_t role)
{
#ifndef CONFIG_BT_A2DP_SINK
	if (role == BT_A2DP_CH_SINK) {
		SYS_LOG_INF("Not config A2DP SINK");
		return 0;
	}
#endif

	if (!hostif_bt_a2dp_connect(conn, role)) {
		SYS_LOG_INF("Connect a2dp\n");
	} else {
		SYS_LOG_ERR("Connect a2dp failed\n");
	}

	return 0;
}

void btsrv_a2dp_halt_aac_endpoint(bool halt)
{
	int ret, i;

	for (i = 0; i < a2dp_register_aac_num; i++) {
		ret = hostif_bt_a2dp_halt_endpoint(&a2dp_aac_endpoint[i], halt);
		SYS_LOG_INF("%s AAC endpoint %d\n", (halt ? "Halt" : "Resume"), ret);
	}
}

int btsrv_a2dp_process(struct app_msg *msg)
{
	struct bt_conn *conn;

#ifdef CONFIG_BT_A2DP_TRS
	if (btsrv_trs_a2dp_process(msg) == true) {
		if (MSG_BTSRV_A2DP_CONNECTED == _btsrv_get_msg_param_cmd(msg)) {
			trs_conn = (struct bt_conn *)(msg->ptr);
		} else if (MSG_BTSRV_A2DP_DISCONNECTED == _btsrv_get_msg_param_cmd(msg)) {
			trs_conn = NULL;
		}
		SYS_LOG_INF("trs proc offset %d", (_btsrv_get_msg_param_cmd(msg) - MSG_BTSRV_A2DP_START));
		return 0;
	}
#endif

	switch (_btsrv_get_msg_param_cmd(msg)) {
	case MSG_BTSRV_A2DP_START:
		SYS_LOG_INF("btsrv a2dp start");
		btsrv_a2dp_init(msg->ptr);
		break;
	case MSG_BTSRV_A2DP_STOP:
		SYS_LOG_INF("btsrv a2dp stop");
		btsrv_a2dp_deinit();
		break;
	case MSG_BTSRV_A2DP_DISABLE:
		SYS_LOG_INF("btsrv a2dp disable");
		btsrv_a2dp_disable();
		break;
	case MSG_BTSRV_A2DP_ENABLE:
		SYS_LOG_INF("btsrv a2dp enable");
		btsrv_a2dp_enable();
		break;
	case MSG_BTSRV_A2DP_CONNECT_TO:
		SYS_LOG_INF("btsrv a2dp connect");
		conn = btsrv_rdm_find_conn_by_addr(msg->ptr);
		if (conn) {
			btsrv_a2dp_connect(conn, (_btsrv_get_msg_param_reserve(msg) ? BT_A2DP_CH_SOURCE : BT_A2DP_CH_SINK));
		}
		break;
	case MSG_BTSRV_A2DP_DISCONNECT:
		SYS_LOG_INF("btsrv a2dp disconnect");
		conn = btsrv_rdm_find_conn_by_addr(msg->ptr);
		if (conn) {
			btsrv_a2dp_disconnect(conn);
		}
		break;
	case MSG_BTSRV_A2DP_CONNECTED:
		SYS_LOG_INF("btsrv a2dp connected");
		_btsrv_a2dp_connected(msg->ptr);
		break;
	case MSG_BTSRV_A2DP_DISCONNECTED:
		SYS_LOG_INF("btsrv a2dp disconnected");
		if (btsrv_rdm_a2dp_get_actived() == (struct bt_conn *)(msg->ptr) ||
			btsrv_rdm_get_conn_role(msg->ptr) == BTSRV_TWS_SLAVE) {
			/* Device with media start, without media suspend/close buf direct
			 * a2dp disconnect, need send media close to upper layer to stop music play.
			 */
			btsrv_a2dp_hdl_cb_user(msg->ptr, BTSRV_A2DP_STREAM_CLOSED, NULL, 0);
		}
		_btsrv_a2dp_disconnected(msg->ptr);
		btsrv_event_notify(MSG_BTSRV_CONNECT, _btsrv_get_msg_param_cmd(msg), msg->ptr);
		break;
	case MSG_BTSRV_A2DP_MEDIA_STATE_CB:
		btsrv_a2dp_media_state_req_proc(msg->ptr, _btsrv_get_msg_param_reserve(msg));
		break;
	case MSG_BTSRV_A2DP_SET_CODEC_CB:
		btsrv_a2dp_seted_codec_proc(msg->ptr);
		break;
	case MSG_BTSRV_A2DP_MEDIA_STATE_OPEN:
		btsrv_a2dp_hdl_cb_user(msg->ptr, BTSRV_A2DP_STREAM_OPENED, NULL, 0);
		break;
	case MSG_BTSRV_A2DP_MEDIA_STATE_START:
		btsrv_a2dp_hdl_cb_user(msg->ptr, BTSRV_A2DP_STREAM_STARED, NULL, 0);
		break;
	case MSG_BTSRV_A2DP_MEDIA_STATE_CLOSE:
		btsrv_a2dp_hdl_cb_user(msg->ptr, BTSRV_A2DP_STREAM_CLOSED, NULL, 0);
		break;
	case MSG_BTSRV_A2DP_MEDIA_STATE_SUSPEND:
		btsrv_a2dp_hdl_cb_user(msg->ptr, BTSRV_A2DP_STREAM_SUSPEND, NULL, 0);
		break;
	case MSG_BTSRV_A2DP_ACTIVED_DEV_CHANGED:
		_btsrv_a2dp_actived_dev_changed(msg->ptr);
		break;
	case MSG_BTSRV_A2DP_CHECK_STATE:
		_btsrv_a2dp_check_state();
		break;
	case MSG_BTSRV_A2DP_SEND_DELAY_REPORT:
		btsrv_a2dp_send_delay_report((uint16_t)_btsrv_get_msg_param_value(msg));
		break;
	default:
		break;
	}
	return 0;
}

int btsrv_a2dp_trs_conn_get(void)
{
	return (int)trs_conn;
}

