/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx.c
*/

#include "ap_autotest_rf_rx.h"
#include "compensation.h"

return_result_t *return_data;
u16_t trans_bytes;

_bt_mp_test_arg_t g_mp_test_arg;

struct mp_test_stru g_mp_test;

cfo_return_arg_t g_cfo_return_arg;

u32_t test_channel_num;

static bool att_ber_rssi_test(_bt_mp_test_arg_t *mp_arg, s16_t cap_value)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;
    s16_t rssi_val[3] = {0};
    u32_t ber_val[3] = {0};
    bool ret = false;
    int ret_val, i;

    for(i = 0; i < 3; i++) {
        if (mp_arg->channel[i] == 0xff)
           	continue;

        LOG_I("test channel : %d\n", mp_arg->channel[i]);

        ret = att_bttool_rx_begin(mp_arg->channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_ber_test_channel(mp_arg, mp_arg->channel[i], cap_value, &rssi_val[i], &ber_val[i]);
        if (ret_val < 0) {
            ret = false;
            LOG_E("ber or rssi test fail, channel[%d]:%d\n", i, mp_arg->channel[i]);
            goto test_end;
        }

        ret = att_bttool_rx_stop();
        if (ret == false) {
            LOG_E("bttool stop fail\n");
            goto test_end;
        }

        //ber(bit error rate) the smaller , the sensitivity higher
        if (mp_arg->ber_test && (ber_val[i] > mp_arg->ber_threshold_high
                    /*|| ber_val[i] < mp_arg->ber_threshold_low*/)) {
            LOG_E("ber test fail ber_val:%d\n", ber_val[i]);
            ret = false;
            goto test_end;
        }

        if (mp_arg->rssi_test && (rssi_val[i] > mp_arg->rssi_high_limit
                    || rssi_val[i] < mp_arg->rssi_low_limit)) {
            LOG_E("rssi test fail rssi_val:%d\n", rssi_val[i]);
            ret = false;
            goto test_end;
        }
    }

    if (ret == true) {
        cfo_return_arg->rssi_val = (rssi_val[0] + rssi_val[1] + rssi_val[2]) / (test_channel_num);
        cfo_return_arg->ber_val = (ber_val[0] + ber_val[1] + ber_val[2]) / (test_channel_num);
        LOG_I("mp read rssi value:%d, ber:%d\n", cfo_return_arg->rssi_val, cfo_return_arg->ber_val);
    }

	test_end:
	return ret;
}


static bool att_cfo_adjust_test(_bt_mp_test_arg_t *mp_arg)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;
    s16_t cap_index[3] = {0};
    s16_t cfo_value[3] = {0};
    bool ret = false;
    int ret_val, i;
    u8_t ret_cap_index;
    struct mp_test_stru *mp_test = &g_mp_test;

    for(i = 0; i < 3; i++) {
        if (mp_arg->channel[i] == 0xff)
            continue;

        ret = att_bttool_rx_begin(mp_arg->channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_cfo_test_channel(mp_arg, mp_arg->channel[i], mp_arg->ref_cap_index, &cap_index[i], &cfo_value[i]);
        if (ret_val < 0) {
            ret = false;
            goto test_end;
        }

        ret = att_bttool_rx_stop();
        if (ret == false)
            goto test_end;
    }

    if (ret == true) {
        cfo_return_arg->cap_index = (cap_index[0] + cap_index[1] + cap_index[2]) / (test_channel_num);
        cfo_return_arg->cfo_val = (cfo_value[0] + cfo_value[1] + cfo_value[2]) / (test_channel_num);

		//fixme
		cfo_return_arg->rssi_val = mp_test->rssi_val[0];

        mp_arg->ref_cap_index = cfo_return_arg->cap_index;

        att_buf_printf("mp test success cap index:%d cfo:%d rssi: %d\n", cfo_return_arg->cap_index, cfo_return_arg->cfo_val, cfo_return_arg->rssi_val);

        //only write nor, if write nor fail, report error
        if (mp_arg->cap_write_region == 1) {
            ret_cap_index = att_write_trim_cap(cfo_return_arg->cap_index, RW_TRIM_CAP_SNOR);
            if (ret_cap_index == 0) {
                att_buf_printf("cap write nvram fail\n");
                ret = false;
                goto test_end;
            }
        }

        //write efuse first, if write efuse fail, write nvram
        if (mp_arg->cap_write_region == 2) {
            ret_cap_index = att_write_trim_cap(cfo_return_arg->cap_index, RW_TRIM_CAP_EFUSE);
            if (ret_cap_index == 0) {
                att_buf_printf("cap write efuse+nvram fail\n");
                ret = false;
                goto test_end;
            }
        }

    }

test_end:

    return ret;
}


