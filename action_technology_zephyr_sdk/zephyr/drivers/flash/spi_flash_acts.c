/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include <board_cfg.h>
#include "spi_flash.h"
#include <linker/linker-defs.h>
#include <dvfs.h>
#include <drivers/gpio.h>


LOG_MODULE_REGISTER(spi_flash_acts, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_SPI_NOR_FLASH_DRV_EXT
extern int spinor_enter_4byte_address_mode(struct spinor_info *sni);
#endif

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
static const struct device *spi_gpio_cs_dev;
#endif

#if (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
static struct k_sem spi_flash_sync = Z_SEM_INITIALIZER(spi_flash_sync, 1, 1);
#endif

#ifdef SPINOR_RESET_FUN_ADDR
typedef void (*spi_reset_func)(struct spi_info *si);
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	spi_reset_func func = (spi_reset_func)(SPINOR_RESET_FUN_ADDR);
	func(si);
}
#else
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	p_spinor_api->continuous_read_reset((struct spinor_info *)si);
}
#endif

__ramfunc  void spi_flash_acts_prepare(struct spi_info *si)
{
	/* wait for spi ready */
#if !defined(CONFIG_SPI_NOR_FLASH_4B_ADDRESS) || defined(CONFIG_NOR_CODE_IN_RAM)
	while(!(sys_read32(SPI_STA(si->base)) & SPI_STA_READY));
	spi_flash_reset(si);
#endif
}

__ramfunc void spi_flash_lock_acquire(void)
{
#ifndef	CONFIG_NOR_CODE_IN_RAM
#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	if(!k_is_in_isr()){
		k_sem_take(&spi_flash_sync, K_FOREVER);
	}
#endif
#endif
}

__ramfunc void spi_flash_lock_release(void)
{
#ifndef	CONFIG_NOR_CODE_IN_RAM

#if defined(CONFIG_SPI_FLASH_SYNC_MULTI_DEV) && (CONFIG_SPI_FLASH_SYNC_MULTI_DEV == 1)
	if(!k_is_in_isr()){
		k_sem_give(&spi_flash_sync);
	}
#endif
#endif
}

__ramfunc int spi_flash_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;

	spi_flash_lock_acquire();

    tmplen = len;
    while(tmplen > 0) {
        if(tmplen <  0x8000)
            len = tmplen;
        else
            len = 0x8000;

#if defined(CONFIG_SPI_NOR_FLASH_4B_ADDRESS) || defined(CONFIG_NOR_CODE_IN_RAM)
        ret = spinor_4b_addr_op_api.read(sni, offset, data, len);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
        ret = p_spinor_api->read(sni, offset, data, len);
#else
        uint32_t key = irq_lock();
        ret = p_spinor_api->read(sni, offset, data, len);
        irq_unlock(key);
#endif
#endif
        offset += len;
        data  = (void *)((unsigned int )data + len);
        tmplen -= len;
    }

	spi_flash_lock_release();

    return ret;
}


__ramfunc int spi_flash_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;

	spi_flash_lock_acquire();

#if defined(CONFIG_SPI_NOR_FLASH_4B_ADDRESS) || defined(CONFIG_NOR_CODE_IN_RAM)
	ret = spinor_4b_addr_op_api.write(sni, offset, data, len);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
	uint32_t flag = sni->spi.flag;
	uint32_t nor_flag = sni->flag;
	sni->flag |= SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY; //unlock wait ready
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //lock
	ret = p_spinor_api->write(sni, offset, data, len);
	sni->spi.flag = flag;
	sni->flag = nor_flag;
#else
	uint32_t key = irq_lock();
	ret = p_spinor_api->write(sni, offset, data, len);
	irq_unlock(key);
#endif
#endif

	spi_flash_lock_release();

	return ret ;
}

