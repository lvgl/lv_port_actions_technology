/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <zephyr.h>
#include <net/buf.h>
#include <stdlib.h>
#include <msg_manager.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/host_interface.h>

//#include "app_defines.h"
#include "ble_ancs_ams.h"
#include "ancs_ams_disc.h"
#include "ancs_service_disc.h"
#include "ancs_service.h"
#include "ams_service_disc.h"
#include "ams_service.h"
#include "gap_service_disc.h"
#include "bt_manager_ble.h"
#include "../bt_manager_inner.h"
#include "ancs_ams_log.h"

#define ANCS_AMS_DELAY_DISCOVER_TIME	(2000)		/* 2s */

static struct ancs_ams_data_t ancs_ams_data;

/* send a message to main thread */
void ble_ancs_ams_send_msg_to_app(uint8_t event_code, int status)
{
	struct app_msg msg = {0};

	msg.type = MSG_BLE_ANCS_AMS_SERVICE;
	msg.cmd = ((ancs_ams_data.connect_index&0xF) << 4)|(event_code&0xF);
	msg.value = status;

	send_async_msg("main", &msg);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;

	if (err) {
		return;
	}

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	anam_log_inf("ANCS_AMS connnected\n");
	ancs_ams_data.state = app_connected;
	ancs_ams_data.pconn = conn;
	ancs_ams_data.connect_index++;
	ancs_ams_data.getting_pos = 0;

#if (!(defined(CONFIG_ACTIVE_REQ_BLE_ENC) || defined(CONFIG_CONNECT_ANCSAMS_AFTER_ENC)))
	/* send a message to main thread */
	ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_CONN_OPEN_IND, err);
#endif
}

static void ble_ancs_ams_disconnected(void)
{
	ancs_ams_data.state = app_disconnected;
	ancs_ams_data.ios_phone = false;
	ancs_ams_data.ancs_configured = false;
	ancs_ams_data.ams_configured = false;
	ancs_ams_data.gap_configured = false;
	ancs_ams_data.service_discovered = false;
	ancs_ams_data.ancs_attr_request_rd_index = 0;
	ancs_ams_data.ancs_attr_request_wr_index = 0;
	memset(&ancs_ams_data.ancs_attr_request_list, 0, sizeof(ancs_ams_data.ancs_attr_request_list));

	os_delayed_work_cancel(&ancs_ams_data.ble_ancs_ams_discover_work);
	ble_set_clear_wake_lock(BLE_WAKE_LOCK_ANCS_DISCOVER,0);

	/* clear ancs service state */
	ancs_init_attribute_data();
	ancs_service_data_init();
	ams_service_data_init();
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	anam_log_inf("ANCS_AMS disconnnected\n");
	ancs_ams_data.pconn = NULL;
	ble_ancs_ams_disconnected();
#if (!(defined(CONFIG_ACTIVE_REQ_BLE_ENC) || defined(CONFIG_CONNECT_ANCSAMS_AFTER_ENC)))
	bt_set_pairing_mode(false, true);
#endif
}

static void security_changed(struct bt_conn *conn, bt_security_t level,enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	if ((hostif_bt_conn_get_info(conn, &info) < 0) || (info.type != BT_CONN_TYPE_LE) || (info.role != BT_HCI_ROLE_SLAVE)) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	anam_log_inf("id:%d %s level %u err:%d\n", info.id,addr, level,err);
	if (!err) {
		/* send a message to main thread */
#if defined(CONFIG_ACTIVE_REQ_BLE_ENC) || defined(CONFIG_CONNECT_ANCSAMS_AFTER_ENC)
		if (level >= BT_SECURITY_L2) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_CONN_OPEN_IND, err);
		}
#else
		ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_CONN_SEC_OPEN_IND, level);
#endif
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

static void ble_ancs_ams_discover_work_callback(struct k_work *work)
{
	if (ancs_ams_data.pconn != NULL) {
		start_gatt_database_discovery(ancs_ams_data.pconn);
	}

	ble_set_clear_wake_lock(BLE_WAKE_LOCK_ANCS_DISCOVER,0);
}

