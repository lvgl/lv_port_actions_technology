/*
 * Copyright (c) 2019 Actions Semi Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file
 * @brief
 *
 * Change log:
 *
 */

#include <os_common_api.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef CONFIG_BLUETOOTH
#include <acts_bluetooth/host_interface.h>
#include <bt_manager.h>
#endif
#include <mem_manager.h>
#include <system_app.h>

#include <user_comm/ap_status.h>
#include <user_comm/ap_record.h>

#include "nv_ble_stream.h"
#include "nv_ble_stream_ctrl.h"
#include "../nv_analyze/nv_protocol.h"

#define BLE_TEST_STACKSIZE	(1576)
#define BLE_TEST_ATTSIZE		(256)

#define NV_RXTX_INTERVAL		(5)
#define NV_RX_BUF_SIZE			(256)
#define NV_TX_SEND_SIZE		(256)
#define NV_TX_BUF_SIZE			(1024)
#define NV_TX_CMD_BUF_SIZE		(256)
#define NV_MIX_PACK_LEN		(5)

#define BLE_NV_RX_BUF_SIZE	 256
#define NV_OPUS_FRAME_LEN	 40
#define NV_OPUS_FRAME_NUM	 4

static OS_MUTEX_DEFINE(nv_tx_mutex);
io_stream_t nv_ble_stream;
static u8_t ble_stream_opened = 0;
static struct nv_info_t *nv_info = NULL;

static os_work_q nv_spp_ble_q;
static os_delayed_work connect_delaywork;
static os_delayed_work disconnect_delaywork;
static os_delayed_work run_delaywork_tx;
static os_delayed_work run_delaywork_rx;
//static u8_t read_buf[128];
static os_delayed_work auth_work;

static u8_t ble_nv_stack[BLE_TEST_STACKSIZE] __aligned(4);

/* BLE */
#define NV_SERVICE_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF0, 0x8B, 0xFA, 0x38)

#define BLE_NV_SV_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF1, 0x8B, 0xFA, 0x38)

#define BLE_NV_REQ_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF2, 0x8B, 0xFA, 0x38)

#define BLE_NV_LWP_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF3, 0x8B, 0xFA, 0x38)

#define BLE_NV_LWO_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF4, 0x8B, 0xFA, 0x38)

#define BLE_NV_DID_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF5, 0x8B, 0xFA, 0x38)

#define BLE_NV_CID_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF6, 0x8B, 0xFA, 0x38)

#define BLE_NV_DP_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF7, 0x8B, 0xFA, 0x38)

#define BLE_NV_ADDR_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF8, 0x8B, 0xFA, 0x38)

#define BLE_NV_AUTH_UUID BT_UUID_DECLARE_128(0xC0, 0x5A, 0xBF, 0x47, \
	0xC6, 0x48, 0x07, 0xB3, 0x38, 0x44, 0x73, 0xA4, 0xF9, 0x8B, 0xFA, 0x38)

static const u8_t nv_lwp = 0;
static const u8_t nv_lwo = 0;
static const u8_t nv_dp = 0;

static ssize_t read_nv_sv(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	u8_t sv[4];
	u16_t tlen;

	tlen = nv_sv_get(sv);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, sv, tlen);

	//return bt_gatt_attr_read(conn, attr, buf, len, offset, nv_sv,
	//			 sizeof(nv_sv));
}

static ssize_t read_nv_lwp(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &nv_lwp,
				 sizeof(nv_lwp));
}

static ssize_t read_nv_lwo(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &nv_lwo,
				 sizeof(nv_lwo));
}

static ssize_t read_nv_did(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	u8_t did[16];
	u16_t tlen;

	tlen = nv_did_get(did);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &did, tlen);

	//return bt_gatt_attr_read(conn, attr, buf, len, offset, &did,
	//				sizeof(struct nv_did));
}

static ssize_t read_nv_addr(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	u8_t addr[6];
	memset(addr, 0x12, 6);
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &addr, 6);
}

static ssize_t write_nv_cid(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	print_hex("write_nv_cid:", buf, len);
	nv_cid_set((u8_t *)buf, len);

	return len;
}

static ssize_t read_dp_data(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &nv_dp,
					sizeof(u8_t));
}