__ramfunc int spi_flash_acts_erase(const struct device *dev, off_t offset, size_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;

	spi_flash_lock_acquire();

#if defined(CONFIG_SPI_NOR_FLASH_4B_ADDRESS) || defined(CONFIG_NOR_CODE_IN_RAM)
	ret = spinor_4b_addr_op_api.erase(sni, offset, size);
#else
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
	uint32_t flag = sni->spi.flag;
	uint32_t nor_flag = sni->flag;
	sni->flag |= SPINOR_FLAG_UNLOCK_IRQ_WAIT_READY; //unlock wait ready
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  //lock
	ret = p_spinor_api->erase(sni, offset, size);
	sni->spi.flag = flag;
	sni->flag = nor_flag;
#else
	uint32_t key;
	#define NOR_ERASE_SECTOR (4*1024)
	ret = 0;
	while(size){// erase sector once
		key= irq_lock();
		ret |= p_spinor_api->erase(sni, offset, NOR_ERASE_SECTOR);
		irq_unlock(key);
		if(size < NOR_ERASE_SECTOR){
			size = 0;
		}else{
			size -= NOR_ERASE_SECTOR;
			offset += NOR_ERASE_SECTOR;
		}
	}
#endif
#endif

	spi_flash_lock_release();

	return ret ;
}

static inline void xspi_delay(void)
{
	volatile int i = 100000;

	while (i--)
		;
}

__ramfunc void xspi_nor_enable_status_qe(struct spinor_info *sni)
{
	uint16_t status;

	/* MACRONIX's spinor has different QE bit */
	if (XSPI_NOR_MANU_ID_MACRONIX == (sni->chipid & 0xff)) {
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x40)) {
			/* set QE bit to disable HOLD/WP pin function */
			status |= 0x40;
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 1);
		}

		return;
	}

	/* check QE bit */
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

	if (!(status & 0x2)) {
		/* set QE bit to disable HOLD/WP pin function, for WinBond */
		status |= 0x2;
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status, 1);

		/* check QE bit again */
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

		if (!(status & 0x2)) {
			/* oh, let's try old write status cmd, for GigaDevice/Berg */
			status = ((status | 0x2) << 8) |
				 p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 2);
		}
	}

	xspi_delay();

}

static inline void xspi_setup_bus_width(struct spinor_info *sni, u8_t bus_width)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	spi->ctrl = (spi->ctrl & ~(0x3 << 10)) | (((bus_width & 0x7) / 2 + 1) << 10);
	xspi_delay();
}

static __sleepfunc void xspi_setup_delaychain(struct spinor_info *sni, u8_t ns)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->ctrl = (spi->ctrl & ~(0xF << 16)) | (ns << 16);

	xspi_delay();
}

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
extern int nor_test_delaychain(const struct device *dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
extern void nor_dual_quad_read_mode_try(struct spinor_info *sni);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL

struct nor_delaychain_tbl {
	uint16_t vdd_volt;
	uint8_t delay;
};

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
static const struct nor_delaychain_tbl nor_delaychains[] = {
	{950, 9},
	{1000, 10},
	{1100, 10},
	{1150, 10},
	{1200, 11},
};
#else
static const struct nor_delaychain_tbl nor_delaychains[] = {
	{950, 11},
	{1000, 10},
	{1100, 11},
	{1150, 11},
	{1200, 13},
};
#endif

static inline void nor_set_delaychain_by_vdd(struct spinor_info *sni, uint16_t vdd)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(nor_delaychains); i++) {
		if (nor_delaychains[i].vdd_volt == vdd) {
			xspi_setup_delaychain(sni, nor_delaychains[i].delay);
			break;
		}
	}
}

__dvfs_notifier_func static void nor_dvfs_notify(void *user_data, struct dvfs_freqs *dvfs_freq)
{
	struct spinor_info *sni = (struct spinor_info *)user_data;
	struct dvfs_level *old_dvfs_level, *new_dvfs_level;
	uint32_t key;

	if (!dvfs_freq) {
		printk("dvfs notify invalid param");
		return ;
	}

	if (dvfs_freq->old_level == dvfs_freq->new_level)
		return ;

	key = irq_lock();

	old_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->old_level);
	new_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->new_level);

	if (old_dvfs_level->vdd_volt > new_dvfs_level->vdd_volt) {
		/* vdd voltage decrease */
		if (dvfs_freq->state == DVFS_EVENT_PRE_CHANGE) {
			if (new_dvfs_level->vdd_volt < 1100)
				clk_set_rate(CLOCK_ID_SPI0, MHZ(64));
			else
				clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));

			nor_set_delaychain_by_vdd(sni, new_dvfs_level->vdd_volt);

			printk("nor delaychain update by vdd:%d => %d\n",
					old_dvfs_level->vdd_volt, new_dvfs_level->vdd_volt);
		}
	} else {
		/* vdd voltage increase */
		if (dvfs_freq->state == DVFS_EVENT_POST_CHANGE) {
			nor_set_delaychain_by_vdd(sni, new_dvfs_level->vdd_volt);

			if (new_dvfs_level->vdd_volt < 1100)
				clk_set_rate(CLOCK_ID_SPI0, MHZ(64));
			else
				clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));

			printk("nor delaychain update by vdd:%d => %d\n",
					old_dvfs_level->vdd_volt, new_dvfs_level->vdd_volt);
		}
	}

	irq_unlock(key);
}

