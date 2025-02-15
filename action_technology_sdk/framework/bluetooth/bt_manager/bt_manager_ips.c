/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt ble manager.
 */
#define SYS_LOG_DOMAIN "btmgr_ips"

#include <os_common_api.h>

#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <msg_manager.h>
#include <mem_manager.h>
#include <acts_bluetooth/host_interface.h>
#include <bt_manager.h>
#include "bt_manager_inner.h"
#include <sys_event.h>
#include "bt_porting_inner.h"
#include <drivers/hrtimer.h>
#include <board_cfg.h>
#include <bt_manager_ips.h>

#define BT_EXT_ADV_SID_BROADCAST 2
#define BT_IPS_EXT_ADV_LEN_MAX	0x30

#define BT_DATA_VS_IPS_CODEC	0xDF
#define BT_IPS_LC3_FRAME_LEN	(40)
#define BT_IPS_LC3_FRAME_NUM_MAX	(4)
#define BT_IPS_OPUS_FRAME_LEN	(40)
#define BT_IPS_AMR_FRAME_LEN	(40)
#define BT_IPS_CHANNEL_MAX (BT_IPS_SUBS_MAX-1)

#define IPS_SERIVCE_UUID	0xFCCF // Notice: Modification not allowed
#define IPS_SERIVCE_VERSION	0x0100 // ver 1.0

#define BT_IPS_NULL_HEAD_TYPE	(0x1)
#define BT_IPS_DATA_HEAD_TYPE	(0x2)

#define BT_IPS_SEARCH_ADV		(0x0)
#define BT_IPS_START_ADV		(0x1)
#define BT_IPS_RESTART_ADV		(0x2)

//#define BT_IPS_NULL_HEAD_LEN	(7)

struct ips_pa_st {
	bool per_adv_found;
	bt_addr_le_t per_addr;
	uint8_t pa_pending_release;
	uint8_t per_sid;
	uint8_t pa_synced : 1;
	uint8_t role : 2;
	uint8_t pa_comp : 1;
	uint8_t subs_number : 3;
	uint8_t res : 1;

	struct bt_le_per_adv_sync *adv_sync;
	uint8_t seq_pa; //
	uint8_t first_pkg_num; // 上一次接收包的起始包序号
	uint8_t cur_pkg_num; //

	char sync_name[BT_IPS_NAME_MAX];
	uint8_t rx_codec; //PA的编码格式
	uint8_t comp_ratio; // 编码压缩比
	uint8_t ch_mode; //单声道或双声道
	uint8_t samp_freq; //采样率8KHZ or 16KHZ
	uint8_t bit_width; // 位宽8bits or 16bits
};

struct ips_search_value_st {
	uint8_t inuse;
	uint8_t role;
	bt_addr_le_t s_addr;
	char s_name[BT_IPS_NAME_MAX];
};

struct ips_search_st {
	ips_search_cb *s_cb;
	uint8_t cur_role;
	uint32_t match_id; // 匹配码, 4bytes 随机数
	uint8_t searching;
	uint16_t seq;
	char cur_name[BT_IPS_NAME_MAX];
	struct bt_le_ext_adv *s_adv;
	struct ips_search_value_st v_st[BT_IPS_CHANNEL_MAX];
	bt_addr_le_t local_addr;
};

struct ips_mgr_st {
	uint32_t match_id; // 匹配码, 4bytes 随机数
	uint8_t ch_max; //最多允许接收的音频通道数量
	char pa_name[BT_IPS_NAME_MAX];
	bt_addr_le_t local_addr;

	bt_addr_t subs_mac[BT_IPS_SUBS_MAX];
	uint8_t subs_number : 3;
	uint8_t cnt_subscriber : 3;
	uint8_t adv_role : 2;
	// adv
	uint8_t tx_codec; //本机广播的编码格式
	uint8_t comp_ratio; // 编码压缩比
	uint8_t ch_mode; //单声道或双声道
	uint8_t samp_freq; //采样率8KHZ or 16KHZ
	uint8_t bit_width; // 位宽8bits or 16bits
	uint8_t advertising;
	uint8_t pending_release;
	struct bt_le_ext_adv *ips_adv;
	uint8_t per_seq; //
	uint8_t f_seq_start; //
	uint8_t f_seq_end; //
	uint8_t last_f_num;
	uint32_t last_time;
	//uint16_t local_adv_10ms;
	//uint16_t remote_adv_10ms;
	uint8_t adv_enable : 1;
	uint8_t pa_enable : 1;
	uint8_t pa_comp : 1;
	uint8_t adv_work_exist : 1;
	uint8_t refer_num;
	uint16_t adv_seq;
	//scan
	struct ips_pa_st pa_st[BT_IPS_CHANNEL_MAX];
	struct bt_ips_cb *pa_cbs;
	int rssi;
	struct bt_le_scan_param scan_param;
	u8_t per_synced_count;

	void *ap_handle;

	uint16_t remote_seq_max;
	uint8_t r_seq_subs_number : 3;
	//bt_addr_t max_a;
};

static struct ips_search_st is_st;
static struct ips_mgr_st entity;
static struct ips_mgr_st *ips_st;
static os_delayed_work ips_adv_work;
uint8_t cur_adv_type;
static os_delayed_work ips_sort_wait_work;

struct ips_pkg_data_head {
	uint8_t seq; //总序号，所有PA包的序号，累加
	uint8_t type : 2; // 01: 空包 10: 数据包
	uint8_t method : 3; //
	uint8_t b_reserve : 3; //
	uint8_t status; //状态
	uint8_t codec; //编码格式
	uint8_t f_len; //帧长
	uint8_t comp_ratio : 2; // 编码压缩比
	uint8_t ch_mode : 2; //单声道或双声道
	uint8_t samp_freq : 2; //采样率8KHZ or 16KHZ
	uint8_t bit_width : 2; // 位宽8bits or 16bits
	uint8_t a_reserve[2]; //
	uint8_t data[0];
} __attribute__((__packed__));

struct ips_pkg_status_head {
	uint8_t seq; //总序号，所有PA包的序号，累加
	uint8_t type : 2; // 01: 空包 10: 数据包
	uint8_t method : 3; //
	uint8_t b_reserve : 3; //
	uint8_t status; //状态
	uint8_t byte1;
	uint8_t byte2;
	uint8_t role; //
	uint8_t num_synced;
} __attribute__((__packed__));

struct ips_ext_adv_data {
	uint16_t s_uuid; //IPS_SERIVCE_UUID
	uint16_t s_ver; // IPS_SERIVCE_VERSION
	uint32_t match_id;//
	uint16_t seq;
	uint8_t adv_status;
	uint8_t adv_role : 2; //
	uint8_t ch_max : 3; //
	uint8_t per_adv : 1; //
	uint8_t comp : 1; //
	uint8_t res1 : 1; //
	uint8_t tx_codec;
	uint8_t comp_ratio;
	uint8_t ch_mode;
	uint8_t samp_freq;
	uint8_t bit_width;
	char ips_name[BT_IPS_NAME_MAX];
	//uint16_t adv_10ms;
	uint8_t subs_number : 3;
	uint8_t cnt_subscriber: 3;
	uint8_t res2: 2;
	bt_addr_t subs_mac[BT_IPS_SUBS_MAX];
} __attribute__((__packed__));

#define  BT_IPS_SEND_BUF_MAX (BT_IPS_LC3_FRAME_NUM_MAX*(2+BT_IPS_LC3_FRAME_LEN) + sizeof(struct ips_pkg_data_head))

