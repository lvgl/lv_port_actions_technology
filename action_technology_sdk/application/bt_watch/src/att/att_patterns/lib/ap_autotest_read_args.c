/*
 * Copyright (c) 2021 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wuyufan<wuyufan@actions-semi.com>
 *
 * Change log:
 *	2021/2/17: Created by wuyufan.
 */

#include "att_pattern_test.h"

/**
 *  \brief Get test arguments.
 *
 *  \param [in] input_arg       Input argument data.
 *  \param [in] output_arg      Pointer fo output argument structure, \ref in output_arg_t.
 *  \param [in] output_arg_num  Output argument number
 *  \return Whether arguments parse success or fail.
 *          0 :     success
 *         -1 :     arguments number is wrong
 *
 *  \details More details
 */
s32_t act_test_read_arg(att_test_para_t *input_arg, output_arg_t *output_arg, u32_t output_arg_num)
{
    int i, offset;
    test_para_t *att_input_para;

    if (input_arg->para_num != output_arg_num)
    {
        att_buf_printf("Wrong arg num: %d, expect: %d", input_arg->para_num, output_arg_num);
        return -1;
    }

    att_input_para = input_arg->para_item;
    for (i = 0; i < output_arg_num; i++)
    {
        output_arg[i].arg_length    = att_input_para->para_len;
        output_arg[i].arg           = att_input_para->para;

        //数据长度 word 对齐
        offset          = 4 + (att_input_para->para_len + 3) / 4 * 4;
        att_input_para  = (test_para_t *)((u32_t)att_input_para + offset);
    }

    return 0;
}