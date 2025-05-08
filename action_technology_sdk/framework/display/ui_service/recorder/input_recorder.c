/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <errno.h>
#include <os_common_api.h>
#include <input_recorder.h>

LOG_MODULE_REGISTER(ui_inputrec, LOG_LEVEL_INF);

#ifdef CONFIG_UI_INPUT_RECORDER

struct input_recorder {
	uint8_t first : 1;
	uint8_t in_prog : 1;
	uint8_t in_capture : 1;
	uint8_t in_playback : 1;

	uint16_t repeat_delay; /* repeat delay */
	uint32_t count; /* record count */
	uint32_t abs_timestamp; /* absolute timestamp */
	input_dev_data_t prev;
	input_rec_data_t curr;

	input_data_read_t read_fn;
	input_data_write_t write_fn;
	input_data_rewind_t rewind_fn;

	void *user_data;
};

static struct input_recorder recorder;

int input_capture_start(input_data_write_t write_fn, void *user_data)
{
	if (write_fn == NULL) {
		return -EINVAL;
	}

	if (recorder.in_capture || recorder.in_playback) {
		return -EBUSY;
	}

	recorder.first = 1;
	recorder.in_prog = 1;
	recorder.in_capture = 1;
	recorder.count = 0;
	recorder.abs_timestamp = 0;
	recorder.write_fn = write_fn;
	recorder.user_data = user_data;

	SYS_LOG_INF("started\n");
	return 0;
}

int input_capture_stop(void)
{
	if (recorder.in_capture) {
		recorder.in_capture = 0;

		SYS_LOG_INF("stopped\n");
		return (int)recorder.count;
	}

	return -ESRCH;
}

bool input_capture_is_running(void)
{
	return recorder.in_capture && recorder.in_prog;
}

int input_capture_write(const input_dev_data_t * data)
{
	uint32_t uptime = os_uptime_get_32();
	input_rec_data_t *curr = &recorder.curr;

	if (!input_capture_is_running()) {
		return -ESRCH;
	}

	if (recorder.first) {
		/* wait until the press */
		if (data->state == INPUT_DEV_STATE_REL) {
			return 0;
		}

		recorder.first = 0;
	} else {
		/* ignore the same event */
		if (curr->data.point.x == data->point.x &&
			curr->data.point.y == data->point.y &&
			curr->data.state == data->state &&
			curr->data.gesture_dir == data->gesture_dir &&
			curr->data.scroll_dir == data->scroll_dir) {
			return 0;
		}
	}

	memcpy(&curr->data, data, sizeof(*data));
	curr->timestamp = (recorder.abs_timestamp > 0) ? (uptime - recorder.abs_timestamp) : 0;

	if (recorder.write_fn(curr, recorder.user_data)) {
		SYS_LOG_WRN("full\n");
		recorder.in_prog = 0;
		return -EIO;
	}

	recorder.count++;
	recorder.abs_timestamp = uptime;
	return 0;
}

int input_playback_start(input_data_read_t read_fn, input_data_rewind_t rewind_fn,
		uint16_t repeat_delay, void *user_data)
{
	if (read_fn == NULL) {
		return -EINVAL;
	}

	if (recorder.in_capture || recorder.in_playback) {
		return -EBUSY;
	}

	if (read_fn(&recorder.curr, user_data)) { /* read the first record */
		return -EIO;
	}

	recorder.curr.timestamp = 0; /* should be 0 */

	recorder.first = 1;
	recorder.in_prog = 1;
	recorder.in_playback = 1;
	recorder.repeat_delay = repeat_delay;
	recorder.count = 1;
	recorder.abs_timestamp = 0;
	recorder.read_fn = read_fn;
	recorder.rewind_fn = rewind_fn;
	recorder.user_data = user_data;

	SYS_LOG_INF("started\n");
	return 0;
}

int input_playback_stop(void)
{
	if (recorder.in_playback) {
		recorder.in_playback = 0;

		SYS_LOG_INF("stopped\n");
		return (int)recorder.count;
	}

	return -ESRCH;
}

bool input_playback_is_running(void)
{
	return recorder.in_playback && recorder.in_prog;
}

int input_playback_read(input_dev_data_t * data)
{
	uint32_t uptime = os_uptime_get_32();
	input_rec_data_t *curr = &recorder.curr;
	input_dev_data_t *prev = &recorder.prev;
	int res = -ENODATA;

	if (!input_playback_is_running()) {
		return -ESRCH;
	}

	if (uptime - recorder.abs_timestamp >= curr->timestamp) {
		memcpy(prev, &curr->data, sizeof(*prev));

		res = recorder.read_fn(curr, recorder.user_data);
		if (res && recorder.rewind_fn) { /* reach the end, then rewind and try again */
			recorder.rewind_fn(recorder.user_data);
			res = recorder.read_fn(curr, recorder.user_data);
			if (res == 0) {
				curr->timestamp = recorder.repeat_delay; /* original value should be 0 */
			}
		}

		if (res) {
			SYS_LOG_WRN("empty\n");
			recorder.in_prog = 0;
		} else {
			recorder.count++;
			recorder.abs_timestamp = uptime;
		}
	}

	*data = *prev;
	return 0;
}

#else /* CONFIG_UI_INPUT_RECORDER */

bool input_playback_is_running(void) { return false; }

int input_playback_read(input_dev_data_t * data) { return -ESRCH; }

bool input_capture_is_running(void) { return false; }

int input_capture_write(const input_dev_data_t * data) { return -ESRCH; }

#endif /* CONFIG_UI_INPUT_RECORDER */
