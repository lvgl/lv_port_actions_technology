/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#include <kernel.h>
#include <string.h>
#include <stream.h>
#include <soc.h>
#include <mem_manager.h>
#include <ota_backend.h>
#include <ota_backend_bt.h>
#include <bt_manager.h>
#include <crc.h>

/* support check data checksum in each ota unit */
#define CONFIG_OTA_BT_SUPPORT_UNIT_CRC

#define TLV_TYPE_OTA_SUPPORT_FEATURES		0x09
#define TLV_TYPE_OTA_TYPE					0x0A

#define OTA_SUPPORT_FEATURE_UNIT_DATA_CRC	(1 << 0)
#define OTA_ERROR_CODE_SUCCESS		100000
#define OTA_ERROR_CODE_OTA_TYPE_FAIL	100010

#define SERVICE_ID_OTA		0x9

#define OTA_SVC_SEND_BUFFER_SIZE	0x80

#define OTA_UNIT_SIZE			0x100
#define OTA_UNIT_FRCOMM_SIZE	(650) // SPP Recommended Value <= (670-10 = 660)
#define OTA_UNIT_BLE_SIZE		(230) // BLE Recommended value <= (244-10 = 234)
#define OTA_IOS_BLE_MAX_PKT		(50)  // IOS BLE Recommended value <= 60
#define OTA_SPPBLE_BUFF_SIZE	(OTA_UNIT_FRCOMM_SIZE*6 + 200) // OTA requires 6 block sizes

#define TLV_MAX_DATA_LENGTH	0x3fff
#define TLV_TYPE_ERROR_CODE	0x7f
#define TLV_TYPE_MAIN		0x80

#define OTA_CMD_H2D_REQUEST_UPGRADE		0x01
#define OTA_CMD_H2D_CONNECT_NEGOTIATION		0x02
#define OTA_CMD_D2H_REQUIRE_IMAGE_DATA		0x03
#define OTA_CMD_H2D_SEND_IMAGE_DATA		0x04
#define OTA_CMD_D2H_REPORT_RECEVIED_DATA_COUNT	0x05
#define OTA_CMD_D2H_VALIDATE_IMAGE		0x06
#define OTA_CMD_D2H_REPORT_STATUS		0x07
#define OTA_CMD_D2H_CANCEL_UPGRADE		0x08
#define OTA_CMD_H2D_NEGOTIATION_RESULT		0x09
#define OTA_CMD_D2H_REQUEST_UPGRADE		0x0A
#define OTA_CMD_H2D_SEND_IMAGE_DATA_WITH_CRC	0x0B

#define TLV_PACK_U8(buf, type, value)	tlv_pack_data(buf, type, sizeof(uint8_t), value)
#define TLV_PACK_U16(buf, type, value)	tlv_pack_data(buf, type, sizeof(uint16_t), value)
#define TLV_PACK_U32(buf, type, value)	tlv_pack_data(buf, type, sizeof(uint32_t), value)

#define TLV_UNPACK_U8(ctx, type, value_ptr)	\
		tlv_unpack_data(ctx, type, NULL, (uint8_t *)value_ptr, sizeof(uint8_t))
#define TLV_UNPACK_U16(ctx, type, value_ptr)	\
		tlv_unpack_data(ctx, type, NULL, (uint8_t *)value_ptr, sizeof(uint16_t))
#define TLV_UNPACK_U32(ctx, type, value_ptr)	\
		tlv_unpack_data(ctx, type, NULL, (uint8_t *)value_ptr, sizeof(uint32_t))

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>

#define ble_ota_wake_lock()		sys_wake_lock(PARTIAL_WAKE_LOCK)
#define ble_ota_wake_unlock() 		sys_wake_unlock(PARTIAL_WAKE_LOCK)
#else
#define ble_ota_wake_lock()
#define ble_ota_wake_unlock()
#endif

static os_delayed_work ble_ota_uparam_work;
#endif
#endif

enum svc_prot_state{
	PROT_STATE_IDLE,
	PROT_STATE_DATA,
};

struct svc_prot_context {
	uint8_t state;
	int send_buf_size;
	uint8_t *send_buf;

	uint8_t *read_buf;
	int read_len;
	int read_offset;
	int read_done_len;
	int read_buf_len;
	uint8_t last_psn;
	uint8_t host_features;
	uint16_t ota_unit_size;

	io_stream_t sppble_stream;
	int sppble_stream_opened;
	int negotiation_done;
	ota_backend_type_cb_t ota_type_cb;
	int ota_type; // 0: app ota 1: factory ota 2: PC config ota
	uint8_t connect_type;
};

struct svc_prot_head {
	uint8_t svc_id;
	uint8_t cmd;
	uint8_t param_type;
	uint16_t param_len;
} __attribute__((packed));

struct tlv_head {
	uint8_t type;
	uint16_t len;
	uint8_t value[0];
} __attribute__((packed));

struct tlv_data {
	uint8_t type;
	uint16_t len;
	uint16_t max_len;
	uint8_t *value_ptr;
};

typedef int (*svc_prot_cmd_handler_t)(struct svc_prot_context *ctx, uint16_t param_len);
struct svc_prot_cmd {
	uint8_t cmd;
	svc_prot_cmd_handler_t handler;
};

struct ota_backend_bt {
	struct ota_backend backend;
	struct svc_prot_context svc_ctx;
};

/* for sppble_stream callback */
static struct ota_backend_bt *g_backend_bt;

