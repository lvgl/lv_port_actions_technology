/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <drivers/adc.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <board.h>
#include <soc_pmu.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <string.h>
#include <drivers/i2c.h>
#include <board_cfg.h>
#include <stdlib.h>

#ifdef CONFIG_PANEL_OFFSET_X
#  define CONFIG_TP_LOC_X_MIN CONFIG_PANEL_OFFSET_X
#else
#  define CONFIG_TP_LOC_X_MIN (0)
#endif

#ifdef CONFIG_PANEL_OFFSET_Y
#  define CONFIG_TP_LOC_Y_MIN CONFIG_PANEL_OFFSET_Y
#else
#  define CONFIG_TP_LOC_Y_MIN (0)
#endif

#ifdef CONFIG_PANEL_HOR_RES
#  define CONFIG_TP_LOC_X_MAX (CONFIG_PANEL_HOR_RES - 1)
#elif defined(CONFIG_PANEL_TIMING_HACTIVE)
#  define CONFIG_TP_LOC_X_MAX (CONFIG_PANEL_TIMING_HACTIVE - 1)
#else
#  define CONFIG_TP_LOC_X_MAX (512 - 1)
#endif

#ifdef CONFIG_PANEL_VER_RES
#  define CONFIG_TP_LOC_Y_MAX (CONFIG_PANEL_VER_RES - 1)
#elif defined(CONFIG_PANEL_TIMING_VACTIVE)
#  define CONFIG_TP_LOC_Y_MAX (CONFIG_PANEL_TIMING_VACTIVE - 1)
#else
#  define CONFIG_TP_LOC_Y_MAX (512 - 1)
#endif

#define CONFIG_TP_FORECAST_LIMIT 2

#define TP_ABS(x) (((x) >= 0) ? (x) : -(x))

//#define CONFIG_DEBUG_TP 1

#define MAX_CACHE_KEY_NUM 5

#define COOR_COEF0  1
#define COOR_COEF1  2
#define COOR_COEF2  3
#define COOR_COEF3  10

/** key action type */
enum GESTURE_TYPE
{
	/** gesture drop down */
	GESTURE_DROP_DOWN  = 1,
	/** gesture drop up */
	GESTURE_DROP_UP,
	/** gesture drop left */
	GESTURE_DROP_LEFT,
	/** gesture drop right */
	GESTURE_DROP_RIGHT,
};

#define COOR_COEF_TOTAL  (COOR_COEF0 + COOR_COEF1 + COOR_COEF2 + COOR_COEF3)

struct acts_tpkey_context {
	struct input_value key_value[MAX_CACHE_KEY_NUM];
	struct input_value next_point;
	uint32_t tp_point_duration;
	uint32_t last_timestamp;
	uint32_t next_timestamp;
	uint16_t last_pessure_value[MAX_CACHE_KEY_NUM];
	uint8_t key_index;
	uint8_t key_num;
	uint8_t gesture;
	uint8_t fixed_gesture;
	uint8_t gesture_speed;
};

static struct acts_tpkey_context key_context;

static int gesture_fixed(int gesture, int scroll_off_x, int scroll_off_y)
{
	int fixed_gesture = gesture;

	static const uint8_t fixed_gesture_table[] = {
		 0, GESTURE_DROP_UP, GESTURE_DROP_DOWN,
		GESTURE_DROP_RIGHT, GESTURE_DROP_LEFT,
	};

	if ((gesture == GESTURE_DROP_DOWN && scroll_off_y < -2)
		|| (gesture == GESTURE_DROP_UP && scroll_off_y > 2)) {
		fixed_gesture = fixed_gesture_table[gesture];
	}

	if ((gesture == GESTURE_DROP_LEFT && scroll_off_x > 2)
		|| (gesture == GESTURE_DROP_RIGHT && scroll_off_x < -2)) {
		fixed_gesture = fixed_gesture_table[gesture];
	}

	key_context.fixed_gesture = fixed_gesture;

	return fixed_gesture;
}

