/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include "errno.h"
#include <zephyr.h>
#include <msg_manager.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/host_interface.h>

//#include "app_defines.h"
#include "ancs_service.h"
#include "ancs_service_disc.h"
#include "ams_service.h"
#include "ams_service_disc.h"
#include "ancs_ams_disc.h"
#include "ble_ancs_ams.h"
#include "gap_service_disc.h"
#include "ancs_ams_log.h"

struct discovery_proc_t {
	/* Boolean flag which tells if the discovery procedure is ongoing or not */
	bool        is_discovering;

	/* variable which tells the index of the service structure currently being
	 * discovered in the g_service_list
	 */
	uint16_t      service_index;

	/* Boolean flag which tells if the discovery procedure was successful */
	bool        discovery_result;
};


/*============================================================================
 *  Private Data
 *============================================================================
 */
/* Discovery procedure data structure.*/
static struct discovery_proc_t g_discovery_data;

/* Pointer to the currently used service structure */
static struct gatt_service *g_service_data;

/* List of pointers to supported service structure. Terminated
 * by a null pointer
 */
static struct gatt_service *g_service_list[] = {
	&g_disc_ancs_service,
	&g_disc_ams_service,
    &g_disc_gap_service,
	NULL
};

/*============================================================================
 *  Public Function Definitions
 *============================================================================
 */
static struct bt_gatt_discover_params discover_params;
static bool chrc_is_found;
static bool chrc_dec_is_found;
static uint8_t service_type;
static void print_chrc_props(uint8_t properties)
{
	dis_log_inf("Properties: ");

	if (properties & BT_GATT_CHRC_BROADCAST) {
		dis_log_inf("[bcast]");
	}

	if (properties & BT_GATT_CHRC_READ) {
		dis_log_inf("[read]");
	}

	if (properties & BT_GATT_CHRC_WRITE) {
		dis_log_inf("[write]");
	}

	if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
		dis_log_inf("[write w/w rsp]");
	}

	if (properties & BT_GATT_CHRC_NOTIFY) {
		dis_log_inf("[notify]");
	}

	if (properties & BT_GATT_CHRC_INDICATE) {
		dis_log_inf("[indicate]");
	}

	if (properties & BT_GATT_CHRC_AUTH) {
		dis_log_inf("[auth]");
	}

	if (properties & BT_GATT_CHRC_EXT_PROP) {
		dis_log_inf("[ext prop]");
	}

	dis_log_inf("\n");
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *gatt_service;
	struct bt_gatt_chrc *gatt_chrc;
	struct bt_gatt_include *gatt_include;
	char uuid[37];

	if (!attr) {
		if (params->type == BT_GATT_DISCOVER_PRIMARY) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_DISC_PRIM_SERV_IND, service_type);
            service_type = BLE_SERVICE_NULL;
		} else if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_DISC_SERVICE_CHAR_IND, !chrc_is_found);
			chrc_is_found = false;
		} else if (params->type == BT_GATT_DISCOVER_DESCRIPTOR) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_DISC_SERVICE_CHAR_DESC_IND, !chrc_dec_is_found);
			chrc_dec_is_found = false;
		}
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
	case BT_GATT_DISCOVER_SECONDARY:
	case BT_GATT_DISCOVER_PRIMARY:
		gatt_service = attr->user_data;
		bt_uuid_to_str(gatt_service->uuid, uuid, sizeof(uuid));
		dis_log_inf("Service %s found: start handle %x, end_handle %x\n",
		       uuid, attr->handle, gatt_service->end_handle);
        if(gatt_service->uuid->type == BT_UUID_TYPE_16){
            service_type = BLE_SERVICE_GAP;
        }
        else{
            service_type = BLE_SERVICE_ANCS_AMS;
        }
		handle_discover_primary_service_ind(attr->handle, gatt_service->end_handle);
		break;
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
		bt_uuid_to_str(gatt_chrc->uuid, uuid, sizeof(uuid));
		dis_log_inf("Characteristic %s found: handle %x\n", uuid,
		       attr->handle);
		print_chrc_props(gatt_chrc->properties);
		chrc_is_found = true;
		handle_gatt_service_characteristic_declaration_info_ind(gatt_chrc, attr->handle);
		break;
	case BT_GATT_DISCOVER_INCLUDE:
		gatt_include = attr->user_data;
		bt_uuid_to_str(gatt_include->uuid, uuid, sizeof(uuid));
		dis_log_inf("Include %s found: handle %x, start %x, end %x\n",
		       uuid, attr->handle, gatt_include->start_handle,
		       gatt_include->end_handle);
		break;
	default:
		bt_uuid_to_str(attr->uuid, uuid, sizeof(uuid));
		dis_log_inf("Descriptor %s found: handle %x\n", uuid, attr->handle);
		//chrc_dec_is_found = true;
		handle_gatt_characteristic_descriptor_info_ind(attr);
		break;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void GattDiscoverPrimaryServiceByUuid(struct bt_conn *conn, struct bt_uuid *uuid)
{
	int err;

	if (!conn) {
		dis_log_err("Not connected\n");
		return;
	}

	discover_params.uuid = uuid;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;
	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.func = discover_func;
	err = hostif_bt_gatt_discover(conn, &discover_params);
	if (err) {
		dis_log_err("Discover failed (err %d)\n", err);
	} else {
		dis_log_inf("Discover pending\n");
	}
}

