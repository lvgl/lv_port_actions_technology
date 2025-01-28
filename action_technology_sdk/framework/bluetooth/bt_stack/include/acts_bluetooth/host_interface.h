/** @file
 * @brief Bluetooth host interface header file.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HOST_INTERFACE_H
#define __HOST_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/hci.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/l2cap.h>
#include <acts_bluetooth/sdp.h>
#include <acts_bluetooth/a2dp.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/hfp_hf.h>
#include <acts_bluetooth/hfp_ag.h>
#include <acts_bluetooth/avrcp_cttg.h>
#include <acts_bluetooth/spp.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/pbap_client.h>
#include <acts_bluetooth/map_client.h>
#include <acts_bluetooth/hid.h>
#include <acts_bluetooth/device_id.h>
#include <acts_bluetooth/pnp.h>

#define CONFIG_BT_HCI CONFIG_ACTS_BT_HCI

#define hostif_bt_addr_to_str(a, b, c)		bt_addr_to_str(a, b, c)
#define hostif_bt_addr_le_to_str(a, b, c)	bt_addr_le_to_str(a, b, c)
#define hostif_bt_addr_cmp(a, b)			bt_addr_cmp(a, b)
#define hostif_bt_addr_copy(a, b)			bt_addr_copy(a, b)

#ifdef CONFIG_BT_HCI
#define hostif_bt_read_ble_name(a, b)		bt_read_ble_name(a, b)
#else
static inline uint8_t hostif_bt_read_ble_name(uint8_t *name, uint8_t len)
{
	return 0;
}
#endif

/************************ hostif hci core *******************************/

/** @brief Increment a connection's reference count.
 *
 *  Increment the reference count of a connection object.
 *
 *  @note Will return NULL if the reference count is zero.
 *
 *  @param conn Connection object.
 *
 *  @return Connection object with incremented reference count, or NULL if the
 *          reference count is zero.
 */
struct bt_conn *hostif_bt_conn_ref(struct bt_conn *conn);

/** @brief Decrement a connection's reference count.
 *
 *  Decrement the reference count of a connection object.
 *
 *  @param conn Connection object.
 */
void hostif_bt_conn_unref(struct bt_conn *conn);

/** @brief Set bt device class
 *
 *  set bt device class. Must be the called before hostif_bt_enable.
 *
 *  @param classOfDevice bt device class
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_init_class(uint32_t classOfDevice);

/** @brief Set class of device
 *
 * @param classOfDevice class of device to set
 *
 * @return Zero if done successfully, other indicator failed.
 */
int hostif_bt_set_class_of_device(uint32_t classOfDevice);

/** @brief Initialize device id
 *
 * @param device id
 *
 * @return Zero if done successfully, other indicator failed.
 */
int hostif_bt_init_device_id(uint16_t *did);

/** @brief Enable Bluetooth
 *
 *  Enable Bluetooth. Must be the called before any calls that
 *  require communication with the local Bluetooth hardware.
 *
 *  @param cb Callback to notify completion or NULL to perform the
 *  enabling synchronously.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_enable(bt_ready_cb_t cb);

/** @brief Disable Bluetooth */
int hostif_bt_disable(void);

/** @brief enable BR/EDR pincode pair mode
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_br_enable_pincode(bool enable);

/** @brief Start BR/EDR discovery
 *
 *  Start BR/EDR discovery (inquiry) and provide results through the specified
 *  callback. When bt_br_discovery_cb_t is called it indicates that discovery
 *  has completed. If more inquiry results were received during session than
 *  fits in provided result storage, only ones with highest RSSI will be
 *  reported.
 *
 *  @param param Discovery parameters.
 *  @param cb Callback to notify discovery results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  bt_br_discovery_cb_t cb);

/** @brief Stop BR/EDR discovery.
 *
 *  Stops ongoing BR/EDR discovery. If discovery was stopped by this call
 *  results won't be reported
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_discovery_stop(void);

/** @brief Direct set scan mode.
 *
 * Can't used bt_br_write_scan_enable and
 * bt_br_set_discoverable/bt_br_set_discoverable at same time.
 *
 *  @param scan: scan mode.
 *
 *  @return Negative if fail set to requested state or requested state has been
 *  already set. Zero if done successfully.
 */
int hostif_bt_br_write_scan_enable(uint8_t scan);