static const struct bt_le_scan_param ips_scan_params = {
	/* BT_LE_EXT_ADV_NCONN */
	.type = BT_LE_SCAN_TYPE_PASSIVE,
	.options = BT_LE_SCAN_OPT_NONE | BT_LE_SCAN_OPT_CODED,
	//.interval = 0x60,
	//.window = 0x30,
	/* [65ms, 110ms], almost 60% duty cycle by default */
	.interval = 176,
	.window = 104,
	.timeout = 0,
	.interval_coded = 0,
	.window_coded = 0,
};

static struct bt_le_adv_param ips_ext_adv_params = {
	/* BT_LE_EXT_ADV_NCONN */

#if (CONFIG_BT_ID_MAX > 1)
	.id = 1,
#else
	.id = BT_ID_DEFAULT,
#endif
	/* [100ms, 100ms] by default */
	.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
	.interval_max = BT_GAP_ADV_FAST_INT_MIN_2,
	.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY /*| BT_LE_ADV_OPT_CODED | BT_LE_ADV_OPT_NO_2M*/,
	.sid = BT_EXT_ADV_SID_BROADCAST,
};

static OS_MUTEX_DEFINE(search_mutex);
static OS_MUTEX_DEFINE(ips_mutex);

static int ips_search_device_add(bt_addr_le_t *addr, char *name, u8_t role)
{
	int i;

	if (!addr || !name)
		return -ESRCH;

	os_mutex_lock(&search_mutex, OS_FOREVER);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (!is_st.v_st[i].inuse) {
			is_st.v_st[i].inuse = 1;
			memcpy(&is_st.v_st[i].s_addr, addr, sizeof(bt_addr_le_t));
			memcpy(&is_st.v_st[i].s_name, name, BT_IPS_NAME_MAX);
			is_st.v_st[i].role = role;
#if 0
			printk("add %d %02X:%02X:%02X:%02X:%02X:%02X\n",i,
						is_st.v_st[i].s_addr.a.val[5], is_st.v_st[i].s_addr.a.val[4], is_st.v_st[i].s_addr.a.val[3],
						is_st.v_st[i].s_addr.a.val[2], is_st.v_st[i].s_addr.a.val[1], is_st.v_st[i].s_addr.a.val[0]);
#endif
			break;
		}
	}
	os_mutex_unlock(&search_mutex);

	if (i == BT_IPS_CHANNEL_MAX) {
		SYS_LOG_ERR("Failed to add search device.");
		return -EIO;
	}

	return 0;
}

#if 0
static int ips_search_device_remove(bt_addr_le_t *addr)
{
	int i;

	if (!addr)
		return -ESRCH;

	os_mutex_lock(&search_mutex, OS_FOREVER);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (is_st.v_st[i].inuse &&
				(!memcmp(&is_st.v_st[i].s_addr, addr, sizeof(bt_addr_le_t)))) {
			memset(&is_st.v_st[i], 0 ,sizeof(struct ips_search_value_st));
			break;
		}
	}
	os_mutex_unlock(&search_mutex);

	if (i == BT_IPS_CHANNEL_MAX) {
		SYS_LOG_ERR("Failed to remove search_device");
		return -EIO;
	}

	return 0;
}
#endif

static struct ips_search_value_st *ips_search_device_get(bt_addr_le_t *addr)
{
	int i;

	if (!addr)
		return NULL;

	os_mutex_lock(&search_mutex, OS_FOREVER);

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (is_st.v_st[i].inuse &&
				(!memcmp(&is_st.v_st[i].s_addr.a, &addr->a, sizeof(bt_addr_t)))) {
			os_mutex_unlock(&search_mutex);
			return &is_st.v_st[i];
		}
	}

	os_mutex_unlock(&search_mutex);
	return NULL;
}

static uint8_t ips_search_device_num(void)
{
	int i, j;

	os_mutex_lock(&search_mutex, OS_FOREVER);
	for (i = 0, j = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (is_st.v_st[i].inuse) {
			j++;
		}
	}
	os_mutex_unlock(&search_mutex);

	if (BT_IPS_CHANNEL_MAX == j) {
		SYS_LOG_INF(" pa channel max.");
	}

	return j;
}

static struct ips_search_value_st *ips_search_device_by_id(uint8_t index)
{
	int i = index;

	if (index >= BT_IPS_CHANNEL_MAX)
		return NULL;

	os_mutex_lock(&search_mutex, OS_FOREVER);

	if (is_st.v_st[index].inuse) {
		os_mutex_unlock(&search_mutex);
#if 0
		printk("add %d %02X:%02X:%02X:%02X:%02X:%02X\n",i,
					is_st.v_st[i].s_addr.a.val[5], is_st.v_st[i].s_addr.a.val[4], is_st.v_st[i].s_addr.a.val[3],
					is_st.v_st[i].s_addr.a.val[2], is_st.v_st[i].s_addr.a.val[1], is_st.v_st[i].s_addr.a.val[0]);
#endif
		return &is_st.v_st[i];
	}

	os_mutex_unlock(&search_mutex);
	return NULL;
}

static int ips_add_pa(void *sync)
{
	int i;

	if (!sync || !ips_st)
		return -ESRCH;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (ips_st->pa_st[i].adv_sync == NULL) {
			ips_st->pa_st[i].adv_sync = sync;
			SYS_LOG_INF("a sync (%p) %p\n", sync,&ips_st->pa_st[i]);
			break;
		}
	}
	os_mutex_unlock(&ips_mutex);

	if (i == BT_IPS_CHANNEL_MAX) {
		SYS_LOG_ERR("Failed to add pa %p", sync);
		return -EIO;
	}

	return 0;
}

static int ips_remove_pa(void *sync)
{
	int i;

	if (!sync || !ips_st)
		return -ESRCH;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (ips_st->pa_st[i].adv_sync == sync) {
			//ips_st->pa_st[i].adv_sync = NULL;
			SYS_LOG_INF("r sync (%p) %p\n", sync,&ips_st->pa_st[i]);
			memset(&ips_st->pa_st[i], 0 ,sizeof(struct ips_pa_st));
			break;
		}
	}
	os_mutex_unlock(&ips_mutex);

	if (i == BT_IPS_CHANNEL_MAX) {
		SYS_LOG_ERR("Failed to remove pa %p", sync);
		return -EIO;
	}

	return 0;
}

static struct ips_pa_st *ips_pa_st_get(void *sync)
{
	int i;
	struct bt_le_per_adv_sync *adv_sync;

	if (!sync || !ips_st)
		return NULL;

	os_mutex_lock(&ips_mutex, OS_FOREVER);

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = ips_st->pa_st[i].adv_sync;
		SYS_LOG_INF("g sync (%p) %p\n", adv_sync,&ips_st->pa_st[i]);
		if (adv_sync == sync) {
			os_mutex_unlock(&ips_mutex);
			return &ips_st->pa_st[i];
		}
	}

	os_mutex_unlock(&ips_mutex);
	return NULL;
}

static struct ips_pa_st *ips_pa_st_by_mac(bt_addr_le_t *addr)
{
	int i;
	struct bt_le_per_adv_sync *adv_sync;

	if (!addr || !ips_st)
		return NULL;

	os_mutex_lock(&ips_mutex, OS_FOREVER);

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = ips_st->pa_st[i].adv_sync;
		SYS_LOG_INF("adv_sync %p.\n",adv_sync);

		if (adv_sync &&
			!memcmp(&ips_st->pa_st[i].per_addr.a, &addr->a, sizeof(bt_addr_t))) {
			os_mutex_unlock(&ips_mutex);
			return &ips_st->pa_st[i];
		}
	}

	os_mutex_unlock(&ips_mutex);
	return NULL;
}

static struct ips_pa_st *ips_pa_st_by_id(uint8_t index)
{
	int i;
	struct bt_le_per_adv_sync *adv_sync;

	if (BT_IPS_CHANNEL_MAX <= index)
		return NULL;

	if (!ips_st)
		return NULL;

