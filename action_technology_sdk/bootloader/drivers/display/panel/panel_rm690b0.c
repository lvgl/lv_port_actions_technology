/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_rm690b0.h"
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

	display_controller_write_config(data->lcdc_dev, (0x02 << 24) | (cmd << 8), tx_data, tx_count);
}

static inline void _panel_transmit_cmd(const struct device *dev, uint32_t cmd)
{
	_panel_transmit(dev, cmd, NULL, 0);
}

static inline void _panel_transmit_p1(const struct device *dev, uint32_t cmd, uint8_t tx_data)
{
	_panel_transmit(dev, cmd, &tx_data, 1);
}

static void _panel_transmit_p2(const struct device *dev, uint32_t cmd, uint8_t data1, uint8_t data2)
{
	uint8_t data_array[2] = { data1, data2, };

	_panel_transmit(dev, cmd, data_array, 2);
}

static void _panel_exit_sleep(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	_panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
	k_msleep(150);

	data->in_sleep = 0;
}

static int _panel_init(const struct device *dev)
{
	const struct lcd_panel_config *config = dev->config;

	_panel_transmit_p1(dev, 0xFE, 0x00);

	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		_panel_transmit_p1(dev, 0x3A, 0x55); /* rgb565 */
	} else {
		_panel_transmit_p1(dev, 0x3A, 0x77); /* rgb888 */
	}

	/* TE */
	_panel_transmit_p2(dev, DDIC_CMD_STESL,
			CONFIG_PANEL_TE_SCANLINE >> 8, CONFIG_PANEL_TE_SCANLINE & 0xff);
	_panel_transmit_p1(dev, DDIC_CMD_TEON, 0x00);

	_panel_transmit_p1(dev, 0x53, 0x20);
	_panel_transmit_p1(dev, 0x51, 0xFF);
	_panel_transmit_p1(dev, 0x63, 0xFF);
	_panel_transmit_p1(dev, 0xC4, 0x60);

	/* Sleep Out */
	_panel_exit_sleep(dev);

	return 0;
}

static int _panel_set_brightness(const struct device *dev, uint8_t brightness)
{
	_panel_transmit_p1(dev, DDIC_CMD_WRDISBV, brightness);
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

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
	.blanking_on = _panel_blanking_on,
	.blanking_off = _panel_blanking_off,
	.set_brightness = _panel_set_brightness,
	.write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_rm690b0_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = (0x32 << 24 | DDIC_CMD_RAMWR << 8),
	.cmd_ramwc = (0x32 << 24 | DDIC_CMD_RAMWRC << 8),
	.tw_reset = 30,
	.ts_reset = 120,
};