static int svc_prot_get_rx_data(struct svc_prot_context *ctx, uint8_t *buf, int size)
{
	int read_size;

	while (size > 0) {
		read_size = stream_read(ctx->sppble_stream, buf, size);
		if (read_size <= 0) {
			SYS_LOG_ERR("need read %d bytes, but only got %d bytes",
				size, read_size);
			return -EIO;
		}

		size -= read_size;
		buf += read_size;
	}

	return 0;
}

static int svc_prot_skip_rx_data(struct svc_prot_context *ctx, int size)
{
	int err;
	uint8_t c;

	while (size > 0) {
		err = svc_prot_get_rx_data(ctx, &c, sizeof(uint8_t));
		if (err) {
			SYS_LOG_ERR("failed to get data");
			return err;
		}

		size--;
	}

	return 0;
}

static int svc_drop_all_rx_data(struct svc_prot_context *ctx, int wait_ms)
{
	int err, data_len;
	int ms = wait_ms;

	while (wait_ms > 0) {
		data_len = stream_tell(ctx->sppble_stream);
		if (data_len > 0) {
			SYS_LOG_INF("drop data len 0x%x", data_len);

			err = svc_prot_skip_rx_data(ctx, data_len);
			if (err)
				return err;

			wait_ms = ms;
		}

		os_sleep(20);
		wait_ms -= 20;
	}

	return 0;
}

static uint8_t *tlv_pack(uint8_t *buf, const struct tlv_data *tlv)
{
	uint16_t len;

	len = tlv->len;

	*buf++ = tlv->type;
	*buf++ = len & 0xff;
	*buf++ = len >> 8;

	if (tlv->value_ptr) {
		memcpy(buf, tlv->value_ptr, len);
		buf += len;
	}

	return buf;
}

static uint8_t *tlv_pack_data(uint8_t *buf, uint8_t type, uint16_t len, uint32_t number)
{
	struct tlv_data tlv;

	tlv.type = type;
	tlv.len = len;
	tlv.value_ptr = (uint8_t *)&number;

	return tlv_pack(buf, &tlv);
}

static int tlv_unpack_head(struct svc_prot_context *ctx, struct tlv_data *tlv)
{
	int err, total_len;

	/* read type */
	err = svc_prot_get_rx_data(ctx, &tlv->type, sizeof(tlv->type));
	if (err) {
		SYS_LOG_ERR("failed to read tlv type");
		return err;
	}

	/* read length */
	err = svc_prot_get_rx_data(ctx, (uint8_t *)&tlv->len, sizeof(tlv->len));
	if (err) {
		SYS_LOG_ERR("failed to read tlv type");
		return err;
	}

	total_len = sizeof(tlv->type) + sizeof(tlv->len);

	return total_len;
}

static int tlv_unpack(struct svc_prot_context *ctx, struct tlv_data *tlv)
{
	uint16_t data_len;
	int err, total_len;

	total_len = tlv_unpack_head(ctx, tlv);
	if (tlv->len <= 0)
		return total_len;

	data_len = tlv->len;
	if (data_len > tlv->max_len) {
		data_len = tlv->max_len;
	}

	/* read value */
	if (tlv->value_ptr) {
		err = svc_prot_get_rx_data(ctx, tlv->value_ptr, data_len);
		if (err) {
			SYS_LOG_ERR("failed to read tlv value");
			return err;
		}

		total_len += data_len;
	}

	return total_len;
}

static int tlv_unpack_data(struct svc_prot_context *ctx, uint8_t type, int *len,
			   uint8_t *value_ptr, int max_len)
{
	struct tlv_data tlv;
	int rlen;

	tlv.value_ptr = value_ptr;
	tlv.max_len = max_len;

	rlen = tlv_unpack(ctx, &tlv);
	if (rlen < 0) {
		SYS_LOG_ERR("tlv_unpack failed, err 0x%x", rlen);
		return rlen;
	}

	if (tlv.type != type) {
		SYS_LOG_ERR("unmatched type, need 0x%x but got 0x%x", type, tlv.type);
		return -EIO;
	}

	if (len)
		*len = tlv.len;

	return rlen;
}

int svc_prot_send_data(struct svc_prot_context *ctx, uint8_t *buf, int size)
{
	int err;

	err = stream_write(ctx->sppble_stream, buf, size);
	if (err < 0) {
		SYS_LOG_ERR("failed to send data, size %d, err %d", size, err);
		return -EIO;
	}

	return 0;
}

int ota_bt_send_cmd(struct svc_prot_context *ctx, uint8_t cmd,
		    uint8_t *buf, int size)
{
	struct svc_prot_head *head;
	int err;

	head = (struct svc_prot_head *)buf;
	head->svc_id = SERVICE_ID_OTA;
	head->cmd = cmd;
	head->param_type = TLV_TYPE_MAIN;
	head->param_len = size - sizeof(struct svc_prot_head);

	//SYS_LOG_INF("send cmd: %d", cmd);
	//print_buffer(buf, 1, size, 16, buf);

	err = svc_prot_send_data(ctx, buf, size);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, err %d", cmd, err);
		return err;
	}

	return 0;
}