	os_mutex_lock(&ips_mutex, OS_FOREVER);

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = ips_st->pa_st[i].adv_sync;
		if (adv_sync && index == i) {
			os_mutex_unlock(&ips_mutex);
			return &ips_st->pa_st[i];
		}
	}

	os_mutex_unlock(&ips_mutex);
	return NULL;
}

static uint8_t ips_pa_sync_num(void)
{
	int i, j;
	struct ips_pa_st *pa_st;

	for (i = 0, j = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		pa_st = ips_pa_st_by_id(i);
		if (pa_st) {
			j++;
		}
	}

	if (BT_IPS_CHANNEL_MAX == j) {
		SYS_LOG_INF(" pa channel max.");
	}

	return j;
}

static void le_ips_mac_addr_get(bt_addr_le_t *addr)
{
	bt_addr_le_t le_addr[CONFIG_BT_ID_MAX];
	hostif_bt_le_get_mac((bt_addr_le_t *)le_addr);
#if 0
	printk("addr_get 0 %02X:%02X:%02X:%02X:%02X:%02X\n",
				le_addr[0].a.val[5], le_addr[0].a.val[4], le_addr[0].a.val[3],
				le_addr[0].a.val[2], le_addr[0].a.val[1], le_addr[0].a.val[0]);
	printk("addr_get 1 %02X:%02X:%02X:%02X:%02X:%02X\n",
				le_addr[1].a.val[5], le_addr[1].a.val[4], le_addr[1].a.val[3],
				le_addr[1].a.val[2], le_addr[1].a.val[1], le_addr[1].a.val[0]);
#endif
#if (CONFIG_BT_ID_MAX > 1)
	memcpy(addr, &le_addr[1], sizeof(bt_addr_le_t));
#else
	memcpy(addr, &le_addr[0], sizeof(bt_addr_le_t));
#endif
}

static uint8_t codec_frame_num_calc(uint8_t codec)
{
	return BT_IPS_LC3_FRAME_NUM_MAX;
}

static uint8_t codec_frame_len_calc(uint8_t codec)
{
	if (BT_IPS_TX_CODEC_LC3 == codec) {
		return BT_IPS_LC3_FRAME_LEN;
	} else if (BT_IPS_TX_CODEC_OPUS == codec) {
		return BT_IPS_OPUS_FRAME_LEN;
	} else if (BT_IPS_TX_CODEC_AMR == codec) {
		return BT_IPS_AMR_FRAME_LEN;
	}

	return BT_IPS_LC3_FRAME_LEN;
}

