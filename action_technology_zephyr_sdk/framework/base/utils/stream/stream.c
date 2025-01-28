/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file stream interface
 */
#define SYS_LOG_DOMAIN "stream"
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "stream_internal.h"

static bool _stream_check_handle_state(io_stream_t handle, uint8_t need_state)
{
	if (handle == NULL) {
		SYS_LOG_INF("handle is null\n");
		return false;
	}

	if (handle->state != need_state) {
		/*SYS_LOG_INF("stream state error current state %d  need_state %d handle %p \n",
						handle->state,need_state,handle);*/
		return false;
	}

	return true;
}

io_stream_t stream_create(const stream_ops_t  *ops, void *init_param)
{
	int ret = 0;
	io_stream_t stream = NULL;

	stream = mem_malloc(sizeof(struct __stream));
	if (!stream) {
		goto exit;
	}

	/*init state */
	stream->state = STATE_INIT;
	stream->rofs = 0;
	stream->wofs = 0;
	stream->ops = ops;
	os_mutex_init(&stream->attach_lock);

	if (stream->ops->init) {
		ret = stream->ops->init(stream, init_param);
	}

	if (ret) {
		SYS_LOG_ERR("create failed 0x%p \n", stream);
		mem_free(stream);
		stream = NULL;
	}

exit:
	SYS_LOG_DBG(" 0x%p \n",stream);
	return stream;
}

int stream_open(io_stream_t handle, stream_mode mode)
{
	int res;

	if (!_stream_check_handle_state(handle,STATE_INIT)) {
		if (!_stream_check_handle_state(handle,STATE_CLOSE)) {
	        return -ENOSYS;
	    }
	}

	if (((mode & MODE_IN) && !handle->ops->read) ||
		((mode & MODE_OUT) && !handle->ops->write)) {
		SYS_LOG_ERR("mode %d not permitted\n", mode);
		return -EPERM;
	}

	res = handle->ops->open(handle,mode);
	if (res) {
		SYS_LOG_ERR("open error %d\n", res);
		return res;
	}

	if((mode & (MODE_READ_BLOCK | MODE_WRITE_BLOCK))){
		handle->sync_sem = mem_malloc(sizeof(os_sem));
		if (!handle->sync_sem) {
			return -ENOMEM;
		}
		os_sem_init(handle->sync_sem, 0, 1);
		handle->write_finished = 0;
	}

	handle->mode = mode;
	handle->state = STATE_OPEN;

	return res;
}

