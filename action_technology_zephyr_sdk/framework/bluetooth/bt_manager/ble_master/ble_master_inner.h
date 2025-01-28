#ifndef __BLE_MASTER_INNER_H__
#define __BLE_MASTER_INNER_H__

#include "ble_master_manager.h"
#include "conn_master_manager.h"
#include "bt_manager_ble.h"
#include "ble_master_dev.h"
#include "../bt_manager_inner.h"

#ifndef	CONFIG_OTA_BLE_MASTER_SUPPORT
#define BLE_MASTER "ble_master"

/* msg type */
enum {
	MSG_BLE_MASTER_EVENT,		/* Ble master process event */
};
#endif

#endif
