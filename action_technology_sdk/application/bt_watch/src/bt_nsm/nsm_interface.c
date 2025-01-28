/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
	
#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <app_switch.h>
#include <app_config.h>
#include <os_common_api.h>
#include <ctype.h>
#include <acts_bluetooth/host_interface.h>
#include <dvfs.h>

#include "nsm_test_backend.h"
#include "nsm_test_inner.h"
#include "nsm_interface.h"
//#include <code_in_btram0.h>
//extern const int code_in_btram0[9740];

#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif

/* Before testing, replace the bin file that supports
 * the system to run non-signaling, soc/arm/actions/lark/libbt_test.a
 *
 */

//#define NSM_DEVICE_NAME		"FACTORY_TXTEST"
//#define NSM_DEVICE_NAME_LEN		(sizeof(NSM_DEVICE_NAME) - 1)

static const struct bt_data nsm_ble_ad_discov[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	//BT_DATA(BT_DATA_NAME_COMPLETE, NSM_DEVICE_NAME, NSM_DEVICE_NAME_LEN),
};

struct nsm_test_mgr_info {

	u32_t switch_times;
	os_delayed_work ble_tx_stress_work;
};

static struct nsm_test_mgr_info nsm_info;

#if FCC_TEST_WITH_SDK_RUN
void sdk_sleep_ms(unsigned int ms)
{
	if (!sys_wakelocks_check(PARTIAL_WAKE_LOCK)
		|| !sys_wakelocks_check(FULL_WAKE_LOCK)) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif
	}

	os_sleep(ms);
}

uint32_t get_sdk_time(void)
{
	return k_uptime_get_32();
}
#endif

int nsm_ble_tx_test(u16_t timeout, u8_t channel, u8_t power, int cmd, u8_t mode)
{
	if (channel > 39) {
		SYS_LOG_ERR("ERR channel %d!",channel);
		return -1;
	}

	if (mode > 1) {
		SYS_LOG_ERR("BLE MODE %d!",mode);
		return -1;
	}

	if ((power < 20) || (power > 43)) {
		SYS_LOG_ERR("ERR tx power %d!",power);
		return -1;
	}

	if (cmd > NSM_BLE_TX_CMD_MAX) {
		SYS_LOG_ERR("ERR tx cmd %d!",cmd);
		return -1;
	}

	int result;

	os_sleep(100);
#if FCC_TEST_WITH_SDK_RUN
#else
	//if (bt_manager_ble_is_connected())
	//	bt_manager_ble_disconnect();
	bt_nsm_prepare_disconnect();
#endif
	SYS_LOG_INF("Input BT TX test mode.");
	//os_sleep(200);
	//uint32_t flags;
	uint8_t bt_param[9];
	uint8_t tx_mode;
	//uint8_t payload_mode;

	SYS_LOG_INF("timeout %d, channel %d, power %d, cmd %d.", timeout, channel, power, cmd);

	if (NSM_BLE_TX_CMD_POWER == cmd) {
		// power and ACP test
		tx_mode = 0x14;
		//payload_mode = 0x10; // invalid value
	} else if (NSM_BLE_TX_CMD_MODULE1 == cmd) {
		// 调制特性00001111
		tx_mode = 0x15;
		//payload_mode = 0x04;
	} else {
		// 调制特性01010101
		tx_mode = 0x16;
		//payload_mode = 0x05;
	}
	printk("tx_mode %d, cmd %d.", tx_mode, cmd);

	/** bt_param(For tx mode):
	*		byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
	*		byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
	*		byte2 : channel //tx channel	 (0-79)
	*		byte3 : tx_power_mode //tx power	   (20-43)
	*		byte4 : tx_mode //tx mode, DH1/DH3/DH5	  (9-19, !=12)
	*		byte5 : payload_mode //payload mode   (0-6)
	*		byte6 : excute mode // excute mode (0-2)
	*		byte7 : test time // unit : s
	*/
	bt_param[0] = 1; //1: BLE TEST
	bt_param[1] = mode; //0: BLE 1M, 1: BLE 2M
	bt_param[2] = channel*2; //tx channel
	bt_param[3] = power;
	bt_param[4] = tx_mode;
	//bt_param[5] = payload_mode;
	bt_param[5] = 0x10; // no use by BLE, to set invalid value.
	bt_param[6] = 1;
	bt_param[7] = (u8_t)(timeout & 0xFF);
	bt_param[8] = (u8_t)(timeout >> 8);
#if FCC_TEST_WITH_SDK_RUN
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_udelay = k_busy_wait;
	global_ft_env_var.ft_mdelay = sdk_sleep_ms;
	global_ft_env_var.ft_get_time_ms = get_sdk_time;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main"); 
	result = fcc_test_main(1, bt_param, NULL);
#else
	uint32_t flags;
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main"); 
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	result = fcc_test_main(1, bt_param, NULL);
	//k_sched_unlock();
	//irq_unlock(flags);
#endif

	printk("nsm_ble_tx_test result %d.\n", result); //进入非信令失败会打印
	//os_sleep(100);
	sys_pm_reboot(0);
	return 0;
}

