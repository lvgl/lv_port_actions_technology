/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#define LOG_LEVEL CONFIG_MMC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mmc_sd);

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <disk/disk_access.h>
#include <drivers/flash.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sd.h>
#include <board.h>
#include "mmc_ops.h"

#if IS_ENABLED(CONFIG_SD)

#define CONFIG_SD_CARD_POWER_RESET_MS    80 /* wait in milliseconds SD card to power off */
#define CONFIG_SD_CARD_HOTPLUG_DEBOUNCE_MS  100 /* SD card hot plug debounce */

#define CONFIG_MMC_SDCARD_RETRY_TIMES    2 /* mmc initialization retry times */
#define CONFIG_MMC_SDCARD_ERR_RETRY_NUM  2 /* mmc read/write retry if error happened */

#define CONFIG_MMC_SDCARD_LOW_POWER      0 /* If 1 to enable SD card low power */
#if (CONFIG_MMC_SDCARD_LOW_POWER == 1)
#define CONFIG_MMC_SDCARD_LOW_POWER_SLEEP   1 /* If 1 to check the mmc device can enter sleep */
#endif

#define CONFIG_MMC_SDCARD_SHOW_PERF      0 /* If 1 to enable sd card performance statistics */

#define MMC_CMD_RETRIES                      (2)

#define SD_CARD_RW_MAX_SECTOR_CNT_PROTOCOL   (65536)
#define SD_CARD_INVALID_OFFSET               (-1)

#define SD_CARD_RW_MAX_SECTOR_CNT            (512)
#define SD_CARD_SECTOR_SIZE                  (512)
#define SD_CARD_INIT_CLOCK_FREQ              (100000)
#define SD_CARD_SDR8_CLOCK_FREQ              (16000000)
#define SD_CARD_SDR16_CLOCK_FREQ             (40000000)

#define SD_CARD_WAITBUSY_TIMEOUT_MS          (1000)

/* card type */
enum {
	CARD_TYPE_SD = 0,
	CARD_TYPE_MMC = 1,
};

struct mmc_csd {
	uint8_t	ccs;
	uint8_t	sd_spec;
	uint8_t	suport_cmd23;
	uint32_t	sector_count;
	uint32_t	sector_size;
};

/* eMMC specific csd and ext_csd all in this structure */
struct mmc_ext_csd {
	/* emmc ext_csd */
	u8_t	rev;

	/* emmc csd */
	u8_t	mmca_vsn;
	u32_t	erase_size;	/* erase size in sectors */
};

struct sd_card_data {
	const struct device *mmc_dev;
	struct k_sem lock;

	u8_t card_type;

	bool card_initialized;
	bool force_plug_out;
	bool is_low_power;

	uint32_t rca;
	struct mmc_csd mmc_csd;
	struct mmc_ext_csd ext_csd;	/* mmc v4 extended card specific */

	struct mmc_cmd cmd; /* use the data segment for reducing stack consumption */

	const struct device *detect_gpio_dev;
	const struct device *power_gpio_dev;

#if (CONFIG_MMC_SDCARD_SHOW_PERF == 1)
	u32_t max_rd_use_time; /* record the max read use time */
	u32_t max_wr_use_time; /* record the max write use time */
	u32_t rec_perf_timestamp; /* record the timestamp for showing performance */
	u32_t rec_perf_rd_size; /* record the read total size for showing performance */
	u32_t rec_perf_wr_size; /* record the read total size for showing performance */
#endif
	u32_t next_rw_offs; /* record the next read/write block offset for high performance mode */
	u8_t on_high_performance : 1; /* indicates that sdcard works on high performance mode */
	u8_t prev_is_write : 1; /* record the previous command is read or write for high performance mode */
};

struct sd_acts_config {
	const char *mmc_dev_name;
	uint8_t detect_gpio;
	uint8_t power_gpio;
	uint8_t use_detect_gpio : 1; /* If 1 to use a GPIO pin to detect SD card host plug */
	uint8_t detect_gpio_level : 1; /* The GPIO level(high/low voltage) when SD card has been detected */
	uint8_t use_power_gpio : 1; /* If 1 to use a GPIO pin to power on/off SD card */
	uint8_t power_gpio_level : 1; /* The GPIO level(high/low voltage) to power on/off SD/eMMC */
};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		uint32_t __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

static int mmc_decode_csd(struct sd_card_data *sd, u32_t *resp)
{
	struct mmc_csd *csd = &sd->mmc_csd;
	u32_t c_size, c_size_mult, capacity;
	int csd_struct;

	csd_struct = UNSTUFF_BITS(resp, 126, 2);

	switch (csd_struct) {
	case 0:
		c_size_mult = UNSTUFF_BITS(resp, 47, 3);
		c_size = UNSTUFF_BITS(resp, 62, 12);

		csd->sector_size = 1 << UNSTUFF_BITS(resp, 80, 4);
		csd->sector_count = (1 + c_size) << (c_size_mult + 2);

		capacity = csd->sector_size * csd->sector_count / 1024 / 1024;
		break;
	case 1:
		/* c_size: 512KB block count */
		c_size = UNSTUFF_BITS(resp, 48, 22);
		csd->sector_size = 512;
		csd->sector_count = (c_size + 1) * 1024;

		capacity = (1 + c_size) / 2;
		break;
	default:
		LOG_ERR("unknown csd version %d", csd_struct);
		return -EINVAL;
	}

	LOG_INF("CSD: capacity %u MB", capacity);

	return 0;
}

static int emmc_decode_csd(struct sd_card_data *sd, u32_t *resp)
{
	struct mmc_csd *csd = &sd->mmc_csd;
	u32_t c_size, c_size_mult, capacity;
	u8_t write_blkbits;
	int csd_struct;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd_struct = UNSTUFF_BITS(resp, 126, 2);
	if (csd_struct == 0) {
		LOG_ERR("unrecognised CSD structure version %d\n", csd_struct);
		return -EINVAL;
	}

	sd->ext_csd.mmca_vsn = UNSTUFF_BITS(resp, 122, 4);

	write_blkbits = UNSTUFF_BITS(resp, 22, 4);
	if (write_blkbits >= 9) {
		u8_t a = UNSTUFF_BITS(resp, 42, 5);
		u8_t b = UNSTUFF_BITS(resp, 37, 5);
		sd->ext_csd.erase_size = (a + 1) * (b + 1);
		sd->ext_csd.erase_size <<= write_blkbits - 9;
	}

	c_size_mult = UNSTUFF_BITS(resp, 47, 3);
	c_size = UNSTUFF_BITS(resp, 62, 12);

	csd->sector_size = 1 << UNSTUFF_BITS(resp, 80, 4);
	csd->sector_count = (1 + c_size) << (c_size_mult + 2);

	capacity = csd->sector_size * csd->sector_count / 1024 / 1024;

	LOG_INF("CSD: capacity %u MB", capacity);

	return 0;
}

