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
	short x_coor_change[MAX_CACHE_KEY_NUM];
	short y_coor_change[MAX_CACHE_KEY_NUM];
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

	if (x_coor_change[0] * x_coor_change[1] * x_coor_change[2] * x_coor_change[3] > 0) {
		x_coor_change[4] = (x_coor_change[0] * COOR_COEF0 + x_coor_change[1] * COOR_COEF1
							+ x_coor_change[2] * COOR_COEF2 + x_coor_change[3] * COOR_COEF3) / COOR_COEF_TOTAL;
#ifdef CONFIG_DEBUG_TP
		printk("x1 %d x1 %d x1 %d x1 %d COOR_COEF_TOTAL %d \n",x_coor_change[0], x_coor_change[1], x_coor_change[2], x_coor_change[3],COOR_COEF_TOTAL);
#endif
	} else {
		x_coor_change[4] = x_coor_change[3];
	}

	if (y_coor_change[0] * y_coor_change[1] * y_coor_change[2] * y_coor_change[3] > 0) {
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
	if (!key_context.fixed_gesture) {
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

	if (!value->point.pessure_value) {
		key_context.gesture = 0;
		key_context.key_num = 0;
		key_context.key_index = 0;
		key_context.fixed_gesture = 0;
	} else {
		if (key_context.gesture != value->point.gesture) {
			key_context.gesture = value->point.gesture;
			key_context.key_index = 0;
			key_context.key_num = 0;
		} else {
			key_context.key_index++;
			if (key_context.key_index >= MAX_CACHE_KEY_NUM) {
				key_context.key_index = 0;
			}
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
			,value->point.gesture,value->point.pessure_value,value->point.loc_x, value->point.loc_y,
			key_context.key_index, key_context.key_num,
			k_cyc_to_us_floor32(timestamp - key_context.last_timestamp));
#endif
	key_context.tp_point_duration = k_cyc_to_us_floor32(timestamp - key_context.last_timestamp);
	key_context.next_timestamp = timestamp - key_context.last_timestamp;
	key_context.last_timestamp = timestamp;
	key_context.next_timestamp += key_context.last_timestamp;

	if (key_context.gesture && key_context.key_num == MAX_CACHE_KEY_NUM) {
		_tpkey_forecast_next_point();
	}

	return 0;
}

int tpkey_get_last_point(struct input_value *value, uint32_t timestamp)
{
	struct input_value *current_value = NULL;
	struct input_value *next_value = NULL;
	uint32_t current_duration = k_cyc_to_us_floor32(timestamp - key_context.last_timestamp) + 6000;

	if (current_duration > key_context.tp_point_duration) {
		current_duration = key_context.tp_point_duration;
	}
#ifdef CONFIG_DEBUG_TP
	printk("current_duration %d \n",current_duration);
#endif
	if (k_cyc_to_us_floor32(timestamp - key_context.last_timestamp) > 20000) {
		return 0;
	}

	if (!key_context.gesture) {
		current_value = &key_context.key_value[0];
		memcpy(value, current_value, sizeof(struct input_value));
	} else {
		current_value = &key_context.key_value[key_context.key_index];
		next_value = &key_context.next_point;
		if (key_context.key_num == MAX_CACHE_KEY_NUM) {
			value->point.loc_x = (next_value->point.loc_x * current_duration
									+ current_value->point.loc_x * (key_context.tp_point_duration - current_duration))
									/ key_context.tp_point_duration;

			value->point.loc_y = (next_value->point.loc_y * current_duration
									+ current_value->point.loc_y * (key_context.tp_point_duration - current_duration))
									/ key_context.tp_point_duration;

			value->point.gesture = current_value->point.gesture;
			value->point.pessure_value = current_value->point.pessure_value;

		} else {
			memcpy(value, current_value, sizeof(struct input_value));
		}
#ifdef CONFIG_DEBUG_TP
		printk("current(%d %d ) next(%d %d ) report(%d %d ) pessure_value %d\n",
				current_value->point.loc_x,current_value->point.loc_y,
				next_value->point.loc_x,next_value->point.loc_y,
				value->point.loc_x,value->point.loc_y,value->point.pessure_value);
#endif
	}

	return 0;

}