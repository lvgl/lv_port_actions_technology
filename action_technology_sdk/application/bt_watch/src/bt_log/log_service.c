/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************
 *      INCLUDES
 *********************/
#define LOG_MODULE_CUSTOMER

#include <sys/byteorder.h>
#include <os_common_api.h>
#include <thread_timer.h>
#include <mem_manager.h>
#include <srv_manager.h>
#include <bt_manager.h>
#include <stream.h>
#include <sys/atomic.h>
#include <logging/act_log.h>
#ifdef CONFIG_SYS_WAKELOCK
#  include <sys_wakelock.h>
#endif
#ifdef CONFIG_SHELL_BACKEND_SERIAL
#  include <shell/shell_uart.h>
#endif
#ifdef CONFIG_SHELL_BACKEND_RTT
#  include <shell/shell_rtt.h>
#endif
#ifdef CONFIG_TASK_WDT
#include <task_wdt_manager.h>
#endif

LOG_MODULE_REGISTER(logsrv, LOG_LEVEL_INF);

/*********************
 *      DEFINES
 *********************/
#define LOG_SERVICE_NAME	"logsrv"

#define LOG_HEAD_SIZE	(SVC_HEAD_SIZE + TLV_HEAD_SIZE)
#define LOG_BUFFER_SIZE	(512u - LOG_HEAD_SIZE)
#define LOG_BUFFER_NUM	(8u) /* must be power of 2 */

#define LOG_TX_PERIOD	(10)
#define LOG_RX_PERIOD	(50)

/* SVC service ID */
#define SVC_SRV_ID		(0x1D)
/* SVC command ID */
#define SVC_CMD_ID_RX	(0x10)
#define SVC_CMD_ID_ACK	(0x10)
#define SVC_CMD_ID_TX	(0xF0)
#define SVC_CMD_ID_TX_END	(0xF1)
/* SVC param type */
#define SVC_PARAM_TYPE_FIXED	(0x81)

/* TLV console input type */
#define TLV_TYPE_CONSOLE_CODE	(0x00)
#define TLV_TYPE_CONSOLE_TEXT	(0x01)
#define TLV_CODE_PREPARE			(0x01)
#define TLV_CODE_LOG_SYSLOG_START	(0x10)
#define TLV_CODE_LOG_COREDUMP_START	(0x11)
#define TLV_CODE_LOG_RUNTIME_START	(0x12)
#define TLV_CODE_LOG_RAMDUMP_START	(0x13)
#define TLV_CODE_LOG_DEFAULT_START	(0x20)
#define TLV_CODE_LOG_STOP			(0x21)

/* TLV ack  */
#define TLV_TYPE_ACK	(0x7F)
#define TLV_ACK_OK		(100000)
#define TLV_ACK_INV		(1)

/* TLV log upload type */
#define TLV_TYPE_LOG_NONE		(0xFF)
#define TLV_TYPE_LOG_DEFAULT	(0x00)
#define TLV_TYPE_LOG_SYSLOG		(0x01)
#define TLV_TYPE_LOG_COREDUMP	(0x02)
#define TLV_TYPE_LOG_RUNTIME	(0x03)
#define TLV_TYPE_LOG_RAMDUMP	(0x04)

/**********************
 *      TYPEDEFS
 **********************/
/* Message type */
enum {
	MSG_LOGSRV_BT_CONNECT = MSG_APP_MESSAGE_START,
};

typedef struct svc_prot_head {
	uint8_t svc_id;
	uint8_t cmd;
	uint8_t param_type;
	//uint16_t param_len;
	uint8_t param_len[2];
} __attribute__((packed)) svc_prot_head_t;

#define SVC_HEAD_SIZE (sizeof(svc_prot_head_t))

typedef struct tlv_head {
	uint8_t type;
	uint8_t len[2]; /* litte-endian [0] lower 1 byte, [1] higher 1 type */
} __attribute__((packed)) tlv_head_t;

typedef struct tlv_dsc {
	uint8_t type;
	uint8_t __pad;
	uint16_t len;
	uint16_t max_len;
	uint8_t *buf;
} tlv_dsc_t;

#define TLV_HEAD_SIZE (sizeof(tlv_head_t))
#define TLV_SIZE(tlv) (TLV_HEAD_SIZE + (tlv)->len)