/*
 * Decode extended CSD.
 */
static int emmc_read_ext_csd(struct sd_card_data *sd, u8_t *ext_csd)
{
	u32_t sectors = 0;
	u32_t data_sector_size = 512;

	sd->ext_csd.rev = ext_csd[EXT_CSD_REV];
	if (sd->ext_csd.rev > 8) {
		LOG_ERR("unrecognised EXT_CSD revision %d\n", sd->ext_csd.rev);
		return -EINVAL;
	}

	sd->ext_csd.rev = ext_csd[EXT_CSD_REV];

	if (sd->ext_csd.rev >= 3) {
		if (ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 1)
			sd->ext_csd.erase_size = ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] << 10;
	}

	if (sd->ext_csd.rev >= 2) {
		sectors =
				ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
				ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
				ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
				ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
	} else {
		sectors = 0;
	}

	/* eMMC v4.5 or later */
	if (sd->ext_csd.rev >= 6) {
		if (ext_csd[EXT_CSD_DATA_SECTOR_SIZE] == 1)
			data_sector_size = 4096;
		else
			data_sector_size = 512;
	} else {
		data_sector_size = 512;
	}

	LOG_INF("EXT_CSD: rev %d, sector_cnt %u, sector_size %u, erase_size %u",
			sd->ext_csd.rev, sectors, data_sector_size, sd->ext_csd.erase_size);

	/* FIXME: copy to mmc_csd */
	if (sectors > 0) {
		sd->mmc_csd.sector_count = sectors;
		sd->mmc_csd.sector_size = data_sector_size;
	}

	LOG_INF("EXT_CSD: capacity %u MB",
		sd->mmc_csd.sector_count * (sd->mmc_csd.sector_size / 512) / 2048);

	return 0;
}

static void mmc_decode_scr(struct sd_card_data *sd, u32_t *scr)
{
	struct mmc_csd *csd = &sd->mmc_csd;
	u32_t resp[4];

	resp[3] = scr[1];
	resp[2] = scr[0];

	csd->sd_spec = UNSTUFF_BITS(resp, 56, 4);
	if (csd->sd_spec == SCR_SPEC_VER_2) {
		/* Check if Physical Layer Spec v3.0 is supported */
		if (UNSTUFF_BITS(resp, 47, 1)) {
			csd->sd_spec = 3;
		}
	}

	/* check Set Block Count command */
	if(csd->sd_spec == 3)
		csd->suport_cmd23 = !!UNSTUFF_BITS(resp, 33, 1);

	LOG_INF("SCR: sd_spec %d, suport_cmd23 %d, bus_width 0x%x",
			csd->sd_spec, csd->suport_cmd23,
			UNSTUFF_BITS(resp, 48, 4));
}

static int mmc_send_if_cond(const struct device *mmc_dev, struct mmc_cmd *cmd, u32_t ocr)
{
	static const u8_t test_pattern = 0xAA;
	u8_t result_pattern;
	int ret;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_SEND_IF_COND;
	cmd->arg = ((ocr & 0xFF8000) != 0) << 8 | test_pattern;
	cmd->flags = MMC_RSP_R7 | MMC_CMD_BCR;

	ret = mmc_send_cmd(mmc_dev, cmd);

	result_pattern = cmd->resp[0] & 0xFF;

	if (result_pattern != test_pattern)
		return -EIO;

	return 0;
}

static int mmc_send_app_op_cond(const struct device *mmc_dev, struct mmc_cmd *cmd, u32_t ocr, u32_t *rocr)
{
	int i, err = 0;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_APP_OP_COND;
	cmd->arg = ocr;
	cmd->flags = MMC_RSP_R3 | MMC_CMD_BCR;

	for (i = 50; i; i--) {
		err = mmc_send_app_cmd(mmc_dev, 0, cmd, MMC_CMD_RETRIES);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (cmd->resp[0] & MMC_CARD_BUSY)
			break;

		err = -ETIMEDOUT;

		k_sleep(K_MSEC(20));

	}

	if (rocr)
		*rocr = cmd->resp[0];

	return err;
}

static int emmc_send_app_op_cond(const struct device *mmc_dev, struct mmc_cmd *cmd, u32_t ocr, u32_t *rocr)
{
	int i, err = 0;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SEND_OP_COND;
	cmd->arg = ocr;
	cmd->flags = MMC_RSP_R3 | MMC_CMD_BCR;

	/* There are some eMMC need more time to power up */
	for (i = 100; i; i--) {
		err = mmc_send_cmd(mmc_dev, cmd);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (cmd->resp[0] & MMC_CARD_BUSY)
			break;

		err = -ETIMEDOUT;

		k_sleep(K_MSEC(20));
	}

	LOG_DBG("Use %d circulations to power up", i);

	if (rocr)
		*rocr = cmd->resp[0];

	return err;
}

static int mmc_sd_switch(const struct device *mmc_dev, int mode, int group,
	u8_t value, u8_t *resp)
{
	int err;
	struct mmc_cmd cmd = {0};

	mode = !!mode;
	value &= 0xF;

	cmd.opcode = SD_SWITCH;
	cmd.arg = mode << 31 | 0x00FFFFFF;
	cmd.arg &= ~(0xF << (group * 4));
	cmd.arg |= value << (group * 4);
	cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ;
	cmd.blk_size = 64;
	cmd.blk_num = 1;
	cmd.buf = resp;

	err = mmc_send_cmd(mmc_dev, &cmd);
	if (err)
		return err;

	return 0;
}