static int _tpkey_forecast_next_point(void)
{
	uint8_t start_index = (key_context.key_index + 1) % MAX_CACHE_KEY_NUM;
	int16_t x_coor_change[MAX_CACHE_KEY_NUM] = {0};
	int16_t y_coor_change[MAX_CACHE_KEY_NUM] = {0};
	//uint16_t x_coor = 0;
	//uint16_t y_coor = 0;
	int i = 0;

	for (i = 0 ; i < key_context.key_num; i++) {
		uint8_t current_index = (start_index + i) % MAX_CACHE_KEY_NUM;
		uint8_t next_index = (start_index + i + 1) % MAX_CACHE_KEY_NUM;
		if (i == 4) {
			x_coor_change[i] = key_context.key_value[current_index].point.loc_x - key_context.key_value[next_index].point.loc_x;
			y_coor_change[i] = key_context.key_value[current_index].point.loc_y - key_context.key_value[next_index].point.loc_y;
		} else {
			x_coor_change[i] = key_context.key_value[next_index].point.loc_x - key_context.key_value[current_index].point.loc_x;
			y_coor_change[i] = key_context.key_value[next_index].point.loc_y - key_context.key_value[current_index].point.loc_y;
		}
	}

	if ((int32_t)x_coor_change[0] * x_coor_change[1] * x_coor_change[2] * x_coor_change[3] > 0) {
		x_coor_change[4] = (x_coor_change[0] * COOR_COEF0 + x_coor_change[1] * COOR_COEF1
							+ x_coor_change[2] * COOR_COEF2 + x_coor_change[3] * COOR_COEF3) / COOR_COEF_TOTAL;
#ifdef CONFIG_DEBUG_TP
		printk("x1 %d x1 %d x1 %d x1 %d COOR_COEF_TOTAL %d \n",x_coor_change[0], x_coor_change[1], x_coor_change[2], x_coor_change[3],COOR_COEF_TOTAL);
#endif
	} else {
		x_coor_change[4] = x_coor_change[3];
	}

	if ((int32_t)y_coor_change[0] * y_coor_change[1] * y_coor_change[2] * y_coor_change[3] > 0) {
		y_coor_change[4] = (y_coor_change[0] * COOR_COEF0 + y_coor_change[1] * COOR_COEF1
							+ y_coor_change[2] * COOR_COEF2 + y_coor_change[3] * COOR_COEF3) / COOR_COEF_TOTAL;

		y_coor_change[4] = (y_coor_change[0] * COOR_COEF0 + y_coor_change[1] * COOR_COEF1
							+ y_coor_change[2] * COOR_COEF2 + y_coor_change[3] * COOR_COEF3) / COOR_COEF_TOTAL;
#ifdef CONFIG_DEBUG_TP
		printk("y1 %d y2 %d y3 %d y4 %d COOR_COEF_TOTAL %d \n",y_coor_change[0], y_coor_change[1],y_coor_change[2], y_coor_change[3],COOR_COEF_TOTAL);
#endif
	} else {
		y_coor_change[4] = y_coor_change[3];
	}

#ifdef CONFIG_DEBUG_TP
	printk("gesture %d xcoor %d ycoor %d \n",key_context.key_value[key_context.key_index].point.gesture,abs(x_coor_change[4]), abs(y_coor_change[4]));
#endif

	if (key_context.gesture && !key_context.fixed_gesture) {
		if (abs(x_coor_change[4]) > abs(y_coor_change[4])
			|| key_context.key_value[key_context.key_index].point.loc_x < 50) {
			if (x_coor_change[4] > 0) {
				key_context.fixed_gesture = 4;
			} else {
				key_context.fixed_gesture = 3;
			}
		} else {
			key_context.fixed_gesture = 0;
		}
	}

#ifdef CONFIG_DEBUG_TP
	printk("fixed_gesture %d\n",key_context.fixed_gesture);
#endif

	if (key_context.fixed_gesture) {
		key_context.key_value[key_context.key_index].point.gesture = key_context.fixed_gesture;
	}

	key_context.next_point.point.loc_x = key_context.key_value[key_context.key_index].point.loc_x + x_coor_change[4];
	key_context.next_point.point.loc_y = key_context.key_value[key_context.key_index].point.loc_y + y_coor_change[4];
	key_context.next_point.point.gesture = gesture_fixed(key_context.key_value[key_context.key_index].point.gesture, x_coor_change[3], y_coor_change[3]);

#ifdef CONFIG_DEBUG_TP
	printk(" x :");
	for (i = 0 ; i < key_context.key_num; i++) {
		printk("  %d ",x_coor_change[i]);
	}
	printk("\n");

	printk(" y :");
	for (i = 0 ; i < key_context.key_num; i++) {
		printk("  %d ",y_coor_change[i]);
	}
	printk("\n");
#endif
	return 0;
}

