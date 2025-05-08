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
#define IPS_CREATE_SYNC_TIME		(15*1000)		/* 15s */

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
	uint8_t local_list[BT_IPS_SUBS_MAX];

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

struct ips_mgr_recv_st {
	uint32_t match_id; // 匹配码, 4bytes 随机数
	uint8_t ch_max; //最多允许接收的音频通道数量
	bt_addr_le_t local_addr;

	bt_addr_t subs_mac[BT_IPS_SUBS_MAX];
	uint8_t subs_number : 3;
	uint8_t cnt_subscriber : 3;
	// adv
	uint8_t all_list[BT_IPS_SUBS_MAX][BT_IPS_SUBS_MAX];
	//scan
	struct ips_pa_st pa_st[BT_IPS_CHANNEL_MAX];
	struct bt_ips_cb *pa_cbs;
	int rssi;
	struct bt_le_scan_param scan_param;
	u8_t per_synced_count;
	void *ap_recv_handle;
	uint8_t comp_number : 3;
	void *hci_comp_sync;
	uint8_t token_number;
	void *create_sync;
};

struct ips_mgr_adv_st {
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
	uint8_t adv_enable : 1;
	uint8_t pa_enable : 1;
	uint8_t pa_comp : 1;
	uint8_t adv_work_exist : 1;
	uint16_t adv_seq;

	void *ap_adv_handle;
	uint16_t remote_seq_max;
	uint8_t r_seq_subs_number : 3;
};


static struct ips_search_st is_st;
static struct ips_mgr_recv_st recv_entity;
static struct ips_mgr_adv_st adv_entity;
static struct ips_mgr_recv_st *ips_recv_st;
static struct ips_mgr_adv_st *ips_adv_st;
static os_delayed_work ips_adv_work;
uint8_t cur_adv_type;
static os_delayed_work ips_sort_wait_work;
static os_delayed_work ips_sync_create_work;
bool ips_sync_start = false;

struct ips_pkg_data_head {
	uint8_t seq; //总序号，所有PA包的序号，累加
	uint8_t type : 2; // 01: 空包 10: 数据包
	uint8_t method : 3; //
	uint8_t b_reserve : 3; //
	uint8_t status; //状态
	uint8_t local_list[BT_IPS_SUBS_MAX];
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
	uint8_t local_list[BT_IPS_SUBS_MAX];
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
	uint8_t res2 : 2;
	bt_addr_t subs_mac[BT_IPS_SUBS_MAX];
	uint8_t local_list[BT_IPS_SUBS_MAX];
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
	.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY /*| BT_LE_ADV_OPT_CODED*/ | BT_LE_ADV_OPT_NO_2M,
	.sid = BT_EXT_ADV_SID_BROADCAST,
};

