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
#include "stub.h"

#define CFG_BT_MAC                  "BT_MAC"
#define CFG_BT_NAME                 "BT_NAME"
#define CFG_BLE_NAME                "BT_LE_NAME"
#define CFG_BT_BQB_MODE			    "BT_BQB_MODE"
#define CFG_AUTOCONN_INFO			"BT_AUTOCONN_INFO"
#define CFG_SN_INFO 			  	"SN_NUM"
#define CFG_BT_CFO                  "BT_CFO_VAL"

int act_test_report_result(u32_t test_id, u8_t *result_data, u32_t result_len)
{
    return stub_write_packet(STUB_CMD_ATT_REPORT_TRESULT, test_id, result_data, result_len);
}

int act_test_normal_report(u32_t test_id, test_result_e result)
{
    return_result_t return_data;

    return_data.test_result = result;

    LOG_I("report id:%d result:%d\n", test_id, result);

    return stub_write_packet(STUB_CMD_ATT_REPORT_TRESULT, test_id, (u8_t*)&return_data, 4);
}


int act_report_test_para(u32_t para_id, u8_t *para, u32_t para_len)
{
    return stub_write_packet(STUB_CMD_ATT_REPORT_PARA, para_id, para, para_len);
}

int act_log_to_pc(u8_t *str, u32_t len)
{
    return stub_write_packet(STUB_CMD_ATT_PRINT_LOG, 0, str, len);
}


int act_get_test_middle_result(u32_t result_id, u8_t *result, u32_t result_len)
{
    return stub_read_packet(STUB_CMD_ATT_GET_TEST_RESULT, result_id, result, result_len);
}


void wait_until_stub_idle(void)
{
    int ret;

    while (1)
    {
        ret = stub_status_inquiry();

        if (STUB_PROTOCOL_OK == ret)
        {
            //wait until stub protocol is NOT busy
            break;
        }
        else if (STUB_PROTOCOL_ERROR == ret)
        {
            // error happend
            while(1)
            {
                k_sleep(100);
            }
        }

        k_sleep(30);
    }
}




int act_test_inform_state(u32_t state)
{
    return stub_write_packet(STUB_CMD_ATT_INFORM_STATE, state, NULL, 0);
}


int act_test_set_bt_name(char *name_str, uint32_t name_len)
{
	return property_set_factory(CFG_BT_NAME, name_str, name_len);
}

int act_test_set_bt_blename(char *name_str, uint32_t name_len)
{
	return property_set_factory(CFG_BLE_NAME, name_str, name_len);
}

int act_test_set_bt_addr(uint8_t *mac_value, uint32_t mac_len)
{
	char mac_bin[6];
	char mac_str[MAC_STR_LEN];
	int i;

	if(mac_len != 6){
		return -EINVAL;
	}

	for(i=0; i<6; i++)/*big endian  to little endian*/
	{
		mac_bin[i] = mac_value[6-i-1];
	}

	bin2hex((const uint8_t *)mac_bin, 6, mac_str, 12);

	return property_set_factory(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
}

int act_test_set_bt_bqbflag(uint8_t flag)
{
    if(flag == 0){
        flag = '0';
    }else{
        flag = '1';
    }

	return property_set(CFG_BT_BQB_MODE, (char *)&flag, sizeof(flag));
}

int act_test_set_sn_info(char *sn_str, uint32_t sn_str_len)
{
	return property_set_factory(CFG_SN_INFO, sn_str, sn_str_len);    
}

int act_test_get_bt_name(char *name_str, uint32_t name_len)
{
    int ret, len;
	char bt_name[BT_NAME_STR_LEN];

    memset(bt_name, 0, BT_NAME_STR_LEN);
	ret = property_get(CFG_BT_NAME, bt_name, (BT_NAME_STR_LEN-1));
	if (ret > 0) {
		len = strlen(bt_name);

		if(len > name_len){
			len = name_len;
		}

		memcpy(name_str, bt_name, len);

		ret = len;
	}

	return ret;
}

int act_test_get_bt_blename(char *name_str, uint32_t name_len)
{
    int ret, len;
	char bt_name[BT_NAME_STR_LEN];

    memset(bt_name, 0, BT_NAME_STR_LEN);
	ret = property_get(CFG_BLE_NAME, bt_name, (BT_NAME_STR_LEN-1));
	if (ret > 0) {
		len = strlen(bt_name);

		if(len > name_len){
			len = name_len;
		}

		memcpy(name_str, bt_name, len);

		ret = len;
	}

	return ret;
}

int act_test_get_bt_addr(uint8_t *mac_value, uint32_t mac_len)
{
    int ret;
	char mac_str[MAC_STR_LEN];

	if(mac_len != 6){
		return -EINVAL;
	}

	ret = property_get(CFG_BT_MAC, mac_str, (MAC_STR_LEN-1));
	if (ret > 0) {
		hex2bin(mac_str, 12, mac_value, mac_len);
	}

	return ret;
}

int act_test_get_sn_info(char *sn_str, uint32_t sn_str_len)
{
    int ret, len;
	char sn_name[SN_STR_LEN];

    memset(sn_name, 0, SN_STR_LEN);
	ret = property_get(CFG_SN_INFO, sn_name, (SN_STR_LEN-1));
	if (ret > 0) {
		len = strlen(sn_name);

		if(len > sn_str_len){
			len = sn_str_len;
		}

		memcpy(sn_str, sn_name, len);

		ret = len;
	}

	return ret; 
}

int32_t act_test_freq_compensation_param_write_factory(uint8_t *trim_cap)
{
    char temp[8];
    memset(temp, 0, sizeof(temp));

    bin2hex((const uint8_t *)temp, sizeof(temp), (char *)trim_cap, sizeof(uint8_t));

    return property_set_factory(CFG_BT_CFO, temp, strlen(temp) + 1);
}

int32_t act_test_freq_compensation_param_read_factory(uint8_t *trim_cap)
{
    char temp[8];
    uint8_t temp_val;
    memset(temp, 0, sizeof(temp));

    int ret = property_get(CFG_BT_CFO, temp, sizeof(temp));

    if (ret >= 0){
        hex2bin(temp, sizeof(temp), &temp_val, sizeof(uint8_t));
        *trim_cap = temp_val;
    }

    return ret;
}