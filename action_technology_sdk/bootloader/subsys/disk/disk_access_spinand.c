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
#include <drivers/spinand.h>
#include <partition/partition.h>


#define CONFIG_SPINAND_DEV_NAME "spinand"

const struct device *spinand_disk;
//static u32_t spinand_disk_sector_cnt;

int spinand_disk_status(struct disk_info *disk)
{
	if (!spinand_disk) {
		return DISK_STATUS_NOMEDIA;
	}

	return DISK_STATUS_OK;
}

int spinand_disk_initialize(struct disk_info *disk)
{
	uint32_t sector_cnt;
	int ret;

	if (spinand_disk) {
		return 0;
	}

	spinand_disk = device_get_binding(CONFIG_SPINAND_DEV_NAME);
	if (!spinand_disk) {
		return -ENODEV;
	}

	ret = spinand_storage_ioctl((struct device *)spinand_disk, DISK_IOCTL_GET_SECTOR_COUNT, (void *)&sector_cnt);
	if (!ret) {
		//sd_disk_sector_cnt = sector_cnt;
		if(disk->sector_cnt){
			if(disk->sector_offset + disk->sector_cnt > sector_cnt)
				printk("error:nand disk part over  max  capcity = %d\n", sector_cnt);
		}else{
			disk->sector_cnt = sector_cnt;
		}
		//spinand_disk_sector_cnt = sector_cnt;
	} else {
		printk("failed to get sector count error=%d\n", ret);
		return -EFAULT;
	}
	return 0;
}

int spinand_disk_read(struct disk_info *disk, uint8_t *buff, uint32_t start_sector,
		     uint32_t sector_count)
{
    //boot_sector is reserved for mbrec and param.
    //int boot_sector = (2048 * 64 * 4) >> 9;

	if (!spinand_disk) {
		return -EIO;
	}

	//if (spinand_disk_sector_cnt == 0) {
		//goto read;
	//}

	if (start_sector >  disk->sector_cnt - 1) {
		return -EIO;
	}

	//if (start_sector + sector_count > spinand_disk_sector_cnt) {
		//sector_count = spinand_disk_sector_cnt - start_sector;
	//}
	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;


//read:
    //start_sector += boot_sector;
	if (flash_read(spinand_disk, start_sector<<9, buff, sector_count<<9) != 0) {
		return -EIO;
	}
	return 0;
}

int spinand_disk_write(struct disk_info *disk, const uint8_t *buff, uint32_t start_sector,
		      uint32_t sector_count)
{
    //boot_sector is reserved for mbrec and param.
    //int boot_sector = (2048 * 64 * 4) >> 9;

	if (!spinand_disk) {
		return -EIO;
	}

	//if (spinand_disk_sector_cnt == 0) {
		//goto write;
	//}

	if (start_sector >  disk->sector_cnt - 1) {
		return -EIO;
	}

	//if (start_sector + sector_count > spinand_disk_sector_cnt) {
		//sector_count = spinand_disk_sector_cnt - start_sector;
	//}
	if (start_sector + sector_count > disk->sector_cnt) {
		sector_count = disk->sector_cnt - start_sector;
	}
	start_sector += disk->sector_offset;

//write:
   // start_sector += boot_sector;
	if (flash_write(spinand_disk, start_sector<<9, buff, sector_count<<9) != 0) {
		return -EIO;
	}

	return 0;
}

int spinand_disk_ioctl(struct disk_info *disk, uint8_t cmd, void *buff)
{
	int ret = 0;

	if (!spinand_disk) {
		return -ENODEV;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
        //todo
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		if (disk->sector_cnt > 0) {
			*(uint32_t *)buff = disk->sector_cnt;
		} else {
			ret = spinand_storage_ioctl((struct device *)spinand_disk, cmd, buff);
		}
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
	case DISK_IOCTL_HW_DETECT:
        ret = spinand_storage_ioctl((struct device *)spinand_disk, cmd, buff);
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

struct disk_operations disk_spinand_operation = {
	.init		= spinand_disk_initialize,
	.status	= spinand_disk_status,
	.read		= spinand_disk_read,
	.write		= spinand_disk_write,
	.ioctl		= spinand_disk_ioctl,
};

struct disk_info disk_spinand_mass = {
	.name		= "NAND",
	.ops		= &disk_spinand_operation,
    .sector_size = 512,
	.sector_offset	= 0,
	.sector_cnt	= 0,
};

static int spinand_disk_init(const struct device *dev)
{
	const struct partition_entry *parti;

	ARG_UNUSED(dev);
	parti = partition_get_stf_part(STORAGE_ID_NAND, PARTITION_FILE_ID_UDISK);
	if(parti != NULL){
		disk_spinand_mass.sector_offset = parti->offset >> 9;
		disk_spinand_mass.sector_cnt = parti->size >> 9;
	}
	printk("spinand_disk_init,size=%d, off=%d secotr\n", disk_spinand_mass.sector_cnt, disk_spinand_mass.sector_offset);
	disk_access_register(&disk_spinand_mass);
	return 0;
}

SYS_INIT(spinand_disk_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

