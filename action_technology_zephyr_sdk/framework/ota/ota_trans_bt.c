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
#include <ota_trans_bt.h>
#include <os_common_api.h>
#include <bt_manager.h>
#include <ota_api.h>

#define PROT_OTA_FACTORY_OFFLINE (1)

struct cli_prot_head {
	u8_t svc_id;
	u8_t cmd;
	u8_t param_type;
	u16_t param_len;
} __attribute__((packed));

struct cli_tlv_head {
	u8_t type;
	u16_t len;
	u8_t value[0];
} __attribute__((packed));

struct cli_tlv_data {
	u8_t type;
	u16_t len;
	u16_t max_len;
	u8_t *value_ptr;
};

typedef int (*cli_prot_cmd_handler_t)(struct cli_prot_context *ctx, u16_t param_len);
struct cli_prot_cmd {
	u8_t cmd;
	cli_prot_cmd_handler_t handler;
};
#if 1
int ota_cli_send_cmd(struct cli_prot_context *ctx, u8_t cmd,u8_t *buf, int size);
int cli_tlv_unpack_head(struct cli_prot_context *ctx, struct cli_tlv_data *tlv);
int cli_prot_get_rx_data(struct cli_prot_context *ctx, u8_t *buf, int size);
int cli_prot_skip_rx_data(struct cli_prot_context *ctx, int size);
int cli_drop_all_rx_data(struct cli_prot_context *ctx, int wait_ms);
u8_t *cli_tlv_pack_data(u8_t *buf, u8_t type, u16_t len, u32_t number);
int cli_tlv_unpack_data(struct cli_prot_context *ctx, u8_t type, int *len,
			   u8_t *value_ptr, int max_len);
			   
#define TLV_PACK_U8(buf, type, value)	cli_tlv_pack_data(buf, type, sizeof(u8_t), value)
#define TLV_PACK_U16(buf, type, value)	cli_tlv_pack_data(buf, type, sizeof(u16_t), value)
#define TLV_PACK_U32(buf, type, value)	cli_tlv_pack_data(buf, type, sizeof(u32_t), value)

#define TLV_UNPACK_U8(ctx, type, value_ptr)	\
		cli_tlv_unpack_data(ctx, type, NULL, (u8_t *)value_ptr, sizeof(u8_t))
#define TLV_UNPACK_U16(ctx, type, value_ptr)	\
		cli_tlv_unpack_data(ctx, type, NULL, (u8_t *)value_ptr, sizeof(u16_t))
#define TLV_UNPACK_U32(ctx, type, value_ptr)	\
		cli_tlv_unpack_data(ctx, type, NULL, (u8_t *)value_ptr, sizeof(u32_t))
#endif

#define print_hex1(str, buf, size)	do {printk("%s\n", str); print_buffer(buf, 1, size, 16, 0);} while(0)

/* support check data checksum in each ota unit */

/* for sppble_stream callback */
static struct ota_trans_bt *g_trans_bt;

int cli_prot_get_rx_data(struct cli_prot_context *ctx, uint8_t *buf, int size)
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

int cli_prot_skip_rx_data(struct cli_prot_context *ctx, int size)
{
	int err;
	uint8_t c;

	while (size > 0) {
		err = cli_prot_get_rx_data(ctx, &c, sizeof(uint8_t));
		if (err) {
			SYS_LOG_ERR("failed to get data");
			return err;
		}

		size--;
	}

	return 0;
}

int cli_drop_all_rx_data(struct cli_prot_context *ctx, int wait_ms)
{
	int err, data_len;
	int ms = wait_ms;

	while (wait_ms > 0) {
		data_len = stream_tell(ctx->sppble_stream);
		if (data_len > 0) {
			SYS_LOG_INF("drop data len 0x%x", data_len);

			err = cli_prot_skip_rx_data(ctx, data_len);
			if (err)
				return err;

			wait_ms = ms;
		}

		os_sleep(20);
		wait_ms -= 20;
	}

	return 0;
}

uint8_t *cli_tlv_pack(uint8_t *buf, const struct cli_tlv_data *tlv)
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

