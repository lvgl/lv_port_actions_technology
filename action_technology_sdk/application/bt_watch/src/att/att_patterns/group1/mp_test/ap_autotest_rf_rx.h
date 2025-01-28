/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx.h
*/

#ifndef _AP_AUTOTEST_RF_RX_H_
#define _AP_AUTOTEST_RF_RX_H_

#include "att_pattern_test.h"
#include "fcc_drv_extern.h"
#include "mp_manager_header.h"
#include "ap_autotest_porting.h"

#define INVALID_CFO_VAL         (0x7fff)
#define INVALID_PWR_VAL         (0x7fff)
#define INVALID_RSSI_VAL        (0x7fff)

//cfo max value,If the value is exceeded, the tool will report error
#define MAX_CFO_DIFF_VAL        (5)
#define MAX_RSSI_DIFF_VAL       (10)
#define TEST_RETRY_NUM          (3)
#define MIN_RESULT_NUM          (2)
#define CFO_RESULT_NUM          (4)

typedef enum
{
    BT_PAYLOAD_TYPE_ALL0 = 0,
    BT_PAYLOAD_TYPE_ALL1 = 1,
    BT_PAYLOAD_TYPE_0101 = 2,
    BT_PAYLOAD_TYPE_1010 = 3,
    BT_PAYLOAD_TYPE_0x0_0xF = 4,
    BT_PAYLOAD_TYPE_0000_1111 = 5,
    BT_PAYLOAD_TYPE_1111_0000 = 6,
    BT_PAYLOAD_TYPE_PRBS9 = 7,
    //////////////////////////////////
    BT_PAYLOAD_TYPE_NUM = 8
} BT_PAYLOAD_TYPE;

typedef enum
{
    BT_PKT_1DH1 = 0,
    BT_PKT_1DH3,
    BT_PKT_1DH5,
    BT_PKT_2DH1,
    BT_PKT_2DH3,
    BT_PKT_2DH5,
    BT_PKT_3DH1,
    BT_PKT_3DH3,
    BT_PKT_3DH5,
    BT_PKT_LE,
    //////////////////////////////////////////////////
    BT_PKT_TYPE_NULL,
    BT_PKT_TYPE_NUM
} BT_PKT_TYPE;

typedef struct
{
    u8_t cfo_channel[3];
    u8_t cfo_test;                 //Is frequency offset tested
    u16_t ref_cap_index;
    u8_t cap_index_low;            //Minimum index value
    u8_t cap_index_high;           //Maximum index value
    u8_t cap_index_changed;        //Whether to modify the index at the end of the test
    s8_t cfo_threshold_low;        //Minimum threshold of frequency offset
    s8_t cfo_threshold_high;       //Maximum threshold of frequency offset
    u8_t cfo_write_mode;           //Frequency offset result writing mode
    //u8_t reserved[2];
    s32_t  cfo_upt_offset;          //UPT Initial frequency offset

    u8_t  ber_test;
    u32_t ber_threshold_low;
    u32_t ber_threshold_high;

    u8_t  rssi_test;
    s16_t rssi_threshold_low;
    s16_t rssi_threshold_high;
} mp_test_arg_t;

typedef struct
{
    u16_t cap_index_low;
    u16_t cap_index_high;
    u16_t cap_index;
    s16_t rssi_val;
    s32_t cfo_val;
    s32_t pwr_val;
    u32_t ber_val;
	u16_t test_id;
	u16_t test_result;
} cfo_return_arg_t;

typedef struct
{
    s32_t cfo_value;
    s32_t rssi;
    s32_t total_bits;
    s32_t error_bits;
    s32_t adjust_hosc_cap;
} cfo_report_arg_t;

struct mp_test_stru
{
    u32_t mp_cap;
    //struct k_sem mp_sem;
    int receive_cfo_flag;
    s16_t cfo_val[CFO_RESULT_NUM];
    s16_t rssi_val[CFO_RESULT_NUM];
    u32_t total_bits;
    u32_t ber_val;

    uint8 mp_rx_data_ready;
    uint8 mp_valid_packets_cn;

    uint32 mp_packets_cn;

    int16 rssi_tmp[8];
    int16 cfo_tmp[8];

};

typedef struct
{
    u8_t mp_type;  // MP type 0 �C 5119  1 �C 5120
    u8_t sdk_type;  // reserved, default 0
    u8_t rf_channel; // MP test rf channel
	u8_t rf_power; // rf tx power
	u8_t packet_type; // packet type, default DH1
	u8_t payload_type;// payload type, default 01010101
    u16_t payload_len; // payload length, default 27
    u16_t timeout;  // Unable to receive enough data within the specified time is invalid data , unit 1ms
    u8_t reserved[6];
} bttools_mp_param_t;

extern struct att_env_var *self;

extern struct mp_test_stru g_mp_test;

bool rf_rx_ber_rssi_calc(s16_t *data_buffer, u32_t result_num, s16_t *rssi_return);
bool rf_rx_cfo_calc(s16_t *data_buffer, u32_t result_num, s16_t *cfo_return);

u8_t att_read_trim_cap(u32_t mode);
u8_t att_write_trim_cap(u8_t index, u32_t mode);

int rf_rx_ber_test_channel(_bt_mp_test_arg_t *mp_arg, u8_t channel, u32_t cap, s16_t *rssi_val, u32_t *ber_val);
int rf_rx_cfo_test_channel_once(_bt_mp_test_arg_t *mp_arg, u8_t channel, u32_t cap, s16_t *cfo);
int rf_rx_cfo_test_channel(_bt_mp_test_arg_t *mp_arg, u32_t channel, s16_t ref_cap, s16_t *return_cap, s16_t *return_cfo);

#endif