static  void GattDiscoverServiceChar(struct bt_conn *conn, uint16_t start_handle, uint16_t end_handle)
{
	int err;

	if (!conn) {
		dis_log_err("Not connected\n");
		return;
	}

	discover_params.func = discover_func;
	discover_params.start_handle = start_handle;
	discover_params.end_handle = end_handle;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
	err = hostif_bt_gatt_discover(conn, &discover_params);
	if (err)  {
		dis_log_err("Discover failed (err %d)\n", err);
	} else {
		dis_log_inf("Discover pending\n");
	}
}

static void GattDiscoverAllCharDescriptors(struct bt_conn *conn, uint16_t start_handle, uint16_t end_handle)
{
	int err;

	if (!conn) {
		dis_log_err("Not connected\n");
		return;
	}

	discover_params.func = discover_func;
	discover_params.start_handle = start_handle;
	discover_params.end_handle = end_handle;
	discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
	err = hostif_bt_gatt_discover(conn, &discover_params);
	if (err) {
		dis_log_err("Discover failed (err %d)\n", err);
	} else {
		dis_log_inf("Discover pending\n");
	}
}

void start_gatt_database_discovery(struct bt_conn *conn)
{
	/* This discovery procedure discovers all the primary services listed
	 * in the array discovery_array. Start with the service at index 0.
	 */
	g_discovery_data.service_index = 0;
	g_discovery_data.is_discovering = true;

	/* Initialise the discovery_result to SUCCESS, reset it to
	 * false upon first failure of discovery
	 */
	g_discovery_data.discovery_result = true;
	discover_a_primary_service(conn);
}

void stop_gatt_database_discovery(void)
{
	/* Service discovery is complete. Set the is_discovering flag to false */
	g_discovery_data.is_discovering = false;

	/* Tell the main application about the service discover completion. */
	ble_ancs_ams_send_msg_to_app(BLE_ANCS_AMS_DISCOVERY_COMPELTE_IND, g_discovery_data.discovery_result);
}

void discover_a_primary_service(struct bt_conn *conn)
{
	/* Initialise the service data */
	g_service_data = g_service_list[g_discovery_data.service_index];

	/* Start discovery */
	GattDiscoverPrimaryServiceByUuid(conn, &g_service_data->uuid.uuid);
}

void handle_discover_primary_service_ind(uint16_t start_handle, uint16_t end_handle)
{
	/* Store start and end handles of the service */
	g_service_data->start_handle = start_handle;
	g_service_data->end_handle   = end_handle;
	g_service_data->char_index = 0;
}

void handle_service_discovery_primary_service_by_uuid_cfm(struct bt_conn *conn, uint8_t status)
{
	/* If primary service discovery was a success and valid handles have been
	 * discovered, go ahead with the characteristic discovery.
	 */
	if ((status == 0) &&
		 (g_service_data->start_handle != INVALID_ATT_HANDLE) &&
		 (g_service_data->end_handle != INVALID_ATT_HANDLE)) {
		/* Initialise the char index to invalid index */
		g_service_data->char_index = 0xFFFF;

		/* Discover service characteristics */
		GattDiscoverServiceChar(conn, g_service_data->start_handle,
			g_service_data->end_handle);
	} else {
		/* Discovery has failed set the discovery_result to false
		 * but continue if there are more services to be discovered
		 */
		g_discovery_data.discovery_result = false;

		/* Increment the service index to the next service in the service list
		 */
		g_discovery_data.service_index++;

		/* If there are more services to be discoverd, go ahead */
		if (g_service_list[g_discovery_data.service_index] != NULL) {
			discover_a_primary_service(conn);
		} else {
			/* If not, stop the discovery */
			stop_gatt_database_discovery();
		}
	}
}

void handle_gatt_service_characteristic_declaration_info_ind(
	struct bt_gatt_chrc *ind, uint16_t handle)
{
	uint16_t index;

	/* Iterate through the whole list of characteristics */
	for (index = 0; index < g_service_data->num_chars; index++) {
		/* Check the UUID to see whether it is one of interest. */
		if (!bt_uuid_cmp(&g_service_data->char_data[index].uuid.uuid, ind->uuid)) {
			/* Store the discovered characteristic handle */
			g_service_data->char_data[index].start_handle = handle;

			/* It could be the last characteristic of the service so
			 * assign service end handle to it. If required, the end handle
			 * will be updated with the appropriate value on the next
			 * characteristic indication
			 */
			g_service_data->char_data[index].end_handle = g_service_data->end_handle;

			if (g_service_data->char_index != 0xFFFF) {
				/* More than 1 characteristics are discovered so assign the
				 * the (value_handle - 2) to the end_handle of previous
				 * characteristic
				 */
				g_service_data->char_data[g_service_data->char_index].end_handle = handle - 2;
			}

			/* Update the char_index to the new identified characteristic
			 * index
			 */
			g_service_data->char_index = index;

			break;
		}
	}
}


