/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#define DT_DRV_COMPAT actions_acts_flash

#include <errno.h>
#include <disk/disk_access.h>
#include "spinand_acts.h"

#include <drivers/spinand.h>
#include <board.h>
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#include <dvfs.h>
#endif

#ifdef CONFIG_BOARD_NANDBOOT
#define TRY_DELAYCHAIN          40
#else
#define TRY_DELAYCHAIN          20
#endif

#define ID_TBL_MAGIC			0x53648673
#define ID_TBL_ADDR				soc_boot_get_nandid_tbl_addr()

#ifndef CONFIG_SPINAND_LIB
//spinand code rom api address
#ifdef CONFIG_SOC_LEOPARD
#define SPINAND_API_ADDR	    0x00007000
#else
#define SPINAND_API_ADDR	    0x00006800
#endif
#define p_spinand_api	((struct spinand_operation_api *)SPINAND_API_ADDR)
#endif

#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_DISK_OK		0x08	/* Medium OK in the drive */

#define LIB_LOG_LEVEL WARN_LEVEL
#define HEAP_SIZE (14*1024)
uint32_t api_bss[HEAP_SIZE/4];

struct k_mutex mutex;

#define SECTOR_SIZE 512

#ifndef CONFIG_SPINAND_DATA
#define CONFIG_SPINAND_DATA 1
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(spinand_acts, CONFIG_FLASH_LOG_LEVEL);

static bool spinand_initial;
#ifdef CONFIG_PM_DEVICE
static bool spinand_resume;
int spinand_resume_init(struct spinand_info *sni);
#endif
void set_spinand_spimfp();

static uint32_t spinand_max_sectors;

uint32_t spinand_phy_special_erase(void *sni, void *OpPar)
{
    struct PhysicOpParameter *PhyParam = (struct PhysicOpParameter *)OpPar;

    soc_watchdog_clear();

#ifndef CONFIG_SPINAND_LIB
    return p_spinand_api->extra_api->phy_erase((struct spinand_info *)sni, PhyParam);
#else
    return spinand_phy_erase((struct spinand_info *)sni, PhyParam);
#endif
}

static struct nand_info system_spinand = {
    .base = SPI0_REG_BASE+(0x4000*CONFIG_SPINAND_USE_SPICONTROLER),
    .bus_width = CONFIG_SPINAND_FLASH_BUS_WIDTH,
    .delay_chain = 0,
    .spi_mode = 3,
    .data = CONFIG_SPINAND_DATA,
    .dma_base = DMA_REG_BASE+0x100+(CONFIG_DMA_SPINAND_RESEVER_CHAN*0x100), //DMA9
    .printf = (void *)printk,
    .loglevel = LIB_LOG_LEVEL,
    .phy_special_erase = spinand_phy_special_erase,
};

static struct spinand_info spinand_acts_data = {
    .protect = 1,
};

static int spinand_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
    if (((offset % 512) != 0) || ((len % 512 != 0))) {
        LOG_ERR("offset=0x%x; len=0x%x not sector align.\n", (u32_t)offset, (u32_t)len);
        return -1;
    }
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    offset >>= 9;
    len >>= 9;
#ifdef CONFIG_PM_DEVICE
    if (spinand_resume) {
        ret = spinand_resume_init(sni);
        if (ret != 0) {
            LOG_ERR("spinand resume err, spinand read err!\n");
            return ret;
        }
    }
#endif

    if (!spinand_initial) {
        LOG_ERR("%s spinand init failed, please check...\n", __func__);
        return -1;
    }

    //printk("read offset = %d; len = %d;\n", offset, len);
    if ((offset + len) > spinand_max_sectors) {
        LOG_ERR("%s error! read from 0x%x len 0x%x excceed storage capacity 0x%x.\n", __func__, offset, len, spinand_max_sectors);
        return -1;
    }
    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret =  p_spinand_api->read(sni, offset, data, len);
#else
    ret = spinand_read(sni, offset, data, len);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

