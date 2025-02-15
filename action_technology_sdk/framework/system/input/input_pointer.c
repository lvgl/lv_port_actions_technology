/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input manager interface
 */


#include <os_common_api.h>
#include <string.h>
#include <key_hal.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <input_manager.h>
#include <msg_manager.h>
#include <sys_event.h>
#include <property_manager.h>
#include <input_manager_inner.h>
#include <sys_wakelock.h>

#define CONFIG_READ_DIRCT 1

/* Drag threshold in pixels */
#define POINTER_SCROLL_LIMIT    30

/* Gesture threshold in pixels */
#define POINTER_GESTURE_LIMIT    50

/* Gesture min velocity at release before swipe (pixels) */
#define POINTER_GESTURE_MIN_VELOCITY  3

#ifndef ABS
# define ABS(x) (((x) >= 0) ? (x) : -(x))
#endif

#ifdef CONFIG_INPUT_DEV_ACTS_TP_KEY
#ifndef CONFIG_READ_DIRCT
K_MSGQ_DEFINE(pointer_scan_msgq, sizeof(input_dev_data_t), 10, 4);
#endif

static uint32_t s_supported_gestures;

static void pointer_gesture_detect(input_drv_t *drv, input_dev_data_t *data)
{
	static point_t scroll_sum;
	static point_t gesture_sum;
	static input_dev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = INPUT_DEV_STATE_REL,
		.gesture_dir = 0,
		.scroll_dir = 0,
	};
	point_t vec;
	uint8_t gesture_dir = 0;
	uint8_t scroll_dir = 0;

	if (prev.state == INPUT_DEV_STATE_REL)
		goto out_exit;

	if (data->state == INPUT_DEV_STATE_PR) {
		vec.x = data->point.x - prev.point.x;
		vec.y = data->point.y - prev.point.y;

		scroll_sum.x += vec.x;
		scroll_sum.y += vec.y;
		if (ABS(scroll_sum.x) > drv->scroll_limit ||
			ABS(scroll_sum.y) > drv->scroll_limit) {
			if (ABS(scroll_sum.x) >= ABS(scroll_sum.y)) {
				scroll_dir = (scroll_sum.x > 0) ? GESTURE_DROP_RIGHT : GESTURE_DROP_LEFT;
			} else {
				scroll_dir = (scroll_sum.y > 0) ? GESTURE_DROP_DOWN : GESTURE_DROP_UP;
			}
		}

		if (ABS(vec.x) < drv->gesture_min_velocity &&
			ABS(vec.y) < drv->gesture_min_velocity) {
			gesture_sum.x = 0;
			gesture_sum.y = 0;
		}

		gesture_sum.x += vec.x;
		gesture_sum.y += vec.y;

		if (ABS(gesture_sum.x) > drv->gesture_limit ||
			ABS(gesture_sum.y) > drv->gesture_limit) {
			if (ABS(gesture_sum.x) >= ABS(gesture_sum.y)) {
				gesture_dir = (gesture_sum.x > 0) ? GESTURE_DROP_RIGHT : GESTURE_DROP_LEFT;
			} else {
				gesture_dir = (gesture_sum.y > 0) ? GESTURE_DROP_DOWN : GESTURE_DROP_UP;
			}
		}
	} else if (data->state == INPUT_DEV_STATE_REL) {
		gesture_sum.x = 0;
		gesture_sum.y = 0;
		scroll_sum.x = 0;
		scroll_sum.y = 0;
	}

out_exit:
	data->gesture_dir = gesture_dir;
	data->scroll_dir = scroll_dir;
	prev = *data;
}

static void pointer_scan_callback(struct device *dev, struct input_value *val)
{
#ifndef CONFIG_READ_DIRCT
	input_dev_data_t data = {
		.point.x = val->point.loc_x,
		.point.y = val->point.loc_y,
		.state = (val->point.pessure_value == 1)? INPUT_DEV_STATE_PR : INPUT_DEV_STATE_REL,
		.gesture  = val->point.gesture,
	};

	if (k_msgq_put(&pointer_scan_msgq, &data, K_NO_WAIT) != 0) {
		SYS_LOG_ERR("Could put input data into queue");
	}
#endif
}

