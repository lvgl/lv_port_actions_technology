#if 1//def CFG_TUYA_BLE_APP_SUPPORT

#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <app_switch.h>
//#include <app_config.h>
#include <os_common_api.h>
#include <ctype.h>
#include <user_comm/ap_record.h>
#include <user_comm/ap_status.h>

#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_event.h"
#include "tuya_ble_service.h"
#include "tuya_ble_app_demo.h"
#include "tuya_ble_demo_version.h"
#include "tuya_ble_log.h"
#include "tuya_ble_vos.h"
#include "tuya_ble_mutli_tsf_protocol.h"
#define BTMUSIC_MULTI_DAE		"BTMUSIC_MULTI_DAE"

//#include "ota.h"


// keys for publish
//static const char auth_key_test[] = "yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy";
//static const char device_id_test[] = "zzzzzzzzzzzzzzzz";
//static const uint8_t mac_test[6] = {0x11,0x22,0x33,0x44,0x55,0x66}; //The actual MAC address is : 66:55:44:33:22:11

// keys form xinlianxin
static const char auth_key_test[] = "a0EmOTgb9YPGuf15IIMekxOUArra1afz";
static const char device_id_test[] = "tuya6ff7b0e3a8e1";
static const uint8_t mac_test[6] = {0xB1,0x84,0x3A,0x4D,0x23,0xDC}; //The actual MAC address is : DC:23:4D:2D:DD:71
//static uint16_t sn = 0;

#define APP_CUSTOM_EVENT_1	1
#define APP_CUSTOM_EVENT_2	2
#define APP_CUSTOM_EVENT_3	3
#define APP_CUSTOM_EVENT_4	4
#define APP_CUSTOM_EVENT_5	5

#define TUYA_DP_ARRAY_LENGTH (255+3)

uint8_t *dp_data_array;
static uint16_t dp_data_len = 0;
uint8_t find_device_timer_id = 0;
uint32_t device_sn = 0;
uint8_t restart_va_status = 0;

typedef struct {
	uint8_t data[50];
} custom_data_type_t;

extern void btmusic_multi_dae_adjust(u8_t dae_index);

//tuya_ble_count_down_set_t tuya_ble_count_down_set;

void custom_data_process(int32_t evt_id,void *data)
{
	custom_data_type_t *event_1_data;
	TUYA_APP_LOG_DEBUG("custom event id = %d",evt_id);
	switch (evt_id)
	{
		case APP_CUSTOM_EVENT_1:
			event_1_data = (custom_data_type_t *)data;
			TUYA_APP_LOG_HEXDUMP_DEBUG("received APP_CUSTOM_EVENT_1 data:",event_1_data->data,50);
			break;
		case APP_CUSTOM_EVENT_2:
			break;
		case APP_CUSTOM_EVENT_3:
			break;
		case APP_CUSTOM_EVENT_4:
			break;
		case APP_CUSTOM_EVENT_5:
			break;
		default:
			break;
  
	}
}

custom_data_type_t *custom_data; //zdx

void tuya_start_tone_play_timer(u32_t type)
{
#if 0

	bt_ma_va_tone_set(TRUE);
	switch(type)
	{
		case TUYA_NET_CONFIG_READY: // 配网成功 ful_alerts_notification_03
			bt_manager_tws_send_notify(SYS_EVENT_TUYA_NET_CONFIG_READY, YES);
			sys_manager_api->event_notify(SYS_EVENT_TUYA_NET_CONFIG_READY, LED_VIEW_SYS_NOTIFY, YES);			//bt_ma_va_tone_set(FALSE);
			break;
		case TUYA_ALEXA_AUTH_PASS: // ALEXA登陆授权通过 connect
			bt_manager_tws_send_notify(SYS_EVENT_BT_CONNECTED, YES);
			sys_manager_api->event_notify(SYS_EVENT_BT_CONNECTED, LED_VIEW_SYS_NOTIFY, YES);
			break;
		case TUYA_VOS_TIMER_SET: // 设置定时 ful_system_alerts_melodic_02
			bt_manager_tws_send_notify(SYS_EVENT_TUYA_VOS_TIMER_SET, YES);
			sys_manager_api->event_notify(SYS_EVENT_TUYA_VOS_TIMER_SET, LED_VIEW_SYS_NOTIFY, YES);
			break;
		case TUYA_VOS_ALARM_SET: // 设置闹钟 ful_system_alerts_melodic_01
			bt_manager_tws_send_notify(SYS_EVENT_TUYA_VOS_ALARM_SET, YES);
			sys_manager_api->event_notify(SYS_EVENT_TUYA_VOS_ALARM_SET, LED_VIEW_SYS_NOTIFY, YES);
			break;
		case TUYA_VOS_ALEXA_TTS: // ALEXA唤醒TTS med_ui_wakesound_TTH
			bt_manager_tws_send_notify(SYS_EVENT_TUYA_VOS_ALEXA_TTS, YES);
			sys_manager_api->event_notify(SYS_EVENT_TUYA_VOS_ALEXA_TTS, LED_VIEW_SYS_NOTIFY, YES);
			break;
		case TUYA_VOS_INCOMING_NOTIFY: // 消息通知 ful_alerts_notification_03
			bt_manager_tws_send_notify(SYS_EVENT_TUYA_NET_CONFIG_READY, YES);
			sys_manager_api->event_notify(SYS_EVENT_TUYA_NET_CONFIG_READY, LED_VIEW_SYS_NOTIFY, YES);
			break;
														 
		default:
			break;
	}
	bt_ma_va_tone_set(FALSE);
#endif

}