static int __ips_ext_adv(u8_t type)
{
	int err;
	struct ips_ext_adv_data adv_data;
	struct bt_data ad[1];
	int items = 0;
	struct bt_le_ext_adv *cur_adv = NULL;
	//uint8_t eadv[sizeof(struct ips_ext_adv_data) + BT_IPS_NAME_MAX*6];

	memset(&adv_data, 0, sizeof(struct ips_ext_adv_data));
	//UUID
	adv_data.s_uuid = IPS_SERIVCE_UUID;
	//version, Notice: Do not modify
	adv_data.s_ver = IPS_SERIVCE_VERSION;
	//match id
	if (BT_IPS_SEARCH_ADV == type) {
		adv_data.match_id = is_st.match_id;
		adv_data.adv_role = is_st.cur_role;
		adv_data.ch_max = 0;
		adv_data.adv_status = BT_IPS_SEARCH_ADV;
		memcpy(adv_data.ips_name, is_st.cur_name, BT_IPS_NAME_MAX);
		cur_adv = is_st.s_adv;
		adv_data.seq = is_st.seq++;
	} else if (BT_IPS_START_ADV == type) {
		adv_data.match_id = ips_st->match_id;
		adv_data.adv_role = ips_st->adv_role;
		adv_data.tx_codec = ips_st->tx_codec;
		adv_data.comp_ratio = ips_st->comp_ratio;
		adv_data.ch_mode = ips_st->ch_mode;
		adv_data.samp_freq = ips_st->samp_freq;
		adv_data.bit_width = ips_st->bit_width;
		adv_data.ch_max = ips_st->ch_max;
		// SYS_LOG_INF("adv_data.ch_max: %d", adv_data.ch_max);
		adv_data.adv_status = BT_IPS_START_ADV;
		memcpy(adv_data.ips_name, ips_st->pa_name, BT_IPS_NAME_MAX);
		cur_adv = ips_st->ips_adv;
		adv_data.subs_number = ips_st->subs_number;
		adv_data.cnt_subscriber = ips_st->cnt_subscriber;

		if (adv_data.cnt_subscriber > 0 && adv_data.cnt_subscriber <= BT_IPS_SUBS_MAX) {
			//memcpy(&eadv[sizeof(struct ips_ext_adv_data)], &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
			memcpy(adv_data.subs_mac, &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
		}
		adv_data.seq = ips_st->adv_seq++;
	} else if (BT_IPS_RESTART_ADV == type) {
		adv_data.match_id = ips_st->match_id;
		adv_data.adv_role = ips_st->adv_role;
		adv_data.tx_codec = ips_st->tx_codec;
		adv_data.comp_ratio = ips_st->comp_ratio;
		adv_data.ch_mode = ips_st->ch_mode;
		adv_data.samp_freq = ips_st->samp_freq;
		adv_data.bit_width = ips_st->bit_width;
		adv_data.ch_max = ips_st->ch_max;
		adv_data.per_adv = 1;
		adv_data.comp = ips_st->pa_comp;
		// SYS_LOG_INF("adv_data.ch_max: %d", adv_data.ch_max);
		adv_data.adv_status = BT_IPS_RESTART_ADV;
		memcpy(adv_data.ips_name, ips_st->pa_name, BT_IPS_NAME_MAX);
		cur_adv = ips_st->ips_adv;
		adv_data.subs_number = ips_st->subs_number;
		adv_data.cnt_subscriber = ips_st->cnt_subscriber;
		if (adv_data.cnt_subscriber > 0 && adv_data.cnt_subscriber <= BT_IPS_SUBS_MAX) {
			//memcpy(&eadv[sizeof(struct ips_ext_adv_data)], &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
			memcpy(&adv_data.subs_mac[0], &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
		}
		adv_data.seq = ips_st->adv_seq++;
	}
	//memcpy(eadv, &adv_data, sizeof(struct ips_ext_adv_data));

	ad[items].type = BT_DATA_MANUFACTURER_DATA;
	ad[items].data_len = sizeof(struct ips_ext_adv_data);// + adv_data.cnt_subscriber*6;
	//ad[items].data = (uint8_t *)&eadv[0];
	ad[items].data = (uint8_t *)&adv_data;
	items++;

	if (cur_adv) {
		err = hostif_bt_le_ext_adv_set_data(cur_adv, ad,
					items, NULL, 0);
		if (err) {
			SYS_LOG_INF("set data: %d", err);
			return err;
		}
	}

	return 0;
}

static void ips_adv_active_loop(struct k_work *work)
{
	if (BT_IPS_SEARCH_ADV == cur_adv_type && 0 == is_st.searching) 
		return;

	if (!ips_st && 
		(BT_IPS_START_ADV == cur_adv_type ||
		BT_IPS_RESTART_ADV == cur_adv_type))
		return;

	if (ips_st && 0 == ips_st->adv_enable)
		return;

	__ips_ext_adv(cur_adv_type);
	os_delayed_work_submit(&ips_adv_work, 100);
}

static uint8_t ips_send_buf[BT_IPS_SEND_BUF_MAX];
static int __ips_per_adv(uint8_t type, uint8_t status, uint8_t num, uint8_t *data, uint16_t len)
{
	int err;
	//u8_t ver_data[30];
	u8_t offset = 0;
	u8_t f_num, f_num_max, f_len;
	struct bt_data ad_data[1];
	int items = 0, i;
	struct ips_pkg_data_head data_head;
	struct ips_pkg_status_head status_head;

	if (BT_IPS_DATA_HEAD_TYPE == type) {
		#if 0
		ver_data[offset++] = ips_st->per_seq++;
		ver_data[offset++] = BT_IPS_DATA_HEAD_TYPE;
		ver_data[offset++] = BT_IPS_STATUS_TALK;
		ver_data[offset++] = ips_st->tx_codec;
		f_len = codec_frame_len_calc(ips_st->tx_codec);
		ver_data[offset++] = f_len;
		ver_data[offset++] = 0;
		ver_data[offset++] = 0;
		ver_data[offset++] = 0;
		memcpy(ips_send_buf, ver_data, offset);
		#endif

		data_head.seq = ips_st->per_seq++;
		data_head.type = BT_IPS_DATA_HEAD_TYPE;
		data_head.status = BT_IPS_STATUS_TALK;
		data_head.codec = ips_st->tx_codec;
		f_len = codec_frame_len_calc(ips_st->tx_codec);
		data_head.f_len = f_len;
		memcpy(ips_send_buf, &data_head, sizeof(struct ips_pkg_data_head));
		offset += sizeof(struct ips_pkg_data_head);
		f_num_max = codec_frame_num_calc(ips_st->tx_codec);
		if (ips_st->f_seq_end < ips_st->f_seq_start) {
			f_num = 0xFF-ips_st->f_seq_start+ips_st->f_seq_end+2;
		} else {
			f_num = ips_st->f_seq_end - ips_st->f_seq_start+1;
		}
		if (ips_st->last_f_num && f_num != ips_st->last_f_num)
			SYS_LOG_ERR("f_num: %d_%d", f_num, ips_st->last_f_num);
		if (f_num > f_num_max)
			SYS_LOG_ERR("f_num: %d_%d", f_num, f_num_max);

		if (1 == f_num) {
			SYS_LOG_INF("f_seq: %d_%d", ips_st->f_seq_end, ips_st->f_seq_start);
			if (!ips_st->last_f_num) {
				f_num = 0;
				ips_st->f_seq_end = 0;
				ips_st->f_seq_start = 0;
			}
		}
		if (f_num == f_num_max) {
			SYS_LOG_INF("f_seq: %d_%d", ips_st->f_seq_end, ips_st->f_seq_start);
			for (i = 1;i < f_num; i++) {
				memcpy(ips_send_buf + offset,
					ips_send_buf + offset + (2 + f_len), f_len + 2);
				offset += (2 + f_len);
			}
			ips_st->f_seq_start++;
			ips_st->f_seq_end++;
			ips_send_buf[offset++] = ips_st->f_seq_end;
			ips_send_buf[offset++] = f_len;
			memcpy(ips_send_buf + offset, data, f_len);
			offset += f_len;
		} else {
			ips_st->f_seq_end++;
			if (0 == ips_st->last_f_num)
				ips_st->f_seq_start++;
			offset += (f_len+2)*f_num;
			ips_send_buf[offset++] = ips_st->f_seq_end;
			ips_send_buf[offset++] = f_len;
			memcpy(ips_send_buf + offset, data, f_len);
			offset += f_len;
		}

		if (ips_st->f_seq_end < ips_st->f_seq_start) {
			ips_st->last_f_num = 0xFF-ips_st->f_seq_start+ips_st->f_seq_end+2;
		} else {
			ips_st->last_f_num = ips_st->f_seq_end - ips_st->f_seq_start+1;
		}
		SYS_LOG_INF("f_seq: %d_%d %d", ips_st->f_seq_end, ips_st->f_seq_start,ips_st->last_f_num);
		ad_data[items].data = ips_send_buf;
	} else {
	#if 0
		ver_data[offset++] = ips_st->per_seq++;
		ver_data[offset++] = BT_IPS_NULL_HEAD_TYPE;
		ver_data[offset++] = 0x30;
		ver_data[offset++] = 0x85;
		ver_data[offset++] = status;
		ver_data[offset++] = num;
	#endif
		status_head.seq = ips_st->per_seq++;
		status_head.type = BT_IPS_NULL_HEAD_TYPE;
		status_head.status = status;
		status_head.byte1 = 0x30;
		status_head.byte2 = 0x85;
		status_head.role = ips_st->adv_role;
		status_head.num_synced = num;
		offset += sizeof(struct ips_pkg_status_head);
		ad_data[items].data = (uint8_t *)&status_head;
		ips_st->last_f_num = 0;
		ips_st->f_seq_start = 0;
		ips_st->f_seq_end = 0;
	}

	ad_data[items].type = BT_DATA_VS_IPS_CODEC;
	ad_data[items].data_len = offset;

	items++;

	err = hostif_bt_le_per_adv_set_data(ips_st->ips_adv, ad_data, items);
	if (err) {
		SYS_LOG_INF("set data: %d", err);
		return err;
	}

	return 0;
}

static int __ips_adv_start(u8_t type)
{
	int err;
	struct bt_le_per_adv_param per_adv_params = { 0 };
	struct bt_le_ext_adv *cur_adv;

	if (ips_st && 1 == ips_st->adv_enable) {
		SYS_LOG_INF("adv already start: %d", ips_st->adv_enable);
		return -EBUSY;
	}

	if (BT_IPS_SEARCH_ADV == type) {
		ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY;// | BT_LE_ADV_OPT_NO_2M;
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(is_st.s_adv));
		cur_adv = is_st.s_adv;
		is_st.seq = 0;
	} else if (BT_IPS_START_ADV == type) {
		if (ips_st->ch_max < 2) {
			ips_ext_adv_params.options |= BT_LE_ADV_OPT_CODED;
		} else {
			ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		}
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(ips_st->ips_adv));
		cur_adv = ips_st->ips_adv;
		ips_st->adv_seq = 0;
	} else if (BT_IPS_RESTART_ADV == type) {
		if (ips_st->ch_max < 2) {
			//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_CODED;// | BT_LE_ADV_OPT_NO_2M;
			ips_ext_adv_params.options |= BT_LE_ADV_OPT_CODED;
			//ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED); // need to modify
		} else {
			//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY;// | BT_LE_ADV_OPT_NO_2M;
			ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		}
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(ips_st->ips_adv));
		cur_adv = ips_st->ips_adv;
		ips_st->adv_seq = 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	if (err) {
		SYS_LOG_ERR("Failed to create advertising set (err %d)\n", err);
		return -EALREADY;
	}

	if (BT_IPS_RESTART_ADV == type) {
		ips_st->pa_enable = 1;
		/* Set periodic advertising parameters */
		per_adv_params.interval_min = 16; //20ms
		per_adv_params.interval_max = 16; //20ms
		per_adv_params.options = BT_LE_PER_ADV_OPT_NONE;

		SYS_LOG_INF("per_param:%d,%d \n", per_adv_params.interval_min, per_adv_params.interval_max);
		err = hostif_bt_le_per_adv_set_param(ips_st->ips_adv, &per_adv_params);
		if (err) {
			SYS_LOG_ERR("Failed to set periodic advertising parameters (err %d)\n", err);
			return -EALREADY;
		}
		/* Enable Periodic Advertising */
		err = hostif_bt_le_per_adv_start(ips_st->ips_adv);
		if (err) {
			SYS_LOG_ERR("Failed to enable periodic advertising (err %d)\n", err);
			return -EALREADY;
		}
		SYS_LOG_INF("Start Periodic Advertising\n");
	}
	err = hostif_bt_le_ext_adv_start(cur_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		SYS_LOG_ERR("Failed to start extended advertising (err %d)\n", err);
		return -EALREADY;
	}

	__ips_ext_adv(type);
	if (BT_IPS_RESTART_ADV == type)
		__ips_per_adv(BT_IPS_NULL_HEAD_TYPE, BT_IPS_STATUS_MATCHING, 0, NULL, 0);

	os_delayed_work_init(&ips_adv_work, ips_adv_active_loop);
	os_delayed_work_cancel(&ips_adv_work);
	os_delayed_work_submit(&ips_adv_work, 100);
	if (ips_st)
		ips_st->adv_enable = 1;
	cur_adv_type = type;

	return 0;
}

