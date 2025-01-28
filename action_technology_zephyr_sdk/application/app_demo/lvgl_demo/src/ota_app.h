/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA application interface
 */

#ifndef __OTA_APP_H__
#define __OTA_APP_H__

int ota_app_init(void);
int ota_app_init_bluetooth(void);
bool ota_is_already_running(void);
bool ota_is_already_done(void);
int ota_app_process(void);
int ota_app_reboot(uint8_t temp_file_id);

#endif /* __OTA_APP_H__ */

