/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief aw87390 audio implementation
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
#include <logging/log.h>
#include <drivers/i2c.h>
#include <stdlib.h>
#include "../phy_audio_common.h"
LOG_MODULE_REGISTER(aw87390_audio, CONFIG_LOG_DEFAULT_LEVEL);

#define AW87390_SLAVE_ADDR 0x58
#define USE_ADP_MODE 0
#define I2C_DEV_NAME "I2C_1"
typedef struct
{
    uint8_t addr;
    uint8_t val;
}iic_set_t;

static const iic_set_t AW87390_register_setting[] =
{
    #if(USE_ADP_MODE == 1)
    {0x67,0x23},
    #else
    {0x67,0x03},
    #endif
    {0x01,0x03},
    {0x02,0x07},
    {0x02,0x00},
    #if(USE_ADP_MODE == 1)
    {0x02,0x1C},
    #else
    {0x02,0x0C},
    #endif
    //{0x03,0x08},  //8V
    {0x03,0x02},    //6.5V
    {0x04,0x05},
    //{0x05,0x08},    //12dB
    {0x05,0x0C},  //18dB
    //{0x05,0x12},    //27dB
    //{0x06,0x07},  //0.8W@8ohm
    {0x06,0x04},    //0.5W@8ohm
    {0x07,0x4E},
    //{0x08,0x06},    //1.6W
    //{0x08,0x08},    //2.0W
    //{0x08,0x04},    //1.2W
    {0x08,0x02},    //0.8W
    //{0x08,0x01},    //0.6W
    {0x09,0x08},
    #if(USE_ADP_MODE == 1)
    {0x0A,0x00},
    #else
    {0x0A,0x3A},
    #endif
    {0x61,0xB3},
    {0x62,0x24},
    {0x63,0x05},
    #if(USE_ADP_MODE == 1)
    {0x64,0x24},
    {0x65,0x15},
    #else
    {0x64,0x48},
    {0x65,0x17},
    #endif
    {0x79,0x7A},
    {0x7A,0x6C},
    {0x78,0x80},
    {0x66,0x38},
    {0x76,0x00},
    {0x78,0x00},
    {0x68,0x1B},
    {0x69,0x5B},
    #if(USE_ADP_MODE == 1)
    {0x70,0x1C},
    {0x71,0x00},
    {0x72,0xFE},
    #else
    {0x70,0x1D},
    {0x71,0x10},
    {0x72,0xB4},
    #endif
    {0x73,0x4F},
    {0x74,0x24},
    {0x75,0x02},
    {0x01,0x07},
    #if(USE_ADP_MODE == 1)
    {0xFF,0x50},
    #else
    {0xFF,0x00},
    #endif
};

static uint8_t drv_aw87390_regs_init(const struct device *dev)
{
	uint8_t buf[2] = {0};
	uint8_t num = ARRAY_SIZE(AW87390_register_setting);
    const struct device *const iic_dev_1 = device_get_binding(I2C_DEV_NAME);

	if (!iic_dev_1) {
		LOG_ERR("iic device not found!");
		return -1;
	}else{
		LOG_INF("iic device has been found!\n");
	}
    for(uint8_t i = 0; i< num; i++)
    {
        buf[0] = AW87390_register_setting[i].addr;
        buf[1] = AW87390_register_setting[i].val;
        i2c_write_async(iic_dev_1, buf, 2, AW87390_SLAVE_ADDR,NULL,NULL);
	    k_msleep(1);
	}
    LOG_INF("drv_aw87390_regs_init  successfully\n");
	return 0;
}

static int aw87390_enable(struct device *dev, void *param)
{
    uint8_t buf[2] = {0x1,0x7};//enable pa
    const struct device *const iic_dev_1 = device_get_binding(I2C_DEV_NAME);

    if (!iic_dev_1) {
        LOG_ERR("iic device not found!");
        return -1;
    }else{
        LOG_DBG("iic device has been found!\n");
    }

    i2c_write_async(iic_dev_1, buf, 2, AW87390_SLAVE_ADDR,NULL,NULL);
    k_msleep(1);
    LOG_INF("aw87390_enable\n");
    return 0;
}

static int aw87390_disable(struct device *dev, void *param)
{
    uint8_t buf[2] = {0x1,0x3};//disable pa
    const struct device *const iic_dev_1 = device_get_binding(I2C_DEV_NAME);

    if (!iic_dev_1) {
        LOG_ERR("iic device not found!");
        return -1;
    }else{
        LOG_DBG("iic device has been found!\n");
    }

    i2c_write_async(iic_dev_1, buf, 2, AW87390_SLAVE_ADDR,NULL,NULL);
    k_msleep(1);

    LOG_INF("aw87390_disable\n");
    return 0;
}

static int aw87390_audioioctl(struct device *dev, uint32_t cmd, void *param)
{
    LOG_INF("aw87390_audioioctl,cmd:%d,param:%p\n",cmd,param);
    return 0;
}

const struct phy_audio_driver_api aw87390_drv_api = {
    .audio_enable = aw87390_enable,
    .audio_disable = aw87390_disable,
    .audio_ioctl = aw87390_audioioctl,
};

/* aw87390_out_init  initialization */
static int aw87390_out_init(const struct device *dev)
{
    uint8_t ret;
    ret = drv_aw87390_regs_init(NULL);
    if(ret == 0)
        LOG_INF("aw87390_out init successfully\n");
    else
        LOG_ERR("aw87390_out init failed\n");
    return ret;
}

static int cmd_config_aw87390(const struct shell *shell, size_t argc, char **argv)
{

	uint16_t i;
	/*read start addr */
	uint8_t r_start_add[1] = {0x00};
	uint8_t r_str_data[12] = {0x00};
    /*write start addr and data */
	uint8_t w_reg_s[2] = {0x00, 0x00};
	uint8_t w_reg_addr = 0x00;
	uint8_t w_reg_data = 0x00;

	if(argc == 3){
        w_reg_addr = atoi(argv[1]);
        w_reg_data = atoi(argv[2]);
        printk("w_reg_addr:%0x\n", w_reg_addr);
        printk("w_reg_data:%0x\n", w_reg_data);
	}

	w_reg_s[0] = w_reg_addr;
	w_reg_s[1] = w_reg_data;
	const struct device *const iic_dev_1 = device_get_binding(I2C_DEV_NAME);

	if (!iic_dev_1) {
		printk("iic device not found!");
		return 0;
	}else
	{
		printk("iic device has been found!\n");
	}
	if(argc == 3)
		i2c_write_async(iic_dev_1, w_reg_s, 2, AW87390_SLAVE_ADDR, NULL, NULL);
	else
		printk("iput addr and data\n");

	i2c_write_async(iic_dev_1, r_start_add, 1, AW87390_SLAVE_ADDR, NULL, NULL);
	i2c_read_async(iic_dev_1, r_str_data, 12, AW87390_SLAVE_ADDR, NULL, NULL);
	k_msleep(1000);
	for(i = 0; i < 12; i++)
	{
		printk("r_str_data%d=====:%0x\n",i,r_str_data[i]);
	}

	return 0;

}

DEVICE_DEFINE(aw87390_audio, "aw87390_out", aw87390_out_init, NULL,
        NULL, NULL,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &aw87390_drv_api);

SHELL_CMD_REGISTER(write_aw87390, NULL, "dbg aw87390 commands", cmd_config_aw87390);