/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_ber_rssi_calc.c
*/

#include "ap_autotest_rf_rx.h"

static u32_t my_abs(s32_t value)
{
    if (value > 0)
        return value;
    else
        return (0 - value);
}

bool rf_rx_ber_rssi_calc(s16_t *data_buffer, u32_t result_num, s16_t *rssi_return)
{
    s32_t i;
    s16_t rssi_val;
    s16_t diff_val;
    s16_t rssi_avg_val;
    s8_t div_val;
    s32_t rssi_val_total;
    s16_t ori_rssi_val;
    s32_t max_diff_val;
    s32_t max_diff_index;
    s32_t invalid_data_flag;

    div_val = 0;

    rssi_avg_val = 0;

    rssi_val_total = 0;

    for (i = 0; i < result_num; i++) {
        ori_rssi_val = data_buffer[i];

        rssi_val = ori_rssi_val;

        if (ori_rssi_val != INVALID_RSSI_VAL){
            rssi_val_total += rssi_val;
            div_val++;
        }
    }

    //at least 10 records
    if (div_val < MIN_RESULT_NUM) {
        //this group data abnormal, need test again
        return false;
    }

    rssi_avg_val = (rssi_val_total / div_val);

    max_diff_val = 0;

    invalid_data_flag = false;

    while (1) {
        //To judge the dispersion degree of sampling points, first find out the points with large dispersion degree
        for (i = 0; i < result_num; i++) {
            ori_rssi_val = data_buffer[i];

            rssi_val = ori_rssi_val;

            if (ori_rssi_val != INVALID_RSSI_VAL) {
                diff_val = my_abs(rssi_avg_val - rssi_val);
                if (diff_val > max_diff_val) {
                    max_diff_index = i;
                    max_diff_val = diff_val;
                }
            }
        }

        //Judge whether the point with the largest degree of dispersion exceeds the limit.
        //If it exceeds the limit, the point will be eliminated and the next point will be calculated repeatedly
        if (max_diff_val > MAX_RSSI_DIFF_VAL) {
            //mask this point invalid
            data_buffer[max_diff_index] = INVALID_RSSI_VAL;

            invalid_data_flag = true;

            max_diff_val = 0;
        } else {
            break;
        }
    }

    //There are invalid points and the average value of RSSI needs to be recalculated
    if (invalid_data_flag == true)
    {
        rssi_val_total = 0;

        div_val = 0;

        for (i = 0; i < result_num; i++) {
            ori_rssi_val = data_buffer[i];

            rssi_val = ori_rssi_val;

            if (ori_rssi_val != INVALID_RSSI_VAL) {
                rssi_val_total += rssi_val;
                div_val++;
            } else {
                continue;
            }
        }

        //at least 5 records
        if (div_val < MIN_RESULT_NUM) {
            //The sample points need to be re collected due to multiple groups of RSSI outliers
            return false;
        }

        rssi_avg_val = (rssi_val_total / div_val);
    }

    *rssi_return = rssi_avg_val;

    //LOG_D("div:%d,rssi:%d\n", div_val, rssi_avg_val);

    return true;
}

