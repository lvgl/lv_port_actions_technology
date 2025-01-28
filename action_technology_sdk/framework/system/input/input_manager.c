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
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define MAX_INPUT_DRIVERS 4

struct input_manager_info {
#ifdef CONFIG_INPUT_POINTER
	input_dev_t input_devs[MAX_INPUT_DRIVERS];
#endif

	bool input_manager_lock;
	bool filter_itself;   /* filter all key event after current key event*/
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	bool req_boosted;
	bool last_boosted;
	os_delayed_work boost_work;
#endif
};

#ifdef CONFIG_INPUT_KEYPAD
extern void input_keypad_set_init_param(input_dev_init_param *data);
#endif
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
void input_boost_work(struct k_work *work);
#endif

static struct input_manager_info global_input_manager;

static struct input_manager_info *input_manager = &global_input_manager;

bool input_dev_read(input_dev_t * indev, input_dev_data_t * data)
{
    bool cont = false;

    memset(data, 0, sizeof(input_dev_data_t));

    /* For touchpad sometimes users don't the last pressed coordinate on release.
     * So be sure a coordinates are initialized to the last point */
    if(indev->driver.type == INPUT_DEV_TYPE_POINTER) {
        data->point.x = indev->proc.types.pointer.act_point.x;
        data->point.y = indev->proc.types.pointer.act_point.y;
    }
    /*Similarly set at least the last key in case of the  the user doesn't set it  on release*/
    else if(indev->driver.type == INPUT_DEV_TYPE_KEYPAD) {
        data->key = indev->proc.types.keypad.last_key;
    }

    if(indev->driver.read_cb) {
        cont = indev->driver.read_cb(&indev->driver, data);
    } else {
        SYS_LOG_WRN("NO indev function registered");
    }

    return cont;
}


void input_manager_dev_enable(input_dev_t *indev)
{
    if(indev->driver.enable) {
        indev->driver.enable(&indev->driver, true);
    } 
}

int input_driver_register(input_drv_t *input_drv)
{
#ifdef CONFIG_INPUT_POINTER
	input_dev_t *free_input_dev = NULL;

	for (int i = 0; i < MAX_INPUT_DRIVERS; i++) {
		if (!input_manager->input_devs[i].used_flag) {
			free_input_dev = &input_manager->input_devs[i];
		}
	}

	if (!free_input_dev)
		return -ENOMEM;

	memset(free_input_dev, 0, sizeof(input_dev_t));
    memcpy(&free_input_dev->driver, input_drv, sizeof(input_drv_t));

	free_input_dev->used_flag = 1;
#endif

	return 0;
}

input_dev_t *input_manager_get_input_dev(input_dev_type_t dev_type)
{
	input_dev_t *input_dev = NULL;
#ifdef CONFIG_INPUT_POINTER
	for (int i = 0; i < MAX_INPUT_DRIVERS; i++) {
		if (input_manager->input_devs[i].driver.type == dev_type
			&& input_manager->input_devs[i].used_flag) {
			input_dev = &input_manager->input_devs[i];
		}
	}
#endif
	return input_dev;
}

/*init manager*/
bool input_manager_init(event_trigger event_cb)
{
	input_manager = &global_input_manager;

	memset(input_manager, 0, sizeof(struct input_manager_info));

#ifdef CONFIG_INPUT_ENCODER
	input_encoder_device_init();
#endif

#ifdef CONFIG_INPUT_KEYPAD
	input_keypad_device_init(event_cb);
#endif

#ifdef CONFIG_INPUT_POINTER
	input_pointer_device_init();
#endif

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
    input_knob_device_init();
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	os_delayed_work_init(&input_manager->boost_work, input_boost_work);
#endif

	SYS_LOG_INF("init success\n");
	input_manager_lock();

	return true;
}

void input_manager_set_init_param(input_dev_init_param *data)
{
#ifdef CONFIG_INPUT_KEYPAD
	input_keypad_set_init_param(data);//ms
#endif
}

bool input_manager_lock(void)
{
	unsigned int key;

	key = os_irq_lock();
	input_manager->input_manager_lock = true;
	os_irq_unlock(key);
	return true;
}

bool input_manager_unlock(void)
{
	unsigned int key;

	key = os_irq_lock();
	input_manager->input_manager_lock = false;
	os_irq_unlock(key);
	return true;
}

bool input_manager_islock(void)
{
	bool result = false;
	unsigned int key;

	key = os_irq_lock();
	result = input_manager->input_manager_lock;
	os_irq_unlock(key);
	return result;
}

bool input_manager_filter_key_itself(void)
{
	unsigned int key;

	key = os_irq_lock();
	input_manager->filter_itself = true;
	os_irq_unlock(key);

	return true;
}

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
void input_boost_work(struct k_work *work)
{
	unsigned int key;
	bool boost, last_boosted;

	key = os_irq_lock();
	boost = input_manager->req_boosted;
	last_boosted = input_manager->last_boosted;
	os_irq_unlock(key);

	if (last_boosted != boost) {
		if (boost) {
			SYS_LOG_INF("input boost start");
			dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "input");
		} else {
			SYS_LOG_INF("input boost stop");
			dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "input");
		}
		key = os_irq_lock();
		input_manager->last_boosted = boost;
		os_irq_unlock(key);
	}

}
#endif

void input_manager_boost(bool boost)
{
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	unsigned int key;
	bool last_boosted;
	int delay_ms;

	key = os_irq_lock();
	last_boosted = input_manager->last_boosted;
	if (last_boosted != boost) {
		input_manager->req_boosted = boost;
	}
	os_irq_unlock(key);

	if (boost) {
		os_delayed_work_cancel(&input_manager->boost_work);
	}

	if (last_boosted != boost) {
		delay_ms = boost ? 0 : 2000; // delay 2s to unboost
		os_delayed_work_submit(&input_manager->boost_work, delay_ms);
	}
#endif
}

