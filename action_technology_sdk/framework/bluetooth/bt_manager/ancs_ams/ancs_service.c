/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include "errno.h"
#include <zephyr.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/host_interface.h>

//#include "app_defines.h"
#include "ancs_service.h"
#include "ancs_service_disc.h"
#include "ble_ancs_ams.h"
#include "bt_manager.h"
#include "bt_porting_inner.h"
#include "ancs_ams_log.h"

//#define ANCS_USED_CACHE_MSG			1

/*============================================================================
 *  Private Definitions
 *===========================================================================
 */

/* Following macros define the offset of various fields in Notification Source
 * Characteristic.
 */
#define ANCS_NS_OFFSET_EVENT_ID                       (0)
#define ANCS_NS_OFFSET_EVENT_FLAGS                    (1)
#define ANCS_NS_OFFSET_CAT_ID                         (2)
#define ANCS_NS_OFFSET_CAT_COUNT                      (3)
#define ANCS_NS_OFFSET_NOTIF_UUID                     (4)

/* Following macro define the data source header length */
#define ANCS_DS_HDR_LEN                               (5)

/* Maximum Length of the notification attribute
 * Note: Do not increase length beyond (DEFAULT_ATT_MTU -3 = 20)
 * octets as GAP service at the moment doesn't support handling of Prepare
 * write and Execute write procedures.
 */
#define ANCS_MAX_NOTIF_ATT_LENGTH                     (20)

#ifdef ANCS_USED_CACHE_MSG
#define ANCS_CACHE_BUF_LEN                           (1024)
#else
#define ANCS_MAX_ATTR_DATA_BUFFER_LENGTH             (64)
#endif

#define ANCS_REQUEST_WRITE_PENDING_MAX                (4)

/*============================================================================
 *  Private Data Types
 *===========================================================================
 */

/* Structure used in decoding and displaying Attribute data
 */
struct ancs_attribute_data_t {
	/* Attribute length - used to know the length of the attribute length */
	uint16_t attr_len;

	/* One more byte pending in the attribute length */
	bool pending_len;

	/* Attribute remaining/pending length - used to know the length of the
	 * remaining attribute length in the new data fragment
	 */
	uint16_t rem_attr_len;

	/* Attribute data pending - used to know the length of the
	 * remaining attribute data in the new data fragment
	 */
	uint16_t pending_attr_data;

	/* Attribute data state - used for decoding the attribute data
	 */
	uint16_t ds_decoder_state;

 /* Used to identify the command processed */
	uint16_t pending_cmd;

	/* Used to store the Attribute ID */
	uint16_t attr_id;
};


/* Structure used for sending Get Notification Attribute request to peer*/
struct ancs_nofif_attr_cmd_t {
	/* Data to store and send the Notification
	 * attribute request
	 */
	uint8_t data[ANCS_MAX_NOTIF_ATT_LENGTH + 1];
};

/* Structure used for sending Get Application Attribute request to peer */
struct ancs_app_attr_cmd_t {
	/* Data to store and send the Get App Attribute
	 * request
	 */
	uint8_t data[ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 3];

};

/* Structure used for storing last UUID notification data */
struct ancs_notif_uuid_data_t {
	/* Array to store last UUID data */
	uint8_t data[ANCS_NS_NOTIF_UUID_SIZE];

};

/* Structure used for special action */
struct ancs_reserve_action_data_t {
	uint8_t data[ANCS_RESERVE_ACTION_SIZE];
};

/*============================================================================
 *  Private Data
 *============================================================================
 */
/* Attribute data used for decoding and displaying peer notification data */
static struct ancs_attribute_data_t attribute_data;

/* Attribute data used for decoding and displaying peer application data */
static struct ancs_attribute_data_t app_attribute_data;

/* Used for sending Get Notification Attribute request data
 */
static struct ancs_nofif_attr_cmd_t notif_attr_req;

/* Used for sending Get Application Attribute request data
 */
static struct ancs_app_attr_cmd_t app_attr_req;

/* Used for storing the last UUID received */
static struct ancs_notif_uuid_data_t uuid_data;

static struct ancs_reserve_action_data_t action_data;

static bool app_id_req = 0;

#ifdef ANCS_USED_CACHE_MSG
struct ancs_cache_msg {
	uint8_t att_cnt;
	uint16_t buf_pos;
	struct {
		uint8_t att_id;
		uint16_t att_len;
		uint16_t data_len;
		uint8_t *att_data;
	} msg[ancd_notif_att_id_max];
	uint8_t buf[ANCS_CACHE_BUF_LEN + 1];
};

static struct ancs_cache_msg cache_msg_info;
#else
/* Buffer to hold the received ANCS data for display purpose */
static uint8_t g_ancs_data[ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 1];
static uint8_t g_ancs_appId_data[ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 1];
static uint8_t g_ancs_appName_data[ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 1];
#endif

static uint8_t ancs_cat_id;

struct bt_gatt_write_params ancs_gatt_write_params;
os_delayed_work ancs_gatt_write_timeout_work;