static int spinand_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
    if (((offset % 512) != 0) || ((len % 512 != 0))) {
        LOG_ERR("offset=0x%x; len=0x%x not sector align.\n", (u32_t)offset, (u32_t)len);
        return -1;
    }
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    offset >>= 9;
    len >>= 9;
#ifdef CONFIG_PM_DEVICE
    if (spinand_resume) {
        ret = spinand_resume_init(sni);
        if (ret != 0) {
            LOG_ERR("spinand resume err, spinand write err!\n");
            return ret;
        }
    }
#endif

    if (!spinand_initial) {
        LOG_ERR("%s spinand init failed, please check...\n", __func__);
        return -1;
    }

    //printk("write offset = %d; len = %d;\n", offset, len);
    if ((offset + len) > spinand_max_sectors) {
        LOG_ERR("%s error! write from 0x%x len 0x%x excceed storage capacity 0x%x.\n", __func__, offset, len, spinand_max_sectors);
        return -1;
    }
    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->write(sni, offset, data, len);
#else
    ret = spinand_write(sni, offset, data, len);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

static int spinand_acts_erase(const struct device *dev, off_t offset, size_t size)
{
    if (((offset % 512) != 0) || ((size % 512 != 0))) {
        LOG_ERR("offset=0x%x; len=0x%x not sector align.\n", (u32_t)offset, (u32_t)size);
        return -1;
    }
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

    offset >>= 9;
    size >>= 9;
#ifdef CONFIG_PM_DEVICE
    if (spinand_resume) {
        ret = spinand_resume_init(sni);
        if (ret != 0) {
            LOG_ERR("spinand resume err, spinand erase err!\n");
            return ret;
        }
    }
#endif

    if (!spinand_initial) {
        LOG_ERR("%s spinand init failed, please check...\n", __func__);
        return -1;
    }

    printk("erase offset = %d; len = %d;\n", offset, size);

    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->erase(sni, offset, size);
#else
    ret = spinand_erase(sni, offset, size);
#endif
    k_mutex_unlock(&mutex);

    return ret;
}

static int spinand_acts_flush(const struct device *dev, bool efficient)
{
    //offset, len shuold align with 512B.
    struct spinand_info *sni = &spinand_acts_data;
    int ret = 0;

#ifdef CONFIG_PM_DEVICE
    if (spinand_resume) {
        ret = spinand_resume_init(sni);
        if (ret != 0) {
            LOG_ERR("spinand resume err, spinand flush err!\n");
            return ret;
        }
    }
#endif
    if (!spinand_initial) {
        LOG_ERR("%s spinand init failed, please check...\n", __func__);
        return -1;
    }
    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->flush(sni);
#else
    ret = spinand_flush(sni, efficient);
#endif

    k_mutex_unlock(&mutex);

    return ret;
}

int get_storage_params(struct spinand_info *sni, u8_t *id, struct FlashChipInfo **ChipInfo)
{
    int i, j;

    struct NandIdTblHeader *id_tbl_header = (struct NandIdTblHeader *)sni->id_tbl;
    struct FlashChipInfo *id_tbl = (struct FlashChipInfo *)((uint8_t *)sni->id_tbl + sizeof(struct NandIdTblHeader));

    if (id_tbl_header->magic != ID_TBL_MAGIC)
    {
        LOG_ERR("id tbl magic err! 0x%x \n", id_tbl_header->magic);
        return -1;
    }

    for (i = 0; i < id_tbl_header->num; i++)
    {
        for (j = 0; j < NAND_CHIPID_LENGTH / 2; j++)
        {
            /* skip compare the 0xff value */
            if ((id_tbl[i].ChipID[j] != 0xff) && (id_tbl[i].ChipID[j] != id[j]))
            {
                //LOG_DBG("id not match; id_tbl[%d].ChipID[%d] = 0x%x; id[%d] = 0x%x\n", i, j, id_tbl[i].ChipID[j], j, id[j]);
                break;
            }
            //LOG_DBG("id match; id_tbl[%d].ChipID[%d] = 0x%x; id[%d] = 0x%x\n", i, j, id_tbl[i].ChipID[j], j, id[j]);
        }

        if (j == NAND_CHIPID_LENGTH / 2)
        {
            *ChipInfo = &id_tbl[i];
            //LOG_DBG("get chipinfo.\n");
            return 0;
        }
    }

    LOG_ERR("spinand id not match, please check!\n");
    return -1;
}