static int emmc_switch(const struct device *mmc_dev, u8_t set, u8_t index, u8_t value)
{
	struct mmc_cmd cmd = {0};

	cmd.opcode = MMC_SWITCH;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd.arg = (0x03 << 24) |
				 (index << 16) |
				 (value << 8) | set;

	return mmc_send_cmd(mmc_dev, &cmd);
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
int mmc_sd_switch_hs(const struct device *mmc_dev)
{
	u32_t cap;
	int err;
	u8_t status[64];

	cap = mmc_get_capability(mmc_dev);
	if (!(cap & MMC_CAP_SD_HIGHSPEED))
		return 0;

	err = mmc_sd_switch(mmc_dev, 1, 0, 1, status);
	if (err)
		goto out;

	if ((status[16] & 0xF) != 1) {
		LOG_WRN("Failed to switch card to high-speed mode");
		err = 0;
	} else {
		err = 1;
	}
out:
	return err;
}

int emmc_switch_hs(const struct device *mmc_dev)
{
	int err;
	err = emmc_switch(mmc_dev, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
	if (err)
		return 0;

	return 1;
}

static int mmc_app_set_bus_width(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, int width)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_APP_SET_BUS_WIDTH;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;

	switch (width) {
	case MMC_BUS_WIDTH_1:
		cmd->arg = SD_BUS_WIDTH_1;
		break;
	case MMC_BUS_WIDTH_4:
		cmd->arg = SD_BUS_WIDTH_4;
		break;
	default:
		return -EINVAL;
	}

	err = mmc_send_app_cmd(mmc_dev, rca, cmd, MMC_CMD_RETRIES);
	if (err)
		return err;

	return 0;
}

static int mmc_app_send_scr(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, u32_t *scr)
{
	int err;
	u32_t tmp_scr[2];

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_APP_SEND_SCR;
	cmd->arg = 0;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ;
	cmd->blk_size = 8;
	cmd->blk_num = 1;
	cmd->buf = (u8_t *)tmp_scr;

	err = mmc_send_app_cmd(mmc_dev, rca, cmd, MMC_CMD_RETRIES);
	if (err)
		return err;

	scr[0] = sys_be32_to_cpu(tmp_scr[0]);
	scr[1] = sys_be32_to_cpu(tmp_scr[1]);

	return 0;
}

#if (CONFIG_MMC_SDCARD_LOW_POWER == 1)

#if (CONFIG_MMC_SDCARD_LOW_POWER_SLEEP == 1)
static int mmc_can_sleep(struct sd_card_data *sd)
{
	return (sd->card_type == CARD_TYPE_MMC && sd->ext_csd.rev >= 3);
}

static int mmc_sleep_awake(struct sd_card_data *sd, int is_sleep)
{
	int err;
	struct mmc_cmd *cmd = &sd->cmd;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SLEEP_AWAKE;
	cmd->flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd->arg = sd->rca << 16;
	if (is_sleep) {
		cmd.arg |= (1 << 15);
	}

	err = mmc_send_cmd(sd->mmc_dev, cmd);
	if (err) {
		LOG_ERR("sleep/wakeup failed, err %d", err);
		return err;
	}

	return 0;
}
#endif	/* CONFIG_MMC_SDCARD_LOW_POWER_SLEEP */

static int mmc_enter_low_power(struct sd_card_data *sd)
{
	int err;

	sd->is_low_power = true;

	err = mmc_select_card(sd->mmc_dev, &sd->cmd, 0);
	if (err)
		return err;

#if (CONFIG_MMC_SDCARD_LOW_POWER_SLEEP == 1)
	if (!mmc_can_sleep(sd))
		return 0;

	err = mmc_sleep_awake(sd, 1);
#endif
	return err;
}

static int mmc_exit_low_power(struct sd_card_data *sd)
{
	int err;

	if (!sd->is_low_power)
		return 0;

	sd->is_low_power = false;

#if (CONFIG_MMC_SDCARD_LOW_POWER_SLEEP == 1)
	if (mmc_can_sleep(sd)) {
		err = mmc_sleep_awake(sd, 0);
		if (err)
			return err;
	}
#endif

	err = mmc_select_card(sd->mmc_dev, &sd->cmd, sd->rca);

	return err;
}
#endif /* CONFIG_MMC_SDCARD_LOW_POWER */

static int get_card_status(struct sd_card_data *sd, u32_t *status)
{
	int err;

	err = mmc_send_status(sd->mmc_dev, &sd->cmd, sd->rca, status);

	return err;
}

int mmc_sigle_blk_rw(const struct device *dev, int is_write, unsigned int addr,
			 void *dst, int blk_size)
{
	struct sd_card_data *sd = dev->data;
	const struct device *mmc_dev = sd->mmc_dev;
	struct mmc_cmd *cmd = &sd->cmd;
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	/* When transmission changed from multi-block to single block
	 * need to send stop command on high performance mode.
	 */
	if (sd->on_high_performance && (sd->next_rw_offs != SD_CARD_INVALID_OFFSET)) {
		LOG_DBG("%d next_rw_offs %d", __LINE__, sd->next_rw_offs);
		sd->next_rw_offs = SD_CARD_INVALID_OFFSET;
		err = mmc_stop_block_transmission(mmc_dev, cmd);
		if (err) {
			LOG_ERR("mmc stop block transmission failed, ret %d", err);
			return err;
		}
	}

	if (is_write) {
		cmd->opcode = MMC_WRITE_BLOCK;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_WRITE;
	} else {
		cmd->opcode = MMC_READ_SINGLE_BLOCK;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ;
	}

	cmd->arg = addr;
	cmd->blk_size = blk_size;
	cmd->blk_num = 1;
	cmd->buf = dst;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err) {
		LOG_ERR("sigle_blk r/w failed, ret %d", err);
		return err;
	}

	return 0;
}

