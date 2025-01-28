
#include <app_defines.h>
#include <sys_manager.h>
#include <bt_manager.h>
#include <msg_manager.h>
#include <app_switch.h>
#include <app_config.h>
#include <os_common_api.h>
#include <ctype.h>
#include <dvfs.h>
#include <sdfs.h>

#include "nsm_test_backend.h"
#include "nsm_test_inner.h"
#include "nsm_interface.h"
//#include <code_in_btram0.h>
//extern const int code_in_btram0[9740];

enum NSM_COMMANDS
{
	NSM_TEST_TX_START  = 0x00FD,
	NSM_TEST_RX_START  = 0x00FE,
	NSM_TEST_RX_READ  = 0x00E1,

	NSM_TEST_REDUCE_LE_TX_START  = 0x00C0,
	NSM_TEST_REDUCE_LE_RX_START  = 0x00C1,
	NSM_TEST_REDUCE_LE_RX_READ  = 0x00C2,

	NSM_TEST_REDUCE_LE_TX_STRESS  = 0x00C8,
};

typedef struct
{
	uint8_t  sync_byte1;   // sync byte1 0xAA
	uint8_t  sync_byte2;   // sync byte2 0xAA
	uint16_t f_len;
	uint16_t cmd;

} __attribute__((packed)) nsm_frame_head_s;

typedef u8_t (*command_callback)(u8_t *buf, u16_t len);
typedef struct{
	u16_t recv_cmd_type;
	command_callback cmd_cbk;
} nsm_recv_cmd_list_s;

#define BT_RAM0_ADDR (0x2FF20000)
//#define CONFIG_BT_MP_BTC_FILE_NAME "/NOR:K/mp_btc.bin"

static int sd_fcc_load(const char *filename, void *dst)
{
	struct sd_file *sdf;
	int ret;

	sdf = sd_fopen(filename);
    if (!sdf) {
        return -EINVAL;
    }

    ret = sd_fread(sdf, dst, sdf->size);
	printk("%s size: %d, load: %d\n", filename, sdf->size, ret);

	if (ret == sdf->size) {
		ret = 0;
	} else {
		printk("load %s failed!\n", filename);
		ret = -EINVAL;
	}
	
	sd_fclose(sdf);
	return ret;
}

void ft_load_fcc_bin(void)
{
	int err;

	err = sd_fcc_load(CONFIG_BT_MP_BTC_FILE_NAME, (void *)BT_RAM0_ADDR); /* Leopard BT_RAM0 */
	if (err) {
		printk("ft_load_fcc_bin error %s.\n",CONFIG_BT_MP_BTC_FILE_NAME);
		return;
	}

//	unsigned int i, *p;
//	p = (unsigned int *)0x2FF20000;

//	for(i=0; i<sizeof(code_in_btram0)/4; i++) {
//		*(p+i) = code_in_btram0[i];
//	}
}

void bt_nsm_prepare_disconnect(void) 
{
	media_player_force_stop(); // stop dsp.

	bt_manager_ble_adv_stop();
	if (bt_manager_ble_is_connected())
		bt_manager_ble_disconnect();

#if CONFIG_BT_BR_ACTS
	int time_out = 0;

	bt_manager_set_visible(false);
	bt_manager_set_connectable(false);
	btif_br_auto_reconnect_stop();
	btif_br_disconnect_device(BTSRV_DISCONNECT_ALL_MODE);

	while (btif_br_get_connected_device_num() && time_out++ < 500) {
		os_sleep(10);
	}
#endif
}


