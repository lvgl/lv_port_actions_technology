
#include <errno.h>
#include <disk/disk_access.h>
#include "spinand_acts.h"

#include <drivers/spinand.h>
#include <board.h>
#include <dvfs.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(spinand_test_frameworks, CONFIG_FLASH_LOG_LEVEL);

#if (CONFIG_SPINAND_USE_SPICONTROLER == 0)
#define DC_MAX 64
#else
#define DC_MAX 32
#endif

#define SECTOR_SIZE 512
#define PAGE_SIZE 2048

static uint8_t in_buf[PAGE_SIZE], out_buf[PAGE_SIZE];
static uint8_t result[DC_MAX];

#include <drivers/flash.h>
#include <partition/partition.h>
uint32_t get_udisk_offset(void)
{
	const struct partition_entry *parti;

#ifdef CONFIG_BOARD_NANDBOOT
	parti = partition_get_stf_part(STORAGE_ID_BOOTNAND, PARTITION_FILE_ID_UDISK);
#else
	parti = partition_get_stf_part(STORAGE_ID_NAND, PARTITION_FILE_ID_UDISK);
#endif
	if (parti != NULL) {
	    LOG_INF("udisk offset=0x%x secotr\n", parti->offset);
		return (parti->offset);
	}
    LOG_ERR("get udisk offset err!\n");
    return 0;
}

uint32_t get_udisk_size(void)
{
	const struct partition_entry *parti;

#ifdef CONFIG_BOARD_NANDBOOT
	parti = partition_get_stf_part(STORAGE_ID_BOOTNAND, PARTITION_FILE_ID_UDISK);
#else
	parti = partition_get_stf_part(STORAGE_ID_NAND, PARTITION_FILE_ID_UDISK);
#endif
	if (parti != NULL) {
	    LOG_INF("udisk size=0x%x\n", parti->size);
		return (parti->size);
	}
    LOG_ERR("get udisk size err!\n");
    return 0;
}

extern int spinand_get_chipid(struct spinand_info *sni, u32_t *chipid);
extern int get_storage_params(struct spinand_info *sni, u8_t *id, struct FlashChipInfo **ChipInfo);

int spinand_scan_delaychain(const struct device *dev)
{
    int i = 0;
    uint8_t j = 0;
    uint8_t sum = 0;
    uint8_t ret = 0;
    uint8_t tmp_buf[SECTOR_SIZE];
    uint8_t dl = 0;
    int dvfs_level = 0;
    uint8_t spi_clk = 0;
    uint8_t min_dc = 0;
    uint8_t max_dc = 0;

    struct spinand_info *sni = DEV_DATA(dev);
    uint8_t old_dc = sni->spi->delay_chain;

    uint32_t chipid = 0;
    struct FlashChipInfo *NandFlashInfo;
    u32_t offset = get_udisk_offset();
    u32_t len = SECTOR_SIZE;
    
    struct k_mutex mutex;
    k_mutex_init(&mutex);
    memset(result, 0x0, DC_MAX);
    memset(tmp_buf, 0x0, SECTOR_SIZE);
    memset(in_buf, 0x0, SECTOR_SIZE);

    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    p_spinand_api->read(sni, offset >> 9, tmp_buf, len >> 9);
#else
    spinand_read(sni, offset >> 9, tmp_buf, len >> 9);
#endif
    k_mutex_unlock(&mutex);

    //fill one sector data
    for (i = 0; i < SECTOR_SIZE; i++) {
        in_buf[i] = (uint8_t)i;
    }

    dvfs_lock();
    for (dl = 20; dl <= DVFS_LEVEL_HIGH_PERFORMANCE; (dl=dl+10)) {
#ifdef CONFIG_DVFS
        dvfs_level = dvfs_get_current_level();
        dvfs_force_unset_level(dvfs_level, "monkey");
        dvfs_force_set_level(dl, "monkey");
#endif
        if (dl >= DVFS_LEVEL_MID_PERFORMANCE)
            spi_clk = CONFIG_SPINAND_FLASH_FREQ_MHZ;
        else
            spi_clk = 64;

        k_mutex_lock(&mutex, K_FOREVER);
        clk_set_rate(CLOCK_ID_SPI0+CONFIG_SPINAND_USE_SPICONTROLER, MHZ(spi_clk));
        k_mutex_unlock(&mutex);
        min_dc = 0;
        max_dc = 0;

        for (i = 0; i < DC_MAX; i++)
        {
            sni->spi->delay_chain = i;

            if (spinand_get_chipid(sni, &chipid) != 0) {
                printk("delay_chain:%d; spinand get chipid err!\n", i);
                continue;
            }
            if (get_storage_params(sni, (u8_t *)&chipid, &NandFlashInfo) != 0) {
                printk("delay_chain:%d; Can't get flashinfo.\n", i);
                continue;
            }

            printk("spinand chipid: 0x%x; chipname: %s; delay_chain: %d \n", chipid, NandFlashInfo->FlashMark, i);
            
            k_mutex_lock(&mutex, K_FOREVER);
            ret = spinand_write(sni, offset >> 9, in_buf, len >> 9);
            k_mutex_unlock(&mutex);
            if (ret != 0) {
                printk("delaychain=%d fail!\n", i);
                result[i] = 0;
                continue;
            }
            memset(out_buf, 0x0, SECTOR_SIZE);
            k_mutex_lock(&mutex, K_FOREVER);
            spinand_read(sni, offset >> 9, out_buf, len >> 9);
            k_mutex_unlock(&mutex);
            ret = memcmp(out_buf, in_buf, SECTOR_SIZE);
            if (ret != 0) {
                printk("delaychain=%d fail!\n", i);
                result[i] = 0;
                continue;
            }
            printk("delaychain=%d pass!\n", i);
            result[i] = 1;
            if (min_dc == 0)
                min_dc = i;
            max_dc = i;
        }
        printk("spinand scan delaychain result:\n");
        for (i = 0; i < DC_MAX; i++) {
            printk("[%d]=%d\r\n", i, result[i]);
            if (result[i] == 1) {
                j += 1;
                sum += i;
            }
        }
        
        if (j != 0) {
            printk("spinand best delaychain is(dvfs_level=%d; spi_clk=%d): %d; old delaychain before test is: %d \n", dl, spi_clk, (min_dc+max_dc)/2, old_dc);
            //writeback udisk sector.
            k_mutex_lock(&mutex, K_FOREVER);
            spinand_write(sni, offset >> 9, tmp_buf, len >> 9);
            k_mutex_unlock(&mutex);
            sni->spi->delay_chain = old_dc;
        } else {
            printk("spinand scan delaychain failed! get all delaychain is 0!\n");
        }
    }

    dvfs_unlock();

    return 0;
}

int spinand_manual_flush(const struct device *dev)
{
    struct spinand_info *sni = DEV_DATA(dev);
    struct k_mutex mutex;
    k_mutex_init(&mutex);
    k_mutex_lock(&mutex, K_FOREVER);
#ifndef CONFIG_SPINAND_LIB
    p_spinand_api->flush(sni);
#else
    spinand_flush(sni, 0);
#endif
    k_mutex_lock(&mutex, K_FOREVER);

    return 0;
}