int tpkey_put_point(struct input_value *value, uint32_t timestamp)
{
	struct input_value *current_value = NULL;
	uint8_t prev_index = key_context.key_index;

	if (!value->point.pessure_value) {
		key_context.gesture = 0;
		key_context.key_num = 0;
		key_context.key_index = 0;
		key_context.fixed_gesture = 0;
	} else {
#ifdef CONFIG_INPUT_DEV_ACTS_CST820_TP_KEY
		if (key_context.gesture == 0)
#endif
		key_context.gesture = value->point.gesture;
		if (key_context.gesture == 0)
			key_context.fixed_gesture = 0;

		key_context.key_index++;
		if (key_context.key_index >= MAX_CACHE_KEY_NUM) {
			key_context.key_index = 0;
		}
	}

	current_value = &key_context.key_value[key_context.key_index];
	memcpy(current_value, value, sizeof(struct input_value));
	key_context.key_num++;

	if (key_context.key_num >= MAX_CACHE_KEY_NUM) {
		key_context.key_num = MAX_CACHE_KEY_NUM;
	}

#ifdef CONFIG_DEBUG_TP
	printk("put gesture = %d,pessure_value = %d local:(%d,%d),index %d num %d duration %d\n"
			,key_context.gesture,value->point.pessure_value,value->point.loc_x, value->point.loc_y,
			key_context.key_index, key_context.key_num,
			k_cyc_to_us_floor32(timestamp - key_context.last_timestamp));
#endif

	key_context.tp_point_duration = k_cyc_to_us_floor32(timestamp - key_context.last_timestamp);
	key_context.next_timestamp = timestamp - key_context.last_timestamp;
	key_context.last_timestamp = timestamp;
	key_context.next_timestamp += key_context.last_timestamp;

	if (/*key_context.gesture && */key_context.key_num == MAX_CACHE_KEY_NUM) {
		struct input_value *prev_value = &key_context.key_value[prev_index];
		int diff_x = TP_ABS((int)current_value->point.loc_x - prev_value->point.loc_x);
		int diff_y = TP_ABS((int)current_value->point.loc_y - prev_value->point.loc_y);

		if (diff_x < CONFIG_TP_FORECAST_LIMIT && diff_y < CONFIG_TP_FORECAST_LIMIT) {
			memcpy(&key_context.next_point, current_value, sizeof(*current_value));
		} else {
			_tpkey_forecast_next_point();
		}
	}

	if(!key_context.last_pessure_value[prev_index] && value->point.pessure_value)
		memcpy(&key_context.next_point, current_value, sizeof(*current_value));
	key_context.last_pessure_value[key_context.key_index] = value->point.pessure_value;

	return 0;
}

#if defined(CONFIG_INPUT_DEV_ACTS_TEST_FPS_FAKE_TP)

typedef struct fps_fake_tp_t {
	int32_t all_move;
	int32_t at_move;
	int16_t move_pix;
	int16_t at_pix;
	uint8_t to_hor : 1;
	uint8_t fake_tp_state : 1;
	uint8_t next_put_up : 1;
}fps_fake_tp;
static fps_fake_tp fake_tp = {0};

/**
 * Start fps fake tp
 * @param to_ver true hor , false ver.
 * @param all_move all move pix.
 * @param move_pix every move pix.
 */
void tpkey_start_fps_fake_tp(bool to_hor, int32_t all_move, int16_t move_pix)
{
	if(fake_tp.fake_tp_state) {
		printk("start_fps_fake_tp in run\r\n");
		return;
	}
	if(all_move == 0 || move_pix == 0) {
		printk("start_fps_fake_tp in param error\r\n");
		return;
	}
	memset(&fake_tp, 0, sizeof(fps_fake_tp));
	if(all_move < 0) {
		if(move_pix > 0)
			move_pix = -move_pix;
		fake_tp.at_pix = to_hor ? CONFIG_PANEL_HOR_RES - 1: CONFIG_PANEL_VER_RES - 1;
	} else {
		if(move_pix < 0)
			move_pix = -move_pix;
	}
	fake_tp.all_move = all_move;
	fake_tp.move_pix = move_pix;
	fake_tp.to_hor = to_hor;
	fake_tp.fake_tp_state = true;
}

