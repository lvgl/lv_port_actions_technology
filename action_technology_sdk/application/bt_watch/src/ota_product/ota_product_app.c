/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ota_product_app.h"
#include <thread_timer.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <sys_wakelock.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>
#include <ota_backend_bt.h>
#include <ota_storage.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <drivers/flash.h>
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#ifdef CONFIG_FILE_SYSTEM
#include <fs_manager.h>
#include <fs/fs.h>
#endif
#include <soc_pstore.h>
#include <partition/partition.h>
#include <sdfs.h>
#include "../launcher/clock_selector/clock_selector.h"
#include "app_ui.h"
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
#include <../bt_manager/ble_master/conn_master_manager.h>
#define BT_OTA_TRANS_INQUIRY_NAME "LE_OTATRANS"
#else
#define BT_OTA_TRANS_INQUIRY_NAME "OTATRANS"
#endif

#define BT_OTA_TRANS_INQUIRY_RSSI (-60)

/* ble mater event */
enum {
/*
	BLE_EVENT_NULL                         = 0,
	BLE_SCAN_FAIL_IND                      = 1,
	BLE_CREAT_CON_FAIL_IND                 = 2,
	BLE_CON_FAIL_IND                       = 3,
	BLE_CON_DISCONECTED_IND                = 4,
	BLE_CON_CONECTED_IND                   = 5,
	BLE_CHARACTERISTIC_DISCOVER_COMPLETE   = 6,
	BLE_PRIMARY_DISCOVER_COMPLETE          = 7,
	BLE_CCC_DISCOVER_COMPLETE              = 8,
	BLE_PROCESS_CLOSE_SCAN_IND             = 9,
*/
	BLE_MATSER_SCAN_START                  = 0x80,
	BLE_MATSER_SCAN_STOP                   = 0x81,
};

struct ble_ltv_data {
	u8_t length;
	u8_t  type;
	char  value[29];
} __packed;

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
void le_master_env_init(void);
void le_master_event_handle(uint8_t event_code, void *event_data);
struct le_scan_param le_ota_scan_cb;
#endif

extern char *itoa(int value, char *string, int radix);

static struct ota_host_upgrade_info *g_ota_host;

#ifdef CONFIG_OTA_BACKEND_SDCARD
static struct ota_backend *backend_sdcard = NULL;
#endif

static struct ota_product_app_t *p_ota_product_app;

static struct ota_trans *trans_bt;

/* "00006666-0000-1000-8000-00805F9B34FB"  ota uuid spp */
static const uint8_t ota_product_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x66, 0x66, 0x00, 0x00};

struct ota_product_app_t *ota_product_get_app(void)
{
	return p_ota_product_app;
}

void ota_product_get_config(void)
{
#if 0
	char str[8];
	int nvram_config_len;
	struct ota_product_app_t *ota = ota_product_get_app();
	
	memset(str,0,8);

	nvram_config_len = nvram_config_get(CFG_BT_INQUIRE_RSSI, str, 8);
	if(nvram_config_len > 0){
		ota->ota_state.ota_product_inquire_rssi = atoi(str);
	}
	else{
		ota->ota_state.ota_product_inquire_rssi = CONFIG_OTA_INQUIRE_RSSI;
	}

	SYS_LOG_INF("%d",(int8)ota->ota_state.ota_product_inquire_rssi);
#endif
}

void ota_product_set_config(void)
{
#if 0
	char temp[8];
	struct ota_product_app_t *ota = ota_product_get_app();
	
	memset(temp,0,8);
	if(itoa(ota->ota_state.ota_product_inquire_rssi,temp, 10) != NULL)
	{
		nvram_config_set(CFG_BT_INQUIRE_RSSI, temp, strlen(temp));
	}
#endif
}

void ota_update_image_info(struct ota_trans_image_info * image_info)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();

//   ota_product_app->image_unit_offset = image_info->image_offset;
//	  ota_product_app->image_unit_len = image_info->read_len;
	memcpy(&ota_product_app->image_info,image_info,sizeof(struct ota_trans_image_info));
	ota_product_app->image_valid = 1;

//	  SYS_LOG_INF("%d %d",ota_product_app->image_info.image_offset,ota_product_app->image_info.read_len);
}

void ota_update_sdap_complete(uint16 result)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();

	SYS_LOG_INF("%d ",result); 
	if(result != 0){
		if(ota_product_app->product_ota_sdap_retry_cn < PRODUCT_OTA_SDAP_RETRY_MAX){
			ota_product_app->product_ota_sdap_retry_cn ++;
			ota_product_app->ota_product_sdap_retry = TRUE; 		
			ota_product_app->ota_product_connecting = FALSE;
		}
		else{
			 ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_CONNECT;
			 ota_product_app->update_screen = 1;			 
		}
	}	 
}

bool ota_product_spp_timeout_check(void)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();

	ota_product_app->ota_product_spp_timeout_cn++;	  
	if(ota_product_app->ota_product_spp_timeout_cn >= ota_product_app->ota_product_spp_timeout_set){
		return true;
	}
	else{
		return false;
	}
}

