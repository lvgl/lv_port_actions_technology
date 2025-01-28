/*
 * Copyright (c) 2016 Actions Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/types.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <disk/disk_access.h>
#include <errno.h>
#include <init.h>
#include <device.h>
#include <drivers/flash.h>
#include <partition/partition.h>
#include <board_cfg.h>


#define NOR_SECTOR_SIZE 512
#define NOR_ERASE_SIZE 4096
#define NOR_CACHE_NUM_SECTOR  (NOR_ERASE_SIZE/NOR_SECTOR_SIZE)

#define STA_DISK_OK		0x08	/* Medium OK in the drive */


const struct device *nor_disk;
/* lock to protect cache buf */
static struct k_mutex n_mutex;
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(diskio.cache.pool)
#endif
static u8_t __aligned(4) nor_cache_data[NOR_ERASE_SIZE];
static bool  b_cache_valid, b_write_valid;
static uint32_t nor_cache_start_sec;


static int nor_disk_status(struct disk_info *disk)
{
	if (!nor_disk) {
		return DISK_STATUS_NOMEDIA;
	}
	return DISK_STATUS_OK;
}

static int nor_disk_initialize(struct disk_info *disk)
{
	if (nor_disk) {
		return 0;
	}
	return -ENODEV;
}
static int nor_cache_flush(void)
{
	int ret = 0;
	if(b_write_valid){
		ret =  flash_erase(nor_disk, nor_cache_start_sec<<9, NOR_ERASE_SIZE);
		ret |= flash_write(nor_disk, nor_cache_start_sec<<9, nor_cache_data, NOR_ERASE_SIZE);
		b_write_valid = false;
		//qprintk("nor flush off=0x%x\n", nor_cache_start_sec*512);
	}
	return ret;
}
static void nor_cache_buf_update(uint32_t off_sec)
{
	if(off_sec & (NOR_CACHE_NUM_SECTOR-1)){
		printk("-----error, off_sec=0x%x not align to erase size-----\n", off_sec);
		return;
	}
	if(b_cache_valid){
		if(off_sec != nor_cache_start_sec){
			nor_cache_flush();			
			nor_cache_start_sec = off_sec;
			flash_read(nor_disk, nor_cache_start_sec<<9, nor_cache_data, NOR_ERASE_SIZE);
		}
	}else{			
		nor_cache_start_sec = off_sec;
		flash_read(nor_disk, nor_cache_start_sec<<9, nor_cache_data, NOR_ERASE_SIZE);
		b_cache_valid = true;
	}
}

static int nor_disk_read(struct disk_info *disk, uint8_t *buff, uint32_t start_sector,
		     uint32_t sector_count)
{
	int ret = 0;
	uint32_t nsec, off_sec, sec_index;

	if (!nor_disk ) {
		return -ENODEV;
	}

	if ((start_sector >= disk->sector_cnt) || (sector_count == 0)){
		return -EIO;
	}
	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;

	//printk("-nor disk read off=0x%x , sec=0x%x\n", start_sector , sector_count);
	k_mutex_lock(&n_mutex, K_FOREVER);

	if(b_cache_valid){
		off_sec = start_sector & (~(NOR_CACHE_NUM_SECTOR-1));
		if(off_sec == nor_cache_start_sec){
			sec_index = (start_sector & (NOR_CACHE_NUM_SECTOR-1));
			nsec = NOR_CACHE_NUM_SECTOR - sec_index;
			if(nsec > sector_count)
				nsec = sector_count;
			memcpy(buff, nor_cache_data+(sec_index<<9), nsec<<9);
			buff += nsec<<9;
			start_sector += nsec;
			sector_count -= nsec;
		}
	}

	if(sector_count){
		//nor_cache_flush();
		ret = flash_read(nor_disk, start_sector<<9, buff, sector_count<<9);
	}
	k_mutex_unlock(&n_mutex);
	if(ret)
		return -EIO;
	return 0;
}

