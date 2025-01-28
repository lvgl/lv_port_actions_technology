/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
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
 * Author: johnsen<jmzhang@actions-semi.com>
 *
 * Change log:
 *	2019/5/28: Created by johnsen.
 */


#include "att_pattern_test.h"



int act_test_modify_bt_bqb_flag(int flag)
{
    int ret_val = 0;
	u8_t bqb_flag = flag;
    ret_val = act_test_set_bt_bqbflag(bqb_flag);
    if(ret_val != 0)
    {
    	ret_val = 0;
        LOG_E("bqb flag write fail\n");
    }else{
		ret_val = 1;
	}

    return ret_val;

}

test_result_e act_test_enter_BQB_mode(void *arg_buffer, uint32_t arg_len)
{
    att_buf_printf("prepare BQB test\n");

    act_test_normal_report(TESTID_BQBMODE, 1);

	//Todo
    //bteg_set_bqb_flag(1);
	act_test_modify_bt_bqb_flag(1);

    return TEST_PASS;

}

bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

	ret_val = act_test_enter_BQB_mode(self->arg_buffer, self->arg_len);

    return ret_val;
}