int spinand_get_chipid(struct spinand_info *sni, u32_t *chipid)
{
    int i = 0;
    struct FlashChipInfo *NandFlashInfo;

retry:
#ifndef CONFIG_SPINAND_LIB
    *chipid = p_spinand_api->read_chipid(sni);
#else
    *chipid = spinand_read_chipid(sni);
#endif
    //LOG_INF("nand id = 0x%x\n", *chipid);
    if (*chipid == 0x0 || *chipid == 0xffffffff) {
        if (i++ < 3) {
            LOG_ERR("Can't get spinand id, retry %d...\n", i);
            goto retry;
        } else {
            LOG_ERR("Can't get spinand id, Please check!\n");
            return -1;
        }
    }

    if (get_storage_params(sni, (u8_t *)chipid, &NandFlashInfo) != 0) {
        LOG_ERR("Get chipid = 0x%x; But Can't get this chipid in idtbl.\n", *chipid);
        return -1;
    }

    return 0;
}

int spinand_get_delaychain(struct spinand_info *sni)
{
    uint32_t chipid = 0;
    struct FlashChipInfo *NandFlashInfo;
    uint8_t t_dc = 0;
    int ret = 0;

    t_dc = sni->spi->delay_chain;
    sni->spi->delay_chain = TRY_DELAYCHAIN;
    ret = spinand_get_chipid(sni, &chipid);
    sni->spi->delay_chain = t_dc;
    if (ret != 0) {
        LOG_ERR("spinand get chipid err!\n");
        return -EINVAL;
    }
    if (get_storage_params(sni, (u8_t *)&chipid, &NandFlashInfo) != 0) {
        LOG_ERR("Can't get flashinfo.\n");
        return -1;
    }

    LOG_INF("spinand chipid: 0x%x; chipname: %s \n", chipid, NandFlashInfo->FlashMark);

    return NandFlashInfo->delayChain;
}

uint32_t spinand_get_storage_capacity(struct spinand_info *sni)
{
    uint32_t chipid = 0;
    uint32_t max_sectors = 0;
    uint32_t zone_num = 0;
    struct FlashChipInfo *NandFlashInfo;
    
    if (spinand_get_chipid(sni, &chipid) != 0) {
        LOG_ERR("spinand get chipid err!\n");
        return 0;
    }
    if (get_storage_params(sni, (u8_t *)&chipid, &NandFlashInfo) != 0) {
        LOG_ERR("Can't get flashinfo.\n");
        return 0;
    }

    zone_num = NandFlashInfo->TotalBlkNumPerDie / NandFlashInfo->DefaultLBlkNumPer1024Blk;
    max_sectors = zone_num * NandFlashInfo->DefaultLBlkNumPer1024Blk * NandFlashInfo->SectorNumPerPage * NandFlashInfo->PageNumPerPhyBlk;

    return max_sectors;
}

