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

#include "tuya_ble_stream.h"
#include "tuya_ble_service.h"
#include "tuya_ble_vos.h"
#include "tuya_ble_app_demo.h"

#define BLE_TEST_STACKSIZE	(4096)//(1576)
#define BLE_TEST_ATTSIZE		(256)

#define TUYA_RXTX_INTERVAL		(5)
#define TUYA_RX_BUF_SIZE			(256)
#define TUYA_TX_SEND_SIZE		(256)
#define TUYA_TX_BUF_SIZE			(2048)
#define TUYA_TX_CMD_BUF_SIZE		(480)
#define TUYA_MIX_PACK_LEN		(5)

#define BLE_TUYA_RX_BUF_SIZE	 256
#define TUYA_OPUS_FRAME_LEN	 40
#define TUYA_OPUS_FRAME_NUM	 4

static OS_MUTEX_DEFINE(tuya_tx_mutex);
io_stream_t tuya_ble_stream;
static u8_t ble_stream_opened = 0;
static struct tuya_info_t *tuya_info = NULL;

static os_work_q tuya_spp_ble_q;
static os_delayed_work connect_delaywork;
static os_delayed_work disconnect_delaywork;
static os_delayed_work run_delaywork_tx;
static os_delayed_work run_delaywork_rx;
//static u8_t read_buf[128];
static os_delayed_work auth_work;
static os_delayed_work find_work;
static os_delayed_work vos_start_work;
static os_delayed_work vos_stop_work;

static u8_t ble_tuya_stack[BLE_TEST_STACKSIZE] __aligned(4);

/* BLE */
#define TUYA_SERVICE_UUID BT_UUID_DECLARE_128(0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x50, 0xfd, 0x00, 0x00)

#define BLE_TUYA_RX_UUID BT_UUID_DECLARE_128(0xD0, 0x07, 0x9B, 0x5F, 0x80, \
	0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00)

#define BLE_TUYA_TX_UUID BT_UUID_DECLARE_128(0xD0, 0x07, 0x9B, 0x5F, 0x80, \
	0x00, 0x01, 0x80, 0x01, 0x10, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00)

static struct bt_gatt_attr	tuya_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(TUYA_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(BLE_TUYA_TX_UUID, BT_GATT_CHRC_NOTIFY , BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BLE_TUYA_RX_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP, BT_GATT_PERM_WRITE, NULL, NULL, NULL),
};

static void tuya_connect_delaywork(os_work *work)
{
	SYS_LOG_INF("tuya_connect_delaywork");

	if (tuya_ble_stream) {
		//ret = stream_open(tuya_ble_stream, MODE_IN_OUT);
		//if (ret) {
		//	SYS_LOG_ERR("stream_open Failed");
		//} else 
		{
			tuya_info = app_mem_malloc(sizeof(struct tuya_info_t));
			if (!tuya_info) {
				SYS_LOG_ERR("Failed malloc tuya_info");
			} else {
				memset(tuya_info, 0, sizeof(struct tuya_info_t));
			}
		}

		ble_stream_opened = 1;
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &run_delaywork_tx, 0);
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &run_delaywork_rx, 0);
	}
}

static void tuya_disconnect_delaywork(os_work *work)
{
	int ret;

	if (tuya_ble_stream) {
		ret = stream_close(tuya_ble_stream);
		if (ret) {
			SYS_LOG_ERR("stream_close Failed");
		} else {
			os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
			SYS_LOG_INF("free-----");

			if (tuya_info) {
				if (tuya_info->record_send_buf.data_buf)
				{
					app_mem_free(tuya_info->record_send_buf.data_buf);
					memset(&tuya_info->record_send_buf, 0 , sizeof(tuya_info->record_send_buf));
				}

				if (tuya_info->tx_loop_buf.data_buf)
				{
					app_mem_free(tuya_info->tx_loop_buf.data_buf);
					memset(&tuya_info->tx_loop_buf, 0 , sizeof(tuya_info->tx_loop_buf));
				}
				app_mem_free(tuya_info);
				tuya_info = NULL;
			}
			os_mutex_unlock(&tuya_tx_mutex);
			ble_stream_opened = 0;
			
		}
	}
}

