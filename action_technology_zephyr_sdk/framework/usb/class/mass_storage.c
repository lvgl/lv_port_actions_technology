/*
 * The Mass Storage protocol state machine in this file is based on mbed's
 * implementation. We augment it by adding Zephyr's USB transport and Storage
 * APIs.
 *
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
 */


/**
 * @file
 * @brief Mass Storage device class driver
 *
 * Driver for USB Mass Storage device class driver
 */

#include <init.h>
#include <errno.h>
#include <string.h>
#include <sys/byteorder.h>
#include <usb/class/usb_msc.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
#include <soc.h>
#endif

#include "mass_storage_descriptor.h"

#define LOG_LEVEL 2
#include <logging/log.h>
LOG_MODULE_REGISTER(mass_storage);

#include "diskio.h"

/* max USB packet size */
#if CONFIG_MASS_STORAGE_BULK_EP_MPS > USB_MAX_FS_BULK_MPS
#define MAX_PACKET	USB_MAX_FS_BULK_MPS	/* Bulk MaxPakcetSize in full-speed */
#else
#define MAX_PACKET	CONFIG_MASS_STORAGE_BULK_EP_MPS
#endif

/*
 * 1: USB will read all left data even if fail to write disk,
 * 0: We will just stall bulk-out endpoint and quit.
 */
#define DISK_WRITE_ERROR_CONTINUE	1
/*
 * 1: USB will read all data even if there is no medium.
 * 0: We will stall bulk-out endpoint.
 */
#define MSC_WRITE_NO_MEDIUM_CONTINUE	1

#define BLOCK_SIZE	512
#define DISK_THREAD_STACK_SZ	CONFIG_MASS_STORAGE_STACK_SIZE
#define DISK_THREAD_PRIO	CONFIG_MASS_STORAGE_PRIORITY

#define THREAD_OP_READ_QUEUED		1
#define THREAD_OP_READ_DONE		2
#define THREAD_OP_WRITE_QUEUED		3
#define THREAD_OP_WRITE_DONE		4

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SS_NO_SENSE				0
#define SS_COMMUNICATION_FAILURE		0x040800
#define SS_INVALID_COMMAND			0x052000
#define SS_INVALID_FIELD_IN_CDB			0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SS_MEDIUM_NOT_PRESENT			0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED		0x055302
#define SS_NOT_READY_TO_READY_TRANSITION	0x062800
#define SS_RESET_OCCURRED			0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SS_UNRECOVERED_READ_ERROR		0x031100
#define SS_WRITE_ERROR				0x030c02
#define SS_WRITE_PROTECTED			0x072700
#define SS_DATA_CRC_ERROR			0x0b4701

#define SK(x)		((u8_t) ((x) >> 16))	/* Sense Key byte, etc. */
#define ASC(x)		((u8_t) ((x) >> 8))	/* AddSenseCode*/
#define ASCQ(x)		((u8_t) (x))	/* AddSnsCodeQlfr */

static u32_t msc_sense_data;

#define MSC_UNKNOWN	0
/* MSC_RUNNING and MSC_ABORTED are mutual exceptional */
#define MSC_RUNNING	BIT(0)
#define MSC_ABORTED	BIT(1)
#define MSC_EJECTED	BIT(2)
#define MSC_PREVENT_MEDIA_REMOVAL	BIT(3)

static u8_t msc_state;

#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
static u8_t switch_to_adfu;
static struct k_work switch_to_adfu_work;
#endif

static struct k_work disk_sync_work;
static u8_t disk_sync_pdrv;

#define SWITCH_TO_STUB_START	1
#define SWITCH_TO_STUB_DONE	2

static u8_t switch_to_stub;

static inline bool msc_state_unknown(void)
{
	return msc_state == MSC_UNKNOWN;
}

static inline bool msc_state_running(void)
{
	return msc_state & MSC_RUNNING;
}

static inline bool msc_state_aborted(void)
{
	return msc_state & MSC_ABORTED;
}

static inline bool msc_state_ejected(void)
{
	return msc_state & MSC_EJECTED;
}

//static inline bool msc_state_prevent_media_removal(void)
//{
//	return msc_state & MSC_PREVENT_MEDIA_REMOVAL;
//}

static inline void msc_state_set_unknown(void)
{
	msc_state = MSC_UNKNOWN;
}

static inline void msc_state_set_running(void)
{
	msc_state |= MSC_RUNNING;
}

static inline void msc_state_clear_running(void)
{
	msc_state &= ~MSC_RUNNING;
}

static inline void msc_state_set_aborted(void)
{
	msc_state |= MSC_ABORTED;
}

static inline void msc_state_set_ejected(void)
{
	msc_state |= MSC_EJECTED;
}

__unused static inline void msc_state_set_prevent_media_removal(void)
{
	msc_state |= MSC_PREVENT_MEDIA_REMOVAL;
}

__unused static inline void msc_state_clear_prevent_media_removal(void)
{
	msc_state &= ~MSC_PREVENT_MEDIA_REMOVAL;
}

static usb_msc_eject_callback eject_cb;

static volatile int thread_op;
#ifndef CONFIG_USB_MASS_STORAGE_SHARE_THREAD
static K_THREAD_STACK_DEFINE(mass_thread_stack, DISK_THREAD_STACK_SZ);
static struct k_thread mass_thread_data;
#endif
static struct k_sem disk_wait_sem;
static struct k_sem usb_wait_sem;
static volatile u32_t defered_wr_sz;
static volatile u32_t defered_rd_off;

static u8_t *page;
static u8_t *page2;
static u32_t page_size;

#define USB_RW_TIMEOUT	K_MSEC(5000)

static u8_t usb_rw_working;

/* Initialized during mass_storage_init() */
static u64_t memory_size;
static u32_t block_count;
static u8_t disk_pdrv = CONFIG_MASS_STORAGE_DISK_PDRV;

#define MSD_OUT_EP_IDX			0
#define MSD_IN_EP_IDX			1

#ifdef CONFIG_STORAGE_APITEST
#include <drivers/flash.h>
#include <partition/partition.h>
static const struct device *flash_dev;
//#define UDISK_OFFSET 0x4a00000
static uint32_t get_udisk_offset(void)
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
#endif

static void mass_storage_process_suspend(void)
{
	/* stop */
	msc_state_set_ejected();

	disk_sync_pdrv = disk_pdrv;

	/* flush memory data to medium*/
	k_work_submit(&disk_sync_work);

	disk_pdrv = MSC_DISK_NO_MEDIA;
	if (eject_cb) {
		eject_cb();
	}
}
static void mass_storage_bulk_out(u8_t ep,
				  enum usb_dc_ep_cb_status_code ep_status);
static void mass_storage_bulk_in(u8_t ep,
				 enum usb_dc_ep_cb_status_code ep_status);

/* Describe EndPoints configuration */
static const struct usb_ep_cfg_data mass_ep_data[] = {
	{
		.ep_cb	= mass_storage_bulk_out,
		.ep_addr = CONFIG_MASS_STORAGE_OUT_EP_ADDR
	},
	{
		.ep_cb = mass_storage_bulk_in,
		.ep_addr = CONFIG_MASS_STORAGE_IN_EP_ADDR
	}
};

