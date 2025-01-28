/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_jd9853.h"
#include "panel_device.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  FUNCTIONS
 **********************/
static void _panel_transmit(const struct device *dev, uint32_t cmd,
		const uint8_t *tx_data, size_t tx_count)
{
	struct lcd_panel_data *data = dev->data;

	assert(data->transfering == 0);

	display_controller_write_config(data->lcdc_dev, DDIC_QSPI_CMD_WR(cmd), tx_data, tx_count);
}

static inline void _panel_transmit_cmd(const struct device *dev, uint32_t cmd)
{
	_panel_transmit(dev, cmd, NULL, 0);
}

static inline void _panel_transmit_p1(const struct device *dev, uint32_t cmd, uint8_t tx_data)
{
	_panel_transmit(dev, cmd, &tx_data, 1);
}

static void _panel_exit_sleep(const struct device *dev)
{
	struct lcd_panel_data *dev_data = dev->data;
	_panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
	k_msleep(120);
	uint8_t data[2] = {0};
	data[0] = 0x02;
	_panel_transmit(dev, 0xDE, data, 1);
	
	data[0] = 0x00;
	data[1] = 0x02;
	_panel_transmit(dev, 0xE5, data, 2);
	
	data[0] = 0x00;
	_panel_transmit(dev, 0xDE, data, 1);
	_panel_transmit_p1(dev, 0x29, 0x00);
	k_msleep(20);
	dev_data->in_sleep = 0;
}

static int _panel_init(const struct device *dev)
{
	uint8_t data[40] = {0};

	data[0] = 0x98;
	data[1] = 0x53;
	_panel_transmit(dev, 0xDF, data, 2);

	data[0] = 0x00;
	_panel_transmit(dev, 0xDE, data, 1);

	data[0] = 0x23;	
	_panel_transmit(dev, 0xB2, data, 1);

	data[0] = 0x00;
	data[1] = 0x35;
	data[2] = 0x00;
	data[3] = 0x5D;
	_panel_transmit(dev, 0xB7, data, 4);
	
	data[0] = 0x4C;
	data[1] = 0x2F;
	data[2] = 0x55;
	data[3] = 0x73;
	data[4] = 0x6F;
	data[5] = 0xF0;
	_panel_transmit(dev, 0xBB, data, 6);
	
	data[0] = 0x66;
	data[1] = 0xE6;
	_panel_transmit(dev, 0xC0, data, 2);
	
	data[0] = 0x12;
	_panel_transmit(dev, 0xC1, data, 1);
	
	data[0] = 0x7D;
	data[1] = 0x07;
	data[2] = 0x14;
	data[3] = 0x06;
	data[4] = 0xCC;
	data[5] = 0x71;
	data[6] = 0x72;
	data[7] = 0x77;
	_panel_transmit(dev, 0xC3, data, 8);
	
	data[0] = 0x00;//00=60Hz 04=57Hz 08=51Hz
	data[1] = 0x00;
	data[2] = 0xA0;//LN=320	Line
	data[3] = 0x79;
	data[4] = 0x0A;
	data[5] = 0x0B;
	data[6] = 0x16;
	data[7] = 0x79;
	data[8] = 0x0A;
	data[9] = 0x0B;
	data[10] = 0x16;
	data[11] = 0x82;
	_panel_transmit(dev, 0xC4, data, 12);
	
	data[0] = 0x3F;
	data[1] = 0x2F;
	data[2] = 0x26;
	data[3] = 0x1F;
	data[4] = 0x26;
	data[5] = 0x28;
	data[6] = 0x24;
	data[7] = 0x24;
	data[8] = 0x24;
	data[9] = 0x23;
	data[10] = 0x21;
	data[11] = 0x16;
	data[12] = 0x12;
	data[13] = 0x0D;
	data[14] = 0x07;
	data[15] = 0x02;
	data[16] = 0x3F;
	data[17] = 0x2F;
	data[18] = 0x26;
	data[19] = 0x1F;
	data[20] = 0x26;
	data[21] = 0x28;
	data[22] = 0x24;
	data[23] = 0x24;
	data[24] = 0x24;
	data[25] = 0x23;
	data[26] = 0x21;
	data[27] = 0x16;
	data[28] = 0x12;
	data[29] = 0x0D;
	data[30] = 0x07;
	data[31] = 0x02;
	_panel_transmit(dev, 0xC8, data, 32);
	
	data[0] = 0x04;
	data[1] = 0x06;
	data[2] = 0x6B;
	data[3] = 0x0F;
	data[4] = 0x00;
	_panel_transmit(dev, 0xD0, data, 5);
	
	data[0] = 0x00;
	data[1] = 0x30;
	_panel_transmit(dev, 0xD7, data, 2);
	
	data[0] = 0x14;
	_panel_transmit(dev, 0xE6, data, 1);
	
	data[0] = 0x01;
	_panel_transmit(dev, 0xDE, data, 1);
	
	data[0] = 0x04;//TE_SEL
	_panel_transmit(dev, 0xBB, data, 1);
	
	data[0] = 0x12;//INH_CHKSUM_SEL
	_panel_transmit(dev, 0xD7, data, 1);
	
	data[0] = 0x03;
	data[1] = 0x13;
	data[2] = 0xEF;
	data[3] = 0x38;
	data[4] = 0x38;
	_panel_transmit(dev, 0xB7, data, 5);
	
	data[0] = 0x14;
	data[1] = 0x15;
	data[2] = 0xC0;
	_panel_transmit(dev, 0xC1, data, 3);
	
	data[0] = 0x06;
	data[1] = 0x3A;
	_panel_transmit(dev, 0xC2, data, 2);
	
	data[0] = 0x72;
	data[1] = 0x12;
	_panel_transmit(dev, 0xC4, data, 2);
	
	data[0] = 0x00;
	_panel_transmit(dev, 0xBE, data, 1);
	
	data[0] = 0x02;
	_panel_transmit(dev, 0xDE, data, 1);
	
	data[0] = 0x00;
	data[1] = 0x02;
	_panel_transmit(dev, 0xE5, data, 2);
	
	data[0] = 0x01;
	data[1] = 0x02;
	_panel_transmit(dev, 0xE5, data, 2);

	data[0] = 0x00;
	_panel_transmit(dev, 0xDE, data, 1);
	
	data[0] = 0x00;
	_panel_transmit(dev, 0x35, data, 1);
	
	data[0] = 0x05;//06=RGB666 05=RGB565
	_panel_transmit(dev, 0x3A, data, 1);

	data[0] = 0x00;
	data[1] = 0x00;//Start_Y=0
	data[2] = 0x00;
	data[3] = 0xC7;//End_Y=119
	_panel_transmit(dev, 0x2A, data, 4);

	data[0] = 0x00;
	data[1] = 0x00;//Start_X=0
	data[2] = 0x01;
	data[3] = 0x3F;//End_X=319
	_panel_transmit(dev, 0x2B, data, 4);

	_panel_exit_sleep(dev);
	return 0;
}

