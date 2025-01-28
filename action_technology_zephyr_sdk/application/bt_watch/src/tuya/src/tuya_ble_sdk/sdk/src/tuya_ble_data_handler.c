
/**
 * \file tuya_ble_data_handler.c
 *
 * \brief
 */
/*
 *  Copyright (C) 2014-2019, Tuya Inc., All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of tuya ble sdk
 */

#if 1//def CFG_TUYA_BLE_APP_SUPPORT

#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_internal_config.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_main.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_unix_time.h"
#include "tuya_ble_log.h"
#include "tuya_ble_gatt_send_queue.h"
#include "tuya_ble_bulk_data.h"
#include "tuya_ble_vos.h"
#include "tuya_ble_app_production_test.h"
#include "tuya_ble_app_uart_common_handler.h"
#if (TUYA_BLE_FEATURE_WEATHER_ENABLE != 0)
#include "tuya_ble_feature_weather.h"
#endif
#if ( TUYA_BLE_FEATURE_IOT_CHANNEL_ENABLE != 0)
#include "tuya_ble_feature_iot_channel.h"
#endif 
#if TUYA_BLE_VOS_ENABLE
#include "tuya_ble_vos.h"
#endif
#include <bt_manager.h>

static uint32_t tuya_ble_firmware_version = 0;
static uint32_t tuya_ble_hardware_version = 0;

static uint32_t tuya_ble_mcu_firmware_version = 0;
static uint32_t tuya_ble_mcu_hardware_version = 0;


static  tuya_ble_r_air_recv_packet  air_recv_packet;  

static frm_trsmitr_proc_s ty_trsmitr_proc;
static uint32_t send_packet_data_len = 20;

uint8_t tuya_ble_pair_rand[6] = {0};
uint8_t tuya_ble_pair_rand_valid = 0;

static uint32_t tuya_ble_receive_sn = 0;
static uint32_t tuya_ble_send_sn = 1;


tuya_ble_ota_status_t tuya_ble_ota_status;
#define isspace(_c)  (((((unsigned int)(_c)) - '\t') < 5U) || ((_c) == ' '))

int my_atoi(char* pstr)
{
	int Ret_Integer = 0;
	int Integer_sign = 1;
	
	/*
	* 判断指针是否为空
	*/
	if(pstr == NULL)
	{
		printf("Pointer is NULL\n");
		return 0;
	}
	
	/*
	* 跳过前面的空格字符
	*/
	while(isspace(*pstr) == 0)
	{
		pstr++;
	}
	
	/*
	* 判断正负号
	* 如果是正号，指针指向下一个字符
	* 如果是符号，把符号标记为Integer_sign置-1，然后再把指针指向下一个字符
	*/
	if(*pstr == '-')
	{
		Integer_sign = -1;
	}
	if(*pstr == '-' || *pstr == '+')
	{
		pstr++;
	}
	
	/*
	* 把数字字符串逐个转换成整数，并把最后转换好的整数赋给Ret_Integer
	*/
	while(*pstr >= '0' && *pstr <= '9')
	{
		Ret_Integer = Ret_Integer * 10 + *pstr - '0';
		pstr++;
	}
	Ret_Integer = Integer_sign * Ret_Integer;
	
	return Ret_Integer;
}


#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==4)

#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)

static uint8_t current_ser_cert_pub_key[64];
static uint8_t current_dev_random_for_sign[6];
static tuya_ble_auth_status_t current_auth_status;

void tuya_ble_auth_data_reset(void)
{
    tuya_ble_device_enter_critical();
    current_auth_status = TUYA_BLE_AUTH_STATUS_PHASE_NONE;
    memset(current_ser_cert_pub_key,0,sizeof(current_ser_cert_pub_key));
    memset(current_dev_random_for_sign,0,sizeof(current_dev_random_for_sign));
    tuya_ble_device_exit_critical();
}

#endif

#endif

static uint32_t get_ble_send_sn(void)
{
    uint32_t sn;
    tuya_ble_device_enter_critical();
    sn = tuya_ble_send_sn++;
    tuya_ble_device_exit_critical();
    return sn;
}

static void set_ble_receive_sn(uint32_t sn)
{
    tuya_ble_device_enter_critical();
    tuya_ble_receive_sn = sn;
    tuya_ble_device_exit_critical();
}

void tuya_ble_reset_ble_sn(void)
{
    tuya_ble_device_enter_critical();
    tuya_ble_receive_sn = 0;
    tuya_ble_send_sn = 1;
    tuya_ble_device_exit_critical();
}


void tuya_ble_set_device_version(uint32_t firmware_version,uint32_t hardware_version)
{
    tuya_ble_firmware_version = firmware_version;
    tuya_ble_hardware_version = hardware_version;
}


void tuya_ble_set_external_mcu_version(uint32_t firmware_version,uint32_t hardware_version)
{
    tuya_ble_device_enter_critical();
    tuya_ble_mcu_firmware_version = firmware_version;
    tuya_ble_mcu_hardware_version = hardware_version;
    tuya_ble_device_exit_critical();
}


void tuya_ble_ota_status_set(tuya_ble_ota_status_t status)
{
    tuya_ble_ota_status = status;
}


tuya_ble_ota_status_t tuya_ble_ota_status_get(void)
{
    return tuya_ble_ota_status;
}


void tuya_ble_pair_rand_clear(void)
{
    tuya_ble_device_enter_critical();
    memset(tuya_ble_pair_rand,0,sizeof(tuya_ble_pair_rand));
    tuya_ble_pair_rand_valid = 0;
    tuya_ble_device_exit_critical();
}


uint8_t tuya_ble_pair_rand_valid_get(void)
{
    return tuya_ble_pair_rand_valid;
}


uint32_t tuya_ble_send_packet_data_length_get(void)
{
    return send_packet_data_len;
}


static bool buffer_value_is_all_x(uint8_t *buffer,uint16_t len,uint8_t value)
{
    bool ret = true;
	uint16_t i = 0;
    for(i = 0; i<len; i++)
    {
        if(buffer[i]!= value)
        {
            ret = false;
            break;
        }
    }
    return ret;
}

void tuya_ble_air_recv_packet_free(void)
{
    if(air_recv_packet.recv_data)
    {
        tuya_ble_free(air_recv_packet.recv_data);
        air_recv_packet.recv_data = NULL;
        air_recv_packet.recv_len_max = 0;
        air_recv_packet.recv_len = 0;
    }
}

static uint32_t ble_data_unpack(uint8_t *buf,uint32_t len)
{
    static uint32_t offset = 0;
    mtp_ret ret;

    ret = trsmitr_recv_pkg_decode(&ty_trsmitr_proc, buf, len);
    if(MTP_OK != ret && MTP_TRSMITR_CONTINUE != ret)
    {
        air_recv_packet.recv_len_max = 0;
        air_recv_packet.recv_len = 0;
        if(air_recv_packet.recv_data)
        {
            tuya_ble_free(air_recv_packet.recv_data);
            air_recv_packet.recv_data = NULL;
        }

        return 1;
    }

    if(FRM_PKG_FIRST == ty_trsmitr_proc.pkg_desc)
    {
        if(air_recv_packet.recv_data)
        {
            tuya_ble_free(air_recv_packet.recv_data);
            air_recv_packet.recv_data = NULL;
        }
        air_recv_packet.recv_len_max = get_trsmitr_frame_total_len(&ty_trsmitr_proc);
        if((air_recv_packet.recv_len_max>TUYA_BLE_AIR_FRAME_MAX)||(air_recv_packet.recv_len_max==0))
        {
            air_recv_packet.recv_len_max = 0;
            air_recv_packet.recv_len = 0;
            TUYA_BLE_LOG_ERROR("ble_data_unpack total size [%d ]error.",air_recv_packet.recv_len_max);
            return 2;
        }
        air_recv_packet.recv_len = 0;
        air_recv_packet.recv_data = tuya_ble_malloc(air_recv_packet.recv_len_max);
        if(air_recv_packet.recv_data==NULL)
        {
            TUYA_BLE_LOG_ERROR("ble_data_unpack malloc failed.");
            return 2;
        }
        memset(air_recv_packet.recv_data,0,air_recv_packet.recv_len_max);
        offset = 0;
    }
    if((offset+get_trsmitr_subpkg_len(&ty_trsmitr_proc))<=air_recv_packet.recv_len_max)
    {
        if(air_recv_packet.recv_data)
        {
            memcpy(air_recv_packet.recv_data+offset,get_trsmitr_subpkg(&ty_trsmitr_proc),get_trsmitr_subpkg_len(&ty_trsmitr_proc));
            offset += get_trsmitr_subpkg_len(&ty_trsmitr_proc);
            air_recv_packet.recv_len = offset;
        }
        else
        {
            TUYA_BLE_LOG_ERROR("ble_data_unpack error.");
            air_recv_packet.recv_len_max = 0;
            air_recv_packet.recv_len = 0;
            return 2;
        }
    }
    else
    {
        ret = MTP_INVALID_PARAM;
        TUYA_BLE_LOG_ERROR("ble_data_unpack[] error:MTP_INVALID_PARAM");
        tuya_ble_air_recv_packet_free();
    }

    if(ret == MTP_OK)
    {
        offset=0;
        TUYA_BLE_LOG_DEBUG("ble_data_unpack[%d]",air_recv_packet.recv_len);

        return 0;
    }
    else
    {
        return 2;
    }
}