int nsm_ble_rx_test(u16_t timeout, u8_t channel, u8_t *mac, u8_t mode)
{
	int8_t rssi = 0;

	if (channel > 39) {
		SYS_LOG_ERR("ERR channel %d!",channel);
		return -1;
	}

	if (mode > 1) {
		SYS_LOG_ERR("BLE MODE %d!",mode);
		return -1;
	}

#if 0
	if (timeout > 10) {
		SYS_LOG_ERR("ERR rx timeout %d!",timeout);
		return -1;
	}
#endif
	SYS_LOG_INF("timeout %d, channel %d.",timeout, channel);
	SYS_LOG_INF("mac 0x%x,0x%x,0x%x,0x%x!",mac[0], mac[1], mac[2], mac[3]);

	int result;
	uint8_t save_test_index = 0, save_test_result = 1, save_report[16], i;
	//uint32_t flags;
	uint8_t rx_param[11], report[18];
	
	memset(save_report, 0, sizeof(save_report));
	property_get("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_get("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_get("FCC_TEST_REPORT", (char *)&save_report, 16);

	SYS_LOG_INF("Pre test index %d result %d", save_test_index, save_test_result);
	for (i=0; i<16; i++) {
		SYS_LOG_INF("Pre test report %d = 0x%x", i, save_report[i]);
	}

	SYS_LOG_INF("Input BT RX test mode.");

	os_sleep(100);

#if FCC_TEST_WITH_SDK_RUN
#else
	//if (bt_manager_ble_is_connected())
	//	bt_manager_ble_disconnect();
	bt_nsm_prepare_disconnect();
#endif
	//os_sleep(200);

	/* * rx_param(For rx mode):
		*	byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
		*	byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		*	byte[2-5]:	access code
		*	byte6 : channel //rx channel	 (0-79)
		*	byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
		*	byte8 : excute mode // excute mode, 0: one packet; 1:continue
		*	byte9 : test time // unit : s
		*	rx_report(Only for rx mode):
		*					buffer[16]: 16byte report.
		* Return:  0:success; 1:failed
	*/
	rx_param[0] = 1; //1: BLE TEST
	rx_param[1] = mode; //0: BLE 1M, 1: BLE 2M
	rx_param[2] = mac[0];
	rx_param[3] = mac[1];
	rx_param[4] = mac[2];
	rx_param[5] = mac[3];
	rx_param[6] = channel*2;	//rx channel
	rx_param[7] = 0x12; /* rx mode 0x12. K_RX_MODE_LE_01010101 */
	rx_param[8] = 1;	/* continue */
	rx_param[9] = (u8_t)(timeout & 0xFF);
	rx_param[10] = (u8_t)(timeout >> 8);

#if FCC_TEST_WITH_SDK_RUN
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_udelay = k_busy_wait;
	global_ft_env_var.ft_mdelay = sdk_sleep_ms;
	global_ft_env_var.ft_get_time_ms = get_sdk_time;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main"); 
	result = fcc_test_main(2, rx_param, report);
#else
	uint32_t flags;
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main"); 
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	result = fcc_test_main(2, rx_param, report);
	//k_sched_unlock();
	//irq_unlock(flags);
#endif

	memset(save_report, 0, sizeof(save_report));
	if (result == 0) {
		memcpy(save_report, report, sizeof(save_report));
		rssi = report[sizeof(save_report)];
		printk("Fcc test rx rssi %d.\n", rssi);
	}
	
	save_test_index++;
	save_test_result = (uint8_t)result;

	property_set("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_set("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_set("FCC_TEST_REPORT", (char *)&save_report, 16);
	property_set("FCC_TEST_RX_RSSI", (char *)&rssi, 1);
	property_flush(NULL);
	
	printk("Fcc test result %d\n", result);	/* fcc_test_main parameter error will print this log */
	//os_sleep(100);
	
	sys_pm_reboot(0);
	return 0;
}

int nsm_ble_rx_report_get(void)
{
	uint8_t save_test_index = 0, save_test_result = 1, save_report[16], i;
	int sum_pkg, err_pkg, cor_pkg; int *ptr;
	memset(save_report, 0, sizeof(save_report));
	property_get("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_get("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_get("FCC_TEST_REPORT", (char *)&save_report, 16);
	SYS_LOG_INF("Pre test index %d result %d", save_test_index, save_test_result);
	for (i=0; i<16; i++) {
		SYS_LOG_INF("Pre test report %d = 0x%x", i, save_report[i]);
	}

	if (0 != save_test_result) {
		SYS_LOG_ERR("test report fail, save_test_result %d", save_test_result);
		return -1;
	}
/* save_report[16]示例
*	00 00 00 00 68 21 37 00 00 00 00 00 57 41 00 00
*	第一个word 4byte 为错误的 bit 数量(0x00000000)
*	第二个 word 4byte 为接收到的 bit 数量 (0x00372168)
*	第三个 word 4byte 为错误的包数量 (0x00000000)
*	第四个 word 4byte 为接收到的包数量(0x00004157) 
*/
	ptr = (int *)save_report;
	err_pkg = ptr[2];
	sum_pkg = ptr[3];
	if (err_pkg > sum_pkg) {
		SYS_LOG_ERR("err_pkg 0x%x, sum_pkg 0x%x.", err_pkg, sum_pkg);
		return -1;		
	}
	
	cor_pkg = sum_pkg - err_pkg;
	return cor_pkg;
}

int8_t nsm_ble_rx_rssi_get(void)
{
	int8_t save_rssi = 0;

	property_get("FCC_TEST_RX_RSSI", (char *)&save_rssi, 1);
	SYS_LOG_INF("save_rssi %d.", save_rssi);
	return save_rssi;
}

static void  nsm_ble_tx_stress_work(struct k_work *work)
{
	SYS_LOG_INF("test_runing %d min.",nsm_info.switch_times);
	struct bt_le_adv_param param;
	const struct bt_data *ad;
	size_t ad_len;
	int err;

	hostif_bt_le_adv_stop();
	os_sleep(200);

	if (nsm_info.switch_times > 120) { // test time for 2 hours
		sys_pm_reboot(0);
		os_sleep(2000);
	}

	memset(&param, 0, sizeof(param));
	param.id = BT_ID_DEFAULT;
	param.interval_min = 0xA0; // 100ms
	param.interval_max = 0xA0; // 100ms
	param.options = (BT_LE_ADV_OPT_NONE | BT_LE_ADV_OPT_USE_NAME);
	if (0 == nsm_info.switch_times%3) {
		param.options |= (BT_LE_ADV_OPT_DISABLE_CHAN_38 | BT_LE_ADV_OPT_DISABLE_CHAN_39);
	} else if (1 == nsm_info.switch_times%3) {
		param.options |= (BT_LE_ADV_OPT_DISABLE_CHAN_37 | BT_LE_ADV_OPT_DISABLE_CHAN_39);
	} else {
		param.options |= (BT_LE_ADV_OPT_DISABLE_CHAN_37 | BT_LE_ADV_OPT_DISABLE_CHAN_38);
	}

	ad = nsm_ble_ad_discov;
	ad_len = ARRAY_SIZE(nsm_ble_ad_discov);
	err = hostif_bt_le_adv_start(&param, ad, ad_len,NULL, 0);
	if (err < 0 && err != (-EALREADY)) {
		SYS_LOG_ERR("Failed to start advertising (err %d)", err);
	} else {
		SYS_LOG_INF("Advertising started ad_len %d", ad_len);
	}
	nsm_info.switch_times++;
	os_delayed_work_submit(&nsm_info.ble_tx_stress_work, (60*1000));
}

int nsm_ble_tx_stress_normal(void)
{
#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(PARTIAL_WAKE_LOCK);
#endif
	nsm_info.switch_times = 0;

#ifdef CONFIG_BT_BR_ACTS
	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
#endif

	os_sleep(100);
	if (bt_manager_ble_is_connected())
		bt_manager_ble_disconnect();

	os_delayed_work_init(&nsm_info.ble_tx_stress_work, nsm_ble_tx_stress_work);
	os_delayed_work_submit(&nsm_info.ble_tx_stress_work, 1000);

#ifdef CONFIG_SYS_WAKELOCK
	//sys_wake_unlock(PARTIAL_WAKE_LOCK);
#endif
	return 0;
}

int nsm_pcba_uart_test(void)
{
	uint32_t flags;

	bt_nsm_prepare_disconnect();
	os_sleep(100);

	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main");
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	SYS_LOG_INF("fcc_test_main");
	fcc_test_main(0, NULL, NULL);
	//k_sched_unlock();
	//irq_unlock(flags);
	printk("Fcc test exit.\n");/* fcc_test_main parameter error will print this log */
	
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(1);
#endif
	sys_pm_reboot(0);
	return 0;
}

