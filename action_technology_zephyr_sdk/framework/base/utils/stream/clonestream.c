/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file clone stream interface
 */
#define SYS_LOG_DOMAIN "clonestream"
#include <mem_manager.h>
#include <clone_stream.h>
#include <mem_manager.h>
#include <stream.h>
#include <assert.h>
#include <errno.h>
#include <sys/util.h>

static bool validate_stream_state(io_stream_t handle, uint8_t state)
{
	return handle->state == state;
}

static int clone_stream_init(io_stream_t handle, void *param)
{
	struct clone_stream_info *info = param;
	int i;

	if (!validate_stream_state(info->origin, STATE_INIT)) {
		SYS_LOG_ERR("state not init\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(info->clones); i++) {
		if (!info->clones[i])
			break;

		if (!validate_stream_state(info->clones[i], STATE_OPEN)) {
			SYS_LOG_ERR("state not open\n", i);
			return -EINVAL;
		}

		if (!(info->clones[i]->mode & MODE_OUT)) {
			SYS_LOG_ERR("stream clone[%d] not writable\n", i);
			return -EINVAL;
		}
	}

	handle->data = mem_malloc(sizeof(*info));
	if (!handle->data) {
		SYS_LOG_ERR("malloc data\n", i);
		return -ENOMEM;
	}

	memcpy(handle->data, info, sizeof(*info));
	return 0;
}

static int clone_stream_open(io_stream_t handle, stream_mode mode)
{
	struct clone_stream_info *info = handle->data;
	int res;

	SYS_LOG_DBG(" call\n");

	res = stream_open(info->origin, mode);
	if (res)
		return res;

	handle->write_finished = 0;
	return 0;
}

static int clone_stream_read(io_stream_t handle, unsigned char *buf,int num)
{
	struct clone_stream_info *info = handle->data;
	int len, i;

	len = stream_read(info->origin, buf, num);
	if (len <= 0)
		return len;

	/* clone read data */
	if (handle->mode & info->clone_mode) {
		for (i = 0; i < ARRAY_SIZE(info->clones); i++) {
			if (!info->clones[i])
				break;

			info->clones[i]->ops->write(info->clones[i], buf, len);
		}
	}

	return len;
}

static int clone_stream_write(io_stream_t handle, unsigned char *buf,int num)
{
	struct clone_stream_info *info = handle->data;
	int len, i;

	len = stream_write(info->origin, buf, num);
	if (len <= 0)
		return len;

	/* clone write data */
	if (handle->mode & info->clone_mode) {
		for (i = 0; i < ARRAY_SIZE(info->clones); i++) {
			if (!info->clones[i])
				break;

			info->clones[i]->ops->write(info->clones[i], buf, len);
		}
	}

	return len;
}

static int clone_stream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	/* FIXME: seek ops will make clone stream data disordered */
	return -ENOSYS;
}

static int clone_stream_tell(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	return stream_tell(info->origin);
}

static int clone_stream_get_length(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	return stream_get_length(info->origin);
}

static int clone_stream_get_space(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	return stream_get_space(info->origin);
}

static int clone_stream_flush(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	int res, i;

	res = stream_flush(info->origin);
	for (i = 0; i < ARRAY_SIZE(info->clones); i++) {
		if (!info->clones[i])
			break;

		if (info->clones[i]->ops->flush)
			info->clones[i]->ops->flush(info->clones[i]);
	}

	return res;
}

static int clone_stream_close(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	int i;

	/* notify write finished */
	for (i = 0; i < ARRAY_SIZE(info->clones); i++) {
		if (!info->clones[i])
			break;

		info->clones[i]->ops->write(info->clones[i], NULL, 0);
	}

	return stream_close(info->origin);
}

static int clone_stream_destroy(io_stream_t handle)
{
	struct clone_stream_info *info = handle->data;
	int res;

	res = stream_destroy(info->origin);
	mem_free(info);
	return res;
}

const stream_ops_t clone_stream_ops = {
	.init = clone_stream_init,
	.open = clone_stream_open,
	.read = clone_stream_read,
	.seek = clone_stream_seek,
	.tell = clone_stream_tell,
	.get_length = clone_stream_get_length,
	.get_space = clone_stream_get_space,
	.write = clone_stream_write,
	.flush = clone_stream_flush,
	.close = clone_stream_close,
	.destroy = clone_stream_destroy,
};

io_stream_t clone_stream_create(struct clone_stream_info *info)
{
	return stream_create(&clone_stream_ops, info);

}