void tuya_va_resource_alloc(void)
{
	record_stream_init_param user_param;

	if (AP_STATUS_RECORDING == ap_status_get())
	{
		SYS_LOG_INF("restart!");
		restart_va_status = 1;
		//p->interupt_voice = 1;
		//bt_ma_va_restart_init(p->link_hdl);
	}
	else 
	{
		if (bt_manager_hfp_get_status() > 0)
			return;

		/* start record after in push-to-talk*/
		user_param.stream_send_cb = tuya_send_pkg_to_stream;
		user_param.release_cb = tuya_record_resource_release;
		//app_switch_add_app(APP_ID_AP_RECORD);
		//app_switch(APP_ID_AP_RECORD, APP_SWITCH_NEXT, false);
		record_start_record(&user_param);
		ap_status_set(AP_STATUS_RECORDING);
	}
}

void tuya_va_resource_release(void)
{
	if (AP_STATUS_RECORDING == ap_status_get())
	{
		record_stop_record();
		ap_status_set(AP_STATUS_AUTH);
		app_switch_add_app(APP_ID_BTMUSIC);
		app_switch(APP_ID_BTMUSIC, APP_SWITCH_NEXT, false);
	}
	else 
	{
		SYS_LOG_INF("voice stop already!");
	}
	restart_va_status = 0;
}

void tuya_upload_speech_request(void)
{
	if (TUYA_BLE_SUCCESS == tuya_ble_start_speech(0))
		tuya_va_resource_alloc();
}

void tuya_upload_speech_restart_request(void)
{
	tuya_ble_stop_speech();
	tuya_ble_start_speech(0);
	restart_va_status = 1;
}

u8_t tuya_restart_va_status_get(void)
{
	return restart_va_status;
}

void tuya_restart_va_status_set(u8_t status)
{
	restart_va_status = status;
}

void tuya_upload_speech_stop_request(u8_t status)
{
	if (0 == status)
		tuya_ble_stop_speech();
	
	tuya_va_resource_release();
}

void tuya_start_tone_play(u32_t type)
{
#if 0
	app_timer_delay_proc_ex
	(
		 APP_ID_MANAGER, 0,
		 (void*)tuya_start_tone_play_timer, (void *)type
	);		
#endif
}


void custom_evt_1_send_test(uint8_t data)
{	 
	tuya_ble_custom_evt_t event;
	uint8_t i=0; 

	if (!custom_data)
	{
		custom_data = (custom_data_type_t *)tuya_ble_malloc(sizeof(custom_data_type_t));
	}
	else
	{
		//SYS_LOG_WRN();
	}

	for(i=0; i<50; i++)
	{
		custom_data->data[i] = data;
	}
	event.evt_id = APP_CUSTOM_EVENT_1;
	event.custom_event_handler = (void *)custom_data_process;
	event.data = custom_data;
	tuya_ble_custom_event_send(event);
}


static int tuya_set_music_play_volume(uint8_t volume)
{
#if 0
	audio_volume_ctrl_t*  vol_ctrl = &(sys_manager_api->get_status()->vol_ctrl);

	bt_manager_configs_t*  bt_configs = bt_manager_get_configs();
	
	bt_manager_dev_info_t*	dev_info = 
		bt_manager_get_dev_info(BT_MANAGER_A2DP_ACTIVE_DEV);
	u8_t  params[7];

	printf("tuya_set_music_play_volume: %d\n", volume);

	volume = volume*CFG_MAX_BT_MUSIC_VOLUME/100;

	if (volume > CFG_MAX_BT_MUSIC_VOLUME)
	{
		return -1;
	}

	if (vol_ctrl->bt_music_vol != volume)
	{
		vol_ctrl->bt_music_vol = volume;
		vol_ctrl->voice_vol = volume;
		vol_ctrl->tone_vol	= volume;

		/* Í¬²½ÊÖ»úÉè±¸ÒôÁ¿
		 */
		if (dev_info != NULL)
		{
			if (dev_info->ext_info->avrcp_support_vol_sync == NO ||
				bt_configs->cfg_bt_music_vol_sync.Volume_Sync_Only_When_Playing == NO ||
				dev_info->a2dp_status_playing)
			{
				dev_info->ext_info->bt_music_vol = vol_ctrl->bt_music_vol;
			}
		}


		memcpy(&params[0], dev_info->bd_addr, 6);
		params[6] = vol_ctrl->bt_music_vol;
		
		bt_manager_tws_send_event_ex
		(
			TWS_EVENT_SYNC_BT_MUSIC_VOL, params, sizeof(params), NO
		);

		audio_manager_set_volume(vol_ctrl);
		
		bt_manager_avrcp_sync_vol_by_key(NULL);
	}
	

#endif
	return 0;
}

void tuya_set_low_latency_mode(bool_t low_latency_mode)
{
	u8_t  last_low_latency_mode = audio_get_low_latency_mode();	

	TUYA_APP_LOG_INFO("%d %d", last_low_latency_mode, low_latency_mode);
	if (last_low_latency_mode != low_latency_mode)
	{
		system_switch_low_latency_mode();
	}
}