int spinand_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff)
{
    struct spinand_info *sni = &spinand_acts_data;
    uint32_t chipid = 0;

#ifdef CONFIG_PM_DEVICE
    int ret;
    if (spinand_resume) {
        ret = spinand_resume_init(sni);
        if (ret != 0) {
            LOG_ERR("spinand resume err, spinand ioctl err!\n");
            return ret;
        }
    }
#endif

    if (!spinand_initial) {
        LOG_ERR("%s spinand init failed, please check...\n", __func__);
        return -1;
    }

    switch (cmd) {
    case DISK_IOCTL_CTRL_SYNC:
        spinand_acts_flush(dev, 1);
        break;
    case DISK_IOCTL_GET_SECTOR_COUNT:
        if (spinand_max_sectors == 0) {
            LOG_ERR("Can't get spinand storage capacity.\n");
            k_mutex_unlock(&mutex);
            return -1;
        }
        *(uint32_t *)buff = spinand_max_sectors;
        break;
    case DISK_IOCTL_GET_SECTOR_SIZE:
        *(uint32_t *)buff = SECTOR_SIZE;
        break;
    case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
        *(uint32_t *)buff  = SECTOR_SIZE;
        break;
    case DISK_IOCTL_HW_DETECT:
        if (spinand_get_chipid(sni, &chipid) != 0) {
            *(uint8_t *)buff = STA_NODISK;
        } else {
            *(uint8_t *)buff = STA_DISK_OK;
        }
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int spinand_env_init(struct spinand_info *sni)
{
    uint8_t feture;
    uint32_t chipid = 0;
    int delay_chain = 0;
    uint8_t max_delaychain;

    /* enable spi controller clock */
    acts_clock_peripheral_enable(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER);

    /* reset spi controller */
    acts_reset_peripheral(RESET_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER);

    set_spinand_spimfp();
    //LOG_INF("GOIO%d = 0x%x\n", CONFIG_SPINAND_POWER_GPIO, sys_read32(SPINAND_POWER_GPIO));

    /* setup SPI clock rate to 32MHz, use a lower clk to try chipid and capacity.*/
    clk_set_rate(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER, MHZ(32));

    if (sni->spi->delay_chain == 0) {
        delay_chain = spinand_get_delaychain(sni);
        if (CONFIG_SPINAND_USE_SPICONTROLER == 0)
            max_delaychain = 63;
        else
            max_delaychain = 31;
        if (delay_chain < 0 || delay_chain > max_delaychain) {
            LOG_ERR("spinand get delaychain failed!\n");
            return -1;
        }
        sni->spi->delay_chain = delay_chain;
        LOG_INF("spinand set delaychain %d.\n", sni->spi->delay_chain);
    } else {
        LOG_INF("spinand set delaychain %d.\n", sni->spi->delay_chain);
    }
    
    if (spinand_get_chipid(sni, &chipid) != 0) {
        return -1;
    }

    spinand_max_sectors = spinand_get_storage_capacity(sni);
    if (spinand_max_sectors == 0) {
        LOG_ERR("spinand get storage capacity failed!\n");
        return -1;
    }
    LOG_INF("spinand contain sectors 0x%x.\n", spinand_max_sectors);

    /* setup SPI clock rate to CONFIG_SPINAND_FLASH_FREQ_MHZ*/
    clk_set_rate(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER, MHZ(CONFIG_SPINAND_FLASH_FREQ_MHZ));

    //if use 4xIO, SIO2 & SIO3 do not need pull up, otherwise, SIO2 & SIO3 need setting pull up.
    if (sni->spi->bus_width == 4) {
#ifndef CONFIG_SPINAND_LIB
        feture = p_spinand_api->spinand_get_feature(sni, 0xB0);
        p_spinand_api->spinand_set_feature(sni, 0xB0, feture | 0x1);
#else
        feture = spinand_get_feature(sni, 0xB0);
        spinand_set_feature(sni, 0xB0, feture | 0x1);
#endif
    } else {
        LOG_INF("spinand use 1xIO.\n");
    }

    //Compatible for HYF1GQ4UT/HYF2GQ4UT/HYF4GQ4UT.
    if (chipid == 0x15011501 || chipid == 0x25012501 || chipid == 0x35013501) {
#ifdef SPINAND_ROM
        p_spinand_api->spinand_set_feature(sni, 0xA0, 0x2);
#else
        spinand_set_feature(sni, 0xA0, 0x2);
#endif
    }

    //For debug only.
    if (0) {
        LOG_DBG("MRCR0 = 0x%x \n", sys_read32(RMU_MRCR0));
        //bit7 for spi3
        LOG_DBG("CMU_DEVCKEN0 = 0x%x \n", sys_read32(CMU_DEVCLKEN0));
        LOG_DBG("CORE_PLL = 0x%x \n", sys_read32(COREPLL_CTL));
        LOG_DBG("SPI3CLK  = 0x%x \n", sys_read32(CMU_SPI3CLK));
        LOG_DBG("GOIO64   = 0x%x \n", sys_read32(0x40068100));
        struct board_pinmux_info pinmux_info;
        board_get_spinand_pinmux_info(&pinmux_info);
	    int i;
        LOG_DBG("spinand pinctrl list:\n");
	    for (i = 0; i < pinmux_info.pins_num; i++) {
            LOG_DBG("GPIO%d = 0x%x\n", pinmux_info.pins_config[i].pin_num, pinmux_info.pins_config[i].mode);
        }
    }

    return 0;
}

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
__dvfs_notifier_func static void spinand_dvfs_notify(void *user_data, struct dvfs_freqs *dvfs_freq)
{
	struct dvfs_level *old_dvfs_level, *new_dvfs_level;

	if (!dvfs_freq) {
		LOG_ERR("dvfs notify invalid param");
		return ;
	}

	if (dvfs_freq->old_level == dvfs_freq->new_level)
		return;

    k_mutex_lock(&mutex, K_FOREVER);

	old_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->old_level);
	new_dvfs_level = dvfs_get_info_by_level_id(dvfs_freq->new_level);

    if (dvfs_freq->state == DVFS_EVENT_PRE_CHANGE) {
    	if (new_dvfs_level->vdd_volt >= 1100)
            clk_set_rate(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER, MHZ(CONFIG_SPINAND_FLASH_FREQ_MHZ));
    	else
    		clk_set_rate(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER, MHZ(64));
    
    	LOG_INF("spi%dclk update by vdd:%d => %d\n",
    			CONFIG_SPINAND_USE_SPICONTROLER, old_dvfs_level->vdd_volt, new_dvfs_level->vdd_volt);
    }

    k_mutex_unlock(&mutex);
}

static struct dvfs_notifier __dvfs_notifier_data spinand_dvfs_notifier = {
	.dvfs_notify_func_t = spinand_dvfs_notify,
	.user_data = NULL,
};
#endif

void set_spinand_gpiohighz()
{
    struct board_pinmux_info pinmux_info;
    board_get_spinand_gpiohighz_info(&pinmux_info);
    acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);
}