void ota_product_spp_timeout_set(u16_t timeout)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();

	ota_product_app->ota_product_spp_timeout_set = timeout;
	ota_product_app->ota_product_spp_timeout_cn = 0; 
}

u16_t ota_product_get_image_precent(u32_t offset)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	u16_t image_percent = ota_product_app->ota_product_image_percent;
	u32_t image_size = 0;;
	u16_t i;
	image_size = image_percent;
	
	for(i = 0 ; i < 100; i++){
		if(offset > image_size){
			image_size += image_percent;
		}
		else{
			break;
		}
	}

	if( i == 100)
		return i;
	else
		return i + 1;
}

int ota_product_backend_check(u8_t state)
{
	
	SYS_LOG_INF("card state %d\n",state);	 
	SYS_LOG_INF("ota_product_backend_check");

	if(state == 1)
	{
		if(p_ota_product_app->ota_product_update_check_ok == 1){
			return 0;
		}

		if(ota_product_init_sdcard()!= 0){
			p_ota_product_app->ota_product_update_check_ok = 0;
			SYS_LOG_ERR("sd card err\n");	 
			return -1;
		}
	
		if(ota_host_upgrade_check(g_ota_host) != 0){
			//ota_product_deinit_sdcard();
			p_ota_product_app->ota_product_update_check_ok = 0; 		   
			SYS_LOG_ERR("upgrade_check err\n");    
			return -1;	  
		}
		
		p_ota_product_app->ota_product_update_check_ok = 1;

		p_ota_product_app->ota_product_image_percent = ota_host_upgrade_get_image_size(g_ota_host) / 100;
		p_ota_product_app->head_info.head_crc = ota_host_upgrade_get_head_crc(g_ota_host);
		p_ota_product_app->head_info.version = ota_host_upgrade_get_image_ver(g_ota_host);
		
		SYS_LOG_INF("ver:%x crc:%x",p_ota_product_app->head_info.version,p_ota_product_app->head_info.head_crc);


	}
	else{
		//ota_product_deinit_sdcard();
		ota_product_exit_ota();
		
		p_ota_product_app->ota_product_update_check_ok = 0;
		ota_host_update_state(g_ota_host,OTA_INIT); 	
		
	}

	p_ota_product_app->update_screen = 1;

	return 0;
}

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
static bt_addr_le_t inquiry_addr_le;
void ble_transmit_scan_start_notify(void)
{
	char *current_app = app_manager_get_current_app();
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_BLE_OTA_MASTER_EVENT;
	new_msg.cmd = BLE_MATSER_SCAN_START;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}

void ble_transmit_scan_stop_notify(void)
{
	char *current_app = app_manager_get_current_app();
	struct app_msg new_msg = { 0 };

	new_msg.type = MSG_BLE_OTA_MASTER_EVENT;
	new_msg.cmd = BLE_MATSER_SCAN_STOP;

	if (current_app) {
		send_async_msg(current_app, &new_msg);
	}
}

static void _ble_ota_list_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];
	int i = 0;
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	struct ble_ltv_data ltv;
	int res_len;
	u8_t *data;

	if (!ota_product_app) {
		SYS_LOG_WRN("ota_product_app null.");
		return;
	}

	if (OTA_PRODUCT_STATUS_INQUIRE_WAIT != ota_product_app->ota_product_state) {
		SYS_LOG_WRN("STATUS ERROR.", ota_product_app->ota_product_state);
		return;
	}
	SYS_LOG_INF("ota_product_state %d.",ota_product_app->ota_product_state);

	bt_addr_le_to_str(addr, dev, sizeof(dev));

	if (BLE_CONN_CONNECTED == conn_master_get_state_by_addr(addr)) {
		SYS_LOG_INF("Ignore connected device (%s)", dev);
		return;
	}

	if ((ad->len > 31) || (ad->len < 3)) {
		SYS_LOG_INF("ad->len (%d)", ad->len);
		return;
	}

	SYS_LOG_INF("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %d", dev, type, ad->len, rssi);
	if (rssi < BT_OTA_TRANS_INQUIRY_RSSI) {
		return;
	}

	res_len = ad->len;
	data = ad->data;
	do {
		memset(ltv.value, 0, sizeof(ltv.value));
		ltv.length = data[i++];
		if ((ltv.length < 2) || (ltv.length + 1 > res_len)) {
			SYS_LOG_INF("ltv.length (%d) res_len (%d).", ltv.length, res_len);
			return;
		}

		ltv.type = ad->data[i++];
		memcpy(ltv.value, &ad->data[i], (ltv.length-1));
		ad->data += (ltv.length - 1);
		res_len -= (ltv.length + 1);
		SYS_LOG_INF("ltv length (%d) type (%d) value (%s).", ltv.length, ltv.type, ltv.value);

		if ((0x09 == ltv.type) &&
			(0 == strncmp(ltv.value, BT_OTA_TRANS_INQUIRY_NAME, strlen(BT_OTA_TRANS_INQUIRY_NAME)))) {
				memcpy(&inquiry_addr_le, addr, sizeof(bt_addr_le_t));
				ota_product_app->ota_product_inquire_finish = 1;
				ble_transmit_scan_stop_notify();
				return;
		}
	} while (res_len > 3);

	//ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
}
#else
static bd_address_t inquiry_addr;
static void _bt_ota_list_info_update(struct bt_device_info_t *bt_info, uint8_t count)
{
	if (0 == count) {
		SYS_LOG_WRN("inquiry null.");
		return;
	}

	struct bt_device_info_t *info = bt_info;
	int i;
	struct ota_product_app_t *ota_product_app = ota_product_get_app();

	if (!ota_product_app) {
		SYS_LOG_WRN("ota_product_app null.");
		return;
	}	

	if (OTA_PRODUCT_STATUS_INQUIRE_WAIT != ota_product_app->ota_product_state) {
		SYS_LOG_WRN("STATUS ERROR.", ota_product_app->ota_product_state);
		return;
	}

	SYS_LOG_INF("ota_product_state %d.",ota_product_app->ota_product_state);

	for (i = 0; i < count; i++) {
		SYS_LOG_INF("name %s, paired %d, connected %d, rssi %d.",
			info[i].name, info[i].paired, info[i].connected, info[i].rssi);
		if ((0 == strncmp(info[i].name, BT_OTA_TRANS_INQUIRY_NAME, strlen(BT_OTA_TRANS_INQUIRY_NAME))) &&
			//(0 == info->paired) && 
			(info->rssi > BT_OTA_TRANS_INQUIRY_RSSI) &&
			(0 == info[i].connected)
			) {
				memcpy(&inquiry_addr, &info[i].addr, sizeof(bd_address_t));
				ota_product_app->ota_product_inquire_finish = 1;
				bt_transmit_inquiry_stop_notify();
				return;
			}
	}

	//ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
}