static int nor_disk_write(struct disk_info *disk, const uint8_t *buff, uint32_t start_sector,
		      uint32_t sector_count)
{
	int ret = 0;
	uint32_t nsec, off_sec, sec_index;
	if (!nor_disk ) {
		return -ENODEV;
	}

	if ((start_sector >= disk->sector_cnt) || (sector_count == 0)){
		return -EIO;
	}
	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;

	//printk("-nor disk write off=0x%x , sec=0x%x\n", start_sector*512 , sector_count);
	k_mutex_lock(&n_mutex, K_FOREVER);
	sec_index = (start_sector & (NOR_CACHE_NUM_SECTOR-1));
	if(sec_index) { // offset not align to erase sector
		off_sec = start_sector & (~(NOR_CACHE_NUM_SECTOR-1)) ;
		nor_cache_buf_update(off_sec);
		nsec = NOR_CACHE_NUM_SECTOR - sec_index;
		if(nsec > sector_count)
			nsec = sector_count;
		memcpy(nor_cache_data+(sec_index<<9), buff, nsec<<9);
		buff += nsec<<9;
		start_sector += nsec;
		sector_count -= nsec;
		b_write_valid = true;
		//printk("head off=0x%x\n", nor_cache_start_sec*512);
	}
	if(sector_count >= NOR_CACHE_NUM_SECTOR){
		nsec = sector_count & (~(NOR_CACHE_NUM_SECTOR-1));
		ret = flash_erase(nor_disk,  start_sector<<9, nsec<<9);
		ret |= flash_write(nor_disk, start_sector<<9, buff, nsec<<9);
		if(b_cache_valid && (start_sector <= nor_cache_start_sec) && (nor_cache_start_sec <= start_sector +nsec)){
			b_cache_valid = false;
			b_write_valid = false;
			printk("nor cache lost off=0x%x\n", nor_cache_start_sec*512);
		}
		buff += nsec<<9;
		start_sector += nsec;
		sector_count -= nsec;
	}

	if(sector_count){ // size  not align to erase sector
		nor_cache_buf_update(start_sector);
		memcpy(nor_cache_data, buff, sector_count<<9);
		//printk("tail off=0x%x\n", nor_cache_start_sec*512);
		b_write_valid = true;
	}

	k_mutex_unlock(&n_mutex);
	if(ret)
		return -EIO;
	return 0;
}

static int nor_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	int ret = 0;
	if (!nor_disk )
		return -ENODEV;

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		printk("-nor sync\n");
		k_mutex_lock(&n_mutex, K_FOREVER);
		nor_cache_flush();
		k_mutex_unlock(&n_mutex);
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = disk->sector_cnt;
		break;
    case DISK_IOCTL_GET_SECTOR_SIZE:
        *(uint32_t *)buff = NOR_SECTOR_SIZE;
        break;
    case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
        *(uint32_t *)buff  = NOR_SECTOR_SIZE;
        break;
	case DISK_IOCTL_HW_DETECT:
		*(uint8_t *)buff = STA_DISK_OK;
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

struct disk_operations disk_nor_operation = {
	.init		= nor_disk_initialize,
	.status		= nor_disk_status,
	.read		= nor_disk_read,
	.write		= nor_disk_write,
	.ioctl		= nor_disk_ioctl,
};

struct disk_info disk_nor_mass = {
	.name		= "NOR",
	.sector_size = NOR_SECTOR_SIZE,
	.sector_offset	= 0,
	.sector_cnt	= 0,
	.ops		= &disk_nor_operation,
};

static int nor_disk_init(const struct device *dev)
{
	const struct partition_entry *parti;

	ARG_UNUSED(dev);
	k_mutex_init(&n_mutex);
	b_write_valid = b_cache_valid = false;
	nor_disk = NULL;
	parti = partition_get_stf_part(STORAGE_ID_DATA_NOR, PARTITION_FILE_ID_UDISK);
	if(parti == NULL){
		printk("nor disk:ext nor\n");
		parti = partition_get_stf_part(STORAGE_ID_NOR, PARTITION_FILE_ID_UDISK);
		if(parti)
			nor_disk = device_get_binding(CONFIG_SPI_FLASH_NAME);
	}else{
		printk("nor disk:data nor\n");
		nor_disk = device_get_binding(CONFIG_SPI_FLASH_2_NAME);
	}

	if (!nor_disk) {
		printk("nor disk: not nor dev\n");
		return -ENODEV;
	}

	if(parti != NULL){
		disk_nor_mass.sector_offset = parti->offset >> 9;
		disk_nor_mass.sector_cnt = parti->size >> 9;
	}

	printk("nor_disk_init,size=0x%x, off=0x%x secotr\n", disk_nor_mass.sector_cnt, disk_nor_mass.sector_offset);
	disk_access_register(&disk_nor_mass);
	return 0;
}

SYS_INIT(nor_disk_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

