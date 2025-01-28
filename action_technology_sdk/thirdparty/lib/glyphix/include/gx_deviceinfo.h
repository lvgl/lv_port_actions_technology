/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_async.h"
#include <stdint.h>
namespace gx {
/* 获取设备品牌 */
JsValue device_info_get_brand();
/* 获取设备生产商 */
JsValue device_info_get_manufacturer();
/* 获取设备型号 */
JsValue device_info_get_model();
/* 获取设备代号 */
JsValue device_info_get_product();
/* 获取设备唯一标识 */
JsValue device_info_get_device_id();
/* 获取用户唯一标识 */
JsValue device_info_get_mac();
/* 获取用户唯一标识 */
JsValue device_info_get_user_id();
/* 获取广告唯一标识 */
JsValue device_info_get_advertis_id();
/* 获取操作系统名称 */
JsValue device_info_get_os_type();
/* 获取操作系统版本名称 */
JsValue device_info_get_os_version_name();
/* 获取设备序列号 */
JsValue device_info_get_serial_number();
/* 获取存储空间的总大小 */
uint64_t device_info_get_totalstorage();
/* 获取存储空间的可用大小 */
uint64_t device_info_get_availablestorage();
} // namespace gx