static uint8_t ancs_write_pending_flag = 0;

/*=============================================================================
 *  Private Function Prototypes
 *============================================================================
 */

/* This function handles the NS notifications */
bool ancs_handle_notification_source_data(struct bt_conn *conn, const uint8_t *notify_data, uint16_t length);

/* This function handles the DS notifications */
bool ancs_handle_data_source_data(struct bt_conn *conn, const uint8_t *notify_data, uint16_t length);

/* This function parses and displays the data source & ANCS notification data */
static bool ancs_parse_data(uint8_t *p_data, uint16_t size_value);

/* This function initialises the Attribute data structure */
//static void ancs_clear_attribute_data(void);

/*============================================================================
 *  Private Function Implementations
 *============================================================================
 */

/**
 * @brief This function parses and displays the DS notification data
 *
 * @return true, if valid else false.
 **/

const uint8_t * ancs_notif_att_id_s[ancd_notif_att_id_max] = {
    "App ID",
    "Title",
    "Sub Title",
    "Message",
    "Message Size",
    "Date",
};

const uint8_t * ancs_app_att_id_s[ancs_app_att_reserved] = {
	"Displya Name",
};

const uint8_t *ancs_event_id_notif_s[ancs_cat_id_max] = {
    "Added",
    "Modified",
    "Removed",
    "Reserved",
};

const uint8_t * ancs_category_id_s[ancs_cat_id_max] = {
    "Other",
    "Incoming call",
    "Missed call",
    "vmail",
    "social",
    "schedule",
    "email",
    "news",
    "hnf",
    "bnf",
    "location",
    "entertainment",
    "reserved",
};

#ifdef ANCS_USED_CACHE_MSG
static void ancs_cache_init(void)
{
	cache_msg_info.att_cnt = 0;
	cache_msg_info.buf_pos = 0;
	memset(cache_msg_info.msg, 0, sizeof(cache_msg_info.msg));
}

static void ancs_cache_add_att_id(uint8_t att_id)
{
	uint8_t pos;

	cache_msg_info.att_cnt++;
	if (cache_msg_info.att_cnt > ancd_notif_att_id_max) {
		cache_msg_info.att_cnt = ancd_notif_att_id_max;
		ancs_log_err("Too mush att\n");
	}

	pos = cache_msg_info.att_cnt - 1;
	cache_msg_info.msg[pos].att_id = att_id;
}

static void ancs_cache_add_att_len(uint16_t len)
{
	uint8_t pos = cache_msg_info.att_cnt - 1;

	cache_msg_info.msg[pos].att_len = len;
}

static void ancs_cache_add_att_data(uint8_t *data, uint16_t len)
{
	uint8_t pos = cache_msg_info.att_cnt - 1;
	uint16_t buf_pos = cache_msg_info.buf_pos;
	uint16_t data_pos;

	if ((cache_msg_info.buf_pos + len) > ANCS_CACHE_BUF_LEN) {
		ancs_log_err("Not enough buf\n");
		return;
	}

	if (cache_msg_info.msg[pos].att_data == NULL) {
		cache_msg_info.msg[pos].att_data = &cache_msg_info.buf[buf_pos];
	}

	data_pos = cache_msg_info.msg[pos].data_len;
	memcpy(&cache_msg_info.msg[pos].att_data[data_pos], data, len);
	cache_msg_info.msg[pos].data_len += len;
	cache_msg_info.buf_pos += len;
	cache_msg_info.buf[cache_msg_info.buf_pos] = '\0';
	if (cache_msg_info.msg[pos].data_len == cache_msg_info.msg[pos].att_len) {
		cache_msg_info.buf_pos++;
		cache_msg_info.buf[cache_msg_info.buf_pos] = '\0';
	}
}

static void ancs_cache_check_finish(void)
{
	uint8_t pos = cache_msg_info.att_cnt - 1;

	if ((cache_msg_info.msg[pos].att_id == ancs_notif_att_id_date) &&
		(cache_msg_info.msg[pos].data_len == cache_msg_info.msg[pos].att_len)) {
		if ((attribute_data.pending_attr_data == 0) &&
			 (attribute_data.attr_len == 0) && (!attribute_data.pending_len)) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_EVENT_NOTIFICATION_MSG_FINISH, 0);
		}
	}
}

static void ancs_app_atribute_cache_check_finish(void)
{
	uint8_t pos = cache_msg_info.att_cnt - 1;

	if ((cache_msg_info.msg[pos].att_id == ancs_app_att_display_name) &&
		(cache_msg_info.msg[pos].data_len == cache_msg_info.msg[pos].att_len)) {
		if ((attribute_data.pending_attr_data == 0) &&
			 (attribute_data.attr_len == 0) && (!attribute_data.pending_len)) {
			ble_ancs_ams_send_msg_to_app(BLE_ANCS_EVENT_NOTIFICATION_MSG_FINISH, 0);
		}
	}
}

