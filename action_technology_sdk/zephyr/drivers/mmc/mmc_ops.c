/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sd.h>
#include "mmc_ops.h"

int mmc_select_card(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SELECT_CARD;

	if (rca) {
		cmd->arg = rca << 16;
		cmd->flags = MMC_RSP_R1B | MMC_CMD_AC;
	} else {
		cmd->arg = 0;
		cmd->flags = MMC_RSP_NONE | MMC_CMD_AC;
	}

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	return 0;
}

int mmc_go_idle(const struct device *mmc_dev, struct mmc_cmd *cmd)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_GO_IDLE_STATE;
	cmd->arg = 0;
	cmd->flags = MMC_RSP_NONE | MMC_CMD_BC;

	err = mmc_send_cmd(mmc_dev, cmd);

	k_sleep(K_MSEC(1));

	return err;
}

int mmc_send_status(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, u32_t *status)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SEND_STATUS;
	cmd->arg = rca << 16;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	if (status)
		*status = cmd->resp[0];

	return 0;
}

int mmc_app_cmd(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_APP_CMD;

	if (rca) {
		cmd->arg = rca << 16;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		cmd->arg = 0;
		cmd->flags = MMC_RSP_R1 | MMC_CMD_BCR;
	}

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	/* Check that card supported application commands */
	if (!(cmd->resp[0] & R1_APP_CMD))
		return -EOPNOTSUPP;

	return 0;
}

int mmc_send_app_cmd(const struct device *mmc_dev, int rca, struct mmc_cmd *cmd,
		     int retries)
{
	int i, err = 0;
	struct mmc_cmd app_cmd;

	for (i = 0; i <= retries; i++) {
		err = mmc_app_cmd(mmc_dev, &app_cmd, rca);
		if (err)
			continue;

		err = mmc_send_cmd(mmc_dev, cmd);
		if (!err)
			break;
	}

	return err;
}

int mmc_all_send_cid(const struct device *mmc_dev, struct mmc_cmd *cmd, u32_t *cid)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_ALL_SEND_CID;
	cmd->arg = 0;
	cmd->flags = MMC_RSP_R2 | MMC_CMD_BCR;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	memcpy(cid, cmd->resp, sizeof(u32_t) * 4);

	return 0;
}

int mmc_send_csd(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, u32_t *csd)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SEND_CSD;
	cmd->arg = rca << 16;
	cmd->flags = MMC_RSP_R2 | MMC_CMD_AC;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	memcpy(csd, cmd->resp, sizeof(u32_t) * 4);

	return 0;
}

int mmc_send_ext_csd(const struct device *mmc_dev, struct mmc_cmd *cmd, u8_t *ext_csd)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SEND_EXT_CSD;
	cmd->arg = 0;
	cmd->flags =  MMC_RSP_R1 | MMC_CMD_ADTC | MMC_DATA_READ;
	cmd->blk_size = 512;
	cmd->blk_num = 1;
	cmd->buf = ext_csd;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	return 0;
}

int mmc_set_blockcount(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int blockcount,
		       bool is_rel_write)
{
	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_SET_BLOCK_COUNT;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;
	cmd->arg = blockcount & 0x0000FFFF;
	if (is_rel_write)
		cmd->arg |= 1 << 31;

	return mmc_send_cmd(mmc_dev, cmd);
}

int mmc_stop_block_transmission(const struct device *mmc_dev, struct mmc_cmd *cmd)
{
	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = MMC_STOP_TRANSMISSION;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;
	cmd->arg = 0;
	return mmc_send_cmd(mmc_dev, cmd);
}

int mmc_send_relative_addr(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int *rca)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_SEND_RELATIVE_ADDR;
	cmd->arg = 0;
	cmd->flags = MMC_RSP_R6 | MMC_CMD_BCR;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	*rca = cmd->resp[0] >> 16;

	return 0;
}

int emmc_send_relative_addr(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int *rca)
{
	int err;

	memset(cmd, 0, sizeof(struct mmc_cmd));

	cmd->opcode = SD_SEND_RELATIVE_ADDR;
	cmd->arg = *rca << 16;
	cmd->flags = MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_send_cmd(mmc_dev, cmd);
	if (err)
		return err;

	return 0;
}

