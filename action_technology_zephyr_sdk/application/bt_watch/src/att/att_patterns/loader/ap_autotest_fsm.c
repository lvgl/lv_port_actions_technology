/*******************************************************************************
 *                              US212A
 *                            Module: Picture
 *                 Copyright(c) 2003-2012 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>    <time>           <version >             <desc>
 *       zhangxs     2011-09-19 10:00     1.0             build this file
 *******************************************************************************/
#include "att_pattern_test.h"
#include "ap_autotest_loader.h"

#if 0
const att_task_stru_t autotest_ops[] =
{
    {TESTID_MODIFY_BTNAME,      act_test_modify_bt_name     },
    {TESTID_MODIFY_BTADDR,      act_test_modify_bt_addr     },
    {TESTID_READ_BTADDR,        act_test_read_bt_addr       },
    {TESTID_READ_BTNAME,        act_test_read_bt_name       },
    {TESTID_BT_TEST,            act_test_bt_test            },
    {TESTID_LEFT_MIC_VERIFY,    act_test_left_mic_verify    },
    {TESTID_RIGHT_MIC_VERIFY,   act_test_right_mic_verify   },
    {TESTID_MP_CFO,             act_test_cfo_adjust         },
    {TESTID_MP_CFO_READ,        act_test_cfo_read           },

    {TESTID_SERIAL_NUMBER_WRITE,act_test_sn_write           },
    {TESTID_PRODUCT_INFO_WRITE, act_test_product_info_write },
};
#endif

static const struct att_code_info att_patterns_list[] =
{
	{ TESTID_MODIFY_BTNAME,    0, 0, "p01_bta.bin" },
	{ TESTID_MODIFY_BLENAME,   0, 0, "p01_bta.bin" },
	{ TESTID_MODIFY_BTADDR,    0, 0, "p01_bta.bin" },
//	{ TESTID_MODIFY_BTADDR2,   0, 0, "p01_bta.bin" },

	{ TESTID_READ_BTNAME,      0, 0, "p02_bta.bin" },
	{ TESTID_READ_BTADDR,      0, 0, "p02_bta.bin" },
	{ TESTID_READ_FW_VERSION,  0, 0, "p02_bta.bin" },

	{ TESTID_BQBMODE,          0, 0, "p03_bqb.bin" },
//	{ TESTID_FCCMODE,          0, 1, "p04_fcc.bin" },

	{ TESTID_MP_CFO,          0, 1, "p05_rx.bin" },
	{ TESTID_MP_CFO_READ,     0, 1, "p05_rx.bin" },

	{ TESTID_PWR_TEST,         0, 1, "p06_tx.bin" },

	{ TESTID_BT_TEST,          0, 0, "p07_btl.bin" },
//	{ TESTID_LED_TEST,         0, 0, "p10_led.bin" },
//	{ TESTID_KEY_TEST,         0, 0, "p11_key.bin" },
//	{ TESTID_CAP_CTR_START,    0, 0, "p12_bat.bin" },
//	{ TESTID_CAP_TEST,         0, 0, "p12_bat.bin" },

	{ TESTID_GPIO_TEST,        0, 0, "p18_gpio.bin" },
//	{ TESTID_AGING_PB_START,   0, 0, "p19_age.bin" },
//	{ TESTID_AGING_PB_CHECK,   0, 0, "p19_age.bin" },
	{ TESTID_LINEIN_CH_TEST,   0, 0, "p20_adc.bin" },
	{ TESTID_MIC_CH_TEST,      0, 0, "p20_adc.bin" },
	{ TESTID_FM_CH_TEST,       0, 0, "p20_adc.bin" },
    {TESTID_SERIAL_NUMBER_WRITE, 0, 0, "p30_sn.bin"},
	{TESTID_SERIAL_NUMBER_READ,  0, 0, "p30_sn.bin"},
	{TESTID_UUID_READ,  	0, 0, 		"uuid.bin"},

};


static const struct att_code_info* match_testid(int testid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(att_patterns_list); i++) {
		if (att_patterns_list[i].testid == testid)
			return &att_patterns_list[i];
	}

	return NULL;
}

