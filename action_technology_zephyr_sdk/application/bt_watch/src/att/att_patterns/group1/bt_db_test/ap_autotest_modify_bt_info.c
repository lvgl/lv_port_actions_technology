/********************************************************************************
 *        Copyright(c) 2014-2016 Actions (Zhuhai) Technology Co., Limited,
 *                            All Rights Reserved.
 *
 * 描述：自动化测试：修改蓝牙地址，修改蓝牙设备名称。
 * 作者：zhouxl
 ********************************************************************************/

#include "att_pattern_test.h"


test_result_e act_test_modify_btaddr(u8_t *bt_addr, u32_t modify_config)
{
    
    test_result_e ret_val = TEST_PASS;
    //u8_t *ma_key_hex;
    //u32_t key_length;

    ret_val = act_test_set_bt_addr(bt_addr, 6);

    if(ret_val == 0){
        ret_val = TEST_PASS;
    }else{
        ret_val = TEST_FAIL;
    }

    //if (modify_config > 0x100)
    //{
    //    ma_key_hex = bt_addr + (BT_DEV_ADDR_LEN + 3) / 4 * 4;
    //    key_length = *((u32_t*)ma_key_hex);
    //    ma_key_hex += 4;
        //printf("MA length:%d\n", key_length);
        // vdisk_write(VFD_GMA_SECRET, ma_key_hex, key_length);
    //}

    return ret_val;
}




test_result_e act_test_modify_bt_addr(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;
    output_arg_t bt_addr_arg[2];

    act_test_read_arg(arg_buffer, bt_addr_arg, 2);

    ret_val = act_test_modify_btaddr(bt_addr_arg[0].arg, *((u32_t*)bt_addr_arg[1].arg));

    act_test_normal_report(TESTID_MODIFY_BTADDR, ret_val);

    return ret_val;
}


test_result_e act_test_modify_bt_name(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;
    output_arg_t bt_name_arg;
    char bt_dev_name[CFG_MAX_BT_DEV_NAME_LEN];

    act_test_read_arg(arg_buffer, &bt_name_arg, 1);

    if (bt_name_arg.arg_length <= sizeof(bt_dev_name))
    {
        memcpy(bt_dev_name, bt_name_arg.arg, bt_name_arg.arg_length);

        if (bt_name_arg.arg_length < sizeof(bt_dev_name))
        {
            memset(bt_dev_name + bt_name_arg.arg_length, 0, sizeof(bt_dev_name) - bt_name_arg.arg_length);
        }

        ret_val = act_test_set_bt_name(bt_dev_name, strlen(bt_dev_name));

        if(ret_val == 0){
            ret_val = TEST_PASS;
        }else{
            ret_val = TEST_FAIL;
        }
    }
    else
    {
        ret_val = TEST_BT_FAIL;
    }

    act_test_normal_report(TESTID_MODIFY_BTNAME, ret_val);

    return ret_val;
}

test_result_e act_test_modify_ble_name(void *arg_buffer, u32_t arg_len)
{
    test_result_e ret_val;
    output_arg_t bt_name_arg;
    char bt_dev_name[CFG_MAX_BT_DEV_NAME_LEN];

    act_test_read_arg(arg_buffer, &bt_name_arg, 1);

    if (bt_name_arg.arg_length <= sizeof(bt_dev_name))
    {
        memcpy(bt_dev_name, bt_name_arg.arg, bt_name_arg.arg_length);

        if (bt_name_arg.arg_length < sizeof(bt_dev_name))
        {
            memset(bt_dev_name + bt_name_arg.arg_length, 0, sizeof(bt_dev_name) - bt_name_arg.arg_length);
        }

        ret_val = act_test_set_bt_name(bt_dev_name, strlen(bt_dev_name));

        if(ret_val == 0){
            ret_val = TEST_PASS;
        }else{
            ret_val = TEST_FAIL;
        }
    }
    else
    {
        ret_val = TEST_BT_FAIL;
    }

    act_test_normal_report(TESTID_MODIFY_BLENAME, ret_val);

    return ret_val;
}

bool __ENTRY_CODE pattern_main(struct att_env_var *para)
{
    bool ret_val = false;

	memset(&__bss_start, 0,
		 ((u32_t) &__bss_end - (u32_t) &__bss_start));

	self = para;

	if (self->test_id == TESTID_MODIFY_BTADDR) {
		ret_val = act_test_modify_bt_addr(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_MODIFY_BTNAME) {
		ret_val = act_test_modify_bt_name(self->arg_buffer, self->arg_len);
	} else if (self->test_id == TESTID_MODIFY_BLENAME) {
		ret_val = act_test_modify_ble_name(self->arg_buffer, self->arg_len);
	}else  {
	}

    return ret_val;
}