void tuya_ctrl_power_off(void)
{
	system_app_enter_poweroff(0);
}

void bt_music_tuya_ble_cmd_deal(tuya_ble_dp_write_data_t *Dp_data)
{
	u8_t Dp_id, Dp_len;
	u8_t dae_index;

	//log_debug();
	Dp_len = Dp_data->data_len;
	Dp_id = Dp_data->p_data[0];

	print_hex("bt_music_tuya_ble_cmd_deal:", Dp_data->p_data, Dp_len);

	
	switch(Dp_id)
	{
		case TUYA_BLE_DP_ID_PLAY_PAUSE:
			if (Dp_data->p_data[Dp_len-1]== 0)
			{
				bt_manager_avrcp_pause();
			}
			else
			{
				bt_manager_avrcp_play();
			}
			break;

		case TUYA_BLE_DP_ID_CHANGE_CONTROL:
			if (Dp_data->p_data[Dp_len-1]== 0)
			{
				bt_manager_avrcp_play_previous();
			}
			else
			{
				bt_manager_avrcp_play_next();
			}
			break;
			
		case TUYA_BLE_DP_ID_EQ_MODE:
			dae_index = Dp_data->p_data[Dp_len-1];
			//btmusic_multi_dae_tws_notify(dae_index);
			//btmusic_tws_send(BTSRV_TWS_SLAVE, TWS_EVENT_SYNC_BT_MUSIC_DAE, &dae_index, sizeof(dae_index), false);
			btmusic_multi_dae_adjust(dae_index);
			break;
	}
}



static int tuya_dp_data_handler(uint8_t *dp_data, uint16_t data_len)
{
	uint8_t Dp_id = dp_data[0];
	tuya_ble_dp_write_data_t tuya_ble_dp_write_data;
	int ret = 0;

	printf("tuya_dp_data_handler:%d\n", Dp_id);

	switch(Dp_id)
	{
		case TUYA_BLE_DP_ID_BROADCAST_MODE: //ÓïÒôÌáÊ¾Ä£Ê½
			tuya_set_low_latency_mode(dp_data[data_len-1]);
			break;
			
		case TUYA_BLE_DP_ID_SWITCH: 
			if (dp_data[data_len-1] == 0) //¹Ø»ú
			{
				tuya_ctrl_power_off();
			}
			break;
			
		case TUYA_BLE_DP_ID_VOLUME_SET: 
			ret = tuya_set_music_play_volume(dp_data[data_len-1]);
			break;

		case TUYA_BLE_DP_ID_PLAY_PAUSE:
		case TUYA_BLE_DP_ID_CHANGE_CONTROL:
		case TUYA_BLE_DP_ID_EQ_MODE:
			tuya_ble_dp_write_data.data_len = data_len;
			tuya_ble_dp_write_data.p_data = dp_data;
			bt_music_tuya_ble_cmd_deal(&tuya_ble_dp_write_data);
			//app_message_post_async(APP_ID_BT_MUSIC, MSG_TUYA_BLE_CMD, (void *)&tuya_ble_dp_write_data, sizeof(tuya_ble_dp_write_data_t)); 		   
			break;

		case TUYA_BLE_DP_ID_DEVICE_FIND:
			if (dp_data[data_len-1] != 0)
			{
				tuya_find_device_start_notify();
			}
			else
			{				 
				tuya_find_device_stop_notify();
			}
			break;

		case TUYA_BLE_DP_ID_COUNTDOWN:
			{

			}
			break;

		default:
			break;
	}

	return ret;
}

