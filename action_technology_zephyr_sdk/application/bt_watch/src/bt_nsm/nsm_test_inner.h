/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __NSM_TEST_INNER_H__
#define __NSM_TEST_INNER_H__

#include <kernel.h>
#include <device.h>
#include <thread_timer.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <app_defines.h>
#include <bt_manager.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <sys_manager.h>
#include <sys_event.h>
#include <spp_test_backend.h>
#include "tool_app.h"
#include "usp_protocol.h"
//#include "../tool_app_inner.h"
#include "app_config.h"
#include <stdio.h>
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#ifdef CONFIG_BLUETOOTH
#include "mem_manager.h"
#endif
#include <bt_production_test.h>


#define FCC_TEST_WITH_SDK_RUN 0 // System runtime testing, 

#if FCC_TEST_WITH_SDK_RUN
void sdk_sleep_ms(unsigned int ms);
uint32_t get_sdk_time(void);
#endif

void nsm_init(void);
void nsm_deinit(void);

extern bool nsm_test_backend_load_bt_init(spp_test_notify_cb_t cb, struct ota_backend_bt_init_param *param);


/*************************************************
* Description:	fcc test function entry
* Input: mode	0--uart test mode, for pcba
*				1--bt tx test mode, for demo
*				2--bt rx test mode, for demo
* bt_param(For tx mode):
*					byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte2 : channel //tx channel	 (0-79)
*					byte3 : tx_power_mode //tx power	   (0-43)
*					byte4 : tx_mode //tx mode, DH1/DH3/DH5	  (9-19, !=12)
*					byte5 : payload_mode //payload mode   (0-6)
*					byte6 : excute mode // excute mode (0-2)
*					byte7 : test time // unit : s
* bt_param(For rx mode):
*					byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte[2-5]:	access code
*					byte6 : channel //rx channel	 (0-79)
*					byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
*					byte8 : excute mode // excute mode, 0: one packet; 1:continue
*					byte9 : test time // unit : s
* rx_report(Only for rx mode):
*					buffer[16]: 16byte report.
* Return:  0:success; 1:failed
****************************************************/
extern int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);

/*
 * 测试蓝牙非信令, 请使用以下代码进入非信令，务必先调用dvfs_set_level调频
	#include <dvfs.h>

	int flags,result;
	extern int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");  //调频
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	result = fcc_test_main(0, NULL, NULL);
	//OR result = fcc_test_main(1, bt_param, NULL);
	//OR result = fcc_test_main(2, bt_param, rx_report);
	printk("result %d.", result);
	k_sched_unlock();
	irq_unlock(flags);
 */

void sys_pm_reboot(int type);
void ft_load_fcc_bin(void);
void media_player_force_stop(void);
void bt_nsm_prepare_disconnect(void);

#endif  /* __NSM_TEST_INNER_H__ */