static int _panel_set_brightness(const struct device *dev, uint8_t brightness)
{
	//pwm
	return 0;
}

static int _panel_set_mem_area(const struct device *dev, uint16_t x,
				 uint16_t y, uint16_t w, uint16_t h)
{
	uint16_t cmd_data[2];

	x += CONFIG_PANEL_MEM_OFFSET_X;
	y += CONFIG_PANEL_MEM_OFFSET_Y;

	cmd_data[0] = sys_cpu_to_be16(x);
	cmd_data[1] = sys_cpu_to_be16(x + w - 1);
	_panel_transmit(dev, DDIC_CMD_CASET, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(y);
	cmd_data[1] = sys_cpu_to_be16(y + h - 1);
	_panel_transmit(dev, DDIC_CMD_RASET, (uint8_t *)&cmd_data[0], 4);

	return 0;
}

static int _panel_blanking_on(const struct device *dev)
{
	_panel_transmit_cmd(dev, DDIC_CMD_DISPOFF);
	_panel_transmit_cmd(dev, DDIC_CMD_SLPIN);
	return 0;
}

static int _panel_blanking_off(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	if (data->in_sleep)
		_panel_exit_sleep(dev);

	_panel_transmit_cmd(dev, DDIC_CMD_DISPON);
	return 0;
}

static int _panel_lowpower_enter(const struct device *dev)
{
	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_cmd(dev, DDIC_CMD_IDMON);
	return 0;
}

static int _panel_lowpower_exit(const struct device *dev)
{
	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_cmd(dev, DDIC_CMD_IDMOFF);
	return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
	.blanking_on = _panel_blanking_on,
	.blanking_off = _panel_blanking_off,
	.lowpower_enter = _panel_lowpower_enter,
	.lowpower_exit = _panel_lowpower_exit,
	.set_brightness = _panel_set_brightness,
	.write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_jd9853_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWR),
	.cmd_ramwc = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWRC),
	.tw_reset = 30,
	.ts_reset = 120,
	.td_slpin = 83,
};