static uint8_t ble_cmd_data_crc_check(uint8_t *input,uint16_t len)
{
    uint16_t data_len = 0;
    uint16_t crc16 = 0xFFFF;
    uint16_t crc16_cal = 0;

    data_len = (input[10]<<8)|input[11];

    if(data_len>=TUYA_BLE_RECEIVE_MAX_DATA_LEN)
    {
        return 1;
    }

    crc16_cal = tuya_ble_crc16_compute(input,12+data_len, &crc16);

    TUYA_BLE_LOG_DEBUG("crc16_cal[0x%04x]",crc16_cal);
    crc16 = (input[12+data_len]<<8)|input[13+data_len];
    TUYA_BLE_LOG_DEBUG("crc16[0x%04x]",crc16);
    if(crc16==crc16_cal)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


void tuya_ble_commonData_rx_proc(uint8_t *buf,uint16_t len)
{
    uint8_t temp;
    uint32_t current_sn = 0;
    uint16_t current_cmd = 0;
    tuya_ble_evt_param_t *evt = NULL;
    uint8_t *ble_evt_buffer=NULL;
    uint8_t current_encry_mode = 0;
    uint16_t p_version = 0;
	bool is_cmd_with_encry_mode_correct = true;

	if (!ty_trsmitr_proc.subpkg)
		ty_trsmitr_proc.subpkg = (u8_t *)tuya_ble_malloc(SNGL_PKG_TRSFR_LMT);

    if(ble_data_unpack(buf,len))
    {
    	tuya_ble_free(ty_trsmitr_proc.subpkg);
		ty_trsmitr_proc.subpkg = NULL;
        return;      //
    }
	tuya_ble_free(ty_trsmitr_proc.subpkg);
	ty_trsmitr_proc.subpkg = NULL;

    if(air_recv_packet.recv_len>TUYA_BLE_AIR_FRAME_MAX)
    {
        TUYA_BLE_LOG_ERROR("air_recv_packet.recv_len bigger than TUYA_BLE_AIR_FRAME_MAX.");
        tuya_ble_air_recv_packet_free();
        return;
    }

    if(ty_trsmitr_proc.version<2)  
    {
        TUYA_BLE_LOG_ERROR("ty_ble_rx_proc version not compatibility!");
        tuya_ble_air_recv_packet_free();
        return;
    }


    if(tuya_ble_current_para->sys_settings.bound_flag==1)
    {
        if((ENCRYPTION_MODE_NONE==air_recv_packet.recv_data[0]) || (ENCRYPTION_MODE_FTM_KEY==air_recv_packet.recv_data[0]))
        {
            TUYA_BLE_LOG_ERROR("ty_ble_rx_proc data encryption mode error since bound_flag = 1.");
            tuya_ble_air_recv_packet_free();
            return;
        }
    }
    current_encry_mode = air_recv_packet.recv_data[0];

    //TUYA_BLE_LOG_HEXDUMP_DEBUG("received encry data",(uint8_t*)air_recv_packet.recv_data,air_recv_packet.recv_len);//

    air_recv_packet.de_encrypt_buf = NULL;
    
    air_recv_packet.de_encrypt_buf = (uint8_t*)tuya_ble_malloc(air_recv_packet.recv_len);
    
    if(air_recv_packet.de_encrypt_buf==NULL)
    {
        TUYA_BLE_LOG_ERROR("air_recv_packet.de_encrypt_buf malloc failed.");
        tuya_ble_air_recv_packet_free();
        return;
    }
    else
    {
        p_version = (TUYA_BLE_PROTOCOL_VERSION_HIGN<<8) + TUYA_BLE_PROTOCOL_VERSION_LOW;
        air_recv_packet.decrypt_buf_len = 0;
		//uart_print_enable(FALSE);
        temp = tuya_ble_decryption(p_version,(uint8_t *)air_recv_packet.recv_data,air_recv_packet.recv_len,&air_recv_packet.decrypt_buf_len,
        (uint8_t *)air_recv_packet.de_encrypt_buf,tuya_ble_current_para,tuya_ble_pair_rand);
		//uart_print_enable(TRUE);
        tuya_ble_air_recv_packet_free();
    }
    
   
    if(temp != 0) //解密失败
    {
        TUYA_BLE_LOG_ERROR("ble receive data decryption error code = %d",temp);
        tuya_ble_free(air_recv_packet.de_encrypt_buf);
        return;
    }

    //TUYA_BLE_LOG_HEXDUMP_DEBUG("decryped data",(uint8_t*)air_recv_packet.de_encrypt_buf,air_recv_packet.decrypt_buf_len);//解密数据
    //指令数据crc验证
    if(ble_cmd_data_crc_check((uint8_t *)air_recv_packet.de_encrypt_buf,air_recv_packet.decrypt_buf_len)!=0)
    {
        TUYA_BLE_LOG_ERROR("ble receive data crc check error!");
        tuya_ble_free(air_recv_packet.de_encrypt_buf);
        return;
    }

#if TUYA_BLE_LINK_LAYER_ENCRYPTION_SUPPORT_ENABLE	
	
        if(current_cmd == FRM_QRY_DEV_INFO_REQ)
		{
			tuya_ble_reset_ble_sn();
		}
		
#endif		
  
    current_sn  = air_recv_packet.de_encrypt_buf[0]<<24;
    current_sn += air_recv_packet.de_encrypt_buf[1]<<16;
    current_sn += air_recv_packet.de_encrypt_buf[2]<<8;
    current_sn += air_recv_packet.de_encrypt_buf[3];

    if(current_sn<=tuya_ble_receive_sn)
    {
        TUYA_BLE_LOG_ERROR("ble receive SN error!");
        tuya_ble_gap_disconnect();//SN错误，断开蓝牙连接
        tuya_ble_free(air_recv_packet.de_encrypt_buf);
        return;
    }
    else
    {
        set_ble_receive_sn(current_sn);
    }

    current_cmd = ((air_recv_packet.de_encrypt_buf[8]<<8)|air_recv_packet.de_encrypt_buf[9]);

	if(current_cmd == FRM_FACTORY_TEST_CMD)
	{
		// Check recvived cmd with encry mode corresponding relationship
		#if (TUYA_BLE_PROD_TEST_SUPPORT_ENCRYPTION != 0)
		if(current_encry_mode != ENCRYPTION_MODE_FTM_KEY)
			is_cmd_with_encry_mode_correct = false;
		#else 
		if(current_encry_mode != ENCRYPTION_MODE_NONE)
			is_cmd_with_encry_mode_correct = false;
		#endif

		if(!is_cmd_with_encry_mode_correct)
		{
			tuya_ble_free(air_recv_packet.de_encrypt_buf);
			TUYA_BLE_LOG_ERROR("ble receive cmd error on prod factory test state, need encrypt!");
			return;
		}
	}

    if((BONDING_CONN != tuya_ble_connect_status_get())&&(FRM_QRY_DEV_INFO_REQ != current_cmd)&&(PAIR_REQ != current_cmd)
            &&(FRM_LOGIN_KEY_REQ != current_cmd)&&(FRM_FACTORY_TEST_CMD != current_cmd)&&(FRM_NET_CONFIG_INFO_REQ != current_cmd)&&(FRM_ANOMALY_UNBONDING_REQ != current_cmd)
            &&(FRM_AUTHENTICATE_PHASE_1_REQ != current_cmd)&&(FRM_AUTHENTICATE_PHASE_2_REQ != current_cmd)&&(FRM_AUTHENTICATE_PHASE_3_REQ != current_cmd))
    {   
        tuya_ble_free(air_recv_packet.de_encrypt_buf);
        TUYA_BLE_LOG_ERROR("ble receive cmd error on current bond state!");
        return;
    }


    if(tuya_ble_ota_status_get()!=TUYA_BLE_OTA_STATUS_NONE)
    {   //OTA状态下，不处理其它事件
        if(!((current_cmd>=FRM_OTA_START_REQ)&&(current_cmd<=FRM_OTA_END_REQ)))
        {
            tuya_ble_free(air_recv_packet.de_encrypt_buf);
            TUYA_BLE_LOG_ERROR("ble receive cmd error on ota state!");
            return;
        }
    }

    ble_evt_buffer=(uint8_t*)tuya_ble_malloc(air_recv_packet.decrypt_buf_len+1);
    if(ble_evt_buffer==NULL)
    {
        TUYA_BLE_LOG_ERROR("ty_ble_rx_proc no mem.");
        tuya_ble_free(air_recv_packet.de_encrypt_buf);
        return;
    }
    else
    {
        memset(ble_evt_buffer,0,air_recv_packet.decrypt_buf_len+1);
    }
    ble_evt_buffer[0] = current_encry_mode;     //首字节拷贝加密方式，便于后续使用
    memcpy(ble_evt_buffer+1,(uint8_t *)air_recv_packet.de_encrypt_buf,air_recv_packet.decrypt_buf_len);
	evt = (tuya_ble_evt_param_t *)tuya_ble_malloc(sizeof(tuya_ble_evt_param_t));
    evt->hdr.event = TUYA_BLE_EVT_BLE_CMD;
    evt->ble_cmd_data.cmd = current_cmd;
    evt->ble_cmd_data.p_data = ble_evt_buffer;
    evt->ble_cmd_data.data_len = air_recv_packet.decrypt_buf_len+1;
    TUYA_BLE_LOG_DEBUG("BLE EVENT SEND-CMD:0x%02x - LEN:0x%02x",current_cmd,air_recv_packet.decrypt_buf_len+1);

    if(tuya_ble_event_send(evt)!=0)
    {
        TUYA_BLE_LOG_ERROR("ble event send fail!");
        tuya_ble_free(ble_evt_buffer);
    }
	
	tuya_ble_free((u8_t *)evt);
    tuya_ble_free((u8_t *)air_recv_packet.de_encrypt_buf);
}


#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==4)

static void tuya_ble_handle_dev_info_req(uint8_t *recv_data,uint16_t recv_len)
{
    uint8_t p_buf[112];
    uint8_t payload_len = 0;
	uint8_t index = 0;
    uint32_t ack_sn = 0;
    uint8_t encry_mode = 0;
    uint32_t version_temp_s,version_temp_h;
	tuya_ble_gap_addr_t mac_addr;
    uint16_t data_len = recv_data[11]<<8|recv_data[12];

    if(data_len!=2)
    {
        TUYA_BLE_LOG_ERROR("get device infor cmd error with v4,data_len = %d",data_len);
        tuya_ble_gap_disconnect();
        return;
    }

    memset(p_buf,0,sizeof(p_buf));

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    TUYA_BLE_LOG_DEBUG("get device infor-%d",tuya_ble_current_para->sys_settings.bound_flag);

    send_packet_data_len = recv_data[13]<<8|recv_data[14];

    if((send_packet_data_len==0)||(send_packet_data_len>TUYA_BLE_DATA_MTU_MAX))
    {
        send_packet_data_len = 20;
    }

    TUYA_BLE_LOG_DEBUG("current app mtu data len = %d",send_packet_data_len);

    tuya_ble_rand_generator(tuya_ble_pair_rand,6);
    tuya_ble_pair_rand_valid = 1;

    if(TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
    {
        version_temp_s = tuya_ble_firmware_version;
        version_temp_h = tuya_ble_hardware_version;
        p_buf[4] = 0x00;
    }
    else
    {
        version_temp_s = tuya_ble_firmware_version>>8;
        version_temp_h = tuya_ble_hardware_version>>8;
        p_buf[4] = 0x05;
    }
    p_buf[0] = (version_temp_s>>8)&0xff;
    p_buf[1] = (version_temp_s&0xff);
    p_buf[2] = TUYA_BLE_PROTOCOL_VERSION_HIGN;
    p_buf[3] = TUYA_BLE_PROTOCOL_VERSION_LOW;

#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)

    p_buf[4] |= 0x02;
    if(TUYA_BLE_ADVANCED_ENCRYPTION_AUTH_ON_CONNECT)
    {
        p_buf[4] |= 0x08;
    }

#endif

#if (TUYA_BLE_PROTOCOL_VERSION_LOW>=1)

    if(TUYA_BLE_BEACON_KEY_ENABLE)
    {
        p_buf[4] |= 0x10;
    }
	
	p_buf[4] |= 0x40;  //alexa enable

#endif

    p_buf[5] = tuya_ble_current_para->sys_settings.bound_flag;
    memcpy(&p_buf[6],tuya_ble_pair_rand,6);
    p_buf[12] = (version_temp_h>>8)&0xff;
    p_buf[13] = (version_temp_h&0xff);

    tuya_ble_register_key_generate(&p_buf[14],tuya_ble_current_para);

    p_buf[46] = (tuya_ble_firmware_version>>16)&0xff;
    p_buf[47] = (tuya_ble_firmware_version>>8)&0xff;
    p_buf[48] = (tuya_ble_firmware_version&0xff);
    p_buf[49] = (tuya_ble_hardware_version>>16)&0xff;
    p_buf[50] = (tuya_ble_hardware_version>>8)&0xff;
    p_buf[51] = (tuya_ble_hardware_version&0xff);

    p_buf[52] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY>>8;
    p_buf[53] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY; //communication ability

    p_buf[54] = 0x00;

    memcpy(&p_buf[55],tuya_ble_current_para->sys_settings.device_virtual_id,DEVICE_VIRTUAL_ID_LEN);

    p_buf[77] = (tuya_ble_mcu_firmware_version>>16)&0xff;
    p_buf[78] = (tuya_ble_mcu_firmware_version>>8)&0xff;
    p_buf[79] = (tuya_ble_mcu_firmware_version&0xff);
    p_buf[80] = (tuya_ble_mcu_hardware_version>>16)&0xff;
    p_buf[81] = (tuya_ble_mcu_hardware_version>>8)&0xff;
    p_buf[82] = (tuya_ble_mcu_hardware_version&0xff);

    p_buf[83] = TUYA_BLE_WIFI_DEVICE_REGISTER_MODE;

#if (TUYA_BLE_PROTOCOL_VERSION_LOW>=2)

	memset(&p_buf[84],0,4);

	p_buf[87] |= 0x40;
	SYS_LOG_INF("");
	if(TUYA_BLE_LINK_LAYER_ENCRYPTION_SUPPORT_ENABLE)
	{
		p_buf[87] |= 0x08;
	}
	
	tuya_ble_gap_addr_get(&mac_addr);
	
	p_buf[88] = mac_addr.addr_type;
	
	memcpy(&p_buf[89],mac_addr.addr,6);

    p_buf[95] = strlen(TUYA_BLE_APP_FIRMWARE_KEY);

    if((p_buf[95]==8)||(p_buf[95]==16))
    {
        memcpy(&p_buf[96],TUYA_BLE_APP_FIRMWARE_KEY,p_buf[95]);
    }
    else
    {
        p_buf[95] = 0;
    }

	index = 96 + p_buf[95];
	
	p_buf[index++] = 0;  //zigbee mac length
	p_buf[index++] = 0;  //attach length
	p_buf[index++] = 2;   //packetMaxSize length
	p_buf[index++] = (512)>>8;
	p_buf[index++] = (512)&0xFF;
	
    payload_len = index;
	
#else
   
    p_buf[84] = strlen(TUYA_BLE_APP_FIRMWARE_KEY);

    if((p_buf[84]==8)||(p_buf[84]==16))
    {
        memcpy(&p_buf[85],TUYA_BLE_APP_FIRMWARE_KEY,p_buf[84]);
    }
    else
    {
        p_buf[84] = 0;
    }

    payload_len = 85 + p_buf[84];

#endif	

    encry_mode = recv_data[0];

    if(tuya_ble_commData_send(FRM_QRY_DEV_INFO_RESP,ack_sn,p_buf,payload_len,encry_mode)==2)
    {
        tuya_ble_pair_rand_clear();
    }
}

#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)

static void tuya_ble_handle_authenticate_phase1(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[7];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    uint8_t sig_data[64];
    uint16_t data_len = 0;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8) + recv_data[12];

    if(((tuya_ble_current_para->sys_settings.bound_flag==0)&&(recv_data[13]==0))||((tuya_ble_current_para->sys_settings.bound_flag==1)&&(recv_data[13]==1)))
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase1 -> invalid state.");
        tuya_ble_gap_disconnect();
        return;
    }

    if((data_len != 1 && data_len != 130)||(current_auth_status!=TUYA_BLE_AUTH_STATUS_PHASE_NONE))
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase1 -> invalid state or data length.");
        tuya_ble_gap_disconnect();
        return;
    }

    p_buf[0] = 0;

    tuya_ble_auth_data_reset();

    if(recv_data[13]==1)
    {
        if(data_len==130)
        {
            memcpy(current_ser_cert_pub_key,recv_data+15,64);
            memcpy(sig_data,recv_data+79,64);
            TUYA_BLE_LOG_HEXDUMP_DEBUG("received server public key:",current_ser_cert_pub_key,sizeof(current_ser_cert_pub_key));
            TUYA_BLE_LOG_HEXDUMP_DEBUG("received server public key signature data:",sig_data,sizeof(sig_data));
            if(!tuya_ble_server_cert_data_verify(recv_data+14,65,sig_data))
            {
                p_buf[0] = 1;
                TUYA_BLE_LOG_ERROR("server public key signature data verify failed.");
            }
            else
            {
                TUYA_BLE_LOG_DEBUG("server public key signature data verify succeed.");
            }
        }
        else
        {
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase1 -> invalid state.");
            tuya_ble_gap_disconnect();
            return;
        }
    }
    else
    {
        memcpy(current_ser_cert_pub_key,tuya_ble_current_para->sys_settings.server_cert_pub_key,64);
        TUYA_BLE_LOG_HEXDUMP_DEBUG("get server public key from tuya ble current para:",current_ser_cert_pub_key,sizeof(current_ser_cert_pub_key));
    }

    if(tuya_ble_current_para->sys_settings.bound_flag==1)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {
        encry_mode = ENCRYPTION_MODE_KEY_2;
    }

    if(p_buf[0]==0)
    {
        tuya_ble_rand_generator(current_dev_random_for_sign,6);
        TUYA_BLE_LOG_HEXDUMP_DEBUG("device generate random:",current_dev_random_for_sign,sizeof(current_dev_random_for_sign));
        memcpy(p_buf+1,current_dev_random_for_sign,6);
        current_auth_status = TUYA_BLE_AUTH_STATUS_PHASE_1;
        tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_1_RESP,ack_sn,p_buf,7,encry_mode);
    }
    else
    {
        tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_1_RESP,ack_sn,p_buf,1,encry_mode);
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase1 -> auth failed.");
        tuya_ble_gap_disconnect();
    }


}