static void tuya_rx_process(void)
{
	u16_t len;
	u16_t payload_len = 0;
	int timeout_count = 0;
	u8_t read_buf[256];
	u8_t loop_count = 0;

	while ((stream_get_length(tuya_ble_stream) > 0) && (loop_count < 10))
	{
		memset(read_buf, 0, 256);
		len = stream_read(tuya_ble_stream, (u8_t *)&payload_len, sizeof(u16_t));
		if (len <= 0) {
			//SYS_LOG_ERR("");
			return;
		}
		SYS_LOG_INF("tuya_ble_stream payload_len: %d", payload_len);
		if (payload_len > 256)
			return;

		while (stream_get_length(tuya_ble_stream) < payload_len) {
			if (timeout_count++ > 100) {
				break;
			}

			os_sleep(1);
		}

		stream_read(tuya_ble_stream, read_buf, payload_len);
		tuya_ble_gatt_receive_data(&read_buf[0], payload_len);
		loop_count++;
	}
}

int tuya_send_pkg_to_stream(u8_t	*data_ptr, u16_t length, u8_t type)
{
	int ret = 0;
	struct tuya_info_t *p;
	u8_t tptr[CODEC_OUT_RING_BUFF_SIZE];
	u16_t tail = 0;

	if ((!length) || (length > 1024) || (!data_ptr)) {
		SYS_LOG_ERR("length 0x%x.",length);
		return 0;
	}

	//SYS_LOG_INF("cmd(%d), %d", cmd, length);

	os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
	p = tuya_info;
	if (!ble_stream_opened || !p) {
		goto exit_send_pkt;
	}
	
	if (type)
	{
		if (!p->tx_loop_buf.data_buf)
		{
			p->tx_loop_buf.buf_size = TUYA_TX_BUF_SIZE;
			p->tx_loop_buf.data_buf = (u8_t *)app_mem_malloc(p->tx_loop_buf.buf_size);
		}

		if (length+sizeof(u16_t) <= (p->tx_loop_buf.buf_size - p->tx_loop_buf.data_count))
		{
			if (sizeof(u16_t) != loop_buffer_write(&p->tx_loop_buf, &length, sizeof(u16_t)))
				SYS_LOG_ERR("%d",__LINE__);

			if(length != loop_buffer_write(&p->tx_loop_buf, data_ptr, length))
				SYS_LOG_ERR("%d",__LINE__);
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
			p->record_send_buf.buf_size = TUYA_TX_CMD_BUF_SIZE;
			p->record_send_buf.data_buf = (u8_t *)app_mem_malloc(p->record_send_buf.buf_size);
		}

		if (p->record_send_buf.data_count + length > p->record_send_buf.buf_size)
		{
			SYS_LOG_ERR("%d length %d data_count %d",__LINE__,length,p->record_send_buf.data_count);
			while (p->record_send_buf.data_count >= CODEC_OUT_RING_BUFF_SIZE)
			{
				loop_buffer_read(&p->record_send_buf, NULL, CODEC_OUT_RING_BUFF_SIZE);
			}
			
			if (p->record_send_buf.data_count+length <= CODEC_OUT_RING_BUFF_SIZE)
				loop_buffer_write(&p->record_send_buf, data_ptr, length);
			else
			{
				tail = (p->record_send_buf.data_count+length)%CODEC_OUT_RING_BUFF_SIZE;
				loop_buffer_read(&p->record_send_buf, NULL, p->record_send_buf.data_count);
				loop_buffer_write(&p->record_send_buf, data_ptr+length-tail, tail);
			}
		} 
		else
		{
			loop_buffer_write(&p->record_send_buf, data_ptr, length);
		}

		os_mutex_unlock(&tuya_tx_mutex);

		while (p->record_send_buf.data_count >= CODEC_OUT_RING_BUFF_SIZE)
		{
			os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
			loop_buffer_read(&p->record_send_buf, tptr, CODEC_OUT_RING_BUFF_SIZE);
			os_mutex_unlock(&tuya_tx_mutex);
			print_hex("d2:",tptr,16);
			tuya_vos_encoded_audio_data_send(
					tptr, CODEC_OUT_RING_BUFF_SIZE);
		}
		os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
	}

exit_send_pkt:
	os_mutex_unlock(&tuya_tx_mutex);
	SYS_LOG_INF("exit");

	return ret;
}