int mmc_multi_blk_rw(const struct device *dev, int is_write, unsigned int addr,
			 void *dst, int blk_size, int blk_num)
{
	struct sd_card_data *sd = dev->data;
	const struct device *mmc_dev = sd->mmc_dev;
	struct mmc_cmd *cmd = &sd->cmd;
	int err;
	struct mmc_csd *csd = &sd->mmc_csd;

	/* if support cmd23, just need to send cmd23 and then cmd25/cmd18 */
	if (csd->suport_cmd23 && !sd->on_high_performance) {
		err = mmc_set_blockcount(mmc_dev, cmd, blk_num, 0);
		if (err) {
			LOG_ERR("mmc_set_blockcount r/w failed, ret %d", err);
			return err;
		}
	}

	memset(cmd, 0, sizeof(struct mmc_cmd));

	if (is_write) {
		cmd->opcode = MMC_WRITE_MULTIPLE_BLOCK;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_WRITE;
	} else {
		cmd->opcode = MMC_READ_MULTIPLE_BLOCK;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ;
	}

	/* record the next read/wirte offset and at next transmission if hit the read/write offset will not send stop command  */
	if (sd->on_high_performance) {
		LOG_DBG("%d next_rw_offs 0x%x, prev_is_write %d, is_write %d, addr %d",
			__LINE__, sd->next_rw_offs, sd->prev_is_write, is_write, addr);
		if (sd->next_rw_offs == SD_CARD_INVALID_OFFSET) {
			sd->next_rw_offs = (addr + blk_num);
		} else {
			if ((sd->prev_is_write) && (is_write) && (addr == sd->next_rw_offs)) {
				cmd->flags = (MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_WRITE_DIRECT);
				sd->next_rw_offs += blk_num;
				LOG_DBG("%d, next_rw_offs 0x%x, blk_num %d", __LINE__, sd->next_rw_offs, blk_num);
			} else if ((!sd->prev_is_write) && (!is_write) && (addr == sd->next_rw_offs)){
				cmd->flags = (MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ_DIRECT);
				sd->next_rw_offs += blk_num;
				LOG_DBG("%d, next_rw_offs 0x%x, blk_num %d", __LINE__, sd->next_rw_offs, blk_num);
			} else {
				LOG_DBG("%d STOP", __LINE__);
				struct mmc_cmd stop_cmd = {0};
				err = mmc_stop_block_transmission(mmc_dev, &stop_cmd);
				if (err) {
					LOG_ERR("mmc stop block transmission failed, ret %d", err);
					return err;
				}
				sd->next_rw_offs = addr + blk_num;
			}
		}

		if (is_write)
			sd->prev_is_write = true;
		else
			sd->prev_is_write = false;
	}

	cmd->arg = addr;
	cmd->blk_size = blk_size;
	cmd->blk_num = blk_num;
	cmd->buf = dst;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err) {
		LOG_ERR("multi_blk r/w failed, ret %d", err);
		return err;
	}

	/* if not support cmd23, need to send the cmd12 to stop the transmission */
	if (!csd->suport_cmd23 && !sd->on_high_performance) {
		err = mmc_stop_block_transmission(mmc_dev, cmd);
		if (err) {
			LOG_ERR("mmc stop block transmission failed, ret %d", err);
			return err;
		}
	}

	return 0;
}

