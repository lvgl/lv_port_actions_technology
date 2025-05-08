/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <os_common_api.h>
#include <input_recorder.h>
#include <stream.h>

static int _input_data_stream_read(input_rec_data_t *data, void *stream)
{
	return stream_read(stream, (void *)data, sizeof(*data)) != sizeof(*data);
}

static int _input_data_stream_write(const input_rec_data_t *data, void *stream)
{
	return stream_write(stream, (void *)data, sizeof(*data)) != sizeof(*data);
}

static void _input_data_stream_rewind(void *stream)
{
	stream_seek(stream, 0, SEEK_DIR_BEG);
}

int input_capture_stream_start(io_stream_t stream)
{
	return input_capture_start(_input_data_stream_write, stream);
}

int input_playback_stream_start(io_stream_t stream, bool repeat)
{
	return input_playback_start(_input_data_stream_read,
			repeat ? _input_data_stream_rewind : NULL,
			INPUTREC_DEFAULT_REPEAT_DELAY, stream);
}
