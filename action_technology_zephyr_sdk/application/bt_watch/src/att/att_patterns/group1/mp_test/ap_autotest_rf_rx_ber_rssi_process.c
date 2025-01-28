/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_ber_rssi_process.c
*/

#include "ap_autotest_rf_rx.h"

int rf_rx_ber_test_channel(_bt_mp_test_arg_t *mp_arg, u8_t channel, u32_t cap, s16_t *rssi_val, u32_t *ber_val)
{
    struct mp_test_stru *mp_test = &g_mp_test;
    u8_t retry_cnt = 0;
    bool ret;

	extern void soc_set_hosc_cap(int cap);
	soc_set_hosc_cap(cap);

retry:

    mp_test->ber_val = 0;

    mp_task_rx_start(channel, 16);

	mp_task_rx_get_report();

    ret = rf_rx_ber_rssi_calc(mp_test->rssi_val, sizeof(mp_test->rssi_val) / sizeof(mp_test->rssi_val[0]), rssi_val);
    if (!ret) {
        goto fail;
    }

    *ber_val = mp_test->ber_val * 10000 / mp_test->total_bits;

    LOG_I("ber val : %d/10000, err bits = %d, total bits = %d\n", *ber_val, mp_test->ber_val, mp_test->total_bits);

    return 0;
fail:
    if (retry_cnt++ < TEST_RETRY_NUM) {
        goto retry;
    }

    return -1;
}