void set_spinand_spimfp()
{
    struct board_pinmux_info pinmux_info;
    board_get_spinand_pinmux_info(&pinmux_info);
    acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);
    // wait for power Stable
    k_busy_wait(4*1000);
}

int spinand_acts_init(const struct device *dev)
{
    int ret = 0;
    //const struct spinand_acts_config *config = DEV_CFG(dev);
    struct spinand_info *sni = &spinand_acts_data;

    sni->bss = api_bss;
    memset(sni->bss, 0x0, HEAP_SIZE);
    sni->id_tbl = (void *)soc_boot_get_nandid_tbl_addr();
    sni->spi = &system_spinand;
    spinand_initial = false;
#ifdef CONFIG_PM_DEVICE
    spinand_resume = false;
#endif

    set_spinand_gpiohighz();
    if (spinand_env_init(sni) != 0) {
        LOG_ERR("spinand env init err.\n");
        return -1;
    }

#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->init(sni);
#else
    LOG_INF("spinand version %s \n", spinand_get_version());
    ret = spinand_init(sni);
#endif
    if (ret != 0) {
        LOG_ERR("SPINand rom driver init err.\n");
        return -1;
    }

    k_mutex_init(&mutex);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_register_notifier(&spinand_dvfs_notifier);
#endif

    spinand_initial = true;
    return 0;
}

static int spinand_drv_init(const struct device *dev)
{
#ifdef CONFIG_BOARD_NANDBOOT
    return spinand_acts_init(dev);
#endif

    return 0;
}

#ifdef CONFIG_PM_DEVICE
static u32_t bss_checksum;
#define CHECKSUM_XOR_VALUE      0xaa55
u32_t spinand_checksum(u32_t *buf, u32_t len)
{
    u32_t i;
    u32_t sum = 0;

    for (i = 0; i < len; i++)
    {
        sum += buf[i];
    }

    sum ^= (uint16_t)CHECKSUM_XOR_VALUE;

    return sum;
}