static struct spinor_info spi_flash_acts_data;
static struct dvfs_notifier __dvfs_notifier_data nor_dvsf_notifier = {
	.dvfs_notify_func_t = nor_dvfs_notify,
	.user_data = &spi_flash_acts_data,
};

#endif /* CONFIG_ACTS_DVFS_DYNAMIC_LEVEL */


__ramfunc int spi_flash_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	uint8_t status, status2, status3;

	sni->spi.prepare_hook = spi_flash_acts_prepare;

	key = irq_lock();
	sni->chipid = p_spinor_api->read_chipid(sni)& 0xffffff;

	printk("read spi nor chipid:0x%x\n", sni->chipid);

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#ifdef CONFIG_NOR_SUSPEND_RESUME
#ifndef CONFIG_SPINOR_TEST_DELAYCHAIN
	if(status2 & (NOR_STATUS2_SUS1|NOR_STATUS2_SUS2))
		spinor_resume_finished(sni);
#endif
#endif

#ifdef CONFIG_SPI_NOR_FLASH_DRV_EXT
void spinor_enable_xip_mode(struct spinor_info *sni);
	spinor_enable_xip_mode(sni);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
	nor_dual_quad_read_mode_try(sni);
	printk("bus width : %d, and cache read use ", sni->spi.bus_width);

#else
	if(sni->spi.bus_width == 4) {
		printk("nor is 4 line mode\n");
		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
		xspi_nor_enable_status_qe(sni);
		/* enable 4x mode */
		xspi_setup_bus_width(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		printk("nor is 2 line mode\n");
		/* enable 2x mode */
		xspi_setup_bus_width(sni, 2);
	} else {
		sni->spi.bus_width = 1;
		printk("nor is 1 line mode\n");
		/* enable 1x mode */
		xspi_setup_bus_width(sni, 1);
	}
#endif
	/* setup SPI clock rate */
	clk_set_rate(CLOCK_ID_SPI0, MHZ(CONFIG_SPI_FLASH_FREQ_MHZ));

	/* configure delay chain */
	xspi_setup_delaychain(sni, sni->spi.delay_chain);

	/* check delay chain workable */
	sni->chipid = p_spinor_api->read_chipid(sni) & 0xffffff;

	printk("read again spi nor chipid:0x%x\n", sni->chipid);

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
	spinor_enter_4byte_address_mode(sni);
#endif

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
	spi_gpio_cs_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_1_GPIO_CS_PIN));
	if (!spi_gpio_cs_dev) {
		printk("failed to get gpio:%d device", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
		irq_unlock(key);
		return -1;
	}
	gpio_pin_configure(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, GPIO_OUTPUT);
	gpio_pin_set(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, 1);
	printk("use GPIO:%d as spi cs pin", CONFIG_SPI_FLASH_1_GPIO_CS_PIN);
#endif

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	nor_test_delaychain(dev);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_register_notifier(&nor_dvsf_notifier);
#endif

	irq_unlock(key);

	flash_write_protection_set(dev, true);
#ifdef CONFIG_NOR_SECURIYT_SUSPPORT
	spinor_test_uid_securty(dev);
#endif

	return 0;
}

#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
static void spi_flash_acts_cs_gpio(struct spi_info *si, int value)
{
	if (spi_gpio_cs_dev) {
		gpio_pin_set(spi_gpio_cs_dev, CONFIG_SPI_FLASH_1_GPIO_CS_PIN % 32, value ? true : false);
		k_busy_wait(1);
	}
}
#endif

#ifndef CONFIG_BOARD_NANDBOOT