static void tuya_ble_handle_authenticate_phase2(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    uint8_t sig_data[64] = {0};
    uint16_t data_len = 0;
    uint32_t dev_cert_data_len = 0;
	uint8_t *dev_cert_data = NULL;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8) + recv_data[12];

    if((data_len != 65)||(current_auth_status!=TUYA_BLE_AUTH_STATUS_PHASE_1))
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase2 -> invalid state or data length.");
        tuya_ble_gap_disconnect();
        return;
    }
    dev_cert_data = (uint8_t *)tuya_ble_malloc(sizeof(dev_cert_data));

    dev_cert_data[0] = 0;

    if(recv_data[13]==0)
    {
        memcpy(sig_data,recv_data+14,64);
        TUYA_BLE_LOG_HEXDUMP_DEBUG("received signature data for device random:",sig_data,sizeof(sig_data));
    }
    else
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase2 -> invalid data type.");
        tuya_ble_gap_disconnect();
		tuya_ble_free(dev_cert_data);
        return;
    }

    if(tuya_ble_current_para->sys_settings.bound_flag==1)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {
        encry_mode = ENCRYPTION_MODE_KEY_2;
    }
    TUYA_BLE_LOG_HEXDUMP_DEBUG("current device random:",current_dev_random_for_sign,sizeof(current_dev_random_for_sign));
    TUYA_BLE_LOG_HEXDUMP_DEBUG("verify signature data of device random use server public key:",current_ser_cert_pub_key,sizeof(current_ser_cert_pub_key));
    if(!tuya_ble_sig_data_verify(current_ser_cert_pub_key,current_dev_random_for_sign,6,sig_data))
    {
        dev_cert_data[0] = 1;
        tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_2_RESP,ack_sn,dev_cert_data,1,encry_mode);
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase2 -> device random signature data verify failed.");
        tuya_ble_gap_disconnect();
		tuya_ble_free(dev_cert_data);
        return;

    }
    else
    {
        TUYA_BLE_LOG_DEBUG("tuya_ble_handle_authenticate_phase2 -> device random signature data verify succeed.");
    }

    if(tuya_ble_current_para->sys_settings.bound_flag==0)
    {
        dev_cert_data_len = tuya_ble_get_dev_crt_len();

        if(dev_cert_data_len==0)
        {
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase2 -> get dev cert length error.");
            tuya_ble_gap_disconnect();
			tuya_ble_free(dev_cert_data);
            return;
        }
        else
        {
            TUYA_BLE_LOG_DEBUG("tuya_ble_handle_authenticate_phase2 -> get dev cert length = %d .",dev_cert_data_len);
            dev_cert_data[0] = 0;
            dev_cert_data[1] = 0;
            if(tuya_ble_get_dev_crt_der(dev_cert_data+2,dev_cert_data_len)!=TUYA_BLE_SUCCESS)
            {
                TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase2 -> get dev cert data error.");
                tuya_ble_gap_disconnect();
				tuya_ble_free(dev_cert_data);
                return;
            }
            TUYA_BLE_LOG_HEXDUMP_DEBUG("get device cert :",dev_cert_data+2,dev_cert_data_len);
            tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_2_RESP,ack_sn,dev_cert_data,dev_cert_data_len+2,encry_mode);
        }
    }
    else
    {
        dev_cert_data[0] = 0;
        tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_2_RESP,ack_sn,dev_cert_data,1,encry_mode);
        TUYA_BLE_LOG_DEBUG("tuya_ble_handle_authenticate_phase2 -> no need send dev cert because in bound state.");
    }
	
	tuya_ble_free(dev_cert_data);
    current_auth_status = TUYA_BLE_AUTH_STATUS_PHASE_2;
}


static void tuya_ble_handle_authenticate_phase3(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    uint8_t p_buf[65] = {0};
    uint16_t data_len = 0;
    uint8_t random[6] = {0};
    uint8_t random_hash[32] = {0};

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8) + recv_data[12];

    if((data_len != 7 && data_len != 1)||(current_auth_status!=TUYA_BLE_AUTH_STATUS_PHASE_2))
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase3 -> invalid state or data length.");
        tuya_ble_gap_disconnect();
        return;
    }

    if(recv_data[13]!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase3 -> dev cert verify failed.");
        tuya_ble_gap_disconnect();
        return;
    }
    else
    {
        TUYA_BLE_LOG_HEXDUMP_DEBUG("received server random for signature :",random,6);
        memcpy(random,&recv_data[14],6);
    }

    if(tuya_ble_ecc_sign_secp256r1(NULL,random,6,p_buf+1)!=TUYA_BLE_SUCCESS)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_authenticate_phase3 -> server random sign failed.");
        tuya_ble_gap_disconnect();
        return;
    }
    else
    {
        TUYA_BLE_LOG_HEXDUMP_DEBUG("tuya_ble_handle_authenticate_phase3 -> server random sign succeed.",p_buf+1,64);
    }

    p_buf[0] = 0;

    if(tuya_ble_current_para->sys_settings.bound_flag==1)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {
        encry_mode = ENCRYPTION_MODE_KEY_2;
    }

    tuya_ble_commData_send(FRM_AUTHENTICATE_PHASE_3_RESP,ack_sn,p_buf,65,encry_mode);

    current_auth_status = TUYA_BLE_AUTH_STATUS_PHASE_3;

}

#endif

extern void tuya_ble_connect_monitor_timer_stop(void);
static void tuya_ble_handle_pair_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    uint16_t data_len = 0;
    tuya_ble_cb_evt_param_t event;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8) + recv_data[12];

#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)

#if (TUYA_BLE_ADVANCED_ENCRYPTION_AUTH_ON_CONNECT)

    if(current_auth_status != TUYA_BLE_AUTH_STATUS_PHASE_3)
    {
        TUYA_BLE_LOG_ERROR("PAIR_REQ failed ,because current_auth_status error!");  //ID not match ,and disconnected.
        tuya_ble_gap_disconnect();
        return;
    }

#else

    if(tuya_ble_current_para->sys_settings.bound_flag==0)
    {
        if(current_auth_status != TUYA_BLE_AUTH_STATUS_PHASE_3)
        {
            TUYA_BLE_LOG_ERROR("PAIR_REQ failed ,because current_auth_status error!");  //ID not match ,and disconnected.
            tuya_ble_gap_disconnect();
            return;
        }
    }

#endif

#endif


    if(0 == memcmp(&recv_data[13],tuya_ble_current_para->auth_settings.device_id,DEVICE_ID_LEN))
    {
        tuya_ble_connect_monitor_timer_stop();

        if(1==tuya_ble_get_adv_connect_request_bit_status())
        {
            TUYA_BLE_LOG_INFO("ble adv data update,because the last broadcast data connection request flag was set!");
            tuya_ble_adv_change();
        }

        if(tuya_ble_current_para->sys_settings.bound_flag==1)
        {
            TUYA_BLE_LOG_INFO("PAIR_REQ already bound!");
            p_buf[0] = 2;
            tuya_ble_connect_status_set(BONDING_CONN);
        }
        else
        {
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

#if (TUYA_BLE_PROTOCOL_VERSION_LOW>=1)

            if(TUYA_BLE_BEACON_KEY_ENABLE)
            {
                if(data_len<61)
                {
                    p_buf[0]=1;
                    goto pair_exit;
                }
                else
                {
                    if(recv_data[57]!=BEACON_KEY_LEN)
                    {
                        p_buf[0]=1;
                        goto pair_exit;
                    }
                    else
                    {
                        memcpy(tuya_ble_current_para->sys_settings.beacon_key,recv_data+58,BEACON_KEY_LEN);
                    }
                }

            }

#endif

            memcpy(tuya_ble_current_para->sys_settings.login_key,recv_data+29,LOGIN_KEY_LEN);
            memcpy(tuya_ble_current_para->sys_settings.device_virtual_id,recv_data+29+LOGIN_KEY_LEN,DEVICE_VIRTUAL_ID_LEN);
            tuya_ble_current_para->sys_settings.bound_flag = 1;

#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)

            if(!tuya_ble_buffer_value_is_all_x(current_ser_cert_pub_key,64,0))
            {
                memcpy(tuya_ble_current_para->sys_settings.server_cert_pub_key,current_ser_cert_pub_key,64);
            }
            else
            {
                p_buf[0]=1;
                tuya_ble_current_para->sys_settings.bound_flag = 0;
                goto pair_exit;
            }

#endif

            tuya_ble_storage_save_sys_settings();

            tuya_ble_connect_status_set(BONDING_CONN);

            tuya_ble_adv_change();
            TUYA_BLE_LOG_INFO("PAIR_REQ ok-%d",tuya_ble_current_para->sys_settings.bound_flag);

            event.evt = TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID;
            event.device_login_key_vid_data.login_key_len = LOGIN_KEY_LEN;
            event.device_login_key_vid_data.vid_len = DEVICE_VIRTUAL_ID_LEN;
            
            #if (TUYA_BLE_PROTOCOL_VERSION_LOW>=1)
            if(TUYA_BLE_BEACON_KEY_ENABLE)
            {
                event.device_login_key_vid_data.beacon_key_len = BEACON_KEY_LEN;
                memcpy( event.device_login_key_vid_data.beacon_key,tuya_ble_current_para->sys_settings.beacon_key,BEACON_KEY_LEN);
            }
            #endif
            
            memcpy( event.device_login_key_vid_data.login_key,tuya_ble_current_para->sys_settings.login_key,LOGIN_KEY_LEN);
            memcpy( event.device_login_key_vid_data.vid,tuya_ble_current_para->sys_settings.device_virtual_id,DEVICE_VIRTUAL_ID_LEN);
            
            if(tuya_ble_cb_event_send(&event)!=0)
            {
                TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
            }
            else
            {
                TUYA_BLE_LOG_DEBUG("tuya ble send cb event succeed.");
            }


#else
            tuya_ble_connect_status_set(UNBONDING_CONN);
#endif
            p_buf[0] = 0x00;
        }

        event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
        event.connect_status = tuya_ble_connect_status_get();
        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
        }
        else
        {
            TUYA_BLE_LOG_INFO("tuya ble send cb event succeed.");
        }


    }
    else
    {
        TUYA_BLE_LOG_ERROR("PAIR_REQ device id not match!");  //ID not match ,and disconnected.
        p_buf[0] = 0x01;

    }


pair_exit:
    if(p_buf[0]==0)
    {
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
#else
        encry_mode = recv_data[0];
#endif
    }
    else if(p_buf[0]==1)
    {
        encry_mode = recv_data[0];
    }
    else if(p_buf[0]==2)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {

    }

    tuya_ble_commData_send(PAIR_RESP,ack_sn,p_buf,1,encry_mode);

    if(encry_mode == ENCRYPTION_MODE_SESSION_KEY)
    {
		if(TUYA_BLE_AUTO_REQUEST_TIME_CONFIGURE==1)
		{
			tuya_ble_commData_send(FRM_GET_UNIX_TIME_CHAR_MS_REQ,0,NULL,0,encry_mode);
			TUYA_BLE_LOG_INFO("send FRM_GET_UNIX_TIME_CHAR_MS_REQ cmd to app.\n");
		}
		else if(TUYA_BLE_AUTO_REQUEST_TIME_CONFIGURE==2)
		{
			tuya_ble_commData_send(FRM_GET_APP_LOCAL_TIME_REQ,0,NULL,0,encry_mode);
			TUYA_BLE_LOG_INFO("send FRM_GET_APP_LOCAL_TIME_REQ cmd to app.\n");
		}
		else
		{
			
		}
    }

    if(p_buf[0]==1)
    {
        tuya_ble_gap_disconnect();
    }

}



#endif  //TUYA_BLE_PROTOCOL_VERSION_HIGN==4


#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==3)

#if (TUYA_BLE_PROTOCOL_VERSION_LOW>=2)

