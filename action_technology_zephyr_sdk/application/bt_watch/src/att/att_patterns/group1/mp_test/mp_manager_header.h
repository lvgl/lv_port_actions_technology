/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file mp_manager_header.h
*/

#ifndef _MP_MANAGER_HEADER_H_
#define _MP_MANAGER_HEADER_H_

#include "att_pattern_header.h"

#define MP_ICTYPE                           0//MP ÀàÐÍ 0 ¨C 5119   1 ¨C 5120
#define MP_TX_GAIN_IDX                      7
//#define MP_TX_GAIN_VAL                      0xcc
#define MP_TX_GAIN_VAL                      0xce
#define MP_TX_DAC                           0x13
#define HIT_ADDRESS_SET_L                   0x009e8b33
#define HIT_ADDRESS_SET_H                   0
#define PKTTYPE_SET		                    BT_PKT_1DH1
#define PKTHEADER_SET                       0x1234
#define WHITENCOEFF_SET	                    0x7F                    //(TX)   --->FF (RX)
#define PAYLOADTYPE_SET                     BT_PAYLOAD_TYPE_1111_0000//BT_PAYLOAD_TYPE_PRBS9
#define MP_REPORT_RX_INTERVAL               25
#define MP_SKIP_PKT_NUM                     0
#define MP_ONCE_REPORT_MIN_PKT_NUM          8
#define MP_RETURN_RESULT_NUM                10
#define MP_REPORT_TIMEOUT                   2
#define BER_TX_PKT_CNT                      0xffff
#define BER_TX_REPORT_INTERVAL              1
#define BER_TX_REPORT_TIMEOUT               4

#define PAYLOAD_LEN                         27
#define RF_POWER                            10


#define CFO_THRESHOLD_LEFT                  (0)
#define CFO_THRESHOLD_RIGHT                 (240)
#define CFO_THRESHOLD_ADJUST_VAL            (5)

#define MP_DEFAULT_CHANNEL                  (36)

void mp_core_load(void);
void mp_task_rx_start(u8_t channel, u32_t packet_counter);

#endif

