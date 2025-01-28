/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <string.h>
#include <device.h>
#include <soc.h>
#include "soc_memctrl.h"
#include <drivers/ipmsg.h>
#include <sdfs.h>
#include <board_cfg.h>
#include <soc_atp.h>
#include <sys/byteorder.h>


#define DEBUGSEL (0x40068410)
#define DEBUGOE0 (0x40068430)

#define HOSC_CTL_HOSC_CAP(x)	 ((x) << HOSC_CTL_HOSC_CAP_SHIFT)

/* LOSC_CTL */
#define LOSC_CTL_LOSC_RDY	BIT(15)


/* CMU_MEMCLKEN1 */
#define BT_ROM_RAM_CLK_EN	BIT(16)

/* CMU_MEMCLKSRC1 */
#define BT_RAM_CLK_SRC		BIT(16)

/* CMU_S1CLKCTL */
#define RC64M_S1EN			BIT(2)
#define HOSC_S1EN			BIT(1)
#define RC4M_S1EN			BIT(0)

/* CMU_S1BTCLKCTL */
#define RC64M_S1BTEN		BIT(2)
#define HOSC_S1BTEN			BIT(1)
#define RC4M_S1BTEN			BIT(0)

/* CMU_DEVCLKEN1 */
#define BTHUB_RC32KEN		BIT(28)
#define BTHUB_RC64MEN		BIT(27)
#define BTHUB_HOSCEN  		BIT(26)
#define BTHUB_RC4MEN  		BIT(25)
#define BTHUB_LOSCEN  		BIT(24)

#define JTAG_CTL_BTSWEN		BIT(20)

/* DEBUGSEL */
#define DBGSE_SHIFT			0
#define DBGSE(x)			((x) << DBGSE_SHIFT)
#define DBGSE_BT_CONTROLLER DBGSE(0x29)


#if defined(CONFIG_BTC_FW_IN_NAND)
#define BT_RF_FILE         "/NAND:K/bt_rf.bin"
#define BT_TABLE_FILE      "/NAND:K/bttbl.bin"
#define BT_PATCH_FILE      "/NAND:K/bt_pth.bin"
#define BT_FCC_FILE        "/NAND:K/fcc.bin"
#define BT_ROM_FILE        "/NAND:K/bt_rom.bin"
#elif  defined(CONFIG_BTC_FW_IN_SD)
#define BT_RF_FILE         "/SD:K/bt_rf.bin"
#define BT_TABLE_FILE      "/SD:K/bttbl.bin"
#define BT_PATCH_FILE      "/SD:K/bt_pth.bin"
#define BT_FCC_FILE        "/SD:K/fcc.bin"
#define BT_ROM_FILE        "/SD:K/bt_rom.bin"
#elif  defined(CONFIG_BTC_FW_IN_NOR_EXT)
#define BT_RF_FILE         "/NOR:K/bt_rf.bin"
#define BT_TABLE_FILE      "/NOR:K/bttbl.bin"
#define BT_PATCH_FILE      "/NOR:K/bt_pth.bin"
#define BT_FCC_FILE        "/NOR:K/fcc.bin"
#define BT_ROM_FILE        "/NOR:K/bt_rom.bin"
#else
#define BT_RF_FILE         "bt_rf.bin"
#define BT_TABLE_FILE      "bttbl.bin"
#define BT_PATCH_FILE      "bt_pth.bin"
#define BT_FCC_FILE        "fcc.bin"
#define BT_ROM_FILE        "bt_rom.bin"

#endif

//#define BT_RAM1_ADDR (0x01208000)
//#define BT_RAM_TABLE_ADDR  (0x01200000)
//#define BT_RAM_PATCH_ADDR  (0x01201310)
#define BT_RAM1_ADDR        (0x2FF26000)
#define BT_RAM_TABLE_ADDR   (0x2FF20000)
#define BT_RAM_PATCH_ADDR   (0x2FF21400)
#define BT_ROM_ADDR         (0x2FE80000)