int ota_cmd_h2d_request_upgrade(struct svc_prot_context *ctx, uint16_t param_size)
{
	int err = 0, send_len, battery_threahold, head_len, ota_type;
	uint8_t *send_buf;
	struct tlv_data tlv;
	uint8_t host_features, device_features;

	SYS_LOG_INF("upgrade request: param_size %d", param_size);

	if (1 == ctx->negotiation_done) {
		SYS_LOG_WRN("request upgrade cmd repeated.");
	} else {
		ctx->host_features = 0;
		ctx->ota_type = PROT_OTA_PHONE_APP;
		device_features = 0;
	}

	while (param_size > 0) {
		head_len = tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_SUPPORT_FEATURES:
			err = svc_prot_get_rx_data(ctx, &host_features, 1);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}

			ctx->host_features = host_features;
			SYS_LOG_INF("host support features: 0x%x", host_features);
			break;
		case TLV_TYPE_OTA_TYPE:
 			err = svc_prot_get_rx_data(ctx, (uint8_t *)&ota_type, 4);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
			
			ctx->ota_type = ota_type;
			SYS_LOG_INF("ota type: 0x%x", ota_type);
			break;

		default:
			/* skip other parameters by now */
			err = svc_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;
		}

		param_size -= head_len + tlv.len;
	}

	if (1 == ctx->negotiation_done) {
		return 0;
	}

	ctx->state = PROT_STATE_IDLE;

	/* dummy value, for debug only */
	battery_threahold = 30;

	/* init ack data */
	send_buf = ctx->send_buf + sizeof(struct svc_prot_head);

	if (ctx->ota_type_cb)
		err = ctx->ota_type_cb(ctx->ota_type);

	if (err) {
		send_buf = TLV_PACK_U32(send_buf, TLV_TYPE_ERROR_CODE, OTA_ERROR_CODE_OTA_TYPE_FAIL);
	} else {
		send_buf = TLV_PACK_U32(send_buf, TLV_TYPE_ERROR_CODE, OTA_ERROR_CODE_SUCCESS);
	}

	send_buf = TLV_PACK_U8(send_buf, 0x04, battery_threahold);

	/* send device features only if host has support features */
	if (ctx->host_features) {
#ifdef CONFIG_OTA_BT_SUPPORT_UNIT_CRC
		device_features |= OTA_SUPPORT_FEATURE_UNIT_DATA_CRC;
#endif
		send_buf = TLV_PACK_U8(send_buf, TLV_TYPE_OTA_SUPPORT_FEATURES, device_features);
		SYS_LOG_INF("device support features: 0x%x", 0);
	}

	send_len = send_buf - ctx->send_buf;

	err = ota_bt_send_cmd(ctx, OTA_CMD_H2D_REQUEST_UPGRADE, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_H2D_REQUEST_UPGRADE, err);
		return err;
	}

	return err;
}

int ota_cmd_h2d_connect_negotiation(struct svc_prot_context *ctx, uint16_t param_len)
{
	uint16_t app_wait_timeout, device_restart_timeout, ota_unit_size, interval;
	uint8_t ack_enable;
	uint8_t *send_buf;
	int err, send_len;

	if (param_len != 0) {
		SYS_LOG_ERR("read_param_len 0, real param_len %d\n", param_len);
	}

	if (0 == ctx->negotiation_done) {
		ctx->state = PROT_STATE_IDLE;
	}

	app_wait_timeout = 3;
	device_restart_timeout = 5;
	ota_unit_size = ctx->ota_unit_size;
	interval = 0;
	ack_enable= 0;

	SYS_LOG_INF("send app_wait_timeout %d, device_restart_timeout %d\n",
		app_wait_timeout, device_restart_timeout);
	SYS_LOG_INF("ota_unit_size %d, interval %d, ack_enable %d\n",
		ota_unit_size, interval, ack_enable);

	send_buf = ctx->send_buf + sizeof(struct svc_prot_head);
	send_buf = TLV_PACK_U16(send_buf, 0x1, app_wait_timeout);
	send_buf = TLV_PACK_U16(send_buf, 0x2, device_restart_timeout);
	send_buf = TLV_PACK_U16(send_buf, 0x3, ota_unit_size);
	send_buf = TLV_PACK_U16(send_buf, 0x4, interval);
	send_buf = TLV_PACK_U8(send_buf, 0x5, ack_enable);
	send_len = send_buf - ctx->send_buf;

	if (1 == ctx->negotiation_done) {
		SYS_LOG_WRN("connect negotiation cmd repeated.");
		return 0;
	}

	err = ota_bt_send_cmd(ctx, OTA_CMD_H2D_CONNECT_NEGOTIATION, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_H2D_CONNECT_NEGOTIATION, err);
		return err;
	}

	return err;
}

int ota_cmd_h2d_negotiation_result(struct svc_prot_context *ctx, uint16_t param_len)
{
	uint8_t negotiation_result;
	int rlen;

	rlen = TLV_UNPACK_U8(ctx, 0x1, &negotiation_result);
	if (rlen < 0) {
		return -EIO;
	}

	if (1 == ctx->negotiation_done) {
		SYS_LOG_WRN("negotiation result cmd repeated.");
		return 0;
	}

	if (rlen != param_len) {
		SYS_LOG_ERR("read param len %d, but real param_len %d\n", rlen, param_len);
	}

	SYS_LOG_INF("negotiation_result %d\n", negotiation_result);

	ctx->negotiation_done = 1;

	ctx->state = PROT_STATE_IDLE;

	return 0;
}