uint8_t *cli_tlv_pack_data(uint8_t *buf, uint8_t type, uint16_t len, uint32_t number)
{
	struct cli_tlv_data tlv;

	tlv.type = type;
	tlv.len = len;
	tlv.value_ptr = (uint8_t *)&number;

	return cli_tlv_pack(buf, &tlv);
}

int cli_tlv_unpack_head(struct cli_prot_context *ctx, struct cli_tlv_data *tlv)
{
	int err, total_len;

	/* read type */
	err = cli_prot_get_rx_data(ctx, &tlv->type, sizeof(tlv->type));
	if (err) {
		SYS_LOG_ERR("failed to read tlv type");
		return err;
	}

	/* read length */
	err = cli_prot_get_rx_data(ctx, (uint8_t *)&tlv->len, sizeof(tlv->len));
	if (err) {
		SYS_LOG_ERR("failed to read tlv type");
		return err;
	}

	total_len = sizeof(tlv->type) + sizeof(tlv->len);

	return total_len;
}

int cli_tlv_unpack(struct cli_prot_context *ctx, struct cli_tlv_data *tlv)
{
	uint16_t data_len;
	int err, total_len;

	total_len = cli_tlv_unpack_head(ctx, tlv);
	if (tlv->len <= 0)
		return total_len;

	data_len = tlv->len;
	if (data_len > tlv->max_len) {
		data_len = tlv->max_len;
	}

	/* read value */
	if (tlv->value_ptr) {
		err = cli_prot_get_rx_data(ctx, tlv->value_ptr, data_len);
		if (err) {
			SYS_LOG_ERR("failed to read tlv value");
			return err;
		}

		total_len += data_len;
	}

	return total_len;
}

int cli_tlv_unpack_data(struct cli_prot_context *ctx, uint8_t type, int *len,
			   uint8_t *value_ptr, int max_len)
{
	struct cli_tlv_data tlv;
	int rlen;

	tlv.value_ptr = value_ptr;
	tlv.max_len = max_len;

	rlen = cli_tlv_unpack(ctx, &tlv);
	if (rlen < 0) {
		SYS_LOG_ERR("cli_tlv_unpack failed, err 0x%x", rlen);
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

int cli_prot_send_data(struct cli_prot_context *ctx, uint8_t *buf, int size)
{
	int err;

	err = stream_write(ctx->sppble_stream, buf, size);
	if (err < 0) {
		SYS_LOG_ERR("failed to send data, size %d, err %d", size, err);
		return -EIO;
	}

	return 0;
}

int ota_cli_send_cmd(struct cli_prot_context *ctx, uint8_t cmd,
			uint8_t *buf, int size)
{
	struct cli_prot_head *head;
	int err;

	head = (struct cli_prot_head *)buf;
	head->svc_id = SERVICE_ID_OTA;
	head->cmd = cmd;
	head->param_type = TLV_TYPE_MAIN;
	head->param_len = size - sizeof(struct cli_prot_head);

	SYS_LOG_INF("send cmd: %d", cmd);
	//print_buffer(buf, 1, size, 16, buf);

	err = cli_prot_send_data(ctx, buf, size);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, err %d", cmd, err);
		return err;
	}

	return 0;
}



int ota_ioctl_request_upgrade_ack(struct cli_prot_context *ctx, u16_t param_size)
{
	int err, head_len;

	struct cli_tlv_data tlv;
	u8_t host_features, device_features;
	struct ota_trans *trans = &g_trans_bt->trans;
		
	SYS_LOG_INF("upgrade request: param_size %d", param_size);

	ctx->host_features = 0;
	device_features = 0;
	while (param_size > 0) {
		head_len = cli_tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_SUPPORT_FEATURES:
			err = cli_prot_get_rx_data(ctx, &host_features, 1);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}

			ctx->host_features = host_features;
			SYS_LOG_INF("host support features: 0x%x", host_features);
			break;
					
		default:
			/* skip other parameters by now */
			err = cli_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;
		}

		param_size -= head_len + tlv.len;
	}

	ctx->state = PROT_STATE_IDLE;

	if (trans->cb) {
		trans->cb(trans,OTA_TRANS_REQUEST_UPGRADE_ACK,NULL);
	}
	return 0;
}