/** @brief BR write iac.
 *
 * @param limit_iac, true: use limit iac, false: use general iac.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_iac(bool limit_iac);

/** @brief BR write page scan activity
 *
 * @param interval: page scan interval.
 * @param windown: page scan windown.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_page_scan_activity(uint16_t interval, uint16_t windown);

/** @brief BR write inquiry scan type
 *
 * @param type: 0:Standard Scan, 1:Interlaced Scan
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_inquiry_scan_type(uint8_t type);

/** @brief BR write page scan type
 *
 * @param type: 0:Standard Scan, 1:Interlaced Scan
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_page_scan_type(uint8_t type);

/** @brief Register br resolve address callback function.
 *
 * @param cb: callback function for resolve br address.
 *
 *  @return Zero on success or error code otherwise.
 */
int hostif_bt_br_reg_resolve_cb(bt_br_resolve_addr_t cb);

/** @brief request remote bluetooth name
 *
 * @param addr remote bluetooth mac address
 * @param cb callback function for report remote bluetooth name
 *
 * @return Zero if done successfully, other indicator failed.
 */
int hostif_bt_remote_name_request(const bt_addr_t *addr, bt_br_remote_name_cb_t cb);

/** @brief BR write inquiry scan activity
 *
 * @param interval: inquiry scan interval.
 * @param windown: inquiry scan windown.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_br_write_inquiry_scan_activity(uint16_t interval, uint16_t windown);

/************************ hostif conn *******************************/

/** @brief Set security level for a connection.
 *
 *  This function enable security (encryption) for a connection. If the device
 *  has bond information for the peer with sufficiently strong key encryption
 *  will be enabled. If the connection is already encrypted with sufficiently
 *  strong key this function does nothing.
 *
 *  If the device has no bond information for the peer and is not already paired
 *  then the pairing procedure will be initiated. If the device has bond
 *  information or is already paired and the keys are too weak then the pairing
 *  procedure will be initiated.
 *
 *  This function may return error if required level of security is not possible
 *  to achieve due to local or remote device limitation (e.g., input output
 *  capabilities), or if the maximum number of paired devices has been reached.
 *
 *  This function may return error if the pairing procedure has already been
 *  initiated by the local device or the peer device.
 *
 *  @note When @option{CONFIG_BT_SMP_SC_ONLY} is enabled then the security
 *        level will always be level 4.
 *
 *  @note When @option{CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY} is enabled then the
 *        security level will always be level 3.
 *
 *  @param conn Connection object.
 *  @param sec Requested security level.
 *
 *  @return 0 on success or negative error
 */
int hostif_bt_conn_set_security(struct bt_conn *conn, bt_security_t sec);

/** @brief Get security level for a connection.
 *
 *  @return Connection security level
 */
bt_security_t hostif_bt_conn_get_security(struct bt_conn *conn);

/** @brief Check connection security is start.
 *
 *  @return 0: security not start, 1: security is start.
 */
int hostif_bt_conn_security_is_start(struct bt_conn *conn);

/** @brief Register/unregister connection callbacks.
 *
 *  Register callbacks to monitor the state of connections.
 *
 *  @param cb Callback struct.
 */
void hostif_bt_conn_cb_register(struct bt_conn_cb *cb);
void hostif_bt_conn_cb_unregister(void);

/** @brief Get connection info
 *
 *  @param conn Connection object.
 *  @param info Connection info object.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info);

/** @brief Get connection type
 *
 *  @param conn Connection object.
 *
 *  @return connection type.
 */
uint8_t hostif_bt_conn_get_type(const struct bt_conn *conn);

/** @brief Get connection handle
 *
 *  @param conn Connection object.
 *
 *  @return connection acl handle.
 */
uint16_t hostif_bt_conn_get_handle(const struct bt_conn *conn);

/** @brief Get acl connection by sco connection
 *
 *  @param sco_conn Connection object.
 *
 *  @return acl connection.
 */
struct bt_conn *hostif_bt_conn_get_acl_conn_by_sco(const struct bt_conn *sco_conn);

/** @brief Disconnect from a remote device or cancel pending connection.
 *
 *  Disconnect an active connection with the specified reason code or cancel
 *  pending outgoing connection.
 *
 *  @param conn Connection to disconnect.
 *  @param reason Reason code for the disconnection.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_disconnect(struct bt_conn *conn, uint8_t reason);

/** @brief Initiate an BR/EDR connection to a remote device.
 *
 *  Allows initiate new BR/EDR link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param peer  Remote address.
 *  @param param Initial connection parameters.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *hostif_bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param);

/** @brief Check address is in connecting
 *
 *  @param peer  Remote address.
 *
 *  @return Valid connection object on success or NULL otherwise.
 */
struct bt_conn *hostif_bt_conn_br_acl_connecting(const bt_addr_t *addr);