int ota_cmd_require_image_data(struct svc_prot_context *ctx, uint32_t offset, int len, uint8_t *buf)
{
	uint8_t read_mask, *send_buf;
	int err, send_len;

	SYS_LOG_DBG("offset 0x%x, len %d, buf %p, \n", offset, len, buf);

	if (!ctx->negotiation_done) {
		SYS_LOG_ERR("negotiation not done");
		return -EIO;

	}

	ctx->read_buf = buf;
	ctx->read_len = len;
	ctx->read_offset = offset;
	ctx->read_done_len = 0;
	ctx->read_buf_len = 0;
	ctx->last_psn = 0xff;

	if (!buf) {
		ctx->state = PROT_STATE_IDLE;
		return 0;
	}

	read_mask = 0x0;

	send_buf = ctx->send_buf + sizeof(struct svc_prot_head);
	send_buf = TLV_PACK_U32(send_buf, 0x1, offset);
	send_buf = TLV_PACK_U32(send_buf, 0x2, len);
	send_buf = TLV_PACK_U8(send_buf, 0x3, read_mask);
	send_len = send_buf - ctx->send_buf;

	err = ota_bt_send_cmd(ctx, OTA_CMD_D2H_REQUIRE_IMAGE_DATA, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_D2H_REQUIRE_IMAGE_DATA, err);
		return err;
	}

	ctx->state = PROT_STATE_IDLE;

	return 0;
}

int ota_cmd_h2d_send_image_data(struct svc_prot_context *ctx, uint16_t param_len, int with_crc)
{
	int err, seg_len;
	uint8_t psn;
	uint32_t crc, crc_orig;

	SYS_LOG_DBG("buf %p, len %d", ctx->read_buf, ctx->read_len);

	if (ctx->read_buf == NULL || ctx->read_len == 0 || param_len < 1) {
		svc_drop_all_rx_data(ctx, 20);
		return -EINVAL;
	}

	/* read psn */
	err = svc_prot_get_rx_data(ctx, &psn, sizeof(uint8_t));
	if (err) {
		SYS_LOG_ERR("read error, err %d", err);
		return -EIO;
	}
	param_len -= sizeof(uint8_t);

	SYS_LOG_DBG("last_psn %d, cur psn %d\n", ctx->last_psn, psn);

	if (psn != (uint8_t)(ctx->last_psn + 1)) {
		SYS_LOG_ERR("last_psn %d, cur psn %d not seq\n", ctx->last_psn, psn);
		return -EIO;
	}

	if (with_crc) {
		/* read crc field */
		err = svc_prot_get_rx_data(ctx, (uint8_t *)&crc_orig, sizeof(uint32_t));
		if (err) {
			SYS_LOG_ERR("read error, err %d", err);
			return -EIO;
		}

		param_len -= sizeof(uint32_t);
	}

	seg_len = ctx->read_len - ctx->read_done_len;
	if (seg_len > param_len) {
		seg_len = param_len;
	}

	if (ctx->read_buf_len + seg_len > OTA_DATA_BUFFER_SIZE) {
		SYS_LOG_ERR("read_buf_len %d + seg_len %d > 4KB.\n", ctx->read_buf_len, seg_len);
		return -EIO;
	}
	/* read data */
	err = svc_prot_get_rx_data(ctx, ctx->read_buf + ctx->read_buf_len, seg_len);
	if (err) {
		SYS_LOG_ERR("read error, err %d", err);
		return -EIO;
	}

	if (with_crc) {
		crc = utils_crc32(0, ctx->read_buf + ctx->read_buf_len, seg_len);
		if (crc != crc_orig) {
			SYS_LOG_ERR("psn%d: crc check error, orig 0x%x != 0x%x",
				psn, crc_orig, crc);
			return -EIO;
		}
	}

	param_len -= seg_len;
	if (param_len != 0) {
		SYS_LOG_ERR("unknown remain param, len %d\n", param_len);
		return -EIO;
	}

	ctx->read_done_len += seg_len;
	ctx->read_buf_len += seg_len;
	ctx->last_psn = psn;
	ctx->state = PROT_STATE_IDLE;

	return 0;
}

int ota_cmd_h2d_send_image_data_with_crc(struct svc_prot_context *ctx, uint16_t param_len)
{
	return ota_cmd_h2d_send_image_data(ctx, param_len, 1);
}

int ota_cmd_h2d_send_image_data_no_crc(struct svc_prot_context *ctx, uint16_t param_len)
{
	return ota_cmd_h2d_send_image_data(ctx, param_len, 0);
}

int ota_cmd_d2h_report_image_valid(struct svc_prot_context *ctx, int is_valid)
{
	uint8_t *send_buf;
	uint8_t valid_flag;
	int err, send_len;

	if (!ctx->negotiation_done) {
		SYS_LOG_ERR("negotiation not done");
		return -EIO;

	}

	valid_flag = is_valid ? 1 : 0;

	send_buf = ctx->send_buf + sizeof(struct svc_prot_head);
	send_buf = TLV_PACK_U8(send_buf, 0x1, valid_flag);
	send_len = send_buf - ctx->send_buf;

	err = ota_bt_send_cmd(ctx, OTA_CMD_D2H_VALIDATE_IMAGE, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
		OTA_CMD_D2H_VALIDATE_IMAGE, err);
	}

	/* reset state to idle anyway */
	ctx->state = PROT_STATE_IDLE;

	return err;
}