int ota_ioctl_connect_negotiation_ack(struct cli_prot_context *ctx, u16_t param_len)
{
	u16_t app_wait_timeout, device_restart_timeout, ota_unit_size, interval;
	int head_len;
	int err;
	struct cli_tlv_data tlv;
	struct ota_trans *trans = &g_trans_bt->trans;

	SYS_LOG_INF("param_len %d\n", param_len);


	while (param_len > 0) {
		head_len = cli_tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_WAIT_TIMEOUT:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&app_wait_timeout, 2);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
			ctx->app_wait_timeout = app_wait_timeout;
//			SYS_LOG_INF("host wait_timeout: 0x%x", app_wait_timeout);
			break;

		case TLV_TYPE_OTA_RESTART_TIMEOUT:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&device_restart_timeout, 2);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
			ctx->device_restart_timeout = device_restart_timeout;
//			SYS_LOG_INF("device_restart_timeout: 0x%x", device_restart_timeout);
			break;

		case TLV_TYPE_OTA_UINT_SIZE:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&ota_unit_size, 2);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
			ctx->ota_unit_size = ota_unit_size;
			
			SYS_LOG_INF("ota_unit_size: 0x%x", ota_unit_size);
			break;

		case TLV_TYPE_OTA_INTERVAL:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&interval, 2);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
			ctx->ota_interval = interval;
			
//			SYS_LOG_INF("interval: 0x%x", interval);
			break;

		default:
			/* skip other parameters by now */
			err = cli_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;
		}

		param_len -= head_len + tlv.len;
	}

	ctx->state = PROT_STATE_IDLE;

	if (trans->cb) {
		trans->cb(trans,OTA_TRANS_CONNECT_NEGOTIATION_ACK,NULL);
	}
	return err;
}

int ota_ioctl_require_image_data(struct cli_prot_context *ctx, u16_t param_size)
{
	u32_t ota_file_offset, ota_file_len;
	u8_t ota_apply_bitmap;
	int head_len;
	int err;
	struct cli_tlv_data tlv;
	struct ota_trans *trans = &g_trans_bt->trans;
	struct ota_trans_image_info image;
	
//	  SYS_LOG_INF("param_len %d\n", param_size);


	while (param_size > 0) {
		head_len = cli_tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_FILE_OFFSET:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&ota_file_offset, 4);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}

	//		SYS_LOG_INF("ota_file_offset: 0x%x", ota_file_offset);
			break;
		case TLV_TYPE_OTA_FILE_LEN:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&ota_file_len, 4);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
	//		SYS_LOG_INF("ota_file_len: 0x%x", ota_file_len);
			break;

		case TLV_TYPE_OTA_APPLY_BITMAP:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&ota_apply_bitmap, 1);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}
	//		SYS_LOG_INF("ota_apply_bitmap: 0x%x", ota_apply_bitmap);
			break;

		default:
			/* skip other parameters by now */
			err = cli_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;
		}

		param_size -= head_len + tlv.len;
	}

	ctx->state = PROT_STATE_IDLE;

	
	if (trans->cb) {
		SYS_LOG_INF("ota_file_offset 0x%x ota_file_len 0x%x\n", ota_file_offset,ota_file_len);
		image.image_offset = ota_file_offset;
		image.read_len = ota_file_len;
		trans->cb(trans,OTA_TRANS_REQUEST_IMAGE_DATA,&image);
	}
	return 0;
}

int ota_ioctl_validate_image_report(struct cli_prot_context *ctx,u16_t param_size)
{
	int err, send_len;
	u8_t *send_buf;
	int head_len;
	u8_t valid_report;
	struct cli_tlv_data tlv;
	struct ota_trans *trans = &g_trans_bt->trans;

	SYS_LOG_INF("%x",(uint32)ctx);

	while (param_size > 0) {
		head_len = cli_tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_VALID_REPORT:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&valid_report, 1);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}

			SYS_LOG_INF("valid_report: 0x%x", valid_report);
			break;

		default:
			/* skip other parameters by now */
			err = cli_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;
		}

		param_size -= head_len + tlv.len;
	}


	send_buf = ctx->send_buf + sizeof(struct cli_prot_head);

	send_len = send_buf - ctx->send_buf;

	err = ota_cli_send_cmd(ctx, OTA_CMD_D2H_VALIDATE_IMAGE, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_D2H_VALIDATE_IMAGE, err);
		return err;
	}

	ctx->state = PROT_STATE_IDLE;

	if (trans->cb) {
		trans->cb(trans,OTA_TRANS_VALIDATE_REPORT,&valid_report);
	}
	return 0;
}