static void ancs_cache_print()
{
	uint8_t i;

	ancs_log_inf("ANCS NOTIFY MSG\n");

	/* Wait TODO: Callback to app with message */
	for (i=0; i<cache_msg_info.att_cnt; i++) {
		ancs_log_inf("Attr ID = %s\n", ancs_notif_att_id_s[cache_msg_info.msg[i].att_id]);
		ancs_log_inf("Attr Len = 0x%02x\n", cache_msg_info.msg[i].att_len);
		if (cache_msg_info.msg[i].att_len) {
			ancs_log_inf("Attribute Data = %s\n", cache_msg_info.msg[i].att_data);
		}
	}

	ancs_cache_init();
}
#else
#define ancs_cache_init()
#define ancs_cache_add_att_id(a)
#define ancs_cache_add_att_len(a)
#define ancs_cache_check_finish()
#define ancs_app_atribute_cache_check_finish()
#define ancs_cache_print()
#endif

void ancs_notify_msg_finish(void)
{
	ancs_cache_print();
}

static bool ancs_parse_data(uint8_t *p_data, uint16_t size_value)
{
	uint16_t count = 0;
	uint8_t attrId = 0;
	uint16_t state = attribute_data.ds_decoder_state;
	uint16_t data_len = 0;
	bool b_skip_reserved = false;

	while (count < size_value) {
		if (b_skip_reserved) {
			break;
		}

		switch (state) {
		case ds_decoder_hdr:
			count = ANCS_DS_HDR_LEN;
			if (p_data[0] != 0) {
				ancs_log_warn("invalid command id\n");
				return false;
			}
			state = ds_decoder_attrid;
			break;

		case ds_decoder_attrid:
			/* Get the Attribute ID */
			attrId = p_data[count++];
			attribute_data.attr_id = attrId;

			ancs_log_inf("Attr ID = %s\n",ancs_notif_att_id_s[attrId]);

			switch (attrId) {
			case ancs_notif_att_id_app_id:
			case ancs_notif_att_id_title:
			case ancs_notif_att_id_subtitle:
			case ancs_notif_att_id_message:
			case ancs_notif_att_id_message_size:
			case ancs_notif_att_id_date:
                break;

			default:
				b_skip_reserved = true;
				break;
			}

			if (!b_skip_reserved) {
				state = ds_decoder_attrlen;
				ancs_cache_add_att_id(attrId);
			} else {
				state = ds_decoder_attrid;
				/* Invalid */
				attribute_data.attr_len = 0;
				attribute_data.attr_id = 0xff;
			}
			break;

		case ds_decoder_attrlen:
			if (attribute_data.pending_len) {
				/* Length was incomplete in the last fragment,
				 * so the first byte will complete the length
				 * data
				 */
				attribute_data.attr_len = ((p_data[count++] << 8) |
						 attribute_data.rem_attr_len);
				attribute_data.rem_attr_len = 0;
				attribute_data.pending_len = false;
			} else if ((count + 1) < size_value) {
				/* Get the Attribute length ( 2 bytes size) */
				attribute_data.attr_len = (p_data[count] |
													(p_data[count + 1] << 8));
				count += 2;
				attribute_data.pending_len = false;
			} else {
				attribute_data.rem_attr_len = p_data[count++];
				/* Length is 2 bytes, so copy the byte and wait for
				 * the next byte in the new fragment
				 */
				attribute_data.pending_len = true;
			}

			if (!attribute_data.pending_len) {
				ancs_log_inf("Attr Len = 0x%02x\n", attribute_data.attr_len);
#ifndef ANCS_USED_CACHE_MSG
				if (attribute_data.attr_len > 0) {
					ancs_log_inf("Attribute Data = ");
				}
#endif
				attribute_data.pending_attr_data = attribute_data.attr_len;
				ancs_cache_add_att_len(attribute_data.attr_len);
				state = ds_decoder_attrdata;
			}
			break;

		case ds_decoder_attrdata:
#ifdef ANCS_USED_CACHE_MSG
		{
			/* Data was incomplete in the last fragment */
			if (attribute_data.pending_attr_data) {
				/* Get the actual data length */
				data_len = size_value - count;
				/* If the received length is greater than
				 * the overall attribute length, just copy
				 * data till attribute length
				 */
				if (attribute_data.pending_attr_data < data_len) {
					data_len  = attribute_data.pending_attr_data;
				}

				ancs_cache_add_att_data(&p_data[count], data_len);
				/* Update till, what we have read */
				count += data_len;
				attribute_data.pending_attr_data -= data_len;
			}

			if (attribute_data.pending_attr_data == 0) {
				/* We are done reading data.Move to next attribute */
				state = ds_decoder_attrid;
				attribute_data.attr_len = 0;
				attribute_data.attr_id = 0xff;
			}
			break;
		}
#else
		{
			/* Data was incomplete in the last fragment */
			if (attribute_data.pending_attr_data) {
				/* Get the actual data length */
				data_len = size_value - count;
				/* If the received length is greater than
				 * the overall attribute length, just copy
				 * data till attribute length
				 */
				if (attribute_data.pending_attr_data < data_len) {
					data_len  = attribute_data.pending_attr_data;
				}

				/* Reset the g_ancs_data */

				memset(g_ancs_data, 0, ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 1);

				if(data_len > ANCS_MAX_ATTR_DATA_BUFFER_LENGTH)
				{
					data_len = ANCS_MAX_ATTR_DATA_BUFFER_LENGTH;
				}

				/* Copy the data */
				uint16_t i;
				for (i = 0; i < data_len; i++) {
					g_ancs_data[i] = p_data[count + i];
				}
				g_ancs_data[i] = '\0';

				if(attribute_data.attr_id == ancs_notif_att_id_app_id){
					memcpy(&g_ancs_appId_data, &g_ancs_data, sizeof(g_ancs_data));
				}

				/* Display to UART */
                if(data_len > 0){
				    ancs_log_inf("Data: %s\n",(const char *)&g_ancs_data[0]);
                }
				/* Update till, what we have read */
				count += data_len;
				attribute_data.pending_attr_data -= data_len;
			}

			if (attribute_data.pending_attr_data == 0) {
				/* We are done reading data.Move to next attribute */
				state = ds_decoder_attrid;
#if 0
				/* Invalid */
                if((ancs_cat_id == ancs_cat_id_incoming_call) && (attribute_data.attr_id == ancs_notif_att_id_title)){
                    uint16_t incomming_call_title_len = strlen(g_ancs_data);
                    if(incomming_call_title_len > 0){
                        bt_manager_event_notify(BT_ANCS_INCOMMING_CALL_EVENT, g_ancs_data,incomming_call_title_len);
                    }
                }
#endif
				attribute_data.attr_len = 0;
				attribute_data.attr_id = 0xff;
			}
			break;
		}
#endif
		}
	}

	attribute_data.ds_decoder_state = state;
	ancs_cache_check_finish();
#ifndef ANCS_USED_CACHE_MSG
	if ((0 == attribute_data.pending_attr_data) && 
		(ds_decoder_attrdata != attribute_data.ds_decoder_state)) {
		app_id_req = 1;
		ble_ancs_ams_send_msg_to_app(BLE_ANCS_EVENT_APP_ATTR_REQ, BLE_ANCS_APP_NO_UUID);
	}
#endif
	return true;
}

