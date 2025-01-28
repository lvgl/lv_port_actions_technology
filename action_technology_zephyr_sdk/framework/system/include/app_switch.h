/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app switch
 */
#ifndef APP_SWITCH_H_
#define APP_SWITCH_H_

#define MAX_APP_NUM 25

#define APP_SWITCH_LOCK_BTCALL		(1 << 0)

typedef enum{
	/** next app */
	APP_SWITCH_NEXT = 0x1,

	/** prev app */
	APP_SWITCH_PREV = 0x2,

	/** last app */
	APP_SWITCH_LAST = 0x04,

	/** curret app  */
	APP_SWITCH_CURR = 0x08,
}app_switch_mode_e;

int app_switch(void *app_name, uint32_t switch_mode, bool force_switch);
uint8_t * app_switch_get_app_name(uint8_t appid);
void app_switch_add_app(const char *app_name);
void app_switch_remove_app(const char *app_name);
void app_switch_lock(uint8_t reason);
void app_switch_unlock(uint8_t reason);
void app_switch_force_lock(uint8_t reason);
void app_switch_force_unlock(uint8_t reason);
int app_switch_init(const char **app_id_switch_list, int app_num);

#endif /* APP_SWITCH_H_ */
