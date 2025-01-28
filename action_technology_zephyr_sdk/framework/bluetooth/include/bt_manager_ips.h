/*
 * Copyright (c) 2024 Actions Semiconductor Co, Inc.
 *
 */


#ifndef __BT_MANAGER_IPS_H__
#define __BT_MANAGER_IPS_H__

#include <ctype.h>

#define BT_IPS_NAME_MAX (16)

enum {
	BT_IPS_TX_CODEC_LC3, //LC3编码
	BT_IPS_TX_CODEC_OPUS, //OPUS编码
	BT_IPS_TX_CODEC_AMR, //AMR编码
	BT_IPS_TX_CODEC_MAX,
};

enum {
	BT_IPS_COMP_RATIO_VARIABLE, // 可变压缩比
	BT_IPS_COMP_RATIO_4, // 压缩比4 : 1
	BT_IPS_COMP_RATIO_8, // 压缩比8 : 1
	BT_IPS_COMP_RATIO_16, // 压缩比16 : 1
	BT_IPS_COMP_RATIO_MAX,
};

enum {
	BT_IPS_TX_CH_SINGLE, // 单身道
	BT_IPS_TX_CH_STEREO, // 立体声
	BT_IPS_TX_CH_MAX,
};

enum {
	BT_IPS_SAMPLE_FREQ_8, // 采样率8KHZ
	BT_IPS_SAMPLE_FREQ_16, // 采样率16KHZ
	BT_IPS_SAMPLE_FREQ_MAX,
};

enum {
	BT_IPS_BIT_WIDTH_8, // 位宽8bits 
	BT_IPS_BIT_WIDTH_16, // 位宽16bits 
	BT_IPS_BIT_WIDTH_MAX,
};

enum {
	BT_IPS_STATUS_NONE,
	BT_IPS_STATUS_MATCHING,
	BT_IPS_STATUS_READY,
	BT_IPS_STATUS_WORKING,
	BT_IPS_STATUS_TALK,
	BT_IPS_STATUS_STOPING,
	BT_IPS_STATUS_CLOSE,
	BT_IPS_STATUS_MAX,
};

enum {
	BT_IPS_ROLE_NONE,
	BT_IPS_ROLE_INITIATOR,
	BT_IPS_ROLE_SUBSCRIBER,
	BT_IPS_ROLE_MAX,
};

struct bt_ips_init_info {
	uint32_t match_id; // 匹配码, 4bytes 随机数
	uint8_t ch_max; //最多允许接收的音频通道数量
	uint8_t tx_codec; //本机广播的编码格式
	uint8_t role; // BT_IPS_ROLE_INITIATOR or BT_IPS_ROLE_SUBSCRIBER
	char name[BT_IPS_NAME_MAX];
	uint8_t comp_ratio; // 编码压缩比
	uint8_t ch_mode; //单声道或双声道
	uint8_t samp_freq; //采样率8KHZ or 16KHZ
	uint8_t bit_width; // 位宽8bits or 16bits
};

struct bt_ips_sync_info {
	uint8_t role;
	char name[BT_IPS_NAME_MAX];
	uint8_t rx_codec; //对端广播的编码格式
	uint8_t comp_ratio; // 编码压缩比
	uint8_t ch_mode; //单声道或双声道
	uint8_t samp_freq; //采样率8KHZ or 16KHZ
	uint8_t bit_width; // 位宽8bits or 16bits
};

struct bt_ips_recv_info {
	uint16_t length; //接收一帧编码数据长度
	uint8_t pkt_num; //帧序号, 累加, 判断是否丢帧
};

struct bt_ips_send_info {
	uint16_t length; //发送编码数据长度
	uint8_t frame_number; //发送帧数
	uint8_t status; // BT_IPS_STATUS_X
	uint8_t num_synced; //正式进入对讲时，已synced数量，用于决定关闭scan，对应状态BT_IPS_STATUS_WORKING
	//uint8_t role; // BT_IPS_ROLE_INITIATOR or BT_IPS_ROLE_SUBSCRIBER
};

struct bt_ips_cb {
	/**
	 * @brief 接收通道捕获成功回调.
	 *
	 * @param out sync_handle 设备句柄.
	 * @param in info  编码数据的相关参数.
	 */
	void (*ips_synced)(void *sync_handle, struct bt_ips_sync_info *info);

	/**
	 * @brief 接收通道丢失回调.
	 *
	 * @param out sync_handle.
	 */
	void (*ips_term)(void *sync_handle);

	/**
	 * @brief 一帧编码数据回调.
	 *
	 * @param out sync_handle.
	 * @param out info  
	 * @param out data  收到的纯编码数据.
	 */
	void (*ips_recv_codec)(void *sync_handle, struct bt_ips_recv_info *info, uint8_t *data);

	/**
	 * @brief 对端adv当前状态.
	 *
	 * @param out sync_handle.
	 * @param status BT_IPS_STATUS_NONE or
	 * @param role
	 * @param num_synced
	 */
	void (*ips_status)(void *sync_handle, uint8_t status, uint8_t num_synced);
};

struct bt_ips_search_rt {
	uint16_t t_num; //搜索到的总设备数量
	uint8_t role; //搜索到设备的角色
	uint8_t name[BT_IPS_NAME_MAX]; //当前搜索到的设备名称
};

typedef void ips_search_cb(struct bt_ips_search_rt *rt);

struct bt_ips_search_init {
	uint8_t role;
	uint32_t match_id;
	char name[BT_IPS_NAME_MAX];
	ips_search_cb *s_cb;
};

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
				struct bt_ips_init_info *ips_info, struct bt_ips_cb *ips_cb);

/** @brief 关闭对讲功能
 *
 *  调用接口后会关闭周期性广播和广播扫描.
 *
 *  @param in 操作句柄
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_stop(void *handle);

/** @brief 关闭对讲功能
 *
 *  调用接口后会进行周期性广播，并且扫描广播.
 *
 *  @param in 操作句柄
 *  @param in info
 *  @param in data 发送状态或纯编码数据.闭麦时data is NULL
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_send_codec(void *handle, struct bt_ips_send_info *info, uint8_t *data);

/** @brief 开启搜索匹配的设备
 *
 *  调用接口后会进行对讲机广播和广播扫描.
 *
 *  @param in info
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_search_open(struct bt_ips_search_init *info);

/** @brief 关闭搜索匹配的设备
 *
 *  调用接口后会关闭对讲机广播和广播扫描.
 *
 *  @return  0  success, -1 fail.
 */
int bt_manager_ips_search_close(void);

#endif  /* __BT_MANAGER_IPS_H__ */