void ota_product_ota_spp_connect(void)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	SYS_LOG_INF("ota_product_state %d.",ota_product_app->ota_product_state);

	if (OTA_PRODUCT_STATUS_CONNECTING != ota_product_app->ota_product_state) {
		return;
	}

	int bt_trans_ota_connect(bd_address_t *bd, uint8_t *uuid);
	int ota_product_channel;
	ota_product_channel = bt_trans_ota_connect(&inquiry_addr, (uint8_t *)ota_product_spp_uuid);
	if (ota_product_channel == 0) {
		SYS_LOG_INF("Failed to do spp connect\n");
		ota_product_exit_ota();
		ota_product_app->ota_state.ota_product_upgrading = 1;
		//ota_product_ota_start();
	} else {
		SYS_LOG_INF("SPP connect channel %d\n", ota_product_channel);
	}
}
#endif

void ota_product_ota_loop_deal(void)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	SYS_LOG_INF("ota_product_state %d.",ota_product_app->ota_product_state);

	if(ota_product_app->running_state == 0) {
		SYS_LOG_INF("exit.");
		return;
	}
	if(ota_product_app->product_ota_ui_wait > 0){
		ota_product_app->product_ota_ui_wait--;
	}
	
	switch(ota_product_app->ota_product_state)
	{
		case OTA_PRODUCT_STATUS_INIT:
		if(ota_product_app->ota_product_update_check_ok == 0) {
			if(ota_product_app->update_screen == 1){
				ota_product_app->update_screen = 0;
				//sys_notify_ui_content(UI_EVENT_OTA_INIT_FAIL,NULL,0);
			}
			break;
		} else {
			ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_CONNECT;
			ota_product_app->update_screen = 1;
			ota_product_app->ota_product_inquire_inteval = 0;
		}		 
		break;

		case OTA_PRODUCT_STATUS_WAIT_CONNECT: 
		if(ota_product_app->ota_product_update_check_ok == 0){
			ota_product_app->update_screen = 1;
			ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INIT;
		} else{
			//ota_product_ota_start();
			if((ota_product_app->ota_product_auto_inquire == TRUE) && (ota_product_app->ota_state.ota_product_upgrading == 1)){
				ota_product_app->ota_product_inquire_inteval++;
				if(ota_product_app->ota_product_inquire_inteval >= 20/*CONFIG_OTA_INQUIRE_INTERVAL*/){
					ota_product_app->ota_product_inquire_inteval = 0;
					ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
					ota_product_app->update_screen = 1;
				}
			}
		}
		if((ota_product_app->update_screen == 1) && (ota_product_app->product_ota_ui_wait == 0)){
			ota_product_app->update_screen = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_INIT_SUCCESS,(u8_t *)&ota_product_app->ota_state,sizeof(ota_state_info_t));
		}		 
		break;
		
		case OTA_PRODUCT_STATUS_INQUIRE:
#ifdef CONFIG_OTA_BLE_MASTER_SUPPORT
		ble_transmit_scan_start_notify();	
#else
		bt_transmit_inquiry_start_notify(_bt_ota_list_info_update);