static bool ancs_parse_data_app_attribute(uint8_t *p_data, uint16_t size_value)
{
	uint16_t count = 0;
	uint8_t attrId = 0;
	uint16_t state = app_attribute_data.ds_decoder_state;
	uint16_t data_len = 0;
	bool b_skip_reserved = false;
	uint8_t cmd = 0;

	while (count < size_value) {
		if (b_skip_reserved) {
			break;
		}

		switch (state) {
		case ds_decoder_hdr:
			if (p_data[0] != 1) {
				ancs_log_warn("invalid command id\n");
				return false;
			}

			cmd = p_data[count++];
#ifndef ANCS_USED_CACHE_MSG
			uint8_t *appidentify;
			appidentify = &g_ancs_appId_data[0];
			count += strlen(g_ancs_appId_data) + 1;
			ancs_log_inf("App identify %s\n", (char*)appidentify);
#endif
			state = ds_decoder_attrid;
			break;

		case ds_decoder_attrid:
			/* Get the APP ID */
			attrId = p_data[count++];
			app_attribute_data.attr_id = attrId;

			ancs_log_inf("App ID = %s\n",ancs_app_att_id_s[attrId]);

			switch (attrId) {
			case ancs_app_att_display_name:
                break;

			default:
				b_skip_reserved = true;
				break;
			}

			if (!b_skip_reserved) {
				state = ds_decoder_attrlen;
				ancs_cache_add_att_id(attrId);
			} else {
				state = ds_decoder_attrid;
				/* Invalid */
				app_attribute_data.attr_len = 0;
				app_attribute_data.attr_id = 0xff;
			}
			break;

		case ds_decoder_attrlen:
			if (app_attribute_data.pending_len) {
				/* Length was incomplete in the last fragment,
				 * so the first byte will complete the length
				 * data
				 */
				app_attribute_data.attr_len = ((p_data[count++] << 8) |
						 app_attribute_data.rem_attr_len);
				app_attribute_data.rem_attr_len = 0;
				app_attribute_data.pending_len = false;
			} else if ((count + 1) < size_value) {
				/* Get the Attribute length ( 2 bytes size) */
				app_attribute_data.attr_len = (p_data[count] |
													(p_data[count + 1] << 8));
				count += 2;
				app_attribute_data.pending_len = false;
			} else {
				app_attribute_data.rem_attr_len = p_data[count++];
				/* Length is 2 bytes, so copy the byte and wait for
				 * the next byte in the new fragment
				 */
				app_attribute_data.pending_len = true;
			}

			if (!app_attribute_data.pending_len) {
				ancs_log_inf("App Len = 0x%02x\n", app_attribute_data.attr_len);
#ifndef ANCS_USED_CACHE_MSG
				if (app_attribute_data.attr_len > 0) {
					ancs_log_inf("App Data = ");
				}
#endif
				app_attribute_data.pending_attr_data = app_attribute_data.attr_len;
				ancs_cache_add_att_len(app_attribute_data.attr_len);
				state = ds_decoder_attrdata;
			}
			break;

		case ds_decoder_attrdata:
#ifdef ANCS_USED_CACHE_MSG
		{
			/* Data was incomplete in the last fragment */
			if (app_attribute_data.pending_attr_data) {
				/* Get the actual data length */
				data_len = size_value - count;
				/* If the received length is greater than
				 * the overall attribute length, just copy
				 * data till attribute length
				 */
				if (app_attribute_data.pending_attr_data < data_len) {
					data_len  = app_attribute_data.pending_attr_data;
				}

				ancs_cache_add_att_data(&p_data[count], data_len);
				/* Update till, what we have read */
				count += data_len;
				app_attribute_data.pending_attr_data -= data_len;
			}

			if (app_attribute_data.pending_attr_data == 0) {
				/* We are done reading data.Move to next attribute */
				state = ds_decoder_attrid;
				app_attribute_data.attr_len = 0;
				app_attribute_data.attr_id = 0xff;
			}
			break;
		}
#else
		{
			/* Data was incomplete in the last fragment */
			if (app_attribute_data.pending_attr_data) {
				/* Get the actual data length */
				data_len = size_value - count;
				/* If the received length is greater than
				 * the overall attribute length, just copy
				 * data till attribute length
				 */
				if (app_attribute_data.pending_attr_data < data_len) {
					data_len  = app_attribute_data.pending_attr_data;
				}

				/* Reset the g_ancs_app_data */

				memset(g_ancs_appName_data, 0, ANCS_MAX_ATTR_DATA_BUFFER_LENGTH + 1);

				if(data_len > ANCS_MAX_ATTR_DATA_BUFFER_LENGTH)
				{
					data_len = ANCS_MAX_ATTR_DATA_BUFFER_LENGTH;
				}

				/* Copy the data */
				uint16_t i;
				for (i = 0; i < data_len; i++) {
					g_ancs_appName_data[i] = p_data[count + i];
				}
				g_ancs_appName_data[i] = '\0';

				/* Display to UART */
                if(data_len > 0){
				    ancs_log_inf("App Data: %s\n",(const char *)&g_ancs_appName_data[0]);
                }
				/* Update till, what we have read */
				count += data_len;
				app_attribute_data.pending_attr_data -= data_len;
			}

			if (app_attribute_data.pending_attr_data == 0) {
				/* We are done reading data.Move to next attribute */
				state = ds_decoder_attrid;
				app_attribute_data.attr_len = 0;
				app_attribute_data.attr_id = 0xff;
			}
			break;
		}
#endif
		}
	}

	app_attribute_data.ds_decoder_state = state;
	ancs_app_atribute_cache_check_finish();
	return true;
}

