/********************************************************************************
 *        Copyright(c) 2014-2016 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * 描述：自动化测试：修改蓝牙地址，修改蓝牙设备名称。
 * 作者：zhouxl
 ********************************************************************************/

#include "att_pattern_test.h"

test_result_e act_test_sn_write(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;
    output_arg_t arg;
	char sn_string[CFG_MAX_SN_NAME_LEN];

    // log_debug();
    act_test_read_arg(arg_buffer, &arg, 1);
	// log_debug("arg.arg_length 0x%x",arg.arg_length);
    if (arg.arg_length < sizeof(sn_string))
    {
        memset(sn_string, 0, sizeof(sn_string));
        memcpy(sn_string, arg.arg, arg.arg_length);

        ret_val = act_test_set_sn_info(sn_string, arg.arg_length);

        if(ret_val == 0){
            ret_val = TEST_PASS;
        }else{
            ret_val = TEST_FAIL;
        }
    }
    else
    {
        ret_val = TEST_SN_ERROR;
    }

    act_test_normal_report(TESTID_SERIAL_NUMBER_WRITE, ret_val);
    return ret_val;
}

test_result_e act_test_sn_read(void *arg_buffer, u32_t arg_len)
{
    int ret_val = TEST_PASS;
    return_result_t *return_data;
    u32_t trans_bytes;

    trans_bytes = 4 + CFG_MAX_SN_NAME_LEN;
    return_data = (return_result_t *)app_mem_malloc(trans_bytes);

    ret_val = act_test_get_sn_info((char *)return_data->return_arg, CFG_MAX_SN_NAME_LEN);

    if(ret_val > 0){
        ret_val = TEST_PASS;    
    }else{
        ret_val = TEST_FAIL;
    }

    return_data->test_result = ret_val;
    act_test_report_result(TESTID_SERIAL_NUMBER_READ, (u8_t*)return_data, trans_bytes);

    app_mem_free(return_data);
    return ret_val;
}

bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

	if (self->test_id == TESTID_SERIAL_NUMBER_WRITE) {
		ret_val = act_test_sn_write(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_SERIAL_NUMBER_READ) {
		ret_val = act_test_sn_read(self->arg_buffer, self->arg_len);
	} else  {
	}

    return ret_val;
}