#endif
		ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE_WAIT;
		ota_product_app->ota_product_inquire_finish = 0;
		ota_product_spp_timeout_set(100);
		//if(bt_manager_ota_inquiry_dev(ota_product_app->ota_state.ota_product_inquire_rssi) == 0){
		//	  ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE_WAIT;
		//	  ota_product_app->ota_product_inquire_finish = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_INQUIRE_RUNNING,(uint8_t *)(&ota_product_app->ota_state.ota_product_total_num),sizeof(u32_t)); 	 
		//} 
		if(ota_product_app->ota_product_update_inquire_rssi == 1){
		   ota_product_app->ota_product_update_inquire_rssi = 0;
		   ota_product_set_config();
		}
		break;

		case OTA_PRODUCT_STATUS_INQUIRE_WAIT:
		if(ota_product_app->ota_product_inquire_finish == 1) {
			//if(ota_product_app->remote_ota_dev.valid == 1){ 
				if(ota_product_app->ota_product_auto_inquire == TRUE){
					ota_product_app->remote_ota_dev.reserved[0] = 1;
					ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_CONNECTING;
					ota_product_app->product_ota_sdap_retry_cn = 0;
					ota_product_app->ota_product_sdap_retry = TRUE;
					ota_product_spp_timeout_set(120);
				}
				else{
					ota_product_app->remote_ota_dev.reserved[0] = 0;
				}
				
				if(ota_product_app->update_screen == 1){
					ota_product_app->update_screen = 0; 							 
					//sys_notify_ui_content(UI_EVENT_OTA_INQUIRE_RESULT,(uint8_t *)(&ota_product_app->remote_ota_dev),sizeof(ota_inquire_info_t));
				}
				
			//}
			//else{
			//	ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_CONNECT;  
			//}
		} else {
				if(ota_product_spp_timeout_check() == true) {
					ota_product_app->ota_product_request_upgrade_wait = 0;
					ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
				}
		}
		break;

		case OTA_PRODUCT_STATUS_CONNECTING:
		if(ota_product_app->update_screen == 1){
			ota_product_app->update_screen = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_CONNECT_RUNNING,NULL,0);	
		}	

		if(ota_product_app->ota_product_sdap_retry == TRUE){
			ota_product_app->ota_product_sdap_retry = FALSE;
			// ota_product_connect_device(ota_product_app->remote_ota_dev.bd,ota_product_app->remote_ota_dev.rssi);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
		le_master_dev_connect(&inquiry_addr_le);
#else
		bt_manager_br_connect(&inquiry_addr);
#endif
		}

		if(ota_product_spp_timeout_check() == true) {
			ota_product_app->ota_product_request_upgrade_wait = 0;
			ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
		}
		break;
		
		case OTA_PRODUCT_STATUS_CONNECTED:
		if(ota_product_app->update_screen == 1){
			ota_product_app->update_screen = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_NEGOTIATION,NULL,0);
		
		}		 
		ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_REQUEST_UPGRADE;
		ota_product_app->ota_product_request_upgrade_wait = 0;
		ota_product_app->ota_product_connect_negotiation_wait = 0;
		ota_product_app->ota_product_negotiation_result_wait = 0;
		break;

		case OTA_PRODUCT_STATUS_REQUEST_UPGRADE:
		if(ota_product_app->ota_product_request_upgrade_wait == 1)
		{
			if(ota_product_spp_timeout_check() == true){
				ota_product_app->ota_product_request_upgrade_wait = 0;
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
			}
		}
		else
		{
			if(ota_trans_ioctl(trans_bt,OTA_TRANS_IOCTL_REQUEST_UPGRADE,&ota_product_app->head_info)) {
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
				break;
			}
			ota_product_app->ota_product_request_upgrade_wait = 1;
			ota_product_spp_timeout_set(100);
		}
		break;

		case OTA_PRODUCT_STATUS_CONNECT_NEGOTIATION:
		if(ota_product_app->ota_product_connect_negotiation_wait == 1){
			if(ota_product_spp_timeout_check() == true){
				ota_product_app->ota_product_connect_negotiation_wait = 0;
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
			}			 
		}
		else
		{
			if (ota_trans_ioctl(trans_bt,OTA_TRANS_IOCTL_CONNECT_NEGOTIATION,NULL)) {
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
				break;
			}
			ota_product_app->ota_product_connect_negotiation_wait = 1;
			ota_product_spp_timeout_set(100);  
			
		}
		break;
	   
		case OTA_PRODUCT_STATUS_NEGOTIATION_RESULT:
		if( ota_product_app->ota_product_negotiation_result_wait == 1)
		{
			if(ota_product_spp_timeout_check() == true){
				ota_product_app->ota_product_negotiation_result_wait = 0;
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT; 
			}
		}
		else
		{
			if (ota_trans_ioctl(trans_bt,OTA_TRANS_IOCTL_NEGOTIATION_RESULT,NULL)) {
				ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT;
				break;
			}
			ota_product_app->ota_product_negotiation_result_wait = 1;  
			ota_product_app->ota_product_validate_report_wait = 0;
			ota_product_spp_timeout_set(100);
		}
		break;

		case OTA_PRODUCT_STATUS_REQUIRE_IMAGE_DATA:
		os_delayed_work_submit(&ota_product_app->ota_product_tx_timeout_work, (20*1000));
		ota_host_upgrade_send_image(g_ota_host,ota_product_app->image_info.image_offset,ota_product_app->image_info.read_len);
		ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD;

		os_delayed_work_cancel(&ota_product_app->ota_product_tx_timeout_work);

		ota_product_app->image_info.image_percent = ota_product_get_image_precent(ota_product_app->image_info.image_offset);
		if(ota_product_app->update_screen == 1){
			ota_product_app->update_screen = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_IMAGE_INFO,(uint8_t *)(&ota_product_app->image_info),sizeof(struct ota_trans_image_info ));			
		}
		
		ota_product_spp_timeout_set(500);
		break;

		case OTA_PRODUCT_STATUS_VALIDATE_REPORT:
		 if(ota_product_app->update_screen == 1){
			ota_product_app->update_screen = 0;   
			if(ota_product_app->ota_product_validate_report == TRUE){
				if(ota_product_app->ota_product_validate_report_wait == 1){
					ota_product_app->ota_state.ota_product_total_num++;
					SYS_LOG_INF("ota_product_total_num %d.", ota_product_app->ota_state.ota_product_total_num);
					ota_product_app->ota_product_validate_report_wait = 0;
				}
				//sys_notify_ui_content(UI_EVENT_OTA_UPGRADE_SUCCESS,(uint8_t *)(&ota_product_app->ota_state.ota_product_total_num),sizeof(u32_t)); 		   
			}
			else{
				//sys_notify_ui_content(UI_EVENT_OTA_UPGRADE_FAIL,(uint8_t *)(&ota_product_app->ota_state.ota_product_total_num),sizeof(u32_t));						
			}			 
		} 
		ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD;
		ota_product_spp_timeout_set(500);
		break;

		case OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD:
		if(ota_product_spp_timeout_check() == true){
			SYS_LOG_ERR("OTA_PRODUCT_WAIT_REMOTE_CMD_TIMEOUT");
			ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_SPP_TIMEOUT; 
		}			
		break;

		case OTA_PRODUCT_STATUS_FIREWARE_NEW:
		ota_product_app->image_info.image_percent = 100;
		if(ota_product_app->update_screen == 1){
			ota_product_app->update_screen = 0;
			//sys_notify_ui_content(UI_EVENT_OTA_IMAGE_INFO,(uint8_t *)(&ota_product_app->image_info),sizeof(struct ota_trans_image_info ));			
		} 
		