void handle_gatt_discover_service_characteristic_cfm(struct bt_conn *conn, uint8_t status)
{
	uint16_t index;
	bool isDescDiscoveryRequired = false;

	/* Characteristic discovery is complete, discover the characteristic
	 * descriptors if required
	 */
	for (index = 0; index < g_service_data->num_chars; index++) {
		if (g_service_data->char_data[index].has_ccd) {
			isDescDiscoveryRequired = true;
			g_service_data->char_index = index;
			break;
		}
	}

	/* If characteristic discovery was successful and there are descriptors to
	 * be discovered, go ahead with the descriptor discovery.
	 */
	if (status == 0 &&
		(g_service_data->char_data[index].start_handle != INVALID_ATT_HANDLE) &&
		isDescDiscoveryRequired) {
		/* Characteristic discovery is successful and at least one
		 * characteristic has descriptor to be discovered
		 */
		/* ccc handle may not be start handle + 2, such as ams remote command */
		GattDiscoverAllCharDescriptors(conn,
									g_service_data->char_data[index].start_handle + 2,
									g_service_data->char_data[index].start_handle + 3); /* TODO */
	} else {
		/* Discovery has failed set the discovery_result to false
		 * but continue if there are more services to be discovered
		 */
		if (status != 0) {
			g_discovery_data.discovery_result = false;
		}

		/* Move to the next location in the service list */
		g_discovery_data.service_index++;

		/* If there are more services to be discovered, discover it. */
		if (g_service_list[g_discovery_data.service_index] != NULL) {
			discover_a_primary_service(conn);
		} else {
			/* If not, stop the discovery */
			stop_gatt_database_discovery();
		}
	}
}

void handle_gatt_characteristic_descriptor_info_ind(const struct bt_gatt_attr *attr)
{
	struct bt_uuid_16 *uuid_16 = (struct bt_uuid_16 *)attr->uuid;
	uint16_t index = g_service_data->char_index;

	/* If client configuration descriptor has been discovered, store its handle
	 */
	if (uuid_16->uuid.type == BT_UUID_TYPE_16 &&
			uuid_16->val == BT_UUID_GATT_CCC_VAL) {
		g_service_data->char_data[index].ccd_handle = attr->handle;
		chrc_dec_is_found = true;
	}
}

void handle_gatt_characteristic_descriptor_cfm(struct bt_conn *conn, uint8_t status)
{
	bool isDescDiscoveryComplete = true;
	uint16_t index;

	/* If descriptor discovery was successful, go ahead with discovering
	 * descriptors and services.
	 */
	if (status == 0) {
		index = g_service_data->char_index;

		/* Continue to discover descriptors if any more
		 * characteristics of the service have descriptors
		 */
		while (++index < g_service_data->num_chars) {
			if (g_service_data->char_data[index].has_ccd) {
				isDescDiscoveryComplete = false;
				g_service_data->char_index = index;
				break;
			}
		}

		if (isDescDiscoveryComplete) {
			/* Move to the next location in the service list */
			g_discovery_data.service_index++;

			/* If there are more services to be discovered, go ahead with
			 * discovering the next service.
			 */
			if (g_service_list[g_discovery_data.service_index] != NULL) {
				discover_a_primary_service(conn);
			} else  {
				/* No more service to be discovered, stop the discovery
				 * procedure.
				 */
				stop_gatt_database_discovery();
			}
		} else {
			/* More descriptors to discover */
			/* ccc handle may not be start handle + 2, such as ams remote command */
			GattDiscoverAllCharDescriptors(conn,
				g_service_data->char_data[index].start_handle + 2,
				g_service_data->char_data[index].start_handle + 3);  /* TODO */

		}
	} else {
		/* Service discovery has failed so stop */
		g_discovery_data.discovery_result = false;
		stop_gatt_database_discovery();
	}

}

void reset_discovered_handles_database(void)
{
	uint8_t srv_index = 0;
	uint8_t char_index;

	while (g_service_list[srv_index] != NULL) {
		/* Reset service start handle and end handle*/
		g_service_data = g_service_list[srv_index];
		g_service_data->start_handle = INVALID_ATT_HANDLE;
		g_service_data->end_handle = INVALID_ATT_HANDLE;

		/* Reset characteristic start handle ,end handle and client
		 * configuration descriptor handle
		 */
		for (char_index = 0; char_index < g_service_data->num_chars;
			 char_index++) {
			g_service_data->char_data[char_index].start_handle =
						 INVALID_ATT_HANDLE;
			g_service_data->char_data[char_index].end_handle =
						 INVALID_ATT_HANDLE;
			g_service_data->char_data[char_index].ccd_handle =
						 INVALID_ATT_HANDLE;
		}

		/* Update the handle values in NVM */
		if (g_service_data->write_disc_service_handles_from_nvm) {
			g_service_data->write_disc_service_handles_from_nvm();
		}
		srv_index++;
	}
}
