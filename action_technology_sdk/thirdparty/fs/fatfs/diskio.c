/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <ffconf.h>		/* import ff config */

#define DISK_MAX_PHY_DRV	_VOLUMES

/* Number of volumes (logical drives) to be used. */
const char* const disk_volume_strs[] = {
	_VOLUME_STRS
};


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

static DSTATUS translate_error(int error)
{
	switch (error) {
	case 0:
		return RES_OK;
	case EINVAL:
		return RES_PARERR;
	case ENODEV:
		return RES_NOTRDY;
	default:
		return RES_ERROR;
	}
}

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	int result;

	if (pdrv >= DISK_MAX_PHY_DRV)
		return STA_NODISK;

	result = disk_access_status(disk_volume_strs[pdrv]);
	return translate_error(result);
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	int result;

	if (pdrv >= DISK_MAX_PHY_DRV)
		return STA_NODISK;

#ifdef CONFIG_DISKIO_CACHE
	diskio_cache_invalid(disk_volume_strs[pdrv]);
#endif

	result = disk_access_init(disk_volume_strs[pdrv]);
	return translate_error(result);

}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int result;

	if (pdrv >= DISK_MAX_PHY_DRV)
		return STA_NODISK;

#ifdef CONFIG_DISKIO_CACHE
	result = diskio_cache_read(disk_volume_strs[pdrv], buff, sector, count);
#else
	result = disk_access_read(disk_volume_strs[pdrv], buff, sector, count);
#endif
	return translate_error(result);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int result;

	if (pdrv >= DISK_MAX_PHY_DRV)
		return STA_NODISK;

#ifdef CONFIG_DISKIO_CACHE
	result = diskio_cache_write(disk_volume_strs[pdrv], buff, sector, count);
#else
	result = disk_access_write(disk_volume_strs[pdrv], buff, sector, count);
#endif
	return translate_error(result);
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	int result = RES_OK;

	if (pdrv >= DISK_MAX_PHY_DRV)
		return STA_NODISK;

	switch (cmd) {
	case CTRL_SYNC:
#ifdef CONFIG_DISKIO_CACHE
		diskio_cache_flush(disk_volume_strs[pdrv]);
#endif
		if(disk_access_ioctl(disk_volume_strs[pdrv], DISK_IOCTL_CTRL_SYNC, buff) != 0) {
			result = RES_ERROR;
		}
		break;

	case GET_SECTOR_SIZE:
		if(disk_access_ioctl(disk_volume_strs[pdrv], DISK_IOCTL_GET_SECTOR_SIZE, buff) != 0) {
			result = RES_ERROR;
		}
		break;

	case GET_SECTOR_COUNT:
		if(disk_access_ioctl(disk_volume_strs[pdrv], DISK_IOCTL_GET_SECTOR_COUNT, buff) != 0) {
			result = RES_ERROR;
		}
		break;

	case GET_BLOCK_SIZE:
		if (disk_access_ioctl(disk_volume_strs[pdrv], DISK_IOCTL_GET_ERASE_BLOCK_SZ, buff) != 0) {
			result = RES_ERROR;
		}
		break;

	case DISK_HW_DETECT:
		if (disk_access_ioctl(disk_volume_strs[pdrv], DISK_IOCTL_HW_DETECT, buff) != 0) {
			result = RES_ERROR;
		}
		break;

	default:
		result = RES_PARERR;
		break;
	}
	return result;

}

