/*
 * Copyright (c) 2021 Actions Semi Co., Ltd.
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
#include <drivers/misc/audio_channel_sel.h>

#define CHAN_SEL_DEVICE_NAME  "audio_channel_sel"

int chan_sel_device_inquiry(struct audio_chansel_data *data,char * dev_name);

#endif   //__ADCKEY_HAL_H__