void nsm_test_tx_start(u8_t* payload, int payload_len)
{
	// BR TX access code : 0x331a3ae2
	// EDR TX access code : 0x4e7a2cce
	// LE TX access code : 0x71764129

	//example :  0xAAAA 0A00 FD00 01 01 0F 26 14 10 03 00
	/* Payload(8bytes): 
		byte0 : bt_mode 0 or 1 //0: BR/EDR TEST, 1: BLE TEST
		byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		byte2 : channel //tx channel	 (0-79)
		byte3 : tx_power //tx power	   (20-43)
		byte4 : tx_mode //tx mode, DH1/DH3/DH5 or  power and ACP test(0x14),调制特性00001111(0x15),调制特性01010101(0x16)
		byte5 : payload_mode //payload mode(only BR/EDR ,  BLE set 0x10)
		byte6 ~ byte7 : test time // unit : s
	*/
	u8_t tx_payload[12];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;
	int result;

	if (8 != payload_len)
	{
		 SYS_LOG_ERR("payload_len %d!",payload_len);
		 ret_code = 0x1;
	}
	else
	{
		if (((0 != payload[0]) && (1 != payload[0])) ||
			((0 != payload[1]) && (1 != payload[1])) ||
			(payload[2] > 79) ||
			(payload[3] > 43) ||
			(payload[5] > 0x10))
		{
			SYS_LOG_ERR("payload0~3 %x %x %x %x.",payload[0],payload[1],payload[2],payload[3]);
			SYS_LOG_ERR("payload4~5 %x %x.",payload[4],payload[5]);
			ret_code = 0x1;
		}

		if ((payload[4] < 9) || (payload[4] > 0x16) || (payload[4] == 12))
		{
			SYS_LOG_ERR("payload 0~3 %x %x %x %x.",payload[0],payload[1],payload[2],payload[3]);
			SYS_LOG_ERR("payload 4~5 %x %x.",payload[4],payload[5]);
			ret_code = 0x1;
		}
	}

	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t);
	fhead.cmd = NSM_TEST_TX_START;
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;

	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("nsm_at_start RESP ERROR!");
		return;
	}

	if (ret_code)
	{
		SYS_LOG_ERR("nsm_at_start payload error!");
		return;
	}

	os_sleep(100);
	SYS_LOG_INF("Input BT test mode.");
#if 1
	bt_nsm_prepare_disconnect();
	// os_sleep(100);

	uint32_t flags;
	uint8_t bt_param[9];

	/** bt_param(For tx mode):
	*		byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
	*		byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
	*		byte2 : channel //tx channel	 (0-79)
	*		byte3 : tx_power_mode //tx power	   (0-43)
	*		byte4 : tx_mode //tx mode, DH1/DH3/DH5	  (9-19, !=12)
	*		byte5 : payload_mode //payload mode   (0-6)
	*		byte6 : excute mode // excute mode (0-2)
	*		byte7~byte8 : test time // unit : s
	*/
	bt_param[0] = payload[0];
	bt_param[1] = payload[1];
	bt_param[2] = payload[2];
	bt_param[3] = payload[3];
	bt_param[4] = payload[4];
	bt_param[5] = payload[5];
	if (1 == payload[0]) {
		bt_param[5] = 0x10; // no use by BLE, to set invalid value.
	}
	bt_param[6] = 1;
	bt_param[7] = payload[6];
	bt_param[8] = payload[7];
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
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "main"); 
	flags = irq_lock();
	k_sched_lock();
	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	//global_ft_env_var.ft_printf = printk;
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	result = fcc_test_main(1, bt_param, NULL);
	printk("result %d.", result);
	//k_sched_unlock();
	//irq_unlock(flags);
#endif

	printk("nsm_ble_tx_test result %d.\n", result); //进入非信令失败会打印
	sys_pm_reboot(0);
#endif
}

