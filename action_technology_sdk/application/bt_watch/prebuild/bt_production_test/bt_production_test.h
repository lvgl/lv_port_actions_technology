/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt production test headfile.
 */

#ifndef uint32
typedef unsigned int uint32;
#endif

enum {
	FCC_UART_MODE = 0,
	FCC_BT_TX_MODE = 1,
	FCC_BT_RX_MODE = 2,
	FCC_BT_ATT_MODE = 3,
	FCC_BT_EXIT = 4,
};

struct ft_env_var {
	void (*ft_printf)(const char *fmt, ...);
	void (*ft_udelay)(unsigned int us);
	void (*ft_mdelay)(unsigned int ms);
	uint32 (*ft_get_time_ms)(void);
	int (*ft_efuse_write_32bits)(uint32 bits, uint32 num);
	uint32 (*ft_efuse_read_32bits)(uint32 num, uint32* efuse_value);
	void (*ft_load_fcc_bin)(void);
};

extern struct ft_env_var global_ft_env_var;

/*************************************************
* Description: uart mode to set uart number
* uart_number : 0 or 1, default 0 , user UART0 or UART1
* uart_txio: gpio number to uart tx, default gpio 28
* uart_rxio: gpio number to uart rx, default gpio 29
****************************************************/
int fcc_test_uart_set(uint8_t uart_number, uint8_t uart_txio, uint8_t uart_rxio);

/*************************************************
* Description:  fcc test function entry
* Input: mode	0--uart test mode, for pcba
*				1--bt tx test mode, for demo
*				2--bt rx test mode, for demo
*                           3--bt att mode, for att test
* bt_param(For tx mode):
*					byte0 : bt_mode 0 or 1  //0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte2 : channel //tx channel     (0-79)
*					byte3 : tx_power_mode //tx power       (0-43)
*					byte4 : tx_mode //tx mode, DH1/DH3/DH5    (9-19, !=12)
*					byte5 : payload_mode //payload mode   (0-6)
*					byte6 : excute mode // excute mode (0-2)
*					byte7 : test time // unit : s
* bt_param(For rx mode):
*					byte0 : bt_mode 0 or 1  //0: BR/EDR TEST, 1: BLE TEST
*					byte1 : BLE_PHY 0 or 1 //0: BLE 1M, 1: BLE 2M
*					byte[2-5]:  access code
*					byte6 : channel //rx channel     (0-79)
*					byte7 : rx_mode //rx mode (0~11, 0x10~0x12)
*					byte8 : excute mode // excute mode, 0: one packet; 1:continue
*					byte9 : test time // unit : s
* rx_report(Only for rx mode):
*					buffer[16]: 16byte report.
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_main(uint8_t mode, uint8_t *bt_param, uint8_t *rx_report);

/*************************************************
* Description:  fcc test send cmd and parameter, only use in BT_ATT_MODE
* Input: cmd    buffer store command and paremeter;
* Input: len       command and paremeter length;
* Input: wait_finish  1: wait cmd send finish, 0: not wait;
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_send_cmd(uint8_t *cmd, uint8_t len, uint8_t wait_finish);

/*************************************************
* Description:  fcc test clear rx buffer, only use in BT_ATT_MODE
* Return:  None
****************************************************/
void fcc_test_clear_rx_buf(void);

/*************************************************
* Description:  fcc test get report, only use in BT_ATT_MODE
* Input: cmd    get report command;
* Output: rx_report   buffer for output report;
* Input: report_len  get report length;
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_get_report(uint8_t cmd, uint8_t *rx_report, uint8_t report_len);

/*************************************************
* Description:  fcc test deinit, only use in BT_ATT_MODE
* Return:  0:success; 1:failed
****************************************************/
int fcc_test_deinit(void);

/* Use sample code*/
#if FCC_TEST_SAMPLE_CODE
#include "code_in_btram0.h"

/*************************************************
* Description:  Load bt bin
* Return:  None
****************************************************/
void ft_load_fcc_bin(void)
{
	unsigned int i, *p
	p = (unsigned int *)0x2FF20000; 			/* Leopard BT_RAM0 */

	for(i=0; i<sizeof(code_in_btram0)/4; i++) {
		*(p+i) = code_in_btram0[i];
	}
}

/* FCC_UART_MODE */
{
	int result;

	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	global_ft_env_var.ft_efuse_write_32bits = xxx;		/* Better privide, if not, use libbt_test.a inner function */
	global_ft_env_var.ft_efuse_read_32bits = yyy;			/* Better privide, if not, use libbt_test.a inner function */

	result = fcc_test_main(FCC_UART_MODE, NULL, NULL);
}

/* FCC_BT_TX_MODE */
{
	int result;
	uint8_t tx_param[8];

	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	global_ft_env_var.ft_efuse_write_32bits = xxx;		/* Better privide, if not, use libbt_test.a inner function */
	global_ft_env_var.ft_efuse_read_32bits = yyy;			/* Better privide, if not, use libbt_test.a inner function */

	tx_param[0] = 1;	/* ble */
	tx_param[1] = 0;	/* 1M */
	tx_param[2] = 0;	/* channel 0 */
	tx_param[3] = 38;	/* 8dbm */
	tx_param[4] = 22;	/* tx mode K_LE_01010101 */
	tx_param[5] = 0;	/* payload_mode K_FCC_PN9 */
	tx_param[6] = 1;	/* continue */
	tx_param[7] = 2;	/* 2s */

	result = fcc_test_main(FCC_BT_TX_MODE, tx_param, NULL);
}

/* FCC_BT_RX_MODE */
{
	int result;
	uint8_t rx_param[10], report[16];

	memset(&global_ft_env_var, 0, sizeof(global_ft_env_var));
	global_ft_env_var.ft_load_fcc_bin = ft_load_fcc_bin;
	global_ft_env_var.ft_efuse_write_32bits = xxx;		/* Better privide, if not, use libbt_test.a inner function */
	global_ft_env_var.ft_efuse_read_32bits = yyy;			/* Better privide, if not, use libbt_test.a inner function */

	rx_param[0] = 1;	/* ble */
	rx_param[1] = 0;	/* 1M */
	rx_param[2] = 0x29;	/* access code, default: 0x71764129 */
	rx_param[3] = 0x41;	/* access code */
	rx_param[4] = 0x76;	/* access code */
	rx_param[5] = 0x71;	/* access code */
	rx_param[6] = 0;	/* channel 0 */
	rx_param[7] = 0x12;	/* rx mode K_RX_MODE_LE_01010101 */
	rx_param[8] = 1;	/* continue */
	rx_param[9] = 2;	/* 2s */

	result = fcc_test_main(FCC_BT_RX_MODE, rx_param, report);
}

/* FCC_BT_ATT_MODE */
/* Refer src\att\att_patterns\group1\mp_test\mp_lib\ */
#endif
