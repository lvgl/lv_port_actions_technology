/********************************************************************************
 *        Copyright(c) 2014-2016 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * 描述：自动化测试：读取蓝牙地址测试，读取蓝牙设备名称测试。
 * 作者：zhouxl
 ********************************************************************************/

#include "att_pattern_test.h"



//该测试项用于读取蓝牙地址，提供给PC工具进行校验，确认地址是否正确
test_result_e act_test_read_bt_addr(void *arg_buffer, u32_t arg_len)
{
    int ret_val = TEST_PASS;
    return_result_t *return_data;
    u32_t trans_bytes;

    trans_bytes = 4 + BT_DEV_ADDR_LEN;
    return_data = (return_result_t *)app_mem_malloc(trans_bytes);

    ret_val = act_test_get_bt_addr(return_data->return_arg, 6);

    if(ret_val == 12){
        ret_val = TEST_PASS;    
    }else{
        ret_val = TEST_FAIL;
    }

    return_data->test_result = ret_val;
    act_test_report_result(TESTID_READ_BTADDR, (u8_t*)return_data, trans_bytes);

    app_mem_free(return_data);
    return ret_val;
}




test_result_e act_test_read_bt_name(void *arg_buffer, u32_t arg_len)
{
    int ret_val = TEST_PASS;
    return_result_t *return_data;
    u32_t trans_bytes;

    trans_bytes = 4 + CFG_MAX_BT_DEV_NAME_LEN;
    return_data = (return_result_t *)app_mem_malloc(trans_bytes);

    ret_val = act_test_get_bt_name((char *)return_data->return_arg, CFG_MAX_BT_DEV_NAME_LEN);

    if(ret_val > 0){
        ret_val = TEST_PASS;    
    }else{
        ret_val = TEST_FAIL;
    }

    return_data->test_result = ret_val;
    act_test_report_result(TESTID_READ_BTNAME, (u8_t*)return_data, trans_bytes);

    app_mem_free(return_data);
    return ret_val;
}


test_result_e act_test_read_ble_name(void *arg_buffer, u32_t arg_len)
{
    int ret_val = TEST_PASS;
    return_result_t *return_data;
    u32_t trans_bytes;

    trans_bytes = 4 + CFG_MAX_BT_DEV_NAME_LEN;
    return_data = (return_result_t *)app_mem_malloc(trans_bytes);

    ret_val = act_test_get_bt_blename((char *)return_data->return_arg, CFG_MAX_BT_DEV_NAME_LEN);

    if(ret_val > 0){
        ret_val = TEST_PASS;    
    }else{
        ret_val = TEST_FAIL;
    }

    return_data->test_result = ret_val;
    act_test_report_result(TESTID_READ_BLENAME, (u8_t*)return_data, trans_bytes);

    app_mem_free(return_data);
    return ret_val;
}

bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

	if (self->test_id == TESTID_READ_BTADDR) {
		ret_val = act_test_read_bt_addr(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_READ_BTNAME) {
		ret_val = act_test_read_bt_name(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_READ_BLENAME) {
		ret_val = act_test_read_ble_name(self->arg_buffer, self->arg_len);
	}else  {
	}

    return ret_val;
}


