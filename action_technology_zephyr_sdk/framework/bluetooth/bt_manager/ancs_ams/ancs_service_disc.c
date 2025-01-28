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
#include "ancs_service.h"
#include "msg_manager.h"
#include "ancs_service_disc.h"
#include "ancs_ams_disc.h"

/*============================================================================
 *  Private Data
 *============================================================================
 */

/* Discovered ANCS notification characteristic */
struct gatt_characteristic g_disc_ancs_char[] = {
	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS notification characteristic */
			{
				0xBD, 0x1D, 0xA2, 0x99, 0xE6, 0x25, 0x58, 0x8C,
				0xD9, 0x42, 0x01, 0x63, 0x0D, 0x12, 0xBF, 0x9F
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
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS control point characteristic */
			{
			0xD9, 0xD9, 0xAA, 0xFD, 0xBD, 0x9B, 0x21, 0x98,
			0xA8, 0x49, 0xE1, 0x45, 0xF3, 0xD8, 0xD1, 0x69
			},
		},

		false,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},

	{
		INVALID_ATT_HANDLE,     /* start_handle */
		INVALID_ATT_HANDLE,     /* end_handle */
		{
			.u128.uuid.type = BT_UUID_TYPE_128,           /* UUID for ANCS data source characteristic */
			{
			0xFB, 0x7B, 0x7C, 0xCE, 0x6A, 0xB3, 0x44, 0xBE,
			0xB5, 0x4B, 0xD6, 0x24, 0xE9, 0xC6, 0xEA, 0x22
			},
		},

		true,                       /* Has_ccd */
		INVALID_ATT_HANDLE,         /* ccd_handle*/
		0 /*value*/
	},
};

/* Discovered ANCS Service  */
struct gatt_service g_disc_ancs_service = {
	INVALID_ATT_HANDLE,         /* start handle */
	INVALID_ATT_HANDLE,         /* end_handle */
	{
		.u128.uuid.type =  BT_UUID_TYPE_128,
		{
		0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4,
		0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79
		},
	},
	3,                          /* Number of characteristics  */
	0,                          /* characteristic index */
	g_disc_ancs_char,
	0,                          /* NVM_OFFSET*/
	NULL,
	NULL,
};


uint16_t get_remote_disc_ancs_service_start_handle(void)
{
	return g_disc_ancs_service.start_handle;
}

uint16_t get_remote_disc_ancs_service_end_handle(void)
{
	return g_disc_ancs_service.end_handle;
}

bool does_handle_belong_to_discovered_ancs_service(uint16_t handle)
{
	return ((handle >= g_disc_ancs_service.start_handle) &&
					(handle <= g_disc_ancs_service.end_handle))
					? true : false;
}

uint16_t get_ancs_notification_handle(void)
{
	return g_disc_ancs_char[0].start_handle + 1;
}

uint16_t get_ancs_notification_ccd_handle(void)
{
	return g_disc_ancs_char[0].ccd_handle;
}

uint16_t get_ancs_control_point_handle(void)
{
	return g_disc_ancs_char[1].start_handle + 1;
}

uint16_t get_ancs_data_source_handle(void)
{
	return g_disc_ancs_char[2].start_handle + 1;
}

uint16_t get_ancs_data_source_ccd_handle(void)
{
	return g_disc_ancs_char[2].ccd_handle;
}