/** @brief Initiate an SCO connection to a remote device.
 *
 *  Allows initiate new SCO link to remote peer using its address.
 *  Returns a new reference that the the caller is responsible for managing.
 *
 *  @param br_conn  br connection object.
 *
 *  @return Zero for success, non-zero otherwise..
 */
int hostif_bt_conn_create_sco(struct bt_conn *br_conn);

/** @brief Send sco data to conn.
 *
 *  @param conn  Sco connection object.
 *  @param data  Sco data pointer.
 *  @param len  Sco data length.
 *
 *  @return  Zero for success, non-zero otherwise..
 */
int hostif_bt_conn_send_sco_data(struct bt_conn *conn, uint8_t *data, uint8_t len);

/** @brief bluetooth conn enter sniff
 *
 * @param conn bt_conn conn
 * @param min_interval Minimum sniff interval.
 * @param min_interval Maxmum sniff interval.
 */
int hostif_bt_conn_check_enter_sniff(struct bt_conn *conn, uint16_t min_interval, uint16_t max_interval);

/** @brief bluetooth conn check and exit sniff
 *
 * @param conn bt_conn conn
 */
void hostif_bt_conn_check_exit_sniff(struct bt_conn *conn);

/** @brief bluetooth conn get rxtx count
 *
 * @param conn bt_conn conn
 *
 *  @return  rx/tx count.
 */
uint16_t hostif_bt_conn_get_rxtx_cnt(struct bt_conn *conn);

/** @brief Bt send connectionless data.
 *
 *  @param conn  Connection object.
 *  @param data send data.
 *  @param len data len.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_send_connectionless_data(struct bt_conn *conn, uint8_t *data, uint16_t len);

/** @brief check br conn ready for send data.
 *
 *  @param conn  Connection object.
 *
 *  @return  0: no ready , 1:  ready.
 */
int hostif_bt_br_conn_ready_send_data(struct bt_conn *conn);

/** @brief Get conn pending packet.
 *
 * @param conn  Connection object.
 * @param host_pending Host pending packet.
 * @param controler_pending Controler pending packet.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_br_conn_pending_pkt(struct bt_conn *conn, uint8_t *host_pending, uint8_t *controler_pending);


/** @brief Register conn tx pending complete callback
 *
 * @param conn  Connection object.
 * @param cb Callback function.
 *
 *  @return  None.
 */
void hostif_bt_conn_reg_tx_pending_cb(struct bt_conn *conn, bt_tx_pending_cb cb);

/** @brief register BR/LE auth cb.
 *
 * @param cb  call back funcion.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb);
int hostif_bt_conn_le_auth_cb_register(const struct bt_conn_auth_cb *cb);

/** @brief auto passkey confirm.
 *
 * @param conn  Connection object.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_conn_auth_passkey_confirm(struct bt_conn *conn);

/** @brief auto pairing confirm.
 *
 * @param conn  Connection object.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_conn_auth_pairing_confirm(struct bt_conn *conn);

/** @brief auto cancel.
 *
 * @param conn  Connection object.
 *
 *  @return  0: sucess, other:  error.
 */
int hostif_bt_conn_auth_cancel(struct bt_conn *conn);

/** @brief Bt role discovery.
 *
 *  @param conn  Connection object.
 *  @param role  return role.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_role_discovery(struct bt_conn *conn, uint8_t *role);

/** @brief Bt switch role.
 *
 *  @param conn  Connection object.
 *  @param role  Switch role.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_switch_role(struct bt_conn *conn, uint8_t role);

/** @brief Bt set supervision timeout.
 *
 *  @param conn  Connection object.
 *  @param timeout.
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_set_supervision_timeout(struct bt_conn *conn, uint16_t timeout);

/** @brief Get conn RSSI.
 *
 *  @param conn  Connection object.
 *  @param *rssi. output RSSI
 *
 *  @return  Zero for success, non-zero otherwise.
 */
int hostif_bt_conn_read_rssi(struct bt_conn *conn, int8_t *rssi);

