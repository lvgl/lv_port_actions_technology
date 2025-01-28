/* usb_msc.h - USB MSC public header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on mass_storage.h
 *
 * This file are derived from material that is
 * Copyright (c) 2010-2011 mbed.org, MIT License
 * Copyright (c) 2016 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */


/**
 * @file
 * @brief USB Mass Storage Class public header
 *
 * Header follows the Mass Storage Class Specification
 * (Mass_Storage_Specification_Overview_v1.4_2-19-2010.pdf) and
 * Mass Storage Class Bulk-Only Transport Specification
 * (usbmassbulk_10.pdf).
 * Header is limited to Bulk-Only Transfer protocol.
 */

#ifndef __USB_MSC_H__
#define __USB_MSC_H__

/** MSC Subclass and Protocol Codes */
#define SCSI_TRANSPARENT_SUBCLASS	0x06
#define BULK_ONLY_TRANSPORT_PROTOCOL	0x50

/** MSC Request Codes for Bulk-Only Transport */
#define MSC_REQUEST_GET_MAX_LUN		0xFE
#define MSC_REQUEST_RESET		0xFF

/** MSC Command Block Wrapper (CBW) Signature */
#define CBW_Signature			0x43425355

/** MSC Command Block Wrapper Flags */
#define CBW_DIRECTION_DATA_IN		0x80

/** MSC Bulk-Only Command Block Wrapper (CBW) */
struct CBW {
	uint32_t Signature;
	uint32_t Tag;
	uint32_t DataLength;
	uint8_t  Flags;
	uint8_t  LUN;
	uint8_t  CBLength;
	uint8_t  CB[16];
} __packed;

/** MSC Command Status Wrapper (CBW) Signature */
#define CSW_Signature			0x53425355

/** MSC Command Block Status Values */
#define CSW_STATUS_CMD_PASSED		0x00
#define CSW_STATUS_CMD_FAILED		0x01
#define CSW_STATUS_PHASE_ERROR		0x02

/** MSC Bulk-Only Command Status Wrapper (CSW) */
struct CSW {
	uint32_t Signature;
	uint32_t Tag;
	uint32_t DataResidue;
	uint8_t  Status;
} __packed;

/** SCSI transparent command set used by MSC */
#define TEST_UNIT_READY			0x00
#define REQUEST_SENSE			0x03
#define FORMAT_UNIT			0x04
#define INQUIRY				0x12
#define MODE_SELECT6			0x15
#define MODE_SENSE6			0x1A
#define START_STOP_UNIT			0x1B
#define MEDIA_REMOVAL			0x1E
#define READ_FORMAT_CAPACITIES		0x23
#define READ_CAPACITY			0x25
#define READ10				0x28
#define WRITE10				0x2A
#define VERIFY10			0x2F
#define READ12				0xA8
#define WRITE12				0xAA
#define MODE_SELECT10			0x55
#define MODE_SENSE10			0x5A
/* For Actions Tool */
#define CMD_INQUIRY_ADFU	0xCC
#define CMD_SWITCH_TO_ADFU	0xCB
#define CMD_ADFU_MISC	0xCD
/* Opcode for CMD_ADFU_MISC */
#define ADFU_MISC_RESTART	0x7E
#define ADFU_MISC_SET_SERIAL_NUMBER	0x7F
#define ADFU_MISC_GET_SERIAL_NUMBER	0xFF
#define ADFU_MISC_SET_MISCINFO	0x7D
#define ADFU_MISC_GET_MISCINFO	0xFD
#define ADFU_MISC_GET_CHIPID	0xFC
#define ADFU_MISC_SWITCH_TO_STUB	0xCB
/* set time and date */
#define CMD_SET_TIME_AND_DATE  0xB0

/** MSC disk type */
#define MSC_DISK_NOR	0
#define MSC_DISK_NAND	1
#define MSC_DISK_PSRAM	2
#define MSC_DISK_USB	3
#define MSC_DISK_SD	4
#define MSC_DISK_RAM	5
#define MSC_DISK_NUM	6
/* NO_MEDIA should be controlled by msc internally */
#define MSC_DISK_NO_MEDIA	0xfe
#define MSC_DISK_NO_LUN	0xff

/* called in interrupt context */
typedef void (*usb_msc_eject_callback)(void);

/**
 * @brief USB mass storage thread entry
 */
void usb_mass_storage_thread(void *p1, void *p2, void *p3);

/**
 * @brief whether USB mass storage is working (disk reading/writing)
 *
 * @return true on working, false on not.
 */
bool usb_mass_storage_working(void);

/*
 * API: whether USB mass storage is reading
 */
bool usb_mass_storage_reading(void);

/*
 * API: whether USB mass storage is writing
 */
bool usb_mass_storage_writing(void);

/**
 * @brief Add a new lun for USB mass storage
 *
 * @param[in]  disk_no      MSC_DISK_TYPE
 */
int usb_mass_storage_add_lun(uint8_t disk_no);

/**
 * @brief Remove current lun for USB mass storage synchronously
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_mass_storage_remove_lun(void);

/**
 * @brief Register eject callback for USB mass storage
 *
 * @param[in]  cb      callback function
 */
void usb_mass_storage_register_eject_cb(usb_msc_eject_callback cb);

/**
 * @brief Register disk for USB mass storage
 *
 * @param[in]  disk_no      MSC_DISK_TYPE
 */
void usb_mass_storage_register(uint8_t disk_no);

/**
 * @brief Initialize USB mass storage device
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_mass_storage_init(struct device *dev, u8_t *buf, u32_t len);

/**
 * @brief De-Initialize USB mass storage device synchronously
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_mass_storage_exit(void);

/**
 * @brief whether USB mass storage is ejected
 *
 * @return true on ejected, false on not.
 */
bool usb_mass_storage_ejected(void);

/**
 * @brief whether USB mass storage is enabled
 *
 * @return true on enabled, false on not.
 */
bool usb_mass_storage_enabled(void);

/**
 * @brief whether USB mass storage is enter stub mode
 *
 * @return true on stub mode, false on not.
 */
bool usb_mass_storage_stub_mode(void);

/**
 * @brief start USB mass storage externally
 *
 * If USB_MASS_STORAGE_SHARE_THREAD not defined, this API does nothing,
 * otherwise usb_mass_storage_start() must be re-implemented externally.
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_mass_storage_start(void);

#endif /* __USB_MSC_H__ */