typedef int (*nor_send_command)(unsigned char cmd);
__sleepfunc void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{
	nor_send_command p_send_command = (nor_send_command)0x00003759;
	volatile int i;
	u32_t spi_ctl_ori = sys_read32(SPI0_REG_BASE);

	/* If spi mode is not the disable or write only mode, we need to disable firstly */
	if (((spi_ctl_ori & 0x3) != 0) && ((spi_ctl_ori & 0x3) != 2)) {
		sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
		for (i = 0; i < 5; i++) {
			;
		}
	}

	/* enable AHB interface for cpu write cmd */
	sys_write32(0xa013A, SPI0_REG_BASE);
	for(i = 0; i < 20; i++) {
		;
	}

	if (is_powerdown){
		/* 4x io need send 0xFF to exit the continuous mode */
		if (spi_ctl_ori & (0x3 << 10))
			p_send_command(0xFF);
		p_send_command(0xB9);
		soc_udelay(5); // max 3us
	} else {
		p_send_command(0xAB);
		soc_udelay(40); // max 30us
	}

	/* set spi in disable mode */
	sys_write32(sys_read32(SPI0_REG_BASE) & ~(3 << 0), SPI0_REG_BASE);
	for (i = 0; i < 5; i++) {
		;
	}

	sys_write32(spi_ctl_ori, SPI0_REG_BASE);

}


#endif

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
__sleepfunc void sys_norflash_exit_4b(void)
{
	//printk("spinor exit 4-byte address mode\n");
	p_spinor_api->write_status(NULL, 0xE9, NULL, 0);
}

__sleepfunc void sys_norflash_enter_4b(void)
{
	//printk("spinor enter 4-byte address mode\n");
	p_spinor_api->write_status(NULL, 0xB7, NULL, 0);
}
#endif

#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_acts_pages_layout(
				const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &(DEV_CFG(dev)->pages_layout);
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
extern int nor_write_protection(const struct device *dev, bool enable);
#endif

__ramfunc int spi_flash_acts_write_protection(const struct device *dev, bool enable)
{
#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)

	spi_flash_lock_acquire();

	nor_write_protection(dev, enable);

	spi_flash_lock_release();
#else
	struct spinor_info *sni = DEV_DATA(dev);
	uint8_t status1, status2, sta_en;
	uint8_t sta[2];
	uint32_t key;
	if (sni->chipid == 0x1560c8){//3085 nor
		key = irq_lock();
		status1 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
		sta_en = (status1 >> 2) & 0x1f;  //bp4-bp0  bit6-bit2
		if(enable){
			if(!sta_en){ // if disable
				#define PROTECT_16KB	0x1b
				#define PROTECT_64KB	0x9
				#define PROTECT_128KB	0xa 
				#define PROTECT_256KB	0xb
				#define PROTECT_512KB	0xc
				#define PROTECT_1MB		0xd 
				sta[0] = status1 | (PROTECT_16KB << 2);  //bit6-bit2 = bp4-bp0 =
				sta[1] = status2 & (~0x40);
				p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, sta, 2);
				printk("enable status1-2: {0x%02x 0x%02x}\n", sta[0], sta[1]);
			}
		}else{
			if(sta_en){ // if enable			 
				sta[0] = status1 & 0x83 ;	//bit6-bit2 = bp4-bp0 = 00000 = disable protect 
				sta[1] = status2 & (~0x40);
				p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, sta, 2);
				printk("disable status1-2: {0x%02x 0x%02x}\n", sta[0], sta[1]);
			}
		}
		irq_unlock(key);
	}

#endif
	return 0;
}

static const struct flash_parameters flash_acts_parameters = {
	.write_block_size = 0x1000,
	.erase_value = 0xff,
};

static const struct flash_parameters *
spi_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_acts_parameters;
}
#ifdef CONFIG_PM_DEVICE

