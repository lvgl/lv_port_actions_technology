#ifndef __SD__H__
#define __SD__H__



/* Disk Status Bits (DSTATUS) */

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */

/* In order to support medium hotplug */
#define STA_DISK_OK		0x08	/* Medium OK in the drive */

/* Detect disk hardware insert or remove */
#define DISK_IOCTL_HW_DETECT			10


/* SD commands                           type  argument     response */
  /* class 0 */
/* This is basically the same command as for MMC with some quirks. */
#define SD_SEND_RELATIVE_ADDR     3   /* bcr                     R6  */
#define SD_SEND_IF_COND           8   /* bcr  [11:0] See below   R7  */
#define SD_SWITCH_VOLTAGE         11  /* ac                      R1  */

  /* class 10 */
#define SD_SWITCH                 6   /* adtc [31:0] See below   R1  */

  /* class 5 */
#define SD_ERASE_WR_BLK_START    32   /* ac   [31:0] data addr   R1  */
#define SD_ERASE_WR_BLK_END      33   /* ac   [31:0] data addr   R1  */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */
#define SD_APP_SD_STATUS         13   /* adtc                    R1  */
#define SD_APP_SEND_NUM_WR_BLKS  22   /* adtc                    R1  */
#define SD_APP_OP_COND           41   /* bcr  [31:0] OCR         R3  */
#define SD_APP_SEND_SCR          51   /* adtc                    R1  */

/* OCR bit definitions */
#define SD_OCR_S18R		(1 << 24)    /* 1.8V switching request */
#define SD_ROCR_S18A		SD_OCR_S18R  /* 1.8V switching accepted by card */
#define SD_OCR_XPC		(1 << 28)    /* SDXC power control */
#define SD_OCR_CCS		(1 << 30)    /* Card Capacity Status */

  /* Application commands */
#define SD_APP_SET_BUS_WIDTH      6   /* ac   [1:0] bus width    R1  */

/*
 * SCR field definitions
 */

#define SCR_SPEC_VER_0		0	/* Implements system specification 1.0 - 1.01 */
#define SCR_SPEC_VER_1		1	/* Implements system specification 1.10 */
#define SCR_SPEC_VER_2		2	/* Implements system specification 2.00-3.0X */

/*
 * SD bus widths
 */
#define SD_BUS_WIDTH_1		0
#define SD_BUS_WIDTH_4		2

/*
 * SD_SWITCH mode
 */
#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SET		1

/*
 * SD_SWITCH function groups
 */
#define SD_SWITCH_GRP_ACCESS	0

/*
 * SD_SWITCH access modes
 */
#define SD_SWITCH_ACCESS_DEF	0
#define SD_SWITCH_ACCESS_HS	1
 
/* OCR bit definitions */
#define SD_OCR_S18R		(1 << 24)    /* 1.8V switching request */
#define SD_ROCR_S18A		SD_OCR_S18R  /* 1.8V switching accepted by card */
#define SD_OCR_XPC		(1 << 28)    /* SDXC power control */
#define SD_OCR_CCS		(1 << 30)    /* Card Capacity Status */



/* Detect disk hardware insert or remove */
#define DISK_IOCTL_HW_DETECT			10


int sd_card_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff);
int sd_card_storage_init(const struct device *dev);



#endif /* __SD__H__ */