/************************ hostif hfp *******************************/
/** @brief Register HFP HF call back function
 *
 *  Register Handsfree callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_register_cb(struct bt_hfp_hf_cb *cb);

/** @brief Handsfree client Send AT
 *
 *  Send specific AT commands to handsfree client profile.
 *
 *  @param conn Connection object.
 *  @param user_cmd user command
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_send_cmd(struct bt_conn *conn, char *user_cmd);

/** @brief Handsfree connect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_connect(struct bt_conn *conn);

/** @brief Handsfree disconnect AG.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_hf_disconnect(struct bt_conn *conn);

/************************ hostif hfp *******************************/
/** @brief Register HFP AG call back function
 *
 *  Register Handsfree callbacks to monitor the state and get the
 *  required HFP details to display.
 *
 *  @param cb callback structure.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_register_cb(struct bt_hfp_ag_cb *cb);

/** @brief Update hfp ag indicater
 *
 *  @param index (enum hfp_hf_ag_indicators ).
 *  @param value indicater value for index.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_update_indicator(uint8_t index, uint8_t value);

/** @brief get hfp ag indicater
 *
 *  @param index (enum hfp_hf_ag_indicators ).
 *
 *  @return 0 and positive in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_get_indicator(uint8_t index);

/** @brief hfp ag send event to hf
 *
 *  @param conn  Connection object.
 *  @param event Event char string.
 * @param len Event char string length.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_send_event(struct bt_conn *conn, char *event, uint16_t len);

/** @brief HFP AG connect HF.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_connect(struct bt_conn *conn);

/** @brief HFP AG disconnect HF.
 *
 *  @param conn Connection object.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_hfp_ag_disconnect(struct bt_conn *conn);

/** @brief HFP AG display current indicators. */
void hostif_bt_hfp_ag_display_indicator(void);

/************************ hostif a2dp *******************************/
/** @brief A2DP Connect.
 *
 *  @param conn Pointer to bt_conn structure.
 *  @param role a2dp as source or sink role
 *
 *  @return 0 in case of success and error code in case of error.
 *  of error.
 */
int hostif_bt_a2dp_connect(struct bt_conn *conn, uint8_t role);

/** @brief A2DP Disconnect.
 *
 *  @param conn Pointer to bt_conn structure.
 *
 *  @return 0 in case of success and error code in case of error.
 *  of error.
 */
int hostif_bt_a2dp_disconnect(struct bt_conn *conn);

/** @brief Endpoint Registration.
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param media_type Media type that the Endpoint is.
 *  @param role Role of Endpoint.
 *
 *  @return 0 in case of success and error code in case of error.
 */
int hostif_bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      uint8_t media_type, uint8_t role);

/** @brief halt/resume registed endpoint.
 *
 *  This function is used for halt/resume registed endpoint
 *
 *  @param endpoint Pointer to bt_a2dp_endpoint structure.
 *  @param halt true: halt , false: resume;
 *
 *  @return 0 in case of success and error code in case of error.
 */
int hostif_bt_a2dp_halt_endpoint(struct bt_a2dp_endpoint *endpoint, bool halt);

/* Register app callback */
int hostif_bt_a2dp_register_cb(struct bt_a2dp_app_cb *cb);

/* Start a2dp play */
int hostif_bt_a2dp_start(struct bt_conn *conn);

/* Suspend a2dp play */
int hostif_bt_a2dp_suspend(struct bt_conn *conn);

/* Reconfig a2dp codec config */
int hostif_bt_a2dp_reconfig(struct bt_conn *conn, struct bt_a2dp_media_codec *codec);

/* Send delay report to source, delay_time: 1/10 milliseconds */
int hostif_bt_a2dp_send_delay_report(struct bt_conn *conn, uint16_t delay_time);

/* Send a2dp audio data */
int hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, uint8_t *data, uint16_t len);

/* Get a2dp seted codec */
struct bt_a2dp_media_codec *hostif_bt_a2dp_get_seted_codec(struct bt_conn *conn);

/* Get a2dp role(source or sink) */
uint8_t hostif_bt_a2dp_get_a2dp_role(struct bt_conn *conn);

/** @brief Get connection direction
 *
 *  @param conn Connection object.
 *
 *  @return connection direction.
 */
uint8_t hostif_bt_conn_get_direction(struct bt_conn *conn);

/* Get a2dp media tx mtu */
uint16_t hostif_bt_a2dp_get_a2dp_media_mtu(struct bt_conn *conn);

/* Send a2dp audio data with callback*/
int hostif_bt_a2dp_send_audio_data_with_cb(struct bt_conn *conn, u8_t *data, u16_t len, void(*cb)(struct bt_conn *, void *));


/************************ hostif spp *******************************/
/* Spp send data */
int hostif_bt_spp_send_data(uint8_t spp_id, uint8_t *data, uint16_t len);

/* Spp connect */
uint8_t hostif_bt_spp_connect(struct bt_conn *conn, uint8_t *uuid);

/* Spp disconnect */
int hostif_bt_spp_disconnect(uint8_t spp_id);

/* Spp register service */
uint8_t hostif_bt_spp_register_service(uint8_t *uuid);

/* Spp register call back */
int hostif_bt_spp_register_cb(struct bt_spp_app_cb *cb);

/* Spp sdp service restore, use to btreset*/
int hostif_bt_spp_service_restore(void);