static struct bt_gatt_attr	nv_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(NV_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(BLE_NV_SV_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_nv_sv, NULL, NULL/*(void *)nv_sv*/),
	BT_GATT_CHARACTERISTIC(BLE_NV_REQ_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BLE_NV_LWP_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_nv_lwp, NULL, (void *)&nv_lwp),
	BT_GATT_CHARACTERISTIC(BLE_NV_LWO_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_nv_lwo, NULL, (void *)&nv_lwo),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BLE_NV_DID_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_nv_did, NULL, NULL/*(void *)&did*/),
	BT_GATT_CHARACTERISTIC(BLE_NV_CID_UUID, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_nv_cid, NULL),
	BT_GATT_CHARACTERISTIC(BLE_NV_DP_UUID, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ, read_dp_data, NULL, (void *)&nv_dp),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BLE_NV_ADDR_UUID, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_nv_addr, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BLE_NV_AUTH_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static void nv_connect_delaywork(os_work *work)
{
	SYS_LOG_INF("nv_connect_delaywork");

	if (nv_ble_stream) {
		//ret = stream_open(nv_ble_stream, MODE_IN_OUT);
		//if (ret) {
		//	SYS_LOG_ERR("stream_open Failed");
		//} else 
		{
			nv_info = app_mem_malloc(sizeof(struct nv_info_t));
			if (!nv_info) {
				SYS_LOG_ERR("Failed malloc nv_info");
			} else {
				memset(nv_info, 0, sizeof(struct nv_info_t));
			}
		}

		ble_stream_opened = 1;
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &run_delaywork_tx, 0);
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &run_delaywork_rx, 0);
	}
}

static void nv_disconnect_delaywork(os_work *work)
{
	int ret;

	if (nv_ble_stream) {
		ret = stream_close(nv_ble_stream);
		if (ret) {
			SYS_LOG_ERR("stream_close Failed");
		} else {
			os_mutex_lock(&nv_tx_mutex, OS_FOREVER);
			SYS_LOG_INF("free-----");

			if (nv_info) {
				if (nv_info->record_send_buf.data_buf)
				{
					app_mem_free(nv_info->record_send_buf.data_buf);
					memset(&nv_info->record_send_buf, 0 , sizeof(nv_info->record_send_buf));
				}

				if (nv_info->tx_loop_buf.data_buf)
				{
					app_mem_free(nv_info->tx_loop_buf.data_buf);
					memset(&nv_info->tx_loop_buf, 0 , sizeof(nv_info->tx_loop_buf));
				}
				app_mem_free(nv_info);
				nv_info = NULL;
			}
			os_mutex_unlock(&nv_tx_mutex);
			ble_stream_opened = 0;
			
		}
	}
}

static void nv_rx_process(void)
{
	u16_t len, total_len = 0;
	u16_t payload_len = 0;
	int timeout_count = 0;
	u8_t read_buf[128];
	
	memset(read_buf, 0, 128);
	len = stream_read(nv_ble_stream, read_buf, sizeof(u32_t));
	if (len <= 0) {
		//SYS_LOG_ERR("");
		return;
	}
	SYS_LOG_INF("nv_ble_stream attr: %p", (void *)&read_buf[0]);
	len = stream_read(nv_ble_stream, (u8_t *)&payload_len, sizeof(u16_t));
	if (len <= 0) {
		SYS_LOG_ERR("");
		return;
	}
	SYS_LOG_INF("nv_ble_stream payload_len: %d", len);
	if (payload_len + sizeof(u32_t) + sizeof(u16_t) > 128)
		return;

	while (stream_get_length(nv_ble_stream) < payload_len) {
		if (timeout_count++ > 100) {
			break;
		}

		os_sleep(1);
	}

	memcpy(read_buf + sizeof(u32_t), &payload_len, sizeof(u16_t));
	stream_read(nv_ble_stream, read_buf + sizeof(u32_t) + sizeof(u16_t), payload_len);
	total_len = payload_len + sizeof(u32_t) + sizeof(u16_t);
	nv_recv_proc(read_buf, total_len);
}