static void tuya_tx_process(void)
{
	int ret;
	u16_t get_len = 0;
	u8_t *tptr = NULL;
	struct tuya_info_t *p;

	os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
	p = tuya_info;
	if (!ble_stream_opened || !p) {
		os_mutex_unlock(&tuya_tx_mutex);
		return;
	}

	while (p->tx_loop_buf.data_count) {

		loop_buffer_read(&p->tx_loop_buf, &get_len, sizeof(u16_t));
		if (get_len > 512) {
			os_mutex_unlock(&tuya_tx_mutex);
			return;
		}

		if (!tptr)
			tptr = app_mem_malloc(get_len);

		loop_buffer_read(&p->tx_loop_buf, tptr, get_len);

		os_mutex_unlock(&tuya_tx_mutex);

		ret = stream_write(tuya_ble_stream, &tptr[0], get_len);

		if (ret > 0) {
			SYS_LOG_INF("TUYA CMD TX: %d, %d\n", get_len, ret);
		} else {
			SYS_LOG_ERR("CMD ERR TX: %d, %d\n", get_len, ret);
		}
		os_mutex_lock(&tuya_tx_mutex, OS_FOREVER);
	}
	
	if (tptr)
		app_mem_free(tptr);
	os_mutex_unlock(&tuya_tx_mutex);
}

int tuya_ble_service_send_data(u8_t* data, int len)
{
	tuya_send_pkg_to_stream(data, len, TRUE);

    return len;
}

static void tuya_run_delaywork_rx(os_work *work)
{
	if (tuya_ble_stream && ble_stream_opened) {
		tuya_rx_process();
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &run_delaywork_rx, 0);
	}
}
static void tuya_run_delaywork_tx(os_work *work)
{
	if (tuya_ble_stream && ble_stream_opened) {
		tuya_tx_process();
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &run_delaywork_tx, 50);
	}

}

static void tuya_ble_connect(BOOL connected)
{
	SYS_LOG_INF("%s", (connected) ? "connected" : "disconnected");
	if (connected) {
		if (stream_open(tuya_ble_stream, MODE_IN_OUT)) {
			SYS_LOG_ERR("stream_open Failed");
			return;
		}

		//tuya_auth_timeout_start(10000);
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &connect_delaywork, 0);

		//ble_adv_limit_set(SYS_BLE_LIMITED_APP);
		tuya_resource_alloc();
		ap_status_set(AP_STATUS_LINKED);
	} else {
		//tuya_auth_timeout_stop();
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &disconnect_delaywork, 0);
		//ble_adv_limit_clear(SYS_BLE_LIMITED_APP);
		tuya_resource_release();
		ap_status_set(AP_STATUS_NONE);
	}
}