int spi_flash_pm_control(const struct device *device, enum pm_device_action action)
{
	if(action == PM_DEVICE_ACTION_LATE_RESUME){		
		sys_write32((sys_read32(SPICACHE_CTL) & ~(0x3 << 5)) | (0x1 << 5) , SPICACHE_CTL);
		//printk("late reusme = 0x%x\n",  sys_read32(SPICACHE_CTL));
	}else if(action == PM_DEVICE_ACTION_EARLY_SUSPEND){
		sys_write32((sys_read32(SPICACHE_CTL) & ~(0x3 << 5)) | (0x2 << 5) , SPICACHE_CTL);
		//printk("nor early suspend = 0x%x\n",  sys_read32(SPICACHE_CTL));
    }
    return 0;
}
#else
#define spi_flash_pm_control 	NULL
#endif



static struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_read,
	.write = spi_flash_acts_write,
	.erase = spi_flash_acts_erase,
	.write_protection = spi_flash_acts_write_protection,
	.get_parameters = spi_flash_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

/* system XIP spinor */
static struct spinor_info spi_flash_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_DELAY_CHAIN,
#if (CONFIG_SPI_FLASH_1 == 0) && (CONFIG_SPI_FLASH_2 == 0)
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
#endif

#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
		.flag = SPI_FLAG_NO_IRQ_LOCK,
#else
		.flag = 0,
#endif
	},
	.flag = 0,

};

static const struct spi_flash_acts_config spi_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_CHIP_SIZE,
	.page_size	 = 0x1000,
};

#if IS_ENABLED(CONFIG_SPI_FLASH_0)
DEVICE_DEFINE(spi_flash_acts, CONFIG_SPI_FLASH_NAME, &spi_flash_acts_init, spi_flash_pm_control,
		    &spi_flash_acts_data, &spi_acts_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);
#endif

#if (CONFIG_SPI_FLASH_1 == 1)

/* system XIP spinor */
static struct spinor_info spi_flash_1_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_1_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_1_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
#if defined(CONFIG_SPI_FLASH_NO_IRQ_LOCK) && (CONFIG_SPI_FLASH_NO_IRQ_LOCK == 1)
		.flag = SPI_FLAG_NO_IRQ_LOCK,
#else
		.flag = 0,
#endif
#if defined(CONFIG_SPI_FLASH_1_GPIO_CS_EN) && (CONFIG_SPI_FLASH_1_GPIO_CS_EN == 1)
		.set_cs = spi_flash_acts_cs_gpio,
#endif
	},
	.flag = 0,
};

static const struct spi_flash_acts_config spi_flash_1_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_1_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_1_CHIP_SIZE,
	.page_size	 = 0x1000,
};

DEVICE_DEFINE(spi_flash_1_acts, CONFIG_SPI_FLASH_1_NAME, &spi_flash_acts_init, NULL,
		    &spi_flash_1_acts_data, &spi_flash_1_acts_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);
#endif




#if IS_ENABLED(CONFIG_SPI_FLASH_2)
const struct spinor_operation_api *g_spi3_nor_api;


static K_MUTEX_DEFINE(flash_2_mutex);

static int spi_flash_2_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    struct spinor_info *sni = DEV_DATA(dev);
    int ret = 0;
    size_t tmplen;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
    tmplen = len;
    while(tmplen > 0) {
        if(tmplen <  0x8000)
            len = tmplen;
        else
            len = 0x8000;

        ret = g_spi3_nor_api->read(sni, offset, data, len);
        offset += len;
        data  = (void *)((unsigned int )data + len);
        tmplen -= len;
    }

	k_mutex_unlock(&flash_2_mutex);

    return ret;
}

static int spi_flash_2_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
	ret = g_spi3_nor_api->write(sni, offset, data, len);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}

static int spi_flash_2_acts_erase(const struct device *dev, off_t offset, size_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
	ret = g_spi3_nor_api->erase(sni, offset, size);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}

static int spi_flash_2_pwoer(struct spinor_info *sni, bool on)
{
#if IS_ENABLED(CONFIG_SPI_FLASH_2_USE_GPIO_POWER)
    int ret;
	int gpio_value = CONFIG_SPI_FLASH_2_GPIO_POWER_LEVEL;
    const struct device *power_gpio_dev;
    uint8_t power_gpio = CONFIG_SPI_FLASH_2_POWER_GPIO % 32;
    power_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_2_POWER_GPIO));
    if (!power_gpio_dev) {
        LOG_ERR("Failed to bind nor  power GPIO(%d:%s)", power_gpio, CONFIG_GPIO_PIN2NAME(CONFIG_SPI_FLASH_2_POWER_GPIO));
        return -1;
    }
    ret = gpio_pin_configure(power_gpio_dev, power_gpio, GPIO_OUTPUT);
    if (ret) {
        LOG_ERR("Failed to config output GPIO:%d", power_gpio);
        return ret;
    }
    if (on) {
        /* power on nor */
        gpio_pin_set(power_gpio_dev, power_gpio, gpio_value);       
    } else {
        /* power off nor */
        gpio_pin_set(power_gpio_dev, power_gpio, !gpio_value);
    }