int nv_send_pkg_to_stream(u8_t	*data_ptr, u16_t length, int num)
{
	int ret = 0;
	struct nv_info_t *p;
	u8_t number = (u8_t)num;

	if ((!length) || (length > 256) || (!data_ptr)) {
		return 0;
	}

	//SYS_LOG_INF("cmd(%d), %d", cmd, length);
	//print_hex("d2:",data_ptr,16);

	os_mutex_lock(&nv_tx_mutex, OS_FOREVER);
	p = nv_info;
	if (!ble_stream_opened || !p) {
		goto exit_send_pkt;
	}
	
	if (num)
	{
		if (!p->tx_loop_buf.data_buf)
		{
			p->tx_loop_buf.buf_size = NV_TX_CMD_BUF_SIZE;
			p->tx_loop_buf.data_buf = (u8_t *)app_mem_malloc(p->tx_loop_buf.buf_size);
		}

		if ((sizeof(u16_t) + sizeof(u8_t) + length) <= (p->tx_loop_buf.buf_size - p->tx_loop_buf.data_count))
		{
			if (sizeof(u8_t) != loop_buffer_write(&p->tx_loop_buf, &number, sizeof(u8_t)))
				SYS_LOG_ERR("");

			if (sizeof(u16_t) != loop_buffer_write(&p->tx_loop_buf, &length, sizeof(u16_t)))
				SYS_LOG_ERR("");
			
			if(length != loop_buffer_write(&p->tx_loop_buf, data_ptr, length))
				SYS_LOG_ERR("");
		}
		else
		{
			SYS_LOG_ERR("0x%x 0x%x.",
				length,(p->tx_loop_buf.buf_size - p->tx_loop_buf.data_count));
			goto exit_send_pkt;
		}
	} else {
		if (!p->record_send_buf.data_buf)
		{
			p->record_send_buf.buf_size = NV_TX_BUF_SIZE;
			p->record_send_buf.data_buf = (u8_t *)app_mem_malloc(p->record_send_buf.buf_size);
		}
		
		if (length <= (p->record_send_buf.buf_size - p->record_send_buf.data_count))
		{
			//print_hex("d3:",data_ptr,16);
			if(length != loop_buffer_write(&p->record_send_buf, data_ptr, length))
				SYS_LOG_ERR("");
		}
		else
		{
			SYS_LOG_ERR("0x%x 0x%x.",
				length,(p->record_send_buf.buf_size - p->record_send_buf.data_count));
			goto exit_send_pkt;
		}
	}

exit_send_pkt:
	os_mutex_unlock(&nv_tx_mutex);
	SYS_LOG_INF("exit");

	return ret;
}

static void nv_tx_process(void)
{
	int ret;
	u16_t get_len = 0;
	u8_t tptr[160];
	struct nv_info_t *p;
	u8_t num;

	os_mutex_lock(&nv_tx_mutex, OS_FOREVER);
	p = nv_info;
	if (!ble_stream_opened || !p) {
		os_mutex_unlock(&nv_tx_mutex);
		return;
	}

	if (p->tx_loop_buf.data_count) {

		loop_buffer_read(&p->tx_loop_buf, &num, sizeof(u8_t));
		loop_buffer_read(&p->tx_loop_buf, &get_len, sizeof(u16_t));
		loop_buffer_read(&p->tx_loop_buf, tptr, get_len);

		os_mutex_unlock(&nv_tx_mutex);

		nv_tx_attr_set((void *)nv_uuid_tx_chrc_attr_get(num), (void *)nv_uuid_tx_attr_get(num));
		ret = stream_write(nv_ble_stream, &tptr[0], get_len);

		if (ret > 0) {
			SYS_LOG_INF("NV CMD TX: %d, %d\n", get_len, ret);
		} else {
			SYS_LOG_ERR("CMD ERR TX: %d, %d\n", get_len, ret);
		}
		return;
	}

	if ((AP_STATUS_RECORDING == ap_status_get()) &&
		(p->record_send_buf.data_count >= OPUS_FRAME_LEN*NV_OPUS_FRAME_NUM)) {

		memset (tptr, 0, sizeof(tptr));
		loop_buffer_read(&p->record_send_buf, tptr, OPUS_FRAME_LEN*NV_OPUS_FRAME_NUM);
		//print_hex("d4:",tptr,16);
		get_len = OPUS_FRAME_LEN*NV_OPUS_FRAME_NUM;
		os_mutex_unlock(&nv_tx_mutex);
		nv_tx_attr_set((void *)nv_uuid_tx_chrc_attr_get(BLE_NV_DP), (void *)nv_uuid_tx_attr_get(BLE_NV_DP));
		ret = stream_write(nv_ble_stream, &tptr[0], get_len);

		if (ret > 0) {
			SYS_LOG_INF("NV CMD TX: %d, %d\n", get_len, ret);
		} else {
			SYS_LOG_ERR("RECORD ERR TX: %d, %d\n", get_len, ret);
		}
		return;
	}	

	os_mutex_unlock(&nv_tx_mutex);
}

static void nv_run_delaywork_rx(os_work *work)
{
	if (nv_ble_stream && ble_stream_opened) {
		nv_rx_process();
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &run_delaywork_rx, 5);
	}
}
static void nv_run_delaywork_tx(os_work *work)
{
	if (nv_ble_stream && ble_stream_opened) {
		nv_tx_process();
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &run_delaywork_tx, 50);
	}

}

static void nv_ble_connect(BOOL connected)
{
	SYS_LOG_INF("%s", (connected) ? "connected" : "disconnected");
	if (connected) {
		if (stream_open(nv_ble_stream, MODE_IN_OUT)) {
			SYS_LOG_ERR("stream_open Failed");
			return;
		}

		//nv_auth_timeout_start(10000);
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &connect_delaywork, 0);

		//ble_adv_limit_set(SYS_BLE_LIMITED_APP);
		nv_resource_alloc();
		ap_status_set(AP_STATUS_LINKED);
	} else {
		//nv_auth_timeout_stop();
		os_delayed_work_submit_to_queue(&nv_spp_ble_q, &disconnect_delaywork, 0);
		//ble_adv_limit_clear(SYS_BLE_LIMITED_APP);
		nv_resource_release();
		ap_status_set(AP_STATUS_NONE);
	}
}