/* Config pos from bt_table_config.txt */
#define BT_RF_FUNC_ADDR_POS					0x1c
#define BT_MAX_RF_POWER_POS_1 				0x22		/* tx_max_pwr_lvl */
#define BT_DEFAULT_TX_POWER					0x23
#define BT_MAX_RF_POWER_POS_2 				0x25		/* tws_max_pwr_lvl */
#define BT_CFG_WAKEUP_ADVANCE_POS			0x26
#define BT_BLE_RF_POWER_POS   				0x29		/* le_tx_pwr_lvl */
#define BT_DUT_MODE							0x2b
#define BT_CFG_FIX_MAX_PWR_POS				0x40
#define BT_CFG_FIX_MAX_PWR_BIT				0x3
#define BT_CFG_DISABLE_3M_POS               0x41
#define BT_CFG_DISABLE_3M_BIT               0x1
#define BT_CFG_SEL_32768_POS				0x42
#define BT_CFG_SEL_32768_BIT				0x0			/* Set to 1 */
#define BT_CFG_INIT_K192_BY_HOST_POS		0x42
#define BT_CFG_INIT_K192_BY_HOST_BIT		0x1
#define BT_CFG_FORCE_LIGHT_SLEEP_POS		0x42
#define BT_CFG_FORCE_LIGHT_SLEEP_BIT		0x5			/* Set to 0 */
#define BT_CFG_FORCE_UART_PRINT_POS			0x42
#define BT_CFG_FORCE_UART_PRINT_BIT			0x7			/* Set to 0 */
#define BT_CFG_LE_USING_US_TIMER_POS		0x43
#define BT_CFG_LE_USING_US_TIMER_BIT		0x1			/* Power test set to 0, normal run set to 1, default value 1 */
#define BT_CFG_BT_FAST_BOOT_POS				0x43
#define BT_CFG_BT_FAST_BOOT_BIT				0x2
#define BT_CFG_EFUSE_SET_POS				0x43
#define BT_CFG_EFUSE_SET_BIT				0x4
#define BT_CFG_TRANS_EN_POS					0x43
#define BT_CFG_TRANS_EN_BIT					0x5			/* Set to 1 */
#define BT_CFG_SEL_RC32K_POS				0x44
#define BT_CFG_SEL_RC32K_BIT				0x2			/* Set to 0 */
#define BT_CFG_BQB_DEFAULT_MODE_POS			0x44
#define BT_CFG_BQB_DEFAULT_MODE_BIT			0x4
#define BT_CFG_UART_BAUD_RATE_POS			0x4C		/* 0x4c, 0x4d,0x4e,0x4f Little-endian */
#define BT_CFG_EFUSE_AVDD_IF_POS			0x50
#define BT_CFG_EFUSE_POWER_VER_POS			0x54
#define BT_CFG_EFUSE_RF_POS					0x58
#define BT_CFG_IC_PKG_POS					0xB6
#define BT_CFG_UART_BAUD_RATE				2000000		/* 2M */
#define BT_CFG_VD12_SET_S1BT				0xb5

#define EFUSE_POWER_VER 0
#define EFUSE_RF		1
#define EFUSE_AVDD_IF	2

struct ipmsg_btc_config {
	void *mem_base;
	uint32_t mem_size;
};

struct ipmsg_btc_data {
	ipmsg_callback_t btc_cb;
	ipmsg_callback_t tws0_cb;
	ipmsg_callback_t tws1_cb;
	ipmsg_pm_ctrl_callback_t pm_ctrl_cb;
};

static ipmsg_btc_init_param_t btc_set_param;

DEVICE_DECLARE(btc);

#ifdef CONFIG_BT_CTRL_DEBUG
#ifndef CONFIG_BT_CTRL_LOG
#define BT_UART_MFP_SEL 23
static const struct acts_pin_config btc_pin_cfg_uart[] = {
	{14, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{15, BT_UART_MFP_SEL | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};
#endif
#endif

static void ipmsg_btc_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;
	
	//clear irq pending
	sys_write32(1, PENDING_FROM_BT_CPU);

	if (data->btc_cb) {
		data->btc_cb(NULL, NULL);
	}
}

static void ipmsg_tws0_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;

	if (data->tws0_cb) {
		data->tws0_cb(NULL, NULL);
	}
}

static void ipmsg_tws1_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;

	if (data->tws1_cb) {
		data->tws1_cb(NULL, NULL);
	}
}

static void ipmsg_btc_init_param(struct device *dev, void *param)
{
	memcpy(&btc_set_param, param, sizeof(btc_set_param));
}

static int sd_load(const char *filename, void *dst)
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

#ifdef CONFIG_SD_FS
extern bool bt_bqb_is_in_test(void);