struct svc_prot_cmd svc_cmds[] = {
	{OTA_CMD_H2D_REQUEST_UPGRADE, ota_cmd_h2d_request_upgrade,},
	{OTA_CMD_H2D_CONNECT_NEGOTIATION, ota_cmd_h2d_connect_negotiation,},
	{OTA_CMD_H2D_NEGOTIATION_RESULT, ota_cmd_h2d_negotiation_result,},
	//{OTA_CMD_D2H_REQUIRE_IMAGE_DATA, ota_cmd_d2h_require_image_data,},
	{OTA_CMD_H2D_SEND_IMAGE_DATA, ota_cmd_h2d_send_image_data_no_crc,},
	{OTA_CMD_H2D_SEND_IMAGE_DATA_WITH_CRC, ota_cmd_h2d_send_image_data_with_crc,},
};


int process_command(struct svc_prot_context *ctx, uint32_t *processed_cmd)
{
	struct svc_prot_head head;
	svc_prot_cmd_handler_t cmd_handler;
	int i, err;

	if (ctx->state != PROT_STATE_IDLE) {
		SYS_LOG_ERR("current state is not idle");
		return -EIO;
	}

	err = svc_prot_get_rx_data(ctx, (uint8_t *)&head, sizeof(struct svc_prot_head));
	if (err) {
		SYS_LOG_ERR("cannot read head bytes");
		return -EIO;
	}

	SYS_LOG_DBG("svc head: svc_id 0x%x, cmd_id 0x%x, param type 0x%x, len 0x%x",
		head.svc_id, head.cmd, head.param_type, head.param_len);

	ctx->state = PROT_STATE_DATA;

	if (head.svc_id != SERVICE_ID_OTA) {
		SYS_LOG_ERR("invalid svc_id: %d", head.svc_id);
		return -EIO;
	}

	if (head.param_type != TLV_TYPE_MAIN) {
		SYS_LOG_ERR("invalid param type: %d", head.svc_id);
		return -EIO;
	}

	for (i = 0; i < ARRAY_SIZE(svc_cmds); i++) {
		if (svc_cmds[i].cmd == head.cmd) {
			cmd_handler = svc_cmds[i].handler;
			err = cmd_handler(ctx, head.param_len);
			if (processed_cmd){
				*processed_cmd = head.cmd;
			}
			if (err) {
				SYS_LOG_ERR("cmd_handler %p, err: %d", cmd_handler, err);
				return err;
			}
		}
	}

	if (i > ARRAY_SIZE(svc_cmds)) {
		SYS_LOG_ERR("invalid cmd: %d", head.cmd);
		return -1;
	}

	SYS_LOG_DBG("after parse ctx %p, state %d", ctx, ctx->state);

	ctx->state = PROT_STATE_IDLE;

	return err;
}

int wait_negotiation_done(struct svc_prot_context *ctx)
{
	int err = 0;

	SYS_LOG_INF("wait, ctx %p\n", ctx);

	while (ctx->negotiation_done == 0) {
		err = process_command(ctx, NULL);
		if (err) {
			break;
		}
	}

	SYS_LOG_INF("wait ctx %p, return %d\n", ctx, err);

	return err;
}

int ota_backend_bt_ioctl(struct ota_backend *backend, int cmd, unsigned int param)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err;

	SYS_LOG_INF("cmd 0x%x: param %d\n", cmd, param);

	switch (cmd) {
	case OTA_BACKEND_IOCTL_REPORT_IMAGE_VALID:
		err = ota_cmd_d2h_report_image_valid(svc_ctx, param);
		if (err) {
			SYS_LOG_INF("send cmd 0x%x error", cmd);
			return -EIO;
		}
		break;
	case OTA_BACKEND_IOCTL_REPORT_PROCESS:
		backend->cb(backend, OTA_BACKEND_UPGRADE_PROGRESS, param);
		break;
	case OTA_BACKEND_IOCTL_GET_UNIT_SIZE:
		*(unsigned int*)param = svc_ctx->ota_unit_size;
		break;
	case OTA_BACKEND_IOCTL_GET_MAX_SIZE:
		if (svc_ctx->ota_unit_size <= OTA_UNIT_BLE_SIZE) {
			*(unsigned int*)param = svc_ctx->ota_unit_size * OTA_IOS_BLE_MAX_PKT;
		}
		break;
	case OTA_BACKEND_IOCTL_GET_CONNECT_TYPE:
		*(unsigned int*)param = svc_ctx->connect_type;
		break;
	case OTA_BACKEND_IOCTL_EXECUTE_EXIT: 
		backend->cb(backend, OTA_BACKEND_UPGRADE_EXIT, 0);
		break;

	default:
		SYS_LOG_ERR("unknow cmd 0x%x", cmd);
		return -EINVAL;
	}

	return 0;
}

int ota_backend_bt_read(struct ota_backend *backend, int offset, uint8_t *buf, int size)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err, retry_times = 0;
	uint32_t processed_cmd;

	SYS_LOG_INF("offset 0x%x, size %d, buf %p", offset, size, buf);

try_again:
	err = ota_cmd_require_image_data(svc_ctx, offset, size, buf);
	if (err) {
		SYS_LOG_INF("read data err %d", err);
		return -EIO;
	}

	while (svc_ctx->read_done_len != size) {
		err = process_command(svc_ctx, &processed_cmd);
		if (err) {
			SYS_LOG_INF("retrun err %d", err);
			break;
		}
		if(processed_cmd == OTA_CMD_H2D_NEGOTIATION_RESULT){
			offset += svc_ctx->read_done_len;
			buf += svc_ctx->read_done_len;
			size -= svc_ctx->read_done_len;

			goto try_again;
		}

		SYS_LOG_DBG("read_done_len size %d", svc_ctx->read_done_len);
	}

	if (err && (retry_times < 1) && 
		(NONE_CONNECT_TYPE != svc_ctx->connect_type)) {
		/* wait 500ms and drop all data in stream buffer */
		svc_drop_all_rx_data(svc_ctx, 500);

		svc_ctx->state = PROT_STATE_IDLE;
		retry_times++;

		SYS_LOG_INF("re-read offset 0x%x, size %d, buf %p", offset, size, buf);
		goto try_again;
	}

	return err;
}