static bool pointer_scan_read(input_drv_t *drv, input_dev_data_t *data)
{
#if CONFIG_READ_DIRCT
	static input_dev_state_t last_state = INPUT_DEV_STATE_REL;

	struct input_value val;

	memset(&val, 0, sizeof(struct input_value));

	input_dev_inquiry(drv->input_dev, &val);
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
	bool filter_state = false;
	if(drv->filter_cb)
		filter_state = drv->filter_cb((void *)&val);
#endif
	data->point.x = val.point.loc_x;
	data->point.y = val.point.loc_y;
	data->state = (val.point.pessure_value == 1)? INPUT_DEV_STATE_PR : INPUT_DEV_STATE_REL;
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
	if (s_supported_gestures == 0 || filter_state) {
#else
	if (s_supported_gestures == 0) {
#endif
		pointer_gesture_detect(drv, data);
	} else {
		data->gesture_dir = val.point.gesture;
		data->scroll_dir = val.point.gesture;
	}

	if (last_state != data->state) {
		input_manager_boost(data->state == INPUT_DEV_STATE_PR);
		last_state = data->state;

#ifdef CONFIG_SYS_WAKELOCK
		if (data->state == INPUT_DEV_STATE_PR) {
			sys_wake_lock(FULL_WAKE_LOCK);
		} else {
			sys_wake_unlock(FULL_WAKE_LOCK);
		}
#endif /* CONFIG_SYS_WAKELOCK */
	}

	return false;
#else
	static input_dev_data_t prev = {
		.point.x = 0,
		.point.y = 0,
		.state = INPUT_DEV_STATE_REL,
		.gesture = 0,
	};

	input_dev_data_t curr;

	if (k_msgq_get(&pointer_scan_msgq, &curr, K_NO_WAIT) == 0) {
		prev = curr;
	}

	*data = prev;
	if (s_supported_gestures == 0) {
		pointer_gesture_detect(data);
	}

	return k_msgq_num_used_get(&pointer_scan_msgq) > 0;
#endif
}

static void pointer_scan_enable(input_drv_t *drv, bool enable)
{
	struct device *input_dev = (struct device *)device_get_binding("tpkey");

	if (input_dev) {
		if (enable)
			input_dev_enable(input_dev);
		else
			input_dev_disable(input_dev);
	}
}

#endif /* CONFIG_INPUT_DEV_ACTS_TP_KEY */

static bool pointer_scan_read_fake(input_drv_t *drv, input_dev_data_t *data)
{
	*data = (input_dev_data_t) {
		.point.x = 0,
		.point.y = 0,
		.state = INPUT_DEV_STATE_REL,
		.gesture_dir = 0,
		.scroll_dir = 0,
	};

	return false;
}

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
void input_pointer_register_filter_cb(bool (*filter_cb)(void *val))
{
	input_dev_t *input_drv = input_manager_get_input_dev(INPUT_DEV_TYPE_POINTER);
    if(input_drv)
		input_drv->driver.filter_cb = filter_cb;
}
#endif

int input_pointer_device_init(void)
{
	input_drv_t input_drv;

	memset(&input_drv, 0, sizeof(input_drv));
	input_drv.type = INPUT_DEV_TYPE_POINTER;
	input_drv.scroll_limit = POINTER_SCROLL_LIMIT;
	input_drv.gesture_limit = POINTER_GESTURE_LIMIT;
	input_drv.gesture_min_velocity = POINTER_GESTURE_MIN_VELOCITY;

#ifdef CONFIG_INPUT_DEV_ACTS_TP_KEY
	input_drv.input_dev = (struct device *)device_get_binding("tpkey");
	if (input_drv.input_dev) {
		struct input_capabilities cap;

		memset(&cap, 0, sizeof(cap));
		input_dev_get_capabilities(input_drv.input_dev, &cap);
		s_supported_gestures = cap.pointer.supported_gestures;

		input_dev_register_notify(input_drv.input_dev, pointer_scan_callback);
		input_drv.read_cb = pointer_scan_read;
		input_drv.enable = pointer_scan_enable;
	}
#endif /*CONFIG_INPUT_DEV_ACTS_TP_KEY*/

	if (input_drv.input_dev == NULL) {
		os_printk("cannot found tpkey dev, fake one\n");
		input_drv.read_cb = pointer_scan_read_fake;
	}

	input_driver_register(&input_drv);

	SYS_LOG_INF("init ok");
	return 0;
}
