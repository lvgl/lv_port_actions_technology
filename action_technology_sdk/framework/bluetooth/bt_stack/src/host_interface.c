/** @file
 * @brief Bluetooth host interface.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <acts_bluetooth/host_interface.h>
#include "keys_br_store.h"
#include "common_internal.h"
#include "keys.h"

static int hostif_set_negative_prio(void)
{
	int prio = 0;
	if(!k_is_in_isr()){
		prio = k_thread_priority_get(k_current_get());
		if (prio >= 0) {
			k_thread_priority_set(k_current_get(), -1);
		}
	}

	return prio;
}

static void hostif_revert_prio(int prio)
{
	if(!k_is_in_isr()){
		if (prio >= 0) {
			k_thread_priority_set(k_current_get(), prio);
		}
	}
}

struct bt_conn *hostif_bt_conn_ref(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	int prio;
	struct bt_conn *ret_conn;

	prio = hostif_set_negative_prio();
	ret_conn = bt_conn_ref(conn);
	hostif_revert_prio(prio);

	return ret_conn;
#else
	return NULL;
#endif
}

void hostif_bt_conn_unref(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_unref(conn);
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_init_class(uint32_t classOfDevice)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_init_class_of_device(classOfDevice);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_set_class_of_device(uint32_t classOfDevice)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_set_class_of_device(classOfDevice);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_init_device_id(uint16_t *did)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_init_device_id(did);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_enable_pincode(bool enable)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_enable_pincode(enable);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_enable(bt_ready_cb_t cb)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_enable(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_disable(void)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_disable();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_discovery_start(const struct bt_br_discovery_param *param,
			  bt_br_discovery_cb_t cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_discovery_start(param, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_discovery_stop(void)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_discovery_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_scan_enable(uint8_t scan)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_scan_enable(scan);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_iac(bool limit_iac)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_iac(limit_iac);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_page_scan_activity(uint16_t interval, uint16_t windown)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_page_scan_activity(interval, windown);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_inquiry_scan_type(uint8_t type)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_inquiry_scan_type(type);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_page_scan_type(uint8_t type)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_page_scan_type(type);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_reg_resolve_cb(bt_br_resolve_addr_t cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_reg_resolve_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_remote_name_request(const bt_addr_t *addr, bt_br_remote_name_cb_t cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_remote_name_request(addr, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_write_inquiry_scan_activity(uint16_t interval, uint16_t windown)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_write_inquiry_scan_activity(interval, windown);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_set_security(struct bt_conn *conn, bt_security_t sec)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_set_security(conn, sec);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

bt_security_t hostif_bt_conn_get_security(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	int prio;
	bt_security_t ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_get_security(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return BT_SECURITY_L0;
#endif
}

int hostif_bt_conn_security_is_start(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_security_is_start(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

void hostif_bt_conn_cb_register(struct bt_conn_cb *cb)
{
#ifdef CONFIG_BT_HCI
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_cb_register(cb);
	hostif_revert_prio(prio);
#endif
}

void hostif_bt_conn_cb_unregister(void)
{
#ifdef CONFIG_BT_HCI
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_cb_unregister();
	hostif_revert_prio(prio);
#endif
}


int hostif_bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_get_info(conn, info);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint8_t hostif_bt_conn_get_type(const struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	uint8_t type;
	int prio;

	prio = hostif_set_negative_prio();
	type = bt_conn_get_type(conn);
	hostif_revert_prio(prio);

	return type;
#else
	return 0;
#endif
}

uint16_t hostif_bt_conn_get_handle(const struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	uint16_t handle;
	int prio;

	prio = hostif_set_negative_prio();
	handle = bt_conn_get_handle(conn);
	hostif_revert_prio(prio);

	return handle;
#else
	return 0;
#endif
}

struct bt_conn *hostif_bt_conn_get_acl_conn_by_sco(const struct bt_conn *sco_conn)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_get_acl_conn_by_sco(sco_conn);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

int hostif_bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	if (!conn) {
		return -ENODEV;
	}

	prio = hostif_set_negative_prio();
	ret = bt_conn_disconnect(conn, reason);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

struct bt_conn *hostif_bt_conn_create_br(const bt_addr_t *peer,
				  const struct bt_br_conn_param *param)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_create_br(peer, param);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

struct bt_conn *hostif_bt_conn_br_acl_connecting(const bt_addr_t *addr)
{
#ifdef CONFIG_BT_BREDR
	struct bt_conn *conn;
	int prio;

	prio = hostif_set_negative_prio();
	conn = bt_conn_br_acl_connecting(addr);
	hostif_revert_prio(prio);

	return conn;
#else
	return NULL;
#endif
}

int hostif_bt_conn_create_sco(struct bt_conn *br_conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
#ifdef CONFIG_BT_HFP_HF
	ret = bt_hfp_hf_req_codec_connection(br_conn);
#else
	ret = -1;
#endif
	if (ret != 0) {
		/* HF not connect or AG device, just do sco connect */
		ret = bt_conn_create_sco(br_conn);
	}
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_send_sco_data(struct bt_conn *conn, uint8_t *data, uint8_t len)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_send_sco_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_check_enter_sniff(struct bt_conn *conn, uint16_t min_interval, uint16_t max_interval)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_check_enter_sniff(conn, min_interval, max_interval);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_conn_check_exit_sniff(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_check_exit_sniff(conn);
	hostif_revert_prio(prio);
#endif
}