/**
 *  @brief This function handles the NS notifications
 *
 *  @return true, if valid else false.
 */
bool ancs_handle_notification_source_data(struct bt_conn *conn, const uint8_t *notify_data, uint16_t length)
{
    uint8_t event_id;
    uint8_t event_flag;
    uint8_t cat_count;
    uint8_t cat_id = 0;
	bool notif_removed = false;
    uint32_t notif_uuid;
	/*  Notification Source Data format
	 * -------------------------------------------------------
	 * |  Event  |  Event  |  Cat  |  Cat   |  Notification  |
	 * |  ID     |  Flag   |  ID   |  Count |  UUID          |
	 * |---------------------------------------------------- |
	 * |   1B    |   1B    |   1B  |   1B   |   4B           |
	 * -------------------------------------------------------
	 */

	if (notify_data != NULL) {
		/* 1st byte of the Notification - Event ID */
		event_id = notify_data[ANCS_NS_OFFSET_EVENT_ID];

		if (event_id == ancs_event_id_notif_removed) {
			notif_removed = true;
		}

		/* 2nd byte of the Notification- Event Flags */
		event_flag = notify_data[ANCS_NS_OFFSET_EVENT_FLAGS];

        ancs_log_inf("Event ID = %s Event Flags = %d\n",ancs_event_id_notif_s[event_id],event_flag);
		
        if((event_flag & ANCS_NS_EVENTFLAG_PRE_EXISTING) != 0){
            return true;
        }

		/* 3rd byte of the Notification - Cat ID */
		cat_id = notify_data[ANCS_NS_OFFSET_CAT_ID];
        if(cat_id > ancs_cat_id_reserved){
            cat_id = ancs_cat_id_reserved;
        }

		/* 4rd byte of the Notification - Cat Count */
		cat_count = notify_data[ANCS_NS_OFFSET_CAT_COUNT];

		ancs_log_inf("Cat ID = %s Cat Count = %d\n",ancs_category_id_s[cat_id],notify_data[ANCS_NS_OFFSET_CAT_COUNT]);

		if (!notif_removed) {
			/* 5th to 8th bytes (4 bytes) of the Notification-Notification UUID */
			/* Clear the UUID notification buffer */
			memcpy(uuid_data.data, &notify_data[ANCS_NS_OFFSET_NOTIF_UUID], ANCS_NS_NOTIF_UUID_SIZE);
            notif_uuid = notify_data[ANCS_NS_OFFSET_NOTIF_UUID];
            notif_uuid += (notify_data[ANCS_NS_OFFSET_NOTIF_UUID + 1] << 8);
            notif_uuid += (notify_data[ANCS_NS_OFFSET_NOTIF_UUID + 2] << 16);
            notif_uuid += (notify_data[ANCS_NS_OFFSET_NOTIF_UUID + 3] << 24);
            ancs_cat_id = cat_id;
            /* Send Notification Attribute Request */
            ancs_log_inf("UUID = 0x%x\n", notif_uuid);
            ble_ancs_ams_send_msg_to_app(BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ,(int)notif_uuid);
        }
    }
    return true;
}