/* CSW Status */
enum Status {
	CSW_PASSED,
	CSW_FAILED,
	CSW_ERROR,
};

/* MSC Bulk-only Stage */
enum Stage {
	READ_CBW,     /* wait a CBW */
	ERROR,        /* error */
	PROCESS_CBW,  /* process a CBW request */
	SEND_CSW,     /* send a CSW */
	WAIT_CSW      /* wait that a CSW has been effectively sent */
};

/* state of the bulk-only state machine */
static enum Stage stage;

/*current CBW*/
static struct CBW cbw __aligned(4);

/*CSW which will be sent*/
static struct CSW csw;

/*addr where will be read or written data*/
static u64_t addr;

/* max USB packet size */
static u32_t max_packet = MAX_PACKET;

/*length of a reading or writing*/
static u32_t length;

static u8_t max_lun_count;

/*memory OK (after a memoryVerify)*/
static bool memOK;

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
static u8_t *usb_mass_transfer_buf;
static u32_t usb_mass_transfer_len;

static struct k_delayed_work usb_mass_transfer_work;

static inline int usb_mass_transfer(u8_t *data, u32_t len)
{
	usb_mass_transfer_buf = data;
	usb_mass_transfer_len = len;

	k_delayed_work_submit(&usb_mass_transfer_work, K_NO_WAIT);

	return 0;
}

static void usb_mass_transfer_handler(struct k_work *work)
{
	if (thread_op == THREAD_OP_READ_QUEUED) {
		usb_dc_ep_write_pending(CONFIG_MASS_STORAGE_IN_EP_ADDR,
				usb_mass_transfer_buf, usb_mass_transfer_len, NULL);
	} else if (thread_op == THREAD_OP_WRITE_QUEUED) {
		usb_dc_ep_read_pending(CONFIG_MASS_STORAGE_OUT_EP_ADDR,
				usb_mass_transfer_buf, usb_mass_transfer_len, NULL);
	}
}
#endif /* CONFIG_USB_AOTG_DC_MULTI_FIFO */

static inline int usb_mass_write(u8_t *data, u32_t len)
{
#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
	return usb_dc_ep_write_pending(CONFIG_MASS_STORAGE_IN_EP_ADDR, data,
					len, NULL);
#else
	return usb_write(CONFIG_MASS_STORAGE_IN_EP_ADDR, data, len, NULL);
#endif
}

static inline int usb_mass_read(u8_t *data, u32_t len)
{
#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
	return usb_dc_ep_read_pending(CONFIG_MASS_STORAGE_OUT_EP_ADDR, data,
					len, NULL);
#else
	return usb_read_async(CONFIG_MASS_STORAGE_OUT_EP_ADDR, data, len, NULL);
#endif
}

static void msd_state_machine_reset(void)
{
	stage = READ_CBW;
}

static void msd_init(void)
{
	memset((void *)&cbw, 0, sizeof(struct CBW));
	memset((void *)&csw, 0, sizeof(struct CSW));
	addr = 0;
	length = 0;
	defered_wr_sz = 0;
	defered_rd_off = 0;
}

static bool sendCSW(void)
{
	csw.Signature = CSW_Signature;
	stage = WAIT_CSW;
	if (usb_mass_write((u8_t *)&csw, sizeof(struct CSW)) != 0) {
		LOG_ERR("usb write failure");
		return false;
	}

	return true;
}

static bool write(u8_t *buf, u16_t size)
{
	if (size >= cbw.DataLength) {
		size = cbw.DataLength;
	}

	/* updating the State Machine , so that we send CSW when this
	 * transfer is complete, ie when we get a bulk in callback
	 */
	stage = SEND_CSW;
	csw.DataResidue -= size;

	if (usb_mass_write(buf, size)) {
		LOG_ERR("USB write failed");
		return false;
	}

	return true;
}

/**
 * @brief Handler called for Class requests not handled by the USB stack.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int mass_storage_class_handle_req(struct usb_setup_packet *pSetup,
		s32_t *len, u8_t **data)
{
	switch (pSetup->bRequest) {
	case MSC_REQUEST_RESET:
		LOG_DBG("MSC_REQUEST_RESET");

		if (sys_le16_to_cpu(pSetup->wLength)) {
			LOG_WRN("Invalid length");
			return -EINVAL;
		}

		msd_state_machine_reset();
		usb_mass_read((u8_t *)&cbw, sizeof(cbw));
		break;

	case MSC_REQUEST_GET_MAX_LUN:
		LOG_DBG("MSC_REQUEST_GET_MAX_LUN");

		if (sys_le16_to_cpu(pSetup->wLength) != 1) {
			LOG_WRN("Invalid length");
			return -EINVAL;
		}

		max_lun_count = 0;
		*data = (u8_t *)(&max_lun_count);
		*len = 1;
		break;

	default:
		LOG_WRN("Unknown request 0x%x, value 0x%x",
			    pSetup->bRequest, pSetup->wValue);
		return -EINVAL;
	}

	return 0;
}

static void testUnitReady(void)
{
	if (cbw.DataLength != 0) {
		if ((cbw.Flags & 0x80) != 0) {
			LOG_WRN("Stall IN endpoint");
			usb_ep_set_stall(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
		} else {
			LOG_WRN("Stall OUT endpoint");
			usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
		}
	}

	if ((disk_pdrv == MSC_DISK_NO_LUN) ||
	    (disk_pdrv == MSC_DISK_NO_MEDIA)) {
		csw.Status = CSW_FAILED;
	} else {
		csw.Status = CSW_PASSED;
	}
	sendCSW();
}

static bool requestSense(void)
{
	u8_t request_sense[] = {
		0x70,
		0x00,
		0x05,   /* Sense Key: illegal request */
		0x00,
		0x00,
		0x00,
		0x00,
		0x0A,
		0x00,
		0x00,
		0x00,
		0x00,
		0x30,
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
	};

	if (msc_sense_data == SS_NO_SENSE) {
		if (disk_pdrv == MSC_DISK_NO_LUN) {
			msc_sense_data = SS_LOGICAL_UNIT_NOT_SUPPORTED;
		} else if (disk_pdrv == MSC_DISK_NO_MEDIA) {
			msc_sense_data = SS_MEDIUM_NOT_PRESENT;
		}
	}

	if (msc_sense_data) {
		request_sense[2] = SK(msc_sense_data);
		request_sense[12] = ASC(msc_sense_data);
		request_sense[13] = ASCQ(msc_sense_data);
	}

	msc_sense_data = SS_NO_SENSE;

	csw.Status = CSW_PASSED;
	return write(request_sense, sizeof(request_sense));
}

