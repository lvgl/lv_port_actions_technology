/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <os_common_api.h>
#include <input_recorder.h>

typedef struct inputrec_buffer_ctx {
	uint8_t *base;
	uint8_t *ptr;
	uint8_t *end;
} inputrec_buffer_ctx_t;

static inputrec_buffer_ctx_t inputrec_buffer_ctx;

static int _input_data_buffer_read(input_rec_data_t *data, void *user_data)
{
	inputrec_buffer_ctx_t *ctx = user_data;

	if (ctx->ptr + sizeof(*data) > ctx->end) {
		return -ENODATA;
	}

	memcpy(data, ctx->ptr, sizeof(*data));
	ctx->ptr += sizeof(*data);
	return 0;
}

static int _input_data_buffer_write(const input_rec_data_t *data, void *user_data)
{
	inputrec_buffer_ctx_t *ctx = user_data;

	if (ctx->ptr + sizeof(*data) > ctx->end) {
		return -ENODATA;
	}

	memcpy(ctx->ptr, data, sizeof(*data));
	ctx->ptr += sizeof(*data);
	return 0;
}

static void _input_data_buffer_rewind(void *user_data)
{
	inputrec_buffer_ctx_t *ctx = user_data;

	ctx->ptr = ctx->base;
}

static int _record_ctx_init(const void *buffer, uint32_t size)
{
	inputrec_buffer_ctx_t *ctx = &inputrec_buffer_ctx;

	if (buffer == NULL || size < sizeof(input_rec_data_t)) {
		return -EINVAL;
	}

	ctx->base = (uint8_t *)buffer;
	ctx->ptr = ctx->base;
	ctx->end = ctx->base + size;
	return 0;
}

int input_capture_buffer_start(void *buffer, uint32_t size)
{
	if (_record_ctx_init(buffer, size)) {
		return -EINVAL;
	}

	return input_capture_start(_input_data_buffer_write, &inputrec_buffer_ctx);
}

int input_playback_buffer_start(const void *buffer, uint32_t size, bool repeat)
{
	if (_record_ctx_init(buffer, size)) {
		return -EINVAL;
	}

	return input_playback_start(_input_data_buffer_read,
			repeat ? _input_data_buffer_rewind : NULL,
			INPUTREC_DEFAULT_REPEAT_DELAY, &inputrec_buffer_ctx);
}