void nsm_test_rx_start(u8_t* payload, int payload_len)
{
	//example 1 :  0xAAAA 0800 FE00 00 01 00 12 05 00
	//example 2 :  0xAAAA 0C00 FE00 00 01 00 12 05 00 29 41 76 71
	/* Payload(6bytes~10bytes): 
		*	byte0 : bt_mode 0 or 1	//0: BR/EDR TEST, 1: BLE TEST
		*	byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
		*	byte2 : channel //rx channel	 (0-79)
		*	byte3 : rx_mode //rx mode (0~11, 0x10~0x12)
		*	byte4~5 : test time // unit : s
		*(option) byte6~9 : 仪器低4bytes mac, 没有就使用默认地址
	*/

	u8_t tx_payload[12];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;
	int8_t rssi = 0;

	int result;
	uint8_t save_test_index = 0, save_test_result = 1, save_report[16], i;
	uint32_t flags;
	uint8_t rx_param[11], report[18];
	
	memset(save_report, 0, sizeof(save_report));
	property_get("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_get("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_get("FCC_TEST_REPORT", (char *)&save_report, 16);

	SYS_LOG_INF("Pre test index %d result %d", save_test_index, save_test_result);
	for (i=0; i<16; i++) {
		SYS_LOG_INF("Pre test report %d = 0x%x", i, save_report[i]);
	}
	os_sleep(50);

	if ((6 != payload_len) && (10 != payload_len))
	{
		 SYS_LOG_ERR("payload_len %d!",payload_len);
		 ret_code = 0x1;
	}
	else
	{
		if (((0 != payload[0]) && (1 != payload[0])) ||
			((0 != payload[1]) && (1 != payload[1])) ||
			(payload[2] > 79) ||
			(payload[3] > 0x12) ||
			((payload[3] > 11) && (payload[3] < 0x10)))
		{
			SYS_LOG_ERR("payload0~3 %x %x %x %x.",payload[0],payload[1],payload[2],payload[3]);
			SYS_LOG_ERR("payload4~5 %x %x.",payload[4], payload[5]);
			ret_code = 0x1;
		}
	}

	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t);
	fhead.cmd = NSM_TEST_RX_START;
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;

	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("RX nsm_at_start RESP ERROR!");
		return;
	}

	if (ret_code)
	{
		SYS_LOG_ERR("RX nsm_at_start payload error!");
		return;
	}

	os_sleep(100);
	SYS_LOG_INF("Input BT RX test mode.");
#if 1
	SYS_LOG_INF("payload 0x%x %x %x %x %x.",payload[0],payload[1],payload[2],payload[3],payload[4]);
	bt_nsm_prepare_disconnect();
	// os_sleep(100);

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
	rx_param[0] = payload[0]; /* 1.ble */
	rx_param[1] = payload[1];/* 0.1M */

	if (5 == payload_len) {
		if (payload[0]) { //BLE
			rx_param[2] = 0x29; /* access code, default: 0x71764129 */
			rx_param[3] = 0x41; /* access code */
			rx_param[4] = 0x76; /* access code */
			rx_param[5] = 0x71; /* access code */
		} else { //BR/EDR
			rx_param[2] = 0x9C; /* access code, default: 0x9cbd359c */
			rx_param[3] = 0x35; /* access code */
			rx_param[4] = 0xBD; /* access code */
			rx_param[5] = 0x9C; /* access code */	
		}
	} else {
			rx_param[2] = payload[6];
			rx_param[3] = payload[7];
			rx_param[4] = payload[8];
			rx_param[5] = payload[9];
	}

	rx_param[6] = payload[2];	/* channel */
	rx_param[7] = payload[3]; /* rx mode 0x12. K_RX_MODE_LE_01010101 */
	rx_param[8] = 1;	/* continue */
	rx_param[9] = payload[4];	/* timeout */
	rx_param[10] = payload[5];
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
#endif
}

void nsm_test_rx_read(void)
{
	// example: 0xaaaa 0200 e100

	u8_t tx_payload[32];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	uint8_t save_test_index = 0, save_test_result = 1, save_report[16], i;
	int8_t save_rssi;

	memset(save_report, 0, sizeof(save_report));
	property_get("FCC_TEST_RESULT", (char *)&save_test_result, 1);
	property_get("FCC_TEST_INDEX", (char *)&save_test_index, 1);
	property_get("FCC_TEST_REPORT", (char *)&save_report, 16);
	property_get("FCC_TEST_RX_RSSI", (char *)&save_rssi, 1);
	SYS_LOG_INF("Pre test index %d result %d rssi %d", save_test_index, save_test_result, save_rssi);
	for (i=0; i<16; i++) {
		SYS_LOG_INF("Pre test report %d = 0x%x", i, save_report[i]);
	}

	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.cmd = NSM_TEST_RX_READ;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t)+sizeof(u8_t)+sizeof(save_report)+sizeof(int8_t);
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = save_test_result;
	tx_payload[sizeof(nsm_frame_head_s) + 1] = save_test_index;
	if ((0 == save_test_result) && (save_test_index > 0)) {
		memcpy(&tx_payload[sizeof(nsm_frame_head_s) + 2], save_report, sizeof(save_report));
		tx_payload[sizeof(nsm_frame_head_s) + 2 + sizeof(save_report)] = save_rssi;
	}
	/*TX Payload(18bytes): 
		*	byte0 : test result 0 or 1
		*	byte1 : test times
		*	byte2-17 : test report
		*	byte18 : test rx rssi
	*/
	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("RX read RESP ERROR!");
		return;
	}
}