//		  bt_manager_ota_disconnect();	
		ota_product_app->product_ota_ui_wait = 100;
		ota_product_spp_timeout_set(100);
		ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD;
		break;
		
		case OTA_PRODUCT_STATUS_SPP_TIMEOUT:
		SYS_LOG_ERR("OTA_PRODUCT_STATUS_SPP_TIMEOUT");
		//bt_manager_ota_disconnect();
		//btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
		//ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_DISCONNECT;
		ota_product_exit_ota();
		ota_product_app->ota_state.ota_product_upgrading = 1;
		break;

		case OTA_PRODUCT_STATUS_WAIT_DISCONNECT:
		//	btif_br_get_connected_device_num();
		//if(bt_manager_get_connect_dev_num() == 0){
			ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_CONNECT;
		//}
		break;
		
		default:
		break;
	}
	
}

void ota_product_loop(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	
	if(ota_product_app == NULL)
		return;

	//SYS_LOG_INF("ota_product_loop");

	ota_product_ota_loop_deal();
}

void ota_product_exit_ota(void)
{
	struct ota_product_app_t *ota_product_app = ota_product_get_app();
	u16_t state = ota_product_app->ota_product_state;
	
	SYS_LOG_INF("state %d",state);
#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	ble_transmit_scan_stop_notify();
	le_master_dev_disconnect(&inquiry_addr_le);
#else
	bt_transmit_inquiry_stop_notify();
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);
#endif

	ota_product_app->ota_product_request_upgrade_wait = 0;
	ota_product_app->ota_product_connect_negotiation_wait = 0;
	ota_product_app->ota_product_negotiation_result_wait = 0;
	ota_product_app->ota_product_inquire_finish = 0;
	ota_product_app->ota_product_sdap_retry = 0;
	ota_product_app->ota_product_connecting = 0;
	ota_product_app->ota_product_validate_report = 0;
	ota_product_app->ota_state.ota_product_upgrading = 0;
	ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INIT;
	ota_product_app->update_screen = 1;
}

static void ota_product_tx_timeout_work_callback(struct k_work *work)
{
	ota_product_exit_ota();
	// p_ota_product_app->ota_state.ota_product_upgrading = 1;

	SYS_LOG_ERR("ota transmit exec, AUTO reboot.");
	sys_pm_reboot(REBOOT_REASON_SYSTEM_EXCEPTION);
}