/**
 *  @brief This function handles the DS notifications
 *
 *  @return true, if valid else false.
 **/
bool ancs_handle_data_source_data(struct bt_conn *conn, const uint8_t *notify_data, uint16_t length)
{
	uint16_t count = 0;
	uint16_t state = attribute_data.ds_decoder_state;

	/* if cmd id = 0*/
	/* parse the data */
	if (attribute_data.pending_cmd == ancs_cmd_get_notification_att) {
		if (((notify_data[count] == ancs_cmd_get_notification_att) &&
			 (attribute_data.pending_attr_data == 0) &&
			 (attribute_data.attr_len == 0) && (!attribute_data.pending_len))) {
			if ((state == ds_decoder_hdr) || (state == ds_decoder_attrid)) {
				/* Wait TODO:  When one fragment end with complete attr,
				 * will trigger a new parse, but the message may not finish.
				 */
				attribute_data.ds_decoder_state = ds_decoder_hdr;
				ancs_cache_init();
			}
		}
		ancs_parse_data((uint8_t *)notify_data, length);
	} else if(attribute_data.pending_cmd == ancs_cmd_get_app_att){
		/* if cmd id = 1*/
		/* Check for APP ID */
		ancs_parse_data_app_attribute((uint8_t *)notify_data, length);
		app_id_req = 0;
	}else{

	}
	return true;
}

/**
 *  @brief This function checks if the UUID provided is one of the ANCS Characteristics
 * 
 *  @return ancs_type
 **/

extern void ancs_init_attribute_data(void)
{
	/* Reset the attribute data */
	attribute_data.ds_decoder_state = ds_decoder_hdr;
	attribute_data.attr_len = 0;
	attribute_data.pending_len = false;
	attribute_data.rem_attr_len = 0;
	attribute_data.pending_attr_data = 0;

	app_attribute_data.ds_decoder_state = ds_decoder_hdr;
	app_attribute_data.attr_len = 0;
	app_attribute_data.pending_len = false;
	app_attribute_data.rem_attr_len = 0;
	app_attribute_data.pending_attr_data = 0;
	ancs_cache_init();
}

/*============================================================================
 *  Public Function Implementations
 *===========================================================================
 */
extern void ancs_service_data_init(void)
{
    ancs_write_pending_flag = 0;
    ancs_cat_id = 0xFF;
	app_id_req = 0;
}

extern void ancs_incoming_call_handle(struct bt_conn *conn,uint8_t action)
{
	uint8_t *value = NULL;
	uint8_t count = 0;
	uint8_t i = 0;

	memset(action_data.data, 0, ANCS_RESERVE_ACTION_SIZE);
	value = &action_data.data[0];
    value[count++] = ancs_cmd_reserve_cmd;
   /* Copy the Notification UUID */
	for (i = 0; i < ANCS_NS_NOTIF_UUID_SIZE; i++){
		value[i + count] = uuid_data.data[i];
	}
	/* Add the length of UUID */
	count += ANCS_NS_NOTIF_UUID_SIZE;
    value[count++] = action;

	ancs_write_request(ancs_cmd_reserve_cmd, &action_data.data[0], count, conn);

	/* Set the command requested for */
	attribute_data.pending_cmd = ancs_cmd_reserve_cmd;
}