static void tuya_ble_handle_dev_info_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[90];
    uint8_t payload_len = 0;
    uint32_t ack_sn = 0;
    uint8_t encry_mode = 0;
    uint32_t version_temp_s,version_temp_h;
    
    memset(p_buf,0,sizeof(p_buf));

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    TUYA_BLE_LOG_DEBUG("get device infor-%d",tuya_ble_current_para->sys_settings.bound_flag);
    send_packet_data_len = 20;

    tuya_ble_rand_generator(tuya_ble_pair_rand,6);
    tuya_ble_pair_rand_valid = 1;

    if(TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
    {
        version_temp_s = tuya_ble_firmware_version;
        version_temp_h = tuya_ble_hardware_version;
        p_buf[4] = 0x00;
    }
    else
    {
        version_temp_s = tuya_ble_firmware_version>>8;
        version_temp_h = tuya_ble_hardware_version>>8;
        p_buf[4] = 0x05;
    }
    p_buf[0] = (version_temp_s>>8)&0xff;
    p_buf[1] = (version_temp_s&0xff);
    p_buf[2] = TUYA_BLE_PROTOCOL_VERSION_HIGN;
    p_buf[3] = TUYA_BLE_PROTOCOL_VERSION_LOW;
    if(TUYA_BLE_ADVANCED_ENCRYPTION_DEVICE==1)
    {
        p_buf[4] |= 0x02;
    }

    p_buf[5] = tuya_ble_current_para->sys_settings.bound_flag;
    memcpy(&p_buf[6],tuya_ble_pair_rand,6);
    p_buf[12] = (version_temp_h>>8)&0xff;
    p_buf[13] = (version_temp_h&0xff);
    
    tuya_ble_register_key_generate(&p_buf[14],tuya_ble_current_para);

    p_buf[46] = (tuya_ble_firmware_version>>16)&0xff;
    p_buf[47] = (tuya_ble_firmware_version>>8)&0xff;
    p_buf[48] = (tuya_ble_firmware_version&0xff);
    p_buf[49] = (tuya_ble_hardware_version>>16)&0xff;
    p_buf[50] = (tuya_ble_hardware_version>>8)&0xff;
    p_buf[51] = (tuya_ble_hardware_version&0xff);

    p_buf[52] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY>>8;
    p_buf[53] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY; //communication ability

    p_buf[54] = 0x00;

    memcpy(&p_buf[55],tuya_ble_current_para->sys_settings.device_virtual_id,DEVICE_VIRTUAL_ID_LEN);

    p_buf[77] = (tuya_ble_mcu_firmware_version>>16)&0xff;
    p_buf[78] = (tuya_ble_mcu_firmware_version>>8)&0xff;
    p_buf[79] = (tuya_ble_mcu_firmware_version&0xff);
    p_buf[80] = (tuya_ble_mcu_hardware_version>>16)&0xff;
    p_buf[81] = (tuya_ble_mcu_hardware_version>>8)&0xff;
    p_buf[82] = (tuya_ble_mcu_hardware_version&0xff);
    
    p_buf[83] = TUYA_BLE_WIFI_DEVICE_REGISTER_MODE;
    
    payload_len = 84;

    encry_mode = recv_data[0];

    if(tuya_ble_commData_send(FRM_QRY_DEV_INFO_RESP,ack_sn,p_buf,payload_len,encry_mode)==2)
    {
        tuya_ble_pair_rand_clear();
    }
}

#else

static void tuya_ble_handle_dev_info_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[90];
    uint8_t payload_len = 0;
    uint32_t ack_sn = 0;
    uint8_t encry_mode = 0;
    uint32_t version_temp_s,version_temp_h;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    TUYA_BLE_LOG_DEBUG("get device infor-%d",tuya_ble_current_para->sys_settings.bound_flag);
    send_packet_data_len = 20;

    tuya_ble_rand_generator(tuya_ble_pair_rand,6);
    tuya_ble_pair_rand_valid = 1;

    if(TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
    {
        version_temp_s = tuya_ble_firmware_version;
        version_temp_h = tuya_ble_hardware_version;
        p_buf[4] = 0x00;
    }
    else
    {
        version_temp_s = tuya_ble_firmware_version>>8;
        version_temp_h = tuya_ble_hardware_version>>8;
        p_buf[4] = 0x05;
    }
    p_buf[0] = (version_temp_s>>8)&0xff;
    p_buf[1] = (version_temp_s&0xff);
    p_buf[2] = TUYA_BLE_PROTOCOL_VERSION_HIGN;
    p_buf[3] = TUYA_BLE_PROTOCOL_VERSION_LOW;
    if(TUYA_BLE_ADVANCED_ENCRYPTION_DEVICE==1)
    {
        p_buf[4] |= 0x02;
    }

    p_buf[5] = tuya_ble_current_para->sys_settings.bound_flag;
    memcpy(&p_buf[6],tuya_ble_pair_rand,6);
    p_buf[12] = (version_temp_h>>8)&0xff;
    p_buf[13] = (version_temp_h&0xff);
    memcpy(&p_buf[14],tuya_ble_current_para->auth_settings.auth_key,AUTH_KEY_LEN);

    p_buf[46] = (tuya_ble_firmware_version>>16)&0xff;
    p_buf[47] = (tuya_ble_firmware_version>>8)&0xff;
    p_buf[48] = (tuya_ble_firmware_version&0xff);
    p_buf[49] = (tuya_ble_hardware_version>>16)&0xff;
    p_buf[50] = (tuya_ble_hardware_version>>8)&0xff;
    p_buf[51] = (tuya_ble_hardware_version&0xff);

    p_buf[52] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY>>8;
    p_buf[53] = TUYA_BLE_DEVICE_COMMUNICATION_ABILITY; //communication ability

    p_buf[54] = 0x00;

    memcpy(&p_buf[55],tuya_ble_current_para->sys_settings.device_virtual_id,DEVICE_VIRTUAL_ID_LEN);

    p_buf[77] = (tuya_ble_mcu_firmware_version>>16)&0xff;
    p_buf[78] = (tuya_ble_mcu_firmware_version>>8)&0xff;
    p_buf[79] = (tuya_ble_mcu_firmware_version&0xff);
    p_buf[80] = (tuya_ble_mcu_hardware_version>>16)&0xff;
    p_buf[81] = (tuya_ble_mcu_hardware_version>>8)&0xff;
    p_buf[82] = (tuya_ble_mcu_hardware_version&0xff);

    payload_len = 83;

    encry_mode = recv_data[0];

    if(tuya_ble_commData_send(FRM_QRY_DEV_INFO_RESP,ack_sn,p_buf,payload_len,encry_mode)==2)
    {
        tuya_ble_pair_rand_clear();
    }
}

#endif

extern void tuya_ble_connect_monitor_timer_stop(void);
static void tuya_ble_handle_pair_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    print_hex("tuya_ble_handle_pair_req:", &recv_data[13], DEVICE_ID_LEN); 
    print_hex("tuya_ble_handle_pair_req:", tuya_ble_current_para->auth_settings.device_id, DEVICE_ID_LEN); 

    if(0 == memcmp(&recv_data[13],tuya_ble_current_para->auth_settings.device_id,DEVICE_ID_LEN))
    {
        tuya_ble_connect_monitor_timer_stop();
        
        if(1==tuya_ble_get_adv_connect_request_bit_status())
        {
            TUYA_BLE_LOG_INFO("ble adv data update,because the last broadcast data connection request flag was set!");
            tuya_ble_adv_change();
        }

        if(tuya_ble_current_para->sys_settings.bound_flag==1)
        {
            TUYA_BLE_LOG_INFO("PAIR_REQ already bound!");
            p_buf[0] = 2;
            tuya_ble_connect_status_set(BONDING_CONN);
        }
        else
        {
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
            memcpy(tuya_ble_current_para->sys_settings.login_key,recv_data+29,LOGIN_KEY_LEN);
            memcpy(tuya_ble_current_para->sys_settings.device_virtual_id,recv_data+29+LOGIN_KEY_LEN,DEVICE_VIRTUAL_ID_LEN);
            tuya_ble_current_para->sys_settings.bound_flag = 1;

            tuya_ble_storage_save_sys_settings();
            
            tuya_ble_connect_status_set(BONDING_CONN);

            tuya_ble_adv_change();
            TUYA_BLE_LOG_INFO("PAIR_REQ ok-%d",tuya_ble_current_para->sys_settings.bound_flag);

            event.evt = TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID;
            event.device_login_key_vid_data.login_key_len = LOGIN_KEY_LEN;
            event.device_login_key_vid_data.vid_len = DEVICE_VIRTUAL_ID_LEN;
            memcpy( event.device_login_key_vid_data.login_key,tuya_ble_current_para->sys_settings.login_key,LOGIN_KEY_LEN);
            memcpy( event.device_login_key_vid_data.vid,tuya_ble_current_para->sys_settings.device_virtual_id,DEVICE_VIRTUAL_ID_LEN);
            if(tuya_ble_cb_event_send(&event)!=0)
            {
                TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
            }
            else
            {
                TUYA_BLE_LOG_DEBUG("tuya ble send cb event succeed.");
            }


#else
            tuya_ble_connect_status_set(UNBONDING_CONN);
#endif
            p_buf[0] = 0x00;
        }

        
        event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
        event.connect_status = tuya_ble_connect_status_get();
        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
        }
        else
        {
            TUYA_BLE_LOG_INFO("tuya ble send cb event succeed.");
        }


    }
    else
    {
        TUYA_BLE_LOG_ERROR("PAIR_REQ device id not match!");  //ID not match ,and disconnected.
        p_buf[0] = 0x01;

    }


    if(p_buf[0]==0)
    {
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
#else
        encry_mode = recv_data[0];
#endif
    }
    else if(p_buf[0]==1)
    {
        encry_mode = recv_data[0];
    }
    else if(p_buf[0]==2)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {

    }
	
	printf("%s pbuf: %d\n", __func__, p_buf[0]);

    tuya_ble_commData_send(PAIR_RESP,ack_sn,p_buf,1,encry_mode);

    if(encry_mode == ENCRYPTION_MODE_SESSION_KEY)
    {
        tuya_ble_commData_send(FRM_GET_UNIX_TIME_CHAR_MS_REQ,0,NULL,0,encry_mode);
        TUYA_BLE_LOG_INFO("send FRM_GET_UNIX_TIME_CHAR_MS_REQ cmd to app.\n");
    }

    if(p_buf[0]==1)
    {
        tuya_ble_gap_disconnect();
    }

}

#endif

#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==2)

static void tuya_ble_handle_dev_info_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[70];
    uint8_t payload_len = 0;
    uint32_t ack_sn = 0;
    uint8_t encry_mode = 0;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    TUYA_BLE_LOG_INFO("get device infor-%d",tuya_ble_current_para->sys_settings.bound_flag);

    tuya_ble_rand_generator(tuya_ble_pair_rand,6);
    tuya_ble_pair_rand_valid = 1;


    p_buf[0] = (tuya_ble_firmware_version>>8)&0xff;
    p_buf[1] = (tuya_ble_firmware_version&0xff);
    p_buf[2] = TUYA_BLE_PROTOCOL_VERSION_HIGN;
    p_buf[3] = TUYA_BLE_PROTOCOL_VERSION_LOW;
    if(TUYA_BLE_ADVANCED_ENCRYPTION_DEVICE==1)
    {
        p_buf[4] = 0x02;
    }
    else
    {
        p_buf[4] = 0x00;
    }
    p_buf[5] = tuya_ble_current_para->sys_settings.bound_flag;
    memcpy(&p_buf[6],tuya_ble_pair_rand,6);
    p_buf[12] = (tuya_ble_hardware_version>>8)&0xff;
    p_buf[13] = (tuya_ble_hardware_version&0xff);
    memcpy(&p_buf[14],tuya_ble_current_para->auth_settings.auth_key,AUTH_KEY_LEN);

    payload_len = 46;

    encry_mode = recv_data[0];

    if(tuya_ble_commData_send(FRM_QRY_DEV_INFO_RESP,ack_sn,p_buf,payload_len,encry_mode)==2)
    {
        tuya_ble_pair_rand_clear();
    }
    
}

extern void tuya_ble_connect_monitor_timer_stop(void);
static void tuya_ble_handle_pair_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    if(0 == memcmp(&recv_data[13],tuya_ble_current_para->auth_settings.device_id,DEVICE_ID_LEN))
    {
        tuya_ble_connect_monitor_timer_stop();
        
        if(tuya_ble_current_para->sys_settings.bound_flag==1)
        {
            TUYA_BLE_LOG_ERROR("PAIR_REQ already bound!");
            p_buf[0] = 2;
        }
        else
        {

#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

            memcpy(tuya_ble_current_para->sys_settings.login_key,recv_data+29,LOGIN_KEY_LEN);
            tuya_ble_current_para->sys_settings.bound_flag = 1;
            tuya_ble_storage_save_sys_settings();
            tuya_ble_adv_change();
            TUYA_BLE_LOG_INFO("PAIR_REQ ok-%d",tuya_ble_current_para->sys_settings.bound_flag);
            
            event.evt = TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID;
            event.device_login_key_vid_data.login_key_len = LOGIN_KEY_LEN;
            event.device_login_key_vid_data.vid_len = 0;
            memcpy( event.device_login_key_vid_data.login_key,tuya_ble_current_para->sys_settings.login_key,LOGIN_KEY_LEN);

            if(tuya_ble_cb_event_send(&event)!=0)
            {
                TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
            }
            else
            {
                TUYA_BLE_LOG_DEBUG("tuya ble send cb event succeed.");
            }
            
#else
            tuya_ble_connect_status_set(UNBONDING_CONN);
#endif
            p_buf[0] = 0x00;

        }

        if(tuya_ble_current_para->sys_settings.bound_flag==1)
        {
            tuya_ble_connect_status_set(BONDING_CONN);
        }

        event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
        event.connect_status = tuya_ble_connect_status_get();
        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
        }
        else
        {
            TUYA_BLE_LOG_INFO("tuya ble send cb event succeed.");
        }


    }
    else
    {
        TUYA_BLE_LOG_ERROR("PAIR_REQ device id not match!");  //ID not match ,and disconnected.
        p_buf[0] = 0x01;

    }


    if(p_buf[0]==0)
    {
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
#else
        encry_mode = recv_data[0];
#endif
    }
    else if(p_buf[0]==1)
    {
        encry_mode = recv_data[0];
    }
    else if(p_buf[0]==2)
    {
        encry_mode = ENCRYPTION_MODE_SESSION_KEY;
    }
    else
    {

    }

    tuya_ble_commData_send(PAIR_RESP,ack_sn,p_buf,1,encry_mode);

    if(encry_mode == ENCRYPTION_MODE_SESSION_KEY)
    {
        tuya_ble_commData_send(FRM_GET_UNIX_TIME_CHAR_MS_REQ,0,NULL,0,encry_mode);
        TUYA_BLE_LOG_INFO("send FRM_GET_UNIX_TIME_CHAR_MS_REQ cmd to app.");
    }

    if(p_buf[0]==1)
    {
        tuya_ble_gap_disconnect();
    }

}

