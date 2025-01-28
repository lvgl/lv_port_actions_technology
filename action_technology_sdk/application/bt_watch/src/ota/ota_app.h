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
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

int ota_app_init(void);
int ota_app_init_sdcard(void);
int ota_app_init_bluetooth(void);

void ota_view_deinit(void);
void ota_view_init(void);
void ota_view_show_upgrade_progress(uint8_t progress);
void ota_view_show_upgrade_result(uint8_t *string, bool is_faill);
void ota_install_start(void);
void ota_view_exit(void);

#endif /* __OTA_APP_H__ */