int ota_backend_bt_read_prepare(struct ota_backend *backend, int offset, uint8_t *buf, int size)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err;

	SYS_LOG_INF("offset 0x%x, size %d, buf %p", offset, size, buf);

	err = ota_cmd_require_image_data(svc_ctx, offset, size, buf);
	if (err) {
		SYS_LOG_INF("read data err %d", err);
		return -EIO;
	}

	return err;
}

int ota_backend_bt_read_complete(struct ota_backend *backend, int offset, uint8_t *buf, int size)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err = 0, retry_times = 0, wait_ms, retry_size = size;
	uint32_t processed_cmd;

	SYS_LOG_INF("offset 0x%x, size %d, buf %p", offset, size, buf);

try_again:
	if (retry_times > 0) {
		err = ota_cmd_require_image_data(svc_ctx, offset, retry_size, buf);
		if (err) {
			SYS_LOG_INF("read data err %d", err);
			return -EIO;
		}
	}

	while (svc_ctx->read_buf_len != size) {
		err = process_command(svc_ctx, &processed_cmd);
		if (err) {
			SYS_LOG_INF("retrun err %d", err);
			break;
		}
		if(processed_cmd == OTA_CMD_H2D_NEGOTIATION_RESULT){
			offset += svc_ctx->read_buf_len;
			buf += svc_ctx->read_buf_len;
			size -= svc_ctx->read_buf_len;

			goto try_again;
		}

		SYS_LOG_DBG("read_buf_len size %d", svc_ctx->read_buf_len);
	}

	if (err && (retry_times < 1) && 
		(NONE_CONNECT_TYPE != svc_ctx->connect_type)) {
		/* wait and drop all data in stream buffer */
		wait_ms = svc_ctx->read_len / 50 + 500;
		svc_drop_all_rx_data(svc_ctx, wait_ms);

		svc_ctx->state = PROT_STATE_IDLE;
		retry_times++;
		retry_size = svc_ctx->read_len - (offset - svc_ctx->read_offset);

		SYS_LOG_INF("re-read offset 0x%x, size %d, buf %p", offset, retry_size, buf);
		goto try_again;
	}

	svc_ctx->read_buf_len = 0;

	return err;
}

int ota_backend_bt_open(struct ota_backend *backend)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err;

	if (svc_ctx->sppble_stream) {
		err = stream_open(svc_ctx->sppble_stream, MODE_IN_OUT);
		if (err) {
			SYS_LOG_ERR("stream_open Failed");
			return err;
		} else {
			svc_ctx->sppble_stream_opened = 1;
		}
	}

	SYS_LOG_INF("open sppble_stream %p", svc_ctx->sppble_stream);

	wait_negotiation_done(svc_ctx);

	return 0;
}

int ota_backend_bt_close(struct ota_backend *backend)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;
	int err;

	SYS_LOG_INF("close: type %d", backend->type);

	if (svc_ctx->sppble_stream_opened) {
		err = stream_close(svc_ctx->sppble_stream);
		if (err) {
			SYS_LOG_ERR("stream_close Failed");
		} else {
			svc_ctx->sppble_stream_opened = 0;
		}

		/* clear internal status */
		svc_ctx->negotiation_done = 0;
		svc_ctx->state = PROT_STATE_IDLE;
	}

	return 0;
}

void ota_backend_bt_exit(struct ota_backend *backend)
{
	struct ota_backend_bt *backend_bt = CONTAINER_OF(backend,
		struct ota_backend_bt, backend);
	struct svc_prot_context *svc_ctx = &backend_bt->svc_ctx;

	/* avoid connect again after exit */
	g_backend_bt = NULL;

	if (svc_ctx->sppble_stream) {
		stream_destroy(svc_ctx->sppble_stream);
	}

	if (backend_bt->svc_ctx.send_buf)
		mem_free(backend_bt->svc_ctx.send_buf);

	mem_free(backend_bt);
}

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
#define BLE_OTA_CHECK_PARAM_INTERVAL	2000		/* 2s */

enum {
	BLE_PHONE_TYPE_ANDROID,
	BLE_PHONE_TYPE_IOS,
    BLE_PHONE_TYPE_MAX,
};

enum {
	BLE_OTA_LEVEL_FAST,
    BLE_OTA_LEVEL_BUSY,
	BLE_OTA_LEVEL_IDLE,
    BLE_OTA_LEVEL_MAX,
};

