/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_cfo_process.c
*/

#include "ap_autotest_rf_rx.h"
#include "soc.h" /*BROM APIS DEFINE*/

static u32_t my_abs(s32_t value)
{
    if (value > 0)
        return value;
    else
        return (0 - value);
}

int rf_rx_cfo_test_channel_once(_bt_mp_test_arg_t *mp_arg, u8_t channel, u32_t cap, s16_t *cfo)
{
    struct mp_test_stru *mp_test = &g_mp_test;
    u8_t retry_cnt = 0;
    bool ret;

    extern void soc_set_hosc_cap(int cap);
	soc_set_hosc_cap(cap);

retry:

    mp_task_rx_start(channel, 4);

    ret = rf_rx_cfo_calc(mp_test->cfo_val, sizeof(mp_test->cfo_val) / sizeof(mp_test->cfo_val[0]), cfo);

    if (!ret)
        goto fail;

    return 0;
fail:
    if (retry_cnt++ < TEST_RETRY_NUM)
        goto retry;

    //cfo unexpected
    LOG_I("get cfo unexpected");
    return -1;
}

int rf_rx_cfo_test_channel(_bt_mp_test_arg_t *mp_arg, u32_t channel, s16_t ref_cap, s16_t *return_cap, s16_t *return_cfo)
{
    s16_t cfo_val;
    s32_t left;
    s32_t right;
    s32_t cfo_threshold_low;
    s32_t cfo_threshold_high;
    s32_t cfo_threshold_middle;
    s32_t ret_val;
    u32_t cap_index;

    s16_t last_negative_value = -INVALID_CFO_VAL;
    s16_t last_positive_value = INVALID_CFO_VAL;
    u32_t n_cap_index = 0;
    u32_t p_cap_index = 0;
    u32_t try_count = 0;
    u32_t test_retry_count = 0;

    bool cfo_test_end_flag = false;

    cap_index = ref_cap;

    left = mp_arg->cap_index_low;
    right = mp_arg->cap_index_high;

    cfo_threshold_low = mp_arg->cfo_low_limit;
    cfo_threshold_high = mp_arg->cfo_high_limit;
    cfo_threshold_middle = (cfo_threshold_low + cfo_threshold_high)/2;

    LOG_I("ref %d %d %d\n", cap_index, left, right);

    if (right > 240)
        right = 240;

    if (cap_index < left || cap_index > right)
        cap_index = (left + right) / 2;

retry:

    try_count ++;

    LOG_I("cfo test cap_index : %d\n", cap_index);

    ret_val = rf_rx_cfo_test_channel_once(mp_arg, channel, cap_index, &cfo_val);

    //If the packet receiving time-out, judge whether the deviation of the first test reference capacitor is too large,
    //resulting in the failure of normal data packet
    if(try_count == 1 && ret_val == -2) {
        cap_index = (left + right) / 2;
        goto retry;
    }

    if (ret_val < 0 || test_retry_count > 10) {
        LOG_I("cfo test fail,try count:%d, ret:%d\n", try_count, ret_val);
        goto test_fail;
    }

    att_buf_printf("cap_index is %d, cfo_val is %d try cnt %d retry cnt %d\n", cap_index, cfo_val, try_count, test_retry_count);

    if (cfo_val > cfo_threshold_middle) {
        if (cfo_val < last_positive_value) {
            //Record the CFO value and capacitance value when CFO is positive
            last_positive_value = cfo_val;
            p_cap_index = cap_index;
        } else {
            att_buf_printf("positive %d, %d, %d, %d\n", cfo_val, last_positive_value, cap_index, left);
            //hosc base cap & trim cap boundary error, but it means cfo trim maybe is ok
            if ((cfo_val - last_positive_value <= 1) && (cap_index - left < 2)) {
                cfo_test_end_flag = true;
                goto test_end;
            } else {
                if ((cfo_val - last_positive_value <= 1) &&  (cap_index - left > 6)) {
                    LOG_E("mp test fail, direction err,current cfo:%d,last positive cfo:%d, cap%d\n",
                                cfo_val, last_positive_value, cap_index);

                    left = mp_arg->cap_index_low;
                    right = mp_arg->cap_index_high;
                    cap_index = (left + right) / 2;
                    test_retry_count++;
                    goto retry;
                }
            }
        }
        left = cap_index;
    } else if (cfo_val < cfo_threshold_middle) {
        if (cfo_val > last_negative_value) {
            //Record the CFO value and capacitance value when CFO is negative
            last_negative_value = cfo_val;
            n_cap_index = cap_index;
        } else {
            att_buf_printf("negative %d, %d, %d, %d\n", cfo_val, last_negative_value, cap_index, right);
            //hosc base cap & trim cap boundary error, but it means cfo trim maybe is ok
            if ((last_negative_value - cfo_val <= 1) && (right - cap_index < 2)) {
                cfo_test_end_flag = true;
                goto test_end;
            } else {
                if ((last_negative_value - cfo_val <= 1) && (right - cap_index > 6)) {
                    LOG_E("mp test fail, direction err,current cfo:%d,last negative cfo:%d, cap%d\n",
                                cfo_val, last_negative_value, cap_index);

                    left = mp_arg->cap_index_low;
                    right = mp_arg->cap_index_high;
                    cap_index = (left + right) / 2;

                    test_retry_count++;
                    goto retry;
                }
            }
        }
        right = cap_index;
    } else {
        last_positive_value = cfo_val;
        p_cap_index = cap_index;
        cfo_test_end_flag = true;
        goto test_end;
    }

    //Judge whether the test is over
    if ((right - left) <= 1) {
        cfo_test_end_flag = true;
        goto test_end;
    }

    //Use dichotomy
    cap_index = (right + left)/2;

test_end:

    if (cfo_test_end_flag == true) {
        //The CFO value with the frequency offset close to 0 and the corresponding capacitance value are obtained
        cfo_val = my_abs(last_positive_value - cfo_threshold_middle) < my_abs(last_negative_value - cfo_threshold_middle)
                    ? last_positive_value : last_negative_value;

        cap_index = my_abs(last_positive_value - cfo_threshold_middle) < my_abs(last_negative_value - cfo_threshold_middle)
                    ? p_cap_index : n_cap_index;

        if (cfo_val >= cfo_threshold_low && cfo_val <= cfo_threshold_high) {
            *return_cap = cap_index;
            *return_cfo = cfo_val;
            att_buf_printf("cfo val: %d,cap: %d,ref cap: %d,count: %d\n", cfo_val, cap_index, ref_cap, try_count);
            return 0;
        } else {
            att_buf_printf("not found proper cfo,current cfo val: %d, cap: %d\n", cfo_val, cap_index);
            return -1;
        }
    }

    LOG_I("test cap : %d, @[%d, %d]\n", cap_index, left, right);
    goto retry;

test_fail:

    return -1;
}

