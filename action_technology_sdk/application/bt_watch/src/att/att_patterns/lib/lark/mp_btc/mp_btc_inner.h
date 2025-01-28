#ifndef __MP_BTC_INNER_H
#define __MP_BTC_INNER_H

#define BT_RAM1_ADDR (0x01208000)
#define BT_RAM_TABLE_ADDR  (0x0120e000)
#define BT_RAM_PATCH_ADDR  (0x01201310)


/* Config pos from bt_table_config.txt */
#define BT_MAX_RF_POWER_POS_1 				0x22		/* tx_max_pwr_lvl */
#define BT_DEFAULT_TX_POWER					0x23
#define BT_MAX_RF_POWER_POS_2 				0x25		/* tws_max_pwr_lvl */
#define BT_CFG_WAKEUP_ADVANCE_POS			0x26
#define BT_BLE_RF_POWER_POS   				0x29		/* le_tx_pwr_lvl */
#define BT_CFG_FIX_MAX_PWR_POS				0x40
#define BT_CFG_FIX_MAX_PWR_BIT				0x3
#define BT_CFG_SEL_32768_POS				0x42
#define BT_CFG_SEL_32768_BIT				0x0			/* Set to 1 */
#define BT_CFG_FORCE_LIGHT_SLEEP_POS		0x42
#define BT_CFG_FORCE_LIGHT_SLEEP_BIT		0x5			/* Set to 0 */
#define BT_CFG_LE_USING_US_TIMER_POS		0x43
#define BT_CFG_LE_USING_US_TIMER_BIT		0x1			/* Power test set to 0, normal run set to 1, default value 1 */
#define BT_CFG_FORCE_UART_PRINT_POS			0x42
#define BT_CFG_FORCE_UART_PRINT_BIT			0x7			/* Set to 0 */
#define BT_CFG_TRANS_EN_POS					0x43
#define BT_CFG_TRANS_EN_BIT					0x5			/* Set to 1 */
#define BT_CFG_SEL_RC32K_POS				0x44
#define BT_CFG_SEL_RC32K_BIT				0x2			/* Set to 0 */
#define BT_CFG_UART_BAUD_RATE_POS			0x4C		/* 0x4c, 0x4d,0x4e,0x4f Little-endian */
#define BT_CFG_UART_BAUD_RATE				2000000		/* 2M */
#define BT_CFG_EFUSE_SET_POS				0x43
#define BT_CFG_EFUSE_SET_BIT				0x4
#define BT_CFG_EFUSE_AVDD_IF_POS			0x50
#define BT_CFG_EFUSE_POWER_VER_POS			0x54
#define BT_CFG_EFUSE_RF_POS					0x58

#define BT_DCDC_MODE 0

#define EFUSE_POWER_VER 0
#define EFUSE_RF		1
#define EFUSE_AVDD_IF	2

void ipmsg_btc_update_bt_table(void *table_addr);

void mp_btc_ipmsg_init(void);

int mp_btc_log_buf_process(void);

#endif