const struct bt_le_conn_param le_ota_param[BLE_PHONE_TYPE_MAX][BLE_OTA_LEVEL_MAX] = {
    {
        {6,12,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
    {
        {12,28,0,600},
        {36,72,0,600},
        {32,48,4,600},
    },
};

void ble_ota_param_set_wakelock(bool set)
{
	static uint8_t param_wake_lock = 0;

	if (set) {
		if (!param_wake_lock) {
			param_wake_lock = 1;
			ble_ota_wake_lock();
			SYS_LOG_INF("Le param lock");
		}
	} else {
		if (param_wake_lock) {
			param_wake_lock = 0;
			ble_ota_wake_unlock();
			SYS_LOG_INF("Le param unlock");
		}
	}
}

/* Be careful: If have multi ble module need operate update ble parameter,  need only operate in one place. */
static void ble_ota_check_param_delaywork(os_work *work)
{
	int phone_type, br_busy;
	uint16_t curr_int = 0, curr_lat = 0;
	uint16_t need_min_int, need_max_int;
	uint8_t need_ota_leve = BLE_OTA_LEVEL_IDLE;
	SYS_LOG_INF("");
#ifdef CONFIG_GATT_OVER_BREDR
	if (!bt_manager_ble_is_connected()) {
		return;
	}
#endif

	phone_type = bt_manager_ble_get_phone_type();
	br_busy = bt_manager_ble_is_br_busy();
	if (bt_manager_ble_get_param(&curr_int, &curr_lat, NULL)) {
		goto next_work;
	}

	if (curr_int == 0) {
		goto next_work;
	}

	need_ota_leve = br_busy ? BLE_OTA_LEVEL_BUSY : BLE_OTA_LEVEL_FAST;

	curr_int += curr_int*curr_lat;

	if (phone_type >= BLE_PHONE_TYPE_MAX){
		phone_type = BLE_PHONE_TYPE_ANDROID;
	}

	need_min_int = le_ota_param[phone_type][need_ota_leve].interval_min * (1 + le_ota_param[phone_type][need_ota_leve].latency);
	need_max_int = le_ota_param[phone_type][need_ota_leve].interval_max * (1 + le_ota_param[phone_type][need_ota_leve].latency);
	SYS_LOG_INF("curr_int %d %d %d", curr_int, need_max_int, need_min_int);

	if ((curr_int < need_min_int) || (curr_int > need_max_int)) {
		/* Be careful: If failed to update several times, refer to stop update again */
		SYS_LOG_INF("App set param %d %d", phone_type, need_ota_leve);
		bt_manager_ble_update_param(&le_ota_param[phone_type][need_ota_leve]);
		goto next_work;
	} else {
		if (need_ota_leve != BLE_OTA_LEVEL_IDLE) {
			goto next_work;
		} else {
			/* In BLE_OTA_LEVEL_IDLE and not need update parameter,
			 * stop delaywork, clear wakelock.
			 */
			SYS_LOG_INF("ble update exit");
			ble_ota_param_set_wakelock(false);
			return;
		}
	}

next_work:
	os_delayed_work_submit(&ble_ota_uparam_work, BLE_OTA_CHECK_PARAM_INTERVAL);
}
#endif
#endif

static void sppble_connect_cb(int connected, uint8_t connect_type)
{
	struct ota_backend *backend;
	uint16_t mtu;

	SYS_LOG_INF("connect: %d connect_type %d.", connected, connect_type);

	/* avoid connect again after exit */
	if (g_backend_bt) {
		backend = &g_backend_bt->backend;
		if (backend->cb) {
			SYS_LOG_INF("call cb %p", backend->cb);
			if (connected && (SPP_CONNECT_TYPE == connect_type)) {
				g_backend_bt->svc_ctx.ota_unit_size = OTA_UNIT_FRCOMM_SIZE;
			} else if (connected && (BLE_CONNECT_TYPE == connect_type)) {
				mtu = bt_manager_get_ble_mtu();
				if ((mtu > OTA_UNIT_BLE_SIZE) || (mtu < 100)) {
					g_backend_bt->svc_ctx.ota_unit_size = OTA_UNIT_BLE_SIZE;
				} else {
					g_backend_bt->svc_ctx.ota_unit_size = mtu - 15;
				}
#ifdef CONFIG_OTA_GATT_OVER_EDR_TEST
				#define OTA_UNIT_EDR_SIZE		(323)
				if (!bt_manager_ble_is_connected()) {
					g_backend_bt->svc_ctx.ota_unit_size = OTA_UNIT_EDR_SIZE;
				}
#endif
			} else {
				g_backend_bt->svc_ctx.ota_unit_size = OTA_UNIT_SIZE;
			}

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
			if (BLE_CONNECT_TYPE == connect_type) {
				if (connected) {
					os_delayed_work_submit(&ble_ota_uparam_work, BLE_OTA_CHECK_PARAM_INTERVAL);
					ble_ota_param_set_wakelock(true);
				} else {
					os_delayed_work_cancel(&ble_ota_uparam_work);
					ble_ota_param_set_wakelock(false);

				}
			}
#endif
#endif
			if (!connected) {
				g_backend_bt->svc_ctx.connect_type = NONE_CONNECT_TYPE;
			} else {
				g_backend_bt->svc_ctx.connect_type = connect_type;
			}

			backend->cb(backend, OTA_BACKEND_UPGRADE_STATE, connected);
		}
	}
}

static struct ota_backend_api ota_backend_api_bt = {
	/* .init = ota_backend_bt_init, */
	.exit = ota_backend_bt_exit,
	.open = ota_backend_bt_open,
	.close = ota_backend_bt_close,
	.read = ota_backend_bt_read,
	.ioctl = ota_backend_bt_ioctl,
	.read_prepare = ota_backend_bt_read_prepare,
	.read_complete = ota_backend_bt_read_complete,
};

struct ota_backend *ota_backend_bt_init(ota_backend_notify_cb_t cb,
					struct ota_backend_bt_init_param *param)
{
	struct ota_backend_bt *backend_bt;
	struct svc_prot_context *svc_ctx;
	struct sppble_stream_init_param init_param;

	SYS_LOG_INF("init");

	backend_bt = mem_malloc(sizeof(struct ota_backend_bt));
	if (!backend_bt) {
		SYS_LOG_ERR("malloc failed");
		return NULL;
	}

	memset(backend_bt, 0x0, sizeof(struct ota_backend_bt));
	svc_ctx = &backend_bt->svc_ctx;

	memset(&init_param, 0, sizeof(struct sppble_stream_init_param));
	init_param.spp_uuid = (uint8_t *)param->spp_uuid;
	init_param.gatt_attr = param->gatt_attr;
	init_param.attr_size = param->attr_size;
	init_param.tx_chrc_attr = param->tx_chrc_attr;
	init_param.tx_attr = param->tx_attr;
	init_param.tx_ccc_attr = param->tx_ccc_attr;
	init_param.rx_attr = param->rx_attr;
	init_param.connect_cb = sppble_connect_cb;
	init_param.read_timeout = param->read_timeout;	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.write_timeout = param->write_timeout;
	init_param.read_buf_size = OTA_SPPBLE_BUFF_SIZE;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	svc_ctx->sppble_stream = sppble_stream_create(&init_param);
	if (!svc_ctx->sppble_stream) {
		SYS_LOG_ERR("stream_create failed");
	}

	svc_ctx->send_buf = mem_malloc(OTA_SVC_SEND_BUFFER_SIZE);
	svc_ctx->send_buf_size = OTA_SVC_SEND_BUFFER_SIZE;
	svc_ctx->ota_unit_size = OTA_UNIT_SIZE;
	svc_ctx->state = PROT_STATE_IDLE;

	g_backend_bt = backend_bt;

	ota_backend_init(&backend_bt->backend, OTA_BACKEND_TYPE_BLUETOOTH,
			 &ota_backend_api_bt, cb);

#ifdef CONFIG_BT_BLE
#ifdef CONFIG_BT_BLE_APP_UPDATE_PARAM
	os_delayed_work_init(&ble_ota_uparam_work, ble_ota_check_param_delaywork);
#endif
#endif

	return &backend_bt->backend;
}

struct ota_backend *ota_backend_load_bt_init(ota_backend_notify_cb_t cb,
					struct ota_backend_bt_init_param *param, stream_cb scb, io_stream_t *pexist_stream)
{
	struct ota_backend_bt *backend_bt;
	struct svc_prot_context *svc_ctx;
	struct sppble_stream_init_param init_param;

	SYS_LOG_INF("init");
	if (!g_backend_bt)
	{
		backend_bt = mem_malloc(sizeof(struct ota_backend_bt));
		if (!backend_bt) {
			SYS_LOG_ERR("malloc failed");
			return NULL;
		}
	}
	else
	{
		backend_bt = g_backend_bt;
	}

	memset(backend_bt, 0x0, sizeof(struct ota_backend_bt));
	svc_ctx = &backend_bt->svc_ctx;

	memset(&init_param, 0, sizeof(struct sppble_stream_init_param));
	init_param.spp_uuid = (uint8_t *)param->spp_uuid;
	init_param.gatt_attr = param->gatt_attr;
	init_param.attr_size = param->attr_size;
	init_param.tx_chrc_attr = param->tx_chrc_attr;
	init_param.tx_attr = param->tx_attr;
	init_param.tx_ccc_attr = param->tx_ccc_attr;
	init_param.rx_attr = param->rx_attr;
	init_param.connect_cb = sppble_connect_cb;
	init_param.read_timeout = param->read_timeout;	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	init_param.write_timeout = param->write_timeout;
	init_param.read_buf_size = OTA_SPPBLE_BUFF_SIZE;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	svc_ctx->sppble_stream = scb(&init_param);
	if (!svc_ctx->sppble_stream) {
		SYS_LOG_ERR("stream_create failed");
	}
	if (!svc_ctx->send_buf)
		svc_ctx->send_buf = mem_malloc(OTA_SVC_SEND_BUFFER_SIZE);
	svc_ctx->send_buf_size = OTA_SVC_SEND_BUFFER_SIZE;
	svc_ctx->ota_unit_size = OTA_UNIT_SIZE;
	svc_ctx->state = PROT_STATE_IDLE;

	g_backend_bt = backend_bt;

	ota_backend_init(&backend_bt->backend, OTA_BACKEND_TYPE_BLUETOOTH,
			 &ota_backend_api_bt, cb);
	
	*pexist_stream = svc_ctx->sppble_stream;

	return &backend_bt->backend;
}

void ota_backend_stream_set(io_stream_t exist_stream)
{
	struct ota_backend_bt *backend_bt;
	struct svc_prot_context *svc_ctx;

	if (!g_backend_bt)
	{
		SYS_LOG_ERR("");
		return;
	}
	
	backend_bt = g_backend_bt;
	svc_ctx = &backend_bt->svc_ctx;
	svc_ctx->sppble_stream = exist_stream;

	return;
}

void ota_backend_ota_type_cb_set(ota_backend_type_cb_t tcb)
{
	struct ota_backend_bt *backend_bt;
	struct svc_prot_context *svc_ctx;

	if (!g_backend_bt)
	{
		SYS_LOG_ERR("");
		return;
	}
	
	backend_bt = g_backend_bt;
	svc_ctx = &backend_bt->svc_ctx;
	svc_ctx->ota_type_cb = tcb;

	return;
}