extern void ancs_get_application_attribute_cmd(struct bt_conn *conn)
{
	uint16_t count = 0;
	uint8_t *value = NULL;

	/* Clear the buffer each time */
	memset(app_attr_req.data, 0, sizeof(app_attr_req.data));

	value = &app_attr_req.data[0];

	/* Fill in the command -id (1) for the Application Attribute request */
	value[count++] = ancs_cmd_get_app_att;

#ifndef ANCS_USED_CACHE_MSG
	uint8_t len = strlen(g_ancs_appId_data);

	/* Copy the APP ID */
	for (uint16_t loop_count = 0; loop_count < MIN(len, (sizeof(g_ancs_appId_data) - 1)); loop_count++){
		value[loop_count + count] = g_ancs_appId_data[loop_count];
	}

	/* Add the length of APP ID */
	count += MIN(len, (sizeof(g_ancs_appId_data) - 1));
	value[count++] = '\0';
#endif

	value[count++] = ancs_app_att_display_name;

	ancs_write_request(ancs_cmd_get_app_att, &app_attr_req.data[0], count, conn);

	/* Set the command requested for */
	attribute_data.pending_cmd = ancs_cmd_get_app_att;
}

extern void ancs_get_notification_attribute_cmd(struct bt_conn *conn,uint32_t notify_uuid)
{
	uint16_t count = 0;
	uint16_t loop_count = 0;
	uint8_t *value = NULL;
	/* "ancs_notif_att_id_app_id" is not added as it is sent by default by the IOS
	 * device
	 */
	uint16_t features = (ancs_notif_att_id_title|ancs_notif_att_id_subtitle |
										ancs_notif_att_id_message|ancs_notif_att_id_message_size|
										ancs_notif_att_id_date);

#if 0
    if(cat_id == ancs_cat_id_incoming_call){
		ancs_incoming_call_handle(conn,ancs_cmd_accept_call);
    }
#endif

    ancs_log_inf("get_notification_attribute_cmd 0x%x\n", notify_uuid);

    uint16_t mtu = bt_manager_get_ble_mtu();

	/* Clear the buffer each time */
	memset(notif_attr_req.data, 0, ANCS_MAX_NOTIF_ATT_LENGTH + 1);

	value = &notif_attr_req.data[0];

	/* Fill in the command -id (0) for the Notification Attribute request */
	value[count++] = ancs_cmd_get_notification_att;

	/* Copy the Notification UUID */
	for (loop_count = 0; loop_count < ANCS_NS_NOTIF_UUID_SIZE; loop_count++){
//		value[loop_count + count] = uuid_data.data[loop_count];
        value[loop_count + count] = notify_uuid & 0xFF;
        notify_uuid = notify_uuid >> 8;
	}

	/* Add the length of UUID */
	count += ANCS_NS_NOTIF_UUID_SIZE;

	/* "ancs_notif_att_id_app_id" is sent by default, so no need to request the
	 * attribute separately. Attribute App Identifier has no length to be filled
	 * in
	 */
	 value[count++] = ancs_notif_att_id_app_id;
	/* Add Attribute ID for Title */
	value[count++] = ancs_notif_att_id_title;

	/* Add Attribute size for Title - 0x14 bytes requested */
//	value[count++] = 0x14;
    value[count++] = mtu - 3;
	value[count++] = 0;

	if (features & ancs_notif_att_id_subtitle) {
		/* Add Attribute ID for Sub title */
		value[count++] = ancs_notif_att_id_subtitle;

		/* Attribute size - 0x14 bytes requested */
		value[count++] = mtu - 3;
		value[count++] = 0;
	}

	if (features & ancs_notif_att_id_message) {
		/* Add Attribute ID for Message */
		value[count++] = ancs_notif_att_id_message;
		value[count++] = mtu - 3;
		value[count++] = 0;
	}

	if (features & ancs_notif_att_id_message_size) {
		/* Add Attribute ID for Message size */
		value[count++] = ancs_notif_att_id_message_size;
	}

	/* Add Attribute ID - Date is sent as UTF-35# support
	 *Date format : YYYYMMDD'T'HHMMSS
	 */
	value[count++] = ancs_notif_att_id_date;

	/* Notification Attribute Request format
	 * -------------------------------------------------------------------------
	 * |  CMD    |  Notification  |  Attr    |  Attr  |   Attr  |  Attr   | Attr|
	 * |  ID(0)  |  UUID          |  ID-1    |  ID-2  |   Len   |  ID-n   | Len |
	 * |         |                | (App ID) |        |         |         |     |
	 * |------------------------------------------------------------------------
	 * |   1B    |      4B        |   1B     |   1B   |   2B    |    1B   |  2B |
	 * --------------------------------------------------------------------------
	 */

	ancs_write_request(ancs_cmd_get_notification_att, &notif_attr_req.data[0], count, conn);

	/* Set the command requested for */
	attribute_data.pending_cmd = ancs_cmd_get_notification_att;
}