static OS_MUTEX_DEFINE(search_mutex);
static OS_MUTEX_DEFINE(ips_mutex);
static OS_MUTEX_DEFINE(recv_mutex);
static OS_MUTEX_DEFINE(adv_mutex);
static OS_MUTEX_DEFINE(token_mutex);

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

	if (!sync)
		return -ESRCH;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	if (!ips_recv_st) {
		os_mutex_unlock(&ips_mutex);
		return -ESRCH;
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (recv_entity.pa_st[i].adv_sync == NULL) {
			recv_entity.pa_st[i].adv_sync = sync;
			//SYS_LOG_INF("a sync (%p) %p\n", sync,&ips_st->pa_st[i]);
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

	if (!sync)
		return -ESRCH;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	if (!ips_recv_st) {
		os_mutex_unlock(&ips_mutex);
		return -ESRCH;
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		if (recv_entity.pa_st[i].adv_sync == sync) {
			//ips_st->pa_st[i].adv_sync = NULL;
			//SYS_LOG_INF("r sync (%p) %p\n", sync,&ips_st->pa_st[i]);
			memset(&recv_entity.pa_st[i], 0 ,sizeof(struct ips_pa_st));
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

	if (!sync)
		return NULL;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	if (!ips_recv_st) {
		os_mutex_unlock(&ips_mutex);
		return NULL;
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = recv_entity.pa_st[i].adv_sync;
		//SYS_LOG_INF("g sync (%p) %p\n", adv_sync,&recv_entity.pa_st[i]);
		if (adv_sync == sync) {
			os_mutex_unlock(&ips_mutex);
			return &recv_entity.pa_st[i];
		}
	}

	os_mutex_unlock(&ips_mutex);
	return NULL;
}

static struct ips_pa_st *ips_pa_st_by_mac(bt_addr_le_t *addr)
{
	int i;
	struct bt_le_per_adv_sync *adv_sync;

	if (!addr)
		return NULL;

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	if (!ips_recv_st) {
		os_mutex_unlock(&ips_mutex);
		return NULL;
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = recv_entity.pa_st[i].adv_sync;
		//SYS_LOG_INF("adv_sync %p.\n",adv_sync);
		if (adv_sync &&
			!memcmp(&recv_entity.pa_st[i].per_addr.a, &addr->a, sizeof(bt_addr_t))) {
			os_mutex_unlock(&ips_mutex);
			return &recv_entity.pa_st[i];
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

	os_mutex_lock(&ips_mutex, OS_FOREVER);
	if (!ips_recv_st) {
		os_mutex_unlock(&ips_mutex);
		return NULL;
	}

	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		adv_sync = recv_entity.pa_st[i].adv_sync;
		if (adv_sync && index == i) {
			os_mutex_unlock(&ips_mutex);
			//SYS_LOG_INF("i sync (%p) %p\n", adv_sync,&recv_entity.pa_st[i]);
			return &recv_entity.pa_st[i];
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
		adv_data.match_id = adv_entity.match_id;
		adv_data.adv_role = adv_entity.adv_role;
		adv_data.tx_codec = adv_entity.tx_codec;
		adv_data.comp_ratio = adv_entity.comp_ratio;
		adv_data.ch_mode = adv_entity.ch_mode;
		adv_data.samp_freq = adv_entity.samp_freq;
		adv_data.bit_width = adv_entity.bit_width;
		adv_data.ch_max = adv_entity.ch_max;
		// SYS_LOG_INF("adv_data.ch_max: %d", adv_data.ch_max);
		adv_data.adv_status = BT_IPS_START_ADV;
		memcpy(adv_data.ips_name, adv_entity.pa_name, BT_IPS_NAME_MAX);
		cur_adv = adv_entity.ips_adv;
		adv_data.subs_number = adv_entity.subs_number;
		adv_data.cnt_subscriber = adv_entity.cnt_subscriber;

		if (adv_data.cnt_subscriber > 0 && adv_data.cnt_subscriber <= BT_IPS_SUBS_MAX) {
			//memcpy(&eadv[sizeof(struct ips_ext_adv_data)], &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
			memcpy(adv_data.subs_mac, &adv_entity.subs_mac[0], adv_data.cnt_subscriber*6);
		}
		adv_data.seq = adv_entity.adv_seq++;
	} else if (BT_IPS_RESTART_ADV == type) {
		adv_data.match_id = adv_entity.match_id;
		adv_data.adv_role = adv_entity.adv_role;
		adv_data.tx_codec = adv_entity.tx_codec;
		adv_data.comp_ratio = adv_entity.comp_ratio;
		adv_data.ch_mode = adv_entity.ch_mode;
		adv_data.samp_freq = adv_entity.samp_freq;
		adv_data.bit_width = adv_entity.bit_width;
		adv_data.ch_max = adv_entity.ch_max;
		adv_data.per_adv = adv_entity.pa_enable;
		adv_data.comp = adv_entity.pa_comp;
		// SYS_LOG_INF("adv_data.ch_max: %d", adv_data.ch_max);
		adv_data.adv_status = BT_IPS_RESTART_ADV;
		if (adv_entity.subs_number < BT_IPS_SUBS_MAX) {
			//os_mutex_lock(&recv_mutex, OS_FOREVER);
			if (ips_recv_st)
				memcpy(adv_data.local_list, recv_entity.all_list[adv_entity.subs_number], BT_IPS_SUBS_MAX);
			//os_mutex_unlock(&recv_mutex);
		}
		memcpy(adv_data.ips_name, adv_entity.pa_name, BT_IPS_NAME_MAX);
		cur_adv = adv_entity.ips_adv;
		adv_data.subs_number = adv_entity.subs_number;
		adv_data.cnt_subscriber = adv_entity.cnt_subscriber;
		if (adv_data.cnt_subscriber > 0 && adv_data.cnt_subscriber <= BT_IPS_SUBS_MAX) {
			//memcpy(&eadv[sizeof(struct ips_ext_adv_data)], &ips_st->subs_mac[0], adv_data.cnt_subscriber*6);
			memcpy(&adv_data.subs_mac[0], &adv_entity.subs_mac[0], adv_data.cnt_subscriber*6);
		}
		adv_data.seq = adv_entity.adv_seq++;
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
	//os_mutex_lock(&adv_mutex, OS_FOREVER);
	if (BT_IPS_SEARCH_ADV == cur_adv_type && 0 == is_st.searching) {
		//os_mutex_unlock(&adv_mutex);
		return;
	}

	if (!ips_adv_st && 
		(BT_IPS_START_ADV == cur_adv_type ||
		BT_IPS_RESTART_ADV == cur_adv_type)) {
		//os_mutex_unlock(&adv_mutex);
		return;
	}

	if (ips_adv_st && 0 == adv_entity.adv_enable) {
		//os_mutex_unlock(&adv_mutex);
		return;
	}
	__ips_ext_adv(cur_adv_type);
	os_delayed_work_submit(&ips_adv_work, 100);
	//os_mutex_unlock(&adv_mutex);
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
		data_head.seq = adv_entity.per_seq++;
		data_head.type = BT_IPS_DATA_HEAD_TYPE;
		data_head.status = BT_IPS_STATUS_TALK;
		data_head.codec = adv_entity.tx_codec;
		if (adv_entity.subs_number < BT_IPS_SUBS_MAX) {
			//os_mutex_lock(&recv_mutex, OS_FOREVER);
			if (ips_recv_st)
				memcpy(data_head.local_list, recv_entity.all_list[adv_entity.subs_number], BT_IPS_SUBS_MAX);
			//os_mutex_unlock(&recv_mutex);
		}
		f_len = codec_frame_len_calc(adv_entity.tx_codec);
		data_head.f_len = f_len;
		memcpy(ips_send_buf, &data_head, sizeof(struct ips_pkg_data_head));
		offset += sizeof(struct ips_pkg_data_head);
		f_num_max = codec_frame_num_calc(adv_entity.tx_codec);
		if (adv_entity.f_seq_end < adv_entity.f_seq_start) {
			f_num = 0xFF-adv_entity.f_seq_start+adv_entity.f_seq_end+2;
		} else {
			f_num = adv_entity.f_seq_end - adv_entity.f_seq_start+1;
		}
		if (adv_entity.last_f_num && f_num != adv_entity.last_f_num)
			SYS_LOG_ERR("f_num: %d_%d", f_num, adv_entity.last_f_num);
		if (f_num > f_num_max)
			SYS_LOG_ERR("f_num: %d_%d", f_num, f_num_max);

		if (1 == f_num) {
			SYS_LOG_INF("f_seq: %d_%d", adv_entity.f_seq_end, adv_entity.f_seq_start);
			if (!adv_entity.last_f_num) {
				f_num = 0;
				adv_entity.f_seq_end = 0;
				adv_entity.f_seq_start = 0;
			}
		}
		if (f_num == f_num_max) {
			SYS_LOG_INF("f_seq: %d_%d", adv_entity.f_seq_end, adv_entity.f_seq_start);
			for (i = 1;i < f_num; i++) {
				memcpy(ips_send_buf + offset,
					ips_send_buf + offset + (2 + f_len), f_len + 2);
				offset += (2 + f_len);
			}
			adv_entity.f_seq_start++;
			adv_entity.f_seq_end++;
			ips_send_buf[offset++] = adv_entity.f_seq_end;
			ips_send_buf[offset++] = f_len;
			memcpy(ips_send_buf + offset, data, f_len);
			offset += f_len;
		} else {
			adv_entity.f_seq_end++;
			if (0 == adv_entity.last_f_num)
				adv_entity.f_seq_start++;
			offset += (f_len+2)*f_num;
			ips_send_buf[offset++] = adv_entity.f_seq_end;
			ips_send_buf[offset++] = f_len;
			memcpy(ips_send_buf + offset, data, f_len);
			offset += f_len;
		}

		if (adv_entity.f_seq_end < adv_entity.f_seq_start) {
			adv_entity.last_f_num = 0xFF-adv_entity.f_seq_start+adv_entity.f_seq_end+2;
		} else {
			adv_entity.last_f_num = adv_entity.f_seq_end - adv_entity.f_seq_start+1;
		}
		SYS_LOG_INF("f_seq: %d_%d %d", adv_entity.f_seq_end, adv_entity.f_seq_start,adv_entity.last_f_num);
		ad_data[items].data = ips_send_buf;
	} else {
		status_head.seq = adv_entity.per_seq++;
		status_head.type = BT_IPS_NULL_HEAD_TYPE;
		status_head.status = status;
		status_head.byte1 = 0x30;
		status_head.byte2 = 0x85;
		status_head.role = adv_entity.adv_role;
		status_head.num_synced = num;
		if (adv_entity.subs_number < BT_IPS_SUBS_MAX) {
			//os_mutex_lock(&recv_mutex, OS_FOREVER);
			if (ips_recv_st)
				memcpy(status_head.local_list, recv_entity.all_list[adv_entity.subs_number], BT_IPS_SUBS_MAX);
			//os_mutex_unlock(&recv_mutex);
		}
		offset += sizeof(struct ips_pkg_status_head);
		ad_data[items].data = (uint8_t *)&status_head;
		adv_entity.last_f_num = 0;
		adv_entity.f_seq_start = 0;
		adv_entity.f_seq_end = 0;
	}

	ad_data[items].type = BT_DATA_VS_IPS_CODEC;
	ad_data[items].data_len = offset;

	items++;

	err = hostif_bt_le_per_adv_set_data(adv_entity.ips_adv, ad_data, items);
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

	if (ips_adv_st && 1 == adv_entity.adv_enable) {
		SYS_LOG_INF("adv already start: %d", adv_entity.adv_enable);
		return -EBUSY;
	}

	if (BT_IPS_SEARCH_ADV == type) {
		//ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		ips_ext_adv_params.options |= BT_LE_ADV_OPT_CODED;
		//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY;// | BT_LE_ADV_OPT_NO_2M;
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(is_st.s_adv));
		cur_adv = is_st.s_adv;
		is_st.seq = 0;
	} else if (BT_IPS_START_ADV == type) {
		//if (ips_st->ch_max < 2) {
			ips_ext_adv_params.options |= BT_LE_ADV_OPT_CODED;
		//} else {
		//	ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		//}
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(adv_entity.ips_adv));
		cur_adv = adv_entity.ips_adv;
		adv_entity.adv_seq = 0;
	} else if (BT_IPS_RESTART_ADV == type) {
		//if (ips_st->ch_max < 2) {
			//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_CODED;// | BT_LE_ADV_OPT_NO_2M;
			ips_ext_adv_params.options |= BT_LE_ADV_OPT_CODED;
			//ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED); // need to modify
		//} else {
			//ips_ext_adv_params.options = BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY;// | BT_LE_ADV_OPT_NO_2M;
		//	ips_ext_adv_params.options &= (~BT_LE_ADV_OPT_CODED);
		//}
		err = hostif_bt_le_ext_adv_create(&ips_ext_adv_params, NULL, &(adv_entity.ips_adv));
		cur_adv = adv_entity.ips_adv;
		adv_entity.adv_seq = 0;
	}

	/* Create a non-connectable non-scannable advertising set */
	if (err) {
		SYS_LOG_ERR("Failed to create advertising set (err %d)\n", err);
		return -EALREADY;
	}

	if (BT_IPS_RESTART_ADV == type) {
		adv_entity.pa_enable = 1;
		/* Set periodic advertising parameters */
		per_adv_params.interval_min = 16; //20ms
		per_adv_params.interval_max = 16; //20ms
		per_adv_params.options = BT_LE_PER_ADV_OPT_NONE;

		SYS_LOG_INF("per_param:%d,%d \n", per_adv_params.interval_min, per_adv_params.interval_max);
		err = hostif_bt_le_per_adv_set_param(adv_entity.ips_adv, &per_adv_params);
		if (err) {
			SYS_LOG_ERR("Failed to set periodic advertising parameters (err %d)\n", err);
			return -EALREADY;
		}
		/* Enable Periodic Advertising */
		err = hostif_bt_le_per_adv_start(adv_entity.ips_adv);
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

	if (ips_adv_st) {
		if (1 == adv_entity.adv_work_exist) {
			adv_entity.adv_work_exist = 0;
			os_delayed_work_cancel(&ips_sort_wait_work);	
		}
		adv_entity.adv_enable = 1;
		adv_entity.remote_seq_max = 0;
		adv_entity.r_seq_subs_number = 0;
		adv_entity.adv_seq = 0;
	}
	is_st.seq = 0;
	os_delayed_work_init(&ips_adv_work, ips_adv_active_loop);
	os_delayed_work_cancel(&ips_adv_work);
	os_delayed_work_submit(&ips_adv_work, 100);
	cur_adv_type = type;

	return 0;
}

static int __ips_adv_stop(u8_t type)
{
	int err;
	struct bt_le_ext_adv *cur_adv = NULL;

	if (ips_adv_st && 0 == adv_entity.adv_enable) {
		SYS_LOG_INF("adv already stop: %d", adv_entity.adv_enable);
		return -EALREADY;
	}

	if (BT_IPS_SEARCH_ADV == type) {
		cur_adv = is_st.s_adv;
	} else if (BT_IPS_START_ADV == type) {
		os_delayed_work_cancel(&ips_sort_wait_work);
		adv_entity.adv_work_exist = 0;
		cur_adv = adv_entity.ips_adv;
		adv_entity.pa_enable = 0;
		adv_entity.adv_enable = 0;
	} else if (BT_IPS_RESTART_ADV == type) {
		cur_adv = adv_entity.ips_adv;
		adv_entity.pa_enable = 0;
		adv_entity.adv_enable = 0;
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

static void voice_data_token_adjust(struct ips_pa_st *pa_st, bool enable)
{
	os_mutex_lock(&token_mutex, OS_FOREVER);
	uint8_t number;
	uint8_t token_number;

	if (!ips_recv_st) {
		os_mutex_unlock(&token_mutex);
		return;
	}

	//SYS_LOG_INF("token %p %d %d 0x%x", pa_st, enable, recv_entity.subs_number, recv_entity.token_number);
	token_number = recv_entity.token_number;
	if (pa_st) {
		number = pa_st->subs_number;
	} else {
		number = recv_entity.subs_number;
	}

	if (true == enable) {
		if (recv_entity.token_number & 0x80) {
			os_mutex_unlock(&token_mutex);
			return;
		}

		recv_entity.token_number = 0;
		if (number < BT_IPS_SUBS_MAX) {
			recv_entity.token_number |= 0x80;
			recv_entity.token_number |= (1<<number);
		}
	} else {
		if ((recv_entity.token_number&0xF)&(1<<number)) {
			recv_entity.token_number = 0;
		}
	}

	if (recv_entity.ch_max > 1 && token_number != recv_entity.token_number && recv_entity.pa_cbs->ips_token_get) {
		recv_entity.pa_cbs->ips_token_get(pa_st, enable);
	}
	//SYS_LOG_INF("token_number 0x%x", recv_entity.token_number);

	os_mutex_unlock(&token_mutex);
	return;
}

static uint8_t voice_token_status_get(void)
{
	uint8_t token_number = 0;

	os_mutex_lock(&token_mutex, OS_FOREVER);
	if (ips_recv_st) {
		token_number = recv_entity.token_number;
	}
	os_mutex_unlock(&token_mutex);
	return token_number;
}


static void extract_codec(struct ips_pa_st *pa_st, const uint8_t *buf, uint16_t len)
{
	struct bt_ips_recv_info info;
	uint16_t offset = 0;
	uint16_t seq, cur_seq;
	struct ips_pkg_data_head *data_head;
	struct ips_pkg_status_head *status_head = (struct ips_pkg_status_head *)buf;
	uint8_t f_len;
	
	if (len < sizeof(struct ips_pkg_status_head)) {
		SYS_LOG_ERR("len %d", len);
		return;
	}
	SYS_LOG_INF("pa: %d_%d", status_head->seq, pa_st->seq_pa);

	if (pa_st->subs_number < BT_IPS_SUBS_MAX) {
		if (ips_recv_st)
			memcpy(recv_entity.all_list[pa_st->subs_number], status_head->local_list, BT_IPS_SUBS_MAX);
	}

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
		if (ips_recv_st && recv_entity.pa_cbs) {
			recv_entity.pa_cbs->ips_status(pa_st->adv_sync, status_head->status, status_head->num_synced);
		}
		pa_st->cur_pkg_num = 0;
		pa_st->first_pkg_num = 0;
		voice_data_token_adjust(pa_st, false);
		return;
	}

	if (BT_IPS_DATA_HEAD_TYPE != status_head->type ||
			sizeof(struct ips_pkg_data_head) >= len) {
		SYS_LOG_ERR("n_pkg len: %d", len);
		return;
	}

	voice_data_token_adjust(pa_st, true);
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

		if (ips_recv_st && recv_entity.pa_cbs) {
			info.length = f_len;
			info.pkt_num = pa_st->cur_pkg_num;

			if (1 == recv_entity.ch_max) {
				recv_entity.pa_cbs->ips_recv_codec(
					pa_st->adv_sync, &info, (uint8_t *)&buf[offset]);
			} else {
				if ((voice_token_status_get()&0x80) &&
					((voice_token_status_get()&0xF)&(1<<pa_st->subs_number))) {
					recv_entity.pa_cbs->ips_recv_codec(
						pa_st->adv_sync, &info, (uint8_t *)&buf[offset]);
				}
			}
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
		if (1 == is_st.searching || !ips_recv_st) {
			return true;
		}
		pa_st = (struct ips_pa_st *)user_data;
		extract_codec(pa_st, data->data, data->data_len);
		return false;

	default:
		return true;
	}
}

static void ips_sync_create_timeout(os_work *work)
{
	SYS_LOG_INF("ips_sync_create_timeout.");

	if (!ips_recv_st) {
		return;
	}

	if (recv_entity.create_sync) {
		hostif_bt_le_per_adv_sync_delete(recv_entity.create_sync);
		ips_remove_pa(recv_entity.create_sync);
		recv_entity.create_sync = NULL;
		/*restart scan*/
		//__ips_scan_start();

		if (ips_adv_st &&
			0 == adv_entity.pa_enable && 
			BT_IPS_START_ADV == cur_adv_type) {
			__ips_adv_start(cur_adv_type);
		}
	}
	SYS_LOG_INF(":");
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

	SYS_LOG_INF("rssi %d, 0x%x.\n",info->rssi, info->addr->a.val[0]);
	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (!ips_recv_st && 0 == is_st.searching) {
		os_mutex_unlock(&recv_mutex);
		return;
	}

	memset(&adv_data, 0, sizeof(adv_data));
	net_buf_simple_save(buf, &state);
	bt_data_parse(buf, filter_data_cb, (void *)&adv_data);
	net_buf_simple_restore(buf, &state);

	if (adv_data.s_uuid != IPS_SERIVCE_UUID) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_INF("Unknow service data.");
		return;
	}
	if (adv_data.s_ver != IPS_SERIVCE_VERSION) {
		SYS_LOG_INF("Unknow service ver.");
	}

	if (is_st.searching) {
		if (adv_data.match_id != is_st.match_id) {
			os_mutex_unlock(&recv_mutex);
			SYS_LOG_INF("Unknow match id.");
			return;
		}

		if (BT_IPS_ROLE_INITIATOR == is_st.cur_role &&
			(BT_IPS_ROLE_SUBSCRIBER == adv_data.adv_role)) {
			if (ips_search_device_get((bt_addr_le_t *)info->addr)) {
				os_mutex_unlock(&recv_mutex);
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
				os_mutex_unlock(&recv_mutex);
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
		os_mutex_unlock(&recv_mutex);
		return;
	}

	if (!ips_recv_st) {
		os_mutex_unlock(&recv_mutex);
		return;
	}
	if (adv_data.match_id != recv_entity.match_id) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_INF("Unknow match id.");
		return;
	}

	if (BT_IPS_SEARCH_ADV == adv_data.adv_status) {
		os_mutex_unlock(&recv_mutex);
		return;
	}

	if (ips_pa_st_by_mac((bt_addr_le_t *)info->addr)) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_INF(" pa exist.");
		return;
	}

	if (BT_IPS_CHANNEL_MAX == ips_pa_sync_num()) {
		os_mutex_unlock(&recv_mutex);
		return;
	}
	os_mutex_unlock(&recv_mutex);

	//os_mutex_lock(&adv_mutex, OS_FOREVER);
	if (0 == adv_data.per_adv && ips_adv_st && 0 == adv_entity.pa_enable) {
		SYS_LOG_INF("remote_seq_max %d_%d_%d_%d .", adv_entity.remote_seq_max, adv_entity.adv_seq, adv_entity.subs_number,adv_data.subs_number);
		if (adv_entity.remote_seq_max < adv_data.seq) {
			adv_entity.remote_seq_max = adv_data.seq;
			adv_entity.r_seq_subs_number = adv_data.subs_number;
			if (0 == adv_entity.adv_work_exist) {
				adv_entity.adv_work_exist = 1;
				os_delayed_work_cancel(&ips_sort_wait_work);
				os_delayed_work_submit(&ips_sort_wait_work, 3000);
			}
		}
		//os_mutex_unlock(&adv_mutex);
		return;
	}
	//os_mutex_unlock(&adv_mutex);
	if (0 == adv_data.per_adv) {
		return;
	}

	if (info->interval) {
		SYS_LOG_INF("Creating Periodic Advertising Sync \n");
		bt_addr_le_copy(&sync_create_param.addr, info->addr);
		sync_create_param.options = 0;
		sync_create_param.sid = info->sid;
		sync_create_param.skip = 0;
		sync_create_param.timeout = 0xaa;
		err = hostif_bt_le_per_adv_sync_create(&sync_create_param, &(pa_sync));
		if (err) {
			SYS_LOG_ERR("Failed to create sync (err %d)\n", err);
			return;
		}

		//os_mutex_lock(&adv_mutex, OS_FOREVER);
		if (ips_adv_st && 0 == adv_entity.pa_enable) {
			__ips_adv_stop(cur_adv_type);
		}
		//os_mutex_unlock(&adv_mutex);

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
			os_delayed_work_submit(&ips_sync_create_work, IPS_CREATE_SYNC_TIME);
			recv_entity.create_sync = pa_sync;
		}
	}
}

static struct bt_le_scan_cb ips_scan_callbacks = {
	.recv = ips_scan_recv,
};
#if 0
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
#endif
static int ips_per_comp_addr_set(struct bt_le_per_adv_sync *sync, bt_addr_le_t *le_addr)
{
	int i;
	uint8_t subs_number = BT_IPS_SUBS_MAX + 1;
	int ret = -1;
	uint8_t of = 0;
	uint8_t en = 0;
	struct ips_pa_st *pa_st;

	if (!ips_recv_st) {
		return 0;
	}

	if (sync == recv_entity.hci_comp_sync) {
		//SYS_LOG_INF(" ");
		return 0;
	}

	if (!sync) {
		pa_st = ips_pa_st_get(recv_entity.hci_comp_sync);
		if (pa_st && pa_st->pa_synced) {
			SYS_LOG_INF("comp_clr %p.",recv_entity.hci_comp_sync);
			hostif_bt_le_per_adv_sync_comp_set(recv_entity.hci_comp_sync, false, 0, 0);
			//hostif_bt_le_per_adv_sync_comp_set(recv_entity.hci_comp_sync, false, 0);
		}
		recv_entity.hci_comp_sync = NULL;
		return 0;
	}

	for (i = 0; i < recv_entity.cnt_subscriber; i++) {
		if (!memcmp(&recv_entity.subs_mac[i], &le_addr->a, 6)) {
			subs_number = i;
			ret = 0;
			break;
		}
	}
	SYS_LOG_INF("c_subs %d_%d_%d.",recv_entity.cnt_subscriber, subs_number,ret);
	if (recv_entity.cnt_subscriber < 2) {
		SYS_LOG_ERR("c_subs %d_%d_%d.",recv_entity.cnt_subscriber, subs_number,ret);
		return ret;
	}
	if (0 == ret) {
		if (subs_number > recv_entity.subs_number) {
			of = subs_number-recv_entity.subs_number;
			of |= 0x80;
			en = true;
			recv_entity.hci_comp_sync = sync;
		} else if (subs_number < recv_entity.subs_number) {
			of = recv_entity.subs_number-subs_number;
			en = true;
			recv_entity.hci_comp_sync = sync;
		} else {
			of = 0;
			en = false;
			recv_entity.hci_comp_sync = NULL;
		}
		SYS_LOG_INF("comp_set %p_%d_%d.",sync, recv_entity.cnt_subscriber - 1, of);
		hostif_bt_le_per_adv_sync_comp_set(sync, en, recv_entity.cnt_subscriber - 1, of);
		//hostif_bt_le_per_adv_sync_comp_set(sync, en, of);
	}

	return ret;
}

static int ips_per_comp_adjust(void)
{
	// step 1 : 补全网格all_list
	int i, j, k;
	struct ips_pa_st *pa_st;
	uint8_t calc_list = 0;
	uint8_t sync_min_num = 0xff, all_min_num = 0xff, comp_num = 0xff;
	
	if (!ips_recv_st) {
		return 0;
	}

	printk("be : \n");
	for (i = 0, j = 0; i < BT_IPS_SUBS_MAX; i++) {
		printk("%d: ", i);
		for (j = 0; j < BT_IPS_SUBS_MAX; j++) {
			printk(" %d", recv_entity.all_list[i][j]);
		}
		printk("\n");
	}
	printk("\n");

	memset(recv_entity.all_list[recv_entity.subs_number], 0, BT_IPS_SUBS_MAX);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		pa_st = ips_pa_st_by_id(i);
		if (pa_st && pa_st->pa_synced && pa_st->subs_number < BT_IPS_SUBS_MAX) {
			//SYS_LOG_INF("i %d subs_number %d.", i, pa_st->subs_number);
			//SYS_LOG_INF("local_list %d %d %d %d.", 
			//	pa_st->local_list[0], pa_st->local_list[1], pa_st->local_list[2], pa_st->local_list[3]);
			calc_list |= (1 << pa_st->subs_number);
			recv_entity.all_list[recv_entity.subs_number][pa_st->subs_number] = pa_st->local_list[pa_st->subs_number];
		}
	}
	recv_entity.all_list[recv_entity.subs_number][recv_entity.subs_number] = calc_list;

	for (i = 0, j = 0; i < BT_IPS_SUBS_MAX; i++) {
		for (j = 0; j < BT_IPS_CHANNEL_MAX; j++) {
			pa_st = ips_pa_st_by_id(j);
			if (0 == recv_entity.all_list[recv_entity.subs_number][i] && pa_st && pa_st->pa_synced && pa_st->local_list[i]) {
			//	SYS_LOG_INF("i %d j %d subs_number %d %d.", i, j, pa_st->subs_number, ips_st->all_list[ips_st->subs_number][i]);
			//	SYS_LOG_INF("pa_st->local_list %d %d %d %d.", 
			//		pa_st->local_list[0], pa_st->local_list[1], pa_st->local_list[2], pa_st->local_list[3]);
				recv_entity.all_list[recv_entity.subs_number][i] = pa_st->local_list[i];
			}
		}
	}

	printk("af : \n");
	for (i = 0, j = 0; i < BT_IPS_SUBS_MAX; i++) {
		printk("i: ");
		for (j = 0; j < BT_IPS_SUBS_MAX; j++) {
			printk(" %d", recv_entity.all_list[i][j]);
		}
		printk("\n");
	}
	printk("\n");

	// step 2 : 遍历网格，设置基准
	if (0 != recv_entity.all_list[recv_entity.subs_number][recv_entity.subs_number]) {
		calc_list = recv_entity.all_list[recv_entity.subs_number][recv_entity.subs_number];
		for (j = 0; j < BT_IPS_SUBS_MAX; j++) {
			if ((calc_list >> j) & 0x1) {
				sync_min_num = j;
				//SYS_LOG_INF("sync_min_num %d.", sync_min_num);
				break;
			}
		}
	}

	all_min_num = sync_min_num;
	comp_num = sync_min_num;
	for (i = 0, j = 0, k = 0; i < BT_IPS_SUBS_MAX; i++) {
		//SYS_LOG_INF("i %d subs_number %d %d.", i, ips_st->subs_number, ips_st->all_list[ips_st->subs_number][i]);
		if (0 != recv_entity.all_list[recv_entity.subs_number][i]) {
			calc_list = recv_entity.all_list[recv_entity.subs_number][i];
			for (j = 0; j < BT_IPS_SUBS_MAX; j++) {
				if ((calc_list >> j) & 0x1) {
					//all_min_num = MIN(all_min_num, j);
					//if (all_min_num < ips_st->subs_number) {
					//	comp_num = i;
					//}
					#if 1
					if (j < all_min_num) {
						all_min_num = j;
						for (k = 0; k < BT_IPS_CHANNEL_MAX; k++) {
							pa_st = ips_pa_st_by_id(k);
							if (pa_st && pa_st->pa_synced && i == pa_st->subs_number) {
								//SYS_LOG_INF("i %d j %d subs_number %d %d.", i, j, pa_st->subs_number, ips_st->all_list[ips_st->subs_number][i]);
								//SYS_LOG_INF("pa_st->local_list %d %d %d %d.", 
								//	pa_st->local_list[0], pa_st->local_list[1], pa_st->local_list[2], pa_st->local_list[3]);
								comp_num = pa_st->subs_number;
								break;
							}  else {
								pa_st = NULL;
							}
						}
					}
					#endif
					break;
				}
			}
		}
	}

	if (recv_entity.subs_number == all_min_num) {
		SYS_LOG_INF("cur is comp %d.", all_min_num);
		comp_num = 0xff;
	} else if (sync_min_num < recv_entity.subs_number) {

	} else if (sync_min_num == recv_entity.subs_number) {
		SYS_LOG_ERR("ERR sync_min_num %d.", sync_min_num);
		comp_num = 0xff;
	} else {
	}

	//SYS_LOG_INF("comp_num %d sync_min_num %d.", comp_num, sync_min_num);
	for (i = 0; i < BT_IPS_CHANNEL_MAX; i++) {
		pa_st = ips_pa_st_by_id(i);
		if (pa_st && pa_st->pa_synced && pa_st->subs_number == comp_num) {
			SYS_LOG_INF("i %d comp_num %d.", i, pa_st->subs_number);
			break;
		} else {
			pa_st = NULL;
		}
	}

	if (pa_st) {
		if (ips_per_comp_addr_set(pa_st->adv_sync, &pa_st->per_addr)) {
			SYS_LOG_ERR("pa_st %p", pa_st);
		}
	} else {
		ips_per_comp_addr_set(NULL, NULL);
	}

	if (0xff != all_min_num) {
		recv_entity.comp_number = all_min_num;
	} else {
		recv_entity.comp_number = recv_entity.subs_number;
	}

	return 0;
}

#if 0
static uint8_t ips_comp_number_get(void)
{
	// 获取网格中原始基准序号
	return 0;
}
#endif

static void ips_sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;
	struct bt_ips_sync_info sync_info;
	int err = 0;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (sync == recv_entity.create_sync) {
		os_delayed_work_cancel(&ips_sync_create_work);
		recv_entity.create_sync = NULL;
	}
	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("sync %p",sync);
		return;
	}

	if (pa_st->pa_synced) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("pa_synced %p",pa_st->pa_synced);
		return;
	}
	pa_st->pa_synced = 1;

	recv_entity.per_synced_count++;
	if (recv_entity.per_synced_count > recv_entity.ch_max) {
		SYS_LOG_ERR(":%d,%d \n",recv_entity.per_synced_count,recv_entity.ch_max);
	}

	if (recv_entity.ch_max == recv_entity.per_synced_count) {
		err = hostif_bt_le_scan_stop();
		//if (err) {
		//	SYS_LOG_INF("err: %d", err);
		//}
		SYS_LOG_INF("scanstop err: %d", err);
	}
	SYS_LOG_INF("per_synced_count %d",recv_entity.per_synced_count);

	sync_info.role = pa_st->role;
	sync_info.rx_codec = pa_st->rx_codec;
	sync_info.comp_ratio = pa_st->comp_ratio;
	sync_info.ch_mode = pa_st->ch_mode;
	sync_info.samp_freq = pa_st->samp_freq;
	sync_info.bit_width = pa_st->bit_width;
	memcpy(sync_info.name, pa_st->sync_name, BT_IPS_NAME_MAX);
	if (ips_recv_st && recv_entity.pa_cbs)
		recv_entity.pa_cbs->ips_synced(sync, &sync_info);

	os_mutex_unlock(&recv_mutex);
}