void ble_ancs_ams_init(void)
{
	/* register conn callbacks into bt stack */
	hostif_bt_conn_cb_register(&conn_callbacks);

	/* Initialise ANCS application State
	 * Always the application state should be initialised before the
	 * GATT Database functions as GATT_ADD_DB_CFM gets invoked
	 * prior to executing the code after GattAddDatabaseReq()
	 */
    ancs_ams_data.state = app_init;
    ancs_ams_data.service_discovered = false;
	ancs_ams_data.ancs_configured = false;
	ancs_ams_data.ams_configured = false;
    ancs_ams_data.gap_configured = false;

    ancs_ams_data.ancs_attr_request_rd_index = 0;
    ancs_ams_data.ancs_attr_request_wr_index = 0;

    ancs_service_data_init();
    ams_service_data_init();

	os_delayed_work_init(&ancs_ams_data.ble_ancs_ams_discover_work, ble_ancs_ams_discover_work_callback);
    ble_set_clear_wake_lock(BLE_WAKE_LOCK_ANCS_DISCOVER,0);

	anam_log_inf("state:%d\n", ancs_ams_data.state);
}

static void ble_gatt_config_ancs(struct bt_conn *conn)
{
	/* Configure for ANCS Notification */
	configure_ancs_notifications(conn,true);
	/* Configure for ANCS Data Source Notification */
	configure_ancs_data_source_notification(conn,true);
}

#if 0
static void ble_gatt_deconfig_ancs(struct bt_conn *conn)
{
	/* Configure for ANCS Data Source Notification */
	configure_ancs_data_source_notification(conn,false);
	/* Configure for ANCS Notification */
	configure_ancs_notifications(conn,false);
}
#endif

static void ble_gatt_config_ams(struct bt_conn *conn)
{
	/* Configure for AMS Remote Command Notification */
	config_ams_remote_cmd_notifications(conn,true);
	/* Configure for AMS Entity Update Notification */
	config_ams_entity_update_notifications(conn,true);
}

#if 0
static void ble_gatt_deconfig_ams(struct bt_conn *conn)
{
	/* Configure for AMS Entity Update Notification */
	config_ams_entity_update_notifications(conn,false);
	/* Configure for AMS Remote Command Notification */
	config_ams_remote_cmd_notifications(conn,false);
}
#endif

static void ble_gatt_config_gap(struct bt_conn *conn)
{
	/* Configure for GAP Service Changed Notification */
	config_gap_service_changed_indicate(conn,true);
}

#if 0
static void ble_gatt_deconfig_gap(struct bt_conn *conn)
{
	/* Configure for GAP Service Changed Notification */
	config_gap_service_changed_indicate(conn,false);
}
#endif

static void ble_ancs_ams_notifications(void)
{
    /* Service discovery is complete. Set the boolean flag
    * remote_gatt_handles_present to true and store it in NVM
    */
    ancs_ams_data.remote_gatt_handles_present = true;
    
    /* Initiate configuring ANCS Notification Handle */
    if (get_ancs_notification_ccd_handle() != INVALID_ATT_HANDLE) {
        if (!ancs_ams_data.ancs_configured) {
            ancs_ams_data.ancs_configured = true;
            ble_gatt_config_ancs(ancs_ams_data.pconn);
        }
    }

    if (get_ams_remote_cmd_ccd_handle() != INVALID_ATT_HANDLE) {
        if (!ancs_ams_data.ams_configured) {
            ancs_ams_data.ams_configured = true;
            ble_gatt_config_ams(ancs_ams_data.pconn);
        }
    }

    if (get_gap_service_changed_ccd_handle() != INVALID_ATT_HANDLE) {
        if (!ancs_ams_data.gap_configured) {
            ancs_ams_data.gap_configured = true;
            ble_gatt_config_gap(ancs_ams_data.pconn);
        }
    }
	ancs_ams_data.state = app_configured;
}

void ble_ancs_write_notification_request_to_list(uint32_t notify_uuid)
{
    uint8_t i;
    uint8_t wr = ancs_ams_data.ancs_attr_request_wr_index;
    uint8_t success = 0;
    for(i = 0; i < BLE_ANCS_ATTR_REQ_MAX; i++){
        if(ancs_ams_data.ancs_attr_request_list[wr].valid == 0){
            ancs_ams_data.ancs_attr_request_list[wr].uuid = notify_uuid;
            ancs_ams_data.ancs_attr_request_list[wr].valid = 1;
            success = 1;
        }
        ancs_ams_data.ancs_attr_request_wr_index++;
        if(ancs_ams_data.ancs_attr_request_wr_index >= BLE_ANCS_ATTR_REQ_MAX){
            ancs_ams_data.ancs_attr_request_wr_index = 0;
        }

        if(success){
            break;
        }
    }
}

