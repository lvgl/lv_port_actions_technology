/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <audio_system.h>
#include <media_mem.h>
#include <media_player.h>
#include "tool_app_inner.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define ECTT_TRANSFER_SIZE (512)
#define ECTT_MODSEL_COUNT (20)

/* should less than ECTT_MODSEL_COUNT */
#define MAX_ECTT_DUMP  (3)
#define ECTT_DUMP_BACKSTORE_BUF_SIZE	(MAX_ECTT_DUMP * ECTT_TRANSFER_SIZE * 2)

//static char ectt_upload_backstore[MAX_ECTT_DUMP * ECTT_TRANSFER_SIZE * 2];
static const uint8_t ectt_tag_table[9] = {
	MEDIA_DATA_TAG_MIC1, MEDIA_DATA_TAG_MIC2, MEDIA_DATA_TAG_REF,
	MEDIA_DATA_TAG_AEC1, MEDIA_DATA_TAG_AEC2, MEDIA_DATA_TAG_ENCODE_IN1,
	MEDIA_DATA_TAG_DECODE_IN, MEDIA_DATA_TAG_DECODE_OUT1, MEDIA_DATA_TAG_ENCODE_OUT,
};

enum ECTT_status {
	ECTT_START = 1,
	ECTT_STOP = 2,
};

/* ECTT upload format*/
/* ----------------------------------------
 * | 2 bytes  |   2 bytes |  data size   |
 * ----------------------------------------
 * | data tag | data size | data content |
 * ----------------------------------------
 */

struct ECTT_transfer {
	uint16_t mod;
	uint16_t size;
	uint8_t  data[0];
};

struct ECTT_modsel {
	uint8_t bSelArray[ECTT_MODSEL_COUNT];
};

struct ETCC_data {
	struct ECTT_modsel modsel;

	uint8_t dump_msel[MAX_ECTT_DUMP];
	uint8_t dump_tags[MAX_ECTT_DUMP];
	struct acts_ringbuf *dump_bufs[MAX_ECTT_DUMP];

	/* backstore memory for dump_bufs */
	char *dump_backstore;
	/* etcc stub_ext_param_t's rw_buffer */
	char *stub_rwbuf;
	size_t stub_rwbuf_len;
};

static void _etcc_data_free(struct ETCC_data *data);
static void *_realloc_ectt_stub_parambuf(struct ETCC_data *data, size_t size);
static void _free_ectt_stub_parambuf(struct ETCC_data *data);

static struct ETCC_data *_etcc_data_alloc(void)
{
	struct ETCC_data *data = app_mem_malloc(sizeof(*data));
	if (!data)
		return NULL;

	memset(data, 0, sizeof(*data));

	if (media_mem_get_cache_pool_size(TOOL_ECTT_BUF, AUDIO_STREAM_MUSIC) < ECTT_DUMP_BACKSTORE_BUF_SIZE) {
		SYS_LOG_ERR("TOOL_ECTT_BUF too small");
		app_mem_free(data);
		return NULL;
	}

	data->dump_backstore = media_mem_get_cache_pool(TOOL_ECTT_BUF, AUDIO_STREAM_MUSIC);

	/* must not less than one transfer size*/
	if (!_realloc_ectt_stub_parambuf(data, sizeof(struct ECTT_transfer) + ECTT_TRANSFER_SIZE)) {
		app_mem_free(data);
		return NULL;
	}

	return data;
}

static void _etcc_data_free(struct ETCC_data *data)
{
	for (int i = 0; i < ARRAY_SIZE(data->dump_bufs); i++) {
		if (data->dump_bufs[i]) {
			media_player_dump_data(NULL, 1, &data->dump_tags[i], NULL);
			acts_ringbuf_destroy_ext(data->dump_bufs[i]);
			data->dump_bufs[i] = NULL;
		}
	}

	_free_ectt_stub_parambuf(data);
	app_mem_free(data);
}

static void *_realloc_ectt_stub_parambuf(struct ETCC_data *data, size_t size)
{
	size_t rwbuf_len = sizeof(stub_ext_cmd_t) + size + 2; /* 2 bytes checksum */
	if (data->stub_rwbuf_len >= rwbuf_len)
		return data->stub_rwbuf;

	size_t new_len = ROUND_UP(rwbuf_len, 512);
	char *new_buf = app_mem_malloc(new_len);
	if (new_buf == NULL)
		return NULL;

	if (data->stub_rwbuf)
		app_mem_free(data->stub_rwbuf);

	data->stub_rwbuf = new_buf;
	data->stub_rwbuf_len = new_len;

	return data->stub_rwbuf;
}