/* Start after bt engine ready */
void nv_ble_stream_init(void)
{
	struct nv_ble_stream_init_param init_param;

	os_work_q_start(&nv_spp_ble_q, (os_thread_stack_t *)ble_nv_stack, BLE_TEST_STACKSIZE, 11);
	os_delayed_work_init(&connect_delaywork, nv_connect_delaywork);
	os_delayed_work_init(&disconnect_delaywork, nv_disconnect_delaywork);
	os_delayed_work_init(&run_delaywork_tx, nv_run_delaywork_tx);
	os_delayed_work_init(&run_delaywork_rx, nv_run_delaywork_rx);
	//thread_timer_init(&auth_timer, nv_ble_active_disconnect, NULL);
	os_delayed_work_init(&auth_work, nv_ble_active_disconnect);

	init_param.gatt_attr = &nv_gatt_attr[0];
	init_param.attr_size = ARRAY_SIZE(nv_gatt_attr);
	init_param.req_write_attr = &nv_gatt_attr[4];
	init_param.lwo_write_attr = &nv_gatt_attr[9];
	init_param.cid_write_attr = &nv_gatt_attr[14];
	init_param.auth_write_attr = &nv_gatt_attr[21];

	init_param.notify_ccc_attr = &nv_gatt_attr[22];

	init_param.connect_cb = nv_ble_connect;
	//SYS_LOG_ERR("connect_cb 0x%p func 0x%p",init_param.connect_cb,nv_ble_connect);

	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.read_timeout = OS_NO_WAIT;
	init_param.write_timeout = OS_NO_WAIT;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	nv_ble_stream = nv_ble_stream_create(&init_param);
	if (!nv_ble_stream) {
		SYS_LOG_ERR("stream_create failed");
	}

}

void nv_ble_active_disconnect(os_work *work)
{	
	if (ap_status_get() < AP_STATUS_AUTH)
	{
		SYS_LOG_INF("DISCONNECT");
		if(bt_manager_ble_is_connected())
			bt_manager_ble_disconnect();
	}
}

void nv_auth_timeout_start(u32_t duration)
{	
	//if (!thread_timer_is_running(&auth_work))
	{
		SYS_LOG_INF("auth timer creater");
		//thread_timer_start(&auth_timer, duration, 0);
		os_delayed_work_submit(&auth_work, duration);
	}
}

void nv_auth_timeout_stop(void)
{	
	//if (thread_timer_is_running(&auth_work))
	{
		SYS_LOG_INF("auth timer destory");
		//thread_timer_stop(&auth_timer);
		os_delayed_work_cancel(&auth_work);
	}
}

u8_t nv_uuid_num_get(void *attr)
{
	if ((&nv_gatt_attr[3] == attr) || (&nv_gatt_attr[4] == attr))
	{
		return BLE_NV_REQ;
	}
	else if ((&nv_gatt_attr[8] == attr) || (&nv_gatt_attr[9] == attr))
	{
		return BLE_NV_LWO;
	} 
	else if ((&nv_gatt_attr[13] == attr) || (&nv_gatt_attr[14] == attr))
	{
		return BLE_NV_CID;
	}
	else if ((&nv_gatt_attr[20] == attr) || (&nv_gatt_attr[21] == attr))
	{
		return BLE_NV_AUTH;
	}

	return BLE_NV_CHARACTERISTICS_NUM;
}

int nv_uuid_tx_chrc_attr_get(u8_t num)
{
	if (BLE_NV_REQ == num)
	{
		return (int)&nv_gatt_attr[3];
	}
	else if (BLE_NV_DP == num)
	{
		return (int)&nv_gatt_attr[15];
	} 
	else if (BLE_NV_AUTH == num)
	{
		return (int)&nv_gatt_attr[20];
	} 

	return NULL;
}

int nv_uuid_tx_attr_get(u8_t num)
{
	if (BLE_NV_REQ == num)
	{
		return (int)&nv_gatt_attr[4];
	}
	else if (BLE_NV_DP == num)
	{
		return (int)&nv_gatt_attr[16];
	} 
	else if (BLE_NV_AUTH == num)
	{
		return (int)&nv_gatt_attr[21];
	} 

	return NULL;
}

void nv_record_resource_release(void)
{
	if (nv_info) {
		if (nv_info->record_send_buf.data_buf)
		{
			app_mem_free(nv_info->record_send_buf.data_buf);
			memset(&nv_info->record_send_buf, 0 , sizeof(nv_info->record_send_buf));
		}
	}
}