int spinand_resume_init(struct spinand_info *sni)
{
    int ret = 0;

    k_mutex_lock(&mutex, K_FOREVER);
    if (spinand_env_init(sni) != 0) {
        LOG_ERR("spinand env init err.\n");
        k_mutex_unlock(&mutex);
        return -1;
    }

#ifndef CONFIG_SPINAND_LIB
    ret = p_spinand_api->init(sni);
#else
    ret = spinand_pdl_init(sni);
#endif
    if (ret != 0) {
        LOG_ERR("SPINand rom driver init err.\n");
        k_mutex_unlock(&mutex);
        return -1;
    }
    
    if (spinand_resume)
        spinand_resume = false;
    spinand_initial = true;
    k_mutex_unlock(&mutex);

    return ret;
}

int spinand_pm_control(const struct device *device, enum pm_device_action action)
{
    int ret;
    u32_t tmp_cksum;
    //struct spinand_info *sni = &spinand_acts_data;

    switch (action) {
    case PM_DEVICE_ACTION_RESUME:
        LOG_INF("spinand resume ...\n");
        tmp_cksum = spinand_checksum(api_bss, sizeof(api_bss)/4);
        if (bss_checksum != tmp_cksum) {
            LOG_ERR("SPINand resume err! api_bss is changed! suspend_checksum = 0x%x; resume_checksum = 0x%x \n", bss_checksum, tmp_cksum);
            return -1;
        }
        //LOG_INF("resume_checksum = 0x%x; sizeof(api_bss) = 0x%x \n", tmp_cksum, sizeof(api_bss));
        k_mutex_lock(&mutex, K_FOREVER);
        spinand_resume = true;
        k_mutex_unlock(&mutex);
        break;
    case PM_DEVICE_ACTION_SUSPEND:
        LOG_INF("spinand suspend ...\n");
        k_mutex_lock(&mutex, K_FOREVER);
		set_spinand_gpiohighz();
        spinand_initial = false;
        k_mutex_unlock(&mutex);
        bss_checksum = spinand_checksum(api_bss, sizeof(api_bss)/4);
        //LOG_INF("suspend_checksum = 0x%x; sizeof(api_bss) = 0x%x \n", bss_checksum, sizeof(api_bss));
        break;
    case PM_DEVICE_ACTION_EARLY_SUSPEND:
        break;
    case PM_DEVICE_ACTION_LATE_RESUME:
        break;
    case PM_DEVICE_ACTION_TURN_OFF:
        //flush all data to spinand.
        spinand_acts_flush(device, 0);
        break;
    default:
        ret = -EINVAL;
    }
    return 0;
}
#else
#define adc_pm_control 	NULL
#endif

static struct flash_driver_api spinand_api = {
    .read = spinand_acts_read,
    .write = spinand_acts_write,
    .erase = spinand_acts_erase,
    .write_protection = NULL,
#ifdef CONFIG_SPINAND_LIB
    .flush = spinand_acts_flush,
#endif
};

#ifdef CONFIG_BOARD_NANDBOOT
#define SPINAND_ACTS_DEVICE_INIT(n)						\
    DEVICE_DEFINE(spinand, CONFIG_SPINAND_FLASH_NAME, \
            &spinand_drv_init, spinand_pm_control, \
            &spinand_acts_data, NULL, PRE_KERNEL_2, \
            CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spinand_api);
#else 
#define SPINAND_ACTS_DEVICE_INIT(n)						\
    DEVICE_DEFINE(spinand, CONFIG_SPINAND_FLASH_NAME, \
            &spinand_drv_init, spinand_pm_control, \
            &spinand_acts_data, NULL, POST_KERNEL, \
            CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &spinand_api);
#endif

#if IS_ENABLED(CONFIG_SPINAND_3) || IS_ENABLED(CONFIG_SPINAND_0)
    SPINAND_ACTS_DEVICE_INIT(3)
#endif

