/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_DISC_H__
#define __APP_DISC_H__

#define INVALID_ATT_HANDLE                          (0x0000)

/*============================================================================*
 *  Public Data Types
 *============================================================================*/
/* Structure for a characteristic */
struct gatt_characteristic
{
    u16_t          start_handle;   /* Start handle for a characteristic */
    u16_t          end_handle;     /* End handle for a characteristic */
		union {
			struct bt_uuid uuid;
			struct bt_uuid_16 u16;
			struct bt_uuid_128 u128;
		} uuid;
    bool            has_ccd;        /* Does the characteristic has client 
                                     * configuration 
                                     */
    u16_t          ccd_handle;     /* Handle for the client configuration 
                                     * attribute
                                     */
    u16_t          value;          /* Client configuration */
};


/* Structure for the discovered service */
struct gatt_service
{
    u16_t          start_handle;   /* Start handle for a service */
    u16_t          end_handle;     /* End handle for a service */
		union {
			struct bt_uuid uuid;
			struct bt_uuid_16 u16;
			struct bt_uuid_128 u128;
		} uuid;
    u16_t          num_chars;      /* Number of characteristics in the service 
                                     */
    u16_t          char_index;     /* Handy characteristic index being used 
                                     * while discovering characteristic 
                                     * descriptors.
                                     */
    struct gatt_characteristic     *char_data;     /* Pointer to characteristic data */
    u16_t          nvm_offset;     /* NVM offset for the service data */

    /* Function for reading discovered service attribute handles stored in NVM
     */
    void    (*read_disc_service_handles_from_nvm)(bool handles_present);

    /* Function for writing handles in NVM space.*/
    void    (*write_disc_service_handles_from_nvm)(void);

   
};

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/**
 *  @brief
 *      start_gatt_database_discovery
 *
 *  
 *      This function starts the GATT database discovery
 *
 *  @return
 *      Nothing
 *
 **/
extern void start_gatt_database_discovery(struct bt_conn *conn);

/**
 *  @brief
 *      stop_gatt_database_discovery
 *
 *  
 *      This function stops the GATT database discovery
 *
 *  @return
 *      Nothing
 *
 **/
extern void stop_gatt_database_discovery(void);

/**
 *  @brief
 *      discover_a_primary_service
 *
 *  
 *      This function discovers a primary service.
 *
 *  @return
 *      Nothing
 *
 */
extern void discover_a_primary_service(struct bt_conn *conn);

/**
 *  @brief
 *      handle_discover_primary_service_ind
 *
 *  
 *      This function handles the signal GATT_DISC_PRIM_SERV_BY_UUID_IND
 *
 *  @return
 *      Nothing
 *
 */
extern void handle_discover_primary_service_ind(u16_t start_handle, u16_t end_handle);

/**
 *  @brief
 *      handle_service_discovery_primary_service_by_uuid_cfm
 *
 *  
 *      This function handles the signal GATT_DISC_PRIM_SERV_BY_UUID_CFM
 *
 *  @return
 *      Nothing
 *
 */
extern void handle_service_discovery_primary_service_by_uuid_cfm(struct bt_conn *conn, u8_t status);

/**
 *  @brief
 *      handle_gatt_service_characteristic_declaration_info_ind
 *
 *  
 *      This function handles the signal GATT_CHAR_DECL_INFO_IND
 *
 *  @return
 *      Nothing
 *
 */
extern void handle_gatt_service_characteristic_declaration_info_ind(
                                     struct bt_gatt_chrc *ind, u16_t handle);

/**
 *  @brief
 *      handle_gatt_discover_service_characteristic_cfm
 *
 *  
 *      This function handles GATT_DISC_SERVICE_CHAR_CFM messages.
 *
 *  @return
 *      Nothing.
 */
extern void handle_gatt_discover_service_characteristic_cfm(struct bt_conn *conn, u8_t status);

/**
 *  @brief
 *      handle_gatt_characteristic_descriptor_info_ind
 *
 *  
 *      This function handles GATT_CHAR_DESC_INFO_IND messages.
 *
 *  @return
 *      Nothing.
 */
extern void handle_gatt_characteristic_descriptor_info_ind(
                                        const struct bt_gatt_attr *attr);

/**
 *  @brief
 *      handle_gatt_characteristic_descriptor_cfm
 *
 *  
 *      This function handles GATT_DISC_ALL_CHAR_DESC_CFM messages.
 *
 *  @return
 *      Nothing.
 */
extern void handle_gatt_characteristic_descriptor_cfm(struct bt_conn *conn, u8_t status);

/**
 *  @brief
 *      read_discovered_gatt_database_from_nvm
 *
 *  
 *      This function reads the already discoverd attribute handles from NVM
 *
 *  @return
 *      Nothing.
 */
extern void read_discovered_gatt_database_from_nvm(bool handles_present);

/**
 *  @brief
 *      write_discovered_gatt_database_to_nvm
 *
 *  
 *      This function should be called on NVM intialisation phase and it stores
 *      INVALID_ATT_HANDLE for all the GATT database attributes
 *
 *  @return
 *      Nothing.
 */
extern void write_discovered_gatt_database_to_nvm(void);

/**
 *  @brief
 *      reset_discovered_handles_database
 *
 *  
 *      This function resets the discovered handles database.
 *
 *  @return
 *      Nothing.
 */
extern void reset_discovered_handles_database(void);

#endif /* __APP_DISC_H__ */
