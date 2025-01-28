/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_mp_manager.c
*/

#include "ap_autotest_rf_rx.h"

/*packet_counter:4 ~ 16*/
void mp_task_rx_start(u8_t channel, u32_t packet_counter)
{
	struct mp_test_stru *mp_test = &g_mp_test;
	int i;

	act_test_bt_packet_receive_start();

	for (i = 0; i < CFO_RESULT_NUM; i++) {
		if (act_test_bt_packet_receive_process(channel, packet_counter) != 0) {
			LOG_E("packet_receive fail!\n");
			mp_test->cfo_val[i] = INVALID_CFO_VAL;
			mp_test->rssi_val[i] = INVALID_RSSI_VAL;
			continue;
		}
	}

	mp_test->mp_packets_cn = 0;
	mp_test->mp_valid_packets_cn = 0;

	act_test_bt_packet_receive_stop();
}

void mp_task_rx_get_report(void)
{
	struct mp_test_stru *mp_test = &g_mp_test;
	mp_rx_report_t rx_report;
	u32_t cur_total_bits;

    mp_test->ber_val = 0;
    mp_test->total_bits = 0;

	act_test_bt_rx_report(&rx_report);
	cur_total_bits = rx_report.rx_packet_cnts * rx_report.packet_len * 8;
	LOG_I("rx report:cfo %d, rssi %d, total bits %d, err bits %d\n",
		rx_report.cfo_index, rx_report.rx_rssi, cur_total_bits, rx_report.total_error_bits);
	mp_test->total_bits += cur_total_bits;
	mp_test->ber_val += rx_report.total_error_bits;

}

