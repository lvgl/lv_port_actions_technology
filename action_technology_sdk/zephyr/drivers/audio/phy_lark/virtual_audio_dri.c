/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief virtual audio implementation
 */

/*
 * Features
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include <drivers/audio/audio_in.h>
#include <fs/fs.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(virtual_audio, CONFIG_LOG_DEFAULT_LEVEL);

#define FIEE_PATH "/NAND:/fs_rw_test_file"
struct fs_file_t fs_rw_test_file;

static int fs_rw_test()
{
    char read_data[8];
    int i;
    int err;
    off_t offset = -1;
	fs_file_t_init(&fs_rw_test_file);
	err = fs_open(&fs_rw_test_file, FIEE_PATH, FS_O_CREATE | FS_O_RDWR);
	if (err) {
	LOG_ERR("Failed to open %s (%d)", FIEE_PATH, err);
		return -ENOEXEC;
	}

	if (offset < 0) {
		err = fs_seek(&fs_rw_test_file, 0, FS_SEEK_END);
	} else {
		err = fs_seek(&fs_rw_test_file, offset, FS_SEEK_SET);
	}
	if (err) {
		LOG_ERR("Failed to seek %s (%d)", FIEE_PATH, err);
		fs_close(&fs_rw_test_file);
		return -ENOEXEC;
	}
    char fs_rw_test_data[] = "this test write fs";
    for(i=0;i<8;i++)
    {
        err=fs_write(&fs_rw_test_file, fs_rw_test_data, 8);
        LOG_ERR("fs_write:%0X",err);
    }
    fs_close(&fs_rw_test_file);
    fs_open(&fs_rw_test_file, FIEE_PATH, FS_O_CREATE | FS_O_RDWR);
    err=fs_read(&fs_rw_test_file,read_data,6);
    LOG_ERR("fs_read:%0X",err);
    for(i=0;i<8;i++)
        LOG_ERR("read_data:%c",read_data[i]);
    fs_close(&fs_rw_test_file);
    return 0;
}

/**
 * struct phy_dac_drv_data
 * @brief The software related data that used by physical dac driver.
 */
struct virtual_audio_drv_data {
    uint32_t sdm_cnt; /* SDM samples counter */
    uint32_t sdm_cnt_timestamp; /* Record the timestamp of SDM counter by overflow irq */
    uint8_t sample_rate; /* The sample rate setting refer to enum audio_sr_sel_e */
    uint8_t lr_sel; /* left and right channel selection to enable, refer to enum a_lr_chl_e */
    uint8_t layout; /* DAC hardware layout */

#ifdef CONFIG_CFG_DRV
    struct phy_dac_external_config external_config; /* DAC external configuration */
#endif

    atomic_t refcount; /* DAC resources reference counter */

    uint8_t ch_fifo0_start : 1; /* The fifo0 channel start indicator */
    uint8_t ch_fifo1_start : 1; /* The fifo1 channel start indicator */
    uint8_t vol_set_mute : 1; /* The flag of the volume setting less than #VOL_MUTE_MIN_DB event*/
    uint8_t is_anc_enable : 1; /* If 1 to indicate that ANC has enabled */
	uint8_t audio_pll_index : 1; /* The index of audio pll */
};

struct device_info {
    struct k_work work;
    char name[16];
	uint8_t *buf;
	int buf_size;
} user_device;

char device_data[]="this is data";

static void work_run_fun(struct k_work *item)
{
	struct device_info *the_device =
	CONTAINER_OF(item, struct device_info, work);
	LOG_ERR("work_run_fun;%s,th_device->size:%d", the_device->name,the_device->buf_size);
	LOG_ERR("the_device->buf:%s",the_device->buf);
}

/**
 * struct phy_dac_config_data
 * @brief The hardware related data that used by physical dac driver.
 */
struct virtual_audio_config_data {
    uint32_t reg_base; /* DAC controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
    struct audio_dma_dt dma_fifo1; /* DMA resource for FIFO1 */
    uint8_t clk_id; /* DAC devclk id */
    uint8_t rst_id; /* DAC reset id */
    void (*irq_config)(void); /* IRQ configuration function */
};

static void virtual_audio_irq_config(void)
{
    // /* Connect and enable DAC digital IRQ */
    // IRQ_CONNECT(IRQ_ID_DAC, CONFIG_AUDIO_DAC_0_IRQ_PRI,
    //         phy_dac_digital_isr,
    //         DEVICE_GET(dac0), 0);
    // irq_enable(IRQ_ID_DAC);

    // /* Connect and enable DAC FIFO IRQ */
    // IRQ_CONNECT(IRQ_ID_DACFIFO, CONFIG_AUDIO_DAC_0_IRQ_PRI,
    //         phy_dac_fifo_isr,
    //         DEVICE_GET(dac0), 0);
    // irq_enable(IRQ_ID_DACFIFO);
}

/* virtual_audio driver data */
static struct virtual_audio_drv_data virtual_audio_drv_data0;

/* physical dac config data */
static const struct virtual_audio_config_data virtual_audio_config_data0 = {
    .reg_base = AUDIO_DAC_REG_BASE,
    AUDIO_DMA_FIFO_DEF(DAC, 0),
    AUDIO_DMA_FIFO_DEF(DAC, 1),
    .clk_id = 1,
    .rst_id = 1,
    .irq_config = virtual_audio_irq_config,
};

static int virtual_audio_enable(struct device *dev, void *param)
{
    printk("virtual_audio_enable\n");
    /* tese work only */
    k_work_submit(&user_device.work);
    return 0;    
}

static int virtual_audio_disable(struct device *dev, void *param)
{
    printk("virtual_audio_disable\n");
    return 0;    
}

static int virtual_audioioctl(struct device *dev, uint32_t cmd, void *param)
{
    printk("virtual_audioioctl,cmd:%d,param:%p\n",cmd,param);
    return 0;    
}

const struct virtual_audio_drv_api virtual_audio_drv_api = {
    .virtual_audio_enable = virtual_audio_enable,
    .virtual_audio_disable = virtual_audio_disable,
    .virtual_audio_ioctl = virtual_audioioctl
};

/* virtual_audio_out_init  initialization */
static int virtual_audio_out_init(const struct device *dev)
{
    virtual_audio_drv_data0.audio_pll_index=1;
    printk("virtual_audio_out init successfully\n");
    /* initialize work for tese work only */
	strcpy(user_device.name, "user_dev");
	user_device.buf = device_data;
	user_device.buf_size = 2;
	/* initialize work item for printing device's error messages */
	k_work_init(&user_device.work,work_run_fun);

    return 0;
}

/* @brief command to creat Actions virtual_audio commands */
static int virtual_audio_start(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct device *dev;
	dev = (struct device *)device_get_binding("virtual_audio_out");
	virtual_audio_enable_io(dev,"virtuel_en");
	virtual_audio_disable_io(dev,"virtuel_disa");
	virtual_audio_ioctl_io(dev, 2, "virtuel_io");
	return 0;
}

DEVICE_DEFINE(virtual_audio, "virtual_audio_out", virtual_audio_out_init, NULL,
        &virtual_audio_drv_data0, &virtual_audio_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &virtual_audio_drv_api);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_virtual_fs_rw,
	SHELL_CMD(virtual_fs_rw, NULL, "virtual_fs_rw test.", fs_rw_test),
    SHELL_CMD(virtual_audio, NULL, "Actions virtual_audio commands.", virtual_audio_start));
SHELL_CMD_REGISTER(shell_virtual_test, &sub_virtual_fs_rw, "shell_virtual_test commands", NULL);