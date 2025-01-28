/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include "errno.h"
#include <zephyr.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/host_interface.h>

#include "msg_manager.h"
#include "gap_service_disc.h"
#include "ancs_ams_disc.h"
#include "ble_ancs_ams.h"
#include "ancs_ams_log.h"

/*============================================================================
 *  Private Data
 *============================================================================
 */

/* Discovered ANCS notification characteristic */
struct gatt_characteristic g_disc_gap_char[] = {
	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u16.uuid.type = BT_UUID_TYPE_16,           /* UUID for AMS remote command characteristic */

			0x2A05,
        },
		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},
};

/* Discovered AMS Service  */
struct gatt_service g_disc_gap_service = {
	INVALID_ATT_HANDLE,         /* start handle */
	INVALID_ATT_HANDLE,         /* end_handle */
	{
		.u16.uuid.type =  BT_UUID_TYPE_16,

		0x01801,

	},
	1,                          /* Number of characteristics  */
	0,                          /* characteristic index */
	g_disc_gap_char,
	0,                          /* NVM_OFFSET*/
	NULL,
	NULL,
};

static struct bt_gatt_subscribe_params subscribe_params_sc;
static uint8_t remote_service_changed = 0;

uint16_t get_remote_disc_gap_service_start_handle(void)
{
	return g_disc_gap_service.start_handle;
}

uint16_t get_remote_disc_gap_service_end_handle(void)
{
	return g_disc_gap_service.end_handle;
}

bool does_handle_belong_to_discovered_gap_service(uint16_t handle)
{
	return ((handle >= g_disc_gap_service.start_handle) &&
					(handle <= g_disc_gap_service.end_handle))
					? true : false;
}

uint16_t get_gap_service_changed_handle(void)
{
	return g_disc_gap_char[0].start_handle + 1;
}

uint16_t get_gap_service_changed_ccd_handle(void)
{
	return g_disc_gap_char[0].ccd_handle;
}

static uint8_t notify_func(struct bt_conn *conn,
	struct bt_gatt_subscribe_params *params,
	const void *data, uint16_t length)
{
    if (!data) {
        return BT_GATT_ITER_STOP;
    }
    
    if(params->ccc_handle == get_gap_service_changed_ccd_handle()) {
        gap_log_inf("remote_service_changed\n");
        if(!remote_service_changed){
            ble_ancs_ams_send_msg_to_app(BLE_GAP_EVENT_SERVICE_CHANGED, !remote_service_changed);
            remote_service_changed = 1;
        }
    } 

    return BT_GATT_ITER_CONTINUE;
}

int config_gap_service_changed_indicate(struct bt_conn *conn,bool enable)
{
    int err;

    remote_service_changed = 0;

	subscribe_params_sc.notify = notify_func;
	subscribe_params_sc.value = BT_GATT_CCC_INDICATE;
	subscribe_params_sc.ccc_handle = get_gap_service_changed_ccd_handle();
	subscribe_params_sc.value_handle = get_gap_service_changed_handle();
	atomic_set_bit(subscribe_params_sc.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	gap_log_inf("subscribe gap handle 0x%2x, 0x%2x %d\n", subscribe_params_sc.ccc_handle, subscribe_params_sc.value_handle, enable);
    if(enable){
	    err = hostif_bt_gatt_subscribe(conn, &subscribe_params_sc);
    	if (err && err != -EALREADY) {
            gap_log_err("GAP subscribe failed (err %d)\n", err);
    	} else {
            gap_log_inf("GAP subscribe success\n");
    	}
    }
    else{
        err = hostif_bt_gatt_unsubscribe(conn, &subscribe_params_sc);
        if (err) {
            gap_log_err("GAP unsubscribe failed (err %d)\n", err);
        } 
        else {
            gap_log_inf("GAP unsubscribe success\n");
        }
    }
	return err;
}