#endif

static void tuya_ble_handle_net_config_info_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len;


    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];
    data_len = (recv_data[11]<<8) + recv_data[12];
    encry_mode = recv_data[0];

    event.evt = TUYA_BLE_CB_EVT_NETWORK_INFO;
    uint8_t *ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(data_len);
    if(ble_cb_evt_buffer==NULL)
    {
        p_buf[0]=1;
        TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
        tuya_ble_commData_send(FRM_NET_CONFIG_INFO_RESP,ack_sn,p_buf,1,encry_mode);
        return;
    }
    else
    {
        memset(ble_cb_evt_buffer,0,data_len);
        memcpy(ble_cb_evt_buffer,&recv_data[13],data_len);
    }
    event.network_data.data_len = data_len;
    event.network_data.p_data = ble_cb_evt_buffer;

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        tuya_ble_free(ble_cb_evt_buffer);
        TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
        p_buf[0]=1;
        tuya_ble_commData_send(FRM_NET_CONFIG_INFO_RESP,ack_sn,p_buf,1,encry_mode);
        return;
    }
    else
    {
        p_buf[0]=0;
    }

    tuya_ble_commData_send(FRM_NET_CONFIG_INFO_RESP,ack_sn,p_buf,1,encry_mode);
	//tuya_ble_free(ble_cb_evt_buffer);


}


static void tuya_ble_handle_ble_passthrough_data_req(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len;

    data_len = (recv_data[11]<<8) + recv_data[12];

    event.evt = TUYA_BLE_CB_EVT_DATA_PASSTHROUGH;

    uint8_t *ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(data_len);
    if(ble_cb_evt_buffer==NULL)
    {
        TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
        return;
    }
    else
    {
        memcpy(ble_cb_evt_buffer,&recv_data[13],data_len);
    }
    event.ble_passthrough_data.data_len = data_len;
    event.ble_passthrough_data.p_data = ble_cb_evt_buffer;

    if(tuya_ble_cb_event_send(&event)!=0)
    {   
		tuya_ble_free(ble_cb_evt_buffer);
        TUYA_BLE_LOG_ERROR("tuya ble send cb event failed.");
    }
    else
    {

    }
	//tuya_ble_free(ble_cb_evt_buffer);

}

#ifdef TUYA_BLE_APP_PRODUCT
// #include "tuya_ble_app_production_test.h"
static void tuya_ble_handle_ble_factory_test_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint16_t data_len;
    uint32_t ack_sn;
    uint8_t sum;
    uint8_t encry_mode;

    data_len = (recv_data[11]<<8) + recv_data[12];

    if(data_len<7)
    {
        return;
    }

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    encry_mode = recv_data[0];

    if((recv_data[13]==0x66)&&(recv_data[14]==0xAA))
    {
        sum = tuya_ble_check_sum(&recv_data[13],data_len-1);
        if(sum==recv_data[13+data_len-1])
        {
            tuya_ble_app_production_test_process(1,&recv_data[13],data_len);
        }
    }

}
#endif

#if 1//def TUYA_BLE_APP_UART_COMMON   // zdx

 // #include "tuya_ble_app_uart_common_handler.h"
static void tuya_ble_handle_ota_req(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len;
    tuya_ble_ota_data_type_t cmd_type;

    data_len = (recv_data[11]<<8) + recv_data[12];

    if(data_len==0)
    {
        return;
    }

    if(recv_data[13]==1)  //extern mcu ota
    {
        //tuya_ble_uart_common_mcu_ota_data_from_ble_handler(cmd,&recv_data[14],data_len-1);
    }
    else if(recv_data[13]==0)
    {
        event.evt = TUYA_BLE_CB_EVT_OTA_DATA;

        uint8_t *ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(data_len);
        if(ble_cb_evt_buffer==NULL)
        {
            TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
            return;
        }
        else
        {
            memcpy(ble_cb_evt_buffer,&recv_data[13],data_len);
        }

        switch (cmd)
        {
        case FRM_OTA_START_REQ:
            cmd_type = TUYA_BLE_OTA_REQ;
            break;
        case FRM_OTA_FILE_INFOR_REQ:
            cmd_type = TUYA_BLE_OTA_FILE_INFO;
            break;
        case FRM_OTA_FILE_OFFSET_REQ:
            cmd_type = TUYA_BLE_OTA_FILE_OFFSET_REQ;
            break;
        case FRM_OTA_DATA_REQ:
            cmd_type = TUYA_BLE_OTA_DATA;
            break;
        case FRM_OTA_END_REQ:
            cmd_type = TUYA_BLE_OTA_END;
            break;
        default:
            cmd_type = TUYA_BLE_OTA_UNKONWN;
            break;
        }

        event.ota_data.type = cmd_type;
        event.ota_data.data_len = data_len;
        event.ota_data.p_data = ble_cb_evt_buffer;

        if(tuya_ble_cb_event_send(&event)!=0)
        {
			tuya_ble_free(ble_cb_evt_buffer);
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_ota_req-tuya ble send cb event failed.");
        }
        else
        {

        }
		//if (ble_cb_evt_buffer)
		//	tuya_ble_free(ble_cb_evt_buffer);
    }
    else
    {
        
    }
}
#endif

static char current_timems_string[14] = "000000000000";

static void tuya_ble_handle_unix_time_char_ms_resp(uint8_t*recv_data,uint16_t recv_len)
{
    int16_t zone_temp = 0;
    uint64_t time_stamp_ms;
    uint32_t time_stamp;
    tuya_ble_cb_evt_param_t event;

	// add by zdx
    char current_timems_string_first_part[11] = {0};
    char current_timems_string_second_part[4] = {0};
    uint32_t time_stamp_ms_first_part = 0;
    uint32_t time_stamp_ms_second_part = 0;

	memset(&event,0,sizeof(tuya_ble_cb_evt_param_t));

	if(recv_len<30)
    {
        TUYA_BLE_LOG_ERROR("received unix time char cmd data length error!");
        return;
    }

    if(!buffer_value_is_all_x(&recv_data[13],13,0))
    {
		int i;
        memcpy(current_timems_string,&recv_data[13],13);
        zone_temp  = (int16_t)((recv_data[26]<<8)|recv_data[27]);

#if 0
        time_stamp_ms = atoll(current_timems_string); 
        TUYA_BLE_LOG_INFO("received unix time_zone = %d\n",zone_temp);
        time_stamp = time_stamp_ms/1000;
        if(time_stamp_ms%1000>=500)
        {
            time_stamp += 1;
        }
#endif		
		// add by zdx
		strncpy(current_timems_string_first_part, current_timems_string, 10);
		strncpy(current_timems_string_second_part, &current_timems_string[10], 4);
        TUYA_BLE_LOG_INFO("received unix time_zone = %d %s\n",zone_temp, current_timems_string);
		printf("current_timems_string_first_part=%s\n", current_timems_string_first_part);
		printf("current_timems_string_second_part=%s\n", current_timems_string_second_part);

		time_stamp_ms_first_part = strtoul(current_timems_string_first_part, NULL, 10);
		time_stamp_ms_second_part = strtoul(current_timems_string_second_part, NULL, 10);
		printf("time_stamp_ms_first_part=%u\n", time_stamp_ms_first_part);
		printf("time_stamp_ms_second_part=%u\n", time_stamp_ms_second_part);
		
		time_stamp_ms = time_stamp_ms_first_part;
		for (i = 0; i < strlen(current_timems_string_second_part); i++) 
			time_stamp_ms *= 10;
		time_stamp_ms += time_stamp_ms_second_part;
		printf("time_stamp_ms=%llu\n", time_stamp_ms);
	
		time_stamp = time_stamp_ms_first_part;
        if(time_stamp_ms_second_part >= 500)
        {
            time_stamp += 1;
        }
      
        tuya_ble_rtc_set_timestamp(time_stamp,zone_temp);

        event.evt = TUYA_BLE_CB_EVT_TIME_STAMP;

        memcpy(event.timestamp_data.timestamp_string,current_timems_string,13);
        event.timestamp_data.time_zone = zone_temp;
        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_unix_time_char_ms_resp-tuya ble send cb event failed.");
        }

    }

}


static void tuya_ble_handle_unix_time_date_resp(uint8_t*recv_data,uint16_t recv_len)
{
    int16_t zone_temp = 0;
    uint32_t time_stamp;
    tuya_ble_cb_evt_param_t event;
    tuya_ble_time_struct_data_t time_temp;

    if(recv_len<24)
    {
        TUYA_BLE_LOG_ERROR("received unix time date cmd data length error!");
        return;
    }

    memset(&event,0,sizeof(tuya_ble_cb_evt_param_t));
    
    if(!buffer_value_is_all_x(&recv_data[13],7,0))
    {
        time_temp.nYear = 2000+recv_data[13];
        time_temp.nMonth = recv_data[14];
        time_temp.nDay = recv_data[15];
        time_temp.nHour = recv_data[16];
        time_temp.nMin = recv_data[17];
        time_temp.nSec = recv_data[18];
        time_temp.DayIndex = recv_data[19];

        time_stamp = tuya_ble_mytime_2_utc_sec(&time_temp,false);

        zone_temp  = (int16_t)((recv_data[20]<<8)|recv_data[21]);

        TUYA_BLE_LOG_INFO("received unix time_zone = %d",zone_temp);

        tuya_ble_rtc_set_timestamp(time_stamp,zone_temp);

        event.evt = TUYA_BLE_CB_EVT_TIME_NORMAL;
        event.time_normal_data.nYear = recv_data[13];
        event.time_normal_data.nMonth = recv_data[14];
        event.time_normal_data.nDay = recv_data[15];
        event.time_normal_data.nHour = recv_data[16];
        event.time_normal_data.nMin = recv_data[17];
        event.time_normal_data.nSec = recv_data[18];
        event.time_normal_data.DayIndex = recv_data[19];
        event.time_normal_data.time_zone = zone_temp;

        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_unix_time_date_resp-tuya ble send cb event failed.");
        }

    }

}

static void tuya_ble_handle_app_local_time_data_resp(uint8_t*recv_data,uint16_t recv_len)
{
    int16_t zone_temp = 0;
    uint32_t local_time, time_stamp;
    tuya_ble_cb_evt_param_t event;
    tuya_ble_time_struct_data_t time_temp;

    if(recv_len<24)
    {
        TUYA_BLE_LOG_ERROR("received app local time date cmd data length error!");
        return;
    }

    memset(&event,0,sizeof(tuya_ble_cb_evt_param_t));
    
    if(!buffer_value_is_all_x(&recv_data[13],7,0))
    {
        time_temp.nYear = 2000+recv_data[13];
        time_temp.nMonth = recv_data[14];
        time_temp.nDay = recv_data[15];
        time_temp.nHour = recv_data[16];
        time_temp.nMin = recv_data[17];
        time_temp.nSec = recv_data[18];
        time_temp.DayIndex = recv_data[19];
		
        local_time = tuya_ble_mytime_2_utc_sec(&time_temp,false);
		
        zone_temp  = (int16_t)((recv_data[20]<<8)|recv_data[21]);
		
        TUYA_BLE_LOG_INFO("received app local time_zone = %d",zone_temp);
		
		time_stamp = (local_time - zone_temp*36); //!< Convert to unix timestamp
        tuya_ble_rtc_set_timestamp(time_stamp,zone_temp); 

        event.evt = TUYA_BLE_CB_EVT_APP_LOCAL_TIME_NORMAL;
        event.time_normal_data.nYear = recv_data[13];
        event.time_normal_data.nMonth = recv_data[14];
        event.time_normal_data.nDay = recv_data[15];
        event.time_normal_data.nHour = recv_data[16];
        event.time_normal_data.nMin = recv_data[17];
        event.time_normal_data.nSec = recv_data[18];
        event.time_normal_data.DayIndex = recv_data[19];
        event.time_normal_data.time_zone = zone_temp;

        if(tuya_ble_cb_event_send(&event)!=0)
        {
            TUYA_BLE_LOG_ERROR("tuya_ble_handle_app_local_time_date_resp-tuya ble send cb event failed.");
        }

    }
}


static void tuya_ble_handle_unix_time_with_dst_date_resp(uint8_t*recv_data,uint16_t recv_len) 
{
	int16_t zone_temp = 0;
    uint32_t time_stamp;
	uint8_t years_of_dst;
	uint16_t dst_content_len;
    tuya_ble_cb_evt_param_t event;
	char current_time_string[11] = "0000000000";
	uint8_t *ble_cb_evt_buffer = NULL;
	
    if(recv_len<28)
    {
        TUYA_BLE_LOG_ERROR("received extend unix time date cmd data length error!");
        return;
    }

    memset(&event,0,sizeof(tuya_ble_cb_evt_param_t));
    
    if(!buffer_value_is_all_x(&recv_data[13],10,0))
    {
        memcpy(current_time_string,&recv_data[13],10);
        time_stamp = my_atoi(current_time_string);
		
		zone_temp  = (int16_t)((recv_data[23]<<8)|recv_data[24]);
        TUYA_BLE_LOG_INFO("received unix time_zone = %d\n",zone_temp);
		
		tuya_ble_rtc_set_timestamp(time_stamp,zone_temp);
		
		/* daylight saving time */
		years_of_dst = recv_data[25];
		dst_content_len = (years_of_dst * 20);
		TUYA_BLE_LOG_INFO("received dst years= %d ",years_of_dst);
		TUYA_BLE_LOG_HEXDUMP_INFO("dst data ", &recv_data[26], dst_content_len);
		
		if(years_of_dst > 0)
		{
			ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(dst_content_len);
			if(ble_cb_evt_buffer==NULL)
			{
				TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
				return;
			}
			else
			{
				memcpy(ble_cb_evt_buffer,&recv_data[26],dst_content_len);
			}
		}

        event.evt = TUYA_BLE_CB_EVT_TIME_STAMP_WITH_DST;
        event.timestamp_with_dst_data.timestamp = time_stamp;
        event.timestamp_with_dst_data.time_zone = zone_temp;
		event.timestamp_with_dst_data.n_years_dst = years_of_dst;
		event.timestamp_with_dst_data.p_data = ble_cb_evt_buffer;
		event.timestamp_with_dst_data.data_len = dst_content_len;
		
        if(tuya_ble_cb_event_send(&event)!=0)
        {			
			if(years_of_dst>0)
			{
				tuya_ble_free(ble_cb_evt_buffer);
			}            TUYA_BLE_LOG_ERROR("tuya_ble_handle_unix_time_with_dst_resp-tuya ble send cb event failed.");
        }
		//if(years_of_dst>0)
		//{
		//	tuya_ble_free(ble_cb_evt_buffer);
		//}
    }
}

