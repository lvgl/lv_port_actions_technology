#ifndef __MMC_OPS_H__
#define __MMC_OPS_H__

int mmc_select_card(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca);
int mmc_go_idle(const struct device *mmc_dev, struct mmc_cmd *cmd);
int mmc_send_status(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, u32_t *status);
int mmc_app_cmd(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca);
int mmc_send_app_cmd(const struct device *mmc_dev, int rca, struct mmc_cmd *cmd,
		     int retries);
int mmc_all_send_cid(const struct device *mmc_dev, struct mmc_cmd *cmd, u32_t *cid);
int mmc_send_csd(const struct device *mmc_dev, struct mmc_cmd *cmd, int rca, u32_t *csd);
int mmc_send_ext_csd(const struct device *mmc_dev, struct mmc_cmd *cmd, u8_t *ext_csd);
int mmc_set_blockcount(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int blockcount,
		       bool is_rel_write);
int mmc_stop_block_transmission(const struct device *mmc_dev, struct mmc_cmd *cmd);
int mmc_send_relative_addr(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int *rca);
int emmc_send_relative_addr(const struct device *mmc_dev, struct mmc_cmd *cmd, unsigned int *rca);


#endif	/* __MMC_OPS_H__ */
