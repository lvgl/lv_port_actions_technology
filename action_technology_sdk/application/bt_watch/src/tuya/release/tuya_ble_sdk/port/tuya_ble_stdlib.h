/**
 * \file tuya_ble_stdlib.h
 *
 * \brief 
 */
/*
 *  Copyright (C) 2014-2019, Tuya Inc., All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of tuya ble sdk 
 */


#ifndef TUYA_BLE_STDLIB_H__
#define TUYA_BLE_STDLIB_H__

/*
 * If not using the standard library,please set to 0 . Then implement the library functions required by the TUYA BLE SDK in other files and declare in this file.
 */

#define TUYA_BLE_USE_STDLIB  0 // modify by zdx

#if TUYA_BLE_USE_STDLIB

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#else
#include <zephyr/types.h>
#include <stdbool.h>

#include <string.h>
#include <stdlib.h>

//#define bool 
#define true 1
#define false 0
    
//..........

//void *memset(void *str, int32_t c, uint32_t n);
//void *memcpy(void *str1, const void *str2, uint32_t n);
//int32_t memcmp(const void *str1, const void *str2, uint32_t n);
//uint32_t strlen(const char *str);
//int32_t rand(void);
//int64_t atoll( const char * str );

//.............

int32_t rand(void);
long long atoll(const char *p);


#endif






#endif


#include "custom_tuya_ble_config.h"