int ota_ioctl_remote_report_satus(struct cli_prot_context *ctx,u16_t param_size)
{
	int err;
	u32_t status_code ;
	int head_len;
	
	struct cli_tlv_data tlv;
	struct ota_trans *trans = &g_trans_bt->trans;

	SYS_LOG_INF("%x",(uint32)ctx);
	
	while (param_size > 0) {	
		head_len = cli_tlv_unpack_head(ctx, &tlv);
		if (head_len <= 0)
			return -EIO;

		switch (tlv.type) {
		case TLV_TYPE_OTA_STATUS_CODE:
			err = cli_prot_get_rx_data(ctx, (u8_t *)&status_code, 4);
			if (err) {
				SYS_LOG_ERR("failed to read tlv value");
				return err;
			}

			SYS_LOG_INF("status_code: 0x%x", status_code);
			break;
		
		default:
			/* skip other parameters by now */
			err = cli_prot_skip_rx_data(ctx, tlv.len);
			if (err)
				return -EIO;
			break;

		}
		param_size -= head_len + tlv.len;
	}

	ctx->state = PROT_STATE_IDLE;

	if (trans->cb) {
		trans->cb(trans,OTA_TRANS_UPGRADE_STATUS_NOTIFY,&status_code);
	}	
	return 0;
}

struct cli_prot_cmd ota_cli_cmds[] = {
	{OTA_CMD_H2D_REQUEST_UPGRADE, ota_ioctl_request_upgrade_ack,},
	{OTA_CMD_H2D_CONNECT_NEGOTIATION, ota_ioctl_connect_negotiation_ack,},
	{OTA_CMD_D2H_REQUIRE_IMAGE_DATA,  ota_ioctl_require_image_data,},
	{OTA_CMD_D2H_VALIDATE_IMAGE, ota_ioctl_validate_image_report,},
	{OTA_CMD_D2H_REPORT_STATUS,ota_ioctl_remote_report_satus,}
};

