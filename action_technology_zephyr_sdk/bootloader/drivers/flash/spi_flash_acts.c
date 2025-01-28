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
	while(!(sys_read32(SPI_STA(si->base)) & SPI_STA_READY));

	spi_flash_reset(si);
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

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
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

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
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

#ifdef CONFIG_SPI_NOR_FLASH_4B_ADDRESS
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
	uint32_t key = irq_lock();
	ret = p_spinor_api->erase(sni, offset, size);
	irq_unlock(key);
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

static inline void xspi_setup_delaychain(struct spinor_info *sni, u8_t delay)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->delaychain = (spi->delaychain & ~(0x3F << 0)) | (delay << 0);

	xspi_delay();
}

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
extern int nor_test_delaychain(struct device *dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
extern void nor_dual_quad_read_mode_try(struct spinor_info *sni);
#endif


__ramfunc int spi_flash_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	uint8_t status, status2, status3;

	sni->spi.prepare_hook = spi_flash_acts_prepare;

	key = irq_lock();
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read spi nor chipid:0x%x\n", sni->chipid);

	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spinor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

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
	sni->chipid = p_spinor_api->read_chipid(sni);

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

	irq_unlock(key);

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

#define SPI0_BASE                   0x40028000UL
static struct spinor_info system_spi_nor = {
	.spi = {
		.base = SPI0_BASE, 
		.bus_width = 1,
		.delay_chain = 0x20,
		.flag = 0,
#if 0
		.dma_base= 0x4001C600, //DMA5
#endif
	},
};

#ifndef CONFIG_BOARD_NANDBOOT
__sleepfunc void  sys_norflash_power_ctrl(uint32_t is_powerdown)

{
	if (is_powerdown){
		p_spinor_api->write_status(&system_spi_nor, 0xB9, NULL, 0);
	} else {
		p_spinor_api->write_status(&system_spi_nor, 0xAB, NULL, 0);
	}
}
#endif

 void  sys_norflash_exit_4b(void)
{
	printk("spinor exit 4-byte address mode\n");
	p_spinor_api->write_status(&system_spi_nor, 0xE9, NULL, 0);
}



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

static struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_read,
	.write = spi_flash_acts_write,
	.erase = spi_flash_acts_erase,
	.write_protection = spi_flash_acts_write_protection,
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
DEVICE_DEFINE(spi_flash_acts, CONFIG_SPI_FLASH_NAME, &spi_flash_acts_init, NULL,
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

        ret = spinor_4b_addr_op_api.read(sni, offset, data, len);
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
	ret = spinor_4b_addr_op_api.write(sni, offset, data, len);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}

static int spi_flash_2_acts_erase(const struct device *dev, off_t offset, size_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	int ret;
	k_mutex_lock(&flash_2_mutex, K_FOREVER);
	ret = spinor_4b_addr_op_api.erase(sni, offset, size);
	k_mutex_unlock(&flash_2_mutex);
	return ret ;
}


static int spi_flash_2_pwoer(bool on)
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
#endif

    return 0;
}
static int spi_flash_2_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint8_t status, status2, status3;
	printk("spi3 flash init\n");

	spi_flash_2_pwoer(true);
	/* enable spi3 controller clock */
	acts_clock_peripheral_enable(CLOCK_ID_SPI3);

	/* reset spi3 controller */
	acts_reset_peripheral(RESET_ID_SPI3);

	/* setup SPI3 clock rate */
	clk_set_rate(CLOCK_ID_SPI3, MHZ(CONFIG_SPI_FLASH_2_FREQ_MHZ));

	sni->chipid = spinor_4b_addr_op_api.read_chipid(sni);

	printk("read spi3 nor chipid:0x%x\n", sni->chipid);

	status = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

	printk("spi3 nor status: {0x%02x 0x%02x 0x%02x}\n", status, status2, status3);

	if(sni->spi.bus_width == 4) {
		printk("data nor is 4 line mode\n");
		sni->spi.flag |= SPI_FLAG_SPI_4XIO;
		/* check QE bit */
		if (!(status2 & 0x2)) {
			/* set QE bit to disable HOLD/WP pin function, for WinBond */
			status2 |= 0x2;
			spinor_4b_addr_op_api.write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status2, 1);
		}
	} else if(sni->spi.bus_width == 2) {
		printk("data nor is 2 line mode\n");
	} else {
		sni->spi.bus_width = 1;
		printk("data nor is 1 line mode\n");
	}

	/* check delay chain workable */
	sni->chipid = spinor_4b_addr_op_api.read_chipid(sni);

	printk("read again spi3 nor chipid:0x%x\n", sni->chipid);

	#if (CONFIG_SPI_FLASH_2_CHIP_SIZE >= 0x20000000)
	spinor_enter_4byte_address_mode(sni);
	#endif

	status = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	status2 = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	status3 = spinor_4b_addr_op_api.read_status(sni, XSPI_NOR_CMD_READ_STATUS3);

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

DEVICE_DEFINE(spi_flash_2_acts, CONFIG_SPI_FLASH_2_NAME, &spi_flash_2_acts_init, NULL,
		    &spi_flash_2_acts_data, &spi_flash_2_acts_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spi_flash_2_nor_api);
#endif