/************************ hostif avrcp *******************************/
/* Register avrcp app callback function */
int hostif_bt_avrcp_cttg_register_cb(struct bt_avrcp_app_cb *cb);

/* Connect to avrcp TG device */
int hostif_bt_avrcp_cttg_connect(struct bt_conn *conn);

/* Disonnect avrcp */
int hostif_bt_avrcp_cttg_disconnect(struct bt_conn *conn);

/* Send avrcp control key */
int hostif_bt_avrcp_ct_pass_through_cmd(struct bt_conn *conn, avrcp_op_id opid, bool push);

/* Target notify change to controller */
int hostif_bt_avrcp_tg_notify_change(struct bt_conn *conn, uint8_t volume);

/* get current playback track id3 info */
int hostif_bt_avrcp_ct_get_id3_info(struct bt_conn *conn);

int hostif_bt_avrcp_ct_get_playback_pos(struct bt_conn *conn);

int hostif_bt_avrcp_ct_set_absolute_volume(struct bt_conn *conn, uint32_t param);

bool hostif_bt_avrcp_ct_check_event_support(struct bt_conn *conn, uint8_t event_id);

int hostif_bt_pts_avrcp_ct_get_capabilities(struct bt_conn *conn);

int hostif_bt_avrcp_ct_get_play_status(struct bt_conn *conn);

int hostif_bt_pts_avrcp_ct_register_notification(struct bt_conn *conn);

/************************ hostif pbap *******************************/
uint8_t hostif_bt_pbap_client_connect(struct bt_conn *conn, struct bt_pbap_client_user_cb *cb);
int hostif_bt_pbap_client_disconnect(struct bt_conn *conn, uint8_t user_id);
int hostif_bt_pbap_client_op(struct stack_pbap_op_param *stack_param);

/************************ hostif map *******************************/
uint8_t hostif_bt_map_client_connect(struct bt_conn *conn,char *path,struct bt_map_client_user_cb *cb);

int hostif_bt_map_client_abort_get(struct bt_conn *conn, uint8_t user_id);

int hostif_bt_map_client_disconnect(struct bt_conn *conn, uint8_t user_id);

int hostif_bt_map_client_set_folder(struct bt_conn *conn, uint8_t user_id,char *path,uint8_t flags);

uint8_t hostif_bt_map_client_get_message(struct bt_conn *conn, char *path, struct bt_map_client_user_cb *cb);

int hostif_bt_map_client_get_messages_listing(struct bt_conn *conn, uint8_t user_id,uint16_t max_list_count,uint32_t parameter_mask);

int hostif_bt_map_client_get_folder_listing(struct bt_conn *conn, uint8_t user_id);

/************************ hostif hid *******************************/
/* register sdp hid service record */
int hostif_bt_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,int attrs_size);

/* Register hid app callback function */
int hostif_bt_hid_register_cb(struct bt_hid_app_cb *cb);

/* Connect to hid device */
int hostif_bt_hid_connect(struct bt_conn *conn);

/* Disonnect hid */
int hostif_bt_hid_disconnect(struct bt_conn *conn);

/* send hid data by ctrl channel */
int hostif_bt_hid_send_ctrl_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len);

/* send hid data by intr channel */
int hostif_bt_hid_send_intr_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len);

/* send hid rsp by ctrl channel */
int hostif_bt_hid_send_rsp(struct bt_conn *conn,uint8_t status);

/* register device id info on sdp */
int hostif_bt_did_register_sdp(struct device_id_info *device_id);

/************************ hostif ble *******************************/
/** @brief Start advertising
 *
 *  Set advertisement data, scan response data, advertisement parameters
 *  and start advertising.
 *
 *  @param param Advertising parameters.
 *  @param ad Data to be used in advertisement packets.
 *  @param ad_len Number of elements in ad
 *  @param sd Data to be used in scan response packets.
 *  @param sd_len Number of elements in sd
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);

/** @brief Stop advertising
 *
 *  Stops ongoing advertising.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_le_adv_stop(void);


/** @brief Reset advertising
 *
 *  Reset advertising.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hostif_bt_le_adv_reset(void);

/** @brief get le adv status.
 *
 *  get le adv status.
 *  @param status 0 adv off, 1 adv on
 
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_get_adv_status(uint8_t* status);

/** @brief Update the connection parameters.
 *
 *  @param conn Connection object.
 *  @param param Updated connection parameters.
 *
 *  @return Zero on success or (negative) error code on failure.
 */
int hostif_bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param);

/** @brief check le conn ready for send data.
 *
 *  @param conn  Connection object.
 *
 *  @return  0: no ready , 1:  ready.
 */
int hostif_bt_le_conn_ready_send_data(struct bt_conn *conn);