static int mmc_card_busy_detect(const struct device *dev, u32_t timeout_ms)
{
	struct sd_card_data *sd = dev->data;
	u32_t status = 0;
	u32_t start_time, curr_time;

	start_time = k_cycle_get_32();

	get_card_status(sd, &status);

	while (!(status & R1_READY_FOR_DATA) ||
		R1_CURRENT_STATE(status) == R1_STATE_PRG) {
		LOG_DBG("card busy, status 0x%x", status);
		get_card_status(sd, &status);

		curr_time = k_cycle_get_32();
		if (k_cyc_to_us_floor32(curr_time - start_time)
			>= (timeout_ms * 1000)) {
			LOG_ERR("wait card busy timeout, status 0x%x", status);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

int sd_card_rw(const struct device *dev, int rw, u32_t sector_offset,
		   void *data, size_t sector_cnt)
{
	struct sd_card_data *sd = dev->data;
	struct mmc_csd *csd = &sd->mmc_csd;
	u32_t addr, chunk_sector_cnt, chunk_size;
	int err = 0;
	u8_t i;

#if (CONFIG_MMC_SDCARD_SHOW_PERF == 1)
	u32_t start_time, end_time, delta, cur_xfer_size;
	start_time = k_cycle_get_32();
	cur_xfer_size = sector_cnt * SD_CARD_SECTOR_SIZE;
#endif

	LOG_DBG("rw%d, sector_offset 0x%x, data %p, sector_cnt %zu\n",
			rw, sector_offset, data, sector_cnt);

	if (!sd->card_initialized) {
		return -ENOENT;
	}

	if ((sector_offset + sector_cnt) > csd->sector_count) {
		return -EINVAL;
	}

	k_sem_take(&sd->lock, K_FOREVER);

#if (CONFIG_MMC_SDCARD_LOW_POWER == 1)
	mmc_exit_low_power(sd);
#endif

	addr = csd->ccs ? sector_offset :
			  sector_offset * SD_CARD_SECTOR_SIZE;

	/* The number of blocks at multi block transmision is 65536 in sdmmc protocol */
	if (sd->on_high_performance)
		chunk_sector_cnt = SD_CARD_RW_MAX_SECTOR_CNT_PROTOCOL;
	else
		chunk_sector_cnt = SD_CARD_RW_MAX_SECTOR_CNT;

	while (sector_cnt > 0) {
		if (sector_cnt < chunk_sector_cnt) {
			chunk_sector_cnt = sector_cnt;
		}

		chunk_size = chunk_sector_cnt * SD_CARD_SECTOR_SIZE;

		if (chunk_sector_cnt > 1) {
			for (i = 0; i < (CONFIG_MMC_SDCARD_ERR_RETRY_NUM + 1); i++) {
				err = mmc_multi_blk_rw(dev, rw, addr, data,
					SD_CARD_SECTOR_SIZE, chunk_sector_cnt);
					if (!err)
						break;
			}
		} else {
			for (i = 0; i < (CONFIG_MMC_SDCARD_ERR_RETRY_NUM + 1); i++) {
				err = mmc_sigle_blk_rw(dev, rw, addr, data,
					SD_CARD_SECTOR_SIZE);
					if (!err)
						break;
			}
		}
		/* we need to wait the card program status when write mode */
		if (rw)
			err |= mmc_card_busy_detect(dev, SD_CARD_WAITBUSY_TIMEOUT_MS);

		if (err) {
			LOG_ERR("Failed: rw:%d, addr:0x%x, scnt:0x%x!",
				rw, addr, chunk_sector_cnt);

			/* card error, need reinitialize */
			sd->force_plug_out = true;
			err = -EIO;
			break;
		}

		if (csd->ccs)
			addr += chunk_sector_cnt;
		else
			addr += chunk_size;

		data = (void *)((uint32_t)data + chunk_size);
		sector_cnt -= chunk_sector_cnt;
	}

#if (CONFIG_MMC_SDCARD_LOW_POWER == 1)
	mmc_enter_low_power(sd);
#endif

	k_sem_give(&sd->lock);

#if (CONFIG_MMC_SDCARD_SHOW_PERF == 1)
	end_time = k_cycle_get_32();
	delta = k_cyc_to_us_floor32(end_time - start_time);
	if (rw) {
		sd->rec_perf_wr_size += cur_xfer_size;
		if (delta > sd->max_wr_use_time)
			sd->max_wr_use_time = delta;
	} else {
		sd->rec_perf_rd_size += cur_xfer_size;
		if (delta > sd->max_rd_use_time)
			sd->max_rd_use_time = delta;
	}

	if (k_cyc_to_us_floor32(k_cycle_get_32()
			- sd->rec_perf_timestamp) > 1000000UL) {
		LOG_INF("bandwidth read %dB/s write %dB/s",
					sd->rec_perf_rd_size, sd->rec_perf_wr_size);
		LOG_INF("current rw:%d speed %dB/s",
					rw, cur_xfer_size * 1000000UL / delta);
		LOG_INF("max read time %dus, max write time %dus",
					sd->max_rd_use_time, sd->max_wr_use_time);
		sd->rec_perf_timestamp = k_cycle_get_32();
		sd->rec_perf_rd_size = 0;
		sd->rec_perf_wr_size = 0;
	}
#endif

	return err;
}

static int sd_scan_host(const struct device *dev)
{
	struct sd_card_data *sd = dev->data;
	const struct device *mmc_dev = sd->mmc_dev;
	int err;
	u32_t ocr, rocr;
	u32_t cid_csd[4];
	u32_t scr[16] = {0};
	u32_t caps;

	ocr = 0x40ff8000;

	k_sem_take(&sd->lock, K_FOREVER);

	/* init controller default clock and width */
	mmc_set_clock(mmc_dev, SD_CARD_INIT_CLOCK_FREQ);
	mmc_set_bus_width(mmc_dev, MMC_BUS_WIDTH_1);

	mmc_go_idle(mmc_dev, &sd->cmd);
	mmc_send_if_cond(mmc_dev, &sd->cmd, ocr);

	/* try CARD_TYPE_SD first */
	sd->card_type = CARD_TYPE_SD;
	err = mmc_send_app_op_cond(mmc_dev, &sd->cmd, ocr, &rocr);
	if (err) {
		/* try eMMC card */
		err = emmc_send_app_op_cond(mmc_dev, &sd->cmd, ocr, &rocr);
		if (err)
			goto out;
		sd->card_type = CARD_TYPE_MMC;
	}

	sd->mmc_csd.ccs = (rocr & SD_OCR_CCS)? 1 : 0;

	err = mmc_all_send_cid(mmc_dev, &sd->cmd, cid_csd);
	if (err)
		goto out;

	if (sd->card_type == CARD_TYPE_MMC) {
		sd->rca = 1;
		sd->mmc_csd.suport_cmd23 = 1;

		err = emmc_send_relative_addr(mmc_dev, &sd->cmd, &sd->rca);
		if (err)
			goto out;

		err = mmc_send_csd(mmc_dev, &sd->cmd, sd->rca, cid_csd);
		if (err)
			goto out;

		err = emmc_decode_csd(sd, cid_csd);
		if (err)
			goto out;

		err = mmc_select_card(mmc_dev, &sd->cmd, sd->rca);
		if (err)
			goto out;
	} else {
		err = mmc_send_relative_addr(mmc_dev, &sd->cmd, &sd->rca);
		if (err)
			goto out;

		err = mmc_send_csd(mmc_dev, &sd->cmd, sd->rca, cid_csd);
		if (err)
			goto out;

		err = mmc_decode_csd(sd, cid_csd);
		if (err)
			goto out;

		err = mmc_select_card(mmc_dev, &sd->cmd, sd->rca);
		if (err)
			goto out;

		err = mmc_app_send_scr(mmc_dev, &sd->cmd, sd->rca, scr);
		if (err)
			goto out;

		mmc_decode_scr(sd, scr);
	}

	/* set bus speed */
	if (CARD_TYPE_SD == sd->card_type)
		err = mmc_sd_switch_hs(mmc_dev);
	else
		err = emmc_switch_hs(mmc_dev);
	if (err > 0) {
		mmc_set_clock(mmc_dev, SD_CARD_SDR16_CLOCK_FREQ);
	} else {
		mmc_set_clock(mmc_dev, SD_CARD_SDR8_CLOCK_FREQ);
	}

	/* set bus width */
	caps = mmc_get_capability(mmc_dev);
	if (caps & MMC_CAP_8_BIT_DATA) {
		if (CARD_TYPE_SD == sd->card_type) {
			err = mmc_app_set_bus_width(mmc_dev, &sd->cmd, sd->rca, MMC_BUS_WIDTH_4);
			if (err)
				goto out;
			mmc_set_bus_width(mmc_dev, MMC_BUS_WIDTH_4);
		} else {
			err = emmc_switch(mmc_dev, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_8);
			if (err)
				goto out;
			mmc_set_bus_width(mmc_dev, MMC_BUS_WIDTH_8);
		}
	} else if (caps & MMC_CAP_4_BIT_DATA) {
		if (CARD_TYPE_SD == sd->card_type) {
			err = mmc_app_set_bus_width(mmc_dev, &sd->cmd, sd->rca, MMC_BUS_WIDTH_4);
			if (err)
				goto out;
		} else {
			err = emmc_switch(mmc_dev, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, EXT_CSD_BUS_WIDTH_4);
			if (err)
				goto out;
		}

		mmc_set_bus_width(mmc_dev, MMC_BUS_WIDTH_4);
	}

	/* FIXME: workaround to put ext_csd reading here, since fail just after reading csd at present */
	if (sd->card_type == CARD_TYPE_MMC && sd->ext_csd.mmca_vsn >= CSD_SPEC_VER_4) {
		u8_t ext_csd[512];

		err = mmc_send_ext_csd(mmc_dev, &sd->cmd, ext_csd);
		if (err) {
			goto out;
		}

		err = emmc_read_ext_csd(sd, ext_csd);
		if (err) {
			goto out;
		}
	}

	if (sd->mmc_csd.sector_size != SD_CARD_SECTOR_SIZE) {
		sd->mmc_csd.sector_count = sd->mmc_csd.sector_count *
			(sd->mmc_csd.sector_size / SD_CARD_SECTOR_SIZE);
		sd->mmc_csd.sector_size = SD_CARD_SECTOR_SIZE;
	}

	err = 0;
	sd->card_initialized = true;
	sd->next_rw_offs = SD_CARD_INVALID_OFFSET;
	sd->on_high_performance = false;

#if (CONFIG_MMC_SDCARD_SHOW_PERF == 1)
	sd->max_rd_use_time = 0;
	sd->max_wr_use_time = 0;
	sd->rec_perf_rd_size = 0;
	sd->rec_perf_wr_size = 0;
	sd->rec_perf_timestamp = 0;
#endif

	LOG_INF("sdcard is plugged");

out:
	k_sem_give(&sd->lock);

	return err;
}

#if (CONFIG_SD_USE_GPIO_DET == 1)
static int sd_card_check_card_by_gpio(const struct device *dev, struct sd_card_data *sd)
{
	int value;
	const struct sd_acts_config *cfg = dev->config;

	/* Depends on card detect task for debounce */
	value = gpio_pin_get(sd->detect_gpio_dev, (cfg->detect_gpio % 32));
	if (value < 0)
		return -EIO;

	if (value != cfg->detect_gpio_level) {
		/* card is not detected */
		return false;
	}

	/* card is detected */
	return true;
}

/*
* return
* 0 disk ok
* 1 STA_NOINIT
* 2 STA_NODISK
* other: unknow error
*/
static int sd_card_detect(const struct device *dev)
{
	struct sd_card_data *sd = dev->data;

	if (sd->force_plug_out) {
		LOG_INF("sdcard plug out forcely due to rw error");
		sd->force_plug_out = false;
		sd->card_initialized = false;
		return STA_NODISK;
	}

	if (sd_card_check_card_by_gpio(sd) == true) {
		if (sd->card_initialized)
			return STA_DISK_OK;
		else
			return STA_NOINIT;
	} else {
		mmc_release_device(sd->mmc_dev);
		sd->card_initialized = false;
	}

	return STA_NODISK;
}
#else
static int sd_card_is_unplugged(struct sd_card_data *sd)
{
	const int retry_times = 3;
	int err, i;
	uint32_t status;

	/* assume emmc is always exist */
	if (sd->card_type == CARD_TYPE_MMC) {
		return false;
	}

	k_sem_take(&sd->lock, K_FOREVER);

	for (i = 0; i < retry_times; i++) {
		err = get_card_status(sd, &status);
		status = R1_CURRENT_STATE(status);
		if (!err && (status == R1_STATE_TRAN ||
			(status == R1_STATE_STBY) ||
			(status == R1_STATE_RCV) ||
			(status == R1_STATE_DATA))) {
			break;
		}
	}

	k_sem_give(&sd->lock);

	if (i == retry_times) {
		return true;
	}

	return false;
}

static int sd_card_detect(const struct device *dev)
{
	struct sd_card_data *sd = dev->data;
	int err, ret = STA_NODISK;

	if (sd->force_plug_out) {
		LOG_INF("sdcard plug out forcely due to rw error");
		sd->force_plug_out = false;
		sd->card_initialized = false;
		return STA_NODISK;
	}

	/* check card status */
	if (!sd->card_initialized) {
		/* detect card by send init commands */
		err = sd_card_storage_init(dev);
		if (!err) {
			ret = STA_DISK_OK;
		}
	} else {
		if (sd_card_is_unplugged(sd)) {
			LOG_INF("sdcard is unplugged");
			sd->card_initialized = false;
		} else {
			ret = STA_DISK_OK;
		}
	}

	return ret;
}

#endif /* CONFIG_SD_USE_GPIO_DET == 1 */

#if 0
int sd_card_scan_delay_chain(struct device *dev)
{
	struct sd_card_data *sd = dev->data;
	int err;
	uint32_t status;
	uint8_t rd;

	printk("%s: scan sd card delay chain\n", __func__);

	for (rd = 0; rd < 0x0f; rd++) {
		printk("%s: scan read delay chain: %d\n", __func__, rd);

		sd_card_set_delay_chain(dev, rd, 0xff);

		err = get_card_status(sd, &status);
		status = R1_CURRENT_STATE(status);
		if (err || (status != R1_STATE_TRAN &&
			status != R1_STATE_STBY &&
			status != R1_STATE_RCV)) {
			continue;
		}

		err = sd_card_storage_read(dev, 0, tmp_card_buf, 1);
		if (err) {
			continue;
		}

		printk("%s: scan read delay chain: %d pass\n", __func__, rd);
	}

	return 0;
}
#endif

#if (CONFIG_SD_USE_GPIO_POWER == 1)
static int board_mmc0_pullup_disable(const struct device *dev,
				const struct acts_pin_config *p_pin_config, uint8_t pin_num)
{
	const struct device *sd_gpio_dev = NULL;
	uint8_t i;

	if (!p_pin_config || !pin_num) {
		LOG_ERR("invalid pin config (%p, %d)", p_pin_config, pin_num);
		return -EINVAL;
	}

	for (i = 0; i < pin_num; i++) {
		sd_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(p_pin_config[i].pin_num));
		if (!sd_gpio_dev) {
			LOG_ERR("Failed to bind SD power GPIO(%d:%s)",
					p_pin_config[i].pin_num,
					CONFIG_GPIO_PIN2NAME(p_pin_config[i].pin_num));
			return -ENODEV;
		}
		gpio_pin_configure(sd_gpio_dev, (p_pin_config[i].pin_num % 32), GPIO_OUTPUT);

		LOG_DBG("set sdmmc pin(%d) to gpio", p_pin_config[i].pin_num);

		/* sd pins output low level to avoid leakage */
		gpio_pin_set(sd_gpio_dev, (p_pin_config[i].pin_num % 32), 0);

	}

	return 0;
}

static void board_mmc0_pullup_enable(const struct device *dev,
				const struct acts_pin_config *p_pin_config, uint8_t pin_num)
{
	/* restore origin pullup pinmux config */
	acts_pinmux_setup_pins(p_pin_config, pin_num);
}

static int mmc_power_gpio_reset(const struct device *dev, uint32_t wait_ms)
{
	const struct sd_acts_config *cfg = dev->config;
	struct board_pinmux_info pinmux_info;
#ifndef CONFIG_SD_USE_IOVCC1_POWER
	struct sd_card_data *sd = dev->data;
	int ret;
	uint8_t power_gpio = cfg->power_gpio % 32;
#endif

	board_get_mmc0_pinmux_info(&pinmux_info);

#ifdef CONFIG_SD_USE_IOVCC1_POWER
	/* high-z iovcc1 to power off mmc card */
	if (cfg->use_power_gpio)
		sys_write32(0x1000, GPION_CTL(cfg->power_gpio));
#else
	sd->power_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(cfg->power_gpio));
	if (!sd->power_gpio_dev) {
		LOG_ERR("Failed to bind SD power GPIO(%d:%s)",
				cfg->power_gpio, CONFIG_GPIO_PIN2NAME(cfg->power_gpio));
		return -ENODEV;
	}

	ret = gpio_pin_configure(sd->power_gpio_dev, power_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Failed to config output GPIO:%d", power_gpio);
		return ret;
	}

	/* power off mmc card */
	gpio_pin_set(sd->power_gpio_dev, power_gpio, !cfg->power_gpio_level);
#endif

	/* disable mmc0 pull-up to avoid leakage */
	board_mmc0_pullup_disable(dev, pinmux_info.pins_config, pinmux_info.pins_num);

	k_sleep(K_MSEC(wait_ms));

#ifdef CONFIG_SD_USE_IOVCC1_POWER
	/* iovcc1 output to power on mmc card */
	if (cfg->use_power_gpio)
		sys_write32(0x1f, GPION_CTL(cfg->power_gpio));
#else
	/* power on mmc card */
	gpio_pin_set(sd->power_gpio_dev, power_gpio, cfg->power_gpio_level);
#endif

	k_sleep(K_MSEC(10));

	/* restore mmc0 pull-up */
	board_mmc0_pullup_enable(dev, pinmux_info.pins_config, pinmux_info.pins_num);

	return 0;
}
#endif

static int mmc_power_reset(const struct device *dev, uint32_t wait_ms)
{
#if (CONFIG_SD_USE_GPIO_POWER == 1)
	const struct sd_acts_config *cfg = dev->config;
	struct sd_card_data *sd = dev->data;

	mmc_power_gpio_reset(dev, wait_ms);

	if (cfg->use_detect_gpio
		&& (cfg->detect_gpio == cfg->use_power_gpio)) {
		gpio_pin_configure(sd->detect_gpio_dev,
							 (cfg->detect_gpio % 32), GPIO_INPUT);
	}
#endif
	return 0;
}

static int sd_card_power_reset(const struct device *dev, uint32_t wait_ms)
{
	return mmc_power_reset(dev, wait_ms);
}

int sd_card_storage_init(const struct device *dev)
{
	struct sd_card_data *sd = dev->data;
	int ret;
	u8_t i, cnt;

	if (!sd->mmc_dev)
		return -ENODEV;

	/* In case of without detect pin will depend on the hotplug thread to initialize */
	cnt = CONFIG_MMC_SDCARD_RETRY_TIMES;

	k_sem_take(&sd->lock, K_FOREVER);
	if (sd->card_initialized) {
		LOG_DBG("SD card has already initialized!");
		k_sem_give(&sd->lock);
		return 0;
	}
	k_sem_give(&sd->lock);

	for (i = 0; i < cnt; i++) {
		sd_card_power_reset(dev, CONFIG_SD_CARD_POWER_RESET_MS * cnt);
		ret = sd_scan_host(dev);
		if (!ret)
			break;
		else
			LOG_DBG("SD card storage init failed");

#if (CONFIG_SD_USE_GPIO_DET == 1)
		if (sd_card_check_card_by_gpio(dev, sd) == false) {
			/* sd cards host plug debounce */
			k_sleep(K_MSEC(CONFIG_SD_CARD_HOTPLUG_DEBOUNCE_MS));
			if (sd_card_check_card_by_gpio(dev, sd) == false) {
				LOG_ERR("SD card is not detected");
				break;
			}
		}
#endif
	}

	if (!ret) {
		LOG_INF("SD card storage initialized!\n");
	} else {
		ret = -ENODEV;
	}
	return ret;
}


static int sd_card_storage_read(const struct device *dev, off_t offset, void *data,
			   size_t len)
{
	offset = offset >> 9;
	len = len >> 9;
	return sd_card_rw((struct device *)dev, 0, offset, data, len);
}

static int sd_card_storage_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	offset = offset >> 9;
	len = len >> 9;
	return sd_card_rw((struct device *)dev, 1, offset, (void *)data, len);
}

