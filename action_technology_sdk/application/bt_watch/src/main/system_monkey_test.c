/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief system monkey test.
 */
#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <stdio.h>
#include <string.h>
#include <shell/shell.h>
#include <stdlib.h>
#include "system_app.h"
#include <drivers/input/input_dev.h>
#include <drivers/power_supply.h>
#include <random/rand32.h>
#include <soc.h>
#include <app_ui.h>

#ifdef CONFIG_DVFS
#include <dvfs.h>
//#define MONKEY_TEST_DVFS                  1
#ifdef MONKEY_TEST_DVFS
#define DVFS_LEVEL_CHANGE_INTERVAL      1000
const static uint8_t dvfs_level_table[] = {DVFS_LEVEL_S2, DVFS_LEVEL_NORMAL, DVFS_LEVEL_PERFORMANCE, DVFS_LEVEL_MID_PERFORMANCE, DVFS_LEVEL_HIGH_PERFORMANCE};
#endif
#endif

#define CONFIG_MONKEY_WORK_Q_STACK_SIZE 1280
#define INPUT_EVENT_INTERVAL 			10
#define KEY_EVENT_INTERVAL				60 /* the minimum report period is 60 ms in real case */
#define MAX_POINTER_TRAVEL 				10
#define STEP_LENGTH                     20

/** onoff key short up*/
#define DEFAULT_KEY_EVENT 				0x4000001

K_THREAD_STACK_DEFINE(monkey_work_q_stack, CONFIG_MONKEY_WORK_Q_STACK_SIZE);

enum MONKEY_INPUT_TYPE {
	MONKEY_KEY = 0,
	MONKEY_TP,
	MONKEY_REMIND,

	NUM_MONKEY_INPUTS,
};

struct monkey_input {
	uint8_t event : 7;
	uint8_t finished : 1;
	uint8_t step;
	uint32_t interval; /* number of INPUT_EVENT_INTERVAL or timestamp directly */
} monkey_input_t;

struct monkey_context_t {
	uint16_t monkey_start:1;
	uint16_t monkey_event_interval;
	int16_t x_res;
	int16_t y_res;
#ifdef MONKEY_TEST_DVFS
	uint16_t dvfs_level;
	uint16_t dvfs_level_change_interval;
#endif
	struct monkey_input inputs[NUM_MONKEY_INPUTS];
	struct k_work_q monkey_work_q;
	struct k_delayed_work monkey_work;
} monkey_context;

enum MONKEY_TYPE {
	KEY_EVENT = 0,
	DRAG_DOWN,
	DRAG_UP,
	DRAG_LEFT,
	DRAG_RIGHT,
	CLICK,
	REMIND_EVENT,

	NUM_MONKEY_TYPES,
};

struct tp_pointer_t {
	short x;
	short y;
	uint16_t pressed;
};

extern int tpkey_put_point(struct input_value *value, uint32_t timestamp);

static uint32_t _hardware_randomizer_gen_rand(void)
{
	uint32_t trng_low, trng_high;

	se_trng_init();
	se_trng_process(&trng_low, &trng_high);
	se_trng_deinit();

	return trng_low;
}

static int get_tp_pointer(struct tp_pointer_t *tp_pointer, uint16_t event, uint8_t *event_step)
{
	int ret = 0;
	int e_step = *event_step;
	switch (event) {
	case DRAG_DOWN: {
		tp_pointer->x = monkey_context.x_res / 2;
		tp_pointer->y = 0 + e_step * STEP_LENGTH;
		tp_pointer->pressed =  ((e_step == MAX_POINTER_TRAVEL) ? 0 : 1);
		e_step++;
		break;
	}
	case DRAG_UP: {
		tp_pointer->x = monkey_context.x_res / 2;
		tp_pointer->y = monkey_context.y_res - *event_step * STEP_LENGTH;
		tp_pointer->pressed =  ((e_step == MAX_POINTER_TRAVEL) ? 0 : 1);
		e_step++;
		break;
	}

	case DRAG_LEFT: {
		tp_pointer->x = monkey_context.x_res - e_step * STEP_LENGTH;
		tp_pointer->y = monkey_context.y_res / 2;
		tp_pointer->pressed =  ((e_step == MAX_POINTER_TRAVEL) ? 0 : 1);
		e_step++;
		break;
	}

	case DRAG_RIGHT: {
		tp_pointer->x = e_step * STEP_LENGTH;
		tp_pointer->y = monkey_context.y_res / 2;
		tp_pointer->pressed =  ((e_step == MAX_POINTER_TRAVEL) ? 0 : 1);
		e_step++;
		break;
	}

	case CLICK: {
		tp_pointer->x = _hardware_randomizer_gen_rand() % monkey_context.x_res;
		tp_pointer->y = _hardware_randomizer_gen_rand() % monkey_context.y_res;
		tp_pointer->pressed =  ((*event_step == 1) ? 1 : 0);
		e_step++;
		if(e_step > 1) {
			ret = 1;
		}
		break;
	}
	}
	if(e_step > MAX_POINTER_TRAVEL) {
		ret = 1;
	}
	if (tp_pointer->x > monkey_context.x_res - 1) {
		tp_pointer->x = monkey_context.x_res - 1;
	}
	if (tp_pointer->x < 0) {
		tp_pointer->x = 0;
	}
	if (tp_pointer->y > monkey_context.y_res - 1) {
		tp_pointer->y = monkey_context.y_res - 1;
	}
	if (tp_pointer->y < 0) {
		tp_pointer->y = 0;
	}
	//os_printk("(%d %d pressed %d) event %d e_step %d ret %d \n", tp_pointer->x, tp_pointer->y, tp_pointer->pressed,event,e_step,ret);
	*event_step = e_step;
	return ret;
}