void nsm_test_reduce_tx_start(u8_t* payload, int payload_len)
{
	//example :  0xAAAA 0800 C000 0500 00 26 00 01
	/* Payload(6bytes): 
		*	byte0~1 : timeout
		*	byte2 : channel
		*	byte3 : power
		*	byte4 : cmd
		*	byte5 : mode
	*/
	u8_t tx_payload[12];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;
	u16_t time = 0;

	if (6 != payload_len)
	{
		SYS_LOG_ERR("payload_len %d!",payload_len);
		ret_code = 0x1;
	}
	else
	{
		if ((payload[2] > 39) || (payload[3] > 43) || (payload[4] > 3) || (payload[5] > 1))
		{
			SYS_LOG_ERR("payload0~4 %x %x %x %x %x.",payload[0],payload[1],payload[2],payload[3],payload[4]);
			ret_code = 0x1;
		}
	}


	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t);
	fhead.cmd = NSM_TEST_REDUCE_LE_TX_START;
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;

	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("nsm_at_start RESP ERROR!");
		return;
	}

	if (ret_code) {
		SYS_LOG_ERR("ret_code %d ERROR!", ret_code);
		return;
	}

	time = payload[1];
	time = (time << 8) | payload[0];
	nsm_ble_tx_test(time, payload[2], payload[3], payload[4], payload[5]);
}

void nsm_test_reduce_rx_start(u8_t* payload, int payload_len)
{
	//example :  0xAAAA 0A00 C100 0500 00 29 41 76 71 01
	/* Payload(8bytes): 
		*	byte0~1 : timeout
		*	byte2 : channel
		*	byte3~6 : mac
		*	byte7 : mode
	*/
	u8_t tx_payload[12];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;
	u8_t mac[4];
	u16_t time = 0;

	if (8 != payload_len)
	{
		SYS_LOG_ERR("payload_len %d!",payload_len);
		ret_code = 0x1;
	}
	else
	{
		if ((payload[2] > 39) || (payload[7] > 1))
		{
			SYS_LOG_ERR("payload2 7 %x %x.",payload[2],payload[7]);
			ret_code = 0x1;
		}
	}


	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t);
	fhead.cmd = NSM_TEST_REDUCE_LE_RX_START;
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;

	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100)) {
		SYS_LOG_ERR("nsm_at_start RESP ERROR!");
		return;
	}
	
	if (ret_code) {
		SYS_LOG_ERR("ret_code %d ERROR!", ret_code);
		return;
	}

	mac[0] = payload[3];
	mac[1] = payload[4];
	mac[2] = payload[5];
	mac[3] = payload[6];

	time = payload[1];
	time = (time << 8) | payload[0];
	nsm_ble_rx_test(time, payload[2], mac, payload[7]);
}

void nsm_test_reduce_rx_read(void)
{
	// example: 0xaaaa 0200 c200

	u8_t tx_payload[16];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;
	int pkg_num = nsm_ble_rx_report_get();
	int8_t save_rssi;

	if (-1 == pkg_num) {
		ret_code = 0x1;
	}

	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.cmd = NSM_TEST_REDUCE_LE_RX_READ;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t)+sizeof(u32_t)+sizeof(int8_t);
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;
	if (0 == ret_code) {
		memcpy(&tx_payload[sizeof(nsm_frame_head_s) + 1], &pkg_num, sizeof(pkg_num));
		save_rssi = nsm_ble_rx_rssi_get();
		tx_payload[sizeof(nsm_frame_head_s) + 1 + sizeof(pkg_num)] = save_rssi;
	}

	/*TX Payload(5bytes): 
		*	byte0 : test result 1 fail or 0 sucess
		*	byte1-4 : correct package number
		*	byte5 : rx rssi
	*/
	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("RX read RESP ERROR!");
		return;
	}
}