static void ips_term_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;
	int err;
	//uint8_t comp_remove = 0;
	uint8_t comp_number = 0;
	uint8_t s_number = 0;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	SYS_LOG_INF("Sync term %d\n", info->sid);

	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (sync == recv_entity.create_sync) {
		os_delayed_work_cancel(&ips_sync_create_work);
		recv_entity.create_sync = NULL;
	}
	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("sync %p",sync);
		return;
	}

	if (0 == pa_st->pa_synced) {
		SYS_LOG_ERR("pa_synced %d",pa_st->pa_synced);
		ips_remove_pa(sync);
		if (ips_adv_st &&
			0 == adv_entity.pa_enable && 
			BT_IPS_START_ADV == cur_adv_type) {
			__ips_adv_start(cur_adv_type);
		}
	}

	if (1 == pa_st->pa_synced) {
		//pa_st->pa_synced = 0;
		hostif_bt_le_per_adv_sync_delete(pa_st->adv_sync);
		if (recv_entity.per_synced_count > 0) {
			recv_entity.per_synced_count--;
		} else {
			SYS_LOG_ERR("per_synced_count %d",recv_entity.per_synced_count);
		}

		SYS_LOG_INF("per_synced_count %d",recv_entity.per_synced_count);


		if (ips_recv_st && recv_entity.pa_cbs)
			recv_entity.pa_cbs->ips_term(sync);

		s_number = pa_st->subs_number;
		voice_data_token_adjust(pa_st, false);
		if (sync == recv_entity.hci_comp_sync) {
			SYS_LOG_INF("comp_clr %p.",sync);
			recv_entity.hci_comp_sync = NULL;
		}
		ips_remove_pa(sync);
		comp_number = recv_entity.comp_number;
		ips_per_comp_adjust();
		if (s_number < BT_IPS_SUBS_MAX) {
			memset(recv_entity.all_list[s_number], 0, BT_IPS_SUBS_MAX);
		}

		if (0 == recv_entity.per_synced_count) {
			SYS_LOG_INF("comp_number %d %d.", comp_number,recv_entity.subs_number);
			if (comp_number == recv_entity.subs_number) {
				// pa not stop
			} else {
				//
				os_mutex_lock(&adv_mutex, OS_FOREVER);
				__ips_adv_stop(cur_adv_type);
				__ips_adv_start(BT_IPS_START_ADV);
				os_mutex_unlock(&adv_mutex);
			}
		}

		if (recv_entity.ch_max == recv_entity.per_synced_count + 1) {
			err = hostif_bt_le_scan_start(&ips_scan_params, NULL);
			if (err) {
				SYS_LOG_INF("Failed to enable periodic advertising (err %d)\n", err);
			}
			SYS_LOG_INF("scan start.");
		}
	}

	SYS_LOG_INF(":");
	os_mutex_unlock(&recv_mutex);
}