typedef struct logsrv_ctx {
	io_stream_t sppble_stream;
	uint8_t stream_opened : 1;
	uint8_t prepared : 1;

	/* log type */
	uint8_t log_type;
	uint8_t new_log_type;
	/* log buffer queue read index */
	uint8_t rd_idx;
	/* log buffer queue write index */
	uint8_t wr_idx;
	/* log buffer length(offset) */
	uint16_t len[LOG_BUFFER_NUM];

	atomic_t drop_cnt;

	struct thread_timer ttimer;
	os_mutex mutex;
} logsrv_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void _sppble_connect_cb(int connected, uint8_t connect_type);
static int _sppble_get_rx_data(io_stream_t stream, void *buf, int size);
static int _sppble_put_tx_data(io_stream_t stream, const void *buf, int size);

/**********************
 *  STATIC VARIABLES
 **********************/

/* spp uuid: "00001101-0000-1000-8000-00805F9B34FB" */
static const uint8_t log_spp_uuid[16] = {
	0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0x01, 0x11, 0x00, 0x00,
};

/* BLE service uuid: "e49a3001-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define LOG_SERVICE_UUID BT_UUID_DECLARE_128( \
			0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
			0xe8, 0x11, 0x9a, 0xf6, 0x01, 0x30, 0x9a, 0xe4)

/* BLE service rx uuid: "e49a3002-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define LOG_CHA_RX_UUID BT_UUID_DECLARE_128( \
			0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
			0xe8, 0x11, 0x9a, 0xf6, 0x02, 0x30, 0x9a, 0xe4)

/* BLE service tx uuid: "e49a3003-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define LOG_CHA_TX_UUID BT_UUID_DECLARE_128( \
			0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
			0xe8, 0x11, 0x9a, 0xf6, 0x03, 0x30, 0x9a, 0xe4)

static struct bt_gatt_attr bt_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(LOG_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(LOG_CHA_RX_UUID,
			BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(LOG_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static const struct sppble_stream_init_param stream_init_param = {
	.spp_uuid = (uint8_t *)log_spp_uuid,
	.gatt_attr = &bt_gatt_attr[0],
	.attr_size = ARRAY_SIZE(bt_gatt_attr),
	.tx_chrc_attr = &bt_gatt_attr[3],
	.tx_attr = &bt_gatt_attr[4],
	.tx_ccc_attr = &bt_gatt_attr[5],
	.rx_attr = &bt_gatt_attr[2],
	.connect_cb = _sppble_connect_cb,
	.read_timeout = OS_FOREVER,	/* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
	.write_timeout = OS_FOREVER,
};

static logsrv_ctx_t *logsrv_ctx = NULL;
static uint8_t logsrv_svc_buf[LOG_BUFFER_NUM][LOG_BUFFER_SIZE + LOG_HEAD_SIZE] __aligned(4);

/**********************
 *      MACROS
 **********************/
#define RD_BUF_IDX(idx) ((idx) & (LOG_BUFFER_NUM - 1))
#define WR_BUF_IDX(idx) ((idx) & (LOG_BUFFER_NUM - 1))

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int bt_log_service_start(void)
{
	if (!srv_manager_check_service_is_actived(LOG_SERVICE_NAME)) {
		return (srv_manager_active_service(LOG_SERVICE_NAME) == true) ? 0 : -1;
	}

	return 0;
}