// static const char suffix[] = {'h', 'e', 'x'};
static void test_modify_testsuffix(char *pt_name, const char *test_name, int max_len)
{
	int i, j;
	int flag = false;

	memcpy(pt_name, test_name, max_len);
	return;
	// if(self->api->compiler_type == 1){
	// 	memcpy(pt_name, test_name, max_len);	
	// 	return;
	// }
	
	// for(i = 0, j = 0; i < max_len; i++){
	// 	if(!flag){
	// 		pt_name[i] = test_name[i];		
	// 	}else{
	// 		pt_name[i] = suffix[j];	
	// 		j++;
	// 		if(j == 3){
	// 			break;
	// 		}
	// 	}

	// 	if(test_name[i] == '.'){
	// 		flag = true;
	// 	}				
	// }

	// pt_name[i + 1] = '\0';
}

int test_load_code(void)
{
	int ret_val;
    const struct att_code_info *cur_att_code_info;
    static int (*att_code_entry)(struct att_env_var *para) = NULL;
	atf_dir_t dir;
	char test_name[13];

	/* load pattern */
	cur_att_code_info = match_testid(g_test_info.test_id);
	if (cur_att_code_info != NULL) {
		test_modify_testsuffix(test_name, cur_att_code_info->pt_name, ATF_MAX_SUB_FILENAME_LENGTH);
		LOG_I("old test %s new %s\n", g_test_info.last_pt_name, test_name);
		if (strncmp(g_test_info.last_pt_name, test_name, ATF_MAX_SUB_FILENAME_LENGTH) != 0) {
			ret_val = read_atf_sub_file(NULL, 0x10000, (const u8_t *)test_name, 0, -1, &dir);
			if (ret_val <= 0) {
				LOG_E("read_atf_sub_file fail, quit!\n");
				goto err_deal;
			}

			memcpy(g_test_info.last_pt_name, test_name, sizeof(g_test_info.last_pt_name));

			att_code_entry = (int (*)(struct att_env_var *))((void *)(dir.run_addr));
		}

		if (att_code_entry != NULL) {

			att_buf_printf("code %s entry %x size %d\n", test_name, (u32_t)att_code_entry, dir.length);

			if (att_code_entry(&g_att_env_var) != TEST_PASS) {
				LOG_E("att_code_entry return false, quit!\n");
				goto err_deal;
			}
		}
	} else {
		LOG_E("match_testid fail, quit,id=%d !\n", g_test_info.test_id);
		goto err_deal;
	}

	return 0;
err_deal:
	return -1;
}

void test_dispatch(void)
{
    u8_t *arg_buffer;
    att_test_item_para test_item;

    // SYS_LOG_INF("%d", __LINE__);
    // 测试项参数长度不能超过 256Byte
    arg_buffer = (u8_t *)malloc(256);

	g_att_env_var.arg_buffer = arg_buffer;

	while (1)
	{
        wait_until_stub_idle();
        if (sizeof(test_item)
            != stub_read_packet(STUB_CMD_ATT_GET_TEST_ID, 0, (u8_t*)&test_item, sizeof(test_item)))
        {
            k_sleep(100);
            continue;
        }

        if(TESTID_ID_WAIT == test_item.id)
        {
            k_sleep(500);
            continue;
        }

        if(TESTID_ID_QUIT == test_item.id)
        {
            break;
        }

        if (test_item.para_length)
        {
            if (test_item.para_length
                != stub_read_packet(STUB_CMD_ATT_GET_TEST_ARG, 0, arg_buffer, test_item.para_length))
            {
                LOG_E("Read para failed.\n");
                k_sleep(100);
                continue;
            }
        }

		g_test_info.test_id = test_item.id;
		g_test_info.arg_len = test_item.para_length;
		
		att_buf_printf("test id:%d arg len %d", g_test_info.test_id, g_test_info.arg_len);

		g_att_env_var.test_id = g_test_info.test_id;
		g_att_env_var.arg_len = g_test_info.arg_len;

		if (test_load_code() != 0)
		{
			break;
		}
	}

	free(arg_buffer);
	LOG_I("test end\n");

}