/** @brief Get le tx pending cnt.
 *
 *  @param conn  Connection object.
 *
 *  @return  Number of pendind send(include host and controler).
 */
uint16_t hostif_bt_le_tx_pending_cnt(struct bt_conn *conn);

/** @brief Register GATT service.
 *
 *  Register GATT service. Applications can make use of
 *  macros such as BT_GATT_PRIMARY_SERVICE, BT_GATT_CHARACTERISTIC,
 *  BT_GATT_DESCRIPTOR, etc.
 *
 *  @param svc Service containing the available attributes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_service_register(struct bt_gatt_service *svc);

/** @brief ATT unregister ble service
 *
 *  @param struct bt_gatt_service.
 *
 *  @return 0: ok , other: fail.
 */
int hostif_bt_gatt_service_unregister(struct bt_gatt_service *svc);

/** @brief Get ATT MTU for a connection
 *
 *  Get negotiated ATT connection MTU, note that this does not equal the largest
 *  amount of attribute data that can be transferred within a single packet.
 *
 *  @param conn Connection object.
 *
 *  @return MTU in bytes
 */
uint16_t hostif_bt_gatt_get_mtu(struct bt_conn *conn);

/** @brief Indicate attribute value change.
 *
 *  Send an indication of attribute value change.
 *  Note: This function should only be called if CCC is declared with
 *  BT_GATT_CCC otherwise it cannot find a valid peer configuration.
 *
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Indicate parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params);

/** @brief Notify attribute value change.
 *
 *  Send notification of attribute value change, if connection is NULL notify
 *  all peer that have notification enabled via CCC otherwise do a direct
 *  notification only the given connection.
 *
 *  The attribute object must be the so called Characteristic Value Descriptor,
 *  its usually declared with BT_GATT_DESCRIPTOR after BT_GATT_CHARACTERISTIC
 *  and before BT_GATT_CCC.
 *
 *  @param conn Connection object.
 *  @param attr Characteristic Value Descriptor attribute.
 *  @param data Pointer to Attribute data.
 *  @param len Attribute value length.
 */
int hostif_bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   const void *data, uint16_t len);

/** @brief Exchange MTU
 *
 *  This client procedure can be used to set the MTU to the maximum possible
 *  size the buffers can hold.
 *
 *  NOTE: Shall only be used once per connection.
 *
 *  @param conn Connection object.
 *  @param params Exchange MTU parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params);

#ifdef CONFIG_GATT_OVER_BREDR
/** @brief Register GATT OVER BREDR connected callback function.
 *
 *  @param cb callback function.
 *
 */
void hostif_bt_gobr_reg_connected_cb(bt_gobr_connected_cb_t cb, bt_gatt_connect_status_cb_t status_cb);
#endif

/** @brief GATT Discover function
 *
 *  This procedure is used by a client to discover attributes on a server.
 *
 *  Primary Service Discovery: Procedure allows to discover specific Primary
 *                             Service based on UUID.
 *  Include Service Discovery: Procedure allows to discover all Include Services
 *                             within specified range.
 *  Characteristic Discovery:  Procedure allows to discover all characteristics
 *                             within specified handle range as well as
 *                             discover characteristics with specified UUID.
 *  Descriptors Discovery:     Procedure allows to discover all characteristic
 *                             descriptors within specified range.
 *
 *  For each attribute found the callback is called which can then decide
 *  whether to continue discovering or stop.
 *
 *  @note This procedure is asynchronous therefore the parameters need to
 *        remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Discover parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_discover(struct bt_conn *conn, struct bt_gatt_discover_params *params);

/** @brief Read Attribute Value by handle
 *
 *  This procedure read the attribute value and return it to the callback.
 *
 *  When reading attributes by UUID the callback can be called multiple times
 *  depending on how many instances of given the UUID exists with the
 *  start_handle being updated for each instance.
 *
 *  If an instance does contain a long value which cannot be read entirely the
 *  caller will need to read the remaining data separately using the handle and
 *  offset.
 *
 *  @note This procedure is asynchronous therefore the parameters need to
 *        remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Read parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params);

/** @brief Write Attribute Value by handle
 *
 *  This procedure write the attribute value and return the result in the
 *  callback.
 *
 *  @note This procedure is asynchronous therefore the parameters need to
 *        remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Write parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params);

/** @brief Subscribe Attribute Value Notification
 *
 *  This procedure subscribe to value notification using the Client
 *  Characteristic Configuration handle.
 *  If notification received subscribe value callback is called to return
 *  notified value. One may then decide whether to unsubscribe directly from
 *  this callback. Notification callback with NULL data will not be called if
 *  subscription was removed by this method.
 *
 *  @note Notifications are asynchronous therefore the parameters need to
 *        remain valid while subscribed.
 *
 *  @param conn Connection object.
 *  @param params Subscribe parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params);

/** @brief Unsubscribe Attribute Value Notification
 *
 *  This procedure unsubscribe to value notification using the Client
 *  Characteristic Configuration handle. Notification callback with NULL data
 *  will be called if subscription was removed by this call, until then the
 *  parameters cannot be reused.
 *
 *  @param conn Connection object.
 *  @param params Subscribe parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int hostif_bt_gatt_unsubscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params);

/** @brief Start (LE) scanning
 *
 *  Start LE scanning with given parameters and provide results through
 *  the specified callback.
 *
 *  @param param Scan parameters.
 *  @param cb Callback to notify scan results.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb);

/** @brief Stop (LE) scanning.
 *
 *  Stops ongoing LE scanning.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_scan_stop(void);

/** @brief get le mac.
 *
 *  get le mac addr.
 *
 *  @return Zero on success or error code otherwise, positive in case
 *  of protocol error or negative (POSIX) in case of stack internal error
 */
