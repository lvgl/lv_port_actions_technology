/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA application interface
 */

#ifndef __OTA_PRODUCT_APP_H__
#define __OTA_PRODUCT_APP_H__

#include <kernel.h>
#include <string.h>
#include <device.h>
#include <drivers/flash.h>
#include <soc.h>
#include <fw_version.h>
#include <partition/partition.h>
#include <mem_manager.h>
#include <crc.h>
#include <ota_upgrade.h>
#include <ota_backend.h>
#include <ota_backend_sdcard.h>
#include <ota_trans.h>
#include <ota_trans_bt.h>
#include "ota_image.h"
#include <ota_storage.h>
#include "ota_manifest.h"
#include "ota_breakpoint.h"
#include "ota_file_patch.h"
#include <os_common_api.h>
#include <drivers/nvram_config.h>
#include <sys/ring_buffer.h>
#include <thread_timer.h>
#include <msg_manager.h>
#include "../bt_transmit/bt_transmit.h"

#ifndef TRUE
#define  TRUE	1
#endif

#ifndef FALSE
#define  FALSE	0
#endif

#define PRODUCT_OTA_SDAP_RETRY_MAX	3

#define PRODUCT_KEY_TYPE_ALL  0xFF

enum PRODUCT_KEY_TYPE
{
	PRODUCT_KEY_TYPE_NONE				= 0x0001,	
	PRODUCT_KEY_TYPE_OTA				= 0x0002,  
	PRODUCT_KEY_TYPE_EFUSE				= 0x0004,  
	PRODUCT_KEY_TYPE_CFO				= 0x0008,	
	PRODUCT_KEY_TYPE_CHECK				= 0x0010,  
};

enum OTA_PRODUCT_ITEM
{
	PRODUCT_ITEM_OTA = 0,	  
	PRODUCT_ITEM_EFUSE, 	  
	PRODUCT_ITEM_CFO,
	PRODUCT_ITEM_CHECK,   
	PRODUCT_ITEM_MAX,	 
};

enum OTA_PRODUCT_STATUS
{
	OTA_PRODUCT_STATUS_NULL 			= 0x0000,
	OTA_PRODUCT_STATUS_INIT 			= 0x0001,
	OTA_PRODUCT_STATUS_WAIT_CONNECT 	 = 0x0002, 
	OTA_PRODUCT_STATUS_INQUIRE			 = 0x0004,
	OTA_PRODUCT_STATUS_INQUIRE_WAIT 	  = 0x0008,    
	OTA_PRODUCT_STATUS_CONNECTING		  = 0x0010, 	   
	OTA_PRODUCT_STATUS_CONNECTED			 = 0x0020,	  
	OTA_PRODUCT_STATUS_REQUEST_UPGRADE	   = 0x0040,   
	OTA_PRODUCT_STATUS_CONNECT_NEGOTIATION = 0x0080,
	OTA_PRODUCT_STATUS_NEGOTIATION_RESULT  = 0x0100,   
	OTA_PRODUCT_STATUS_REQUIRE_IMAGE_DATA  = 0x0200,   
	OTA_PRODUCT_STATUS_WAIT_REMOTE_CMD		= 0x0400,	 
	OTA_PRODUCT_STATUS_VALIDATE_REPORT		= 0x0800,	 
	OTA_PRODUCT_STATUS_FIREWARE_NEW 		= 0x1000,		
	OTA_PRODUCT_STATUS_WAIT_DISCONNECT		= 0x4000,		   
	OTA_PRODUCT_STATUS_SPP_TIMEOUT		   = 0x8000,	 
  
};

typedef struct
{
	u8_t ota_product_inquire_rssi;
	u8_t ota_product_upgrading;
	u8_t reserved[2];
	u32_t ota_product_total_num;
}ota_state_info_t;

typedef struct
{
	u8_t valid;  
	u8_t rssi;
	u8_t bd[6];	
	u8_t name[30];  
	u8_t reserved[2];
} ota_inquire_info_t;

struct ota_product_app_t
{
	u16_t ota_product_state;
	u16_t ota_product_update_check_ok		: 1;	  
	u16_t ota_product_inquire_finish	   : 1;  
	u16_t ota_product_sdap_retry		   : 1; 		
	u16_t ota_product_connecting			 : 1;	   
	u16_t ota_product_request_upgrade_wait	 : 1;
	u16_t ota_product_connect_negotiation_wait	 : 1;  
	u16_t ota_product_negotiation_result_wait	: 1;   
	u16_t ota_product_validate_report			 : 1;	  
	u16_t ota_product_validate_report_wait		: 1;		 
	u16_t ota_product_auto_inquire				  : 1;	  
	u16_t ota_product_update_inquire_rssi		: 1;	   
	

	u16_t ota_product_spp_timeout_set;
	u16_t ota_product_spp_timeout_cn;
	u16_t ota_product_image_percent;
	u16_t ota_product_inquire_inteval;		  
	
	u8_t image_valid; 
	u8_t product_item;
	u8_t product_key_type;	  
	u8_t running_state;
	u8_t update_screen;
	u8_t product_ota_ui_wait;
	u8_t product_ota_sdap_retry_cn;

	u32_t product_ota_upgrade_status_code;
	
	struct ota_trans_image_info image_info;   
	struct ota_trans_image_head_info head_info;
	struct thread_timer ota_product_timer;
	os_delayed_work ota_product_tx_timeout_work;
	ota_inquire_info_t remote_ota_dev;	 
	ota_state_info_t ota_state;
};



int ota_product_global_init(void);
int ota_product_init_sdcard(void);
int ota_product_deinit_sdcard(void);

int ota_product_init_trans(void);
void ota_product_input_event_msg_proc(struct app_msg *msg);
uint32_t ota_product_get_status(void);
bool ota_product_key_event_handle(int key_event, int event_stage);
struct ota_product_app_t *ota_product_get_app(void);
int ota_product_connect_device(u8_t *bd, u8_t rssi);
void ota_product_exit_ota(void);
void ota_product_ota_start(void);
void ota_product_ota_stop(void);

#endif /* __OTA_APP_H__ */
