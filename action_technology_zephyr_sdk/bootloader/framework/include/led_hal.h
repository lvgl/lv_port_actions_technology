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
 *	2017/4/13: Created by wh.
 */

#ifndef __LED_HAL_H__
#define __LED_HAL_H__
#include <msg_manager.h>
#include <drivers/pwm.h>

void led_on(u8_t led_index);
void led_off(u8_t led_index);
void led_breath(u8_t led_index, pwm_breath_ctrl_t *ctrl);
void led_blink(u8_t led_index, u16_t period, u16_t pulse, u8_t start_state);

#endif
