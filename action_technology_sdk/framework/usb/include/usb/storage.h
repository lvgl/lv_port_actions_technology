/*
 *  Copyright (c) 2016 Actions Corporation
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief Public API for usb-storage drivers
 */

#ifndef __USB_STORAGE_H__
#define __USB_STORAGE_H__

/**
 * @brief USB disk IO Interface
 * @defgroup storage_interface USB disk IO Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <disk/disk_access.h>
#include <drivers/mmc/sd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*storage_api_read)(struct device *dev, off_t offset, void *data,
				size_t len);
typedef int (*storage_api_write)(struct device *dev, off_t offset,
				const void *data, size_t len);
typedef int (*storage_api_ioctl)(struct device *dev, uint8_t cmd, void *buff);

struct storage_driver_api {
	storage_api_read read;
	storage_api_write write;
	storage_api_ioctl ioctl;
};

/* Get the number of sectors in the usb disk  */
#define USB_DISK_IOCTL_GET_SECTOR_COUNT	1
/* Get the size of a usb disk SECTOR in bytes */
#define USB_DISK_IOCTL_GET_SECTOR_SIZE	2
/* Get the size of the usb disk in bytes */
#define USB_DISK_IOCTL_GET_DISK_SIZE	3
/* Get usb disk status: connected/disconnected */
#define USB_DISK_IOCTL_GET_STATUS		4

/**
 *  @brief  Read data from usb disk
 *
 *  @param  dev             : USB disk dev
 *  @param  offset          : Offset (byte aligned) to read
 *  @param  data            : Buffer to store read data
 *  @param  len             : Number of sectors to read.
 *
 *  @return  0 on success, negative errno code on fail.
 */
static inline int storage_read(struct device *dev, off_t offset, void *data,
			     size_t len)
{
	const struct storage_driver_api *api = dev->api;

	return api->read(dev, offset, data, len);
}

/**
 *  @brief  Write buffer into usb disk.
 *
 *  @param  dev             : USB disk device
 *  @param  offset          : starting offset for the write
 *  @param  data            : data to write
 *  @param  len             : Number of sectors to write
 *
 *  @return  0 on success, negative errno code on fail.
 */
static inline int storage_write(struct device *dev, off_t offset,
			      const void *data, size_t len)
{
	const struct storage_driver_api *api = dev->api;

	return api->write(dev, offset, data, len);
}

/**
 *  @brief  Read meta data from usb disk
 *
 *  @param  dev             : USB disk dev
 *  @param  cmd            : USB disk ioctl cmd type
 *  @param  buff            : Buffer to store read data
 *
 *  @return  0 on success, negative errno code on fail.
 */
static inline int storage_ioctl(struct device *dev, uint8_t cmd, void *buff)
{
	const struct storage_driver_api *api = dev->api;

	return api->ioctl(dev, cmd, buff);
}


/**
 * USB disk hotplug support
 * NOTE: should keep pace with Disk Status Bits (DSTATUS)
 * which defined in diskio.h.
 */
#define USB_DISK_NOOP			0x0
/* STA_DISK_OK */
#define USB_DISK_CONNECTED		STA_DISK_OK
/* STA_NODISK */
#define USB_DISK_DISCONNECTED	STA_NODISK

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __USB_STORAGE_H__ */