static void ips_recv_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	struct ips_pa_st *pa_st;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	SYS_LOG_INF("0x%x recv %d rssi %d.\n", info->addr->a.val[0], info->sid, info->rssi);
	int prio;

	prio = os_thread_priority_get(os_current_get());
	if (prio >= 0) {
		os_thread_priority_set(os_current_get(), -1);
	}

	//os_mutex_lock(&recv_mutex, OS_FOREVER);
	pa_st = ips_pa_st_get(sync);
	if (!pa_st) {
		//os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("sync %p",sync);
		goto recv_exit;
		//return;
	}

	if (buf && buf->len) {
		bt_data_parse(buf, filter_data_cb, pa_st);
	}

	if (!ips_recv_st) {
		goto recv_exit;
	}

	if (pa_st->subs_number < BT_IPS_SUBS_MAX) {
		memcpy(pa_st->local_list, recv_entity.all_list[pa_st->subs_number], BT_IPS_SUBS_MAX);
	}

	ips_per_comp_adjust();
	//os_mutex_unlock(&recv_mutex);

	//os_mutex_lock(&adv_mutex, OS_FOREVER);
	if (ips_adv_st && 1 == adv_entity.advertising && 0 == adv_entity.pa_enable) {
		__ips_adv_stop(cur_adv_type);
		__ips_adv_start(BT_IPS_RESTART_ADV);
	}
	//os_mutex_unlock(&adv_mutex);