static int ota_product_init(void)
{
	int result = 0;
	p_ota_product_app = app_mem_malloc(sizeof(struct ota_product_app_t));
	if (!p_ota_product_app) {
		SYS_LOG_ERR("ota malloc fail!");
		result = -ENOMEM;
		return result;
	}

	memset(p_ota_product_app,0,sizeof(struct ota_product_app_t));

	p_ota_product_app->product_item = PRODUCT_ITEM_OTA;
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_NULL;
	//p_ota_product_app->running_state = 0;
	p_ota_product_app->running_state = 1;
	p_ota_product_app->product_key_type = PRODUCT_KEY_TYPE_NONE;
	p_ota_product_app->ota_product_inquire_inteval = 0;
#if 1//def CONFIG_OTA_AUTO_INQUIRE	  
	p_ota_product_app->ota_product_auto_inquire = 1;
#else
	p_ota_product_app->ota_product_auto_inquire = 0;
#endif
	p_ota_product_app->ota_state.ota_product_upgrading = 0;

	p_ota_product_app->ota_state.ota_product_total_num = 0;
	p_ota_product_app->product_ota_ui_wait = 0;
	
	ota_product_get_config();
	
	ota_product_global_init();	

	ota_product_init_trans();
	SYS_LOG_INF("ota_product_backend_check");

	if (0 == ota_product_backend_check(1)) {
		p_ota_product_app->running_state = 1;
		p_ota_product_app->update_screen = 1;
		p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INIT;
		p_ota_product_app->product_key_type = PRODUCT_KEY_TYPE_OTA;
	}

	thread_timer_init(&p_ota_product_app->ota_product_timer,
		ota_product_loop, NULL);

	thread_timer_start(&p_ota_product_app->ota_product_timer, 100, 100);

	os_delayed_work_init(&p_ota_product_app->ota_product_tx_timeout_work, ota_product_tx_timeout_work_callback);

	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);

	return result;
}
#if 0
void ota_product_exit(void)
{
	SYS_LOG_INF("%s %d", __func__, __LINE__);

	if (!p_ota_product_app) goto exit;


	if(thread_timer_is_running(&p_ota_product_app->ota_product_timer))
	{
		thread_timer_stop(&p_ota_product_app->ota_product_timer);
	}

	ota_trans_exit(trans_bt);
	
	ota_backend_sdcard_exit(backend_sdcard); 

	backend_sdcard = NULL;

	trans_bt = NULL;

	ota_host_upgrade_exit(g_ota_host);

	app_mem_free(p_ota_product_app);
	p_ota_product_app = NULL;	
	
exit: 
	app_thread_exit(APP_ID_OTA_PRODUCT);	
}
#endif
static void ota_product_backend_callback(struct ota_backend *backend, int cmd, int state)
{
	int err;

	SYS_LOG_INF("backend %p cmd %d state %d", backend, cmd, state);

	if (cmd == OTA_BACKEND_UPGRADE_STATE) {
		if (state == 1) {
			err = ota_host_upgrade_attach_backend(g_ota_host, backend);
			if (err) {
				SYS_LOG_INF("unable attach backend");
				return;
			}
		} else {
			ota_host_upgrade_detach_backend(g_ota_host, backend);
		}
	} else if (cmd == OTA_BACKEND_UPGRADE_PROGRESS) {
#ifdef CONFIG_UI_MANAGER
		//ota_view_show_upgrade_progress(state);
#endif
	}
}


#ifdef CONFIG_OTA_BACKEND_SDCARD
//ruct ota_backend_sdcard_init_param ota_sdcard_init_param = {
//fpath = "/SD:/product.bin",
//
#define SDCARD_OTA_FILE				CONFIG_APP_FAT_DISK"/product.bin"

struct ota_backend_sdcard_init_param ota_sdcard_init_param = {
	.fpath = SDCARD_OTA_FILE,
};