int ble_ancs_read_notification_request_from_list(uint32_t *notify_uuid)
{
    uint8_t i; 
    uint8_t rd = ancs_ams_data.ancs_attr_request_rd_index;
    uint8_t success = 0;
    for(i = 0; i < BLE_ANCS_ATTR_REQ_MAX; i++){
        if(ancs_ams_data.ancs_attr_request_list[rd].valid == 1){
            *notify_uuid = ancs_ams_data.ancs_attr_request_list[rd].uuid;
            ancs_ams_data.ancs_attr_request_list[rd].valid = 0;
            success = 1;
        }
        ancs_ams_data.ancs_attr_request_rd_index++;
        if(ancs_ams_data.ancs_attr_request_rd_index >= BLE_ANCS_ATTR_REQ_MAX){
            ancs_ams_data.ancs_attr_request_rd_index = 0;
        }

        if(success){
            return 0;
        }
    }
    return -1;
}

void ble_ancs_ams_event_handle(uint8_t event_code, void *event_data)
{
	int prio;

	prio = os_thread_priority_get(os_current_get());
	if (prio >= 0) {
		os_thread_priority_set(os_current_get(), -1);
	}

	anam_log_inf("ind %d code %d state %d\n",  (event_code >> 4), (event_code & 0xF), ancs_ams_data.state);
	if (!ancs_ams_data.pconn) {
		goto handle_exit;
	}

	if ((event_code >> 4) != ancs_ams_data.connect_index) {
		anam_log_warn("Drop diff ind %d %d\n", ancs_ams_data.connect_index, (event_code >> 4));
		goto handle_exit;
	}

	switch ((event_code & 0xF)) {
	case BLE_ANCS_AMS_CONN_OPEN_IND:
	{
		if (ancs_ams_data.state == app_connected) {
#if (!(defined(CONFIG_ACTIVE_REQ_BLE_ENC) || defined(CONFIG_CONNECT_ANCSAMS_AFTER_ENC)))
		    bt_set_pairing_mode(true, false);
#endif
	        if (ancs_ams_data.pconn != NULL) {
	            ble_set_clear_wake_lock(BLE_WAKE_LOCK_ANCS_DISCOVER, 1);
	            os_delayed_work_submit(&ancs_ams_data.ble_ancs_ams_discover_work, ANCS_AMS_DELAY_DISCOVER_TIME);
	        }
		}
	}
	break;

	case BLE_ANCS_AMS_CONN_SEC_OPEN_IND:
	{
        if ((ancs_ams_data.state != app_configured) && (ancs_ams_data.service_discovered)){
            ble_ancs_ams_notifications();
		}
	}
	break;

	case BLE_ANCS_AMS_DISC_PRIM_SERV_IND:
	{
	    if((int)event_data != BLE_SERVICE_GAP){
            if((int)event_data == BLE_SERVICE_ANCS_AMS){
                ancs_ams_data.ios_phone = true;
            }
            else{
                ancs_ams_data.ios_phone = false;
            }
#if (!(defined(CONFIG_ACTIVE_REQ_BLE_ENC) || defined(CONFIG_CONNECT_ANCSAMS_AFTER_ENC)))
            bt_set_pairing_mode(false, false);
#endif
            bt_manager_ble_set_phone(ancs_ams_data.ios_phone);
        }
		if (ancs_ams_data.pconn != NULL){
		    if((int)event_data)
			    handle_service_discovery_primary_service_by_uuid_cfm(ancs_ams_data.pconn, 0);
			else
			    handle_service_discovery_primary_service_by_uuid_cfm(ancs_ams_data.pconn, 1);
		}
	}
	break;

	case BLE_ANCS_AMS_DISC_SERVICE_CHAR_IND:
	{
		if (ancs_ams_data.pconn != NULL) {
			handle_gatt_discover_service_characteristic_cfm(ancs_ams_data.pconn, (int)event_data);
		}
	}
	break;

	case BLE_ANCS_AMS_DISC_SERVICE_CHAR_DESC_IND:
	{
		if (ancs_ams_data.pconn != NULL) {
			handle_gatt_characteristic_descriptor_cfm(ancs_ams_data.pconn, (int)event_data);
		}
	}
	break;

	case BLE_ANCS_AMS_DISCOVERY_COMPELTE_IND:
	{
        if ((bool)event_data == true) {
            ancs_ams_data.state = app_discovered;
            ancs_ams_data.service_discovered = true;
            if (hostif_bt_conn_get_security(ancs_ams_data.pconn) < BT_SECURITY_L2) {
                if (hostif_bt_conn_set_security(ancs_ams_data.pconn, BT_SECURITY_L2)) {
                    anam_log_err("Failed to set security\n");
                }
            } else {
			    ble_ancs_ams_notifications();
			}
        } else {
            if (ancs_ams_data.ios_phone) {
                start_gatt_database_discovery(ancs_ams_data.pconn);
            }
        }
	}
	break;
	
    case BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ:
        if (ancs_ams_data.pconn != NULL){
            if(!ancs_get_write_pending_status()){
                ancs_get_notification_attribute_cmd((struct bt_conn *)ancs_ams_data.pconn,(uint32_t)event_data);
            }
            else{
                ble_ancs_write_notification_request_to_list((uint32_t)event_data);
            }
        }
	    break;

    case BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ_COMPLETE:
		anam_log_inf("BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ_COMPLETE handle:%d\n",(uint32_t)event_data);
		if (ancs_ams_data.pconn != NULL){
			if(!ancs_get_write_pending_status()){
				uint32_t notify_uuid = 0;
				if(!ble_ancs_read_notification_request_from_list(&notify_uuid)){
					if(notify_uuid == BLE_ANCS_APP_NO_UUID){
						ancs_get_application_attribute_cmd((struct bt_conn *)ancs_ams_data.pconn);
					}else{
						ancs_get_notification_attribute_cmd((struct bt_conn *)ancs_ams_data.pconn,notify_uuid);
					}
				}
			}
		}
		break;

	case BLE_ANCS_EVENT_APP_ATTR_REQ:
		if (ancs_ams_data.pconn != NULL){
			if(!ancs_get_write_pending_status()){
				ancs_get_application_attribute_cmd((struct bt_conn *)ancs_ams_data.pconn);
			}else{
				ble_ancs_write_notification_request_to_list((uint32_t)event_data);
			}
		}
		break;

    case BLE_AMS_EVENT_WRITE_ENTITY_UPDATE:
        anam_log_inf("BLE_AMS_EVENT_WRITE_ENTITY_UPDATE %d\n",(uint32_t)event_data);
	    break;

    case BLE_AMS_EVENT_READ_ENTITY_UPDATE:
        anam_log_inf("BLE_AMS_EVENT_READ_ENTITY_UPDATE %d\n",(uint32_t)event_data);
        break;

    case BLE_GAP_EVENT_SERVICE_CHANGED:
        if (ancs_ams_data.ios_phone && (ancs_ams_data.pconn != NULL)) {
            anam_log_inf("Service change disconnect ble, need phone or apk connect ble again!\n");
            bt_manager_ble_disconnect();
        }
        break;

	case BLE_ANCS_EVENT_NOTIFICATION_MSG_FINISH:
		ancs_notify_msg_finish();
		break;

	default:
		break;
	}

handle_exit:
	if (prio >= 0) {
		os_thread_priority_set(os_current_get(), prio);
	}
}

struct ancs_ams_data_t *ble_get_ancs_ams(void)
{
    return &ancs_ams_data;
}

int ble_get_ams_status(void)
{
    if(!ancs_ams_data.ams_configured){
        return -EIO;
    }
    ams_write_entity_attr(ENTITY_TRACK,TRACK_ATTR_TITLE);
    ams_read_entity_attr();
    return 0;
}

int ble_ancs_ams_notify_playback_pos(float elapsed_time)
{
	ancs_ams_data.getting_pos = 0;

#ifdef CONFIG_BT_AVRCP_GET_ID3
	uint32_t pos = (uint32_t)(elapsed_time * 1000);

	bt_manager_avrcp_notify_playback_pos(pos);
#endif

	return 0;
}

int ble_ancs_ams_get_playback_pos(void)
{
	if (ancs_ams_data.getting_pos) {
		return -EBUSY;
	}

	ancs_ams_data.getting_pos = 1;
	return ams_write_entity_update(ENTITY_PLAYER);
}
