/*
 * Copyright (c) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Host Storage class driver header file
 *
 * Header file for USB Host Storage class driver
 */

#ifndef __USB_HOST_STORAGE_PROTOCOL_H__
#define __USB_HOST_STORAGE_PROTOCOL_H__

/* Bulk-only Command Block Wrapper (CBW) */
struct bulk_cb_wrap {
	u32_t Signature;
	u32_t Tag;
	u32_t DataLength;
	u8_t  Flags;
	u8_t  LUN;
	u8_t  CBLength;
	u8_t  CB[16];
} __packed;

#define US_BULK_CB_WRAP_LEN	31
#define US_BULK_CB_SIGN		0x43425355	/*spells out USBC */
#define US_BULK_FLAG_IN		(1 << 7)
#define US_BULK_FLAG_OUT	0

/* Bulk-only Command Status Wrapper (CSW) */
struct bulk_cs_wrap {
	u32_t Signature;
	u32_t Tag;
	u32_t DataResidue;
	u8_t  Status;
} __packed;

/* For Read Capacity Command */
typedef struct {
	u32_t lba;
	u32_t blksz;
} read_cap_data_t;

#define US_BULK_CS_WRAP_LEN	13
#define US_BULK_CS_SIGN		0x53425355      /* spells out 'USBS' */
#define US_BULK_STAT_OK		0
#define US_BULK_STAT_FAIL	1
#define US_BULK_STAT_PHASE	2

/* SCSI Commands */
#define TEST_UNIT_READY            0x00
#define REQUEST_SENSE              0x03
#define FORMAT_UNIT                0x04
#define INQUIRY                    0x12
#define MODE_SELECT6               0x15
#define MODE_SENSE6                0x1A
#define START_STOP_UNIT            0x1B
#define MEDIA_REMOVAL              0x1E
#define READ_FORMAT_CAPACITIES     0x23
#define READ_CAPACITY              0x25
#define READ10                     0x28
#define WRITE10                    0x2A
#define VERIFY10                   0x2F
#define READ12                     0xA8
#define WRITE12                    0xAA
#define MODE_SELECT10              0x55
#define MODE_SENSE10               0x5A
#define MODE_SENSE6                0x1A

/* bulk-only class specific requests */
#define US_BULK_RESET_REQUEST   0xff
#define US_BULK_GET_MAX_LUN     0xfe

#define INQUIRY_LENGTH			0x24
#define REQUEST_SENSE_LENGTH	0x12
#define READ_CAPACITY_LENGTH	0x08
#define MODE_SENSE_LENGTH		0x04

/* Peripheral Device Type */
#define DIRECT_ACCESS_DEVICE           0x00

/* usb storage bulk transfer return codes, in order of severity */
#define USB_STOR_XFER_GOOD		0	/* good transfer */
#define USB_STOR_XFER_SHORT		1	/* transferred less than expected */
#define USB_STOR_XFER_STALLED	2	/* endpoint stalled */
#define USB_STOR_XFER_LONG		3	/* device tried to send too much */
#define USB_STOR_XFER_ERROR		4	/* transfer died in the middle */
#define USB_STOR_XFER_FAILED	5	/* Transport good, command failed   */
#define USB_STOR_XFER_NODEV		6	/* device disconnected */

#endif /* __USB_HOST_STORAGE_PROTOCOL_H__ */