static bool tpkey_fps_fake_tp_exe(struct input_value *value)
{
	if(fake_tp.fake_tp_state) {
		if(fake_tp.to_hor) {
			value->point.loc_x = fake_tp.at_pix;
			if(fake_tp.move_pix > 0)
				value->point.gesture = GESTURE_DROP_RIGHT;
			else
				value->point.gesture = GESTURE_DROP_LEFT;
		} else {
			value->point.loc_y = fake_tp.at_pix;
			if(fake_tp.move_pix > 0)
				value->point.gesture = GESTURE_DROP_DOWN;
			else
				value->point.gesture = GESTURE_DROP_UP;
		}
		if(fake_tp.next_put_up) {
			value->point.pessure_value = 0;
			fake_tp.next_put_up = false;
			return true;
		}
		value->point.pessure_value = 1;
		fake_tp.at_pix += fake_tp.move_pix;
		fake_tp.at_move += fake_tp.move_pix;
		if(abs(fake_tp.at_move) >= abs(fake_tp.all_move)) {
			fake_tp.move_pix = -fake_tp.move_pix;
			fake_tp.all_move = -fake_tp.all_move;
		}
		int16_t max_pix = fake_tp.to_hor ? CONFIG_PANEL_HOR_RES - 1 : CONFIG_PANEL_VER_RES - 1;
		if(fake_tp.at_pix < 0 || fake_tp.at_pix > max_pix) {
			if(fake_tp.all_move < 0)
				fake_tp.at_pix = max_pix;
			else
				fake_tp.at_pix = 0;
			fake_tp.next_put_up = true;
		}
		if(fake_tp.at_move == 0) {
			fake_tp.fake_tp_state = false;
			value->point.pessure_value = 0;
		}
		return true;
	}
	return false;
}

#endif

int tpkey_get_last_point(struct input_value *value, uint32_t timestamp)
{
	#if defined(CONFIG_INPUT_DEV_ACTS_TEST_FPS_FAKE_TP)
	if(tpkey_fps_fake_tp_exe(value))
		return 0;
	#endif

	struct input_value *current_value = NULL;
	struct input_value *next_value = NULL;

	/* FIXME: add 6ms to compensate the duration between this and the time show on the screen. */
	uint32_t current_duration = k_cyc_to_us_floor32(timestamp - key_context.last_timestamp) + 6000;

	if (current_duration > key_context.tp_point_duration) {
		current_duration = key_context.tp_point_duration;
	}

	current_value = &key_context.key_value[key_context.key_index];

	if (k_cyc_to_us_floor32(timestamp - key_context.last_timestamp) > 20000) {
		memcpy(value, current_value, sizeof(struct input_value));
		next_value = current_value;
		goto exit;
	}

	next_value = &key_context.next_point;
	if (key_context.key_num == MAX_CACHE_KEY_NUM) {
		int32_t next_duration = (int32_t)(key_context.tp_point_duration - current_duration);

		if (next_value->point.loc_x != current_value->point.loc_x) {
			value->point.loc_x = (next_value->point.loc_x * (int32_t)current_duration
						+ current_value->point.loc_x * next_duration)
					/ (int32_t)key_context.tp_point_duration;
		} else {
			value->point.loc_x = current_value->point.loc_x;
		}

		if (next_value->point.loc_y != current_value->point.loc_y) {
			value->point.loc_y = (next_value->point.loc_y * (int32_t)current_duration
						+ current_value->point.loc_y * next_duration)
					/ (int32_t)key_context.tp_point_duration;
		} else {
			value->point.loc_y = current_value->point.loc_y;
		}

		value->point.gesture = current_value->point.gesture;
		value->point.pessure_value = current_value->point.pessure_value;
	} else {
		memcpy(value, current_value, sizeof(struct input_value));
		if (!current_value->point.pessure_value) {

			int32_t next_duration = (int32_t)(key_context.tp_point_duration - current_duration);

			if (next_value->point.loc_x != current_value->point.loc_x) {
				value->point.loc_x = (next_value->point.loc_x * (int32_t)current_duration
							+ current_value->point.loc_x * next_duration)
						/ (int32_t)key_context.tp_point_duration;
			} else {
				value->point.loc_x = current_value->point.loc_x;
			}

			if (next_value->point.loc_y != current_value->point.loc_y) {
				value->point.loc_y = (next_value->point.loc_y * (int32_t)current_duration
							+ current_value->point.loc_y * next_duration)
						/ (int32_t)key_context.tp_point_duration;
			} else {
				value->point.loc_y = current_value->point.loc_y;
			}
		}
	}

exit:
	value->point.loc_x = MIN(MAX(value->point.loc_x - CONFIG_TP_LOC_X_MIN, 0), CONFIG_TP_LOC_X_MAX);
	value->point.loc_y = MIN(MAX(value->point.loc_y - CONFIG_TP_LOC_Y_MIN, 0), CONFIG_TP_LOC_Y_MAX);

#ifdef CONFIG_DEBUG_TP
	printk("duration %d, current(%d %d) next(%d %d) report(%d %d) pessure_value %d\n\n",
			current_duration, current_value->point.loc_x, current_value->point.loc_y,
			next_value->point.loc_x, next_value->point.loc_y,
			value->point.loc_x, value->point.loc_y, value->point.pessure_value);
#endif

	return 0;
}