static void tuya_ble_handle_dp_query_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint16_t dp_num = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;
    uint8_t *ble_cb_evt_buffer = NULL;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    dp_num = (recv_data[11]<<8)|recv_data[12];

    p_buf[0] = 0x00;

    tuya_ble_commData_send(FRM_STATE_QUERY_RESP,ack_sn,p_buf,1,ENCRYPTION_MODE_SESSION_KEY);

    event.evt = TUYA_BLE_CB_EVT_DP_QUERY;
	//SYS_LOG_ERR("dp_num %d.",dp_num);		

    if(dp_num>0)
    {
        ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(dp_num);
        if(ble_cb_evt_buffer==NULL)
        {
            TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
            return;
        }
        else
        {
            memcpy(ble_cb_evt_buffer,&recv_data[13],dp_num);
        }
    }
    event.dp_query_data.p_data = ble_cb_evt_buffer;
    event.dp_query_data.data_len = dp_num;
	print_hex("ble_cb_evt_buffer:",ble_cb_evt_buffer,dp_num);

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_query_req-tuya ble send cb event failed.");
	    if(dp_num>0)
	    {
	        tuya_ble_free(ble_cb_evt_buffer);
	    }    }
    //if(dp_num>0)
   // {
    //    tuya_ble_free(ble_cb_evt_buffer);
    //}
}


void tuya_ble_device_unbond(void)
{

    tuya_ble_gap_disconnect();
    memset(tuya_ble_current_para->sys_settings.login_key,0,LOGIN_KEY_LEN);
    memset(tuya_ble_current_para->sys_settings.beacon_key,0,BEACON_KEY_LEN);
    tuya_ble_current_para->sys_settings.bound_flag= 0;
    tuya_ble_storage_save_sys_settings();
    tuya_ble_adv_change();
    tuya_ble_connect_status_set(UNBONDING_UNCONN);
	#if TUYA_BLE_VOS_ENABLE
	//tuya_vos_token_reset();
	#endif
    TUYA_BLE_LOG_INFO("tuya_ble_device_unbond current bound flag = %d",tuya_ble_current_para->sys_settings.bound_flag);
}



static void tuya_ble_handle_unbond_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;
    
#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    encry_mode = ENCRYPTION_MODE_SESSION_KEY;

    p_buf[0] = 0;

    tuya_ble_commData_send(FRM_UNBONDING_RESP,ack_sn,p_buf,1,encry_mode);
    
#if (TUYA_BLE_DEVICE_UNBIND_MODE)    
    tuya_ble_device_unbond();

    event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
    event.connect_status = tuya_ble_connect_status_get();

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_unbond_req-tuya ble send cb event (connect status update) failed.");
    }
    else
    {

    }
#else
    tuya_ble_gap_disconnect();
#endif    
    
#endif
    
    event.evt = TUYA_BLE_CB_EVT_UNBOUND;
    event.unbound_data.data = 0;

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_unbond_req-tuya ble send cb event (unbound req) failed.");
    }
    else
    {

    }
    
}


static void tuya_ble_handle_anomaly_unbond_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    encry_mode = ENCRYPTION_MODE_KEY_1;

    p_buf[0] = 0;

    tuya_ble_commData_send(FRM_ANOMALY_UNBONDING_RESP,ack_sn,p_buf,1,encry_mode);
    tuya_ble_device_unbond();

    event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
    event.connect_status = tuya_ble_connect_status_get();

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_anomaly_unbond_req-tuya ble send cb event (connect status update) failed.");
    }
    else
    {

    }

#endif
    
    event.evt = TUYA_BLE_CB_EVT_ANOMALY_UNBOUND;
    event.anomaly_unbound_data.data = 0;

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_anomaly_unbond_req-tuya ble send cb event (unbound req) failed.");
    }
    else
    {

    }
    
}


static void tuya_ble_handle_device_reset_req(uint8_t*recv_data,uint16_t recv_len)
{
    uint8_t p_buf[1];
    uint8_t encry_mode = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

#if (TUYA_BLE_DEVICE_REGISTER_FROM_BLE)

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    encry_mode = ENCRYPTION_MODE_SESSION_KEY;

    p_buf[0] = 0;

    tuya_ble_commData_send(FRM_DEVICE_RESET_RESP,ack_sn,p_buf,1,encry_mode);
    
#if (TUYA_BLE_DEVICE_UNBIND_MODE)    
    memset(tuya_ble_current_para->sys_settings.device_virtual_id,0,DEVICE_VIRTUAL_ID_LEN);
    tuya_ble_device_unbond();

    event.evt = TUYA_BLE_CB_EVT_CONNECTE_STATUS;
    event.connect_status = tuya_ble_connect_status_get();

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_device_reset_req-tuya ble send cb event (connect status update) failed.");
    }
    else
    {

    }
#else
    tuya_ble_gap_disconnect();
#endif    
    
#endif
    
    event.evt = TUYA_BLE_CB_EVT_DEVICE_RESET;
    event.device_reset_data.data = 0;

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_device_reset_req-tuya ble send cb event device reset req failed.");
    }
    else
    {

    }
    
}

static void tuya_ble_handle_bt_state_req(uint8_t*recv_data,uint16_t recv_len)
{
    // uint8_t p_buf[1];
}

#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==4)

static void tuya_ble_handle_dp_data_write_req(uint8_t*recv_data,uint16_t recv_len)
{
    mtp_ret ret;
    klv_node_s *list = NULL;
    uint8_t p_buf[6];
    uint16_t data_len = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8)|recv_data[12];

    if((data_len<5)||(data_len>TUYA_BLE_RECEIVE_MAX_DP_DATA_LEN))
    {
        TUYA_BLE_LOG_ERROR("cmd dp write error,receive data len == %d",data_len);
        if(data_len<5)
        {
            memcpy(&p_buf[0],&recv_data[13],data_len);
        }
        else
        {
            memcpy(&p_buf[0],&recv_data[13],5);
        }
        p_buf[5] = 1;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }
    else if(recv_data[13]!=0)
    {
        TUYA_BLE_LOG_ERROR("cmd dp write version not support");

        memcpy(&p_buf[0],&recv_data[13],5);

        p_buf[5] = 1;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }
    else
    {

    }
    
    TUYA_BLE_LOG_HEXDUMP_DEBUG("cmd_dp_write data : ",recv_data+13,data_len);
    memcpy(&p_buf[0],&recv_data[13],5);
    ret = data_2_klvlist(&recv_data[18],data_len-5,&list,1);
    if(MTP_OK != ret)
    {
        TUYA_BLE_LOG_ERROR("cmd rx fail-%d",ret);
        p_buf[5] = 1;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }

    free_klv_list(list);

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_RECEIVED;

    uint8_t *ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(data_len-5);
    if(ble_cb_evt_buffer==NULL)
    {
        TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
        p_buf[5] = 0x01;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }
    else
    {
        memcpy(ble_cb_evt_buffer,&recv_data[18],data_len-5);
        event.dp_received_data.sn = (recv_data[14]<<24)|(recv_data[15]<<16)|(recv_data[16]<<8)|recv_data[17];
        event.dp_received_data.p_data = ble_cb_evt_buffer;
        event.dp_received_data.data_len = data_len-5;
    }

    if(tuya_ble_cb_event_send(&event)!=0)
    {
		tuya_ble_free(ble_cb_evt_buffer);
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_write_req-tuya ble send cb event failed.");
        p_buf[5] = 0x01;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
    }
    else
    {
        p_buf[5] = 0x00;
        tuya_ble_commData_send(FRM_DP_DATA_WRITE_RESP,ack_sn,p_buf,6,ENCRYPTION_MODE_SESSION_KEY);
    }
	//if (ble_cb_evt_buffer)
	//	tuya_ble_free(ble_cb_evt_buffer);
}

static void tuya_ble_handle_dp_data_send_response(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len = recv_data[11]<<8|recv_data[12];

    if(data_len!=8)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_send_response- invalid data len received.");
        return;
    }

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_SEND_RESPONSE;
    event.dp_send_response_data.sn = recv_data[14]<<24|recv_data[15]<<16|recv_data[16]<<8|recv_data[17];
    event.dp_send_response_data.ack = ((recv_data[18]&0x80)? DP_SEND_WITHOUT_RESPONSE:DP_SEND_WITH_RESPONSE);
    if((recv_data[18]&0x0F)==1)
    {    
        event.dp_send_response_data.type = DP_SEND_TYPE_PASSIVE;
    }
    else if((recv_data[18]&0x0F)==0)
    {
        event.dp_send_response_data.type = DP_SEND_TYPE_ACTIVE;
    }
    else
    {
        return;
    }
  
    if(recv_data[19]>3)
    {
        return;
    }
    event.dp_send_response_data.mode = recv_data[19];
    event.dp_send_response_data.status = recv_data[20];
    
    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_send_response-tuya ble send cb event failed.");
    }
    else
    {

    }
}

static void tuya_ble_handle_dp_data_with_time_send_response(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len = recv_data[11]<<8|recv_data[12];

    if(data_len!=8)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_time_send_response- invalid data len received.");
        return;
    }

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_WITH_TIME_SEND_RESPONSE;
    event.dp_with_time_send_response_data.sn = recv_data[14]<<24|recv_data[15]<<16|recv_data[16]<<8|recv_data[17];
    event.dp_with_time_send_response_data.ack = ((recv_data[18]&0x80)? DP_SEND_WITHOUT_RESPONSE:DP_SEND_WITH_RESPONSE);
    if((recv_data[18]&0x0F)==1)
    {    
        event.dp_with_time_send_response_data.type = DP_SEND_TYPE_PASSIVE;
    }
    else if((recv_data[18]&0x0F)==0)
    {
        event.dp_with_time_send_response_data.type = DP_SEND_TYPE_ACTIVE;
    }
    else
    {
        return;
    }
  
    if(recv_data[19]>3)
    {
        return;
    }
    event.dp_with_time_send_response_data.mode = recv_data[19];
    event.dp_with_time_send_response_data.status = recv_data[20];
    
    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_time_send_response-tuya ble send cb event failed.");
    }
    else
    {

    }
}

#else //(TUYA_BLE_PROTOCOL_VERSION_HIGN<4)

static void tuya_ble_handle_dp_write_req(uint8_t*recv_data,uint16_t recv_len)
{
    mtp_ret ret;
    klv_node_s *list = NULL;
    uint8_t p_buf[1];
    uint16_t data_len = 0;
    uint32_t ack_sn = 0;
    tuya_ble_cb_evt_param_t event;

    ack_sn  = recv_data[1]<<24;
    ack_sn += recv_data[2]<<16;
    ack_sn += recv_data[3]<<8;
    ack_sn += recv_data[4];

    data_len = (recv_data[11]<<8)|recv_data[12];

    if((data_len==0)||(data_len>TUYA_BLE_RECEIVE_MAX_DP_DATA_LEN))
    {
        TUYA_BLE_LOG_ERROR("cmd dp write receive data len == %d",data_len);
        p_buf[0] = 0x01;
        tuya_ble_commData_send(FRM_CMD_RESP,ack_sn,p_buf,1,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }
    TUYA_BLE_LOG_HEXDUMP_DEBUG("cmd_dp_write data : ",recv_data+13,data_len);
    ret = data_2_klvlist(&recv_data[13],data_len,&list,0);
    if(MTP_OK != ret)
    {
        TUYA_BLE_LOG_ERROR("cmd rx fail-%d",ret);
        p_buf[0] = 0x01;
        tuya_ble_commData_send(FRM_CMD_RESP,ack_sn,p_buf,1,ENCRYPTION_MODE_SESSION_KEY);
        return;
    }

    free_klv_list(list);
    p_buf[0] = 0x00;

    tuya_ble_commData_send(FRM_CMD_RESP,ack_sn,p_buf,1,ENCRYPTION_MODE_SESSION_KEY);

    event.evt = TUYA_BLE_CB_EVT_DP_WRITE;

    uint8_t *ble_cb_evt_buffer=(uint8_t*)tuya_ble_malloc(data_len);
    if(ble_cb_evt_buffer==NULL)
    {
        TUYA_BLE_LOG_ERROR("ble_cb_evt_buffer malloc failed.");
        return;
    }
    else
    {
        memcpy(ble_cb_evt_buffer,&recv_data[13],data_len);
    }
    event.dp_write_data.p_data = ble_cb_evt_buffer;
    event.dp_write_data.data_len = data_len;

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        tuya_ble_free(ble_cb_evt_buffer);  
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_write_req-tuya ble send cb event failed.");
    }
	//if (ble_cb_evt_buffer)
	//	tuya_ble_free(ble_cb_evt_buffer);

}