static inline void bttbl_set_bit(uint8_t *start, uint16_t pos, uint8_t bit)
{
	start[pos] |= (0x1 << bit);
}

static inline void bttbl_clear_bit(uint8_t *start, uint16_t pos, uint8_t bit)
{
	start[pos] &= (~(0x1 << bit));
}

static inline void bttbl_set_value_u8t(uint8_t *start, uint16_t pos, uint8_t value)
{
	start[pos] = value;
}

static inline void bttbl_set_value_u32t(uint8_t *start, uint16_t pos, uint32_t value)
{
	sys_put_le32(value ,&start[pos]);
}

static void ipmsg_btc_update_bt_table(void *table_addr)
{
	int err;
	uint32_t calib_val = 0;
	uint8_t *start = table_addr;
	int opt = soc_dvfs_opt();
	int valid_number = 0;
	printk("opt:%d Befor set 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", opt,start[0x42], start[0x43], start[0x44]);

	err = soc_atp_get_rf_calib(EFUSE_AVDD_IF, &calib_val);
	printk("get efuse avdd ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_AVDD_IF_POS]);
	if (err == 0) {
		bttbl_set_value_u32t(start, BT_CFG_EFUSE_AVDD_IF_POS, calib_val);
		valid_number ++;
	}

	err = soc_atp_get_rf_calib(EFUSE_POWER_VER, &calib_val);
	printk("get efuse power version ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_POWER_VER_POS]);
	if (err == 0) {
		bttbl_set_value_u32t(start, BT_CFG_EFUSE_POWER_VER_POS, calib_val);
		valid_number ++;
	}

	err = soc_atp_get_rf_calib(EFUSE_RF, &calib_val);
	printk("get efuse rf ret:%d value:%x default %x\n", err,calib_val,start[BT_CFG_EFUSE_RF_POS]);
	if (err == 0) {
		bttbl_set_value_u32t(start, BT_CFG_EFUSE_RF_POS, calib_val);
		valid_number ++;
	}

	if (valid_number) {
		bttbl_set_bit(start, BT_CFG_EFUSE_SET_POS, BT_CFG_EFUSE_SET_BIT);
	}

#ifdef CONFIG_BT_CTRL_RF_DEBUG
    /* 0x106000, bt_rf start address */
    bttbl_set_value_u32t(start, BT_RF_FUNC_ADDR_POS, 0x106000);
#endif

#ifdef CONFIG_IPMSG_BTC_SEL_32K /*ext 32 losc*/
	bttbl_set_bit(start, BT_CFG_SEL_32768_POS, BT_CFG_SEL_32768_BIT);
#else
	#if 1 	/* Use RC32K */
	bttbl_set_bit(start, BT_CFG_SEL_32768_POS, BT_CFG_SEL_32768_BIT);
	bttbl_set_bit(start, BT_CFG_SEL_RC32K_POS, BT_CFG_SEL_RC32K_BIT);	
	#else  /*hcl */
	bttbl_clear_bit(start, BT_CFG_SEL_32768_POS, BT_CFG_SEL_32768_BIT);
	bttbl_clear_bit(start, BT_CFG_SEL_RC32K_POS, BT_CFG_SEL_RC32K_BIT);
	#endif
#endif

#ifdef CONFIG_BT_ECC_ACTS
	bttbl_set_bit(start, BT_CFG_INIT_K192_BY_HOST_POS, BT_CFG_INIT_K192_BY_HOST_BIT);
#else
	bttbl_clear_bit(start, BT_CFG_INIT_K192_BY_HOST_POS, BT_CFG_INIT_K192_BY_HOST_BIT);
#endif

	bttbl_clear_bit(start, BT_CFG_FORCE_LIGHT_SLEEP_POS, BT_CFG_FORCE_LIGHT_SLEEP_BIT);

#ifdef CONFIG_BT_CTRL_LOG
	bttbl_clear_bit(start, BT_CFG_FORCE_UART_PRINT_POS, BT_CFG_FORCE_UART_PRINT_BIT);
#else
	bttbl_set_bit(start, BT_CFG_FORCE_UART_PRINT_POS, BT_CFG_FORCE_UART_PRINT_BIT);
#endif

#ifdef CONFIG_BT_TRANSMIT
	bttbl_set_bit(start, BT_CFG_TRANS_EN_POS, BT_CFG_TRANS_EN_BIT);
#endif

#ifndef CONFIG_BOARD_LARK_DVB_EARPHONE
	bttbl_set_value_u32t(start, BT_CFG_UART_BAUD_RATE_POS, BT_CFG_UART_BAUD_RATE);
	printk("Bt uart rate %d\n", BT_CFG_UART_BAUD_RATE);
#endif

	if (btc_set_param.set_max_rf_power) {
		bttbl_set_value_u8t(start, BT_MAX_RF_POWER_POS_1, btc_set_param.bt_max_rf_tx_power);
		bttbl_set_value_u8t(start, BT_MAX_RF_POWER_POS_2, btc_set_param.bt_max_rf_tx_power);
		printk("max rf power %d\n", btc_set_param.bt_max_rf_tx_power);

#if 0
		/* Fixed tx power */
		#define BT_FIX_POWER_LEVEL	38		/* 8db */
		bttbl_set_value_u8t(start, BT_MAX_RF_POWER_POS_1, BT_FIX_POWER_LEVEL);
		bttbl_set_value_u8t(start, BT_MAX_RF_POWER_POS_2, BT_FIX_POWER_LEVEL);
		bttbl_set_value_u8t(start, BT_DEFAULT_TX_POWER, BT_FIX_POWER_LEVEL);
		bttbl_set_bit(start, BT_CFG_FIX_MAX_PWR_POS, BT_CFG_FIX_MAX_PWR_BIT);
		printk("After set 0x22 = %d 0x23 = %d 0x25 = %d\n", start[0x22], start[0x23], start[0x25]);
		printk("After set 0x40 = 0x%x\n", start[BT_CFG_FIX_MAX_PWR_POS]);
#endif
	}

	if (btc_set_param.set_ble_rf_power) {
		bttbl_set_value_u8t(start, BT_BLE_RF_POWER_POS, btc_set_param.ble_rf_tx_power);
		printk("ble rf tx power %d\n", start[BT_BLE_RF_POWER_POS]);
	}

#ifdef CONFIG_BT_3M
	bttbl_clear_bit(start, BT_CFG_DISABLE_3M_POS, BT_CFG_DISABLE_3M_BIT);
#endif

#ifdef CONFIG_BT_CTRL_BQB
	if (bt_bqb_is_in_test()) {
		bttbl_clear_bit(start, BT_CFG_DISABLE_3M_POS, BT_CFG_DISABLE_3M_BIT);
		bttbl_set_bit(start, BT_CFG_BQB_DEFAULT_MODE_POS, BT_CFG_BQB_DEFAULT_MODE_BIT);
		bttbl_set_value_u8t(start, BT_DUT_MODE, 0x01);
		bttbl_set_value_u8t(start, BT_CFG_VD12_SET_S1BT, 0x0E); // VD12 set 1.3V to bt core.
	}
#endif

#ifdef CONFIG_BT_FAST_BOOT
	if (soc_dvfs_opt())
		bttbl_set_bit(start, BT_CFG_BT_FAST_BOOT_POS, BT_CFG_BT_FAST_BOOT_BIT);
#endif

	bttbl_set_value_u8t(start, BT_CFG_IC_PKG_POS, ipmsg_btc_get_ic_pkt());

	printk("After set 0x42 = 0x%x 0x43 = 0x%x 0x44 = 0x%x\n", start[0x42], start[0x43], start[0x44]);
}
#endif

static int ipmsg_btc_load(struct device *dev, void *data, uint32_t size)
{
	uint32_t val;
	int err = 0;
	const struct ipmsg_btc_config *config = dev->config;

	if (size > config->mem_size) {
		return -EINVAL;
	}

	val = sys_read32(CMU_MEMCLKSRC1) & ~BT_RAM_CLK_SRC;
	sys_write32(val, CMU_MEMCLKSRC1);

#ifdef CONFIG_SD_FS

#if (CONFIG_BT_FCC_TEST == 1 || CONFIG_BT_SRRC_TEST == 1)
	#define BT_RAM_FCC_ADDR 	BT_RAM_TABLE_ADDR

	sys_write32((sys_read32(MEMORYCTL) | (0x1<<4)), MEMORYCTL);	// bt cpu boot from btram0
	k_sleep(K_MSEC(1));
#ifdef CONFIG_BT_FCC_TEST
	sd_load(CONFIG_IPMSG_BTC_FCC_NAME, (void *)BT_RAM_FCC_ADDR);  /* load bt config table */
#else
	sd_load(CONFIG_IPMSG_BTC_SRRC_NAME, (void *)BT_RAM_FCC_ADDR);  /* load bt config table */
#endif
	k_sleep(K_MSEC(1));
	return err;
#endif

#ifdef CONFIG_BT_CTRL_RF_DEBUG
	err = sd_load(BT_RF_FILE, (void *)BT_RAM1_ADDR);
	if (err) {
		return -EINVAL;
	}
#endif

	err = sd_load(BT_TABLE_FILE, (void *)BT_RAM_TABLE_ADDR);  /* load bt config table */
	ipmsg_btc_update_bt_table((void *)BT_RAM_TABLE_ADDR);
	err |= sd_load(BT_PATCH_FILE, (void *)BT_RAM_PATCH_ADDR);  /* load bt patch */
#else
	memcpy(config->mem_base, data, size);
#endif

	return err;
}

static void ipmsg_btc_soc_set_hoscldo_ctl(void)
{
	int val, i, j, cnt;

	sys_write32(0x80100204, HOSCLDO_CTL);
	k_sleep(K_MSEC(10));
	val = sys_read32(HOSCLDO_CTL) | (0x1 << HOSCLDO_CTL_OSC32M_EN);
	sys_write32(val, HOSCLDO_CTL);
	k_sleep(K_MSEC(10));
	val = sys_read32(HOSCLDO_CTL) | (0x1 << HOSCLDO_CTL_OSC32M_CALEN) | (0x1 << HOSCLDO_CTL_OSC32M_CALMODE);
	sys_write32(val, HOSCLDO_CTL);

	i = 0;
	while ((sys_read32(HOSCLDO_CTL) & (0x1 << HOSCLDO_CTL_OSC32M_CALDONE))) {
		if (i++ > 10) {
			printk("HOSCLDO_CTL OSC32M wait 0 timeout\n");
			break;
		}

		k_sleep(K_MSEC(1));
	}

	i = 0;
	while (1) {
		cnt = 0;
		for (j=0; j<3; j++) {
			if (sys_read32(HOSCLDO_CTL) & (0x1 << HOSCLDO_CTL_OSC32M_CALDONE)) {
				cnt++;
			}
			k_busy_wait(10);
		}

		if (cnt == 3) {
			break;
		}

		if (i++ > 50) {
			printk("HOSCLDO_CTL OSC32M wait 1 timeout\n");
			break;
		}
		k_sleep(K_MSEC(1));
	}

	val = sys_read32(HOSCLDO_CTL) & HOSCLDO_CTL_OSC32M_FRQCAL_MASK;
	val >>= HOSCLDO_CTL_OSC32M_FRQCAL_SHIFT;
	val <<= HOSCLDO_CTL_OSC32M_FRQMSET_SHIFT;
	val |= 0x80000204;
	sys_write32(val, HOSCLDO_CTL);
}

void ipmsg_btc_soc_set_hosc_cap(int cap)
{
	int val;

	printk("Set hosc cap %d.%d pf\n", cap / 10, cap % 10);

	val = sys_read32(HOSC_CTL);
	val &= (~HOSC_CTL_HOSC_CAP_MASK);
	val |= HOSC_CTL_HOSC_CAP(cap);
	sys_write32(val, HOSC_CTL);

	ipmsg_btc_soc_set_hoscldo_ctl();
	printk("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));
}

/* Start BT CPU */
static inline int ipmsg_btc_start(struct device *dev, uint32_t *send_id, uint32_t *recv_id)
{
	uint32_t val;

	ARG_UNUSED(dev);

	/* System runing, some bit can't change in HOSC_CTL, init in leopard_init */
	//sys_write32(0xa01f9b2, HOSC_CTL);

	if (btc_set_param.set_hosc_cap) {
		ipmsg_btc_soc_set_hosc_cap((int)btc_set_param.hosc_capacity);
	} else {
		/* Default cap 0x55 */
		val = sys_read32(HOSC_CTL) | HOSC_CTL_HOSC_CAP(0x55);
		sys_write32(val, HOSC_CTL);
		ipmsg_btc_soc_set_hoscldo_ctl();
		printk("HOSC_CTL = 0x%x HOSCLDO_CTL = 0x%x\n", sys_read32(HOSC_CTL), sys_read32(HOSCLDO_CTL));
	}

#ifdef CONFIG_IPMSG_BTC_SEL_32K
	sys_write32(0x5515FB, LOSC_CTL);

	int i = 0;
	while (!(sys_read32(LOSC_CTL) & LOSC_CTL_LOSC_RDY)) {
		if (i++ > 500) {
			printk("LOSC_CTL not ready!\n");
			break;
		}

		k_sleep(K_MSEC(1));
	}
#else
	sys_write32(0x951, HOSCOK_CTL);
#endif

	val = sys_read32(CMU_MEMCLKSRC1) | BT_RAM_CLK_SRC;
	sys_write32(val, CMU_MEMCLKSRC1);

	val = sys_read32(CMU_MEMCLKEN1) | BT_ROM_RAM_CLK_EN;
	sys_write32(val, CMU_MEMCLKEN1);

	val = sys_read32(CMU_DEVCLKEN1) | BTHUB_RC32KEN | BTHUB_RC64MEN | BTHUB_HOSCEN |
						BTHUB_RC4MEN | BTHUB_LOSCEN;
	sys_write32(val, CMU_DEVCLKEN1);

#if (CONFIG_BT_FCC_TEST == 1 || CONFIG_BT_SRRC_TEST == 1)
	val = sys_read32(CMU_S1CLKCTL) | RC64M_S1EN | HOSC_S1EN | RC4M_S1EN;
	sys_write32(val, CMU_S1CLKCTL);
	val = sys_read32(CMU_S1BTCLKCTL) | RC64M_S1BTEN | HOSC_S1BTEN | RC4M_S1BTEN;
	sys_write32(val, CMU_S1BTCLKCTL);
#else
	val = sys_read32(CMU_S1CLKCTL) | HOSC_S1EN | RC4M_S1EN;
	sys_write32(val, CMU_S1CLKCTL);
	val = sys_read32(CMU_S1BTCLKCTL) | HOSC_S1BTEN | RC4M_S1BTEN;
	sys_write32(val, CMU_S1BTCLKCTL);
#endif

#ifdef CONFIG_BT_CTRL_DEBUG
#ifdef CONFIG_BT_CTRL_BQB
	extern bool bt_bqb_is_in_test(void);
	if (!bt_bqb_is_in_test())
#endif
	{
		sys_write32(DBGSE_BT_CONTROLLER, DEBUGSEL);
		sys_write32(0x3C000, DEBUGOE0);
		//sys_write32(0xF03C000, DEBUGOE0);
	}

#ifndef CONFIG_BT_CTRL_LOG
	acts_pinmux_setup_pins(btc_pin_cfg_uart, ARRAY_SIZE(btc_pin_cfg_uart));
#endif
#endif

	/* BT sleep clock RC4M/4, RAM can deepsleep */
	//sys_clear_bit(PWRGATE_DIG, 29); // RAM_BT_FORCE=0
	//sys_clear_bit(PWRGATE_DIG, 27); // BT_FORCE=0

#ifdef CONFIG_BT_FAST_BOOT
	if (soc_dvfs_opt())
		soc_powergate_set(POWERGATE_BT_PG_DEV, false);
	else
		soc_powergate_set(POWERGATE_BT_PG_DEV, true);
#else
	soc_powergate_set(POWERGATE_BT_PG_DEV, true);
#endif

	/* BT RAM0/1 deepsleep */
	sys_write32(sys_read32(RAM_DEEPSLEEP) | 0x03000000, RAM_DEEPSLEEP); 

	acts_reset_peripheral(RESET_ID_BT);
	irq_enable(IRQ_ID_BT);

	return 0;
}

static int ipmsg_btc_stop(struct device *dev)
{
	uint32_t val;

	ARG_UNUSED(dev);

	// disable irq
	irq_disable(IRQ_ID_BT);

	// reset btc
	acts_reset_peripheral_assert(RESET_ID_BT);

	// reset BTRAM clock src
	val = sys_read32(CMU_MEMCLKSRC1) & ~BT_RAM_CLK_SRC;
	sys_write32(val, CMU_MEMCLKSRC1);

	return 0;
}

static int ipmsg_btc_notify(struct device *dev)
{
	ARG_UNUSED(dev);
	volatile uint8_t i = 5;

	// send interrupt to btc
	sys_write32(0, INT_TO_BT_CPU);
	while(i--);
	sys_write32(1, INT_TO_BT_CPU);

	return 0;
}

static void ipmsg_btc_register_callback(struct device *dev,
					 ipmsg_callback_t cb, void *context)
{
	struct ipmsg_btc_data *data = dev->data;
	uint8_t irq = *((uint8_t *)context);

	switch (irq) {
	case IPMSG_BTC_IRQ:
		data->btc_cb = cb;
		break;
	case IPMSG_TWS0_IRQ:
		data->tws0_cb = cb;
		break;
	case IPMSG_TWS1_IRQ:
		data->tws1_cb = cb;
		break;
	case IPMSG_REG_PW_CTRL:
		data->pm_ctrl_cb = (ipmsg_pm_ctrl_callback_t)cb;
		break;
	default:
		printk("Unknown irq %d\n", irq);
		break;
	}
}

static void ipmsg_btc_clear_irq_pending(int32_t irq)
{
	if (irq < 32) {
		sys_write32((0x1 << irq), NVIC_ICPR0);
	} else {
		sys_write32((0x1 << (irq - 32)), NVIC_ICPR1);
	}
}

static void ipmsg_btc_irq_enable(struct device *dev, uint8_t irq)
{
	switch (irq) {
	case IPMSG_BTC_IRQ:
		irq_enable(IRQ_ID_BT);
		break;
	case IPMSG_TWS0_IRQ:
		ipmsg_btc_clear_irq_pending(IRQ_ID_TWS);
		irq_enable(IRQ_ID_TWS);
		break;
	case IPMSG_TWS1_IRQ:
		ipmsg_btc_clear_irq_pending(IRQ_ID_TWS1);
		irq_enable(IRQ_ID_TWS1);
		break;
	}
}

static void ipmsg_btc_irq_disable(struct device *dev, uint8_t irq)
{
	switch (irq) {
	case IPMSG_BTC_IRQ:
		irq_disable(IRQ_ID_BT);
		break;
	case IPMSG_TWS0_IRQ:
		irq_disable(IRQ_ID_TWS);
		break;
	case IPMSG_TWS1_IRQ:
		irq_disable(IRQ_ID_TWS1);
		break;
	}
}

static int ipmsg_btc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* btc irq init */
	IRQ_CONNECT(IRQ_ID_BT, CONFIG_BTC_IRQ_PRI,
			ipmsg_btc_isr, DEVICE_GET(btc), 0);

	/* tws irq init */
	IRQ_CONNECT(IRQ_ID_TWS, CONFIG_TWS_IRQ_PRI,
			ipmsg_tws0_isr, DEVICE_GET(btc), 0);
	IRQ_CONNECT(IRQ_ID_TWS1, CONFIG_TWS_IRQ_PRI,
			ipmsg_tws1_isr, DEVICE_GET(btc), 0);

	return 0;
}


#ifdef CONFIG_PM_DEVICE
int ipmsg_btc_pm_control(const struct device *device, enum pm_device_action action)
{
	int ret;
	struct ipmsg_btc_data *data = device->data;



	if (data && data->pm_ctrl_cb) {
		data->pm_ctrl_cb(0, action);
	}

	switch (action) {
    case PM_DEVICE_ACTION_RESUME: 
        break;
    case PM_DEVICE_ACTION_SUSPEND:
        break;
	case PM_DEVICE_ACTION_EARLY_SUSPEND:
		break;
	case PM_DEVICE_ACTION_LATE_RESUME:
		break;
	default:
		ret = -EINVAL;
	}
	return 0;
}
#else
#define ipmsg_btc_pm_control 	NULL
#endif

static const struct ipmsg_driver_api ipmsg_btc_driver_api = {
	.init_param = ipmsg_btc_init_param,
	.load = ipmsg_btc_load,
	.start = ipmsg_btc_start,
	.stop = ipmsg_btc_stop,
	.notify = ipmsg_btc_notify,
	.register_callback = ipmsg_btc_register_callback,
	.enable = ipmsg_btc_irq_enable,
	.disable = ipmsg_btc_irq_disable,
};

static const struct ipmsg_btc_config ipmsg_btc_cfg = {
	.mem_base = (void *)BTC_REG_BASE,
	.mem_size = 0x40000,
};

static struct ipmsg_btc_data ipmsg_btc_dat;

DEVICE_DEFINE(btc, CONFIG_BTC_NAME,
			&ipmsg_btc_init, ipmsg_btc_pm_control,
			&ipmsg_btc_dat, &ipmsg_btc_cfg,
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			&ipmsg_btc_driver_api);

