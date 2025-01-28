/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <utils/acts_ringbuf.h>
#include <ui_service.h>
#include <lvgl/lvgl.h>
#include "lvgl_input_dev.h"

#define INPUT_POINTER_MSGQ_COUNT 8

static lv_indev_data_t pointer_scan_rbuf_data[INPUT_POINTER_MSGQ_COUNT];
static ACTS_RINGBUF_DEFINE(pointer_scan_rbuf, pointer_scan_rbuf_data, sizeof(pointer_scan_rbuf_data));

static int _lvgl_pointer_put(const input_dev_data_t *data, void *_indev)
{
	static uint8_t put_cnt;
	static uint8_t drop_cnt;
	static lv_indev_state_t prev_state = LV_INDEV_STATE_RELEASED;

	lv_indev_t *indev = _indev;
	lv_indev_data_t indev_data = {
		.point.x = data->point.x,
		.point.y = data->point.y,
		.state = data->state,
	};
	int ret = 0;

	if (lv_indev_get_display(indev) == NULL) {
		return 0;
	}

	/* drop redundant released point */
	if (indev_data.state == LV_INDEV_STATE_RELEASED && prev_state == LV_INDEV_STATE_RELEASED) {
		return 0;
	}

	if (acts_ringbuf_put(&pointer_scan_rbuf, &indev_data, sizeof(indev_data)) != sizeof(indev_data)) {
		drop_cnt++;
		ret = -ENOBUFS;
	} else {
		prev_state = indev_data.state;
	}

	if (++put_cnt == 0) { /* about 4s for LCD refresh-rate 60 Hz */
		if (drop_cnt > 0) {
			SYS_LOG_WRN("%d tp input dropped\n", drop_cnt);
			drop_cnt = 0;
		}
	}

	return ret;
}

static void _lvgl_pointer_read_cb(lv_indev_t * indev, lv_indev_data_t *data)
{
	static lv_indev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = LV_INDEV_STATE_RELEASED,
	};

	lv_indev_data_t curr;

	if (acts_ringbuf_get(&pointer_scan_rbuf, &curr, sizeof(curr)) == sizeof(curr)) {
		prev = curr;
	}

	*data = prev;
	data->continue_reading = (acts_ringbuf_is_empty(&pointer_scan_rbuf) == 0);
}

static void _lvgl_indev_enable(bool en, void *indev)
{
	lv_indev_enable((lv_indev_t *)indev, en);
}

static void _lvgl_indev_wait_release(void *indev)
{
	lv_indev_wait_release((lv_indev_t *)indev);
}

static const ui_input_gui_callback_t pointer_callback = {
	.enable = _lvgl_indev_enable,
	.wait_release = _lvgl_indev_wait_release,
	.put_data = _lvgl_pointer_put,
};

int lvgl_input_pointer_init(void)
{
	lv_indev_t *indev = lv_indev_create();
	if (indev == NULL) {
		LV_LOG_ERROR("Failed to create input pointer.");
		return -ENOMEM;
	}

	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, _lvgl_pointer_read_cb);

	/*
	 * The indev read_timer may run after the display refr_timer because the
	 * timers run eariler if inserted later.
	 *
	 * So pause the indev timer, and invoke it manually.
	 */
	lv_timer_pause(lv_indev_get_read_timer(indev));

	if (ui_service_register_gui_input_callback(INPUT_DEV_TYPE_POINTER, &pointer_callback, indev)) {
		SYS_LOG_ERR("Failed to register input device to ui service.");
		return -EPERM;
	}

	return 0;
}