int tuya_get_report_data_by_Dp_id(tuya_ble_dp_id_e Dp_id, uint8_t *offset)
{
	int len = 0;
	//audio_manager_configs_t*  cfg = audio_channel_get_config();

	offset[len++] = Dp_id;

	if (Dp_id == TUYA_BLE_DP_ID_BROADCAST_MODE)
	{
		bool_t	low_latency_mode = audio_get_low_latency_mode();
		offset[len++] = DT_ENUM;
		offset[len++] = 0x0;
		offset[len++] = DT_ENUM_LEN;
		offset[len++] = (u8_t)low_latency_mode;
	}
	else if (Dp_id == TUYA_BLE_DP_ID_VOLUME_SET)
	{
		offset[len++] = DT_VALUE;
		offset[len++] = 0x0;
		offset[len++] = DT_VALUE_LEN;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0;//(system_volume_get(AUDIO_STREAM_MUSIC)*100)/CFG_MAX_BT_MUSIC_VOLUME;
	}
	else if (Dp_id == TUYA_BLE_DP_ID_LEFTEAR_BATTERY_PERCENTAGE)
	{
		//u8_t  dev_channel = (cfg->hw_sel_dev_channel | cfg->sw_sel_dev_channel);
		u8_t  dev_channel = 0;
		u8_t bat_level;

		offset[len++] = DT_VALUE;
		offset[len++] = 0x0;
		offset[len++] = DT_VALUE_LEN;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0x0;


		//if (dev_channel == AUDIO_DEVICE_CHANNEL_L)
		if (dev_channel == 0)
		{
			bat_level = get_battery_percent();
			if (bat_level > 100)
				bat_level = 100;
			offset[len++] = bat_level;
			
		}
		else
		{
			if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
			{
				//bat_level = power_manager->slave_cap;
				if (bat_level > 100)
					bat_level = 100;
				offset[len++] = bat_level;
			}
			else
			{
				offset[len++] = 0x0;
			}
		}
		TUYA_APP_LOG_INFO("left_channel_battery: %d", offset[len-1]);

	}
	else if (Dp_id == TUYA_BLE_DP_ID_RIGHTEAR_BATTERY_PERCENTAGE)
	{
		u8_t  dev_channel = 0;//(cfg->hw_sel_dev_channel | cfg->sw_sel_dev_channel);

		u8_t bat_level;

		offset[len++] = DT_VALUE;
		offset[len++] = 0x0;
		offset[len++] = DT_VALUE_LEN;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0x0;


		//if (dev_channel == AUDIO_DEVICE_CHANNEL_R)
		if (dev_channel == 0)
		{
			bat_level = get_battery_percent();
			if (bat_level > 100)
				bat_level = 100;
			offset[len++] = bat_level;
		}
		else
		{
			if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
			{
				//bat_level = power_manager->slave_cap;
				if (bat_level > 100)
					bat_level = 100;
				offset[len++] = bat_level;
				
			}
			else
			{
				offset[len++] = 0x0;
			}
		}

		TUYA_APP_LOG_INFO("right_channel_battery: %d", offset[len-1]);
	}
	else if (Dp_id == TUYA_BLE_DP_ID_PLAY_PAUSE)
	{
		offset[len++] = DT_BOOL;
		offset[len++] = 0x0;
		offset[len++] = DT_BOOL_LEN;

		//if (bt_manager_a2dp_get_status() == BT_STATUS_PLAYING)
		if (1)
		{
			offset[len++] = 0x1;
		}
		else
		{			 
			offset[len++] = 0x0;
		}
	}
	else if (Dp_id == TUYA_BLE_DP_ID_EQ_MODE)
	{
		u8_t dae_index = 0;
		
		offset[len++] = DT_ENUM;
		offset[len++] = 0x0;
		offset[len++] = DT_ENUM_LEN;

		if (property_get(BTMUSIC_MULTI_DAE, &dae_index) < 0)
		{
			offset[len++] = 0x0;
		}
		else
		{
			offset[len++] = dae_index;
		}
	}
	else if (Dp_id == TUYA_BLE_DP_ID_DEVICE_FIND)
	{
		offset[len++] = DT_BOOL;
		offset[len++] = 0x0;
		offset[len++] = DT_BOOL_LEN;
		offset[len++] = 0x0;
	}
	else if (Dp_id == TUYA_BLE_DP_ID_SWITCH)
	{
		offset[len++] = DT_BOOL;
		offset[len++] = 0x0;
		offset[len++] = DT_BOOL_LEN;
		offset[len++] = 0x1;
	}
	else if (Dp_id == TUYA_BLE_DP_ID_COUNTDOWN)
	{
		offset[len++] = DT_VALUE;
		offset[len++] = 0x0;
		offset[len++] = DT_VALUE_LEN;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0x0;
		offset[len++] = 0;
	}
	
	return len;
}

int tuya_dp_report_data_process(u8_t Dp_id)
{
	int len;
	u8_t data_buf[10];

	memset(data_buf, 0 ,10);
	len = tuya_get_report_data_by_Dp_id(Dp_id, data_buf);
	print_hex("dpp:",data_buf,len);
	tuya_ble_dp_data_send(device_sn++, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, data_buf, len);

	return 0;
}