void ota_product_trans_callback(struct ota_trans*trans, int state, void *param)
{

	SYS_LOG_INF("%d",state);
	uint8 validate;	 

	switch(state){

	case OTA_TRANS_SDAP_RESULT:
	ota_update_sdap_complete(*((uint16 *)param));
	break;
	
	case OTA_TRANS_CONNECTED:
	ota_trans_open(trans);
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_CONNECTED;
	ota_host_upgrade_attach_trans(g_ota_host,trans);    
	p_ota_product_app->ota_product_connecting = 0;
	p_ota_product_app->update_screen = 1;	   
	break;

	case OTA_TRANS_DISCONNECT:
	ota_trans_close(trans);
	ota_host_upgrade_attach_trans(g_ota_host,NULL);
	//p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_CONNECT;
	//p_ota_product_app->ota_product_negotiation_result_wait = 0;
	//p_ota_product_app->ota_product_request_upgrade_wait = 0;
	//p_ota_product_app->ota_product_connect_negotiation_wait = 0;
	//p_ota_product_app->ota_product_connecting = 0;
	//p_ota_product_app->update_screen = 1;
	ota_product_exit_ota();
	p_ota_product_app->ota_state.ota_product_upgrading = 1;
	break;
	
	case OTA_TRANS_REQUEST_UPGRADE_ACK:
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_CONNECT_NEGOTIATION;	  
	p_ota_product_app->ota_product_request_upgrade_wait = 0;

	break;

	case OTA_TRANS_CONNECT_NEGOTIATION_ACK:
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_NEGOTIATION_RESULT;
	p_ota_product_app->ota_product_connect_negotiation_wait = 0; 	  
	break;

	case OTA_TRANS_NEGOTIATION_RESULT_ACK:
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD; 
	p_ota_product_app->ota_product_negotiation_result_wait = 0;	 
	break;

	case OTA_TRANS_REQUEST_IMAGE_DATA:
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_REQUIRE_IMAGE_DATA;
	ota_update_image_info((struct ota_trans_image_info *)param);
	if(p_ota_product_app->ota_product_negotiation_result_wait == 1){
		p_ota_product_app->ota_product_negotiation_result_wait = 0;
	}
	p_ota_product_app->update_screen = 1;
	p_ota_product_app->ota_product_validate_report_wait = 1;
	break;

	case OTA_TRANS_VALIDATE_REPORT:
	p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_VALIDATE_REPORT;  
	validate = *(u8_t *)param;
	if(validate == 1){
		p_ota_product_app->ota_product_validate_report = TRUE;
	}
	else{
		p_ota_product_app->ota_product_validate_report = FALSE;		 
	}

	if(p_ota_product_app->ota_product_validate_report_wait == 1){
		p_ota_product_app->update_screen = 1;
		SYS_LOG_INF("VALID REPROT");
	}
	break;

	case OTA_TRANS_UPGRADE_STATUS_NOTIFY:
	p_ota_product_app->product_ota_upgrade_status_code = *((uint32 *)param);
	if(p_ota_product_app->product_ota_upgrade_status_code == 0x10){
		SYS_LOG_INF("FW ALREADY UPGRADE!");
		p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_FIREWARE_NEW;
		p_ota_product_app->update_screen = 1;
	}
	break;
	
	default:
	break;
   }
   
}

int ota_product_init_sdcard(void)
{
	backend_sdcard = ota_backend_sdcard_init(ota_product_backend_callback, &ota_sdcard_init_param);
	if (!backend_sdcard) {
		SYS_LOG_INF("failed to init sdcard ota");
		return -ENODEV;
	}
	return 0;
}

#if 0
int ota_product_deinit_sdcard(void)
{
	ota_backend_sdcard_exit(backend_sdcard);
	backend_sdcard = NULL;
	return 0;
}
#endif
#endif

#if 1//def CONFIG_OTA_TRANS_BLUETOOTH



const static struct ota_trans_bt_init_param bt_trans_init_param = {
	.spp_uuid = ota_product_spp_uuid,
};

int ota_product_init_trans(void)
{
	SYS_LOG_INF("");
	
	trans_bt = ota_trans_bt_init(ota_product_trans_callback,
		(struct ota_trans_bt_init_param *)&bt_trans_init_param);
	if (!trans_bt) {  
		SYS_LOG_INF("failed to init ota trans");
		return -ENODEV;
	}
	return 0;
}
#if 0
int ota_product_connect_device(u8_t *bd, u8_t rssi)
{
	if (!trans_bt) {  
		SYS_LOG_INF("failed to init ota trans");
		return -ENODEV;
	}

	return bt_manager_ota_connect_service((u8_t *)ota_product_spp_uuid,bd);	
}
#endif
#endif

void ota_product_notify(int state, int old_state)
{
	SYS_LOG_INF("ota state: %d->%d", old_state, state);
}

#define CONFIG_XSPI_NOR_ACTS_DEV_NAME "spi_flash"

int ota_product_global_init(void)
{
	struct ota_upgrade_param param;

	SYS_LOG_INF("ota app init");

	memset(&param, 0x0, sizeof(struct ota_upgrade_param));

	param.storage_name = CONFIG_XSPI_NOR_ACTS_DEV_NAME;
	param.notify = ota_product_notify;

	if(g_ota_host != NULL)
	{
		SYS_LOG_INF("g_ota not NULL!");
		ota_host_upgrade_exit(g_ota_host);
		g_ota_host = NULL;
	}

	g_ota_host = ota_host_upgrade_init(&param);
	if (!g_ota_host) {
		SYS_LOG_ERR("ota app init error!");
		return -1;
	}

	return 0;
}