/* Start after bt engine ready */
void tuya_ble_stream_init(void)
{
#if 1
	struct tuya_ble_stream_init_param init_param;

	os_work_q_start(&tuya_spp_ble_q, (os_thread_stack_t *)ble_tuya_stack, BLE_TEST_STACKSIZE, 11);
	os_delayed_work_init(&connect_delaywork, tuya_connect_delaywork);
	os_delayed_work_init(&disconnect_delaywork, tuya_disconnect_delaywork);
	os_delayed_work_init(&run_delaywork_tx, tuya_run_delaywork_tx);
	os_delayed_work_init(&run_delaywork_rx, tuya_run_delaywork_rx);
	//thread_timer_init(&auth_timer, tuya_ble_active_disconnect, NULL);
	os_delayed_work_init(&auth_work, tuya_ble_active_disconnect);
	os_delayed_work_init(&find_work, tuya_find_device_start_notify);
	os_delayed_work_init(&vos_start_work, tuya_dev_record_start);
	os_delayed_work_init(&vos_stop_work, tuya_dev_record_stop);

	init_param.gatt_attr = &tuya_gatt_attr[0];
	init_param.attr_size = ARRAY_SIZE(tuya_gatt_attr);
	init_param.write_norsp_attr = &tuya_gatt_attr[5];
	init_param.notify_chrc_attr = &tuya_gatt_attr[1];
	init_param.notify_attr = &tuya_gatt_attr[2];

	init_param.notify_ccc_attr = &tuya_gatt_attr[3];

	init_param.connect_cb = tuya_ble_connect;
	//SYS_LOG_ERR("connect_cb 0x%p func 0x%p",init_param.connect_cb,tuya_ble_connect);

	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.read_timeout = OS_NO_WAIT;
	init_param.write_timeout = OS_NO_WAIT;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	tuya_ble_stream = tuya_ble_stream_create(&init_param);
	if (!tuya_ble_stream) {
		SYS_LOG_ERR("stream_create failed");
	}
#endif
	tuya_ble_app_init();

}

void tuya_ble_active_disconnect(os_work *work)
{	
	if (ap_status_get() < AP_STATUS_AUTH)
	{
		SYS_LOG_INF("DISCONNECT");
		if(bt_manager_ble_is_connected())
			bt_manager_ble_disconnect();
	}
}

void tuya_auth_timeout_start(u32_t duration)
{	
	//if (!thread_timer_is_running(&auth_work))
	{
		SYS_LOG_INF("auth timer creater");
		//thread_timer_start(&auth_timer, duration, 0);
		os_delayed_work_submit(&auth_work, duration);
	}
}

void tuya_auth_timeout_stop(void)
{	
	//if (thread_timer_is_running(&auth_work))
	{
		SYS_LOG_INF("auth timer destory");
		//thread_timer_stop(&auth_timer);
		os_delayed_work_cancel(&auth_work);
	}
}


void tuya_find_device_start_notify(void)
{
	//sys_manager_delete_timer(&find_device_timer_id);
	
	if (bt_manager_tws_get_dev_role() == BTSRV_TWS_MASTER)
	{
		bt_manager_tws_send_event_sync(TWS_UI_EVENT, SYS_EVENT_CUSTOMED_5);
	}
	else
	{
		sys_event_notify(SYS_EVENT_CUSTOMED_5);
	}
	os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &find_work, 6200);

	//find_device_timer_id = app_timer_delay_proc(6200,(void*)tuya_find_device_start_notify, NULL);
}

void tuya_find_device_stop_notify(void)
{
	//sys_manager_delete_timer(&find_device_timer_id);
	os_delayed_work_cancel(&find_work);
}

void tuya_dev_sent_record_start_stop(u8_t cmd)
{
	if (0 == cmd)
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &vos_start_work, 0);
	else
		os_delayed_work_submit_to_queue(&tuya_spp_ble_q, &vos_stop_work, 0);
}

void tuya_record_resource_release(void)
{
	if (tuya_info) {
		if (tuya_info->record_send_buf.data_buf)
		{
			app_mem_free(tuya_info->record_send_buf.data_buf);
			memset(&tuya_info->record_send_buf, 0 , sizeof(tuya_info->record_send_buf));
		}
	}
}

