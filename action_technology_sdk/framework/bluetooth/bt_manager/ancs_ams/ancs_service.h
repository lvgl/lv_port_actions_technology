/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ANCS_SERVICE_H__
#define __ANCS_SERVICE_H__



/*============================================================================*
 *  Public Definitions
 *============================================================================*/
/* Notification Source Event Flags */
#define ANCS_NS_EVENTFLAG_SILENT                 (0x01) 
#define ANCS_NS_EVENTFLAG_IMPORTANT              (0x02)
#define ANCS_NS_EVENTFLAG_PRE_EXISTING           (0x04)
#define ANCS_NS_EVENTFLAG_POSITIVE_ACTION        (0x08)
#define ANCS_NS_EVENTFLAG_NEGITIVE_ACTION        (0x10)
#define ANCS_NS_EVENTFLAG_RESERVED               ((1<<5)-(1<<7))

/* This macro defines the size of the notification UUID */
#define ANCS_NS_NOTIF_UUID_SIZE                  (4)

#define ANCS_RESERVE_ACTION_SIZE                 (6)

/* ANCS error codes */
#define ANCS_ERROR_UNKNOWN_COMMAND               (0xAA0)
#define ANCS_ERROR_INVALID_COMMAND               (0xAA1)
#define ANCS_ERROR_INVALID_PARAMETER             (0xAA2)

/*============================================================================*
 *  Public Data Types
 *============================================================================*/
/* enum for ANC characteristic */
typedef enum 
{
    ancs_notification_source = 0,
    ancs_control_point,
    ancs_data_source,
    ancs_type_invalid
}ancs_type;

/* enum for event id */
typedef enum 
{
    ancs_event_id_notif_added     = 0x0,
    ancs_event_id_notif_modified,
    ancs_event_id_notif_removed,
    ancs_event_id_notif_reserved,
    ancs_event_id_notif_max
}ancs_event_id;

/* enum for category id */
typedef enum
{
    ancs_cat_id_other             = 0x0,   /* Other : updates */
    ancs_cat_id_incoming_call,             /*  Call: Incoming call */
    ancs_cat_id_missed_call,               /* Missed call: Missed Call */
    ancs_cat_id_vmail,                     /* Voice mail: Voice mail*/
    ancs_cat_id_social,                    /* Social message indications */
    ancs_cat_id_schedule,                  /* Schedule: Calendar, planner */
    ancs_cat_id_email,                     /* Email: mail message arrives */
    ancs_cat_id_news,                      /* News: RSS/Atom feeds etc */
    ancs_cat_id_hnf,                       /* Health and Fitness: updates  */
    ancs_cat_id_bnf,                       /* Business and Finance: updates */
    ancs_cat_id_location,                  /* Location: updates */
    ancs_cat_id_entertainment,             /* Entertainment: updates */
    ancs_cat_id_reserved,                   /* Reserved */
    ancs_cat_id_max
} ancs_category_id;

/* enum for command id */
typedef enum
{
   ancs_cmd_get_notification_att = 0x0,
   ancs_cmd_get_app_att,
   ancs_cmd_reserve_cmd
}ancs_command_id;

/* enum for notification attribute id */
typedef enum
{
    ancs_notif_att_id_app_id = 0x0,
    ancs_notif_att_id_title,
    ancs_notif_att_id_subtitle,
    ancs_notif_att_id_message,
    ancs_notif_att_id_message_size,
    ancs_notif_att_id_date,
    ancd_notif_att_id_max
}ancs_notif_att_id;


/* enum for app attribute id */
typedef enum
{
    ancs_app_att_display_name = 0x0,
    ancs_app_att_reserved
}ancs_app_att_id;

/* enum for decoding the attribute data */
typedef enum
{
    ds_decoder_hdr = 0x0,
    ds_decoder_attrid,
    ds_decoder_attrlen,
    ds_decoder_attrdata,
    ds_decoder_unknown
}ds_decoder_state;

/* enum for command id */
typedef enum
{
   ancs_cmd_accept_call = 0x0,
   ancs_cmd_reject_call = 0x1
}ancs_reserve_action_id;


struct ancs_write_request_param {
	sys_snode_t node;
	struct bt_gatt_write_params params;
};

extern void ancs_init_attribute_data(void);

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/**
 *  @brief
 *      ancs_service_data_init
 *
 *  
 *      This function initialises the Apple Notification Control Service data
 *
 *  @return
 *      Nothing.
 *
 */
extern void ancs_service_data_init(void);

/**
 *  @brief
 *      ancs_handler_notif_ind
 *
 *  
 *      This function handles all the registered notifications for ANCS service
 *
 *  @return
 *      TRUE, if successful else FALSE
 *
 */
extern bool ancs_handler_notif_ind(struct bt_conn *conn, uint16_t handle, const uint8_t *notify_data, uint16_t length);

/**
 *  @brief
 *      ancs_write_request
 *
 *  
 *      This function initiates a GATT Write procedure for the characteristic
 *      that supports Write procedure
 *
 *  @return
 *      TRUE, if successful else FALSE
 *
 */
extern bool ancs_write_request(uint16_t type, uint8_t *data, uint16_t size, struct bt_conn *conn);

/**
 *  @brief
 *      ancs_get_notification_attribute_cmd
 *
 *  
 *      This function sends the Get Notification Attributes command to the
 *      peer device based on the UUID  and features requested.
 *
 *  @return
 *     Nothing
 *
 */
extern void ancs_get_notification_attribute_cmd(struct bt_conn *conn,uint32_t notity_uuid);

/**
 *  @brief
 *      ancs_get_application_attribute_cmd
 *
 *
 *      This function sends the Get Application Attributes command to the
 *      peer device based on the APP Identify and features requested.
 *
 *  @return
 *     Nothing
 *
 */
extern void ancs_get_application_attribute_cmd(struct bt_conn *conn);

/**
 *  @brief
 *      configure_ancs_notifications
 *
 *  
 *      This function configures indications on the ANCS Notification
 *      Characteristics.
 *  @return
 *      Nothing
 **/
extern bool configure_ancs_notifications(struct bt_conn *conn,bool enable);

/**
 *  @brief
 *      configure_ancs_data_source_notification
 *
 *  
 *      This function configures indications on the ANCS Data Source
 *      Notification Characteristics.
 *
 *  @return
 *      Nothing
 */
extern bool configure_ancs_data_source_notification(struct bt_conn *conn,bool enable);

/**
 *  @brief
 *      does_handle_belongs_to_ancs_service
 *
 *  
 *      This function checks if the passed handle belongs to ANCS Service.
 *
 *  @return
 *      TRUE if it belongs to ANCS Service
 *
 */
extern bool does_handle_belongs_to_ancs_service(uint16_t handle);

extern uint8_t ancs_get_write_pending_status(void);

extern void ancs_notify_msg_finish(void);

#endif /* __ANCS_SERVICE_H__ */
