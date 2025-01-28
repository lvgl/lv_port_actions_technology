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
#include "ams_service.h"
#include "msg_manager.h"
#include "ams_service_disc.h"
#include "ancs_ams_disc.h"

/*============================================================================
 *  Private Data
 *============================================================================
 */

/* Discovered ANCS notification characteristic */
struct gatt_characteristic g_disc_ams_char[] = {
	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for AMS remote command characteristic */
			{
				0xc2, 0x51, 0xca, 0xf7, 0x56, 0x0e, 0xdf, 0xb8,
    			0x8a, 0x4a, 0xb1, 0x57, 0xd8, 0x81, 0x3c, 0x9b
			},
		},

		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},

	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for AMS entity update characteristic */
			{
				0x02, 0xc1, 0x96, 0xba, 0x92, 0xbb, 0x0c, 0x9a,
    			0x1f, 0x41, 0x8d, 0x80, 0xce, 0xab, 0x7c, 0x2f
			},
		},

		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},

	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for AMS entity attribute characteristic */
			{
				0xd7, 0xd5, 0xbb, 0x70, 0xa8, 0xa3, 0xab, 0xa6,
    			0xd8, 0x46, 0xab, 0x23, 0x8c, 0xf3, 0xb2, 0xc6
			},
		},

		false,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},
};

/* Discovered AMS Service  */
struct gatt_service g_disc_ams_service = {
	INVALID_ATT_HANDLE,         /* start handle */
	INVALID_ATT_HANDLE,         /* end_handle */
	{
		.u128.uuid.type =  BT_UUID_TYPE_128,
		{
			0xdc, 0xf8, 0x55, 0xad, 0x02, 0xc5, 0xf4, 0x8e,
			0x3a, 0x43, 0x36, 0x0f, 0x2b, 0x50, 0xd3, 0x89
		},
	},
	3,                          /* Number of characteristics  */
	0,                          /* characteristic index */
	g_disc_ams_char,
	0,                          /* NVM_OFFSET*/
	NULL,
	NULL,
};


uint16_t get_remote_disc_ams_service_start_handle(void)
{
	return g_disc_ams_service.start_handle;
}

uint16_t get_remote_disc_ams_service_end_handle(void)
{
	return g_disc_ams_service.end_handle;
}

bool does_handle_belong_to_discovered_ams_service(uint16_t handle)
{
	return ((handle >= g_disc_ams_service.start_handle) &&
					(handle <= g_disc_ams_service.end_handle))
					? true : false;
}

uint16_t get_ams_remote_cmd_handle(void)
{
	return g_disc_ams_char[0].start_handle + 1;
}

uint16_t get_ams_remote_cmd_ccd_handle(void)
{
	return g_disc_ams_char[0].ccd_handle;
}

uint16_t get_ams_entity_update_handle(void)
{
	return g_disc_ams_char[1].start_handle + 1;
}

uint16_t get_ams_entity_update_ccd_handle(void)
{
	return g_disc_ams_char[1].ccd_handle;
}

uint16_t get_ams_entity_attr_handle(void)
{
	return g_disc_ams_char[2].start_handle + 1;
}