//static uint32_t time_stamp = 1587795793;
static void tuya_cb_handler(tuya_ble_cb_evt_param_t* event)
{
	int16_t result = 0;
	int i;
	tuya_ble_speech_state_t speech_state;
	printf("tuya_cb_handler:%d\n", event->evt);

	switch (event->evt)
	{
	case TUYA_BLE_CB_EVT_CONNECTE_STATUS:
		TUYA_APP_LOG_INFO("received tuya ble conncet status update event,current connect status = %d",event->connect_status);
		#if 1
		if (event->connect_status == BONDING_CONN)
		{
			ap_status_set(AP_STATUS_AUTH);
			tuya_start_tone_play(TUYA_NET_CONFIG_READY);
			// 这里给bt manager发消息，播放提示音ful_alerts_notification_03_16KHZ_配网成功
		}
		#endif
		break;	

	case TUYA_BLE_CB_EVT_DP_DATA_RECEIVED:
		dp_data_len = event->dp_received_data.data_len;
		memset(dp_data_array,0,TUYA_DP_ARRAY_LENGTH);
		memcpy(dp_data_array,event->dp_received_data.p_data,dp_data_len);  
		TUYA_APP_LOG_HEXDUMP_DEBUG("received dp write data :",dp_data_array,dp_data_len);
		//sn = 0;

		tuya_dp_data_handler(dp_data_array, dp_data_len);
		tuya_ble_dp_data_send(device_sn++, DP_SEND_TYPE_PASSIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, dp_data_array,dp_data_len);//1

		break;
	case TUYA_BLE_CB_EVT_DP_DATA_REPORT_RESPONSE:
		TUYA_APP_LOG_INFO("received dp data report response result code =%d",event->dp_response_data.status);

		break;
	case TUYA_BLE_CB_EVT_DP_DATA_WTTH_TIME_REPORT_RESPONSE:
		TUYA_APP_LOG_INFO("received dp data report response result code =%d",event->dp_response_data.status);
		break;
		case TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_REPORT_RESPONSE:
		TUYA_APP_LOG_INFO("received dp data with flag report response sn = %d , flag = %d , result code =%d",event->dp_with_flag_response_data.sn,event->dp_with_flag_response_data.mode
	,event->dp_with_flag_response_data.status);

		break;
	case TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_AND_TIME_REPORT_RESPONSE:
		TUYA_APP_LOG_INFO("received dp data with flag and time report response sn = %d , flag = %d , result code =%d",event->dp_with_flag_and_time_response_data.sn,
	event->dp_with_flag_and_time_response_data.mode,event->dp_with_flag_and_time_response_data.status);
	   
		break;
	case TUYA_BLE_CB_EVT_UNBOUND:
		
		TUYA_APP_LOG_INFO("received unbound req");

		break;
	case TUYA_BLE_CB_EVT_ANOMALY_UNBOUND:
		
		TUYA_APP_LOG_INFO("received anomaly unbound req");

		break;
	case TUYA_BLE_CB_EVT_DEVICE_RESET:
		
		TUYA_APP_LOG_INFO("received device reset req");

		break;
	case TUYA_BLE_CB_EVT_DP_QUERY:
		TUYA_APP_LOG_INFO("received TUYA_BLE_CB_EVT_DP_QUERY event");
		memset(dp_data_array,0,TUYA_DP_ARRAY_LENGTH);
		dp_data_len = 0;

		print_hex("dp_query_data:",event->dp_query_data.p_data,event->dp_query_data.data_len);
		
		for (i = 0; i < event->dp_query_data.data_len; i++)
		{
			dp_data_len += tuya_get_report_data_by_Dp_id(event->dp_query_data.p_data[i], \
			  &dp_data_array[dp_data_len]);
		}
		
		print_hex("[DATA]", dp_data_array, dp_data_len);
		if(dp_data_len>0)
			tuya_ble_dp_data_send(device_sn++,DP_SEND_TYPE_PASSIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITHOUT_RESPONSE, dp_data_array,dp_data_len);

		break;
	case TUYA_BLE_CB_EVT_OTA_DATA:
		//tuya_ble_ota_response(tuya_ble_ota_response_t *p_data)
		//print_hex("[otarx]", event->ota_data.p_data,event->ota_data.data_len);
		//TUYA_APP_LOG_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_OTA_DATA :",event->ota_data.p_data,event->ota_data.data_len);
#if 1//def TUYA_ENABLE_OTA
		tuya_ota_proc(event->ota_data.type,event->ota_data.p_data,event->ota_data.data_len);
#endif
		break;
	case TUYA_BLE_CB_EVT_NETWORK_INFO:
		TUYA_APP_LOG_INFO("received net info : %s",event->network_data.p_data);
		tuya_ble_net_config_response(result);
		break;
	case TUYA_BLE_CB_EVT_WIFI_SSID:
		TUYA_APP_LOG_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_WIFI_SSID :",event->wifi_info_data.p_data,event->timestamp_with_dst_data.data_len);
		break;
	case TUYA_BLE_CB_EVT_TIME_STAMP:
		TUYA_APP_LOG_INFO("received unix timestamp : %s ,time_zone : %d",event->timestamp_data.timestamp_string,event->timestamp_data.time_zone);
		break;
	case TUYA_BLE_CB_EVT_TIME_NORMAL:

		break;
	case TUYA_BLE_CB_EVT_DATA_PASSTHROUGH:
		TUYA_APP_LOG_HEXDUMP_DEBUG("received ble passthrough data :",event->ble_passthrough_data.p_data,event->ble_passthrough_data.data_len);
		tuya_ble_data_passthrough(event->ble_passthrough_data.p_data,event->ble_passthrough_data.data_len);
		break;
	case TUYA_BLE_CB_EVT_APP_LOCAL_TIME_NORMAL:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_APP_LOCAL_TIME_NORMAL");
		break;
	case TUYA_BLE_CB_EVT_TIME_STAMP_WITH_DST:
		TUYA_APP_LOG_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_TIME_STAMP_WITH_DST :",event->timestamp_with_dst_data.p_data,event->timestamp_with_dst_data.data_len);
		break;
	case TUYA_BLE_CB_EVT_DP_DATA_SEND_RESPONSE:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_DP_DATA_SEND_RESPONSE  %d %d",event->dp_send_response_data.type,event->dp_send_response_data.status);
		break;
	case TUYA_BLE_CB_EVT_DP_DATA_WITH_TIME_SEND_RESPONSE:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_DP_DATA_WITH_TIME_SEND_RESPONSE	%d %d",event->dp_with_time_send_response_data.type,event->dp_with_time_send_response_data.status);
		break;
	case TUYA_BLE_CB_EVT_WEATHER_DATA_REQ_RESPONSE:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_WEATHER_DATA_REQ_RESPONSE %d",event->weather_req_response_data.status);
		break;
	case TUYA_BLE_CB_EVT_UNBIND_RESET_RESPONSE:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_UNBIND_RESET_RESPONSE %d %d",event->reset_response_data.type,event->reset_response_data.status);
		break;
	case TUYA_BLE_CB_EVT_WEATHER_DATA_RECEIVED:
		TUYA_APP_LOG_HEXDUMP_DEBUG("TUYA_BLE_CB_EVT_WEATHER_DATA_RECEIVED :",event->weather_received_data.p_data,event->ble_passthrough_data.data_len);
		break;
	case TUYA_BLE_CB_EVT_BULK_DATA:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_BULK_DATA ");
		break;
	case TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID:
		TUYA_APP_LOG_INFO("TUYA_BLE_CB_EVT_UPDATE_LOGIN_KEY_VID  %d %d:",event->device_login_key_vid_data.login_key_len,event->device_login_key_vid_data.vid_len);
		break;

	case TUYA_BLE_CB_EVT_AVS_SPEECH_STATE:
		speech_state = event->speech_state_data.state;
		TUYA_APP_LOG_DEBUG("speech_state_data.state %d",speech_state);
		if(speech_state==SpeechState_IDLE)
		{
			//tuya_va_resource_release();
			TUYA_APP_LOG_DEBUG("Received Speech State : SpeechState_IDLE");
		}
		else if(speech_state==SpeechState_LISTENING)
		{
			TUYA_APP_LOG_DEBUG("Received Speech State : SpeechState_LISTENING");
		}
		else if(speech_state==SpeechState_PROCESSING)
		{
			TUYA_APP_LOG_DEBUG("%d",__LINE__);
			tuya_va_resource_release();
			//TUYA_APP_LOG_DEBUG("1Received Speech State :SpeechState_PROCESSING");
		}
		else if(speech_state==SpeechState_SPEAKING)
		{
			//tuya_va_resource_release();
			TUYA_APP_LOG_DEBUG("Received Speech State : SpeechState_SPEAKING");
		}
		else if(speech_state==SpeechState_Result_Data)
		{
			TUYA_APP_LOG_HEXDUMP_DEBUG("Received Speech Result data : ",event->speech_state_data.p_data,event->speech_state_data.data_length);
		}
		else
		{
		}
		break;
	case TUYA_BLE_CB_EVT_AVS_SPEECH_CMD_RES:
		switch (event->avs_cmd_event_data.cmd)
		{
			case AVS_SPEECH_CMD: //avs command from app
				if(event->avs_cmd_event_data.cmd_data.type==AVS_SPEECH_CMD_ENDPOINT)
				{
					if (0 == tuya_restart_va_status_get())
					{
						tuya_va_resource_release();
					}
					tuya_restart_va_status_set(0);

					TUYA_APP_LOG_DEBUG("Received speech endpoint cmd.");
				}
				else if(event->avs_cmd_event_data.cmd_data.type==AVS_SPEECH_CMD_STOP)
				{
					tuya_va_resource_release();
					TUYA_APP_LOG_DEBUG("Received speech stop cmd.");
				}
				else
				{
					TUYA_APP_LOG_DEBUG("Received speech unknown cmd.");
				}
				break;
			case AVS_SPEECH_CMD_RESPONSE: //avs command response from app
				if(event->avs_cmd_event_data.res_data.type ==AVS_SPEECH_CMD_RES_START)
				{
					
					if(event->avs_cmd_event_data.res_data.status==0)
					{
						if (AP_STATUS_AUTH == ap_status_get())
							tuya_va_resource_alloc();

						TUYA_APP_LOG_DEBUG("Start speech succeed.");
					}
				}
				else if(event->avs_cmd_event_data.res_data.type==AVS_SPEECH_CMD_RES_STOP)
				{
					if(event->avs_cmd_event_data.res_data.status==0)
					{
						TUYA_APP_LOG_DEBUG("Stop speech succeed.");
					}
				}
				else
				{
					TUYA_APP_LOG_DEBUG("Received speech unknown cmd.");
				}
				break;
			default:
				break;
		}
		break;

	case TUYA_BLE_CB_EVT_AVS_SPEECH_WEATHER_DATA:
		TUYA_APP_LOG_HEXDUMP_DEBUG("Weather data -> MainTitle :",
			event->avs_weather_data.p_weather_data->p_main_title,
			event->avs_weather_data.p_weather_data->main_title_length);
		break;
		
	case TUYA_BLE_CB_EVT_AVS_SPEECH_LIST_DATA:
		TUYA_APP_LOG_HEXDUMP_DEBUG("List data -> MainTitle :",
			event->avs_list_data.p_list_data->p_main_title,
			event->avs_list_data.p_list_data->main_title_length);
		break;

	case TUYA_BLE_CB_EVT_AVS_SPEECH_TIMER_SET_DATA:
		TUYA_APP_LOG_DEBUG("Received timer SET data : type = %d ,time = %d, loop count = %d ,loop count pause time = %d",
			event->avs_timer_set_data.p_timer_set_data->type,
			event->avs_timer_set_data.p_timer_set_data->time,
			event->avs_timer_set_data.p_timer_set_data->loop_count,
			event->avs_timer_set_data.p_timer_set_data->loop_count_pause_time);
		TUYA_APP_LOG_HEXDUMP_DEBUG("Timer token md5 :",
			event->avs_timer_set_data.p_timer_set_data->token_md5,16);
		TUYA_APP_LOG_HEXDUMP_DEBUG("Timer reminder data :",
			event->avs_timer_set_data.p_timer_set_data->p_reminder_text,
			event->avs_timer_set_data.p_timer_set_data->reminder_text_length);
		if (TUYA_VOS_TIMER_TIMER == event->avs_timer_set_data.p_timer_set_data->type)
		{
			tuya_start_tone_play(TUYA_VOS_TIMER_SET);
			// 这里给bt manager发消息，播放提示音ful_system_alerts_melodic_02_16KHZ_设置闹铃成功.act
		}
		else if (TUYA_VOS_TIMER_ALARM == event->avs_timer_set_data.p_timer_set_data->type)
		{
			tuya_start_tone_play(TUYA_VOS_ALARM_SET);
			// 这里给bt manager发消息，播放提示音ful_system_alerts_melodic_01_16KHZ_闹铃.act
		}
		else
		{
			tuya_start_tone_play(TUYA_VOS_ALEXA_TTS);
			// 这里给bt manager发消息，播放提示音med_ui_wakesound._TTH_16KHZ_MONO.act
		}
#if 0
		if (event->avs_timer_set_data.p_timer_set_data->type)
		{
			// 这里给bt manager发消息，播放闹铃/警告/提醒等TTS
		}
#endif

		break;

	case TUYA_BLE_CB_EVT_AVS_SPEECH_TIMER_CANCEL_DATA:
		TUYA_APP_LOG_DEBUG("Received timer CANCEL data : type = %d ",
			event->avs_timer_cancel_data.p_timer_cancel_data->type);
		TUYA_APP_LOG_HEXDUMP_DEBUG("Cancel Timer token md5 :",
			event->avs_timer_cancel_data.p_timer_cancel_data->token_md5,16);
		break;

	case TUYA_BLE_CB_EVT_AVS_NOTIFICATIONS_INDICATOR:
		if(event->avs_notifications_data.cmd == TUYA_AVS_NOTIFICATIONS_INDICATOR_SET)
		{
			TUYA_APP_LOG_DEBUG("Received TUYA_AVS_NOTIFICATIONS_INDICATOR_SET :persistVisualIndicator = %d , playAudioIndicator = %d .",
				event->avs_notifications_data.persist_visual_indicator,event->avs_notifications_data.play_audio_indicator);

			tuya_start_tone_play(TUYA_VOS_INCOMING_NOTIFY);
#if 0
			if (event->avs_notifications_data.play_audio_indicator)
			{
				// 这里给bt manager发消息，播放提示音ful_alerts_notification_03_16KHZ_配网成功
			}
#endif
			// 
		}
		else if(event->avs_notifications_data.cmd == TUYA_AVS_NOTIFICATIONS_INDICATOR_CLEAR)
		{
			TUYA_APP_LOG_DEBUG("Received TUYA_AVS_NOTIFICATIONS_INDICATOR_CLEAR");
		}
		else
		{
			TUYA_APP_LOG_DEBUG("Received AVS notifications UNKOWN cmd.");
		}
		break;

	default:
		TUYA_APP_LOG_WARNING("app_tuya_cb_queue msg: unknown event type 0x%04x",event->evt);
		break;
	}
	tuya_ble_event_response(event);
}

