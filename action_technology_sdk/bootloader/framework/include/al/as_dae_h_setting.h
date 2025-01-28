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

 * Author: jpl<jianpingliao@actions-semi.com>
 *
 * Change log:
 *	2019/12/16: Created by jpl.
 */

#ifndef __AS_DAE_H_SETTING_H__
#define __AS_DAE_H_SETTING_H__

#define AS_DAE_H_EQ_NUM_BANDS  (14)
#define AS_DAE_H_PARA_BIN_SIZE (sizeof(((as_dae_h_para_info_t*)0)->as_dae_h_para))

/*for DAE AL*/
typedef struct
{
	short bypass;
	short reserve0;
	/* ASQT configurable parameters */
	char as_dae_h_para[192];
} as_dae_h_para_info_t;


#endif /* __AS_DAE_H_SETTING_H__ */