static int sd_card_enter_high_speed(const struct device *dev)
{
	struct sd_card_data *sd = dev->data;
	const struct sd_acts_config *cfg = dev->config;

	if (!cfg->use_detect_gpio) {
		LOG_ERR("high speed only support with detect pin");
		return -ENOTSUP;
	}

	k_sem_take(&sd->lock, K_FOREVER);
	if (sd->on_high_performance) {
		LOG_DBG("already enter high speed mode");
		goto out;
	}

	sd->on_high_performance = true;
out:
	k_sem_give(&sd->lock);

	LOG_INF("enter high speed mode");

	return 0;
}

static int sd_card_exit_high_speed(const struct device *dev)
{
	int ret;
	u32_t status;
	struct sd_card_data *sd = dev->data;
	const struct sd_acts_config *cfg = dev->config;

	if (!cfg->use_detect_gpio)
		return 0;

	k_sem_take(&sd->lock, K_FOREVER);
	if (!sd->on_high_performance) {
		LOG_DBG("already exit high speed mode");
		ret = 0;
		goto out;
	}

	ret = mmc_send_status(sd->mmc_dev, &sd->cmd, sd->rca, &status);
	if (!ret) {
		status = R1_CURRENT_STATE(status);
		if ((status == R1_STATE_DATA) || (status == R1_STATE_RCV)) {
			LOG_INF("status:0x%x send stop command", status);
			mmc_stop_block_transmission(sd->mmc_dev, &sd->cmd);
		}
	}
	sd->on_high_performance = false;
	sd->next_rw_offs = SD_CARD_INVALID_OFFSET;
	LOG_INF("sd->next_rw_offs %d", sd->next_rw_offs);

out:
	k_sem_give(&sd->lock);

	LOG_INF("exit high speed mode");

	return ret;
}

