/**
 * \file tuya_ble_port_ats310x.h
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


#ifndef TUYA_BLE_PORT_ATS310X_H__
#define TUYA_BLE_PORT_ATS310X_H__


#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_config.h"
//#include "app_util_platform.h"
//#include "elog.h"
#define VFD_TUYA_AUTH            "TUYA_AUTH"
#define VFD_TUYA_AUTH_BAK        "TUYA_AUTH_BAK"
#define VFD_TUYA_SYS             "TUYA_SYS"
#define VFD_TUYA_SYS_BAK         "TUYA_SYS_BAK"
#define VFD_BLE_DEV_ADDR         "BLE_DEV_ADDR"
#define VFD_COUNTDOWN_VALUE      "COUNTDOWN_VALUE"
#define VFD_TUYA_VOS_TOKEN       "TUYA_VOS_TOKEN"


tuya_ble_status_t nrf_err_code_convert(uint32_t errno);
extern int print_hex(const char* prefix, const void* data, size_t size);

#if (TUYA_BLE_LOG_ENABLE||TUYA_APP_LOG_ENABLE)

#define TUYA_BLE_PRINTF(...)            printf(__VA_ARGS__)//
#define TUYA_BLE_HEXDUMP(...)           print_hex("", __VA_ARGS__)// 

#else

#define TUYA_BLE_PRINTF(...)           
#define TUYA_BLE_HEXDUMP(...)  

#endif

#endif