extern tuya_ble_parameters_settings_t *tuya_ble_current_para;

void myChartoByte(u8_t *dest, char *src, u8_t size)
{
	u8_t i, j;

	for (j = 0, i=5; j < size*2; j++)
	{
		dest[i] <<= 4;
		
		if (src[j] >= '0' && src[j] <= '9')
		{
			dest[i] |= src[j] - '0';
		}
		else if (src[j] >= 'a' && src[j] <= 'f')
		{				 
			dest[i] |= src[j] - 'a';
			dest[i] += 10;
		}
		else if (src[j] >= 'A' && src[j] <= 'F')
		{				 
			dest[i] |= src[j] - 'A';				
			dest[i] += 10;
		}
		else
		{
			dest[i] |= 0xF;
		}

		if (j%2 == 0) 
			continue;
		
		//SYS_LOG_INF("%d 0x%x\n", i, dest[i]);

		i--;
	}
}

void tuya_ble_app_init(void)
{
	int    size = 128;//SIZE(CFG_Struct_Usr_Reserved_Data, String);
	char *s;
	char *mac_str, *auth_key, *device_id;
	tuya_ble_device_param_t *device_param; 

	// add by zdx
	if (!tuya_ble_current_para)
	{
		tuya_ble_current_para = (tuya_ble_parameters_settings_t *)tuya_ble_malloc(sizeof(tuya_ble_parameters_settings_t));
	}
	else
	{
		SYS_LOG_WRN("");
	}

	if (!tuya_ble_current_para)
		printf("tuya_ble_current_para malloc failed\n");

	if (!dp_data_array)
	{
		dp_data_array = tuya_ble_malloc(TUYA_DP_ARRAY_LENGTH);
	}
	else
	{
		SYS_LOG_WRN("");
	}	
	

	device_param = (tuya_ble_device_param_t *)tuya_ble_malloc(sizeof(tuya_ble_device_param_t));	
	device_param->device_id_len = 16;	 //If use the license stored by the SDK,initialized to 0, Otherwise 16 or 20.	
	if(device_param->device_id_len==16)
	{

		s = (char *)tuya_ble_malloc(size);

		//app_config_read
		//(
		//	CFG_ID_USR_RESERVED_DATA,
		//	s,
		//	offsetof(CFG_Struct_Usr_Reserved_Data, String),
		//	size
		//);
		SYS_LOG_INF("size %d.",size);
			
		//memcpy(device_param->auth_key,(void *)auth_key_test,AUTH_KEY_LEN);
		//memcpy(device_param->device_id,(void *)device_id_test,DEVICE_ID_LEN);
		//memcpy(device_param->mac_addr.addr,mac_test,6);
		auth_key = s;
		device_id = s + AUTH_KEY_LEN;
		mac_str = s + AUTH_KEY_LEN + DEVICE_ID_LEN;

		myChartoByte(device_param->mac_addr.addr, mac_str, 6);
		memcpy(device_param->mac_addr_string, (void *)mac_str, MAC_STRING_LEN);
		memcpy(device_param->auth_key, (void *)auth_key, AUTH_KEY_LEN);
		memcpy(device_param->device_id,(void *)device_id,DEVICE_ID_LEN);
		printf("auth_key : %s\n", device_param->auth_key);		
		printf("device_id : %s\n", device_param->device_id);	  
		printf("mac_addr : %02x:%02x:%02x:%02x:%02x:%02x\n", device_param->mac_addr.addr[5], device_param->mac_addr.addr[4], device_param->mac_addr.addr[3],
		device_param->mac_addr.addr[2], device_param->mac_addr.addr[1], device_param->mac_addr.addr[0] );				 
		device_param->mac_addr.addr_type = TUYA_BLE_ADDRESS_TYPE_PUBLIC;

		tuya_ble_free(s);

	}	

	device_param->p_type = TUYA_BLE_PRODUCT_ID_TYPE_PID;
	device_param->product_id_len = 8;
	memcpy(device_param->product_id,APP_PRODUCT_ID,8);
	device_param->firmware_version = TY_APP_VER_NUM;
	device_param->hardware_version = TY_HARD_VER_NUM;

	device_param->use_ext_license_key = 1;


	tuya_ble_sdk_init(device_param);
	//tuya_ble_service_init(); 
	tuya_ble_callback_queue_register(tuya_cb_handler);	
	tuya_ble_free(device_param);
	
	tuya_vos_init();

	//TUYA_APP_LOG_INFO("demo project version : "TUYA_BLE_DEMO_VERSION_STR);
	//TUYA_APP_LOG_INFO("app version : "TY_APP_VER_STR);
	//printf("APP_PRODUCT_ID : %s", APP_PRODUCT_ID);	  
	//printf("auth_key : %s", auth_key_test);	   
	//printf("device_id : %s", device_id_test); 	 
	//printf("mac_addr : %02x:%02x:%02x:%02x:%02x:%02x", mac_test[5], mac_test[4], mac_test[3],
	//	mac_test[2], mac_test[1], mac_test[0] );	  

}