static void _free_ectt_stub_parambuf(struct ETCC_data *data)
{
	if (data->stub_rwbuf) {
		app_mem_free(data->stub_rwbuf);
		data->stub_rwbuf = NULL;
	}

	data->stub_rwbuf_len = 0;
}

static int _ectt_stub_read(struct ETCC_data *data, uint16_t opcode, void *buf, size_t size)
{
	stub_ext_param_t param = { .opcode = opcode, .payload_len = 0, };
	int ret = 0;

	/* allocate stub param rw_buffer */
	param.rw_buffer = _realloc_ectt_stub_parambuf(data, size);
	if (param.rw_buffer == NULL) {
		SYS_LOG_ERR("mem_malloc stub rw_buffer failed");
		return -ENOMEM;
	}

	/* send read request */
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read request failed");
		return ret;
	}

	/* receive data */
	param.payload_len = size;
	ret = stub_ext_read(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read data failed");
		return ret;
	}

	memcpy(buf, &param.rw_buffer[sizeof(stub_ext_cmd_t)], size);

	/* send ack */
	param.opcode = STUB_CMD_ECTT_ACK;
	param.payload_len = 0;
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("write ack failed");
		return ret;
	}

	return 0;
}

/* For a data transfer (size > 0 && msel >= 0), assume buf is a dsp session buffer */
static int _ectt_stub_write(struct ETCC_data *data, uint16_t opcode, const void *buf, size_t size, int msel)
{
	size_t ext_size = (msel >= 0) ? sizeof(struct ECTT_transfer)  : 0;
	stub_ext_param_t param = { .opcode = opcode, .payload_len = size + ext_size, };
	int ret = 0;

	/* allocate stub param rw_buffer */
	param.rw_buffer = _realloc_ectt_stub_parambuf(data, size + ext_size);
	if (param.rw_buffer == NULL) {
		SYS_LOG_ERR("mem_malloc stub rw_buffer failed");
		return -ENOMEM;
	}

	if (size > 0) {
		if (msel >= 0) {
			struct ECTT_transfer *transfer = (struct ECTT_transfer *)&param.rw_buffer[sizeof(stub_ext_cmd_t)];
			transfer->mod = (uint16_t)msel;
			transfer->size = size;
			ret = acts_ringbuf_get((void *)buf, transfer->data, size);
			if (ret != size) {
				SYS_LOG_ERR("no enough data");
				return ret;
			}
		} else {
			memcpy(&param.rw_buffer[sizeof(stub_ext_cmd_t)], buf, size);
		}
	}

	/* send data */
	ret = stub_ext_write(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("write data failed");
		return ret;
	}

	/* receive ack (timeout has already added in stub driver) */
	param.payload_len = 0;
	ret = stub_ext_read(tool_stub_dev_get(), &param);
	if (ret) {
		SYS_LOG_ERR("read ack failed");
		return ret;
	}

	if (sys_get_be16(&param.rw_buffer[1]) != STUB_CMD_ECTT_ACK) {
		SYS_LOG_ERR("invalid ack 0x%02x", sys_get_be16(&param.rw_buffer[1]));
		return -EINVAL;
	}

	return 0;
}

static int _get_status(struct ETCC_data *data, uint8_t *status)
{
	if (_ectt_stub_read(data, STUB_CMD_ECTT_GET_STATUS, status, sizeof(*status)))
		return -EIO;

	return 0;
}

static int _get_modsel(struct ETCC_data *data, struct ECTT_modsel *modsel)
{
	uint8_t flag = 0;

	if (_ectt_stub_read(data, STUB_CMD_ECTT_GET_MODSEL_FLAG, &flag, sizeof(flag)))
		return -EIO;

	if (!flag)
		return 0;

	if (_ectt_stub_read(data, STUB_CMD_ECTT_GET_MODSEL_VALUE, modsel, sizeof(*modsel)))
		return -EIO;

	return 0;
}

