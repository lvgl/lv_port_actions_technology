/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: wh<wanghui@actions-semi.com>
 *
 * Change log:
 *	2017/3/15: Created by wh.
 */

#ifndef __ADCKEY_HAL_H__
#define __ADCKEY_HAL_H__

#ifndef CONFIG_SIMULATOR
#include <drivers/input/input_dev.h>
#endif

#define ADC_KEY_DEVICE_NAME  "keyadc"

#define ONOFF_KEY_DEVICE_NAME "onoffkey"

#define GPIO_KEY_DEVICE_NAME "keygpio"

typedef void (*key_notify_cb)(struct device *dev, struct input_value *val);

struct key_handle
{
	struct device *input_dev;
	key_notify_cb key_notify;
};

void* key_device_open(key_notify_cb cb,char * dev_name);

void key_device_close(void * handle);

int key_device_inquiry(void * handle);

void key_device_abort_cb(void * handle);

void key_device_enable_cb(void * handle);

#endif   //__ADCKEY_HAL_H__
