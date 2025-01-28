/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __AMS_SERVICE_DISC_H__
#define __AMS_SERVICE_DISC_H__


/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/*============================================================================*
 *  Public Data Declaration
 *============================================================================*/

extern struct gatt_service g_disc_ams_service;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/**
 *  @brief
 *      write_disc_ancs_service_handles_to_nvm
 *
 *  
 *      This function stores the discovered service handles in NVM
 *
 *  @return
 *      Nothing.
 */
extern void write_disc_ams_service_handles_to_nvm(void);

/**
 *  @brief
 *      read_disc_ancs_service_handles_from_nvm
 *
 *  
 *      This function reads the ANCS Service handles from NVM
 *
 *  @return
 *      Nothing.
 */
extern void read_disc_ams_service_handles_from_nvm(bool handles_present);


uint16_t get_remote_disc_ams_service_start_handle(void);


uint16_t get_remote_disc_ams_service_end_handle(void);


bool does_handle_belong_to_discovered_ams_service(uint16_t handle);


uint16_t get_ams_remote_cmd_handle(void);


uint16_t get_ams_remote_cmd_ccd_handle(void);


uint16_t get_ams_entity_update_handle(void);


uint16_t get_ams_entity_update_ccd_handle(void);


uint16_t get_ams_entity_attr_handle(void);

#endif /* __AMS_SERVICE_DISC_H__ */
