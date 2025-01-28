/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <thread_timer.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <app_switch.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <sys_wakelock.h>
#include <ota_backend_bt.h>
#ifdef CONFIG_UI_MANAGER
#include <ui_manager.h>
#endif
#include <drivers/flash.h>
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
//#ifdef CONFIG_FILE_SYSTEM
#include <fs_manager.h>
#include <fs/fs.h>
#include <file_stream.h>
//#endif
#include <shell/shell.h>

#include "vendor_app.h"

#define FILE_DIR "/NAND:/"

io_stream_t ble_stream_create(void *param);

struct tlv_data {
	uint8_t  type;
	uint16_t length;
	uint8_t  value[0];
} __packed;

struct vs_pkt_hdr {
	uint8_t svc_id;
	uint8_t cmd_id;
	struct tlv_data tl;
} __packed;

static vendor_app_t *p_vendor_app;

/* Vendor service uuid */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define WATCH_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xc0, 0x23, 0x8a, 0xf4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define WS_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xc1, 0x24, 0x8a, 0xf4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define WS_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xc2, 0x25, 0x8a, 0xf4)

static struct bt_gatt_attr ws_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(WATCH_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(WS_CHA_RX_UUID,
                    BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
					BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(WS_CHA_TX_UUID,
                    BT_GATT_CHRC_NOTIFY,
				    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

static inline vendor_app_t *vendor_app_get(void)
{
	return p_vendor_app;
}

static void send_app_msg(uint8_t type)
{
	struct app_msg msg = {0};

	msg.type = type;

	send_async_msg("vendor", &msg);
}

static int read_data(void *buf, uint16_t buf_len, uint16_t len)
{
	int ret;
	vendor_app_t *app = vendor_app_get();

	if (buf_len >= len) {
		ret = stream_read(app->ble_stream, buf, len);
		return ret;
	}

	ret = stream_read(app->ble_stream, app->buf, len);
	if (ret) {
		memcpy(buf, app->buf, buf_len);
	}	

	return ret;
}
#if 0
static void vendor_app_exit(void)
{
	if (srv_manager_check_service_is_actived("vendor")) {
		srv_manager_exit_service("vendor");
	}

	SYS_LOG_INF("vendor service stop");
}
#endif
static int vendor_app_stop(void)
{
	vendor_app_t *app = vendor_app_get();

	app->stream_ready = 0;
	return stream_close(app->ble_stream);
}

static void ble_connect_cb(bool connected)
{
	SYS_LOG_INF("connect: %d", connected);

    if (connected) {
        send_app_msg(MSG_START_APP);
    } else {
		vendor_app_stop();
	}
}

static const struct ble_stream_init_param bt_init_param = {
	.gatt_attr = &ws_gatt_attr[0],
	.attr_size = ARRAY_SIZE(ws_gatt_attr),
	.rx_attr = &ws_gatt_attr[2],
	.tx_chrc_attr = &ws_gatt_attr[3],
	.tx_attr = &ws_gatt_attr[4],
	.tx_ccc_attr = &ws_gatt_attr[5],
    .connect_cb = ble_connect_cb,
	.read_timeout = OS_FOREVER,
	.write_timeout = OS_FOREVER,
};

static int tlv_pack(uint8_t *buf, uint8_t type, uint16_t len, uint8_t *value)
{
	uint8_t size = 0;

	buf[size++] = type;
	sys_put_le16(len, &buf[size]);
	size += 2;
	memcpy(&buf[size], value, len);
	size += len;

	return size;
}

static int send_result(uint8_t cmd, uint8_t result)
{
	int ret;
	uint8_t buf[16];
	uint8_t len = 0;

	buf[len++] = SVC_WATCH;
	buf[len++] = CMD_D2H_RESULT;
	/* param command */
	len += tlv_pack(&buf[len], TLV_RESULT_CMD, 0x01, &cmd);
	/* param result */
	len += tlv_pack(&buf[len], TLV_RESULT_CODE, 0x01, &result);

	ret = stream_write(p_vendor_app->ble_stream, buf, len);
	if (ret != len) {
		return -EIO;
	}

	return 0;
}

static uint8_t set_time(struct vs_date_time *time, struct rtc_time *rtc)
{
	time->year = sys_le16_to_cpu(time->year);

	SYS_LOG_INF("%d-%02d-%02d %02d:%02d:%02d",
			time->year, time->month, time->day,
			time->hours, time->minutes, time->seconds);

	rtc->tm_year = time->year;
	rtc->tm_mon = time->month;
	rtc->tm_mday = time->day;
	rtc->tm_hour = time->hours;
	rtc->tm_min = time->minutes;
	rtc->tm_sec = time->seconds;
	rtc->tm_wday = time->day_of_week;

	struct app_msg  msg = {0};
	msg.type = MSG_BT_MGR_EVENT;
	msg.cmd = BT_MAP_SET_TIME_EVENT;
	msg.ptr = rtc;
	send_async_msg(app_manager_get_current_app(), &msg);

	return VS_CMD_SUCCEED;
}

static int cmd_set_time(uint16_t param_size)
{
	int rd;
	uint8_t ret;
	struct tlv_data tlv;
	vendor_app_t *app = vendor_app_get();

	while (param_size) {
		rd = stream_read(app->ble_stream, &tlv, sizeof(struct tlv_data));
		if (rd <= 0) {
			break;
		}
		param_size -= rd;

		if (tlv.type != TLV_DATE_TIME) {
			break;
		}

		rd = stream_read(app->ble_stream, (void *)&app->time, tlv.length);
		if (rd <= 0) {
			break;
		}
		param_size -= rd;
	}

	if (!param_size) {
		ret = set_time(&app->time, &app->rtc_time);
	} else {
		ret = VS_CMD_FAILED;
	}

	return send_result(CMD_H2D_SET_TIME, ret);
}

static uint8_t save_file(struct vs_file_info *file)
{
	int rd, err;
	uint8_t timeout = 0;
	vendor_app_t *app = vendor_app_get();

	file->length = sys_le32_to_cpu(file->length);
	SYS_LOG_INF("name %s, type %d, date %s, length %u",
		file->name, file->type, file->date, file->length);

	strcpy(file->path, FILE_DIR);
	strcat(file->path, file->name);
	SYS_LOG_INF("file path %s", file->path);

	app->file_stream = file_stream_create(file->path);
	if (!app->file_stream) {
		SYS_LOG_ERR("create file stream failed");
	}

	if (app->file_stream) {
		stream_open(app->file_stream, MODE_OUT);
	}

	while (file->length) {
		rd = stream_read(p_vendor_app->ble_stream, file->buf, sizeof(file->buf));
		if (rd < 0) {
			SYS_LOG_ERR("read file data failed %d", rd);
			break;
		}
		if (rd == 0) {
			/* recv file timeout */
			if (timeout == 10) {
				break;
			}
			os_sleep(100);
			timeout++;
		}
		file->length -= rd;
		
		//SYS_LOG_INF("recv data %u, remain %u", rd, file->length);
		if (app->file_stream) {
			err = stream_write(app->file_stream, file->buf, rd);
			if (err < 0) {
				break;
			}
		}
	}

	if (app->file_stream) {
		//stream_flush(app->file_stream);
		stream_close(app->file_stream);
		stream_destroy(app->file_stream);
		app->file_stream = NULL;
	}

	/* if file incomplete, then delete it */
	if (file->length) {
		SYS_LOG_INF("file recv incomplete");
		fs_unlink(file->path);
		return VS_CMD_FAILED;
	}

	SYS_LOG_INF("file recv complete");
	return VS_CMD_SUCCEED;
}

static int cmd_send_file(uint16_t param_size)
{
	int rd;
	uint8_t ret;
	struct tlv_data tlv;
	vendor_app_t *app = vendor_app_get();

	app->file = app_mem_malloc(sizeof(struct vs_file_info));
	if (!app->file) {
		return 0;
	}
	memset(app->file, 0, sizeof(struct vs_file_info));

	while (param_size) {
		uint16_t len = 0;

		rd = stream_read(app->ble_stream, &tlv, sizeof(struct tlv_data));
		if (rd <= 0) {
			break;
		}
		tlv.length = sys_le16_to_cpu(tlv.length);
		param_size -= rd;

		switch (tlv.type) {
		case TLV_FILE_NAME:
			len = VS_NAME_LEN - 1;
			rd = read_data(app->file->name, len, tlv.length);
			app->file->name[len] = '\0';
			break;
		case TLV_FILE_TYPE:
			rd = read_data(&app->file->type, 1, tlv.length);
			break;
		case TLV_FILE_DATE:
			len = VS_DATE_LEN - 1;
			rd = read_data(app->file->date, len, tlv.length);
			app->file->date[len] = '\0';
			break;
		case TLV_FILE_LENGTH:
			rd = read_data(&app->file->length, 4, tlv.length);
			break;
		default:
			SYS_LOG_ERR("unknown tlv type %u", tlv.type);
			break;
		}

		if (rd <= 0) {
			break;
		}
		param_size -= rd;
	}

	if (!param_size) {
		ret = save_file(app->file);
	} else {
		ret = VS_CMD_FAILED;
	}

	app_mem_free(app->file);
	app->file = NULL;
	return send_result(CMD_H2D_SEND_FILE, ret);
}

static uint8_t show_msg(struct vs_msg_info *msg)
{
	SYS_LOG_INF("name %s", msg->app_name);
	SYS_LOG_INF("title %s", msg->title);
	SYS_LOG_INF("text %s", msg->text);
	SYS_LOG_INF("date %s", msg->date);

	//TODO send msg to ui service
	return VS_CMD_SUCCEED;
}

static int cmd_send_msg(uint16_t param_size)
{
	int rd;
	uint8_t ret;
	struct tlv_data tlv;
	vendor_app_t *app = vendor_app_get();

	app->msg = app_mem_malloc(sizeof(struct vs_msg_info));
	if (!app->msg) {
		return 0;
	}
	memset(app->msg, 0, sizeof(struct vs_msg_info));

	while (param_size) {
		uint16_t len;

		rd = stream_read(app->ble_stream, &tlv, sizeof(struct tlv_data));
		if (rd <= 0) {
			break;
		}
		tlv.length = sys_le16_to_cpu(tlv.length);
		param_size -= rd;

		switch (tlv.type) {
		case TLV_MSG_APP_NAME:
			len = VS_NAME_LEN - 1;
			rd = read_data(app->msg->app_name, len, tlv.length);
			app->msg->app_name[len] = '\0';
			break;
		case TLV_MSG_TITLE:
			len = VS_TEXT_LEN - 1;
			rd = read_data(app->msg->title, len, tlv.length);
			app->msg->title[len] = '\0';
			break;
		case TLV_MSG_TEXT:
			len = VS_TEXT_LEN - 1;
			rd = read_data(app->msg->text, len, tlv.length);
			app->msg->text[len] = '\0';
			break;
		case TLV_MSG_DATE:
			len = VS_DATE_LEN - 1;
			rd = read_data(app->msg->date, len, tlv.length);
			app->msg->date[len] = '\0';
			break;
		default:
			SYS_LOG_ERR("unknown tlv type %u", tlv.type);
			break;
		}

		if (rd <= 0) {
			break;
		}
		param_size -= rd;
	}

	if (!param_size) {
		ret = show_msg(app->msg);
	} else {
		ret = VS_CMD_FAILED;
	}
	
	app_mem_free(app->msg);
	app->msg = NULL;
	return send_result(CMD_H2D_SEND_MSG, ret);
}

/* command handlers */
static const struct {
	int (*func)(uint16_t param_size);
} handlers[] = {
	{ }, /* No cmd-id defined for 0x00 */
	{ cmd_set_time },
	{ cmd_send_file },
	{ cmd_send_msg },
};

static int ble_stream_read(void)
{
	int ret;
	struct vs_pkt_hdr hdr;
	vendor_app_t *app = vendor_app_get();

	ret = stream_read(app->ble_stream, &hdr, sizeof(struct vs_pkt_hdr));
	if (ret <= 0) {
		return 0;
	}

    SYS_LOG_INF("svc_id %x, cmd_id %x", hdr.svc_id, hdr.cmd_id);

	if (hdr.svc_id != SVC_WATCH) {
		return 0;
	}

	if (hdr.cmd_id >= ARRAY_SIZE(handlers)) {
		return 0;
	}

	if (handlers[hdr.cmd_id].func == NULL) {
		return 0;
	}

	if (hdr.tl.type != TLV_PARAMS_LENGTH) {
		return 0;
	}

	hdr.tl.length = sys_le16_to_cpu(hdr.tl.length);
	SYS_LOG_INF("param size %u", hdr.tl.length);

	handlers[hdr.cmd_id].func(hdr.tl.length);

	return ret;
}

#define CONFIG_VENDOR_READ_STACKSIZE	2048
static char __aligned(ARCH_STACK_PTR_ALIGN) __in_section_unique(noinit)
vendor_read_stack[CONFIG_VENDOR_READ_STACKSIZE];

static void vendor_read_thread(void *p1, void *p2, void *p3)
{
#ifndef CONFIG_SIMULATOR
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
#endif
	vendor_app_t *app = vendor_app_get();

	SYS_LOG_INF("vendor_read thread started");

	while (1) {
		if (app->stream_ready) {
			ble_stream_read();
		} else {
			os_sleep(10);
		}
	}
}

static int vendor_app_start(void)
{
	int ret;
	vendor_app_t *app = vendor_app_get();

	SYS_LOG_INF("vendor app start");

	ret = stream_open(app->ble_stream, MODE_IN_OUT);
	if (ret) {
		SYS_LOG_ERR("ble stream open failed %d", ret);
		return ret;
	}
	app->stream_ready = 1;

	if (app->read_tid == 0) {
		app->read_tid = (os_tid_t)os_thread_create((char*)vendor_read_stack,
			CONFIG_VENDOR_READ_STACKSIZE, vendor_read_thread,
			NULL, NULL, NULL, 13, 0, OS_NO_WAIT);
		os_thread_name_set(app->read_tid, "vendor_read");
	}

	return ret;
}

int vendor_app_init(void)
{
    p_vendor_app = app_mem_malloc(sizeof(vendor_app_t));
	if (!p_vendor_app) {
		return -ENOMEM;
	}

	memset(p_vendor_app, 0, sizeof(vendor_app_t));

#ifdef CONFIG_RTC_ACTS
	p_vendor_app->rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	if (!p_vendor_app->rtc_dev) {
		SYS_LOG_ERR("rtc device " CONFIG_RTC_0_NAME " not found");
	}
#endif

    p_vendor_app->ble_stream = ble_stream_create((void *)&bt_init_param);
	if (p_vendor_app->ble_stream == NULL) {
		SYS_LOG_ERR("ble stream create failed");
	}

	/* active vendor service */
	if (!srv_manager_check_service_is_actived("vendor")) {
		srv_manager_active_service("vendor");
	}
    SYS_LOG_INF("vendor service start");

	return 0;
}

static void vendor_app_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg = {0};
	bool terminaltion = false;

	SYS_LOG_INF("enter");

	while (!terminaltion) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_START_APP:
				vendor_app_start();
				break;
			case MSG_EXIT_APP:
				terminaltion = true;
				break;
			case MSG_SUSPEND_APP:
				SYS_LOG_INF("SUSPEND_APP");
				break;
			case MSG_RESUME_APP:
				SYS_LOG_INF("RESUME_APP");
				break;
			default:
				SYS_LOG_ERR("unknown: 0x%x!", msg.type);
				break;
			}
			if (msg.callback != NULL)
				msg.callback(&msg, result, NULL);
		}

		if (!terminaltion) {
		    thread_timer_handle_expired();
		}
	}
}

#define CONFIG_VENDOR_STACKSIZE 1536
char __aligned(ARCH_STACK_PTR_ALIGN) vendor_stack_area[CONFIG_VENDOR_STACKSIZE];

SERVICE_DEFINE(vendor, \
                vendor_stack_area, CONFIG_VENDOR_STACKSIZE, \
				13, BACKGROUND_APP, \
                NULL, NULL, NULL,   \
				vendor_app_main);
