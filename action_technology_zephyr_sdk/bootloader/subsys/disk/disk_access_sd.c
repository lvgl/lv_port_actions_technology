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
#include <drivers/mmc/sd.h>
#include <partition/partition.h>



#define CONFIG_MMC_SDCARD_DEV_NAME "sd"

const struct device *sd_disk;
//static u32_t sd_disk_sector_cnt;

static int sd_disk_status(struct disk_info *disk)
{
	if (!sd_disk) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

static int sd_disk_initialize(struct disk_info *disk)
{
	u32_t sector_cnt;
	int ret;

	if (sd_disk) {
		return 0;
	}

	sd_disk = device_get_binding(CONFIG_MMC_SDCARD_DEV_NAME);
	if (!sd_disk) {
		return -ENODEV;
	}

	if (0 != sd_card_storage_init(sd_disk)) {
		sd_disk = NULL;
		return -ENODEV;
	}
	ret = sd_card_storage_ioctl(sd_disk, DISK_IOCTL_GET_SECTOR_COUNT, (void *)&sector_cnt);
	if (!ret) {
		//sd_disk_sector_cnt = sector_cnt;
		if(disk->sector_cnt){
			if(disk->sector_offset + disk->sector_cnt > sector_cnt)
				printk("error:sd disk part over sd max  capcity = %d\n", sector_cnt);
		}else{
			disk->sector_cnt = sector_cnt;
		}
	} else {
		printk("failed to get sector count error=%d\n", ret);
		sd_disk = NULL;
		return -EFAULT;
	}

	return 0;
}

static int sd_disk_read(struct disk_info *disk, uint8_t *buff, uint32_t start_sector,
		     uint32_t sector_count)
{
	uint32_t start_addr;
	uint32_t len;

	if (!sd_disk) {
		return -ENODEV;
	}

	//if (sd_disk_sector_cnt == 0) {
		//goto read;
	//}

	if ((start_sector >= disk->sector_cnt) || (sector_count == 0)){
		return -EIO;
	}

	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;

	start_addr = start_sector << 9;
	len        = sector_count << 9;

//read:
	/* is it OK? */
	if (flash_read(sd_disk, start_addr, buff, len) != 0) {
		return -EIO;
	}
	return 0;
}

static int sd_disk_write(struct disk_info *disk, const uint8_t *buff, uint32_t start_sector,
		      uint32_t sector_count)
{
	uint32_t start_addr;
	uint32_t len;

	if (!sd_disk) {
		return -ENODEV;
	}

	//if (sd_disk_sector_cnt == 0) {
		//goto write;
	//}

	if ((start_sector >= disk->sector_cnt) || (sector_count == 0)){
		return -EIO;
	}


	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;

	start_addr = start_sector << 9;
	len 	   = sector_count << 9;

//write:
	if (flash_write(sd_disk, start_addr, buff, len) != 0) {
		return -EIO;
	}

	return 0;
}

static int sd_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	int ret = 0;
	const struct device *tmp_disk;

	if (!sd_disk && (cmd != DISK_IOCTL_HW_DETECT)) {
		return -ENODEV;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		if (disk->sector_cnt > 0) {
			*(uint32_t *)buff = disk->sector_cnt;
		} else {
			ret = sd_card_storage_ioctl(sd_disk, cmd, buff);
		}
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
	//case DISK_IOCTL_GET_DISK_SIZE:
		ret = sd_card_storage_ioctl(sd_disk, cmd, buff);
		break;

	case DISK_IOCTL_HW_DETECT:
		if (sd_disk) {
			ret = sd_card_storage_ioctl(sd_disk, cmd, buff);
		} else {
			tmp_disk = device_get_binding(CONFIG_MMC_SDCARD_DEV_NAME);
			if (tmp_disk) {
				ret = sd_card_storage_ioctl(tmp_disk, cmd, buff);
			} else {
				return -ENODEV;
			}
		}

		if (ret == 0) {
			if (*(uint8_t *)buff != STA_DISK_OK) {
				sd_disk = NULL;
			}
		}
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

struct disk_operations disk_sd_operation = {
	.init		= sd_disk_initialize,
	.status	= sd_disk_status,
	.read		= sd_disk_read,
	.write		= sd_disk_write,
	.ioctl		= sd_disk_ioctl,
};

struct disk_info disk_sd_mass = {
	.name		= "SD",
	.sector_size = 512,
	.sector_offset	= 0,
	.sector_cnt	= 0,
	.ops		= &disk_sd_operation,
};

static int sd_disk_init(const struct device *dev)
{
	const struct partition_entry *parti;

	ARG_UNUSED(dev);
	parti = partition_get_stf_part(STORAGE_ID_SD, PARTITION_FILE_ID_UDISK);
	if(parti != NULL){
		disk_sd_mass.sector_offset = parti->offset >> 9;
		disk_sd_mass.sector_cnt = parti->size >> 9;
	}

	printk("sd_disk_init,size=%d, off=%d secotr\n", disk_sd_mass.sector_cnt, disk_sd_mass.sector_offset);
	disk_access_register(&disk_sd_mass);
	return 0;
}

SYS_INIT(sd_disk_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