static int _start_upload_data(struct ETCC_data *data)
{
	void *debug_buff;
	int num = 0;

	if (_get_modsel(data, &data->modsel)) {
		SYS_LOG_ERR("fail to get modsel");
		return -EIO;
	}

	media_player_t *player = media_player_get_current_dumpable_player();
	if (!player) {
		SYS_LOG_ERR("fail to get player");
		return -EFAULT;
	}

	for (int i = 0; i < min(ECTT_MODSEL_COUNT, ARRAY_SIZE(ectt_tag_table)); i++) {
		if (!data->modsel.bSelArray[i])
			continue;

		if (num >= ARRAY_SIZE(data->dump_bufs)) {
			SYS_LOG_WRN("exceed max %d tags", (unsigned int)ARRAY_SIZE(data->dump_bufs));
			break;
		}

		debug_buff = data->dump_backstore + num * ECTT_TRANSFER_SIZE * 2;
		data->dump_bufs[num] = acts_ringbuf_init_ext(debug_buff, ECTT_TRANSFER_SIZE * 2);
		if (!data->dump_bufs[num]) {
			SYS_LOG_ERR("acts_ringbuf_init_ext failed");
			break;
		}

		SYS_LOG_INF("start idx=%d, tag=%u, buff=%p, msel=%d", i, ectt_tag_table[i], data->dump_bufs[num], i);
		data->dump_tags[num] = ectt_tag_table[i];
		data->dump_msel[num++] = i;
	}

	media_player_dump_data(player, num, data->dump_tags, data->dump_bufs);
	return 0;
}

static int _upload_data(struct ETCC_data *data)
{
	bool has_upload;
	int ret, len = 0;

	do {
		has_upload = false;

		for (int i = 0; i < ARRAY_SIZE(data->dump_bufs); i++) {
			if (!data->dump_bufs[i] || acts_ringbuf_length(data->dump_bufs[i]) < ECTT_TRANSFER_SIZE)
				continue;

			ret = _ectt_stub_write(data, STUB_CMD_ECTT_UPLOAD_DATA, data->dump_bufs[i], ECTT_TRANSFER_SIZE, data->dump_msel[i]);
			if (ret <= 0)
				return -EIO;

			len += ECTT_TRANSFER_SIZE;
			has_upload = true;
		}
	} while (has_upload);

	return len;
}

static int _stop_upload_data(struct ETCC_data *data)
{
	for (int i = 0; i < ARRAY_SIZE(data->dump_bufs); i++) {
		if (data->dump_bufs[i]) {
			SYS_LOG_INF("stop idx=%d, tag=%u, buff=%p", i, data->dump_tags[i], data->dump_bufs[i]);
			media_player_dump_data(NULL, 1, &data->dump_tags[i], NULL);
			acts_ringbuf_destroy_ext(data->dump_bufs[i]);
			data->dump_bufs[i] = NULL;
		}
	}

	return _ectt_stub_write(data, STUB_CMD_ECTT_STOP_UPLOAD, NULL, 0, -1);
}

static int _upload_samplerate(struct ETCC_data *data, uint32_t sample_rate)
{
	return _ectt_stub_write(data, STUB_CMD_ECTT_UPLOAD_SAMPLERATE, &sample_rate, sizeof(sample_rate), -1);
}

void tool_ectt_loop(void)
{
	struct ETCC_data *etcc_data = NULL;
	uint8_t new_status = ECTT_STOP;
	uint8_t old_status = ECTT_STOP;

	SYS_LOG_INF("Enter");

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	etcc_data = _etcc_data_alloc();
	if (!etcc_data) {
		SYS_LOG_ERR("_etcc_data_alloc failed");
		goto out_exit;
	}

	/* upload sample rate once */
	if (_upload_samplerate(etcc_data, 16000)) {
		SYS_LOG_ERR("fail to upload sample-rate");
		goto out_exit;
	}

	do {
		/* polling status */
		if (_get_status(etcc_data, &new_status)) {
			SYS_LOG_ERR("fail to get etcc status");
			break;
		}

		if (media_player_get_current_dumpable_player() == NULL) {
			/* force stop */
			if (new_status == ECTT_START) {
				SYS_LOG_DBG("player exit, force stop");
				new_status = ECTT_STOP;
			}
		}

		if (new_status != old_status) {
			SYS_LOG_INF("etcc status = %u", new_status);
			switch (new_status) {
			case ECTT_START:
				_start_upload_data(etcc_data);
				break;
			case ECTT_STOP:
				_stop_upload_data(etcc_data);
				break;
			default:
				break;
			}

			old_status = new_status;
		}

		if (new_status == ECTT_START) {
			_upload_data(etcc_data);
		} else {
			os_sleep(4);
		}
	} while (!tool_is_quitting());

out_exit:
	if (etcc_data)
		_etcc_data_free(etcc_data);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

	SYS_LOG_INF("Exit");
}