#else
    if (on) {
        spinor_write_cmd(sni, 0xAB); //exit deep power down       
    } else {
		spinor_write_cmd(sni, 0xB9); // enter deep power down
    }
#endif

    return 0;
}


#ifdef CONFIG_PM_DEVICE

int spi_flash_2_pm_control(const struct device *device, enum pm_device_action action)
{
	struct spinor_info *sni = DEV_DATA(device);	
	if(action == PM_DEVICE_ACTION_RESUME){
        LOG_INF("spi2 nor resume ...\n");
		spi_flash_2_pwoer(sni, true);
	}else if(action == PM_DEVICE_ACTION_SUSPEND){
        LOG_INF("spi2 nor suspend ...\n");
		spi_flash_2_pwoer(sni, false);
    }
    return 0;
}
#else
#define spi_flash_2_pm_control 	NULL
#endif

static int spi_flash_2_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint8_t status, status2, status3;
	printk("spi3 flash init\n");
	g_spi3_nor_api = spi3nor_get_api();

	/* enable spi3 controller clock */
	acts_clock_peripheral_enable(CLOCK_ID_SPI3);

	/* reset spi3 controller */
	acts_reset_peripheral(RESET_ID_SPI3);

	/* setup SPI3 clock rate */
	clk_set_rate(CLOCK_ID_SPI3, MHZ(CONFIG_SPI_FLASH_2_FREQ_MHZ));

	spi_flash_2_pwoer(sni, true);

	sni->chipid = g_spi3_nor_api->read_chipid(sni);

	printk("read spi3 nor chipid:0x%x\n", sni->chipid);

	status = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spi3 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

	if(sni->spi.bus_width == 4) {
		printk("data nor is 4 line mode\n");
		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
		/* check QE bit */
		if (!(status2 & 0x2)) {
			/* set QE bit to disable HOLD/WP pin function, for WinBond */
			status2 |= 0x2;
			g_spi3_nor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status2, 1);
		}
	} else if(sni->spi.bus_width == 2) {
		printk("data nor is 2 line mode\n");
	} else {
		sni->spi.bus_width = 1;
		printk("data nor is 1 line mode\n");
	}

	/* check delay chain workable */
	sni->chipid = g_spi3_nor_api->read_chipid(sni);

	printk("read again spi3 nor chipid:0x%x\n", sni->chipid);

	#if (CONFIG_SPI_FLASH_2_CHIP_SIZE >= 0x20000000)
	spinor_enter_4byte_address_mode(sni);
	#endif

	status = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = g_spi3_nor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spi3 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	nor_test_delaychain(dev);
#endif

	return 0;
}

static struct flash_driver_api spi_flash_2_nor_api = {
	.read = spi_flash_2_acts_read,
	.write = spi_flash_2_acts_write,
	.erase = spi_flash_2_acts_erase,
	.get_parameters = spi_flash_get_parameters,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

static struct spinor_info spi_flash_2_acts_data = {
	.spi = {
		.base = SPI3_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_2_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_2_DELAY_CHAIN,
#if (CONFIG_DMA_SPINOR_RESEVER_CHAN < CONFIG_DMA_0_PCHAN_NUM)
		.dma_base= (DMA_REG_BASE + 0x100 + (CONFIG_DMA_SPINOR_RESEVER_CHAN * 0x100)),
#endif
		.flag = SPI_FLAG_NO_IRQ_LOCK,

	},
	.flag = 0,
};

static const struct spi_flash_acts_config spi_flash_2_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_2_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_2_CHIP_SIZE,
	.page_size	 = 0x1000,
};

DEVICE_DEFINE(spi_flash_2_acts, CONFIG_SPI_FLASH_2_NAME, &spi_flash_2_acts_init, spi_flash_2_pm_control,
		    &spi_flash_2_acts_data, &spi_flash_2_acts_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spi_flash_2_nor_api);