int sd_card_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff)
{
	struct sd_card_data *sd = dev->data;
	int ret;

	if (!sd->card_initialized && (cmd != DISK_IOCTL_HW_DETECT)) {
		return -ENOENT;
	}

	switch (cmd) {
	case DISK_IOCTL_CTRL_SYNC:
		break;
	case DISK_IOCTL_GET_SECTOR_COUNT:
		*(uint32_t *)buff = sd->mmc_csd.sector_count;
		break;
	case DISK_IOCTL_GET_SECTOR_SIZE:
		*(uint32_t *)buff = sd->mmc_csd.sector_size;
		break;
	case DISK_IOCTL_GET_ERASE_BLOCK_SZ:
		//*(uint32_t *)buff  = (sd->mmc_csd.ccs) ? 1 : SD_CARD_SECTOR_SIZE;
		*(uint32_t *)buff  = SD_CARD_SECTOR_SIZE;
		break;
#if 0
	case DISK_IOCTL_GET_DISK_SIZE:
		/* > 4GByte??, add 2GByte replase */
		*(uint32_t *)buff  = (sd->mmc_csd.ccs)? 0x80000000 :
			(sd->mmc_csd.sector_size * sd->mmc_csd.sector_count);
#endif
		break;

	case DISK_IOCTL_HW_DETECT:
		ret = sd_card_detect(dev);
		if (STA_NOINIT == ret || STA_DISK_OK == ret) {
			*(uint8_t *)buff = STA_DISK_OK;
		} else {
			*(uint8_t *)buff = STA_NODISK;
		}

		break;

	case DISK_IOCTL_ENTER_HIGH_SPEED:
		ret = sd_card_enter_high_speed(dev);
		if (ret)
			return ret;
		break;

	case DISK_IOCTL_EXIT_HIGH_SPEED:
		ret = sd_card_exit_high_speed(dev);
		if (ret)
			return ret;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static const struct flash_driver_api sd_card_storage_api = {
	.read = sd_card_storage_read,
	.write = sd_card_storage_write,
};

static int sd_card_init(const struct device *dev)
{
	const struct sd_acts_config *cfg = dev->config;
	struct sd_card_data *sd = dev->data;
	int ret = 0;

	LOG_INF("sd_card_init");

	sd->mmc_dev = (struct device *)device_get_binding(cfg->mmc_dev_name);
	if (!sd->mmc_dev) {
		LOG_ERR("Cannot find mmc device %s!\n", cfg->mmc_dev_name);
		return -ENODEV;
	}

	if (cfg->use_detect_gpio) {
		sd->detect_gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(cfg->detect_gpio));
		if (!sd->detect_gpio_dev) {
			LOG_ERR("Failed to bind SD detect GPIO(%d:%s)",
					cfg->detect_gpio, CONFIG_GPIO_PIN2NAME(cfg->detect_gpio));
			return -ENODEV;
		}

		/* switch gpio function to input for detecting card plugin */
		ret = gpio_pin_configure(sd->detect_gpio_dev,
					 (cfg->detect_gpio % 32), GPIO_INPUT);
		if (ret)
			return ret;
	}

	k_sem_init(&sd->lock, 1, 1);
	sd->is_low_power = false;
	sd->card_initialized = false;
	sd->force_plug_out = false;

	/* assume that the type of mmc medium is SD card */
	sd->card_type = CARD_TYPE_SD;

#ifdef CONFIG_BOARD_EMMCBOOT
	sd_card_storage_init(dev);
#endif

	return ret;
}


#define  gpio_det_use(n)  	(\
		.detect_gpio = CONFIG_SD_GPIO_DET_NUM,\
		.detect_gpio_level = CONFIG_SD_GPIO_DET_LEVEL,\
		.use_detect_gpio = 1,					\
		)