static void tuya_ble_handle_dp_data_report_res(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_REPORT_RESPONSE;
    event.dp_response_data.status = recv_data[13];

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_report_res-tuya ble send cb event failed.");
    }
    else
    {

    }
}

static void tuya_ble_handle_dp_data_with_time_report_res(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_WTTH_TIME_REPORT_RESPONSE;
    event.dp_response_data.status = recv_data[13];

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_report_res-tuya ble send cb event failed.");
    }
    else
    {

    }
}

static void tuya_ble_handle_dp_data_with_flag_report_res(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len = recv_data[11]<<8|recv_data[12];

    if(data_len!=4)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_flag_report_res- invalid data len received.");
        return;
    }

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_REPORT_RESPONSE;
    event.dp_with_flag_response_data.sn = recv_data[13]<<8|recv_data[14];
    event.dp_with_flag_response_data.mode = recv_data[15];
    event.dp_with_flag_response_data.status = recv_data[16];

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_flag_report_res-tuya ble send cb event failed.");
    }
    else
    {

    }
}

static void tuya_ble_handle_dp_data_with_flag_and_time_report_res(uint8_t*recv_data,uint16_t recv_len)
{
    tuya_ble_cb_evt_param_t event;
    uint16_t data_len = recv_data[11]<<8|recv_data[12];

    if(data_len!=4)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_flag_and_time_report_res- invalid data len received.");
        return;
    }

    event.evt = TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_AND_TIME_REPORT_RESPONSE;
    event.dp_with_flag_and_time_response_data.sn = recv_data[13]<<8|recv_data[14];
    event.dp_with_flag_and_time_response_data.mode = recv_data[15];
    event.dp_with_flag_and_time_response_data.status = recv_data[16];

    if(tuya_ble_cb_event_send(&event)!=0)
    {
        TUYA_BLE_LOG_ERROR("tuya_ble_handle_dp_data_with_flag_and_time_report_res-tuya ble send cb event failed.");
    }
    else
    {

    }
}

#endif
#if 0

void tuya_ble_evt_process1(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
	switch(cmd)
	{
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==4) 
        
    case FRM_DP_DATA_WRITE_REQ:
        tuya_ble_handle_dp_data_write_req(recv_data,recv_len);
        break; 
    case FRM_DP_DATA_SEND_RESP:
        tuya_ble_handle_dp_data_send_response(recv_data,recv_len);
        break;
    case FRM_DP_DATA_WITH_TIME_SEND_RESP:
        tuya_ble_handle_dp_data_with_time_send_response(recv_data,recv_len);
        break;
    
#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)
    
    case FRM_AUTHENTICATE_PHASE_1_REQ:
        tuya_ble_handle_authenticate_phase1(recv_data,recv_len);
        break;
    case FRM_AUTHENTICATE_PHASE_2_REQ:
        tuya_ble_handle_authenticate_phase2(recv_data,recv_len);
        break;
    case FRM_AUTHENTICATE_PHASE_3_REQ:
        tuya_ble_handle_authenticate_phase3(recv_data,recv_len);
        break;
    
#endif
    
#if (TUYA_BLE_FEATURE_WEATHER_ENABLE != 0)
	case FRM_WEATHER_DATA_REQUEST_RESP:
		tuya_ble_handle_weather_data_request_response(recv_data,recv_len);
		break;
	case FRM_WEATHER_DATA_RECEIVED:
		tuya_ble_handle_weather_data_received(recv_data,recv_len);
		break;
#endif 

#else  	
	
	case FRM_CMD_SEND:
		tuya_ble_handle_dp_data_write_req(recv_data,recv_len);
		break;
    case FRM_STAT_REPORT_RESP:
        tuya_ble_handle_dp_data_report_res(recv_data,recv_len);
        break;
    case FRM_DATA_WITH_FLAG_REPORT_RESP:
        tuya_ble_handle_dp_data_with_flag_report_res(recv_data,recv_len);
        break;
    case FRM_STAT_WITH_TIME_REPORT_RESP:
        tuya_ble_handle_dp_data_with_time_report_res(recv_data,recv_len);
        break;
    case FRM_DATA_WITH_FLAG_AND_TIME_REPORT_RESP:
        tuya_ble_handle_dp_data_with_flag_and_time_report_res(recv_data,recv_len);
        break;  
#endif 
	case FRM_STATE_QUERY:
		tuya_ble_handle_dp_query_req(recv_data,recv_len);
		break;
	case FRM_QRY_DEV_INFO_REQ:
		tuya_ble_handle_dev_info_req(recv_data,recv_len);
		break;
	case PAIR_REQ:
		tuya_ble_handle_pair_req(recv_data,recv_len);
		break;
	}

}


void tuya_ble_evt_process2(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
		switch(cmd)
		{
		case FRM_NET_CONFIG_INFO_REQ:
			tuya_ble_handle_net_config_info_req(recv_data,recv_len);
			break;
		case FRM_DATA_PASSTHROUGH_REQ:
			tuya_ble_handle_ble_passthrough_data_req(recv_data,recv_len);
			break;
	case FRM_BULK_DATA_READ_INFO_REQ:
    case FRM_BULK_DATA_READ_DATA_REQ:
    case FRM_BULK_DATA_ERASE_DATA_REQ:
        TUYA_BLE_LOG_INFO("RECEIVED BULK DATA CMD:0x%02x DATA LEN:0x%02x",cmd,recv_len);
        tuya_ble_handle_bulk_data_req(cmd,recv_data,recv_len);
        break;
#ifdef TUYA_BLE_APP_OTA // zdx
		case FRM_OTA_START_REQ:
		case FRM_OTA_FILE_INFOR_REQ:
		case FRM_OTA_FILE_OFFSET_REQ:
		case FRM_OTA_DATA_REQ:
		case FRM_OTA_END_REQ:
			TUYA_BLE_LOG_INFO("RECEIVED OTA CMD:0x%02x DATA LEN:0x%02x",cmd,recv_len);
			tuya_ble_handle_ota_req(cmd,recv_data,recv_len);
			break;
#endif

		case FRM_GET_UNIX_TIME_CHAR_MS_RESP:
			tuya_ble_handle_unix_time_char_ms_resp(recv_data,recv_len);
			break;
		case FRM_GET_UNIX_TIME_CHAR_DATE_RESP:
			tuya_ble_handle_unix_time_date_resp(recv_data,recv_len);
			break;
		case FRM_GET_APP_LOCAL_TIME_RESP:
	        tuya_ble_handle_app_local_time_data_resp(recv_data,recv_len);
	        break;
	    case FRM_GET_UNIX_TIME_WITH_DST_RESP:
	        tuya_ble_handle_unix_time_with_dst_date_resp(recv_data,recv_len);
	        break;
		case FRM_UNBONDING_REQ:
			TUYA_BLE_LOG_INFO("RECEIVED FRM_UNBONDING_REQ");
			tuya_ble_handle_unbond_req(recv_data,recv_len);
			break;
		}
}

void tuya_ble_evt_process3(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
	switch(cmd)
	{
	case FRM_ANOMALY_UNBONDING_REQ:
		TUYA_BLE_LOG_INFO("RECEIVED FRM_ANOMALY_UNBONDING_REQ");
		tuya_ble_handle_anomaly_unbond_req(recv_data,recv_len);
		break;
	case FRM_DEVICE_RESET:
		TUYA_BLE_LOG_INFO("RECEIVED FRM_DEVICE_RESET_REQ");
		tuya_ble_handle_device_reset_req(recv_data,recv_len);
		break;
	case FRM_STAT_REPORT_RESP:
		tuya_ble_handle_dp_data_report_res(recv_data,recv_len);
		break;
	case FRM_DATA_WITH_FLAG_REPORT_RESP:
		tuya_ble_handle_dp_data_with_flag_report_res(recv_data,recv_len);
		break;
	case FRM_STAT_WITH_TIME_REPORT_RESP:
		tuya_ble_handle_dp_data_with_time_report_res(recv_data,recv_len);
		break;
	case FRM_DATA_WITH_FLAG_AND_TIME_REPORT_RESP:
		tuya_ble_handle_dp_data_with_flag_and_time_report_res(recv_data,recv_len);
		break;
#ifdef TUYA_BLE_APP_PRODUCT //zdx
	case FRM_FACTORY_TEST_CMD:
		tuya_ble_handle_ble_factory_test_req(recv_data,recv_len);
		break;
#endif
    case BT_STATE_QUREY:
        TUYA_BLE_LOG_INFO("RECEIVED BT_STATE_QUREY");
        tuya_ble_handle_bt_state_req(recv_data,recv_len);
        break;
	default:
		TUYA_BLE_LOG_WARNING("RECEIVED UNKNOWN BLE EVT CMD-0x%04x",cmd);
		break;
	}
}
#endif

void tuya_ble_evt_process(uint16_t cmd,uint8_t*recv_data,uint32_t recv_len)
{
	SYS_LOG_INF("%d\n", cmd);
    switch(cmd)
    {
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN==4) 
        
    case FRM_DP_DATA_WRITE_REQ:
        tuya_ble_handle_dp_data_write_req(recv_data,recv_len);
        break; 
    case FRM_DP_DATA_SEND_RESP:
        tuya_ble_handle_dp_data_send_response(recv_data,recv_len);
        break;
    case FRM_DP_DATA_WITH_TIME_SEND_RESP:
        tuya_ble_handle_dp_data_with_time_send_response(recv_data,recv_len);
        break;
    
#if (TUYA_BLE_SECURE_CONNECTION_TYPE==TUYA_BLE_SECURE_CONNECTION_WITH_AUTH_KEY_ADVANCED_ENCRYPTION)
    
    case FRM_AUTHENTICATE_PHASE_1_REQ:
        tuya_ble_handle_authenticate_phase1(recv_data,recv_len);
        break;
    case FRM_AUTHENTICATE_PHASE_2_REQ:
        tuya_ble_handle_authenticate_phase2(recv_data,recv_len);
        break;
    case FRM_AUTHENTICATE_PHASE_3_REQ:
        tuya_ble_handle_authenticate_phase3(recv_data,recv_len);
        break;
    
#endif
    
#if (TUYA_BLE_FEATURE_WEATHER_ENABLE != 0)
	case FRM_WEATHER_DATA_REQUEST_RESP:
		tuya_ble_handle_weather_data_request_response(recv_data,recv_len);
		break;
	case FRM_WEATHER_DATA_RECEIVED:
		tuya_ble_handle_weather_data_received(recv_data,recv_len);
		break;
#endif 

#if (TUYA_BLE_FEATURE_IOT_CHANNEL_ENABLE != 0)
	case FRM_IOT_DATA_REQUEST_RESP:
		tuya_ble_handle_iot_data_request_response(recv_data,recv_len);
		break;
	case FRM_IOT_DATA_RECEIVED:
		tuya_ble_handle_iot_data_received(recv_data,recv_len);
		break;
#endif

#else        
    case FRM_CMD_SEND:
        tuya_ble_handle_dp_write_req(recv_data,recv_len);
        break;
    case FRM_STAT_REPORT_RESP:
        tuya_ble_handle_dp_data_report_res(recv_data,recv_len);
        break;
    case FRM_DATA_WITH_FLAG_REPORT_RESP:
        tuya_ble_handle_dp_data_with_flag_report_res(recv_data,recv_len);
        break;
    case FRM_STAT_WITH_TIME_REPORT_RESP:
        tuya_ble_handle_dp_data_with_time_report_res(recv_data,recv_len);
        break;
    case FRM_DATA_WITH_FLAG_AND_TIME_REPORT_RESP:
        tuya_ble_handle_dp_data_with_flag_and_time_report_res(recv_data,recv_len);
        break;  
#endif        
    case FRM_STATE_QUERY:
        tuya_ble_handle_dp_query_req(recv_data,recv_len);
        break;
    case FRM_QRY_DEV_INFO_REQ:
        tuya_ble_handle_dev_info_req(recv_data,recv_len);
        break;
    case PAIR_REQ:
        tuya_ble_handle_pair_req(recv_data,recv_len);
        break;
    case FRM_NET_CONFIG_INFO_REQ:
        tuya_ble_handle_net_config_info_req(recv_data,recv_len);
        break;
    case FRM_DATA_PASSTHROUGH_REQ:
        tuya_ble_handle_ble_passthrough_data_req(recv_data,recv_len);
        break;
	case FRM_BULK_DATA_READ_INFO_REQ:
    case FRM_BULK_DATA_READ_DATA_REQ:
    case FRM_BULK_DATA_ERASE_DATA_REQ:
        TUYA_BLE_LOG_INFO("RECEIVED BULK DATA CMD:0x%02x DATA LEN:0x%02x",cmd,recv_len);
        tuya_ble_handle_bulk_data_req(cmd,recv_data,recv_len);
        break;
#if TUYA_BLE_VOS_ENABLE	
	case TUYA_BLE_FRM_VOS_CMD:
    case TUYA_BLE_FRM_VOS_STREAMING:
    case TUYA_BLE_FRM_VOS_RESPONSE:
	case TUYA_BLE_FRM_VOS_TIMER_SETTING:
	case TUYA_BLE_FRM_VOS_TOKEN_READING:
	case TUYA_BLE_FRM_VOS_TOKEN_SEND_RESP:
	case TUYA_BLE_FRM_VOS_TOKEN_WRITE:
	case TUYA_BLE_FRM_VOS_COMMON_SET:
        TUYA_BLE_LOG_INFO("RECEIVED VOS CMD:0x%02x DATA LEN:0x%02x",cmd,recv_len);
        tuya_ble_handle_speech_req(cmd,recv_data,recv_len);
        break;
#endif
#if 1//def TUYA_BLE_APP_OTA // zdx
    case FRM_OTA_START_REQ:
    case FRM_OTA_FILE_INFOR_REQ:
    case FRM_OTA_FILE_OFFSET_REQ:
    case FRM_OTA_DATA_REQ:
    case FRM_OTA_END_REQ:
        TUYA_BLE_LOG_INFO("RECEIVED OTA CMD:0x%02x DATA LEN:0x%02x",cmd,recv_len);
        tuya_ble_handle_ota_req(cmd,recv_data,recv_len);
#endif
        break;

    case FRM_GET_UNIX_TIME_CHAR_MS_RESP:
        tuya_ble_handle_unix_time_char_ms_resp(recv_data,recv_len);
        break;
    case FRM_GET_UNIX_TIME_CHAR_DATE_RESP:
        tuya_ble_handle_unix_time_date_resp(recv_data,recv_len);
        break;
	case FRM_GET_APP_LOCAL_TIME_RESP:
        tuya_ble_handle_app_local_time_data_resp(recv_data,recv_len);
        break;


    case FRM_GET_UNIX_TIME_WITH_DST_RESP:
        tuya_ble_handle_unix_time_with_dst_date_resp(recv_data,recv_len);
        break;
    case FRM_UNBONDING_REQ:
        TUYA_BLE_LOG_INFO("RECEIVED FRM_UNBONDING_REQ");
        tuya_ble_handle_unbond_req(recv_data,recv_len);
        break;
    case FRM_ANOMALY_UNBONDING_REQ:
        TUYA_BLE_LOG_INFO("RECEIVED FRM_ANOMALY_UNBONDING_REQ");
        tuya_ble_handle_anomaly_unbond_req(recv_data,recv_len);
        break;


	case FRM_DEVICE_RESET:
        TUYA_BLE_LOG_INFO("RECEIVED FRM_DEVICE_RESET_REQ");
        tuya_ble_handle_device_reset_req(recv_data,recv_len);
        break;
#ifdef TUYA_BLE_APP_PRODUCT //zdx    
    case FRM_FACTORY_TEST_CMD:
        tuya_ble_handle_ble_factory_test_req(recv_data,recv_len);
        break;
#endif
    case BT_STATE_QUREY:
        TUYA_BLE_LOG_INFO("RECEIVED BT_STATE_QUREY");
        tuya_ble_handle_bt_state_req(recv_data,recv_len);
        break;

    default:
        // TUYA_BLE_LOG_WARNING("RECEIVED UNKNOWN BLE EVT CMD-0x%04x",cmd);
        break;

    }
}