static int __ips_adv_stop(u8_t type)
{
	int err;
	struct bt_le_ext_adv *cur_adv = NULL;

	if (ips_st && 0 == ips_st->adv_enable) {
		SYS_LOG_INF("adv already stop: %d", ips_st->adv_enable);
		return -EALREADY;
	}

	if (BT_IPS_SEARCH_ADV == type) {
		cur_adv = is_st.s_adv;
	} else if (BT_IPS_START_ADV == type) {
		os_delayed_work_cancel(&ips_sort_wait_work);
		ips_st->adv_work_exist = 0;
		cur_adv = ips_st->ips_adv;
		ips_st->pa_enable = 0;
		ips_st->adv_enable = 0;
	} else if (BT_IPS_RESTART_ADV == type) {
		cur_adv = ips_st->ips_adv;
		ips_st->pa_enable = 0;
		ips_st->adv_enable = 0;
	}

	os_delayed_work_cancel(&ips_adv_work);

	if (cur_adv) {
		/* Stop periodic advertising */
		err = hostif_bt_le_per_adv_stop(cur_adv);
		if (err) {
			SYS_LOG_ERR("per_adv: %d", err);
			//return err;
		}

		/* Stop extended advertising */
		err = hostif_bt_le_ext_adv_stop(cur_adv);
		if (err) {
			SYS_LOG_ERR("ext_adv: %d", err);
			//return err;
		}
		hostif_bt_le_ext_adv_delete(cur_adv);
	}

	return 0;
}

static void extract_codec(struct ips_pa_st *pa_st, const uint8_t *buf, uint16_t len)
{
	struct bt_ips_recv_info info;
	uint16_t offset = 0;
	uint16_t seq, cur_seq;
	struct ips_pkg_data_head *data_head;
	struct ips_pkg_status_head *status_head = (struct ips_pkg_status_head *)buf;
	uint8_t f_len;

	SYS_LOG_INF("pa: %d_%d", status_head->seq, pa_st->seq_pa);

	if (status_head->seq == pa_st->seq_pa) {
		SYS_LOG_ERR("pa: %d_%d", status_head->seq, pa_st->seq_pa);
		return;
	}

	if (status_head->seq != pa_st->seq_pa + 1) {
		SYS_LOG_ERR("pa: %d_%d", status_head->seq, pa_st->seq_pa);
	}

	pa_st->seq_pa = status_head->seq;
	if (BT_IPS_NULL_HEAD_TYPE == status_head->type) {
		if (sizeof(struct ips_pkg_status_head) != len) {
			SYS_LOG_ERR("n_pkg len: %d", len);
		}
		if (ips_st && ips_st->pa_cbs) {
			ips_st->pa_cbs->ips_status(pa_st->adv_sync, status_head->status, status_head->num_synced);
		}
		pa_st->cur_pkg_num = 0;
		pa_st->first_pkg_num = 0;
		return;
	}

	if (BT_IPS_DATA_HEAD_TYPE != status_head->type ||
			sizeof(struct ips_pkg_data_head) >= len) {
		SYS_LOG_ERR("n_pkg len: %d", len);
		return;
	}

	data_head = (struct ips_pkg_data_head *)buf;
	cur_seq = pa_st->first_pkg_num;
	offset += sizeof(struct ips_pkg_data_head);
	while (len > offset) {
		seq = buf[offset];
		if (sizeof(struct ips_pkg_data_head) == offset) {
			pa_st->first_pkg_num = seq;
		}
		offset++;
		f_len = buf[offset++];
		if (seq != cur_seq + 1) {
			SYS_LOG_ERR("seq: %d_%d", cur_seq, seq);
		}
		if (f_len != codec_frame_len_calc(data_head->codec)) {
			SYS_LOG_ERR("f_len: %d_%d", f_len, codec_frame_len_calc(data_head->codec));
			return;
		}
		cur_seq = seq;
		if (pa_st->cur_pkg_num < seq &&
				seq - pa_st->cur_pkg_num < 0x7F) {
			pa_st->cur_pkg_num = seq;
		} else if (pa_st->cur_pkg_num > seq &&
				pa_st->cur_pkg_num - seq > 0x7F) {
			pa_st->cur_pkg_num = seq; // loop
		} else {
			offset += f_len;
			continue;
		}

		if (ips_st && ips_st->pa_cbs) {
			info.length = f_len;
			info.pkt_num = pa_st->cur_pkg_num;
			ips_st->pa_cbs->ips_recv_codec(
				pa_st->adv_sync, &info, (uint8_t *)&buf[offset]);
		}
		offset += f_len;
		SYS_LOG_INF("cur_pkg_num: %d_%d", pa_st->cur_pkg_num, cur_seq);
	}
}

static bool filter_data_cb(struct bt_data *data, void *user_data)
{
	uint8_t len;
	uint8_t *buf;
	struct ips_pa_st *pa_st;

	switch (data->type) {
	case BT_DATA_MANUFACTURER_DATA:
		buf = user_data;
		len = MIN(data->data_len, sizeof(struct ips_ext_adv_data));
		memcpy(buf, data->data, len);
		return false;

	case BT_DATA_VS_IPS_CODEC:
		pa_st = (struct ips_pa_st *)user_data;
		extract_codec(pa_st, data->data, data->data_len);
		return false;

	default:
		return true;
	}
}