#define gpio_det_not(n)	(\
		.use_detect_gpio = 0, \
		 )

#define  gpio_power_use(n)  	(\
		.power_gpio = CONFIG_SD_GPIO_POWER_NUM,\
		.power_gpio_level = CONFIG_SD_GPIO_POWER_LEVEL,\
		.use_power_gpio = 1,					\
		)

#define gpio_power_not(n)	(\
		.use_power_gpio = 0, \
		 )

static const struct sd_acts_config sd_acts_config_0 = {
	.mmc_dev_name = CONFIG_SD_MMC_DEV,
	COND_CODE_1(CONFIG_SD_USE_GPIO_DET, gpio_det_use(0), gpio_det_not(0))
	COND_CODE_1(CONFIG_SD_USE_GPIO_POWER, gpio_power_use(0), gpio_power_not(0))
};

struct sd_card_data sdcard_acts_data_0;
DEVICE_DEFINE(sd_storage_0, CONFIG_SD_NAME, sd_card_init,
		NULL,
		&sdcard_acts_data_0, &sd_acts_config_0,
		POST_KERNEL, 35,
		&sd_card_storage_api);

#else  //#if IS_ENABLED(CONFIG_SD)

int sd_card_storage_init(const struct device *dev)
{
	return -ENODEV;
}
int sd_card_storage_ioctl(const struct device *dev, uint8_t cmd, void *buff)
{
	return -ENODEV;
}

#endif //END #if IS_ENABLED(CONFIG_SD)

