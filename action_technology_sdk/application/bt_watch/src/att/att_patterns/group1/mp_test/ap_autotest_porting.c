#include "ap_autotest_rf_rx.h"

extern _bt_mp_test_arg_t g_mp_test_arg;
extern struct att_env_var *self;
extern cfo_return_arg_t g_cfo_return_arg;

void analyze_mp_test_para(void *arg_buffer, _bt_mp_test_arg_t *bt_mp_test_arg)
{
    output_arg_t bt_mp_arg[11];

    act_test_read_arg(arg_buffer, bt_mp_arg, ARRAY_SIZE(bt_mp_arg));

    bt_mp_test_arg->channel[0]      = *((u8_t*)bt_mp_arg[0].arg);
    bt_mp_test_arg->channel[1]      = 0xff;
    bt_mp_test_arg->channel[2]      = 0xff;

    bt_mp_test_arg->cfo_test_swtich = *((u8_t*)bt_mp_arg[1].arg);
	bt_mp_test_arg->cap_index_low           = *((u8_t*)bt_mp_arg[2].arg);
	bt_mp_test_arg->cap_index_high          = *((u8_t*)bt_mp_arg[3].arg);
    bt_mp_test_arg->cap_write_region= *((u8_t*)bt_mp_arg[4].arg);

    bt_mp_test_arg->cfo_low_limit   = *((s8_t*)bt_mp_arg[5].arg);
    bt_mp_test_arg->cfo_high_limit  = *((s8_t*)bt_mp_arg[6].arg);
    bt_mp_test_arg->upt_cfo_offset  = *((s16_t*)bt_mp_arg[7].arg);

    bt_mp_test_arg->rssi_test           = *((u8_t*)bt_mp_arg[8].arg);
    bt_mp_test_arg->rssi_low_limit      = *((s8_t*)bt_mp_arg[9].arg);
    bt_mp_test_arg->rssi_high_limit     = *((s8_t*)bt_mp_arg[10].arg);

    //bt_mp_test_arg->max_test_count  = *((u32_t*)bt_mp_arg[11].arg);
    //bt_mp_test_arg->result_timeout  = *((u32_t*)bt_mp_arg[12].arg);

    LOG_D("read arg channel %d index:%d~%d limit %d~%d\n", bt_mp_test_arg->channel[0], bt_mp_test_arg->cap_index_low, bt_mp_test_arg->cap_index_high, \
        bt_mp_test_arg->cfo_low_limit, bt_mp_test_arg->cfo_high_limit);
}

int act_test_read_rf_rx_arg(void)
{
	_bt_mp_test_arg_t *mp_arg = &g_mp_test_arg;

	analyze_mp_test_para(self->arg_buffer, mp_arg);

	mp_arg->ref_cap_index= (mp_arg->cap_index_low + mp_arg->cap_index_high) / 2;
	mp_arg->ber_test = false;

	return 0;
}


bool act_test_report_rf_rx_result(bool ret_val)
{
    return_result_t *return_data;
    u32_t trans_bytes;
    cfo_report_arg_t *report_arg;
    cfo_return_arg_t *cfo_return_arg = &g_cfo_return_arg; 

    trans_bytes = 4 + sizeof(cfo_report_arg_t);
    return_data = (return_result_t *)app_mem_malloc(trans_bytes);  

	report_arg = (cfo_report_arg_t *)((u8_t *)return_data + 4);

    report_arg->cfo_value = cfo_return_arg->cfo_val;
    report_arg->adjust_hosc_cap = cfo_return_arg->cap_index;

    if(ret_val == 1){
        ret_val = TEST_PASS;    
    }else{
        ret_val = TEST_FAIL;
    }

    return_data->test_result = ret_val;
    act_test_report_result(TESTID_READ_BTADDR, (u8_t*)return_data, trans_bytes);

    app_mem_free(return_data);
    return ret_val;
}