int hostif_bt_le_get_mac(bt_addr_le_t *le_addr);

/** @brief Register property flush call back function
 *
 *  @param cb flush call back function.
 *
 *  @return Zero on success or error code otherwise, positive in case
 */
int hostif_reg_flush_nvram_cb(void *cb);

/** @brief Get store linkkey
 *
 *  @param info save get linkkey information.
 *  @param cnt number of info for save linkkey.
 *
 *  @return number of get linkkey
 */
int hostif_bt_store_get_linkkey(struct br_linkkey_info *info, uint8_t cnt);

/** @brief Update store linkkey
 *
 *  @param info linkkey for update.
 *  @param cnt number of linkkey for update.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_store_update_linkkey(struct br_linkkey_info *info, uint8_t cnt);

/** @brief Store original linkkey
 *
 *  @param addr bluetooht address.
 *  @param link_key original linkkey.
 *
 *  @return NONE.
 */
void hostif_bt_store_write_ori_linkkey(bt_addr_t *addr, uint8_t *link_key);

/** @brief Clear store linkkey
 *
 *  @param addr  NULL: clear all, addr: clear one.
 *
 *  @return NONE.
 */
void hostif_bt_store_clear_linkkey(const bt_addr_t *addr);

/** @brief find linkkey by addr
 *
 *  @param addr bluetooht address.
 *  @param key for save linkkey.
 *
 *  @return Zero on success or error code.
 */
int hostif_bt_store_find_linkkey(const bt_addr_t *addr,uint8_t *key);

/** @brief find le linkkey by addr
 *
  * @param id 
 *  @param addr bluetooht address.
 *
 *  @return Zero on success or error code.
 */
int hostif_bt_le_find_linkkey(const bt_addr_le_t *addr);

/** @brief Clear le store linkkey
 *
 *  @param addr  NULL: clear all, addr: clear one.
 *
 *  @return NONE.
 */
void hostif_bt_le_clear_linkkey(const bt_addr_le_t *addr);

/** @brief Get le linkkey num
 *
 *  @param NULL;
 *
 *  @return store le linkkey num.
 */
uint8_t hostif_bt_le_get_linkkey_mum(void);

/** @brief Get and dump bt stack version infomation
 *
 *  @param NULL;
 *
 *  @return store le linkkey num.
 */
static inline uint32_t hostif_libbtstack_version_dump(void)
{
	extern uint32_t libbtstack_version_dump(void);
	return libbtstack_version_dump();
}

/** @brief CSB interface */
int hostif_bt_csb_set_broadcast_link_key_seed(uint8_t *seed, uint8_t len);
int hostif_bt_csb_set_reserved_lt_addr(uint8_t lt_addr);
int hostif_bt_csb_set_CSB_broadcast(uint8_t enable, uint8_t lt_addr, uint16_t interval_min, uint16_t interval_max);
int hostif_bt_csb_set_CSB_receive(uint8_t enable, struct bt_hci_evt_sync_train_receive *param, uint16_t timeout);
int hostif_bt_csb_write_sync_train_param(uint16_t interval_min, uint16_t interval_max, uint32_t trainTO, uint8_t service_data);
int hostif_bt_csb_start_sync_train(void);
int hostif_bt_csb_set_CSB_date(uint8_t lt_addr, uint8_t *data, uint8_t len);
int hostif_bt_csb_receive_sync_train(uint8_t *mac, uint16_t timeout, uint16_t windown, uint16_t interval);
int hostif_bt_csb_broadcase_by_acl(uint16_t handle, uint8_t *data, uint16_t len);

