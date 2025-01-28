/*
* Copyright (c) 2019 Actions Semiconductor Co., Ltd
*
* SPDX-License-Identifier: Apache-2.0
*/

/**
* @file ap_autotest_rf_rx_cfo_calc.c
*/

#include "ap_autotest_rf_rx.h"

static u32_t my_abs(s32_t value)
{
    if (value > 0)
        return value;
    else
        return (0 - value);
}

bool rf_rx_cfo_calc(s16_t *data_buffer, u32_t result_num, s16_t *cfo_return)
{
    s32_t i;
    s16_t cfo_val;
    s32_t diff_val;
    s16_t cfo_avg_val;
    s8_t div_val;
    s32_t max_cfo_val;
    s32_t min_cfo_val;
    s32_t cfo_val_total;
    s32_t ori_cfo_val;
    s32_t max_diff_val;
    s32_t max_diff_index;
    s32_t invalid_data_flag;

    div_val = 0;

    cfo_avg_val = 0;

    max_cfo_val = -INVALID_CFO_VAL;

    min_cfo_val = 0x7fff;

    cfo_val_total = 0;

    //calc average value
    for (i = 0; i < result_num; i++) {
        ori_cfo_val = data_buffer[i];

        cfo_val = ori_cfo_val;

        if (ori_cfo_val != INVALID_CFO_VAL) {
        	//LOG_D("cfo result num[%d]: %d hz, pwr: %d\n", i, cfo_val, pwr_val);
            cfo_val_total += cfo_val;
            div_val++;

            if (cfo_val > max_cfo_val)
                max_cfo_val = cfo_val;

            if (cfo_val < min_cfo_val)
                min_cfo_val = cfo_val;
        }
    }

    //at least 10 invalid values
    if (div_val < MIN_RESULT_NUM){
		//this group data abnormal, need test again
		return false;

	}

    cfo_avg_val = (cfo_val_total / div_val);

    //cfo average value must not be 0
    if (cfo_avg_val == 0) {
        cfo_avg_val = (max_cfo_val + min_cfo_val) / 2;

        if ((cfo_avg_val == 0) && (max_cfo_val != 0))
            cfo_avg_val = max_cfo_val;
    }

    max_diff_val = 0;

    invalid_data_flag = false;

    while (1) {
        //To judge the dispersion degree of sampling points, first find out the points with large dispersion degree
        for (i = 0; i < result_num; i++) {
            ori_cfo_val = data_buffer[i];

            cfo_val = ori_cfo_val;

            if (ori_cfo_val != INVALID_CFO_VAL) {

                diff_val = my_abs(cfo_avg_val - cfo_val);
                if (diff_val > max_diff_val) {
                    max_diff_index = i;
                    max_diff_val = diff_val;
                }
            }
        }

        //Judge whether the point with the largest degree of dispersion exceeds the limit.
        //If it exceeds the limit, the point will be eliminated and the next point will be calculated repeatedly
        if (max_diff_val > MAX_CFO_DIFF_VAL) {
            //mask this point invalid
            data_buffer[max_diff_index] = INVALID_CFO_VAL;

            invalid_data_flag = true;

            max_diff_val = 0;
        } else {
            break;
        }
    }

    //There are invalid points and the average value of CFO needs to be recalculated
    if (invalid_data_flag == true) {
        cfo_val_total = 0;

        div_val = 0;

        for (i = 0; i < result_num; i++) {
            ori_cfo_val = data_buffer[i];

            cfo_val = ori_cfo_val;

            if (ori_cfo_val != INVALID_CFO_VAL) {
                cfo_val_total += cfo_val;
                div_val++;
            } else {
                continue;
            }

            if (cfo_val > max_cfo_val)
                max_cfo_val = cfo_val;

            if (cfo_val < min_cfo_val)
                min_cfo_val = cfo_val;
        }

        //at least 5 records
        if (div_val < MIN_RESULT_NUM) {
            //The sample points that need to be re collected due to multiple groups of abnormal CFO values
            //g_add_cfo_result_flag = true;
			LOG_E("div not enough %d\n", div_val);
            return false;
        }

        cfo_avg_val = (cfo_val_total / div_val);

        //cfo average value must not be 0
        if (cfo_avg_val == 0) {
            cfo_avg_val = (max_cfo_val + min_cfo_val) / 2;

            if ((cfo_avg_val == 0) && (max_cfo_val != 0))
                cfo_avg_val = max_cfo_val;
        }
    }

    LOG_I("div:%d, cfo:%d\n", div_val, cfo_avg_val);
    *cfo_return = cfo_avg_val;

    return true;
}