void tuya_ble_unbond(void)
{
	tuya_ble_device_unbond();
}

void  tuya_nv_sync(u8_t* buf, u16_t len)
{
	uint32_t addr;
	memcpy(&addr ,buf, sizeof(uint32_t));
	
	//print_hex("tuya_nv_sync:", buf, len);
	tuya_ble_nv_write(addr, buf + sizeof(uint32_t), len);

	tuya_ble_storage_load_settings();
}

void  tuya_role_switch_access(u8_t* buf, u32_t len)
{
	print_hex("tuya_role_switch_access:", buf, len);
	tuys_ble_tws_role_switch_info_t *cmd_buf = (tuys_ble_tws_role_switch_info_t *)buf;
	
	tuya_ble_nv_write(TUYA_BLE_SYS_FLASH_ADDR, (uint8_t *)&cmd_buf->sys_settings, sizeof(tuya_ble_sys_settings_t));
	tuya_ble_nv_write(TUYA_BLE_SYS_FLASH_BACKUP_ADDR, (uint8_t *)&cmd_buf->sys_settings, sizeof(tuya_ble_sys_settings_t));
	
	memcpy(tuya_ble_pair_rand, cmd_buf->tuya_ble_pair_rand, 6);
	
	tuya_ble_connect_status_set(cmd_buf->tuya_ble_connect_status);
	
	tuya_ble_storage_load_settings();
}


void tuya_dev_record_start(void)
{
	if (ap_status_get() == AP_STATUS_RECORDING)
	{
		SYS_LOG_INF("%d.",__LINE__);
		tuya_upload_speech_restart_request();
		return;
	}

	if (ap_status_get() >= AP_STATUS_AUTH)
	{
		SYS_LOG_INF("%d.",__LINE__);
		tuya_upload_speech_request();
	}
}

void tuya_dev_record_stop(void)
{
	tuya_upload_speech_stop_request(0);
}

int tuya_ble_service_on_connected(void)
{
	SYS_LOG_INF("%d.",__LINE__);
	tuya_ble_connected_handler();
	return 0;
}

int tuya_ble_service_on_disconnected(void)
{
	SYS_LOG_INF("%d.",__LINE__);
	tuya_ble_disconnected_handler();

	return 0;
}

void tuya_resource_alloc(void)
{
	tuya_ble_service_on_connected();
}

void tuya_resource_release(void)
{
	tuya_va_resource_release();
	tuya_ota_resource_release();
	tuya_ble_service_on_disconnected();
}

#endif