/** @brief Adjust link time
 *
 *  @param conn Connection object..
 *  @param time Adjust time.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_adjust_link_time(struct bt_conn *conn, int16_t time);

/** @brief get conn bt clock
 *
 *  @param conn Connection object..
 *  @param bt_clk Output bt clock time by 312.5us.
 *  @param bt_intraslot Output bt clock time by 1us.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_get_bt_clock(struct bt_conn *conn, uint32_t *bt_clk, uint16_t *bt_intraslot);

/** @brief set conn tws interrupt clock
 *
 *  @param conn Connection object.
 *  @param bt_clk bt clock time by 312.5us.
 *  @param bt_intraslot bt clock time by 1us.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_set_tws_int_clock(struct bt_conn *conn, uint32_t bt_clk, uint16_t bt_intraslot);

/** @brief set tws interrupt enable
 *
 *  @param enable Eanble/disable tws interrupt.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_enable_tws_int(uint8_t enable);

/** @brief set acl flow control enable/disable(Just for test)
 *
 *  @param enable Eanble/disable acl flow control.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_enable_acl_flow_control(struct bt_conn *conn, uint8_t enable);

/** @brief Read how many null packet add for sync link(Just for test)
 *
 *  @param conn Connection object.
 *  @param *cnt Output add null packet number.
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_read_add_null_cnt(struct bt_conn *conn, uint16_t *cnt);

/* reserved */
int hostif_bt_vs_enable_tws_int_new(uint8_t enable, uint8_t index);

/* reserved */
int hostif_bt_vs_set_tws_sync_int_time(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint32_t bt_clk, uint16_t bt_slot);

/* reserved */
int hostif_bt_vs_set_tws_int_delay_play_time(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint8_t per_int, uint32_t bt_clk, uint16_t bt_slot);

/* reserved */
int hostif_bt_vs_set_tws_int_clock_new(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint32_t clk, uint16_t slot, uint32_t per_clk, uint16_t per_slot);

/** @brief Get locat bt clock
 *
 *  @param *cnt Output bt clock us
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_read_bt_us_cnt(uint32_t *cnt);

/** @brief Enable/disable apll temperature compensate
 *
 *  @param enable 0:disable, 1:enable
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_set_apll_temp_comp(uint8_t enable);

/** @brief Do apll temperature compensate
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_vs_do_apll_temp_comp(void);


/** @brief Register sdp pnp information callback function.
 *
 * @param cb: callback function for pnp info.
 *
 *  @return Zero on success or error code otherwise.
 */
int hostif_bt_pnp_info_register_cb(struct bt_pnp_info_cb *cb);

/** @brief Search pnp information sevice.
 *
 *  @param conn Connection object.
 *
 *  @return Zero on success or error code otherwise.
 */
int hostif_bt_pnp_info_search(struct bt_conn *conn);

/** @brief Enable a2dp connectable
 *
 *  @param default 0, reserve
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_a2dp_enable(uint32_t param);

/** @brief Disable a2dp connectable
 *
 *  @param default 0, reserve
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_a2dp_disable(uint32_t param);

/** @brief Enable avrcp connectable
 *
 *  @param default 0, reserve
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_avrcp_enable(uint32_t param);

/** @brief Disable avrcp connectable
 *
 *  @param default 0, reserve
 *
 *  @return 0: successful, other failed.
 */
int hostif_bt_avrcp_disable(uint32_t param);

int hostif_bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync);

int hostif_bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			const struct bt_le_ext_adv_cb *cb,
			struct bt_le_ext_adv **out_adv);

int hostif_bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
				const struct bt_le_per_adv_param *param);

int hostif_bt_le_per_adv_start(struct bt_le_ext_adv *adv);

int hostif_bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_start_param *param);

void hostif_bt_le_scan_cb_register(struct bt_le_scan_cb *cb);

void hostif_bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb);

int hostif_bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
					struct bt_le_per_adv_sync **out_sync);

int hostif_bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
				const struct bt_data *ad, size_t ad_len,
				const struct bt_data *sd, size_t sd_len);

int hostif_bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
				const struct bt_data *ad, size_t ad_len);

void hostif_bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb);

void hostif_bt_le_per_adv_sync_cb_unregister(struct bt_le_per_adv_sync_cb *cb);

int hostif_bt_le_per_adv_stop(struct bt_le_ext_adv *adv);

int hostif_bt_le_ext_adv_stop(struct bt_le_ext_adv *adv);

int hostif_bt_le_ext_adv_delete(struct bt_le_ext_adv *adv);

#endif /* __HOST_INTERFACE_H */