static bool inquiryRequest(void)
{
	/*
	 * inquiry[2]: ANSI SCSI level 2
	 * inquiry[3]: SCSI-2 INQUIRY data format
	 */
	u8_t inquiry[] = { 0x00, 0x80, 0x02, 0x02,
	36 - 4, 0x00, 0x00, 0x00,
	'Z', 'E', 'P', 'H', 'Y', 'R', ' ', ' ',
	'Z', 'E', 'P', 'H', 'Y', 'R', ' ', 'U', 'S', 'B', ' ',
	'D', 'I', 'S', 'K', ' ',
	'0', '.', '0', '1',
	};

	csw.Status = CSW_PASSED;
	return write(inquiry, sizeof(inquiry));
}

static bool modeSense6(void)
{
	u8_t sense6[] = { 0x03, 0x00, 0x00, 0x00 };

#ifdef CONFIG_MASS_STORAGE_WP
	sense6[2] = 0x80;
#endif
	csw.Status = CSW_PASSED;
	return write(sense6, sizeof(sense6));
}

static bool modeSense10(void)
{
	u8_t sense10[] = { 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#ifdef CONFIG_MASS_STORAGE_WP
	sense10[3] = 0x80;
#endif
	csw.Status = CSW_PASSED;
	return write(sense10, sizeof(sense10));
}

static bool readFormatCapacity(void)
{
	u8_t capacity[] = { 0x00, 0x00, 0x00, 0x08,
			(u8_t)((block_count >> 24) & 0xff),
			(u8_t)((block_count >> 16) & 0xff),
			(u8_t)((block_count >> 8) & 0xff),
			(u8_t)((block_count >> 0) & 0xff),

			0x02,
			(u8_t)((BLOCK_SIZE >> 16) & 0xff),
			(u8_t)((BLOCK_SIZE >> 8) & 0xff),
			(u8_t)((BLOCK_SIZE >> 0) & 0xff),
	};

	csw.Status = CSW_PASSED;
	return write(capacity, sizeof(capacity));
}

static bool readCapacity(void)
{
	u8_t capacity[] = {
		(u8_t)(((block_count - 1) >> 24) & 0xff),
		(u8_t)(((block_count - 1) >> 16) & 0xff),
		(u8_t)(((block_count - 1) >> 8) & 0xff),
		(u8_t)(((block_count - 1) >> 0) & 0xff),

		(u8_t)((BLOCK_SIZE >> 24) & 0xff),
		(u8_t)((BLOCK_SIZE >> 16) & 0xff),
		(u8_t)((BLOCK_SIZE >> 8) & 0xff),
		(u8_t)((BLOCK_SIZE >> 0) & 0xff),
	};

	csw.Status = CSW_PASSED;
	return write(capacity, sizeof(capacity));
}

static bool do_cmd_inquiry_adfu(void)
{
#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
	u8_t data[11];

	stage = SEND_CSW;
	csw.DataResidue = 0;
	csw.Status = CSW_PASSED;

	memcpy(data, "ACTIONSUSBD", sizeof(data));

	if (usb_mass_write(data, sizeof(data))) {
		LOG_ERR("USB write failed");
		return false;
	}

	return true;
#else
	stage = SEND_CSW;
	csw.DataResidue = 0;
	csw.Status = CSW_FAILED;

	/* send 0-length packet */
	if (usb_mass_write(NULL, 0)) {
		LOG_ERR("USB write failed");
		return false;
	}

	return true;
#endif
}

#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
static bool do_cmd_switch_to_adfu(void)
{
	u8_t data[2] = { 0xff, 0x55 };

	stage = SEND_CSW;
	switch_to_adfu = 1;
	csw.DataResidue = 0;
	csw.Status = CSW_PASSED;

	/* send 0-length packet */
	if (usb_mass_write(data, sizeof(data))) {
		LOG_ERR("USB write failed");
		return false;
	}

	return true;
}
#endif

static bool do_cmd_adfu_misc(void)
{
	LOG_DBG("0x%x", cbw.CB[1]);

	switch (cbw.CB[1]) {
	case ADFU_MISC_RESTART:
		break;
	case ADFU_MISC_SET_SERIAL_NUMBER:
		break;
	case ADFU_MISC_GET_SERIAL_NUMBER:
		break;
	case ADFU_MISC_SET_MISCINFO:
		break;
	case ADFU_MISC_GET_MISCINFO:
		break;
	case ADFU_MISC_GET_CHIPID:
		break;
	case ADFU_MISC_SWITCH_TO_STUB:
		if (cbw.CB[2] == 0x01) {
			switch_to_stub = SWITCH_TO_STUB_START;

			csw.Status = CSW_PASSED;
			return sendCSW();
		}
		break;
	default:
		break;
	}

	usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
	csw.Status = CSW_ERROR;
	sendCSW();

	return false;
}

#include <drivers/rtc.h>
static void set_time_and_date(void)
{
	u8_t *temp_buff = (u8_t *)&cbw;
	struct rtc_time time = {0};
	const struct device *rtc = NULL;

#ifdef CONFIG_RTC_0_NAME
	rtc = device_get_binding(CONFIG_RTC_0_NAME);
#endif

	time.tm_year = temp_buff[0] | (temp_buff[1]<<8);
	time.tm_mon = temp_buff[2];
	time.tm_mday = temp_buff[3];
	time.tm_hour = temp_buff[4];
	time.tm_min = temp_buff[5];
	time.tm_sec = temp_buff[6];
	time.tm_ms = 500;

	time.tm_year -= 1900; // years since 1900
	time.tm_mon --; // month 0~11

	if (rtc)
		rtc_set_time(rtc, &time);

	csw.Status = CSW_PASSED;
	sendCSW();
	return;
}

static __unused void thread_memory_read_done(void)
{
	u32_t n;

	n = (length > max_packet) ? max_packet : length;
	if ((addr + n) > memory_size) {
		n = memory_size - addr;
		stage = ERROR;
	}

	addr += n;
	length -= n;
	defered_rd_off += n;
	csw.DataResidue -= n;

	if (!length || (stage != PROCESS_CBW)) {
		csw.Status = (stage == PROCESS_CBW) ? CSW_PASSED : CSW_FAILED;
		stage = (stage == PROCESS_CBW) ? SEND_CSW : stage;
	}

	thread_op = THREAD_OP_READ_DONE;

	if (usb_mass_write(page, n) != 0) {
		LOG_ERR("Failed to write EP 0x%x",
			    mass_ep_data[MSD_IN_EP_IDX].ep_addr);
	}
}

static __unused void memoryRead(void)
{
	u32_t n, offset;

	n = (length > max_packet) ? max_packet : length;
	if ((addr + n) > memory_size) {
		n = memory_size - addr;
		stage = ERROR;
	}

	/* read data */
	if ((defered_rd_off == 0) || (defered_rd_off == page_size)) {
		thread_op = THREAD_OP_READ_QUEUED;
		LOG_DBG("Signal thread for %d", (u32_t)(addr/BLOCK_SIZE));
		k_sem_give(&disk_wait_sem);
		return;
	}

	addr += n;
	length -= n;
	csw.DataResidue -= n;
	offset = defered_rd_off;

	if (!length || (stage != PROCESS_CBW)) {
		csw.Status = (stage == PROCESS_CBW) ? CSW_PASSED : CSW_FAILED;
		stage = (stage == PROCESS_CBW) ? SEND_CSW : stage;
	}

	defered_rd_off += n;

	usb_mass_write(&page[offset], n);
}

static bool infoTransfer(void)
{
	u64_t n = 0;

	/* Logical Block Address of First Block */
	n = (cbw.CB[2] << 24) | (cbw.CB[3] << 16) | (cbw.CB[4] <<  8) |
				 (cbw.CB[5] <<  0);

	LOG_DBG("LBA (block) : 0x%x ", (u32_t)n);
	addr = n * BLOCK_SIZE;

	if (addr > memory_size) {
		LOG_WRN("lba: 0x%x", (u32_t)n);
		csw.Status = CSW_FAILED;
		sendCSW();
		return false;
	}

	/* Number of Blocks to transfer */
	switch (cbw.CB[0]) {
	case READ10:
	case WRITE10:
	case VERIFY10:
		n = (cbw.CB[7] <<  8) | (cbw.CB[8] <<  0);
		break;

	case READ12:
	case WRITE12:
		n = (cbw.CB[6] << 24) | (cbw.CB[7] << 16) |
			(cbw.CB[8] <<  8) | (cbw.CB[9] <<  0);
		break;
	}

	LOG_DBG("Size (block) : 0x%x ", (u32_t)n);
	length = n * BLOCK_SIZE;

	if ((addr + length) > memory_size) {
		LOG_WRN("lba: 0x%x, size: 0x%x", (u32_t)addr/BLOCK_SIZE,
			    (u32_t)n);
		csw.Status = CSW_FAILED;
		sendCSW();
		return false;
	}

	if (!cbw.DataLength) {              /* host requests no data*/
		LOG_WRN("Zero length in CBW");
		csw.Status = CSW_FAILED;
		sendCSW();
		return false;
	}

	if (cbw.DataLength != length) {
		if ((cbw.Flags & 0x80) != 0) {
			LOG_WRN("Stall IN endpoint");
			usb_ep_set_stall(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
		} else {
			LOG_WRN("Stall OUT endpoint");
			usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
		}

		csw.Status = CSW_FAILED;
		sendCSW();
		return false;
	}

	return true;
}

static void fail(void)
{
	csw.Status = CSW_FAILED;
	sendCSW();
}

static void CBWDecode(u8_t *buf, u16_t size)
{
	if (cbw.Signature != CBW_Signature) {
		LOG_ERR("CBW Signature Mismatch 0x%x", cbw.Signature);
		return;
	}

	csw.Tag = cbw.Tag;
	csw.DataResidue = cbw.DataLength;

	if ((cbw.CBLength <  1) || (cbw.CBLength > 16) || (cbw.LUN != 0)) {
		LOG_DBG("cbw.CBLength %d", cbw.CBLength);
		fail();
	} else {
		switch (cbw.CB[0]) {
		case TEST_UNIT_READY:
		case REQUEST_SENSE:
		case INQUIRY:
		case CMD_INQUIRY_ADFU:
		case CMD_SWITCH_TO_ADFU:
		case CMD_ADFU_MISC:
		case CMD_SET_TIME_AND_DATE:
#if MSC_WRITE_NO_MEDIUM_CONTINUE
		case WRITE10:
		case WRITE12:
#endif
			break;

		default:
			/* if no lun or no media */
			if ((disk_pdrv == MSC_DISK_NO_LUN) ||
			    (disk_pdrv == MSC_DISK_NO_MEDIA)) {
				if ((cbw.Flags & 0x80) || (cbw.DataLength == 0)) {
					/* Halt bulk-in endpoint */
					usb_ep_set_stall(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
				} else {
					/* Halt bulk-out endpoint */
					usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
					usb_dc_ep_flush(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
				}

				fail();
				return;
			}
			break;
		}

		switch (cbw.CB[0]) {
		case TEST_UNIT_READY:
			LOG_DBG(">> TUR");
			testUnitReady();
			break;
		case REQUEST_SENSE:
			LOG_DBG(">> REQ_SENSE");
			requestSense();
			break;
		case INQUIRY:
			LOG_DBG(">> INQ");
			inquiryRequest();
			break;
		case MODE_SENSE6:
			LOG_DBG(">> MODE_SENSE6");
			modeSense6();
			break;
		case MODE_SENSE10:
			LOG_DBG(">> MODE_SENSE10");
			modeSense10();
			break;
		case READ_FORMAT_CAPACITIES:
			LOG_DBG(">> READ_FORMAT_CAPACITY");
			readFormatCapacity();
			break;
		case READ_CAPACITY:
			LOG_DBG(">> READ_CAPACITY");
			readCapacity();
			break;
		case READ10:
		case READ12:
			LOG_DBG(">> READ");
			if (infoTransfer()) {
				if ((cbw.Flags & 0x80)) {
					stage = PROCESS_CBW;
					defered_rd_off = 0;
					thread_op = THREAD_OP_READ_QUEUED;
					k_sem_init(&usb_wait_sem, 0, 1);
					LOG_DBG("Signal thread for %d",
						    (u32_t)(addr/BLOCK_SIZE));
					k_sem_give(&disk_wait_sem);
				} else {
					usb_ep_set_stall(
					  mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
					LOG_DBG("Stall OUT endpoint");
					csw.Status = CSW_ERROR;
					sendCSW();
				}
			}
			break;
		case WRITE10:
		case WRITE12:
			LOG_DBG(">> WRITE");
			if (infoTransfer()) {
				if (!(cbw.Flags & 0x80)) {
					stage = PROCESS_CBW;
					thread_op = THREAD_OP_WRITE_QUEUED;  /* write_queued */
					k_sem_init(&usb_wait_sem, 0, 1);
					k_sem_give(&disk_wait_sem);
				} else {
					usb_ep_set_stall(
					  mass_ep_data[MSD_IN_EP_IDX].ep_addr);
					LOG_DBG("Stall IN endpoint");
					csw.Status = CSW_ERROR;
					sendCSW();
				}
			}
			break;
#if 0
		case VERIFY10:
			LOG_DBG(">> VERIFY10");
			if (!(cbw.CB[1] & 0x02)) {
				csw.Status = CSW_PASSED;
				sendCSW();
				break;
			}
			if (infoTransfer()) {
				if (!(cbw.Flags & 0x80)) {
					stage = PROCESS_CBW;
					memOK = true;
				} else {
					usb_ep_set_stall(
					  mass_ep_data[MSD_IN_EP_IDX].ep_addr);
					LOG_WRN("Stall IN endpoint");
					csw.Status = CSW_ERROR;
					sendCSW();
				}
			}
			break;
#endif
		case MEDIA_REMOVAL:
			LOG_DBG(">> MEDIA_REMOVAL");
			/*
			 * FIXME: if prevent bit set, we are not allowed to
			 * unload the media.
			 */
			csw.Status = CSW_PASSED;
			sendCSW();
			break;
		case START_STOP_UNIT:
			LOG_DBG(">> START_STOP_UNIT");

			/* respond CSW first */
			csw.Status = CSW_PASSED;
			sendCSW();

			/* LOEJ: load eject bit */
			if (!(cbw.CB[4] & 0x02)) {
				break;
			}

			/* START: start bit */
			if (!(cbw.CB[4] & 0x01)) {
				if (eject_cb) {
					eject_cb();
				}
				msc_state_set_ejected();
				msc_sense_data = SS_MEDIUM_NOT_PRESENT;
			} else {
				if (!msc_state_running()) {
					msc_sense_data = SS_MEDIUM_NOT_PRESENT;
				}
			}
			break;

		case CMD_INQUIRY_ADFU:
			LOG_DBG(">> INQUIRY_ADFU");
			do_cmd_inquiry_adfu();
			break;
#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
		case CMD_SWITCH_TO_ADFU:
			LOG_DBG(">> SWITCH_TO_ADFU");
			do_cmd_switch_to_adfu();
			break;
#endif
		case CMD_ADFU_MISC:
			LOG_DBG(">> ADFU_MISC");
			do_cmd_adfu_misc();
			break;
		case CMD_SET_TIME_AND_DATE:
			LOG_DBG(">> SET_TIME_AND_DATE");
			usb_mass_read((u8_t *)&cbw, 7);
			stage = PROCESS_CBW;
			break;
		default:
			LOG_DBG(">> default CB[0] %x", cbw.CB[0]);
			fail();
			break;
		}		/*switch(cbw.CB[0])*/
	}		/* else */
}

static __unused void memoryVerify(u8_t *buf, u16_t size)
{
	u32_t n;

	if ((addr + size) > memory_size) {
		size = memory_size - addr;
		stage = ERROR;
		usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
		LOG_WRN("Stall OUT endpoint");
	}

	/* beginning of a new block -> load a whole block in RAM */
#ifdef CONFIG_STORAGE_APITEST
    u32_t offset = get_udisk_offset();
	if (!((addr+offset) % BLOCK_SIZE)) {
		LOG_DBG("Flash READ sector %d", (u32_t)((addr+offset)/BLOCK_SIZE));
        if (offset != 0) {
		    if (flash_read(flash_dev, (addr+offset), page, 1<<9)) {
		    	LOG_ERR("---- Flash Read Error %d", (u32_t)((addr+offset)/BLOCK_SIZE));
		    }
        }
	}
#else
	if (!(addr % BLOCK_SIZE)) {
		LOG_DBG("Disk READ sector %d", (u32_t)(addr/BLOCK_SIZE));
		if (disk_read(disk_pdrv, page, addr/BLOCK_SIZE, 1)) {
			LOG_ERR("---- Disk Read Error %d", (u32_t)(addr/BLOCK_SIZE));
		}
	}
#endif

	/* info are in RAM -> no need to re-read memory */
	for (n = 0; n < size; n++) {
		if (page[addr%BLOCK_SIZE + n] != buf[n]) {
			LOG_DBG("Mismatch sector %d offset %d",
				    (u32_t)(addr/BLOCK_SIZE), (u32_t)n);
			memOK = false;
			break;
		}
	}

	addr += size;
	length -= size;
	csw.DataResidue -= size;

	if (!length || (stage != PROCESS_CBW)) {
		csw.Status = (memOK && (stage == PROCESS_CBW)) ?
						CSW_PASSED : CSW_FAILED;
		sendCSW();
	}
}

static __unused void memoryWrite(u8_t *buf, u16_t size)
{
	if ((addr + size) > memory_size) {
		size = memory_size - addr;
		stage = ERROR;
		usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
		LOG_WRN("Stall OUT endpoint");
	}

	memcpy(page + defered_wr_sz, buf, size);

	length -= size;
	defered_wr_sz += size;
	csw.DataResidue -= size;

	/* if the array is filled, write it in memory */
	if ((defered_wr_sz == page_size) || (length == 0)) {
		if (!(disk_status(disk_pdrv) & STA_PROTECT)) {
			LOG_DBG("Disk WRITE Qd %d", (u32_t)(addr/BLOCK_SIZE));
			thread_op = THREAD_OP_WRITE_QUEUED;  /* write_queued */
			k_sem_give(&disk_wait_sem);
			return;
		}
	}

	if ((!length) || (stage != PROCESS_CBW)) {
		csw.Status = (stage == ERROR) ? CSW_FAILED : CSW_PASSED;
		sendCSW();
	}
}


static void mass_storage_bulk_out(u8_t ep,
		enum usb_dc_ep_cb_status_code ep_status)
{
	ARG_UNUSED(ep_status);

	switch (stage) {
	/*the device has to decode the CBW received*/
	case READ_CBW:
		LOG_DBG("> BO - READ_CBW");
		CBWDecode((u8_t *)&cbw, sizeof(cbw));
		break;

	/*the device has to receive data from the host*/
	case PROCESS_CBW:
		switch (cbw.CB[0]) {
		case WRITE10:
		case WRITE12:
			LOG_DBG("> BO - PROC_CBW WR");
			usb_rw_working = 0;
			k_sem_give(&usb_wait_sem);
			break;
		case CMD_SET_TIME_AND_DATE:
			set_time_and_date();
			break;
#if 0
		case VERIFY10:
			LOG_DBG("> BO - PROC_CBW VER");
			memoryVerify(bo_buf, bytes_read);
			break;
#endif
		default:
			LOG_ERR("> BO - PROC_CBW default"
					"<<ERROR!!!>> 0x%x", cbw.CB[0]);
			break;
		}
		break;

	/*an error has occurred: stall endpoint and send CSW*/
	default:
		LOG_WRN("Stall OUT endpoint, stage: %d", stage);
		usb_ep_set_stall(ep);
		csw.Status = CSW_ERROR;
		sendCSW();
		break;
	}
}

static __unused void thread_memory_write_done(void)
{
	addr += defered_wr_sz;
	defered_wr_sz = 0;

	thread_op = THREAD_OP_WRITE_DONE;

	usb_ep_read_continue(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);

	if ((!length) || (stage != PROCESS_CBW)) {
		csw.Status = (stage == ERROR) ? CSW_FAILED : CSW_PASSED;
		sendCSW();
	}
}

#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
static void switch_to_adfu_work_handler(struct k_work *work)
{
	usb_mass_storage_exit();
	sys_pm_reboot(REBOOT_TYPE_GOTO_ADFU);
	LOG_INF("switch to adfu");

	/* If no delay 1s, adfu will be a probability(about 1/10) of failure */
	k_sleep(K_MSEC(1000));
}
#endif

/* flush memory data to storage medium */
static void disk_sync_work_handler(struct k_work *work)
{
	if (disk_ioctl(disk_sync_pdrv, CTRL_SYNC, NULL)) {
		LOG_WRN("MSC disk sync failed");
	}
}

/**
 * @brief EP Bulk IN handler, used to send data to the Host
 *
 * @param ep        Endpoint address.
 * @param ep_status Endpoint status code.
 *
 * @return  N/A.
 */
static void mass_storage_bulk_in(u8_t ep,
				 enum usb_dc_ep_cb_status_code ep_status)
{
	ARG_UNUSED(ep_status);
	ARG_UNUSED(ep);

	switch (stage) {
	/*the device has to send data to the host*/
	case PROCESS_CBW:
		switch (cbw.CB[0]) {
		case READ10:
		case READ12:
			/* LOG_DBG("< BI - PROC_CBW  READ"); */
			usb_rw_working = 0;
			k_sem_give(&usb_wait_sem);
			break;
		default:
			LOG_ERR("< BI-PROC_CBW default <<ERROR!!>> 0x%x",
				cbw.CB[0]);
			break;
		}
		break;

	/*the device has to send a CSW*/
	case SEND_CSW:
		LOG_DBG("< BI - SEND_CSW");
		sendCSW();
		break;

	/*the host has received the CSW -> we wait a CBW*/
	case WAIT_CSW:
		LOG_DBG("< BI - WAIT_CSW");
		stage = READ_CBW;
		usb_mass_read((u8_t *)&cbw, sizeof(cbw));
#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
		if (switch_to_adfu) {
			k_work_submit(&switch_to_adfu_work);
		}
#endif
		if (switch_to_stub == SWITCH_TO_STUB_START) {
			switch_to_stub = SWITCH_TO_STUB_DONE;
		}
		break;

	/*an error has occurred*/
	default:
		LOG_WRN("Stall IN endpoint, stage: %d", stage);
		usb_ep_set_stall(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
		sendCSW();
		break;
	}
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 *
 * @return  N/A.
 */
static void mass_storage_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	ARG_UNUSED(param);

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
		msd_state_machine_reset();
		msd_init();
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		usb_mass_read((u8_t *)&cbw, sizeof(cbw));
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device supended");
		mass_storage_process_suspend();
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_HIGHSPEED:
		/* Update MaxPacketSize */
		max_packet = USB_MAX_HS_BULK_MPS;
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}

/* Configuration of the Mass Storage Device send to the USB Driver */
static const struct usb_cfg_data mass_storage_config = {
	.usb_device_description = NULL,
	.cb_usb_status = mass_storage_status_cb,
	.interface = {
		.class_handler = mass_storage_class_handle_req,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(mass_ep_data),
	.endpoint = mass_ep_data
};

/* retry to make sure CSW succeed */
static int handle_csw(void)
{
	u8_t retry = 3;

	do {
		if (sendCSW()) {
			goto done;
		}
		k_busy_wait(100);
		LOG_WRN("retry");
	} while (--retry);

	/* fail */
	usb_dc_ep_flush(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
	csw.Status = CSW_FAILED;
	LOG_ERR("failed");
	sendCSW();
	return -EIO;
done:
	return 0;
}

void usb_mass_storage_thread(void *p1, void *p2, void *p3)
{
	u32_t sectors, len, len_ping, len_pong, disk_left;
	u8_t *page_disk, *page_usb;
	u8_t skip_disk_write;
	u8_t disk_state;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (msc_state_running()) {
restart:
		k_sem_take(&disk_wait_sem, K_FOREVER);

		if (!msc_state_running()) {
			LOG_ERR("msc_state_NOT_running");
			break;
		}

		LOG_DBG("thread_op %d\n", thread_op);

		switch (thread_op) {
		case THREAD_OP_READ_QUEUED:
			usb_rw_working = 0;
			page_disk = page;

			while (length) {
				if (length > page_size) {
					len = page_size;
				} else {
					len = length;
				}

				if ((addr + len) > memory_size) {
					len = memory_size - addr;
					stage = ERROR;
				}
				sectors = len / BLOCK_SIZE;

				LOG_DBG("read %d sectors", sectors);

#ifdef CONFIG_STORAGE_APITEST
                u32_t offset = get_udisk_offset();
                if (offset != 0) {
		            if (flash_read(flash_dev, (addr+offset), page_disk, sectors<<9)) {
					    if (disk_ioctl(disk_pdrv, DISK_HW_DETECT, &disk_state) ||
					        (disk_state != STA_DISK_OK)) {
					    	disk_pdrv = MSC_DISK_NO_MEDIA;
					    }

					    msc_sense_data = SS_UNRECOVERED_READ_ERROR;
					    LOG_ERR("Flash Read Error 0x%x %d",
					    	    (u32_t)((addr+offset)/BLOCK_SIZE), sectors);
					    break;
		            }
                }
#else
				if (disk_read(disk_pdrv, page_disk, (addr/BLOCK_SIZE), sectors)) {
					if (disk_ioctl(disk_pdrv, DISK_HW_DETECT, &disk_state) ||
					    (disk_state != STA_DISK_OK)) {
						disk_pdrv = MSC_DISK_NO_MEDIA;
					}

					msc_sense_data = SS_UNRECOVERED_READ_ERROR;
					LOG_ERR("Disk Read Error 0x%x %d",
						    (u32_t)(addr/BLOCK_SIZE), sectors);
					break;
				}
#endif

				addr += len;
				length -= len;
				csw.DataResidue -= len;

				while (usb_rw_working) {
					ret = k_sem_take(&usb_wait_sem, USB_RW_TIMEOUT);
					if (ret != 0) {
						LOG_ERR("usb write %d", ret);
						goto restart;
					}
					if (!msc_state_running()) {
						goto exit;
					}
				}

				usb_rw_working = 1;
#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
				if (usb_mass_transfer(page_disk, len) != 0) {
					LOG_ERR("Failed to write EP 0x%x",
							mass_ep_data[MSD_IN_EP_IDX].ep_addr);
				}
#else
				if (usb_write(mass_ep_data[MSD_IN_EP_IDX].ep_addr,
				    page_disk, len, NULL) != 0) {
					LOG_ERR("Failed to write EP 0x%x",
							mass_ep_data[MSD_IN_EP_IDX].ep_addr);
				}
#endif
				if (page_disk == page) {
					page_disk = page2;
				} else {
					page_disk = page;
				}

				if (stage == ERROR) {
					break;
				}
			}

			while (usb_rw_working) {
				ret = k_sem_take(&usb_wait_sem, USB_RW_TIMEOUT);
				if (ret != 0) {
					LOG_ERR("usb write last %d", ret);
					goto restart;
				}
				if (!msc_state_running()) {
					goto exit;
				}
			}

			/* stall unitl USB write done */
			if (msc_sense_data) {
				stage = ERROR;
				usb_ep_set_stall(mass_ep_data[MSD_IN_EP_IDX].ep_addr);
			}

			thread_op = THREAD_OP_READ_DONE;

			csw.Status = (stage == ERROR) ? CSW_FAILED : CSW_PASSED;
			handle_csw();
			break;

		case THREAD_OP_WRITE_QUEUED:
			page_usb = page;
			page_disk = page2;
			disk_left = length;
			skip_disk_write = 0;

			LOG_DBG("length: %d, disk_left: %d",
				length, disk_left);

			if (length > page_size) {
				len_ping = page_size;
			} else {
				len_ping = length;
			}
			length -= len_ping;
			len_pong = 0;	/* for compiler */

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
			usb_mass_transfer(page_usb, len_ping);
#else
			usb_read_async(mass_ep_data[MSD_OUT_EP_IDX].ep_addr,
					page_usb, len_ping, NULL);
#endif
			ret = k_sem_take(&usb_wait_sem, USB_RW_TIMEOUT);
			if (ret != 0) {
				LOG_ERR("usb read first %d", ret);
				goto restart;
			}
			if (!msc_state_running()) {
				goto exit;
			}
			LOG_DBG("usb read ping");

			while (1) {
				while (usb_rw_working) {
					ret = k_sem_take(&usb_wait_sem, USB_RW_TIMEOUT);
					if (ret != 0) {
						LOG_ERR("usb read %d", ret);
						goto restart;
					}
					if (!msc_state_running()) {
						goto exit;
					}
				}

				if (length) {
					usb_rw_working = 1;

					if (length > page_size) {
						len = page_size;
					} else {
						len = length;
					}
					length -= len;

					if (page_usb == page) {
						page_usb = page2;
						len_pong = len;
						LOG_DBG("usb read pong");
					} else {
						page_usb = page;
						len_ping = len;
						LOG_DBG("usb read ping");
					}

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
					usb_mass_transfer(page_usb, len);
#else
					usb_read_async(mass_ep_data[MSD_OUT_EP_IDX].ep_addr,
						page_usb, len, NULL);
#endif
				}

				if (page_disk == page) {
					page_disk = page2;
					len = len_pong;
					LOG_DBG("disk write pong");
				} else {
					page_disk = page;
					len = len_ping;
					LOG_DBG("disk write ping");
				}

				if ((addr + len) > memory_size) {
					len = memory_size - addr;
					stage = ERROR;
					usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
					LOG_WRN("Stall OUT endpoint");
				}
				sectors = len / BLOCK_SIZE;

				LOG_DBG("write %d sectors\n", sectors);

#ifdef CONFIG_STORAGE_APITEST
                u32_t offset = get_udisk_offset();
                if (offset != 0) {
    				if ((skip_disk_write == 0) &&
    				    flash_write(flash_dev, (addr+offset), page_disk, sectors<<9)) {
    					if (disk_ioctl(disk_pdrv, DISK_HW_DETECT, &disk_state) ||
    					    (disk_state != STA_DISK_OK)) {
    						disk_pdrv = MSC_DISK_NO_MEDIA;
    					}

    					msc_sense_data = SS_WRITE_ERROR;
#if DISK_WRITE_ERROR_CONTINUE
					skip_disk_write = 1;
#else
					stage = ERROR;
					usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
#endif
    					LOG_ERR("Flash Write Error 0x%x %d",
    							(u32_t)((addr+offset)/BLOCK_SIZE), sectors);
    				}
                }
#else
				if ((skip_disk_write == 0) &&
				    disk_write(disk_pdrv, page_disk, (addr/BLOCK_SIZE), sectors)) {
					if (disk_ioctl(disk_pdrv, DISK_HW_DETECT, &disk_state) ||
					    (disk_state != STA_DISK_OK)) {
						disk_pdrv = MSC_DISK_NO_MEDIA;
					}

					msc_sense_data = SS_WRITE_ERROR;
#if DISK_WRITE_ERROR_CONTINUE
					skip_disk_write = 1;
#else
					stage = ERROR;
					usb_ep_set_stall(mass_ep_data[MSD_OUT_EP_IDX].ep_addr);
#endif
					LOG_ERR("Disk Write Error 0x%x %d",
							(u32_t)(addr/BLOCK_SIZE), sectors);
				}
#endif
				disk_left -= len;
				addr += len;
				csw.DataResidue -= len;
				LOG_DBG("wrote %d, left %d", len, disk_left);

				if ((!disk_left) || (stage != PROCESS_CBW)) {
#if DISK_WRITE_ERROR_CONTINUE
					if (skip_disk_write) {
						stage = ERROR;
					}
#endif
					thread_op = THREAD_OP_WRITE_DONE;
					LOG_DBG("write done");
					csw.Status = (stage == ERROR) ? CSW_FAILED : CSW_PASSED;
					sendCSW();
					break;
				}
			}
			break;

		default:
			LOG_ERR("thread_op %d", thread_op);
		}
	}

exit:
	msc_state_set_aborted();
}

/**
 * @brief Initialize USB mass storage setup
 *
 * This routine is called to reset the USB device controller chip to a
 * quiescent state. Also it initializes the backing storage and initializes
 * the mass storage protocol state.
 *
 * @param disk_no MSC_DISK_XXX.
 *
 * @return negative errno code on fatal failure, 0 otherwise
 */
static int mass_storage_init(u8_t disk_no)
{
	u32_t block_size = 0;
	u64_t tmp;
	int ret;

	disk_pdrv = disk_no;

	switch_to_stub = 0;
	thread_op = 0;

	msd_state_machine_reset();
	msd_init();

#ifdef CONFIG_STORAGE_APITEST
    if (disk_no == MSC_DISK_NAND) {
	    flash_dev =
	    	device_get_binding("spinand");
	    if (!flash_dev) {
	    	LOG_ERR("spinand flash dev error\n");
	    	goto error;
	    }
    }
#endif

	ret = disk_initialize(disk_no);
	if (ret) {
		LOG_ERR("no lun/media: %d", disk_no);
		goto error;
	}

	ret = disk_ioctl(disk_no, GET_SECTOR_COUNT, &block_count);
	if (ret) {
		LOG_ERR("Unable to get sector count");
		goto error;
	}

	ret = disk_ioctl(disk_no, GET_BLOCK_SIZE, &block_size);
	if (ret) {
		LOG_ERR("Unable to get block size");
		goto error;
	}

	if (block_size != BLOCK_SIZE) {
		LOG_ERR("block_size: %d", block_size);
		goto error;
	}

	tmp = block_count;
	LOG_WRN("Sect Count %d\n", block_count);
	memory_size = tmp * BLOCK_SIZE;
	LOG_WRN("Memory Size 0x%x%x\n", (u32_t)(memory_size >> 32),
		(u32_t)(memory_size & 0xffffffff));

	return 0;
error:
	if (disk_no != MSC_DISK_NO_LUN) {
		disk_pdrv = MSC_DISK_NO_MEDIA;
	}

	return -ENODEV;
}

/*
 * API: whether USB mass storage is working (reading/writing)
 */
bool usb_mass_storage_working(void)
{
	if (!msc_state_running()) {
		return false;
	}

	if ((thread_op == THREAD_OP_READ_QUEUED) ||
	    (thread_op == THREAD_OP_WRITE_QUEUED)) {
		return true;
	}

	return false;
}

/*
 * API: whether USB mass storage is reading
 */
bool usb_mass_storage_reading(void)
{
	if (!msc_state_running()) {
		return false;
	}

	if (thread_op == THREAD_OP_READ_QUEUED) {
		return true;
	}

	return false;
}

/*
 * API: whether USB mass storage is writing
 */
bool usb_mass_storage_writing(void)
{
	if (!msc_state_running()) {
		return false;
	}

	if (thread_op == THREAD_OP_WRITE_QUEUED) {
		return true;
	}
	return false;
}

/*
 * API: add lun/disk for USB mass storage
 */
int usb_mass_storage_add_lun(u8_t disk_no)
{
	/* make sure no lun exist */
	if (disk_pdrv < MSC_DISK_NUM) {
		LOG_ERR("%d exist", disk_pdrv);
		return -EBUSY;
	}

	/* make sure new lun is valid */
	if (disk_no >= MSC_DISK_NUM) {
		LOG_ERR("%d invalid", disk_no);
		return -EINVAL;
	}

	if (disk_pdrv == MSC_DISK_NO_MEDIA) {
		msc_sense_data = SS_NOT_READY_TO_READY_TRANSITION;
	} else if (disk_pdrv == MSC_DISK_NO_LUN) {
		msc_sense_data = SS_NO_SENSE;
	}

	mass_storage_init(disk_no);

	LOG_INF("%d, %d", disk_pdrv, disk_no);

	return 0;
}

/*
 * API: remove lun/disk for USB mass storage
 */
int usb_mass_storage_remove_lun(void)
{
	/* make sure current lun is valid */
	if ((disk_pdrv == MSC_DISK_NO_LUN) ||
	    (disk_pdrv == MSC_DISK_NO_MEDIA)) {
		LOG_WRN("already");
		return -EALREADY;
	}

	disk_pdrv = MSC_DISK_NO_MEDIA;

	LOG_INF("done");

	return 0;
}

/*
 * API: register disk eject callback for USB mass storage
 */
void usb_mass_storage_register_eject_cb(usb_msc_eject_callback cb)
{
	eject_cb = cb;
}

/*
 * API: register disk for USB mass storage
 */
void usb_mass_storage_register(u8_t disk_no)
{
	/* make sure new lun is valid */
	if (disk_no >= MSC_DISK_NUM) {
		LOG_ERR("%d invalid", disk_no);
		disk_pdrv = MSC_DISK_NO_LUN;
	} else {
		disk_pdrv = disk_no;
	}

	LOG_INF("%d", disk_pdrv);
}

/*
 * API: initialize USB mass storage
 */
int usb_mass_storage_init(struct device *dev, u8_t *buf, u32_t len)
{
	int ret;

	ARG_UNUSED(dev);
	if (!buf || !len) {
		LOG_ERR("param is null");
		return ERROR;
	}

	if ((long)buf & 0x3) {
		LOG_ERR("4-byte alignment required!");
		return ERROR;
	}

	if (len % (BLOCK_SIZE * 2)) {
		LOG_ERR("illegal buff len");
		return ERROR;
	}

	page_size = len/2;
	page = buf;
	page2 = buf + page_size;
	if (!msc_state_unknown()) {
		LOG_ERR("state: %d", msc_state);
		return 0;
	}

	msc_sense_data = SS_NO_SENSE;

	/* for example: eject -> exit -> init */
	if (disk_pdrv == MSC_DISK_NO_MEDIA) {
		disk_pdrv = CONFIG_MASS_STORAGE_DISK_PDRV;
	}

	mass_storage_init(disk_pdrv);

	/* Register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC,
			CONFIG_MASS_STORAGE_MANUFACTURER,
			strlen(CONFIG_MASS_STORAGE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC,
			CONFIG_MASS_STORAGE_PRODUCT,
			strlen(CONFIG_MASS_STORAGE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(DEV_SN_DESC,
			CONFIG_MASS_STORAGE_SN,
			strlen(CONFIG_MASS_STORAGE_SN));
	if (ret) {
		return ret;
	}

	/* Register device descriptor */
	usb_device_register_descriptors(mass_storage_fs_descriptor,
					mass_storage_hs_descriptor);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&mass_storage_config);
	if (ret < 0) {
		LOG_ERR("Failed to config USB");
		return ret;
	}

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
	usb_dc_ep_set_multi_fifo(CONFIG_MASS_STORAGE_OUT_EP_ADDR);
	usb_dc_ep_set_multi_fifo(CONFIG_MASS_STORAGE_IN_EP_ADDR);
#endif

	/* Enable USB driver */
	ret = usb_enable(&mass_storage_config);
	if (ret < 0) {
		LOG_ERR("Failed to enable USB");
		return ret;
	}

	k_sem_init(&disk_wait_sem, 0, 1);
	k_sem_init(&usb_wait_sem, 0, 1);

#ifdef CONFIG_MASS_STORAGE_SWITCH_TO_ADFU
	k_work_init(&switch_to_adfu_work, switch_to_adfu_work_handler);
#endif
	k_work_init(&disk_sync_work, disk_sync_work_handler);

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
	k_delayed_work_init(&usb_mass_transfer_work, usb_mass_transfer_handler);
#endif

	msc_state_set_running();

#ifndef CONFIG_USB_MASS_STORAGE_SHARE_THREAD
	/* Start a thread to offload disk ops */
	k_thread_create(&mass_thread_data, mass_thread_stack,
			DISK_THREAD_STACK_SZ,
			usb_mass_storage_thread, NULL, NULL, NULL,
			DISK_THREAD_PRIO, 0, K_NO_WAIT);
#endif
	return 0;
}

/*
 * API: deinitialize USB mass storage synchronously
 */
int usb_mass_storage_exit(void)
{
	int ret;

	if (!msc_state_running()) {
		LOG_ERR("state: %d", msc_state);
		return 0;
	}

	LOG_INF("");

	msc_state_clear_running();

	k_sem_give(&disk_wait_sem);
	k_sem_give(&usb_wait_sem);

#ifdef CONFIG_USB_AOTG_DC_MULTI_FIFO
	usb_dc_ep_io_cancel();
	k_delayed_work_cancel(&usb_mass_transfer_work);
#endif

	/* wait for completion */
	while (!msc_state_aborted()) {
		k_sleep(K_MSEC(10));
	}

	disk_ioctl(disk_pdrv, CTRL_SYNC, NULL);

	eject_cb = NULL;	/* Disk may be changed */
	msc_state_set_unknown();

	page = NULL;
	page2 = NULL;
	ret = usb_disable();
	if (ret) {
		LOG_ERR("Failed to disable USB: %d", ret);
		return ret;
	}
	usb_deconfig();

	LOG_INF("done");

	return 0;
}

/*
 * API: whether USB mass storage is ejected
 */
bool usb_mass_storage_ejected(void)
{
	return msc_state_ejected();
}

/*
 * API: whether USB mass storage is enabled
 */
bool usb_mass_storage_enabled(void)
{
	return msc_state_running();
}

/*
 * API: whether USB mass storage is switched to stub
 */
bool usb_mass_storage_stub_mode(void)
{
	return msc_state_running() && (switch_to_stub == SWITCH_TO_STUB_DONE);
}

/*
 * API: start USB mass storage after initializing
 */
int __weak usb_mass_storage_start(void)
{
	LOG_INF("weak");

	return 0;
}