uint8_t tuya_ble_commData_send(uint16_t cmd,uint32_t ack_sn,uint8_t *data,uint16_t len,uint8_t encryption_mode)
{
    mtp_ret ret;
    uint16_t send_len = 0;
    uint8_t *p_buf = NULL;
    uint32_t err=0;
    //int8_t retries_cnt = 0;
    uint8_t iv[16];
    uint16_t rand_value = 0,i=0;
    uint16_t crc16 = 0;
    uint16_t en_len  = 0;
    uint32_t out_len = 0;
    uint32_t temp_len = 0;
	uint32_t package_number = 0;
    uint16_t p_version = 0;
    tuya_ble_r_air_send_packet  air_send_packet;
    frm_trsmitr_proc_s* ty_trsmitr_proc_send = NULL;

    memset(&air_send_packet,0,sizeof(air_send_packet));

    tuya_ble_connect_status_t currnet_connect_status = tuya_ble_connect_status_get();

    if((currnet_connect_status == BONDING_UNCONN)||(currnet_connect_status== UNBONDING_UNCONN))
    {
        TUYA_BLE_LOG_ERROR("tuya ble commData_send failed,because ble not in connect status.");
        return 2;
    }

    if((encryption_mode>=ENCRYPTION_MODE_MAX)||(len>TUYA_BLE_SEND_MAX_DATA_LEN))
    {
        return 1;
    }

    //生成随机IV
    if(encryption_mode != ENCRYPTION_MODE_NONE)
    {
        for(i=0; i<16; i+=2)
        {
            rand_value = rand();
            iv[i+0] = rand_value>>8;
            iv[i+1] = rand_value;
        }
        en_len = 17;
    }
    else
    {
        en_len = 1;
        memset(iv,0,sizeof(iv));
    }
    
    air_send_packet.send_len = 14+len;

    if(air_send_packet.send_len%16==0)
    {
        temp_len = 0;
    }
    else
    {
        temp_len = 16 - air_send_packet.send_len%16;
    }

    temp_len += air_send_packet.send_len;

    if(temp_len>(TUYA_BLE_AIR_FRAME_MAX-en_len))
    {
        TUYA_BLE_LOG_ERROR("The length of the send to ble exceeds the maximum length.");
        air_send_packet.send_len = 0;
        return 1; //加密后数据加上加密头超过AIR_FRAME_MAX
    }
      
    air_send_packet.send_data = NULL;
       
    air_send_packet.send_data = (uint8_t *)tuya_ble_malloc(temp_len); //must temp_len
    
    if(air_send_packet.send_data==NULL)
    {
        TUYA_BLE_LOG_ERROR("air_send_packet.send_data malloc failed return 3.");
        air_send_packet.send_len = 0;
        return 3;
    }
    else
    {
        memset(air_send_packet.send_data,0,temp_len);
    }


    uint32_t send_sn = get_ble_send_sn();
    //利用send_data buffer缓存明文指令数据
    air_send_packet.send_data[0] = send_sn>>24;
    air_send_packet.send_data[1] = send_sn>>16;
    air_send_packet.send_data[2] = send_sn>>8;
    air_send_packet.send_data[3] = send_sn;

    air_send_packet.send_data[4] = ack_sn>>24;
    air_send_packet.send_data[5] = ack_sn>>16;
    air_send_packet.send_data[6] = ack_sn>>8;
    air_send_packet.send_data[7] = ack_sn;

    air_send_packet.send_data[8] = cmd>>8;
    air_send_packet.send_data[9] = cmd;

    air_send_packet.send_data[10] = len>>8;
    air_send_packet.send_data[11] = len;

    memcpy(&air_send_packet.send_data[12],data,len);

    crc16 = tuya_ble_crc16_compute(air_send_packet.send_data,12+len, NULL);

    air_send_packet.send_data[12+len] = crc16>>8;
    air_send_packet.send_data[13+len] = crc16;
    

    //TUYA_BLE_LOG_HEXDUMP_DEBUG("ble_commData_send plain data",(uint8_t*)air_send_packet.send_data,air_send_packet.send_len);//
  
    air_send_packet.encrypt_data_buf = NULL;
    
    air_send_packet.encrypt_data_buf = (uint8_t *)tuya_ble_malloc(temp_len+en_len);
    
    if(air_send_packet.encrypt_data_buf==NULL)
    {
        TUYA_BLE_LOG_ERROR("air_send_packet.encrypt_data_buf malloc failed.");
        tuya_ble_free(air_send_packet.send_data);
        return 3;
    }
    else
    {
        air_send_packet.encrypt_data_buf_len = 0;
        memset(air_send_packet.encrypt_data_buf,0,temp_len+en_len);
    }
     
         
    air_send_packet.encrypt_data_buf[0] = encryption_mode;

    if(encryption_mode != ENCRYPTION_MODE_NONE)
    {
        memcpy(&air_send_packet.encrypt_data_buf[1],iv,16);
    }
    else
    {
        
    }
    
    p_version = (TUYA_BLE_PROTOCOL_VERSION_HIGN<<8) + TUYA_BLE_PROTOCOL_VERSION_LOW;
	print_hex("pair_rand:", tuya_ble_pair_rand, sizeof(tuya_ble_pair_rand));
    if(tuya_ble_encryption(p_version,encryption_mode,iv,(uint8_t *)air_send_packet.send_data,air_send_packet.send_len,&out_len,
        (uint8_t *)(air_send_packet.encrypt_data_buf+en_len),tuya_ble_current_para,tuya_ble_pair_rand)==0)
    {
        if((encryption_mode != ENCRYPTION_MODE_NONE)&&(out_len != temp_len))
        {
            TUYA_BLE_LOG_ERROR("ble_commData_send encryed error."); 
            tuya_ble_free(air_send_packet.send_data);        
            tuya_ble_free(air_send_packet.encrypt_data_buf);        
            return 1;
        }

        air_send_packet.encrypt_data_buf_len = en_len + out_len;

        //TUYA_BLE_LOG_HEXDUMP_DEBUG("ble_commData_send encryped data",(uint8_t*)air_send_packet.encrypt_data_buf,air_send_packet.encrypt_data_buf_len);//
    }
    else
    {
        TUYA_BLE_LOG_ERROR("ble_commData_send encryed fail."); 
        tuya_ble_free(air_send_packet.send_data);        
        tuya_ble_free(air_send_packet.encrypt_data_buf);        
        return 1;
    }
    
    tuya_ble_free(air_send_packet.send_data);
    package_number = 0;
    if((send_packet_data_len==0)||(send_packet_data_len>TUYA_BLE_DATA_MTU_MAX))
    {
        send_packet_data_len = 20;
    }

	ty_trsmitr_proc_send = tuya_ble_malloc(sizeof(frm_trsmitr_proc_s));
	ty_trsmitr_proc_send->subpkg = tuya_ble_malloc(SNGL_PKG_TRSFR_LMT);
	//SYS_LOG_ERR("subpkg 0x%p",ty_trsmitr_proc_send->subpkg);
    //trsmitr_init(&ty_trsmitr_proc_send);
    do
    {
        ret = trsmitr_send_pkg_encode_with_packet_length(ty_trsmitr_proc_send,send_packet_data_len,TUYA_BLE_PROTOCOL_VERSION_HIGN,
        (uint8_t *)(air_send_packet.encrypt_data_buf), air_send_packet.encrypt_data_buf_len);
        if (MTP_OK != ret && MTP_TRSMITR_CONTINUE != ret)
        {
            tuya_ble_free(air_send_packet.encrypt_data_buf);
			tuya_ble_free(ty_trsmitr_proc_send->subpkg);
			tuya_ble_free(ty_trsmitr_proc_send);
			SYS_LOG_ERR("");
            return 1;
        }
        send_len = get_trsmitr_subpkg_len(ty_trsmitr_proc_send);
        p_buf = get_trsmitr_subpkg(ty_trsmitr_proc_send);
		package_number++;

		tuya_ble_gatt_send_data_enqueue(p_buf,send_len);
        
    } while (ret == MTP_TRSMITR_CONTINUE);

    TUYA_BLE_LOG_INFO("ble_commData_send len = %d , package_number = %d , protocol version : 0x%02x , error code : 0x%02x",air_send_packet.encrypt_data_buf_len,package_number,TUYA_BLE_PROTOCOL_VERSION_HIGN,err);

    tuya_ble_free(air_send_packet.encrypt_data_buf);  
	//SYS_LOG_ERR("subpkg 0x%p",ty_trsmitr_proc_send->subpkg);

	tuya_ble_free(ty_trsmitr_proc_send->subpkg);
	tuya_ble_free(ty_trsmitr_proc_send);

    return 0;
}

#if 0
#include "tuya_ble_event_handler.h"

void tuya_ble_event_process_new(tuya_ble_evt_param_t *tuya_ble_evt)
{
	printf("%s %d\n", __func__, tuya_ble_evt->hdr.event);
    switch(tuya_ble_evt->hdr.event)
    {
    case TUYA_BLE_EVT_DEVICE_INFO_UPDATE:
        tuya_ble_handle_device_info_update_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_DP_DATA_REPORTED:
        tuya_ble_handle_dp_data_reported_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_FACTORY_RESET:
        tuya_ble_handle_factory_reset_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_OTA_RESPONSE:
        tuya_ble_handle_ota_response_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_DATA_PASSTHROUGH:
        tuya_ble_handle_data_passthrough_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_MTU_DATA_RECEIVE:
         TUYA_BLE_LOG("ble receive data event\n");
        // tuya_ble_commData_send_mtu(tuya_evt.ble_rev_event.data,tuya_evt.ble_rev_event.len);
//        TUYA_BLE_HEXDUMP("received ble mtu data",20,(uint8_t*)tuya_ble_evt->mtu_data.data,tuya_ble_evt->mtu_data.len);//
        tuya_ble_commonData_rx_proc(tuya_ble_evt->mtu_data.data,tuya_ble_evt->mtu_data.len);
        break;
    case TUYA_BLE_EVT_DP_DATA_WITH_TIME_REPORTED:
        tuya_ble_handle_dp_data_with_time_reported_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_UART_CMD:
        tuya_ble_handle_uart_cmd_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_BLE_CMD:
        tuya_ble_handle_ble_cmd_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_NET_CONFIG_RESPONSE:
        tuya_ble_handle_net_config_response_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_CUSTOM:
        tuya_ble_evt->custom_evt.custom_event_handler(tuya_ble_evt->custom_evt.evt_id, tuya_ble_evt->custom_evt.data);
        break;
    case TUYA_BLE_EVT_CONNECT_STATUS_UPDATE:
        tuya_ble_handle_connect_change_evt(tuya_ble_evt);
        break;
#if (!TUYA_BLE_DEVICE_REGISTER_FROM_BLE)
    case TUYA_BLE_EVT_UNBOUND_RESPONSE:
        tuya_ble_handle_unbound_response_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_ANOMALY_UNBOUND_RESPONSE:
        tuya_ble_handle_anomaly_unbound_response_evt(tuya_ble_evt);
        break;
    case TUYA_BLE_EVT_DEVICE_RESET_RESPONSE:
        tuya_ble_handle_device_reset_response_evt(tuya_ble_evt);
        break;
#endif
    default:
        break;
    }
}
#endif
#endif