static void ips_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	int err;
	struct bt_le_per_adv_sync *pa_sync = NULL;
	struct ips_pa_st *pa_st;
	struct ips_ext_adv_data adv_data;
	struct net_buf_simple_state state;
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_ips_search_rt rt;
	int i;
	struct ips_search_value_st *v_st;

	if (info->adv_type != BT_GAP_ADV_TYPE_EXT_ADV) {
		return;
	}

	if (!ips_st && 0 == is_st.searching) {
		return;
	}

	SYS_LOG_INF("rssi %d, 0x%x.\n",info->rssi, info->addr->a.val[0]);
	memset(&adv_data, 0, sizeof(adv_data));
	net_buf_simple_save(buf, &state);
	bt_data_parse(buf, filter_data_cb, (void *)&adv_data);
	net_buf_simple_restore(buf, &state);

	if (adv_data.s_uuid != IPS_SERIVCE_UUID) {
		SYS_LOG_INF("Unknow service data.");
		return;
	}
	if (adv_data.s_ver != IPS_SERIVCE_VERSION) {
		SYS_LOG_INF("Unknow service ver.");
		//return;
	}

	if (is_st.searching) {
		if (adv_data.match_id != is_st.match_id) {
			SYS_LOG_INF("Unknow match id.");
			return;
		}

		if (BT_IPS_ROLE_INITIATOR == is_st.cur_role &&
			(BT_IPS_ROLE_SUBSCRIBER == adv_data.adv_role)) {
			if (ips_search_device_get((bt_addr_le_t *)info->addr)) {
				SYS_LOG_INF("already search..");
				return;
			}
#if 0
			printk("info %02X:%02X:%02X:%02X:%02X:%02X\n",
						info->addr->a.val[5], info->addr->a.val[4], info->addr->a.val[3],
						info->addr->a.val[2], info->addr->a.val[1], info->addr->a.val[0]);
#endif
			ips_search_device_add((bt_addr_le_t *)info->addr, (char *)adv_data.ips_name, adv_data.adv_role);
			rt.t_num = ips_search_device_num();
			rt.role = BT_IPS_ROLE_SUBSCRIBER;
			memcpy(rt.name, adv_data.ips_name, BT_IPS_NAME_MAX);

			rt.cnt_subscriber = rt.t_num + 1;
#if 0
			printk("ass %02X:%02X:%02X:%02X:%02X:%02X\n",
						is_st.local_addr.a.val[5], is_st.local_addr.a.val[4], is_st.local_addr.a.val[3],
						is_st.local_addr.a.val[2], is_st.local_addr.a.val[1], is_st.local_addr.a.val[0]);
#endif
			memcpy(&rt.subs_mac[0], &is_st.local_addr.a, 6);
			if (rt.cnt_subscriber > 1 && rt.cnt_subscriber <= BT_IPS_SUBS_MAX) {
				for (i = 0; i + 1 < rt.cnt_subscriber; i++) {
					v_st = ips_search_device_by_id(i);
					if (v_st) {
						memcpy(&rt.subs_mac[i+1], &v_st->s_addr.a, 6);
					}
				}
			}
#if 0
			printk("0 %02X:%02X:%02X:%02X:%02X:%02X\n",
						rt.subs_mac[0].val[5], rt.subs_mac[0].val[4], rt.subs_mac[0].val[3],
						rt.subs_mac[0].val[2], rt.subs_mac[0].val[1], rt.subs_mac[0].val[0]);
			printk("1 %02X:%02X:%02X:%02X:%02X:%02X\n",
						rt.subs_mac[1].val[5], rt.subs_mac[1].val[4], rt.subs_mac[1].val[3],
						rt.subs_mac[1].val[2], rt.subs_mac[1].val[1], rt.subs_mac[1].val[0]);
#endif
			if (is_st.s_cb)
				is_st.s_cb(&rt);
		} else if (BT_IPS_ROLE_SUBSCRIBER == is_st.cur_role &&
			(BT_IPS_ROLE_INITIATOR == adv_data.adv_role) /*&& adv_data.ch_max > 0*/) {
			if (0 == adv_data.ch_max && ips_search_device_get((bt_addr_le_t *)info->addr)) {
				SYS_LOG_INF("already search..");
				return;
			}
			ips_search_device_add((bt_addr_le_t *)info->addr, (char *)adv_data.ips_name, adv_data.adv_role);
			rt.t_num = adv_data.ch_max;
			memcpy(rt.name, adv_data.ips_name, BT_IPS_NAME_MAX);
			rt.role = BT_IPS_ROLE_INITIATOR;
			rt.cnt_subscriber = adv_data.cnt_subscriber;
			if (rt.cnt_subscriber > 0 && rt.cnt_subscriber <= BT_IPS_SUBS_MAX) {
				memcpy(&rt.subs_mac[0], &adv_data.subs_mac[0], rt.cnt_subscriber*6);
			}
#if 0
			printk("0 %d. %02X:%02X:%02X:%02X:%02X:%02X\n",rt.cnt_subscriber,
						rt.subs_mac[0].val[5], rt.subs_mac[0].val[4], rt.subs_mac[0].val[3],
						rt.subs_mac[0].val[2], rt.subs_mac[0].val[1], rt.subs_mac[0].val[0]);
			printk("1 %02X:%02X:%02X:%02X:%02X:%02X\n",
						rt.subs_mac[1].val[5], rt.subs_mac[1].val[4], rt.subs_mac[1].val[3],
						rt.subs_mac[1].val[2], rt.subs_mac[1].val[1], rt.subs_mac[1].val[0]);
#endif
			if (is_st.s_cb)
				is_st.s_cb(&rt);
		}
		return;
	} else if (adv_data.match_id != ips_st->match_id) {
		SYS_LOG_INF("Unknow match id.");
		return;
	}

	if (BT_IPS_SEARCH_ADV == adv_data.adv_status) {
		return;
	}

	SYS_LOG_INF("remote_seq_max %d_%d_%d_%d .", ips_st->remote_seq_max,ips_st->adv_seq,ips_st->subs_number,adv_data.subs_number);

	if (BT_IPS_START_ADV == adv_data.adv_status) {
		if (ips_st->remote_seq_max < adv_data.seq) {
			ips_st->remote_seq_max = adv_data.seq;
			ips_st->r_seq_subs_number = adv_data.subs_number;
			if (BT_IPS_START_ADV == cur_adv_type) {
				if (0 == ips_st->adv_work_exist) {
					ips_st->adv_work_exist = 1;
					os_delayed_work_cancel(&ips_sort_wait_work);
					os_delayed_work_submit(&ips_sort_wait_work, 5000);
				}
			}
		}
		return;
	}

	if (ips_pa_st_by_mac((bt_addr_le_t *)info->addr)) {
		SYS_LOG_INF(" pa exist.");
		return;
	}

	if (BT_IPS_CHANNEL_MAX == ips_pa_sync_num()) {
		return;
	}
	
	if (0 == adv_data.per_adv) {
		return;
	}

	if (info->interval) {
		SYS_LOG_INF("Creating Periodic Advertising Sync \n");
		bt_addr_le_copy(&sync_create_param.addr, info->addr);
		sync_create_param.options = 0;
		sync_create_param.sid = info->sid;
		sync_create_param.skip = 0;
		sync_create_param.timeout = 0xaa*2;
		err = hostif_bt_le_per_adv_sync_create(&sync_create_param, &(pa_sync));
		if (err) {
			SYS_LOG_ERR("Failed to create sync (err %d)\n", err);
			return;
		}

		if (0 == ips_st->pa_enable) {
			__ips_adv_stop(cur_adv_type);
		}

		SYS_LOG_INF("pa_sync (%p)\n", pa_sync);
		if (pa_sync) {
			ips_add_pa(pa_sync);
			pa_st = ips_pa_st_get(pa_sync);
			if (pa_st) {
				pa_st->per_sid = info->sid;
				bt_addr_le_copy(&pa_st->per_addr, info->addr);
				pa_st->role = adv_data.adv_role;
				pa_st->rx_codec = adv_data.tx_codec; //PA的编码格式
				pa_st->comp_ratio = adv_data.comp_ratio; // 编码压缩比
				pa_st->ch_mode = adv_data.ch_mode; //单声道或双声道
				pa_st->samp_freq = adv_data.samp_freq; //采样率8KHZ or 16KHZ
				pa_st->bit_width = adv_data.bit_width; // 位宽8bits or 16bits
				pa_st->pa_comp = adv_data.comp; // 基准
				pa_st->subs_number = adv_data.subs_number;
				memcpy(pa_st->sync_name, adv_data.ips_name, BT_IPS_NAME_MAX);
			}
		}
	}
}

static struct bt_le_scan_cb ips_scan_callbacks = {
	.recv = ips_scan_recv,
};

static struct ips_pa_st* ips_comp_pa_st_get(void)
{
	int i;
	struct ips_pa_st *pa_st;
	struct ips_pa_st *cur_pa_st = NULL;
	uint8_t c_subs_number = 0xFF;
	uint8_t comp_exit = 0;

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		pa_st = ips_pa_st_by_id(i);
		if (pa_st) {
			if (0 == comp_exit && 1 == pa_st->pa_comp) {
				cur_pa_st = pa_st;
				c_subs_number = pa_st->subs_number;
				comp_exit = 1;
			} else if (comp_exit == pa_st->pa_comp) {
				if (c_subs_number > pa_st->subs_number) {
					cur_pa_st = pa_st;
					c_subs_number = pa_st->subs_number;
				}
			} else {
				//
			}
		}
	}
	return cur_pa_st;
}

