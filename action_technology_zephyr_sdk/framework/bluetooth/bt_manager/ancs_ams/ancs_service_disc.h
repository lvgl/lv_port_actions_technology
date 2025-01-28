/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ANCS_SERVICE_DISC_H__
#define __ANCS_SERVICE_DISC_H__


/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/*============================================================================*
 *  Public Data Declaration
 *============================================================================*/

extern struct gatt_service g_disc_ancs_service;

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
extern void write_disc_ancs_service_handles_to_nvm(void);

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
extern void read_disc_ancs_service_handles_from_nvm(bool handles_present);

/**
 *  @brief
 *      get_remote_disc_ancs_service_start_handle
 *
 *  
 *      This function returns the start handle for the Discovered ANCS 
 *      service.
 *
 *  @return
 *      Nothing.
 */
extern uint16_t get_remote_disc_ancs_service_start_handle(void);

/**
 *  @brief
 *      get_remote_disc_ancs_service_end_handle
 *
 *  
 *      This function returns the end handle for the Discovered ANCS 
 *      service.
 *
 *  @return
 *      Nothing.
 */
extern uint16_t get_remote_disc_ancs_service_end_handle(void);


/**
 *  @brief
 *      does_handle_belong_to_discovered_ancs_service
 *
 *  
 *      This function checks if the handle belongs to discovered ANCS 
 *      service
 *
 *  @return
 *      Boolean - Indicating whether handle falls in range or not.
 *
 */
extern bool does_handle_belong_to_discovered_ancs_service(uint16_t handle);

/**
 *  @brief
 *      get_ancs_notification_handle
 *
 *  
 *      This function gives the ANCS Notification Characteristic Handle.
 *
 *  @return
 *      ANCS Notification Characteristic Handle
 *
 */
extern uint16_t get_ancs_notification_handle(void);

/**
 *  @brief
 *      get_ancs_notification_ccd_handle
 *
 *  
 *      This function gives the ANCS Notification Characteristic CCD Handle.
 *
 *  @return
 *      This function gives the ANCS Notification Characteristic CCD Handle.
 *
 */
extern uint16_t get_ancs_notification_ccd_handle(void);

/**
 *  @brief
 *      get_ancs_control_point_handle
 *
 *  
 *      This function gives the ANCS Control Point Characteristic Handle.
 *
 *  @return
 *      This function gives the ANCS Control Point Characteristic Handle.
 *
 */
extern uint16_t get_ancs_control_point_handle(void);

/**
 *  @brief
 *      get_ancs_control_point_handle
 *
 *  
 *      This function gives the ANCS Data Source Characteristic Handle.
 *
 *  @return
 *      This function gives the ANCS Data Source Characteristic Handle.
 *
 */
extern uint16_t get_ancs_data_source_handle(void);

/**
 *  @brief
 *      get_ancs_data_source_ccd_handle
 *
 *  
 *      This function gives the ANCS Data Source Characteristic CCD Handle.
 *
 *  @return
 *      ANCS Data Source Characteristic Configuration handle
 *
 */
extern uint16_t get_ancs_data_source_ccd_handle(void);

#endif /* __ANCS_SERVICE_DISC_H__ */