uint16_t hostif_bt_conn_get_rxtx_cnt(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HCI
	int prio;
	uint16_t ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_get_rxtx_cnt(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

int hostif_bt_conn_send_connectionless_data(struct bt_conn *conn, uint8_t *data, uint16_t len)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_send_connectionless_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_conn_ready_send_data(struct bt_conn *conn)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_conn_ready_send_data(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

uint16_t hostif_bt_le_tx_pending_cnt(struct bt_conn *conn)
{
#ifdef CONFIG_BT_LE_ATT
	int prio;
	uint16_t ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_tx_pending_cnt(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

int hostif_bt_br_conn_ready_send_data(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_conn_ready_send_data(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_br_conn_pending_pkt(struct bt_conn *conn, uint8_t *host_pending, uint8_t *controler_pending)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_br_conn_pending_pkt(conn, host_pending, controler_pending);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_conn_reg_tx_pending_cb(struct bt_conn *conn, bt_tx_pending_cb cb)
{
#ifdef CONFIG_BT_HCI
	int prio;

	prio = hostif_set_negative_prio();
	bt_conn_reg_tx_pending_cb(conn, cb);
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_conn_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_auth_cb_register(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_le_auth_cb_register(const struct bt_conn_auth_cb *cb)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_le_auth_cb_register(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_auth_passkey_confirm(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_auth_passkey_confirm(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_auth_pairing_confirm(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_auth_pairing_confirm(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_auth_cancel(struct bt_conn *conn)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_auth_cancel(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_role_discovery(struct bt_conn *conn, uint8_t *role)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_role_discovery(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_switch_role(struct bt_conn *conn, uint8_t role)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_switch_role(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_set_supervision_timeout(struct bt_conn *conn, uint16_t timeout)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_set_supervision_timeout(conn, timeout);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_read_rssi(struct bt_conn *conn, int8_t *rssi)
{
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_read_rssi(conn, rssi);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_hfp_hf_register_cb(struct bt_hfp_hf_cb *cb)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_send_cmd(struct bt_conn *conn, char *user_cmd)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_send_cmd(conn, user_cmd);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_hf_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_HF
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_hf_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_register_cb(struct bt_hfp_ag_cb *cb)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_update_indicator(uint8_t index, uint8_t value)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_update_indicator(index, value);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_get_indicator(uint8_t index)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_get_indicator(index);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_send_event(struct bt_conn *conn, char *event, uint16_t len)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_send_event(conn, event, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hfp_ag_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HFP_AG
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hfp_ag_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

void hostif_bt_hfp_ag_display_indicator(void)
{
#ifdef CONFIG_BT_HFP_AG
	int prio;

	prio = hostif_set_negative_prio();
	bt_hfp_ag_display_indicator();
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_a2dp_connect(struct bt_conn *conn, uint8_t role)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_connect(conn, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_register_endpoint(struct bt_a2dp_endpoint *endpoint,
			      uint8_t media_type, uint8_t role)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_register_endpoint(endpoint, media_type, role);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_halt_endpoint(struct bt_a2dp_endpoint *endpoint, bool halt)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_halt_endpoint(endpoint, halt);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_register_cb(struct bt_a2dp_app_cb *cb)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_start(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_start(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_suspend(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_suspend(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_reconfig(struct bt_conn *conn, struct bt_a2dp_media_codec *codec)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_reconfig(conn, codec);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_send_delay_report(struct bt_conn *conn, uint16_t delay_time)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_send_delay_report(conn, delay_time);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_send_audio_data(struct bt_conn *conn, uint8_t *data, uint16_t len)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_send_audio_data(conn, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

struct bt_a2dp_media_codec *hostif_bt_a2dp_get_seted_codec(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	struct bt_a2dp_media_codec *codec;
	int prio;

	prio = hostif_set_negative_prio();
	codec = bt_a2dp_get_seted_codec(conn);
	hostif_revert_prio(prio);

	return codec;
#else
	return NULL;
#endif
}

uint8_t hostif_bt_a2dp_get_a2dp_role(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	uint8_t role;
	int prio;

	prio = hostif_set_negative_prio();
	role = bt_a2dp_get_a2dp_role(conn);
	hostif_revert_prio(prio);

	return role;
#else
	return BT_A2DP_CH_UNKOWN;
#endif
}

uint8_t hostif_bt_conn_get_direction(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP_TRS
	uint8_t direction;
	int prio;

	prio = hostif_set_negative_prio();
	direction = bt_conn_get_direction(conn);
	hostif_revert_prio(prio);

	return direction;
#else
	return 0;
#endif
}

uint16_t hostif_bt_a2dp_get_a2dp_media_mtu(struct bt_conn *conn)
{
#ifdef CONFIG_BT_A2DP
	uint16_t mtu;
	int prio;

	prio = hostif_set_negative_prio();
	mtu = bt_a2dp_get_a2dp_media_tx_mtu(conn);
	hostif_revert_prio(prio);

	return mtu;
#else
	return 0;
#endif
}

int hostif_bt_a2dp_send_audio_data_with_cb(struct bt_conn *conn, u8_t *data, u16_t len, void(*cb)(struct bt_conn *, void *))
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

#ifdef CONFIG_BT_A2DP_TRS
	/* For transmit debug */
	static uint32_t pre_send, send_cnt, send_cnt_pkt, pre_cnt_time;
	uint32_t cur_time1, cur_time2;

	cur_time1 = k_uptime_get_32();
#endif

	prio = hostif_set_negative_prio();
	ret = bt_a2dp_send_audio_data_with_cb(conn, data, len, cb);
	hostif_revert_prio(prio);

#ifdef CONFIG_BT_A2DP_TRS
	/* For transmit debug */
	cur_time2 = k_uptime_get_32();

	if ((cur_time2 - cur_time1) > 20) {
		printk("trs send time %d\n", (cur_time2 - cur_time1));
	}

	if ((cur_time1 - pre_send) > 50) {
		printk("trs send inv %d\n", (cur_time1 - pre_send));
	}
	pre_send = cur_time1;

	if (ret < 0) {
		printk("trs send err %d\n", ret);
	} else {
		send_cnt += len;
		send_cnt_pkt++;
		if ((cur_time1 - pre_cnt_time) > 5000) {
			printk("trs debug rate %d pkt %d\n", (send_cnt/5), send_cnt_pkt);
			send_cnt = 0;
			send_cnt_pkt = 0;
			pre_cnt_time = cur_time1;
		}
	}
#endif

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_spp_send_data(uint8_t spp_id, uint8_t *data, uint16_t len)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_send_data(spp_id, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint8_t hostif_bt_spp_connect(struct bt_conn *conn, uint8_t *uuid)
{
#ifdef CONFIG_BT_SPP
	uint8_t spp_id;
	int prio;

	prio = hostif_set_negative_prio();
	spp_id = bt_spp_connect(conn, uuid);
	hostif_revert_prio(prio);

	return spp_id;
#else
	return -EIO;
#endif
}

int hostif_bt_spp_disconnect(uint8_t spp_id)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_disconnect(spp_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint8_t hostif_bt_spp_register_service(uint8_t *uuid)
{
#ifdef CONFIG_BT_SPP
	uint8_t spp_id;
	int prio;

	prio = hostif_set_negative_prio();
	spp_id = bt_spp_register_service(uuid);
	hostif_revert_prio(prio);

	return spp_id;
#else
	return 0;
#endif
}

int hostif_bt_spp_register_cb(struct bt_spp_app_cb *cb)
{
#ifdef CONFIG_BT_SPP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_spp_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_spp_service_restore(void)
{
#ifdef CONFIG_BT_SPP
	int prio;

	prio = hostif_set_negative_prio();
	bt_spp_service_restore();
	hostif_revert_prio(prio);

	return 0;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_register_cb(struct bt_avrcp_app_cb *cb)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_cttg_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_cttg_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_pass_through_cmd(struct bt_conn *conn, avrcp_op_id opid, bool push)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_pass_through_cmd(conn, opid, push);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_tg_notify_change(struct bt_conn *conn, uint8_t volume)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_tg_notify_change(conn, volume);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_get_id3_info(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_get_id3_info(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_get_playback_pos(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_get_playback_pos(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_set_absolute_volume(struct bt_conn *conn, uint32_t param)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_set_absolute_volume(conn, param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

bool hostif_bt_avrcp_ct_check_event_support(struct bt_conn *conn, uint8_t event_id)
{
#ifdef CONFIG_BT_AVRCP
	int prio;
	bool ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_check_event_support(conn, event_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return false;
#endif
}

int hostif_bt_pts_avrcp_ct_get_capabilities(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_pts_avrcp_ct_get_capabilities(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_ct_get_play_status(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_ct_get_play_status(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_pts_avrcp_ct_register_notification(struct bt_conn *conn)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_pts_avrcp_ct_register_notification(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint8_t hostif_bt_pbap_client_connect(struct bt_conn *conn, struct bt_pbap_client_user_cb *cb)
{
#ifdef CONFIG_BT_PBAP_CLIENT
	uint8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_pbap_client_connect(conn, cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

int hostif_bt_pbap_client_disconnect(struct bt_conn *conn, uint8_t user_id)
{
#ifdef CONFIG_BT_PBAP_CLIENT
	int ret, prio;

	prio = hostif_set_negative_prio();
	ret = bt_pbap_client_disconnect(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_pbap_client_op(struct stack_pbap_op_param *stack_param)
{
#ifdef CONFIG_BT_PBAP_CLIENT
	int ret, prio;

	prio = hostif_set_negative_prio();
	ret = bt_pbap_client_op(stack_param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint8_t hostif_bt_map_client_connect(struct bt_conn *conn,char *path,struct bt_map_client_user_cb *cb)
{
#ifdef CONFIG_BT_MAP_CLIENT
	uint8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_map_client_connect(conn,path,cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

uint8_t hostif_bt_map_client_get_message(struct bt_conn *conn, char *path, struct bt_map_client_user_cb *cb)
{
#ifdef CONFIG_BT_MAP_CLIENT
	uint8_t user_id;
	int prio;

	prio = hostif_set_negative_prio();
	user_id = bt_map_client_get_message(conn, path, cb);
	hostif_revert_prio(prio);

	return user_id;
#else
	return 0;
#endif
}

int hostif_bt_map_client_abort_get(struct bt_conn *conn, uint8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_abort_get(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_disconnect(struct bt_conn *conn, uint8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_disconnect(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_set_folder(struct bt_conn *conn, uint8_t user_id,char *path,uint8_t flags)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_set_folder(conn, user_id,path,flags);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_map_client_get_folder_listing(struct bt_conn *conn, uint8_t user_id)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_get_folder_listing(conn, user_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}


int hostif_bt_map_client_get_messages_listing(struct bt_conn *conn, uint8_t user_id,uint16_t max_list_count,uint32_t parameter_mask)
{
#ifdef CONFIG_BT_MAP_CLIENT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_map_client_get_messages_listing(conn, user_id,max_list_count,parameter_mask);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_register_sdp(struct bt_sdp_attribute * hid_attrs,int attrs_size)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_register_sdp(hid_attrs, attrs_size);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_register_cb(struct bt_hid_app_cb *cb)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_connect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_connect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_disconnect(struct bt_conn *conn)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_disconnect(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_ctrl_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_ctrl_data(conn,report_type, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_intr_data(struct bt_conn *conn,uint8_t report_type,uint8_t * data,uint16_t len)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_intr_data(conn,report_type, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_hid_send_rsp(struct bt_conn *conn,uint8_t status)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_hid_send_rsp(conn,status);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_did_register_sdp(struct device_id_info *device_id)
{
#ifdef CONFIG_BT_HID
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_did_register_sdp(device_id);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_adv_start(param, ad, ad_len, sd, sd_len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_adv_stop(void)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_adv_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_adv_reset(void)
{
#ifdef CONFIG_BT_LE_ATT
    int prio,ret;
	prio = hostif_set_negative_prio();
	ret = bt_le_adv_reset();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_get_adv_status(uint8_t* status)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret = 0;
	prio = hostif_set_negative_prio();
    ret = bt_le_get_adv_status(status);
	hostif_revert_prio(prio);
	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_conn_le_param_update(conn, param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_service_register(struct bt_gatt_service *svc)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_service_register(svc);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_service_unregister(struct bt_gatt_service *svc)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_service_unregister(svc);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

uint16_t hostif_bt_gatt_get_mtu(struct bt_conn *conn)
{
#ifdef CONFIG_BT_LE_ATT
	uint16_t mtu;
	int prio;

	prio = hostif_set_negative_prio();
	mtu = bt_gatt_get_mtu(conn);
	hostif_revert_prio(prio);

	return mtu;
#else
	return 0;
#endif
}

int hostif_bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_indicate(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_notify(struct bt_conn *conn, const struct bt_gatt_attr *attr,
		   const void *data, uint16_t len)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_notify(conn, attr, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_exchange_mtu(struct bt_conn *conn,
			 struct bt_gatt_exchange_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_exchange_mtu(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

#ifdef CONFIG_GATT_OVER_BREDR
void hostif_bt_gobr_reg_connected_cb(bt_gobr_connected_cb_t cb, bt_gatt_connect_status_cb_t status_cb)
{
	int prio;

	prio = hostif_set_negative_prio();
	bt_gobr_reg_connected_cb(cb, status_cb);
	hostif_revert_prio(prio);

}
#endif

int hostif_bt_gatt_discover(struct bt_conn *conn, struct bt_gatt_discover_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_discover(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_read(struct bt_conn *conn, struct bt_gatt_read_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_read(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_write(struct bt_conn *conn, struct bt_gatt_write_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_write(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_subscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_subscribe(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_gatt_unsubscribe(struct bt_conn *conn, struct bt_gatt_subscribe_params *params)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_gatt_unsubscribe(conn, params);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_scan_start(param, cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_scan_stop(void)
{
#ifdef CONFIG_BT_LE_ATT
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_le_scan_stop();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_le_get_mac(bt_addr_le_t *le_addr)
{
#ifdef CONFIG_BT_LE_ATT
	int prio;
 	size_t count = CONFIG_BT_ID_MAX;

	prio = hostif_set_negative_prio();
	bt_id_get(le_addr, &count);
	hostif_revert_prio(prio);

	return 0;
#else
	return -EIO;
#endif
}

int hostif_reg_flush_nvram_cb(void *cb)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_property_reg_flush_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

int hostif_bt_store_get_linkkey(struct br_linkkey_info *info, uint8_t cnt)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_store_get_linkkey(info, cnt);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

int hostif_bt_store_update_linkkey(struct br_linkkey_info *info, uint8_t cnt)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_store_update_linkkey(info, cnt);
	hostif_revert_prio(prio);

	return ret;
#else
	return 0;
#endif
}

void hostif_bt_store_write_ori_linkkey(bt_addr_t *addr, uint8_t *link_key)
{
#ifdef CONFIG_BT_PROPERTY
	int prio;

	prio = hostif_set_negative_prio();
	bt_store_write_ori_linkkey(addr, link_key);
	hostif_revert_prio(prio);
#endif
}

void hostif_bt_store_clear_linkkey(const bt_addr_t *addr)
{
#ifdef CONFIG_BT_PROPERTY
	int prio;

	prio = hostif_set_negative_prio();
	bt_store_clear_linkkey(addr);
	hostif_revert_prio(prio);
#endif
}

int hostif_bt_store_find_linkkey(const bt_addr_t *addr, uint8_t *key)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_store_find_linkkey(addr,key);
	hostif_revert_prio(prio);
	return ret;
#else
	return 0;
#endif
}

int hostif_bt_le_find_linkkey(const bt_addr_le_t *addr)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret = 0;

	prio = hostif_set_negative_prio();
	ret = acts_le_keys_find_link_key(addr);
	hostif_revert_prio(prio);
	return ret;
#else
	return 0;
#endif
}

void hostif_bt_le_clear_linkkey(const bt_addr_le_t *addr)
{
#ifdef CONFIG_BT_PROPERTY
	int prio;
	prio = hostif_set_negative_prio();
	acts_le_keys_clear_link_key(addr);
	hostif_revert_prio(prio);
#endif
}

uint8_t hostif_bt_le_get_linkkey_mum(void)
{
#ifdef CONFIG_BT_PROPERTY
	int prio, ret = 0;

	prio = hostif_set_negative_prio();
	ret = acts_le_keys_get_link_key_num();
	hostif_revert_prio(prio);
	return ret;
#else
	return 0;
#endif
}

int hostif_bt_csb_set_broadcast_link_key_seed(uint8_t *seed, uint8_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_broadcast_link_key_seed(seed, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_reserved_lt_addr(uint8_t lt_addr)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_reserved_lt_addr(lt_addr);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_broadcast(uint8_t enable, uint8_t lt_addr, uint16_t interval_min, uint16_t interval_max)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_broadcast(enable, lt_addr, interval_min, interval_max);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_receive(uint8_t enable, struct bt_hci_evt_sync_train_receive *param, uint16_t timeout)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_receive(enable, param, timeout);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_write_sync_train_param(uint16_t interval_min, uint16_t interval_max, uint32_t trainTO, uint8_t service_data)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_write_sync_train_param(interval_min, interval_max, trainTO, service_data);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_start_sync_train(void)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_start_sync_train();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_set_CSB_date(uint8_t lt_addr, uint8_t *data, uint8_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_set_CSB_date(lt_addr, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_receive_sync_train(uint8_t *mac, uint16_t timeout, uint16_t windown, uint16_t interval)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_receive_sync_train(mac, timeout, windown, interval);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_csb_broadcase_by_acl(uint16_t handle, uint8_t *data, uint16_t len)
{
#ifdef CONFIG_CSB
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_csb_broadcase_by_acl(handle, data, len);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_adjust_link_time(struct bt_conn *conn, int16_t time)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_adjust_link_time(conn, time);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_get_bt_clock(struct bt_conn *conn, uint32_t *bt_clk, uint16_t *bt_intraslot)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_get_bt_clock(conn, bt_clk, bt_intraslot);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_set_tws_int_clock(struct bt_conn *conn, uint32_t bt_clk, uint16_t bt_intraslot)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_set_tws_int_clock(conn, bt_clk, bt_intraslot);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_enable_tws_int(uint8_t enable)
{
#ifdef CONFIG_BT_BREDR
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_enable_tws_int(enable);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_enable_acl_flow_control(struct bt_conn *conn, uint8_t enable)
{
#ifdef CONFIG_BT_CONN
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_enable_acl_flow_control(conn, enable);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_read_add_null_cnt(struct bt_conn *conn, uint16_t *cnt)
{
#ifdef CONFIG_BT_CONN
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_read_add_null_cnt(conn, cnt);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_set_tws_sync_int_time(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint32_t bt_clk, uint16_t bt_slot)
{
#ifdef CONFIG_BT_CONN
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_set_tws_sync_int_time(conn, index, mode, bt_clk, bt_slot);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_set_tws_int_delay_play_time(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint8_t per_int, uint32_t bt_clk, uint16_t bt_slot)
{
#ifdef CONFIG_BT_CONN
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_set_tws_int_delay_play_time(conn, index, mode, per_int, bt_clk, bt_slot);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_set_tws_int_clock_new(struct bt_conn *conn, uint8_t index, uint8_t mode,
	uint32_t clk, uint16_t slot, uint32_t per_clk, uint16_t per_slot)
{
#ifdef CONFIG_BT_CONN
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_set_tws_int_clock_new(conn, index, mode, clk, slot, per_clk, per_slot);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_enable_tws_int_new(uint8_t enable, uint8_t index)
{
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_enable_tws_int_new(enable, index);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_vs_read_bt_us_cnt(uint32_t *cnt)
{
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_read_bt_us_cnt(cnt);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_vs_set_apll_temp_comp(uint8_t enable)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_set_apll_temp_comp(enable);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_vs_do_apll_temp_comp(void)
{
#ifdef CONFIG_BT_HCI
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_vs_do_apll_temp_comp();
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_pnp_info_register_cb(struct bt_pnp_info_cb *cb)
{
#ifdef CONFIG_BT_PNP_INFO_SEARCH
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_pnp_info_register_cb(cb);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_pnp_info_search(struct bt_conn *conn)
{
#ifdef CONFIG_BT_PNP_INFO_SEARCH
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_pnp_info_search(conn);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_enable(uint32_t param)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avdtp_enable(param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_a2dp_disable(uint32_t param)
{
#ifdef CONFIG_BT_A2DP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avdtp_disable(param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_enable(uint32_t param)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_enable(param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}

int hostif_bt_avrcp_disable(uint32_t param)
{
#ifdef CONFIG_BT_AVRCP
	int prio, ret;

	prio = hostif_set_negative_prio();
	ret = bt_avrcp_disable(param);
	hostif_revert_prio(prio);

	return ret;
#else
	return -EIO;
#endif
}
#if defined(CONFIG_BT_EXT_ADV)
int hostif_bt_le_per_adv_sync_delete(struct bt_le_per_adv_sync *per_adv_sync)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_per_adv_sync_delete(per_adv_sync);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_ext_adv_create(const struct bt_le_adv_param *param,
			const struct bt_le_ext_adv_cb *cb,
			struct bt_le_ext_adv **out_adv)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_ext_adv_create(param,cb,out_adv);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_per_adv_set_param(struct bt_le_ext_adv *adv,
				const struct bt_le_per_adv_param *param)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret =  bt_le_per_adv_set_param(adv,param);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_per_adv_start(struct bt_le_ext_adv *adv)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_per_adv_start(adv);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_ext_adv_start(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_start_param *param)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret =  bt_le_ext_adv_start(adv,param);
	hostif_revert_prio(prio);

	return ret;
}

void hostif_bt_le_scan_cb_register(struct bt_le_scan_cb *cb)
{
	int prio;

	prio = hostif_set_negative_prio();
	bt_le_scan_cb_register(cb);
	hostif_revert_prio(prio);
}

void hostif_bt_le_per_adv_sync_cb_register(struct bt_le_per_adv_sync_cb *cb)
{
	int prio;

	prio = hostif_set_negative_prio();
	bt_le_per_adv_sync_cb_register(cb);
	hostif_revert_prio(prio);
}


int hostif_bt_le_per_adv_sync_create(const struct bt_le_per_adv_sync_param *param,
					struct bt_le_per_adv_sync **out_sync)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_per_adv_sync_create(param,out_sync);
	hostif_revert_prio(prio);

	return ret;

}

int hostif_bt_le_ext_adv_set_data(struct bt_le_ext_adv *adv,
				const struct bt_data *ad, size_t ad_len,
				const struct bt_data *sd, size_t sd_len)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_ext_adv_set_data(adv,ad, ad_len,sd, sd_len);   
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_per_adv_set_data(const struct bt_le_ext_adv *adv,
				const struct bt_data *ad, size_t ad_len)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_per_adv_set_data(adv,ad,ad_len);
	hostif_revert_prio(prio);

	return ret;
}

void hostif_bt_le_scan_cb_unregister(struct bt_le_scan_cb *cb)
{
	int prio;

	prio = hostif_set_negative_prio();
	bt_le_scan_cb_unregister(cb);
	hostif_revert_prio(prio);
}

void hostif_bt_le_per_adv_sync_cb_unregister(struct bt_le_per_adv_sync_cb *cb)
{
	int prio;

	prio = hostif_set_negative_prio();
	bt_le_per_adv_sync_cb_unregister(cb);
	hostif_revert_prio(prio);
}

int hostif_bt_le_per_adv_stop(struct bt_le_ext_adv *adv)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_per_adv_stop(adv);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_ext_adv_stop(struct bt_le_ext_adv *adv)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_ext_adv_stop(adv);
	hostif_revert_prio(prio);

	return ret;
}

int hostif_bt_le_ext_adv_delete(struct bt_le_ext_adv *adv)
{
	int ret;
	int prio;

	prio = hostif_set_negative_prio();
	ret = bt_le_ext_adv_delete(adv);
	hostif_revert_prio(prio);

	return ret;
}
#endif