static int ips_per_comp_addr_set(struct bt_le_per_adv_sync *sync, bt_addr_le_t *le_addr)
{
	int i;
	uint8_t subs_number = BT_IPS_SUBS_MAX + 1;
	int ret = -1;
	uint8_t of = 0;
	uint8_t en = 0;

	for (i = 0; i < ips_st->cnt_subscriber; i++) {
		if (!memcmp(&ips_st->subs_mac[i], &le_addr->a, 6)) {
			subs_number = i;
			ret = 0;
			break;
		}
	}
	SYS_LOG_INF("c_subs %d_%d_%d.",ips_st->cnt_subscriber, subs_number,ret);

	if (0 == ret) {
		if (subs_number > ips_st->subs_number) {
			of = subs_number-ips_st->subs_number;
			of |= 0x80;
			en = true;
		} else if (subs_number < ips_st->subs_number) {
			of = ips_st->subs_number-subs_number;
			en = true;
		} else {
			of = 0;
			en = false;
		}
		hostif_bt_le_per_adv_sync_comp_set(sync, en, of);
	}

	return ret;
}

static int ips_per_comp_adjust(void)
{
	struct ips_pa_st *c_pa_st;

	c_pa_st = ips_comp_pa_st_get();
	if (!c_pa_st)
		return 0;

	if ((1 == c_pa_st->pa_comp && 0 == ips_st->pa_comp) ||
		(c_pa_st->pa_comp == ips_st->pa_comp && 
		c_pa_st->subs_number < ips_st->subs_number)) {
		c_pa_st->pa_comp = 1;
		ips_st->pa_comp = 0;
		if (ips_per_comp_addr_set(c_pa_st->adv_sync, &c_pa_st->per_addr)) {
			SYS_LOG_ERR("c_pa_st %p", c_pa_st);
		}
	} else {
		c_pa_st->pa_comp = 0;
		ips_st->pa_comp = 1;
		SYS_LOG_INF("cur pa_comp %d.", ips_st->pa_comp);
	}

	return 0;
}

static void ips_sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;
	struct bt_ips_sync_info sync_info;
	int err = 0;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		SYS_LOG_ERR("sync %p",sync);
		return;
	}

	if (pa_st->pa_synced) {
		SYS_LOG_ERR("pa_synced %p",pa_st->pa_synced);
		return;
	}

	pa_st->pa_synced = 1;
	ips_st->per_synced_count++;
	if (ips_st->per_synced_count > ips_st->ch_max) {
		SYS_LOG_ERR(":%d,%d \n",ips_st->per_synced_count,ips_st->ch_max);
	}

	if (ips_st->ch_max == ips_st->per_synced_count) {
		err = hostif_bt_le_scan_stop();
		//if (err) {
		//	SYS_LOG_INF("err: %d", err);
		//}
		SYS_LOG_INF("scanstop err: %d", err);
	}
	SYS_LOG_INF("per_synced_count %d",ips_st->per_synced_count);

	sync_info.role = pa_st->role;
	sync_info.rx_codec = pa_st->rx_codec;
	sync_info.comp_ratio = pa_st->comp_ratio;
	sync_info.ch_mode = pa_st->ch_mode;
	sync_info.samp_freq = pa_st->samp_freq;
	sync_info.bit_width = pa_st->bit_width;
	memcpy(sync_info.name, pa_st->sync_name, BT_IPS_NAME_MAX);
	if (ips_st && ips_st->pa_cbs)
		ips_st->pa_cbs->ips_synced(sync, &sync_info);

	ips_per_comp_adjust();

	if (0 == ips_st->pa_enable) {
		__ips_adv_stop(BT_IPS_START_ADV);
		__ips_adv_start(BT_IPS_RESTART_ADV);
	}
}

static void ips_term_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;
	int err;
	uint8_t comp_remove = 0;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	SYS_LOG_INF("Sync term %d\n", info->sid);
	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		SYS_LOG_ERR("sync %p",sync);
		return;
	}

	if (0 == pa_st->pa_synced) {
		SYS_LOG_ERR("pa_synced %d",pa_st->pa_synced);
		//return;
	}

	if (1 == pa_st->pa_synced) {
		//pa_st->pa_synced = 0;
		hostif_bt_le_per_adv_sync_delete(pa_st->adv_sync);
		if (ips_st->per_synced_count > 0) {
			ips_st->per_synced_count--;
		} else {
			SYS_LOG_ERR("per_synced_count %d",ips_st->per_synced_count);
		}

		SYS_LOG_INF("per_synced_count %d",ips_st->per_synced_count);

		if (1 == pa_st->pa_comp) {
			comp_remove = 1;
		}

		if (ips_st && ips_st->pa_cbs)
			ips_st->pa_cbs->ips_term(sync);

		if (0 == ips_st->per_synced_count) {
			ips_st->pa_comp = 1;
		} else if (1 == pa_st->pa_comp) {
			ips_per_comp_adjust();
		}
	}
	ips_remove_pa(sync);

	//if (info->reason != 0x16) {
		/*restart scan*/
	if (ips_st->ch_max == ips_st->per_synced_count + 1) {
		err = hostif_bt_le_scan_start(&ips_scan_params, NULL);
		if (err) {
			SYS_LOG_INF("Failed to enable periodic advertising (err %d)\n", err);
			return;
		}
		SYS_LOG_INF("scan start.");
	}
	//}
	SYS_LOG_INF(":");
}

static void ips_recv_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	SYS_LOG_INF("recv %d\n", info->sid);
	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		SYS_LOG_ERR("sync %p",sync);
		return;
	}

	if (buf && buf->len) {
		bt_data_parse(buf, filter_data_cb, pa_st);
	}
}

static struct bt_le_per_adv_sync_cb ips_sync_callbacks = {
	.synced = ips_sync_cb,
	.term = ips_term_cb,
	.recv = ips_recv_cb,
};

static int __ips_scan_start(void)
{
	int err;

	//return 0;
	hostif_bt_le_scan_cb_register((struct bt_le_scan_cb *)&ips_scan_callbacks);
	hostif_bt_le_per_adv_sync_cb_register((struct bt_le_per_adv_sync_cb *)&ips_sync_callbacks);

	err = hostif_bt_le_scan_start(&ips_scan_params, NULL);
	if (err) {
		SYS_LOG_INF("Failed to enable periodic advertising (err %d)\n", err);
		return -EALREADY;
	}

	return 0;
}

static int __ips_scan_stop(void)
{
	int i;
	struct ips_pa_st *pa_st;
	int err;

	err = hostif_bt_le_scan_stop();
	if (err) {
		SYS_LOG_INF("err: %d", err);
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		pa_st = ips_pa_st_by_id(i);
		if (pa_st) {
			hostif_bt_le_per_adv_sync_delete(pa_st->adv_sync);
			ips_remove_pa(pa_st->adv_sync);
			//if (ips_st->per_synced_count > 0)
			//	ips_st->per_synced_count--;
			//else
			//	SYS_LOG_ERR("per_synced_count : %d\n",ips_st->per_synced_count);
		}
	}

	hostif_bt_le_scan_cb_unregister((struct bt_le_scan_cb *)&ips_scan_callbacks);
	hostif_bt_le_per_adv_sync_cb_unregister((struct bt_le_per_adv_sync_cb *)&ips_sync_callbacks);
	return err;
}

static void ips_sort_wait_complete(struct k_work *work)
{
	SYS_LOG_INF("pa_enable %d .", ips_st->pa_enable);
	SYS_LOG_INF("remote_seq_max %d_%d_%d_%d .", ips_st->remote_seq_max,ips_st->adv_seq,ips_st->subs_number,ips_st->r_seq_subs_number);

	if (0 == ips_st->adv_enable)
		return;

	if ((ips_st->remote_seq_max < ips_st->adv_seq) ||
			((ips_st->subs_number < ips_st->r_seq_subs_number &&
					ips_st->adv_seq == ips_st->remote_seq_max))) {
		if (1 == ips_st->pa_enable) {
			SYS_LOG_ERR("status err, pa_enable %d.", ips_st->pa_enable);
		} else {
			__ips_adv_stop(BT_IPS_START_ADV);
			ips_st->pa_comp = 1;
			__ips_adv_start(BT_IPS_RESTART_ADV);
		}
	} else {
		ips_st->pa_comp = 0;
		//
	}
}