int bt_log_service_stop(void)
{
	if (srv_manager_check_service_is_actived(LOG_SERVICE_NAME)) {
		return (srv_manager_exit_service(LOG_SERVICE_NAME) == true) ? 0 : -1;
	}

	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void _sppble_connect_cb(int connected, uint8_t connect_type)
{
	if (!logsrv_ctx)
		return;

	SYS_LOG_INF("logsrv: connect: %d", connected);

	struct app_msg msg = {
		.type = MSG_LOGSRV_BT_CONNECT,
		.value = connected,
	};

	send_async_msg(LOG_SERVICE_NAME, &msg);
}

static int _sppble_get_rx_data(io_stream_t stream, void *buf, int size)
{
	uint8_t *buf8 = buf;
	int read_size;

	while (size > 0) {
		read_size = stream_read(stream, buf, size);
		if (read_size <= 0) {
			SYS_LOG_ERR("logsrv: rx failed (%d)", size);
			return -EIO;
		}

		size -= read_size;
		buf8 += read_size;
	}

	return 0;
}

static int _sppble_put_tx_data(io_stream_t stream, const void *buf, int size)
{
	int len = stream_write(stream, buf, size);

	if (len != size) {
		SYS_LOG_ERR("logsrv: tx failed (%d/%d)", len, size);
		return -EIO;
	}

	return 0;
}

static int _sppble_drop_rx_data(io_stream_t stream, int size, int timeout)
{
	int wait_ms;
	uint8_t c;

	if (size == 0)
		return 0;

	if (timeout < 0 && size < 0)
		return -EINVAL;

	if (timeout < 0) /* wait forever */
		timeout = INT_MAX;

	if (size < 0) /* drop all data */
		size = INT_MAX;

	wait_ms = timeout;

	while (size > 0) {
		int data_len = stream_tell(stream);

		if (data_len > 0) {
			do {
				int res = _sppble_get_rx_data(stream, &c, sizeof(uint8_t));
				if (res)
					return res;
			} while (--data_len > 0 && --size > 0);

			wait_ms = timeout;
			continue;
		}

		if (wait_ms <= 0)
			break;

		os_sleep(10);
		wait_ms -= 10;
	}

	return (size > 0) ? -ETIME : 0;
}

static void * _tlv_pack_head(void *buf, uint8_t type, uint16_t total_len)
{
	tlv_head_t *head = buf;

	head->type = type;
	sys_put_le16(total_len, head->len);

	return (uint8_t *)buf + sizeof(*head);
}

static void * _tlv_pack_data(void *buf, uint16_t len, const void *data)
{
	uint8_t *buf8 = buf;

	if (len > 0) {
		memcpy(buf8, data, len);
		buf8 += len;
	}

	return buf8;
}

static void * _tlv_pack(void *buf, uint8_t type, uint16_t len,  const void *data)
{
	buf = _tlv_pack_head(buf, type, len);
	buf = _tlv_pack_data(buf, len, data);

	return buf;
}

__unused
static void * _tlv_pack_u8(void *buf, uint8_t type, uint8_t value)
{
	return _tlv_pack(buf, type, sizeof(value), &value);
}

__unused
static void * _tlv_pack_u16(void *buf, uint8_t type, uint16_t value)
{
	return _tlv_pack(buf, type, sizeof(value), &value);
}

__unused
static void * _tlv_pack_u32(void *buf, uint8_t type, uint32_t value)
{
	return _tlv_pack(buf, type, sizeof(value), &value);
}

static int _tlv_unpack_head(io_stream_t stream, tlv_dsc_t *tlv)
{
	tlv_head_t head;
	int res;

	/* read type */
	res = _sppble_get_rx_data(stream, &head, sizeof(head));
	if (res) {
		SYS_LOG_ERR("logsrv: read tlv head failed");
		return res;
	}

	tlv->type = head.type;
	tlv->len = sys_get_le16(head.len);
	return 0;
}

static int _tlv_unpack_data(io_stream_t stream, tlv_dsc_t *tlv)
{
	uint16_t data_len;

	data_len = (tlv->len <= tlv->max_len) ? tlv->len : tlv->max_len;
	if (data_len > 0) {
		int res = _sppble_get_rx_data(stream, tlv->buf, data_len);
		if (res) {
			SYS_LOG_ERR("logsrv: read tlv data failed");
			return res;
		}
	}

	if (tlv->len > tlv->max_len) {
		SYS_LOG_WRN("logsrv: tlv %d: len %u exceed max %u", tlv->type, tlv->len, tlv->max_len);
		_sppble_drop_rx_data(stream, tlv->len - tlv->max_len, -1);
		tlv->len = tlv->max_len;
	}

	return 0;
}

static int _tlv_unpack(io_stream_t stream, tlv_dsc_t *tlv)
{
	int res;

	res = _tlv_unpack_head(stream, tlv);
	if (res == 0 && tlv->len > 0)
		res = _tlv_unpack_data(stream, tlv);

	return res;
}

static void * _svc_prot_pack_head(void *buf, uint8_t cmd, uint16_t param_len)
{
	svc_prot_head_t *head = buf;

	head->svc_id = SVC_SRV_ID;
	head->cmd = cmd;
	head->param_type = SVC_PARAM_TYPE_FIXED;
	sys_put_le16(param_len, head->param_len);

	return (uint8_t *)buf + sizeof(*head);
}

static int _svc_prot_send_ack(io_stream_t stream)
{
	uint8_t buf[sizeof(svc_prot_head_t) + sizeof(tlv_head_t) + 4];
	uint8_t *tlv_buf = _svc_prot_pack_head(buf, SVC_CMD_ID_ACK, sizeof(tlv_head_t) + 4);

	_tlv_pack_u32(tlv_buf, TLV_TYPE_ACK, TLV_ACK_OK);

	return _sppble_put_tx_data(stream, buf, sizeof(buf));
}

static int _svc_prot_send_end(io_stream_t stream)
{
	uint8_t buf[sizeof(svc_prot_head_t)];
	_svc_prot_pack_head(buf, SVC_CMD_ID_TX_END, 0);

	return _sppble_put_tx_data(stream, buf, sizeof(buf));
}

static void _logsrv_clear_log(logsrv_ctx_t *ctx)
{
	os_mutex_lock(&ctx->mutex, OS_FOREVER);

	memset(ctx->len, 0, sizeof(ctx->len));
	ctx->rd_idx = ctx->wr_idx;

	os_mutex_unlock(&ctx->mutex);
}

/* Return number of data written */
static uint32_t _logsrv_save_log(logsrv_ctx_t *ctx, const uint8_t *data, uint32_t len, uint8_t type)
{
	uint32_t org_len = len;

	os_mutex_lock(&ctx->mutex, OS_FOREVER);

	do {
		if (ctx->wr_idx - ctx->rd_idx >= LOG_BUFFER_NUM) {
			atomic_inc(&ctx->drop_cnt);
			break;
		}

		uint8_t idx = WR_BUF_IDX(ctx->wr_idx);
		uint16_t size = ctx->len[idx];
		uint16_t space = LOG_BUFFER_SIZE - size;
		uint8_t *buf = &logsrv_svc_buf[idx][LOG_HEAD_SIZE + size];
		uint16_t copy_len = (len <= space) ? len : space;

		if (size > 0) {
			if (logsrv_svc_buf[idx][0] != type) {
				ctx->wr_idx++;
				continue;
			}
		} else { /* save log type */
			logsrv_svc_buf[idx][0] = type;
		}

		memcpy(buf, data, copy_len);
		ctx->len[idx] += copy_len;

		data += copy_len;
		len -= copy_len;
		space -= copy_len;
		if (space == 0) {
			ctx->wr_idx++;
		}
	} while (len > 0);

	os_mutex_unlock(&ctx->mutex);

	return org_len -len;
}

static int _logsrv_send_log(logsrv_ctx_t *ctx)
{
	int drop_cnt;

	while (ctx->rd_idx != ctx->wr_idx) {
		do {
			uint8_t idx = RD_BUF_IDX(ctx->rd_idx);
			uint8_t *buf8 = logsrv_svc_buf[idx];
			uint8_t type = buf8[0];
			int ret;

			drop_cnt = atomic_set(&ctx->drop_cnt, 0);
			if (drop_cnt > 0) {
				//SYS_LOG_WRN("logsrv: lost %u\n", drop_cnt);
				os_printk("logsrv: drop %d\n", drop_cnt);
			}

			buf8 = _svc_prot_pack_head(buf8, SVC_CMD_ID_TX, TLV_HEAD_SIZE + ctx->len[idx]);
			buf8 = _tlv_pack_head(buf8, type, ctx->len[idx]);

			ret = _sppble_put_tx_data(ctx->sppble_stream, logsrv_svc_buf[idx],
					ctx->len[idx] + LOG_HEAD_SIZE);
			if (ret) {
				return ret;
			}

			ctx->len[idx] = 0;
			ctx->rd_idx++;
		} while (ctx->rd_idx != ctx->wr_idx);

		os_mutex_lock(&ctx->mutex, OS_FOREVER);
		if (ctx->rd_idx == ctx->wr_idx) {
			uint8_t idx = WR_BUF_IDX(ctx->wr_idx);
			if (ctx->len[idx] > 0) {
				ctx->wr_idx++;
			}
		}
		os_mutex_unlock(&ctx->mutex);
	}

	return 0;
}

static int _logsrv_send_log_end(logsrv_ctx_t *ctx)
{
	return _svc_prot_send_end(ctx->sppble_stream);
}

static int _logsrv_actlog_traverse_callback(uint8_t *data, uint32_t len)
{
	logsrv_ctx_t *ctx = logsrv_ctx;
	uint32_t xfer_len;

	while (len > 0) {
		xfer_len = _logsrv_save_log(ctx, data, len, ctx->log_type);
		if (xfer_len != len) {
			/* make sure the drop cnt correct */
			atomic_dec(&ctx->drop_cnt);

			if (_logsrv_send_log(ctx)) {
				atomic_inc(&ctx->drop_cnt);
				break;
			}
		}

		len -= xfer_len;
		data += xfer_len;
	}

	return 0;
}

static void _logsrv_actlog_send_syslog(logsrv_ctx_t *ctx, uint8_t syslog_type)
{
	int drop_cnt;

#ifdef CONFIG_TASK_WDT
	task_wdt_stop();
#endif

	int len = actlog_bt_transfer(syslog_type, _logsrv_actlog_traverse_callback);
	os_printk("logsrv: log %d len %d\n", syslog_type, len);

	/* send the last logs */
	if (_logsrv_send_log(ctx)) {
		atomic_inc(&ctx->drop_cnt);
	}

	_logsrv_send_log_end(ctx);

	drop_cnt = atomic_get(&ctx->drop_cnt);
	if (drop_cnt > 0) {
		os_printk("logsrv: drop %d (type %u)\n", drop_cnt, ctx->log_type);
	}
}

static void _logsrv_actlog_backend_callback(const uint8_t *data, uint32_t len, void *user_data)
{
	logsrv_ctx_t *ctx = user_data;

	_logsrv_save_log(ctx, data, len, ctx->log_type);
}

static void _logsrv_update_log_type(logsrv_ctx_t *ctx)
{
	if (ctx->new_log_type == ctx->log_type) {
		return;
	}

	if (ctx->log_type == TLV_TYPE_LOG_DEFAULT) {
		act_log_unregister_output_backend(ACTLOG_OUTPUT_MODE_USER);
		_logsrv_send_log_end(ctx);
	}

	_logsrv_clear_log(ctx);
	atomic_set(&ctx->drop_cnt, 0);
	ctx->log_type = ctx->new_log_type;

	switch (ctx->log_type) {
	case TLV_TYPE_LOG_DEFAULT:
		act_log_register_output_backend(ACTLOG_OUTPUT_MODE_USER,
				_logsrv_actlog_backend_callback, ctx);
		thread_timer_start(&ctx->ttimer, LOG_TX_PERIOD, LOG_TX_PERIOD);
		break;
	case TLV_TYPE_LOG_SYSLOG:
		_logsrv_actlog_send_syslog(ctx, LOG_TYPE_SYSLOG);
		break;
	case TLV_TYPE_LOG_COREDUMP:
		_logsrv_actlog_send_syslog(ctx, LOG_TYPE_COREDUMP);
		break;
	case TLV_TYPE_LOG_RUNTIME:
		_logsrv_actlog_send_syslog(ctx, LOG_TYPE_RUN_LOG);
		break;
	case TLV_TYPE_LOG_RAMDUMP:
		_logsrv_actlog_send_syslog(ctx, LOG_TYPE_RAMDUMP);
		break;
	default:
		break;
	}

	if (ctx->log_type != TLV_TYPE_LOG_DEFAULT) {
		thread_timer_start(&ctx->ttimer, LOG_RX_PERIOD, LOG_RX_PERIOD);
	}
}

static int _logsrv_proc_console_codes(logsrv_ctx_t *ctx, tlv_dsc_t *tlv)
{
	uint16_t code;

	if (tlv->len != 2)
		return -EINVAL;

	code = sys_get_le16(tlv->buf);
	SYS_LOG_INF("logsrv: code 0x%x\n", code);

	switch (code) {
	case TLV_CODE_LOG_DEFAULT_START:
		ctx->new_log_type = TLV_TYPE_LOG_DEFAULT;
		break;
	case TLV_CODE_LOG_SYSLOG_START:
		ctx->new_log_type = TLV_TYPE_LOG_SYSLOG;
		break;
	case TLV_CODE_LOG_COREDUMP_START:
		ctx->new_log_type = TLV_TYPE_LOG_COREDUMP;
		break;
	case TLV_CODE_LOG_RUNTIME_START:
		ctx->new_log_type = TLV_TYPE_LOG_RUNTIME;
		break;
	case TLV_CODE_LOG_RAMDUMP_START:
		ctx->new_log_type = TLV_TYPE_LOG_RAMDUMP;
		break;
	case TLV_CODE_LOG_STOP:
		ctx->new_log_type = TLV_TYPE_LOG_NONE;
		break;
	case TLV_CODE_PREPARE:
		SYS_LOG_INF("logsrv: prepared\n");
		ctx->prepared = 1;
		break;
	default:
		break;
	}

	return 0;
}

static int _logsrv_proc_console_text(logsrv_ctx_t *ctx, tlv_dsc_t *tlv)
{
	int ret = 0;

	if (tlv->len <= 0)
		return -ENODATA;

	tlv->buf[MIN(tlv->len, tlv->max_len - 1)] = 0;
	SYS_LOG_INF("logsrv: cmd(%d) %s", tlv->len, (char *)tlv->buf);

#ifdef CONFIG_SHELL_BACKEND_SERIAL
	shell_uart_register_output_callback(_logsrv_actlog_backend_callback, ctx);
	ret = shell_execute_cmd(shell_backend_uart_get_ptr(), tlv->buf);
	shell_uart_register_output_callback(NULL, NULL);
#elif defined(CONFIG_SHELL_BACKEND_RTT)
	shell_rtt_register_output_callback(_logsrv_actlog_backend_callback, ctx);
	ret = shell_execute_cmd(shell_backend_rtt_get_ptr(), tlv->buf);
	shell_rtt_register_output_callback(NULL, NULL);
#endif
	/* send real-time logs */
	_logsrv_send_log(ctx);

	return ret;
}

static int _logsrv_proc_tlv_cmd(logsrv_ctx_t *ctx, uint16_t param_len)
{
	static uint8_t s_tlv_buf[64];
	tlv_dsc_t tlv = { .max_len = sizeof(s_tlv_buf), .buf = s_tlv_buf, };

	while (param_len > 0) {
		if (_tlv_unpack(ctx->sppble_stream, &tlv))
			return -ENODATA;

		SYS_LOG_DBG("logsrv: tlv type 0x%x, len %u, param_len %d\n",
				tlv.type, tlv.len, param_len);

		switch (tlv.type) {
		case TLV_TYPE_CONSOLE_CODE:
			_logsrv_proc_console_codes(ctx, &tlv);
			break;
		case TLV_TYPE_CONSOLE_TEXT:
			_logsrv_proc_console_text(ctx, &tlv);
			break;
		default:
			SYS_LOG_ERR("logsrv: invalid tlv type %u\n", tlv.type);
			break;
		}

		param_len -= TLV_SIZE(&tlv);
	}

	return 0;
}

static void _logsrv_proc_svc_cmd(logsrv_ctx_t *ctx)
{
	svc_prot_head_t head;
	uint16_t param_len;
	int res = -EINVAL;

	if (!ctx->stream_opened ||
		stream_tell(ctx->sppble_stream) < sizeof(head)) {
		return;
	}

	if (_sppble_get_rx_data(ctx->sppble_stream, &head, sizeof(head))) {
		SYS_LOG_ERR("read svc_prot_head failed");
		return;
	}

	param_len = sys_get_le16(head.param_len);

	SYS_LOG_DBG("logsrv: svc_id 0x%x, cmd_id 0x%x, param type 0x%x, param len 0x%x\n",
		head.svc_id, head.cmd, head.param_type, param_len);

	if (head.svc_id != SVC_SRV_ID) {
		SYS_LOG_ERR("logsrv: invalid svc_id 0x%x\n", head.svc_id);
		goto out_exit;
	}

	if (head.param_type != SVC_PARAM_TYPE_FIXED) {
		SYS_LOG_ERR("logsrv: invalid param type 0x%x\n", head.param_type);
		goto out_exit;
	}

	switch (head.cmd) {
	case SVC_CMD_ID_RX:
		res = _logsrv_proc_tlv_cmd(ctx, param_len);
		break;
	default:
		SYS_LOG_ERR("logsrv: invalid svc cmd %d", head.cmd);
		break;
	}

out_exit:
	if (res) {
		SYS_LOG_ERR("logsrv: invalid svc, drop all\n");
		_sppble_drop_rx_data(ctx->sppble_stream, -1, 500);
	} else {
		_svc_prot_send_ack(ctx->sppble_stream);
	}
}

static void _logsrv_timer_handle(struct thread_timer *ttimer, void *arg)
{
	logsrv_ctx_t *ctx = arg;

	_logsrv_proc_svc_cmd(ctx);
	_logsrv_update_log_type(ctx);

	if (ctx->log_type == TLV_TYPE_LOG_DEFAULT) {
		/* send real-time logs */
		_logsrv_send_log(ctx);
	}
}

static int _logsrv_init(void)
{
	logsrv_ctx_t *ctx = app_mem_malloc(sizeof(*ctx));
	if (!ctx) {
		SYS_LOG_ERR("logsrv: alloc ctx failed\n");
		return -ENOMEM;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->log_type = TLV_TYPE_LOG_NONE;
	ctx->new_log_type = TLV_TYPE_LOG_NONE;

	/* Just call stream_create once, for register spp/ble service
	 * not need call stream_destroy
	 */
	ctx->sppble_stream = sppble_stream_create((void *)&stream_init_param);
	if (!ctx->sppble_stream) {
		SYS_LOG_ERR("logsrv: stream_create failed");
		app_mem_free(ctx);
		return -EIO;
	}

	os_mutex_init(&ctx->mutex);
	thread_timer_init(&ctx->ttimer, _logsrv_timer_handle, ctx);
	logsrv_ctx = ctx;

	SYS_LOG_INF("logsrv: init\n");
	return 0;
}

static void _logsrv_exit(void)
{
	logsrv_ctx_t *ctx = logsrv_ctx;

	ctx->new_log_type = TLV_TYPE_LOG_NONE;
	_logsrv_update_log_type(ctx);

	thread_timer_stop(&ctx->ttimer);

	if (ctx->stream_opened) {
		stream_close(ctx->sppble_stream);
	}

	stream_destroy(ctx->sppble_stream);
	app_mem_free(ctx);

	logsrv_ctx = NULL;

	SYS_LOG_INF("logsrv: exit\n");
}

static void _logsrv_handle_connection(int connected)
{
	logsrv_ctx_t *ctx = logsrv_ctx;
	int res;

	if (connected) {
		if (!ctx->stream_opened) {
			res = stream_open(ctx->sppble_stream, MODE_IN_OUT);
			if (!res) {
				ctx->stream_opened = 1;
				thread_timer_start(&ctx->ttimer, LOG_RX_PERIOD, LOG_RX_PERIOD);
			}

#ifdef CONFIG_SYS_WAKELOCK
			sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif

			SYS_LOG_INF("logsrv: stream open (res=%d)\n", res);
		}
	} else if (ctx->stream_opened) {
		stream_close(ctx->sppble_stream);
		ctx->stream_opened = 0;
		ctx->prepared = 0;

		ctx->new_log_type = TLV_TYPE_LOG_NONE;
		_logsrv_update_log_type(ctx);

		thread_timer_stop(&ctx->ttimer);

#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_unlock(PARTIAL_WAKE_LOCK);
#endif

		SYS_LOG_INF("logsrv: stream close\n");
	}
}

static void _log_service_main(void *p1, void *p2, void *p3)
{
	struct app_msg msg;
	bool quit = false;

	if (_logsrv_init()) {
		srv_manager_thread_exit(LOG_SERVICE_NAME);
		return;
	}

	while (!quit) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			int result = 0;

			switch (msg.type) {
			case MSG_EXIT_APP:
				_logsrv_exit();
				srv_manager_thread_exit(LOG_SERVICE_NAME);
				quit = true;
				break;

			case MSG_LOGSRV_BT_CONNECT:
				_logsrv_handle_connection(msg.value);
				break;

			case MSG_START_APP:
			default:
				break;
			}

			if (msg.callback) {
				msg.callback(&msg, result, NULL);
			}
		}

		if (!quit) {
			thread_timer_handle_expired();
		}
	}
}

static char __aligned(ARCH_STACK_PTR_ALIGN) logsrv_stack[CONFIG_BT_LOG_SERVICE_STACK_SIZE];

SERVICE_DEFINE(logsrv, logsrv_stack, CONFIG_BT_LOG_SERVICE_STACK_SIZE,
		CONFIG_BT_LOG_SERVICE_PRIORITY, BACKGROUND_APP, NULL, NULL, NULL,
		_log_service_main);