#if 0
void ota_product_bt_event_msg_proc(void* msg_data)
{
	bt_manager_event_msg_t*  msg = msg_data;
	bt_inquire_dev_t *ota_dev;
	
#ifdef CONFIG_TRACE_APP_EVENT
	SYS_LOG_INF("event: %s", bteg_evt2str(msg->event));
#else
	SYS_LOG_INF("event: %d", msg->event);
#endif

	switch (msg->event)
	{
		case BT_WAIT_PAIR_EVENT:
			break; 
			
		case BT_OTA_INQUIRE_COMPLETE_EVENT:
			ota_dev = (bt_inquire_dev_t *)msg->param;
			print_hex("INQUIRE",ota_dev,sizeof(bt_inquire_dev_t));
			if(ota_dev->valid == 1){
				memcpy(&ota_product_get_app()->remote_ota_dev,ota_dev,sizeof(bt_inquire_dev_t));
	//			ota_product_connect_device(ota_dev->bd_addr,ota_dev->rssi);
			}
			ota_product_get_app()->ota_product_inquire_finish = 1;
			
			ota_product_get_app()->update_screen = 1;
			
			break;
			
		case BT_OTA_INQUIRE_START_EVENT:
			memset(&ota_product_get_app()->remote_ota_dev,0,sizeof(bt_inquire_dev_t));
			break;		
			
		default:
			break;
	}
}
#endif

#ifndef	CONFIG_OTA_BLE_MASTER_SUPPORT
static void btota_pruduct_event_proc(struct app_msg *msg)
{
	SYS_LOG_INF("bt_product: bt cmd %d\n", msg->cmd);

	switch (msg->cmd) {
	case BT_TRS_INQUIRY_START_EVENT:
	{
		bt_transmit_inquiry_start(msg->ptr);
		break;
	}

	case BT_TRS_INQUIRY_RESTART_EVENT:
	{
		bt_transmit_inquiry_restart(msg->ptr);
		break;
	}

	case BT_TRS_INQUIRY_STOP_EVENT:
	{
		bt_transmit_inquiry_stop();
		break;
	}

	default:
		break;
	}
}
#endif

bool btota_product_key_event_proc(uint32_t event)
{
	struct ota_product_app_t *ota = ota_product_get_app();

	SYS_LOG_INF("msg.value %d ota_product_upgrading %d.",event,ota->ota_state.ota_product_upgrading);
	switch (event) {
	case (KEY_POWER | KEY_TYPE_SHORT_UP):
		/* stop or start */
		if (ota->ota_state.ota_product_upgrading == 1){
			ota_product_ota_stop();
		} else {
			ota_product_ota_start();
		}
		break;

	case (KEY_TBD | KEY_TYPE_LONG_DOWN):

		break;

	default:
		break;
	}

	return 0;
}

static void ota_product_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;
	uint16_t reboot_type = 0;
	uint8_t reason = 0;

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif
	ota_product_init();

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
	le_master_env_init();
#endif

	system_power_get_reboot_reason(&reboot_type, &reason);
	printk("reboot_type %d, reason %d.\n",reboot_type, reason);
	if ((reason == REBOOT_REASON_HCI_TIMEOUT) || (reason == REBOOT_REASON_SYSTEM_EXCEPTION)) {
		printk("exec reboot, continue ota.\n");
		//p_ota_product_app->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
		p_ota_product_app->ota_state.ota_product_upgrading = 1;
	}

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {

			case MSG_EXIT_APP:
				terminaltion = true;
				//ota_product_exit();
				break;

#ifdef	CONFIG_OTA_BLE_MASTER_SUPPORT
			case MSG_BLE_OTA_MASTER_EVENT:

				if (BLE_MATSER_SCAN_START == msg.cmd) {
					le_ota_scan_cb.cb = &_ble_ota_list_device_found;
					le_master_scan_start(&le_ota_scan_cb);
				} else if (BLE_MATSER_SCAN_STOP == msg.cmd) {
					le_master_scan_stop();
				} else {
					le_master_event_handle(msg.cmd, (void *)msg.value);
				}
				break;
#else
			case MSG_BT_EVENT:
				btota_pruduct_event_proc(&msg);
				break;
#endif

#if 0
			case MSG_KEY_INPUT:
				SYS_LOG_INF("msg.value %d.",msg.value);
				btota_product_key_event_proc(msg.value);
				break;

			case MSG_ABT_BT_ENGINE:
				ota_product_bt_event_msg_proc(msg.ptr);
				break;

			case MSG_INPUT_EVENT:
				ota_product_input_event_msg_proc(&msg);
				break;

			case MSG_APP_FUNC_CALL:
				{
					app_func_call_msg_t *msg_data = (app_func_call_msg_t *)msg.ptr;

					msg_data->func(msg_data->param1, msg_data->param2);

					break;
				}

			case MSG_SDCARD_INSERT:
				ota_product_backend_check(1);
				break;

			case MSG_SDCARD_DRAWOUT:
				ota_product_backend_check(0);
				break;				  
#endif
			default:
				SYS_LOG_ERR("unknown message type: 0x%x!", msg.type);
				break;
		}

		if (msg.callback != NULL)
			msg.callback(&msg, result, NULL);
		}

		if(!terminaltion){
			thread_timer_handle_expired();
		}
	}
}

APP_DEFINE(ota_product, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY, 
	FOREGROUND_APP, NULL, NULL,NULL, 
	ota_product_app_main, ota_product_key_event_handle);