void nsm_test_reduce_tx_stress(void)
{
	// example: 0xaaaa 0200 c800
	u8_t tx_payload[16];
	nsm_frame_head_s fhead;
	u16_t tx_len = 0;
	u8_t ret_code = 0;

	fhead.sync_byte1 = 0xAA;
	fhead.sync_byte2 = 0xAA;
	fhead.cmd = NSM_TEST_REDUCE_LE_TX_STRESS;
	fhead.f_len = sizeof(u16_t)+sizeof(u8_t);
	memcpy(tx_payload, &fhead, sizeof(nsm_frame_head_s));
	tx_len += (sizeof(nsm_frame_head_s)-sizeof(u16_t));
	tx_len += fhead.f_len;
	tx_payload[sizeof(nsm_frame_head_s)] = ret_code;
	if (tx_len != nsm_test_backend_write(tx_payload, tx_len, 100))
	{
		SYS_LOG_ERR("RX read RESP ERROR!");
		return;
	}

	nsm_ble_tx_stress_normal();
}

const nsm_recv_cmd_list_s nsm_recv_cmd_list[] = {
	{NSM_TEST_TX_START, (command_callback)nsm_test_tx_start},
	{NSM_TEST_RX_START, (command_callback)nsm_test_rx_start},
	{NSM_TEST_RX_READ, (command_callback)nsm_test_rx_read},
	{NSM_TEST_REDUCE_LE_TX_START, (command_callback)nsm_test_reduce_tx_start},
	{NSM_TEST_REDUCE_LE_RX_START, (command_callback)nsm_test_reduce_rx_start},
	{NSM_TEST_REDUCE_LE_RX_READ, (command_callback)nsm_test_reduce_rx_read},
	{NSM_TEST_REDUCE_LE_TX_STRESS, (command_callback)nsm_test_reduce_tx_stress},
};

u16_t nsm_cmd_xml_parse(void)
{
	int i = 0,j;
	u16_t ret_len = 0;
	nsm_frame_head_s fhead;
	u8_t read_buf[128];
	u8_t *ptr = (u8_t *)&fhead;

	ret_len = nsm_test_backend_read((uint8_t *)&fhead, sizeof(nsm_frame_head_s), 5000);

	printk("head:");
	for (j = 0;j < sizeof(nsm_frame_head_s);j++)
	{
		printk(" %x", ptr[j]);
	}
	printk("\n");

	if (sizeof(nsm_frame_head_s) != ret_len)
	{
		SYS_LOG_ERR("ret_len %d!", ret_len);
		return ret_len;
	}

	if ((0xAA != fhead.sync_byte1) || 
		(0xAA != fhead.sync_byte2) ||
		(fhead.f_len > 128) ||
		(fhead.f_len < 2))
	{
		SYS_LOG_ERR("head error %x %x %x!", 
			fhead.sync_byte1,fhead.sync_byte2,fhead.f_len);
		return (ret_len + sizeof(nsm_frame_head_s));
	}

	ret_len = nsm_test_backend_read(read_buf, fhead.f_len-2, 5000);
	if (fhead.f_len != ret_len+2)
	{
		SYS_LOG_ERR("ret_len %x f_len %x!", ret_len,fhead.f_len);
		return (ret_len + sizeof(nsm_frame_head_s));
	}

	for (i = 0 ; i < sizeof(nsm_recv_cmd_list) / sizeof(nsm_recv_cmd_list_s); i++) {
		if (nsm_recv_cmd_list[i].recv_cmd_type	== fhead.cmd) {
			SYS_LOG_INF(" nsm cmd 0x%x start!!", fhead.cmd);
			//print_hex("rxpayload", payload, payload_len);
			nsm_recv_cmd_list[i].cmd_cbk(read_buf, fhead.f_len-2);
			break;
		}
	}

	return (fhead.f_len-2 + sizeof(nsm_frame_head_s));
}
