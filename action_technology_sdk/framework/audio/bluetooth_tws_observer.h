/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file bluetooth tws observer interface
 */

#ifndef __BLUETOOTH_TWS_OBSERVER_H__
#define __BLUETOOTH_TWS_OBSERVER_H__

#include <btservice_api.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t pkt_num; /* 包号 */
    uint16_t pkt_len; /* 包的数据长度 */
    uint16_t samples; /* 当前帧pcm样点数 */
    uint64_t pkt_bttime_us; /* 蓝牙时钟 us */
} __attribute__((packed)) tws_pkt_info_t;


typedef int32_t (*media_set_start_pkt_num)(void *handle, uint16_t pkt_num);
typedef int32_t (*media_start_playback)(void *handle);
//level==0xFF: set default aps level
typedef int32_t (*media_set_base_aps_level)(void *handle, uint8_t level);
typedef int32_t (*media_notify_time_diff)(void *handle, int32_t diff_time);


typedef struct {
	void *media_handle;
    void *tws_observer;
    media_set_start_pkt_num set_start_pkt_num;
	media_start_playback start_playback;
    media_set_base_aps_level set_base_aps_level;
    media_notify_time_diff notify_time_diff;
} media_observer_t;


typedef uint64_t (*bt_tws_get_bt_clk_us)(void);
typedef uint8_t (*bt_tws_get_role)(void);
typedef int32_t (*bt_tws_set_stream_info)(uint8_t format, uint16_t first_pktnum, uint16_t sample_rate);
typedef int32_t (*bt_tws_aps_change_request)(uint8_t level);
typedef int32_t (*bt_tws_set_pkt_info)(tws_pkt_info_t *info);


typedef struct {
    bt_tws_get_bt_clk_us get_bt_clk_us;
    bt_tws_get_role get_role;
    bt_tws_set_stream_info set_stream_info;
	bt_tws_aps_change_request aps_change_request;
    bt_tws_set_pkt_info set_pkt_info;
} bt_tws_observer_t;


bt_tws_observer_t* bluetooth_tws_observer_init(media_observer_t *media_observer);
void bluetooth_tws_observer_deinit(bt_tws_observer_t *tws_observer);


#ifdef __cplusplus
}
#endif

#endif  //END OF __BLUETOOTH_TWS_OBSERVER_H__