static struct bt_gatt_subscribe_params subscribe_params_ns;
static struct bt_gatt_subscribe_params subscribe_params_nc;
static uint8_t notify_func(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, uint16_t length)
{
	if (!data) {
		ancs_log_inf("[UNSUBSCRIBED]\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	ancs_log_inf("[NOTIFICATION] handle 0x%x length %u\n", params->value_handle, length);

	if (get_ancs_notification_handle() == params->value_handle) {
		if (1 == app_id_req) {
			ancs_log_inf("[NOTIFICATION] ERROR handle 0x%x length %u\n", params->value_handle, length);
			return BT_GATT_ITER_CONTINUE;
		}

		ancs_handle_notification_source_data(conn, data, length);
	} else if (params->value_handle == get_ancs_data_source_handle()) {
		ancs_handle_data_source_data(conn, data, length);
	}
	return BT_GATT_ITER_CONTINUE;
}

extern bool configure_ancs_notifications(struct bt_conn *conn,bool enable)
{
	int err;

	subscribe_params_ns.notify = notify_func;
	subscribe_params_ns.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_ns.ccc_handle = get_ancs_notification_ccd_handle();
	subscribe_params_ns.value_handle = get_ancs_notification_handle();
	atomic_set_bit(subscribe_params_ns.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	ancs_log_inf("subscribe ANCS N handle 0x%2x, 0x%2x %d\n",
	subscribe_params_ns.ccc_handle, subscribe_params_ns.value_handle, enable);
	if(enable){
        err = hostif_bt_gatt_subscribe(conn, &subscribe_params_ns);
        if (err && err != -EALREADY) {
            ancs_log_err("ANCS N subscribe failed (err %d)\n", err);
        }
        else {
            ancs_log_inf("ANCS N subscribe success\n");
        }
	}
	else{
        err = hostif_bt_gatt_unsubscribe(conn, &subscribe_params_ns);
        if (err) {
            ancs_log_err("ANCS N unsubscribe failed (err %d)\n", err);
        }
        else {
            ancs_log_inf("ANCS N unsubscribe success\n");
        }
	}
	return err;
}

extern bool configure_ancs_data_source_notification(struct bt_conn *conn,bool enable)
{
	int err;

	subscribe_params_nc.notify = notify_func;
	subscribe_params_nc.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_nc.ccc_handle = get_ancs_data_source_ccd_handle();
	subscribe_params_nc.value_handle = get_ancs_data_source_handle();
	atomic_set_bit(subscribe_params_nc.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	ancs_log_inf("subscribe ANCS DS handle 0x%2x, 0x%2x %d\n",
	    subscribe_params_nc.ccc_handle, subscribe_params_nc.value_handle, enable);
    if(enable){
        err = hostif_bt_gatt_subscribe(conn, &subscribe_params_nc);
        if (err && err != -EALREADY) {
            ancs_log_err("ANCS DS subscribe failed (err %d)\n", err);
        }
        else{
            ancs_log_inf("ANCS DS subscribe success\n");
        }
	}
	else{
        err = hostif_bt_gatt_unsubscribe(conn, &subscribe_params_nc);
        if (err) {
            ancs_log_err("ANCS DS unsubscribe failed (err %d)\n", err);
        }
        else {
            ancs_log_inf("ANCS DS unsubscribe success\n");
        }
	}
	return err;
}


bool ancs_handler_notif_ind(struct bt_conn *conn, uint16_t handle, const uint8_t *notify_data, uint16_t length)
{
	if (handle == get_ancs_notification_handle()) {  /* Notification has arrived */
		ancs_handle_notification_source_data(conn, notify_data, length);
		return true;
	} else if (handle == get_ancs_data_source_handle()) {
		/* Detailed data about the previous notification has arrived */
		ancs_handle_data_source_data(conn, notify_data, length);
		return false;
	} else {
		return false;
	}
}

bool does_handle_belongs_to_ancs_service(uint16_t handle)
{
	return ((handle >= get_remote_disc_ancs_service_start_handle()) &&
					(handle <= get_remote_disc_ancs_service_end_handle()))
					? true : false;
}

extern uint8_t ancs_get_write_pending_status(void)
{
    return ancs_write_pending_flag;
}

static void write_func(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{
	ancs_log_inf("ancs write complete: err %u\n", err);

	if(params->handle == get_ancs_control_point_handle()){
	    ble_ancs_ams_send_msg_to_app(BLE_ANCS_EVENT_NOTIFICATION_ATTR_REQ_COMPLETE,params->handle);
	}
	ancs_write_pending_flag = 0;
}

bool ancs_write_request(uint16_t type, uint8_t *data, uint16_t size, struct bt_conn *conn)
{
    if(ancs_write_pending_flag){
        ancs_log_err("ancs write busy!");
        return false;
    }

	ancs_gatt_write_params.data = data;
	ancs_gatt_write_params.length = size;
	ancs_gatt_write_params.handle = get_ancs_control_point_handle();
	ancs_gatt_write_params.offset = 0;
	ancs_gatt_write_params.func = write_func;

	if (hostif_bt_gatt_write(conn, &ancs_gatt_write_params) == 0) {
		ancs_write_pending_flag = 1;
	    ancs_log_inf("ancs write pending:%d handle:0x%x\n", ancs_write_pending_flag,get_ancs_control_point_handle());
		return true;
	}
	else{
	    return false;
	}
}

