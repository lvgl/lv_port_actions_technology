/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __GAP_SERVICE_DISC_H__
#define __GAP_SERVICE_DISC_H__


/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/*============================================================================*
 *  Public Data Declaration
 *============================================================================*/

extern struct gatt_service g_disc_gap_service;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/



uint16_t get_remote_disc_gap_service_start_handle(void);


uint16_t get_remote_disc_gap_service_end_handle(void);


bool does_handle_belong_to_discovered_gap_service(uint16_t handle);


uint16_t get_gap_service_changed_handle(void);


uint16_t get_gap_service_changed_ccd_handle(void);

int config_gap_service_changed_indicate(struct bt_conn *conn,bool enable);


#endif /* __AMS_SERVICE_DISC_H__ */
