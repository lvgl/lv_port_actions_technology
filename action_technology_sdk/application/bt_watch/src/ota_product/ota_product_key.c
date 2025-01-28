#include "ota_product_app.h"
//#include <app_keymap.h>
//#include <app_message.h>

//extern const struct input_app_key_tbl ota_product_keymap[];

uint32_t ota_product_get_status(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();

	return ota->ota_product_state;
}

uint32_t ota_product_get_key_type(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();

	return ota->product_key_type;
}

void ota_product_item_change(u8_t direction)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t product_item = ota->product_item;
	
	if(ota->running_state != 0){
		return;
	}

	if(direction == 0){
		 if(product_item == PRODUCT_ITEM_OTA){
			 product_item = PRODUCT_ITEM_CHECK;
		 }
		 else{
			 product_item --;
		 }
	}
	else{
		if(product_item == PRODUCT_ITEM_CHECK){
			product_item = PRODUCT_ITEM_OTA;
		}
		else{
			product_item += 1;		  
		}
	} 

	ota->product_item = product_item; 
	//sys_notify_ui_content(UI_EVENT_OTA_MENU_UPDATE,&product_item,sizeof(uint8_t));
	
}

void ota_product_ota_deal(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t state = ota->running_state;

	if(state == 0){
		ota->running_state = 1;
		ota->update_screen = 1;
		ota->ota_product_state = OTA_PRODUCT_STATUS_INIT;
		ota->product_key_type = PRODUCT_KEY_TYPE_OTA;
	} else {
		ota->running_state = 0;
		ota->product_key_type = PRODUCT_KEY_TYPE_NONE;
		ota_product_exit_ota();
		//sys_notify_ui_content(UI_EVENT_OTA_MENU_UPDATE,&ota->product_item,sizeof(uint8_t));
	}
}

void ota_product_ota_start(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t ota_state	= ota->ota_product_state;	

	SYS_LOG_INF("%d",ota_state);						 
	if (ota->ota_state.ota_product_upgrading == 1){
		ota_product_exit_ota();
	}

	switch(ota_state){
		case OTA_PRODUCT_STATUS_WAIT_CONNECT:
		ota->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
		ota->ota_state.ota_product_upgrading = 1;
		break;
#if 0
		case OTA_PRODUCT_STATUS_INQUIRE_WAIT:
		if((ota->ota_product_inquire_finish == 1) && (ota->ota_product_connecting == 0)){		 
			if(ota->remote_ota_dev.valid == 1){
				ota_product_connect_device(ota->remote_ota_dev.bd,ota->remote_ota_dev.rssi);
				ota->ota_product_connecting = 1;
				ota->product_ota_sdap_retry_cn = 0;	
				ota->ota_product_sdap_retry = FALSE;
				ota->ota_product_state = OTA_PRODUCT_STATUS_CONNECTING;  
				ota->update_screen = 1;
				ota->ota_state.ota_product_upgrading = 1;
			}
			else{
				SYS_LOG_ERR(" %d %d",ota->ota_product_inquire_finish,ota->ota_product_connecting);				  
			}
		}
		else{
			SYS_LOG_INF("remote valid:%d",ota->remote_ota_dev.valid);
		}
		break;
#endif
		default:
		break;
	}
}

void ota_product_ota_stop(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t ota_state	= ota->ota_product_state;	

	SYS_LOG_INF("%d",ota_state);						 
	if ((OTA_PRODUCT_STATUS_CONNECTED > ota_state)// &&
		//(ota->ota_state.ota_product_upgrading == 1)
		) {
		ota_product_exit_ota();
	}
}

void ota_product_ota_inquire_next(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t ota_state	= ota->ota_product_state;	
	
	if(ota->ota_state.ota_product_upgrading == 0){
		if(ota->ota_state.ota_product_inquire_rssi < 255){
			ota->ota_state.ota_product_inquire_rssi++;
			ota->ota_product_update_inquire_rssi = 1;
			ota->update_screen = 1;
		}
	}
	else{
		switch(ota_state){
			case OTA_PRODUCT_STATUS_INQUIRE_WAIT:
			if((ota->ota_product_inquire_finish == 1) && (ota->ota_product_connecting == 0)){		 
				ota->ota_product_state = OTA_PRODUCT_STATUS_INQUIRE;
				ota->remote_ota_dev.valid = 0;
				memset(&ota->remote_ota_dev,0,sizeof(ota_inquire_info_t));
				ota->product_ota_sdap_retry_cn = 0;	
				ota->update_screen = 1;
			}
			break;
			
			default:
			break;
		}
	}
}

void ota_product_ota_inquire_prev(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	if(ota->ota_state.ota_product_upgrading == 1){
		return;
	}
	if(ota->ota_state.ota_product_inquire_rssi > 0){
		ota->ota_state.ota_product_inquire_rssi--;
		ota->ota_product_update_inquire_rssi = 1;
		ota->update_screen = 1; 	   
	}
	
}

void ota_product_item_select(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t item = ota->product_item;
	
	switch(item){
		case PRODUCT_ITEM_OTA:
			ota_product_ota_deal();
			break;

		default:			
			break;
		
	}
}

void ota_product_item_start(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();
	u8_t item = ota->product_item;

	
	switch(item){
		case PRODUCT_ITEM_OTA:
			ota_product_ota_start();
			break;

		default: 
			SYS_LOG_ERR("OTA ITEM ERROR %d",ota->product_item); 						
			break;
		
	}
}

void ota_product_ota_inquire_mode(void)
{
	struct ota_product_app_t *ota = ota_product_get_app();	 
	if(ota->ota_product_auto_inquire == TRUE){
		ota->ota_product_auto_inquire = FALSE;
	}
	else{
		ota->ota_product_auto_inquire = TRUE;	 
	}
}

#if 0
void ota_product_input_event_msg_proc(struct app_msg *msg)
{
	switch (msg->cmd) {
		case MSG_PRODUCT_PREV_ITEM:
			ota_product_item_change(0);
			break;

		case MSG_PRODUCT_NEXT_ITEM:
			ota_product_item_change(1); 		   
			break;

		case MSG_PRODUCT_SEL_ITEM:
			ota_product_item_select();						 
			break;

		case MSG_PRODUCT_OTA_START:
			ota_product_item_start();
			break;

		case MSG_PRODUCT_OTA_INQUIRE_NEXT:
			ota_product_ota_inquire_next();
			break;

		 case MSG_PRODUCT_OTA_INQUIRE_PREV:
			ota_product_ota_inquire_prev();
			break;			 

		case MSG_PRODUCT_OTA_INQUIRE_MODE:
			ota_product_ota_inquire_mode();
			break;
			
		default:
			break;
	}
}
#endif

bool ota_product_key_event_handle(int key_event, int event_stage)
{
	bool result = true;
#if 0
	if (event_stage == KEY_EVENT_PRE_DONE) {
		return false;
	}

	if (app_manager_get_apptid(APP_ID_OTA_PRODUCT) == NULL) {
		return false;
	}

	result = app_key_deal(app_manager_get_current_app(), key_event, ota_product_keymap, ota_product_get_key_type);
#endif
	return result;
}