recv_exit:
	if (prio >= 0) {
		os_thread_priority_set(os_current_get(), prio);
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
	//os_mutex_lock(&adv_mutex, OS_FOREVER);

	if (!ips_adv_st) {
		SYS_LOG_ERR("ips_adv_st %p.", ips_adv_st);
		return;
	}
	
	SYS_LOG_INF("pa_enable %d .", adv_entity.pa_enable);
	SYS_LOG_INF("remote_seq_max %d_%d_%d_%d .", adv_entity.remote_seq_max,adv_entity.adv_seq,adv_entity.subs_number,adv_entity.r_seq_subs_number);

	if (0 == adv_entity.adv_enable) {
		//os_mutex_unlock(&adv_mutex);
		return;
	}

	if ((adv_entity.remote_seq_max < adv_entity.adv_seq) ||
			((adv_entity.subs_number < adv_entity.r_seq_subs_number &&
					adv_entity.adv_seq == adv_entity.remote_seq_max))) {
		if (1 == adv_entity.pa_enable) {
			SYS_LOG_ERR("status err, pa_enable %d.", adv_entity.pa_enable);
		} else {
			__ips_adv_stop(BT_IPS_START_ADV);
			__ips_adv_start(BT_IPS_RESTART_ADV);
		}
	} else {
		//
	}
	//os_mutex_unlock(&adv_mutex);
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

	if (!handle || (*handle) || !ips_info|| !ips_cb) {
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (0 == ips_info->match_id || 
			0 == ips_info->ch_max || 
			ips_info->ch_max > BT_IPS_CHANNEL_MAX ||
			0 == ips_info->cnt_subscriber || 
			ips_info->cnt_subscriber > BT_IPS_SUBS_MAX) {
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (ips_recv_st || ips_adv_st) {
		SYS_LOG_ERR("ips_st active.");
		return -EALREADY;
	}
	/************adv start***********/
	os_mutex_lock(&adv_mutex, OS_FOREVER);
	memset(&adv_entity, 0, sizeof(struct ips_mgr_adv_st));
	ips_adv_st = &adv_entity;
	if (ips_adv_st->advertising) {
		os_mutex_unlock(&adv_mutex);
		SYS_LOG_INF("already");
		return -EALREADY;
	}
	ips_adv_st->match_id = ips_info->match_id; // 匹配码, 4bytes 随机数
	ips_adv_st->adv_role = ips_info->role;
	ips_adv_st->ch_max = ips_info->ch_max; //最多允许接收的音频通道数量
	if (ips_adv_st->ch_max > BT_IPS_CHANNEL_MAX) {
		ips_adv_st->ch_max = BT_IPS_CHANNEL_MAX;
	}
	SYS_LOG_INF("ips_st->match_id 0x%x %d.", ips_adv_st->match_id, ips_adv_st->ch_max);
	ips_adv_st->tx_codec = ips_info->tx_codec; //本机广播的编码格式
	ips_adv_st->comp_ratio = ips_info->comp_ratio; // 编码压缩比
	ips_adv_st->ch_mode = ips_info->ch_mode; //单声道或双声道
	ips_adv_st->samp_freq = ips_info->samp_freq; //采样率8KHZ or 16KHZ
	ips_adv_st->bit_width = ips_info->bit_width; // 位宽8bits or 16bits
	memcpy(ips_adv_st->pa_name, ips_info->name, BT_IPS_NAME_MAX);
	le_ips_mac_addr_get(&ips_adv_st->local_addr);

	ips_adv_st->cnt_subscriber = ips_info->cnt_subscriber & 0x7;
	if (ips_adv_st->cnt_subscriber <= BT_IPS_SUBS_MAX) {
		memcpy(&ips_adv_st->subs_mac[0], &ips_info->subs_mac[0], ips_adv_st->cnt_subscriber*6);
		for (i = 0; i < ips_adv_st->cnt_subscriber; i++) {
			if (!memcmp(&ips_adv_st->subs_mac[i], &ips_adv_st->local_addr.a, 6)) {
				ips_adv_st->subs_number = i;
				break;
			}
		}
	}
	if (0 == ips_adv_st->subs_number && BT_IPS_ROLE_INITIATOR == ips_adv_st->adv_role) {
		SYS_LOG_INF("cur comp.");
		__ips_adv_start(BT_IPS_RESTART_ADV);
	} else {
		__ips_adv_start(BT_IPS_START_ADV);
	}
	ips_adv_st->advertising = 1;
	ips_adv_st->ap_adv_handle = (void *)&recv_entity; //only handle
	SYS_LOG_INF(":\n");
	os_mutex_unlock(&adv_mutex);
	/************adv end***********/

	/************recv start***********/
	os_mutex_lock(&recv_mutex, OS_FOREVER);
	memset(&recv_entity, 0, sizeof(struct ips_mgr_recv_st));
	ips_recv_st = &recv_entity;
	ips_recv_st->pa_cbs = ips_cb;
	ips_recv_st->match_id = ips_info->match_id;
	ips_recv_st->ch_max = ips_info->ch_max;
	if (ips_recv_st->ch_max > BT_IPS_CHANNEL_MAX) {
		ips_recv_st->ch_max = BT_IPS_CHANNEL_MAX;
	}
	le_ips_mac_addr_get(&ips_recv_st->local_addr);
	ips_recv_st->cnt_subscriber = ips_info->cnt_subscriber & 0x7;
	if (ips_recv_st->cnt_subscriber <= BT_IPS_SUBS_MAX) {
		memcpy(&ips_recv_st->subs_mac[0], &ips_info->subs_mac[0], ips_recv_st->cnt_subscriber*6);
		for (i = 0; i < ips_recv_st->cnt_subscriber; i++) {
			if (!memcmp(&ips_recv_st->subs_mac[i], &ips_recv_st->local_addr.a, 6)) {
				ips_recv_st->subs_number = i;
				break;
			}
		}
	}

	__ips_scan_start();

	os_delayed_work_init(&ips_sort_wait_work, ips_sort_wait_complete);
	ips_recv_st->ap_recv_handle = (void *)ips_recv_st;
	*handle = ips_recv_st->ap_recv_handle;

	if (false == ips_sync_start) {
		os_delayed_work_init(&ips_sync_create_work, ips_sync_create_timeout);
		ips_sync_start = true;
	}

	SYS_LOG_INF(":\n");
	os_mutex_unlock(&recv_mutex);
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
	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (!ips_recv_st || !handle || ips_recv_st->ap_recv_handle != handle) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (true == ips_sync_start) {
		os_delayed_work_cancel(&ips_sync_create_work);
		if (recv_entity.create_sync) {
			hostif_bt_le_per_adv_sync_delete(recv_entity.create_sync);
			ips_remove_pa(recv_entity.create_sync);
			recv_entity.create_sync = NULL;
		}
	}

	__ips_scan_stop();
	ips_recv_st->ap_recv_handle = NULL;
	ips_recv_st->hci_comp_sync = NULL;
	ips_recv_st->create_sync = NULL;
	ips_recv_st = NULL;
	os_mutex_unlock(&recv_mutex);

	os_mutex_lock(&adv_mutex, OS_FOREVER);
	__ips_adv_stop(cur_adv_type);
	ips_adv_st->advertising = 0;
	ips_adv_st = NULL;
	os_mutex_unlock(&adv_mutex);

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
	uint8_t token_number;

	os_mutex_lock(&adv_mutex, OS_FOREVER);
	if (!ips_adv_st || !ips_adv_st->pa_enable || !handle || !info || ips_adv_st->ap_adv_handle != handle) {
		os_mutex_unlock(&adv_mutex);
		//SYS_LOG_ERR("Handle invalid.");
		return -EALREADY;
	}

	if (!data) {
		voice_data_token_adjust(NULL, false);
		__ips_per_adv(BT_IPS_NULL_HEAD_TYPE, info->status , 0, NULL, 0);
		os_mutex_unlock(&adv_mutex);
		return 0;
	}

	f_len = codec_frame_len_calc(ips_adv_st->tx_codec);
	if (info->length%f_len) {
		os_mutex_unlock(&adv_mutex);
		SYS_LOG_ERR("length invalid %d.", info->length);
		return -EALREADY;
	}

	for (i = 0; i < (info->length/f_len); i++) {
		voice_data_token_adjust(NULL, true);
		token_number = voice_token_status_get();
		if (ips_adv_st->ch_max > 1) {
			if ((token_number&0xF)&(1<<ips_adv_st->subs_number)) {
				__ips_per_adv(BT_IPS_DATA_HEAD_TYPE, BT_IPS_STATUS_TALK , 0, data + i*f_len, f_len);
			} else {
				SYS_LOG_ERR("mic token busy 0x%x %d.", token_number, ips_adv_st->subs_number);
				__ips_per_adv(BT_IPS_NULL_HEAD_TYPE, BT_IPS_STATUS_WORKING, 0, NULL, 0);
				//__ips_per_adv(BT_IPS_DATA_HEAD_TYPE, BT_IPS_STATUS_TALK , 0, data + i*f_len, f_len);
			}
		} else {
			__ips_per_adv(BT_IPS_DATA_HEAD_TYPE, BT_IPS_STATUS_TALK , 0, data + i*f_len, f_len);
		}

	}

	os_mutex_unlock(&adv_mutex);
	return 0;
}

int bt_manager_ips_search_open(struct bt_ips_search_init *info)
{
	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (is_st.searching) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("searching %d.", is_st.searching);
		return -EBUSY;
	}

	if (ips_recv_st) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("ips_recv_st active.");
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
#if 1
	printk("aaass %02X:%02X:%02X:%02X:%02X:%02X\n",
				is_st.local_addr.a.val[5], is_st.local_addr.a.val[4], is_st.local_addr.a.val[3],
				is_st.local_addr.a.val[2], is_st.local_addr.a.val[1], is_st.local_addr.a.val[0]);
#endif
	__ips_scan_start();
	os_mutex_unlock(&recv_mutex);

	os_mutex_lock(&adv_mutex, OS_FOREVER);
	__ips_adv_start(BT_IPS_SEARCH_ADV);
	os_mutex_unlock(&adv_mutex);
	return 0;
}

int bt_manager_ips_search_close(void)
{
	os_mutex_lock(&recv_mutex, OS_FOREVER);
	if (!is_st.searching) {
		os_mutex_unlock(&recv_mutex);
		SYS_LOG_ERR("searching %d.", is_st.searching);
		return -EALREADY;
	}
	is_st.searching = 0;
	__ips_scan_stop();
	os_mutex_unlock(&recv_mutex);

	os_mutex_lock(&adv_mutex, OS_FOREVER);
	__ips_adv_stop(BT_IPS_SEARCH_ADV);
	os_mutex_unlock(&adv_mutex);
	return 0;
}

uint8_t bt_manager_ips_status_get(void)
{
	if (is_st.searching) {
		return BT_IPS_STATUS_MATCHING;
	}

	if (ips_recv_st) {
		return BT_IPS_STATUS_READY;
	}

	return 0;
}