#endif


#ifdef CONFIG_NOR_SECURIYT_SUSPPORT

#define NOR_SE_PAGE_SIZE  				256
#define NOR_SE_PAGE_MASK 				(NOR_SE_PAGE_SIZE-1)
#define NOR_SE_MAX_SIZE_EACH_REGN	  	1024   /**/

/*security_regn 0-3*/
int spi_flash_security_erase(const struct device *dev, unsigned int security_regn)
{
    struct spinor_info *sni = DEV_DATA(dev);
	return spinor_erase_security(sni, security_regn<<12);
}

int spi_flash_security_write(const struct device *dev, unsigned int security_regn, unsigned int offset, void *data, unsigned int len)
{
	unsigned int wlen, unlen;
    struct spinor_info *sni = DEV_DATA(dev);
	if(offset + len >= NOR_SE_MAX_SIZE_EACH_REGN)
		return -1;
	unlen = offset & NOR_SE_PAGE_MASK;
	while(len){
		if(unlen){
			wlen = NOR_SE_PAGE_SIZE - unlen;
			if(wlen > len)
				wlen = len;
			unlen = 0;
		}else{
			if(len < NOR_SE_PAGE_SIZE)
				wlen = len;
			else
				wlen = NOR_SE_PAGE_SIZE;
		}
		spinor_write_security(sni, (security_regn<<12)|offset, data, wlen);
		data = (unsigned char *)data + wlen;
        len -= wlen;
        offset += wlen;
	}
	return 0;
}

int spi_flash_security_read(const struct device *dev, unsigned int security_regn, unsigned int offset, void *data, unsigned int len)
{
    struct spinor_info *sni = DEV_DATA(dev);
	if(offset + len >= NOR_SE_MAX_SIZE_EACH_REGN)
		return -1;

	return spinor_read_security(sni, (security_regn<<12)|offset, data, len);
}

int spi_flash_uid_read(const struct device *dev, void *uid, unsigned int len)
{
	 struct spinor_info *sni = DEV_DATA(dev);
	 return spinor_read_uid(sni, uid, len);
}


#include <string.h>
static unsigned int g_tmp_buf[256];
void spinor_test_uid_securty(const struct device *dev)
{
	unsigned int  start_ms,end_ms, i, k;
	unsigned int *pb = g_tmp_buf;
	spi_flash_uid_read(dev, pb, 16);
	printk("uid=0x%x, 0x%x, 0x%x, 0x%x\n", pb[0], pb[1], pb[2], pb[3]);
	for(i = 1; i < 4; i++){// test security1-security3
		start_ms = k_cycle_get_32();
		spi_flash_security_erase(dev, i);
		end_ms = k_cyc_to_ms_near32(k_cycle_get_32() -start_ms) ;
		printk("scurity erase %d use=%d ms\n", i, end_ms);
		spi_flash_security_read(dev, i, 200,  pb, NOR_SE_PAGE_SIZE);
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++) {// check erase ok
			if(pb[k] != 0xffffffff){
				printk("erase check fail %d : off=0x%x, 0x%x!=0xffffffff\n", i, k*4, pb[k]);
				break;
			}
		}
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++) {
			pb[k] = k + 0x12345600*i;
		}
		start_ms = k_cycle_get_32();
		spi_flash_security_write(dev, i, 200, pb, NOR_SE_PAGE_SIZE);
		end_ms = k_cyc_to_ms_near32(k_cycle_get_32() -start_ms) ;
		printk("scurity write 1KB %d use=%d ms\n", i, end_ms);
	}
	for(i = 1; i < 4; i++){
		memset(pb, 0, NOR_SE_PAGE_SIZE);
		spi_flash_security_read(dev, i, 200,  pb, NOR_SE_PAGE_SIZE);
		for(k = 0; k < NOR_SE_PAGE_SIZE/4; k++){
			if(pb[k] != k + 0x12345600*i){
				printk("scurity read cmp fail:%d,off=0x%x,0x%x!=0x%x\n",i, k*4, pb[k],  k + 0x12345600*i);
				break;
			}
		}
	}
	printk("secutrity test finished\n");
}

#endif