/** @brief 启动对讲功能, 开启广播和扫描
 *
 *  调用接口后会进行周期性广播和广播扫描.
 *
 *  @param out handle 生成的句柄
 *  @param in ips_info
 *  @param in ips_cb
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_start(void **handle,
				struct bt_ips_init_info *ips_info, struct bt_ips_cb *ips_cb)
{
	int i;

	if (is_st.searching) {
		SYS_LOG_ERR("need to stop searching.");
		return -EBUSY;
	}

	if (!handle || (*handle)) {
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (ips_st) {
		SYS_LOG_ERR("ips_st active.");
		return -EALREADY;
	}

	memset(&entity, 0, sizeof(struct ips_mgr_st));
	ips_st = &entity;
	if (ips_st->advertising) {
		SYS_LOG_INF("already");
		return -EALREADY;
	}
	ips_st->match_id = ips_info->match_id; // 匹配码, 4bytes 随机数
	SYS_LOG_INF("ips_st->match_id 0x%x.", ips_st->match_id);
	ips_st->adv_role = ips_info->role;
	ips_st->ch_max = ips_info->ch_max; //最多允许接收的音频通道数量
	if (ips_st->ch_max > BT_IPS_CHANNEL_MAX) {
		ips_st->ch_max = BT_IPS_CHANNEL_MAX;
	}
	ips_st->tx_codec = ips_info->tx_codec; //本机广播的编码格式
	ips_st->comp_ratio = ips_info->comp_ratio; // 编码压缩比
	ips_st->ch_mode = ips_info->ch_mode; //单声道或双声道
	ips_st->samp_freq = ips_info->samp_freq; //采样率8KHZ or 16KHZ
	ips_st->bit_width = ips_info->bit_width; // 位宽8bits or 16bits
	memcpy(ips_st->pa_name, ips_info->name, BT_IPS_NAME_MAX);
	le_ips_mac_addr_get(&ips_st->local_addr);

	ips_st->cnt_subscriber = ips_info->cnt_subscriber & 0x7;
	if (ips_st->cnt_subscriber <= BT_IPS_SUBS_MAX) {
		memcpy(&ips_st->subs_mac[0], &ips_info->subs_mac[0], ips_st->cnt_subscriber*6);
		for (i = 0; i < ips_st->cnt_subscriber; i++) {
			if (!memcmp(&ips_st->subs_mac[i], &ips_st->local_addr.a, 6)) {
				ips_st->subs_number = i;
				break;
			}
		}
	}

	SYS_LOG_INF(":\n");
	ips_st->pa_cbs = ips_cb;
	if (0 == ips_st->subs_number && BT_IPS_ROLE_INITIATOR == ips_st->adv_role) {
		ips_st->pa_comp = 1;
		SYS_LOG_INF("enter pa_comp %d.", ips_st->pa_comp);
		__ips_adv_start(BT_IPS_RESTART_ADV);
	} else {
		ips_st->pa_comp = 0;
		__ips_adv_start(BT_IPS_START_ADV);
	}
	
	__ips_scan_start();

	os_delayed_work_init(&ips_sort_wait_work, ips_sort_wait_complete);
	ips_st->advertising = 1;
	ips_st->ap_handle = (void *)&ips_st;
	*handle = ips_st->ap_handle;

	SYS_LOG_INF(":\n");
	return 0;
}

/** @brief 关闭对讲功能
 *
 *  调用接口后会关闭周期性广播和广播扫描.
 *
 *  @param in 操作句柄
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_stop(void *handle)
{
	if (!ips_st || !handle || ips_st->ap_handle != handle) {
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	__ips_adv_stop(BT_IPS_RESTART_ADV);
	__ips_scan_stop();
	ips_st->pa_comp = 0;
	ips_st->advertising = 0;
	ips_st->ap_handle = NULL;
	ips_st = NULL;
	return 0;
}

/** @brief 关闭对讲功能
 *
 *  调用接口后会进行周期性广播，并且扫描广播.
 *
 *  @param in 操作句柄
 *  @param in info
 *  @param in data 发送的纯编码数据. 闭麦时data is NULL
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_send_codec(void *handle, struct bt_ips_send_info *info, uint8_t *data)
{
	int i;
	uint8_t f_len;
	int err;

	if (!ips_st || !ips_st->pa_enable || !handle || !info || ips_st->ap_handle != handle) {
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (!data) {
		__ips_per_adv(BT_IPS_NULL_HEAD_TYPE, info->status , info->num_synced, NULL, 0);
		if (BT_IPS_STATUS_WORKING == info->status && info->num_synced > 0) {
			ips_st->ch_max = info->num_synced;
			if (info->num_synced >= ips_st->per_synced_count) {
				err = hostif_bt_le_scan_stop();
				//if (err) {
				//	SYS_LOG_INF("err: %d", err);
				//}
				SYS_LOG_INF("scan stop err: %d", err);
			} else {
				err = hostif_bt_le_scan_start(&ips_scan_params, NULL);
				//if (err) {
				//	SYS_LOG_INF("Failed to enable periodic advertising (err %d)\n", err);
				//}
				SYS_LOG_INF("scan start err: %d", err);
			}
		}
		return 0;
	}

	f_len = codec_frame_len_calc(ips_st->tx_codec);
	if (info->length%f_len) {
		SYS_LOG_ERR("length invalid %d.", info->length);
		return -EALREADY;
	}

	for (i = 0; i < (info->length/f_len); i++) {
		__ips_per_adv(BT_IPS_DATA_HEAD_TYPE, BT_IPS_STATUS_TALK , 0, data + i*f_len, f_len);
	}

	return 0;
}

int bt_manager_ips_search_open(struct bt_ips_search_init *info)
{
	if (is_st.searching) {
		SYS_LOG_ERR("searching %d.", is_st.searching);
		return -EBUSY;
	}

	if (ips_st) {
		SYS_LOG_ERR("ips_st active.");
		return -EALREADY;
	}

	memset(&is_st, 0, sizeof(struct ips_search_st));

	is_st.searching = 1;
	is_st.cur_role = info->role;
	is_st.match_id = info->match_id;
	SYS_LOG_INF("is_st.match_id 0x%x.", is_st.match_id);
	is_st.s_cb = info->s_cb;
	memcpy(is_st.cur_name, info->name, BT_IPS_NAME_MAX);

	le_ips_mac_addr_get(&is_st.local_addr);
#if 0
	printk("aaass %02X:%02X:%02X:%02X:%02X:%02X\n",
				is_st.local_addr.a.val[5], is_st.local_addr.a.val[4], is_st.local_addr.a.val[3],
				is_st.local_addr.a.val[2], is_st.local_addr.a.val[1], is_st.local_addr.a.val[0]);
#endif
	__ips_adv_start(BT_IPS_SEARCH_ADV);
	__ips_scan_start();
	return 0;
}

int bt_manager_ips_search_close(void)
{
	if (!is_st.searching) {
		SYS_LOG_ERR("searching %d.", is_st.searching);
		return -EALREADY;
	}
	is_st.searching = 0;
	__ips_scan_stop();
	__ips_adv_stop(BT_IPS_SEARCH_ADV);

	return 0;
}

uint8_t bt_manager_ips_status_get(void)
{
	if (is_st.searching) {
		return BT_IPS_STATUS_MATCHING;
	}

	if (ips_st) {
		return BT_IPS_STATUS_READY;
	}

	return 0;
}