static int send_remind(uint8_t *event_step)
{
	struct app_msg msg = { 0 };

	/* simulate lowpower event */
	if (*event_step == 0) {
		SYS_LOG_INF("monkey remind");

		msg.type = MSG_SYS_EVENT;
		msg.cmd = SYS_EVENT_BATTERY_LOW;
	} else {
		msg.type = MSG_BAT_CHARGE_EVENT;
		msg.cmd = BAT_CHG_EVENT_DC5V_IN;
	}

	if (send_async_msg_discardable(APP_ID_LAUNCHER, &msg) == true) {
		*event_step += 1;
	}

	return (*event_step == 1) ? 0 : 1;
}

static void _monkey_work(struct k_work *work)
{
	uint8_t event = _hardware_randomizer_gen_rand() % NUM_MONKEY_TYPES;
#ifdef MONKEY_TEST_DVFS
	monkey_context.dvfs_level_change_interval++;
	if (monkey_context.dvfs_level_change_interval > DVFS_LEVEL_CHANGE_INTERVAL) {
		monkey_context.dvfs_level_change_interval = 0;
		dvfs_force_unset_level(monkey_context.dvfs_level, "monkey");
		monkey_context.dvfs_level = dvfs_level_table[_hardware_randomizer_gen_rand() % sizeof(dvfs_level_table)];
		dvfs_force_set_level(monkey_context.dvfs_level, "monkey");
	}
#endif

	if (event == KEY_EVENT) {
		uint32_t uptime = os_uptime_get_32();
		if (uptime - monkey_context.inputs[MONKEY_KEY].interval >= KEY_EVENT_INTERVAL) {
			sys_event_report_input(DEFAULT_KEY_EVENT);
			monkey_context.inputs[MONKEY_KEY].interval = uptime;
			monkey_context.inputs[MONKEY_KEY].finished = 1;
		}
	} else if (event == REMIND_EVENT) {
		if (send_remind(&monkey_context.inputs[MONKEY_REMIND].step)) {
			monkey_context.inputs[MONKEY_REMIND].step = 0;
			monkey_context.inputs[MONKEY_REMIND].finished = 1;
		}
	} else {
		if (monkey_context.inputs[MONKEY_TP].finished) {
			monkey_context.inputs[MONKEY_TP].finished = 0;
			monkey_context.inputs[MONKEY_TP].event = event;
			monkey_context.inputs[MONKEY_TP].step = 0;
		}
	}

	if (!monkey_context.inputs[MONKEY_TP].finished) {
		struct tp_pointer_t tp_pointer = { 0 };
		if (get_tp_pointer(&tp_pointer, monkey_context.inputs[MONKEY_TP].event,
		                   &monkey_context.inputs[MONKEY_TP].step)) {
			monkey_context.inputs[MONKEY_TP].finished = 1;
		}

		struct input_value val;
		val.point.loc_x = tp_pointer.x;
		val.point.loc_y = tp_pointer.y;
		val.point.pessure_value = tp_pointer.pressed;
		val.point.gesture = monkey_context.inputs[MONKEY_TP].event;
		tpkey_put_point(&val, k_cycle_get_32());

		if (monkey_context.inputs[MONKEY_TP].finished) {
			os_sleep(monkey_context.monkey_event_interval);
		}
	}

	if (monkey_context.monkey_start) {
		k_delayed_work_submit_to_queue(&monkey_context.monkey_work_q, &monkey_context.monkey_work, Z_TIMEOUT_MS(INPUT_EVENT_INTERVAL));
	}
}

int system_monkey_test_start(uint16_t interval)
{
#ifdef MONKEY_TEST_DVFS
	monkey_context.dvfs_level = dvfs_get_current_level();
	dvfs_lock();
#endif

	if (!monkey_context.monkey_start) {
		k_delayed_work_submit_to_queue(&monkey_context.monkey_work_q,&monkey_context.monkey_work, Z_TIMEOUT_MS(INPUT_EVENT_INTERVAL));
		monkey_context.monkey_start = 1;
		monkey_context.monkey_event_interval = interval;
		os_printk("monkey start ...\n");
	} else {
		os_printk("monkey already start ...\n");
	}
	return 0;
}

int system_monkey_test_stop(void)
{
	if (monkey_context.monkey_start) {
		k_delayed_work_cancel(&monkey_context.monkey_work);
		monkey_context.monkey_start = 0;
		os_printk("monkey stop ...\n");
	} else {
		os_printk("monkey not start ...\n");
	}
#ifdef MONKEY_TEST_DVFS
	dvfs_unlock();
#endif
	return 0;
}

int system_monkey_init(const struct device * dev)
{
	ARG_UNUSED(dev);
	k_work_queue_start(&monkey_context.monkey_work_q,
			monkey_work_q_stack, K_THREAD_STACK_SIZEOF(monkey_work_q_stack), 2, NULL);

	k_delayed_work_init(&monkey_context.monkey_work, _monkey_work);

	monkey_context.x_res = DEF_UI_WIDTH;
	monkey_context.y_res = DEF_UI_HEIGHT;
	monkey_context.monkey_start = 0;

	for (int i = 0; i < NUM_MONKEY_INPUTS; i++) {
		monkey_context.inputs[i].finished = 1;
	}

	return 0;
}

SYS_INIT(system_monkey_init, APPLICATION, 5);