int ota_trans_process_command(struct cli_prot_context *ctx, u32_t *processed_cmd)
{
	struct cli_prot_head head;
	cli_prot_cmd_handler_t cmd_handler;
	int i, err;

	if (ctx->state != PROT_STATE_IDLE) {
		SYS_LOG_ERR("current state is not idle");
		return -EIO;
	}

	err = cli_prot_get_rx_data(ctx, (u8_t *)&head, sizeof(struct cli_prot_head));
	if (err) {
		SYS_LOG_ERR("cannot read head bytes");
		return -EIO;
	}

	SYS_LOG_INF("cli head: svc_id 0x%x, cmd_id 0x%x, param type 0x%x, len 0x%x",
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

	for (i = 0; i < ARRAY_SIZE(ota_cli_cmds); i++) {
		if (ota_cli_cmds[i].cmd == head.cmd) {
			cmd_handler = ota_cli_cmds[i].handler;
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

	if (i > ARRAY_SIZE(ota_cli_cmds)) {
		SYS_LOG_ERR("invalid cmd: %d", head.cmd);
		return -1;
	}

	ctx->state = PROT_STATE_IDLE;

	return err;
}

int ota_ioctl_request_upgrade(struct cli_prot_context *ctx,struct ota_trans_image_head_info * head_info)
{
	int err, send_len;
	u8_t *send_buf;
	u32_t version = head_info->version;
	u32_t head_crc = head_info->head_crc;
	u8_t temp = 0;
	u32_t feature = 1;
	u32_t ota_type = PROT_OTA_FACTORY_OFFLINE;

	SYS_LOG_INF("%x %x %x",(uint32)ctx,version,head_crc);

	ctx->host_features = 1;

	send_buf = ctx->send_buf + sizeof(struct cli_prot_head);
	send_buf = TLV_PACK_U32(send_buf,0x01,version);
	send_buf = TLV_PACK_U32(send_buf,0x03,head_crc);
	send_buf = TLV_PACK_U8(send_buf, 0x04, temp);
	send_buf = TLV_PACK_U8(send_buf, 0x09, feature);
	send_buf = TLV_PACK_U32(send_buf, 0x0A, ota_type);

	send_len = send_buf - ctx->send_buf;

	err = ota_cli_send_cmd(ctx, OTA_CMD_H2D_REQUEST_UPGRADE, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_H2D_REQUEST_UPGRADE, err);
		return err;
	}

	ctx->state = PROT_STATE_IDLE;
	
	return 0;
}

int ota_ioctl_connect_negotiation(struct cli_prot_context *ctx)
{
	int err, send_len;	
	SYS_LOG_INF("%x",(uint32)ctx);

	send_len = sizeof(struct cli_prot_head);

	err = ota_cli_send_cmd(ctx, OTA_CMD_H2D_CONNECT_NEGOTIATION, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_H2D_CONNECT_NEGOTIATION, err);
		return err;
	}

	ctx->state = PROT_STATE_IDLE;
	
	return 0;
}

int ota_ioctl_negotiation_result(struct cli_prot_context *ctx)
{
	int err, send_len;	
	u8_t *send_buf;
	u8_t negotiation_result = 1;
	struct ota_trans *trans = &g_trans_bt->trans;
	
	SYS_LOG_INF("%x",(uint32)ctx);
	
	send_buf = ctx->send_buf + sizeof(struct cli_prot_head);

	send_buf = TLV_PACK_U8(send_buf, 0x01, negotiation_result);
	send_len = send_buf - ctx->send_buf;

	err = ota_cli_send_cmd(ctx, OTA_CMD_H2D_NEGOTIATION_RESULT, ctx->send_buf, send_len);
	if (err) {
		SYS_LOG_ERR("failed to send cmd %d, error %d",
			OTA_CMD_H2D_NEGOTIATION_RESULT, err);
		return err;
	}

	ctx->state = PROT_STATE_IDLE;

	ctx->negotiation_done = 1;

	if (trans->cb) {
		trans->cb(trans,OTA_TRANS_NEGOTIATION_RESULT_ACK,NULL);
	}	
	
	return 0;
}

int ota_ioctl_send_image_data(struct cli_prot_context *ctx,struct ota_trans_upgrade_image * image_info)
{
	int err, send_len;	
	u8_t *send_buf;
	int image_size = image_info->size;
	u8_t *image_buffer = image_info->buffer;
	u16_t unit_size = ctx->ota_unit_size;
	u8_t * crc_pos;
	u32_t crc;
	SYS_LOG_INF("%x %d %x",(uint32)ctx,ctx->ota_unit_size,image_size);

	if (0 == image_info->ctn) {
		ctx->last_psn = 0;
	}

	while(image_size)
	{
		send_buf = ctx->send_buf + sizeof(struct cli_prot_head);

		*(send_buf++) = ctx->last_psn;
		crc_pos = send_buf;
		send_buf += sizeof(u32_t);

		if(image_size > unit_size)
		{
			memcpy(send_buf,image_buffer,unit_size);
			crc = utils_crc32(0,send_buf, unit_size);
			image_size -= unit_size;
			image_buffer += unit_size;
			send_buf += unit_size;
		}
		else
		{
			memcpy(send_buf,image_buffer,image_size);
			crc = utils_crc32(0,send_buf, image_size);
			send_buf += image_size; 	
			image_size = 0; 	   
		}

		*(crc_pos++) = (u8_t)(crc & 0xff);
		*(crc_pos++) = (u8_t)((crc >> 8) & 0xff);
		*(crc_pos++) = (u8_t)((crc >> 16) & 0xff);
		*(crc_pos++) = (u8_t)((crc >> 24) & 0xff);	 
		send_len = send_buf - ctx->send_buf;

		SYS_LOG_INF("%d:",send_len);
//		print_hex1("image",ctx->send_buf,32);

		err = ota_cli_send_cmd(ctx, OTA_CMD_H2D_SEND_IMAGE_DATA_WITH_CRC, ctx->send_buf, send_len);
		if (err) {
			SYS_LOG_ERR("failed to send cmd %d, error %d",
				OTA_CMD_H2D_CONNECT_NEGOTIATION, err);
			return err;
		}

		ctx->state = PROT_STATE_IDLE;

		
		ctx->last_psn += 1;
		//os_sleep(1);
	}
	
	return 0;
}

int ota_trans_bt_open(struct ota_trans *trans)
{
	struct ota_trans_bt *trans_bt = CONTAINER_OF(trans,
		struct ota_trans_bt, trans);
	struct cli_prot_context *cli_ctx = &trans_bt->cli_ctx;
	int err;

	if (cli_ctx->sppble_stream) {
		err = stream_open(cli_ctx->sppble_stream, MODE_IN_OUT);
		if (err) {
			SYS_LOG_ERR("stream_open Failed");
			return err;
		} else {
			cli_ctx->sppble_stream_opened = 1;
		}
	}

	SYS_LOG_INF("open sppble_stream %p", cli_ctx->sppble_stream);

	return 0;
}

int ota_trans_bt_close(struct ota_trans *trans)
{
	struct ota_trans_bt *trans_bt = CONTAINER_OF(trans,
		struct ota_trans_bt, trans);
	struct cli_prot_context *cli_ctx = &trans_bt->cli_ctx;
	int err;

	SYS_LOG_INF("close: type %d", trans->type);

	if (cli_ctx->sppble_stream_opened) {
		err = stream_close(cli_ctx->sppble_stream);
		if (err) {
			SYS_LOG_ERR("stream_close Failed");
		} else {
			cli_ctx->sppble_stream_opened = 0;
		}

		/* clear internal status */
		cli_ctx->negotiation_done = 0;
	}

	return 0;
}

void ota_trans_bt_exit(struct ota_trans *trans)
{
	struct ota_trans_bt *trans_bt = CONTAINER_OF(trans,
		struct ota_trans_bt, trans);
	struct cli_prot_context *cli_ctx = &trans_bt->cli_ctx;

	/* avoid connect again after exit */
	g_trans_bt = NULL;

	if (cli_ctx->sppble_stream) {
		stream_destroy(cli_ctx->sppble_stream);
	}

	if (trans_bt->cli_ctx.send_buf)
		mem_free(trans_bt->cli_ctx.send_buf);

	mem_free(trans_bt);
}

int ota_trans_bt_ioctl(struct ota_trans *trans, int cmd, void* param)
{
	struct ota_trans_bt *trans_bt = CONTAINER_OF(trans,
		struct ota_trans_bt, trans);
	struct cli_prot_context *cli_ctx = &trans_bt->cli_ctx;
	int err = 0;
	
	SYS_LOG_INF("cmd %d", cmd);

	switch(cmd)
	{
		case OTA_TRANS_IOCTL_REQUEST_UPGRADE:
		err = ota_ioctl_request_upgrade(cli_ctx,(struct ota_trans_image_head_info *)param);
		break;
		
		case OTA_TRANS_IOCTL_CONNECT_NEGOTIATION:
		err = ota_ioctl_connect_negotiation(cli_ctx);
		break;
		
		case OTA_TRANS_IOCTL_NEGOTIATION_RESULT:
		err = ota_ioctl_negotiation_result(cli_ctx);
		break;

		case OTA_TRANS_IOCTL_SEND_IMAGE:
		err = ota_ioctl_send_image_data(cli_ctx,(struct ota_trans_upgrade_image *)param);
		break;

		case OTA_TRANS_IOCTL_UNITSIZE_GET:
		return cli_ctx->ota_unit_size;

		default :
		break;
	}

	if (err) {
		SYS_LOG_INF("send cmd 0x%x error", cmd);
		return -EIO;
	}
	
	return 0;
}

int ota_trans_bt_read(struct ota_trans *trans, int offset, void *buf, int size)
{
	return 0;
}

static void ota_sppble_connect_cb(int connected)
{
	struct ota_trans *trans;

	SYS_LOG_INF("connect: %d", connected);

	/* avoid connect again after exit */
	if (g_trans_bt) {
		trans = &g_trans_bt->trans;
		if (trans->cb) {
			SYS_LOG_INF("call cb %p", trans->cb);
			if(connected)
			{
				trans->cb(trans, OTA_TRANS_CONNECTED,NULL);
			}
			else
			{
				trans->cb(trans, OTA_TRANS_DISCONNECT,NULL);
			}
		}
	}
}
#if 0
static void ota_sppble_sdap_cb(uint16 status)
{
	struct ota_trans *trans;
	uint16 sdp_status = status;
	SYS_LOG_INF("sdp status: %d", status);
	if (g_trans_bt) {
		trans = &g_trans_bt->trans;
		if (trans->cb) {
			trans->cb(trans, OTA_TRANS_SDAP_RESULT,&sdp_status);
		}
	}	 
}
#endif
static void ota_sppble_rxdata_cb(void)
{
	struct ota_trans *trans;
	trans = &g_trans_bt->trans;

	struct ota_trans_bt *trans_bt = CONTAINER_OF(trans,
		struct ota_trans_bt, trans);
	struct cli_prot_context *cli_ctx = &trans_bt->cli_ctx;
	
//	SYS_LOG_INF("");

	/* avoid connect again after exit */
	if (g_trans_bt) {
		ota_trans_process_command(cli_ctx,NULL);
	}
}

static struct ota_trans_api ota_trans_api_bt = {
	/* .init = ota_backend_bt_init, */
	.exit = ota_trans_bt_exit,
	.open = ota_trans_bt_open,
	.close = ota_trans_bt_close,
	.read = ota_trans_bt_read,
	.ioctl = ota_trans_bt_ioctl,
};

struct ota_trans *ota_trans_bt_init(ota_trans_notify_cb_t cb,
					struct ota_trans_bt_init_param *param)
{
	struct ota_trans_bt *trans_bt;
	struct sppble_stream_init_param init_param;
	struct cli_prot_context *cli_ctx;

	SYS_LOG_INF("init");

	trans_bt = mem_malloc(sizeof(struct ota_trans_bt));
	if (!trans_bt) {
		SYS_LOG_ERR("malloc failed");
		return NULL;
	}

	memset(trans_bt, 0x0, sizeof(struct ota_trans_bt));
	cli_ctx = &trans_bt->cli_ctx;

	memset(&init_param, 0, sizeof(struct sppble_stream_init_param));
	init_param.spp_uuid = (uint8_t *)param->spp_uuid;
	init_param.connect_cb = ota_sppble_connect_cb;
	//init_param.read_timeout = param->read_timeout;	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	//init_param.write_timeout = param->write_timeout;
	//init_param.read_buf_size = OTA_SPPBLE_BUFF_SIZE;
	init_param.rxdata_cb = ota_sppble_rxdata_cb;
	//init_param.sdap_cb = ota_sppble_sdap_cb;
	init_param.read_timeout = OS_FOREVER;
	init_param.write_timeout = OS_FOREVER;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	cli_ctx->sppble_stream = sppble_stream_create(&init_param);
	if (!cli_ctx->sppble_stream) {
		SYS_LOG_ERR("stream_create failed");
	}
	cli_ctx->send_buf = mem_malloc(OTA_SVC_TRANS_SEND_BUFFER_SIZE);
	cli_ctx->send_buf_size = OTA_SVC_TRANS_SEND_BUFFER_SIZE;
	cli_ctx->ota_unit_size = OTA_UNIT_SIZE;
	cli_ctx->state = PROT_STATE_IDLE;
	cli_ctx->negotiation_done = 0;

	g_trans_bt = trans_bt;

	ota_trans_init(&trans_bt->trans, OTA_TRANS_TYPE_BLUETOOTH,
			 &ota_trans_api_bt, cb);

	return &trans_bt->trans;
}