int stream_read(io_stream_t handle, void *buf, int num)
{
	int i;
	int brw;
	int try_cnt = 0;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	if (!(handle->mode & MODE_IN)) {
		return -EPERM;
	}

	if ((handle->mode & MODE_READ_BLOCK)) {
		while (stream_get_length(handle) < num) {
			if((handle->mode & MODE_BLOCK_TIMEOUT)){
				if (try_cnt ++ > 20) {
					SYS_LOG_INF("time out 1s");
					handle->write_finished = 1;
					return 0;
				}
			}
			os_sem_take(handle->sync_sem, 50);
			if(!_stream_check_handle_state(handle,STATE_OPEN)) {
				return -ENOSYS;
			}
			if (handle->write_finished) {
				break;
			}
		}
	}

	brw = handle->ops->read(handle, buf, num);
	if (brw < 0) {
		SYS_LOG_DBG("read failed [%d]\n", brw);
		brw = 0;
		return brw;
	}

	if (handle->sync_sem) {
		os_sem_give(handle->sync_sem);
	}

	if (!os_is_in_isr()) {
		os_mutex_lock(&handle->attach_lock, OS_FOREVER);
	}

	/**data read to attached stream */
	for (i = 0; i < ARRAY_SIZE(handle->attach_stream); i++) {
		if (handle->attach_mode[i] != MODE_IN)
			continue;

		if (!handle->attach_stream[i])
			continue;

		brw = handle->attach_stream[i]->ops->write(handle->attach_stream[i], buf, num);
		if (brw != num) {
			if (!os_is_in_isr()) {
				os_mutex_unlock(&handle->attach_lock);
			}
			return brw;
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_unlock(&handle->attach_lock);
	}

	for (i = 0; i < ARRAY_SIZE(handle->observer_notify); i++) {
		if (handle->observer_notify[i] && (handle->observer_type[i] & STREAM_NOTIFY_READ)) {
			handle->observer_notify[i](handle->observer[i], handle->rofs,
				handle->wofs, handle->total_size, buf, brw, STREAM_NOTIFY_READ);
		}
	}

	return brw;
}

int stream_seek(io_stream_t handle, int offset, seek_dir origin)
{
	int i;
	int brw = 0;
	int target_off = offset;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	if (!handle->ops->seek) {
		return -ENOSYS;
	}

	switch(origin) {
	case SEEK_DIR_BEG:
		target_off = offset;
		break;
	case SEEK_DIR_CUR:
		if ((handle->mode & MODE_IN_OUT) == MODE_OUT) {
			target_off = handle->wofs + offset;
		} else {
			target_off = handle->rofs + offset;
		}
		break;
	case SEEK_DIR_END:
		target_off = handle->total_size + offset;
		break;
	default:
		SYS_LOG_ERR("mode not support 0x%x \n", origin);
		return -1;
	}

	if ((handle->mode & MODE_IN_OUT) == MODE_IN_OUT) {
		while (target_off > handle->wofs) {
			os_sem_take(handle->sync_sem, 50);
			if(!_stream_check_handle_state(handle,STATE_OPEN)) {
				return -ENOSYS;
			}
		}
	}

	brw = handle->ops->seek(handle, target_off, SEEK_DIR_BEG);
	if (brw < 0) {
		SYS_LOG_ERR("seek failed [%d]\n", brw);
		return brw;
	}

	for (i = 0; i < ARRAY_SIZE(handle->observer_notify); i++) {
		if (handle->observer_notify[i] && (handle->observer_type[i] & STREAM_NOTIFY_SEEK)) {
			handle->observer_notify[i](handle->observer[i], handle->rofs,
				handle->wofs, handle->total_size, NULL, 0, STREAM_NOTIFY_SEEK);
		}
	}

	return brw;
}

int stream_tell(io_stream_t handle)
{
	int brw;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	if  (!handle->ops->tell)  {
		return -ENOSYS;
	}

	brw = handle->ops->tell(handle);
	if (brw < 0) {
		SYS_LOG_ERR("tell failed [%d]\n", brw);
		return brw;
	}

	return brw;
}

int stream_write(io_stream_t handle, const void *buf, int num)
{
	int brw;
	int i;
	int try_cnt = 0;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	if (!(handle->mode & MODE_OUT)) {
		return -EPERM;
	}

	if ((handle->mode & MODE_WRITE_BLOCK)) {
		while (stream_get_space(handle) < num) {
			if ((handle->mode & MODE_BLOCK_TIMEOUT)) {
				if (try_cnt ++ > 20) {
					SYS_LOG_INF("time out 1s");
					handle->write_finished = 1;
					return 0;
				}
			}
			os_sem_take(handle->sync_sem, 50);
			if(!_stream_check_handle_state(handle,STATE_OPEN)) {
				return -ENOSYS;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(handle->observer_notify); i++) {
		if (handle->observer_notify[i] && (handle->observer_type[i] & STREAM_NOTIFY_PRE_WRITE)) {
			handle->observer_notify[i](handle->observer[i], handle->rofs,
				handle->wofs, handle->total_size, (void *)buf, num, STREAM_NOTIFY_PRE_WRITE);
		}
	}

	brw = handle->ops->write(handle, (void *)buf, num);
	if (brw != num) {
		//SYS_LOG_ERR("Failed writing to stream [%d]\n", brw);
		return brw;
	}

	if (!num) {
		handle->write_finished = 1;
	}

	if (handle->sync_sem)
		os_sem_give(handle->sync_sem);


	if (!os_is_in_isr()) {
		os_mutex_lock(&handle->attach_lock, OS_FOREVER);
	}

	/**data write to attached stream */
	for (i = 0; i < ARRAY_SIZE(handle->attach_stream); i++) {
		if (handle->attach_mode[i] != MODE_OUT)
			continue;

		if (!handle->attach_stream[i])
			continue;

		brw = handle->attach_stream[i]->ops->write(handle->attach_stream[i], (void *)buf, num);
		if (brw != num) {
			//SYS_LOG_ERR("Failed writing to stream [%d]\n", brw);
			if (!os_is_in_isr()) {
				os_mutex_unlock(&handle->attach_lock);
			}
			return brw;
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_unlock(&handle->attach_lock);
	}
	for (i = 0; i < ARRAY_SIZE(handle->observer_notify); i++) {
		if (handle->observer_notify[i] && (handle->observer_type[i] & STREAM_NOTIFY_WRITE)) {
			handle->observer_notify[i](handle->observer[i], handle->rofs,
				handle->wofs, handle->total_size, (void *)buf, num, STREAM_NOTIFY_WRITE);
		}
	}
	return brw;
}

int stream_flush(io_stream_t handle)
{
	int brw;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	/** same stream not support flush */
	if (!handle->ops->flush) {
		return 0;
	}

	brw = handle->ops->flush(handle);
	if (brw < 0) {
		SYS_LOG_ERR("failed [%d]\n", brw);
		return brw;
	}

	return 0;
}

int stream_close(io_stream_t handle)
{
	int res;

	if (!_stream_check_handle_state(handle, STATE_OPEN)) {
		SYS_LOG_ERR("state error\n");
		return -ENOSYS;
	}

	if (handle->attached_stream) {
		stream_detach(handle->attached_stream, handle);
	}

	res = handle->ops->close(handle);
	if (res) {
		SYS_LOG_ERR("close failed [%d]\n", res);
	}

	if (handle->sync_sem) {
		handle->write_finished = 1;
		os_sem_give(handle->sync_sem);
	}
	handle->state = STATE_CLOSE;
	return res;
}

int stream_destroy(io_stream_t handle)
{
	int res = 0;

	if (handle->attached_stream) {
		stream_detach(handle->attached_stream, handle);
	}

	/* ops->destroy should also be allowed to be NULL, since ops->init is allowed to be NULL */
	if (handle->ops->destroy) {
		res = handle->ops->destroy(handle);
		if (res) {
			SYS_LOG_ERR("destroy failed [%d]\n", res);
		}
	}

	if (handle->sync_sem)
		mem_free(handle->sync_sem);

	mem_free(handle);
	return res;
}

bool stream_check_finished(io_stream_t handle)
{
	bool brw = false;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		goto exit;
	}

	if ((handle->write_finished) && (stream_get_length(handle) <= 0)) {
		brw = true;
	}

exit:
	return brw;
}


int stream_get_length(io_stream_t handle)
{
	int brw = -ENOSYS;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		goto exit;
	}

	if (handle->ops->get_length) {
		brw = handle->ops->get_length(handle);
	} else {
		brw = handle->wofs - handle->rofs;
	}

exit:
	return brw;
}

int stream_get_space(io_stream_t handle)
{
	int brw = -ENOSYS;
	int attached_space = 0;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		goto exit;
	}

	if (handle->ops->get_space) {
		brw = handle->ops->get_space(handle);
	} else {
		brw = stream_get_length(handle);
		if (brw >= 0) {
			brw = handle->total_size - stream_get_length(handle);
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_lock(&handle->attach_lock, OS_FOREVER);
	}
	/**data write to attached stream */
	for (int i = 0; i < ARRAY_SIZE(handle->attach_stream); i++) {
		if (handle->attach_mode[i] != MODE_OUT)
			continue;

		if (!handle->attach_stream[i])
			continue;

		attached_space = stream_get_space(handle->attach_stream[i]);
		if (brw > attached_space) {
			brw = attached_space;
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_unlock(&handle->attach_lock);
	}
exit:
	return brw;
}

int stream_set_observer(io_stream_t handle, void * observer, stream_observer_notify notify, uint8_t type)
{
	int i;

	if (!_stream_check_handle_state(handle,STATE_OPEN)) {
		return -ENOSYS;
	}

	for (i = 0; i < ARRAY_SIZE(handle->observer); i++) {
		if (!handle->observer_notify[i]) {
			handle->observer[i] = observer;
			handle->observer_type[i] = type;
			handle->observer_notify[i] = notify;
			return 0;
		}
	}

	return -EBUSY;
}

int stream_attach(io_stream_t origin, io_stream_t attach_stream, int attach_type)
{
	int brw = -ENOSYS;
	int i;

	if (!(attach_stream->mode & MODE_OUT))
		return -EINVAL;

	if (!_stream_check_handle_state(origin,STATE_OPEN)) {
		goto exit;
	}

	if (!(origin->mode & attach_type)) {
			SYS_LOG_ERR("mode  %d not match %d \n", origin->mode, attach_type);
			return -EINVAL;
	}

	if (!os_is_in_isr()) {
		os_mutex_lock(&origin->attach_lock, OS_FOREVER);
	}

	for (i = 0; i < ARRAY_SIZE(origin->attach_stream); i++) {
		if (!origin->attach_stream[i]) {
			origin->attach_stream[i] = attach_stream;
			origin->attach_mode[i] = attach_type;
			attach_stream->attached_stream = origin;
			SYS_LOG_INF("origin %p , attach %p mode %d \n",origin,attach_stream,attach_type);
			brw = 0;
			break;
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_unlock(&origin->attach_lock);
	}

exit:
	return brw;
}


int stream_detach(io_stream_t origin, io_stream_t detach_stream)
{
	int brw = -ENOSYS;
	int i;

	if (!_stream_check_handle_state(origin, STATE_OPEN)) {
		detach_stream->attached_stream = NULL;
		return -ENOSYS;
	}

	if (!os_is_in_isr()) {
		os_mutex_lock(&origin->attach_lock, OS_FOREVER);
	}

	for (i = 0; i < ARRAY_SIZE(origin->attach_stream); i++) {
		if (origin->attach_stream[i] == detach_stream) {
			origin->attach_stream[i] = NULL;
			origin->attach_mode[i] = 0;
			detach_stream->attached_stream = NULL;
			SYS_LOG_INF("origin %p , detach %p\n",origin,detach_stream);
			brw = 0;
			break;
		}
	}

	if (!os_is_in_isr()) {
		os_mutex_unlock(&origin->attach_lock);
	}

	return brw;
}

void *stream_get_ringbuffer(io_stream_t handle)
{
	void *buf = NULL;

	if (handle == NULL) {
		return buf;
	}

	if (handle->ops && handle->ops->get_ringbuffer) {
		buf = handle->ops->get_ringbuffer(handle);
	}

	return buf;
}

void stream_dump(io_stream_t stream, const char *name, const char *line_prefix)
{
	os_printk("%s%s (t:%d,s:%d): rofs=0x%x, wofs=0x%x, size=0x%x, length=0x%x, space=0x%x\n",
		line_prefix, name, stream->type, stream->state, stream->rofs, stream->wofs,
		stream->total_size, stream_get_length(stream), stream_get_space(stream));
}