static bool att_cfo_read_test(_bt_mp_test_arg_t *mp_arg, u8_t cap_value)
{
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg;;
    s16_t cfo_val[3] = {0};

    bool ret = true;
    int ret_val, i;
    struct mp_test_stru *mp_test = &g_mp_test;

    LOG_D("mp read test start\n");

    for(i = 0; i < 3; i++) {
        if (mp_arg->channel[i] == 0xff)
            continue;

        ret = att_bttool_rx_begin(mp_arg->channel[i]);
        if (ret == false)
            goto test_end;

        ret_val = rf_rx_cfo_test_channel_once(mp_arg, mp_arg->channel[i], cap_value, &cfo_val[i]);
        if (ret_val < 0) {
            LOG_E("mp read test,channel:%d,return fail\n", mp_arg->channel[i]);
            ret = false;
            goto test_end;
        }

        ret = att_bttool_rx_stop();
        if (ret == false) {
            LOG_E("bttool stop fail\n");
            goto test_end;
        }

        if (cfo_val[i] < mp_arg->cfo_low_limit || cfo_val[i] > mp_arg->cfo_high_limit) {
            att_buf_printf("mp read test fail,channel:%d,cfo:%d\n", mp_arg->channel[i], cfo_val[i]);
            ret = false;
            goto test_end;
        }
    }

    if (ret == true) {
        cfo_return_arg->cfo_val = (cfo_val[0] + cfo_val[1] + cfo_val[2]) / (test_channel_num);
		cfo_return_arg->cap_index = cap_value;
		//fixme
		cfo_return_arg->rssi_val = mp_test->rssi_val[0];

		att_buf_printf("mp read test success cap index:%d cfo:%d rssi: %d\n", cfo_return_arg->cap_index, cfo_return_arg->cfo_val, cfo_return_arg->rssi_val);
    }

test_end:

    return ret;
}

bool att_check_need_adjust_cfo(void)
{
    u32_t trim_cap;
	int ret_val;

	if (g_mp_test_arg.cap_write_region == 1)
		ret_val = freq_compensation_read(&trim_cap, RW_TRIM_CAP_SNOR);
	else
		ret_val = freq_compensation_read(&trim_cap, RW_TRIM_CAP_EFUSE);

    if (ret_val == TRIM_CAP_READ_NO_ERROR) {
		att_buf_printf("read trim cap %d, start read test\n", trim_cap);
		ret_val = att_cfo_read_test(&g_mp_test_arg, trim_cap);
		if(ret_val == true){
			att_buf_printf("read trim cap %d test ok\n", trim_cap);
		}else{
			att_buf_printf("read trim cap %d test failed\n", trim_cap);
		}
	}else{
		att_buf_printf("read fail cap=%d\n", trim_cap);
		ret_val = false;
	}

	return ret_val;
}

void test_loop(void)
{
	void (*p_launch)(void);

	p_launch = (void *)0x1008141;

	p_launch();
}


bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;
    int i;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;
	trans_bytes = 0;

	if (self->arg_len != 0) {
		if (act_test_read_rf_rx_arg() < 0) {
			LOG_E("read rf rx arg fail\n");
			goto test_end_0;
		}
	}

    test_channel_num = 0;
    for (i = 0; i < 3; i++) {
        if (g_mp_test_arg.channel[i] != 0xff) {
            test_channel_num++;
        }
    }

    if (test_channel_num == 0) {
        LOG_E("mp channel num is zero\n");
        goto test_end_0;
    }

    g_cfo_return_arg.cap_index_low = g_mp_test_arg.cfo_low_limit;
    g_cfo_return_arg.cap_index_high = g_mp_test_arg.cfo_high_limit;

    soc_cmu_init();
	mp_btc_init();

	if (self->test_id == TESTID_MP_CFO) {
		if (g_mp_test_arg.cfo_test_swtich) {
			ret_val = att_check_need_adjust_cfo();
			if(ret_val == false){
				ret_val = att_cfo_adjust_test(&g_mp_test_arg);
			}
		}
	} else /*TESTID_MP_READ_TEST*/ {
		if (g_mp_test_arg.cfo_test_swtich) {
			ret_val = att_cfo_read_test(&g_mp_test_arg, att_read_trim_cap(RW_TRIM_CAP_SNOR));
		}
	}

	if (ret_val == true) {
		if (g_mp_test_arg.ber_test || g_mp_test_arg.rssi_test) {
			s16_t cap_temp;
	        cap_temp = att_read_trim_cap(RW_TRIM_CAP_SNOR);
			ret_val = att_ber_rssi_test(&g_mp_test_arg, cap_temp);
		}
	}

	mp_btc_deinit();

	ret_val = act_test_report_rf_rx_result(ret_val);

test_end_0:
    att_buf_printf("att test end : %d\n", ret_val);

	//if(ret_val == true){
	//	test_loop();
	//}

    return ret_val;
}
