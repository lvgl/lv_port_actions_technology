/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_st77916.h"
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
	struct lcd_panel_data *data = dev->data;

	_panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
	k_msleep(120);

	data->in_sleep = 0;
}

static int _panel_init(const struct device *dev)
{
	const struct lcd_panel_config *config __unused = lcd_panel_get_config(dev);

	_panel_transmit_p1(dev, 0xF0, 0x28);
	_panel_transmit_p1(dev, 0xF2, 0x28);
	_panel_transmit_p1(dev, 0x73, 0xF0);
	_panel_transmit_p1(dev, 0x7C, 0xD1);
	_panel_transmit_p1(dev, 0x83, 0xE0);
	_panel_transmit_p1(dev, 0x84, 0x61);
	_panel_transmit_p1(dev, 0xF2, 0x82);
	_panel_transmit_p1(dev, 0xF0, 0x00);
	_panel_transmit_p1(dev, 0xF0, 0x01);
	_panel_transmit_p1(dev, 0xF1, 0x01);
	_panel_transmit_p1(dev, 0xB0, 0x56);
	_panel_transmit_p1(dev, 0xB1, 0x45);
	_panel_transmit_p1(dev, 0xB2, 0x28);
	_panel_transmit_p1(dev, 0xB3, 0x01);
	_panel_transmit_p1(dev, 0xB4, 0x47);
	_panel_transmit_p1(dev, 0xB5, 0x34);
	_panel_transmit_p1(dev, 0xB6, 0xC5);
	_panel_transmit_p1(dev, 0xB7, 0x30);
	_panel_transmit_p1(dev, 0xB8, 0x8C);
	_panel_transmit_p1(dev, 0xB9, 0x15);
	_panel_transmit_p1(dev, 0xBA, 0x00);
	_panel_transmit_p1(dev, 0xBB, 0x08);
	_panel_transmit_p1(dev, 0xBC, 0x08);
	_panel_transmit_p1(dev, 0xBD, 0x00);
	_panel_transmit_p1(dev, 0xBE, 0x00);
	_panel_transmit_p1(dev, 0xBF, 0x07);
	_panel_transmit_p1(dev, 0xC0, 0x80);
	_panel_transmit_p1(dev, 0xC1, 0x10);
	_panel_transmit_p1(dev, 0xC2, 0x37);
	_panel_transmit_p1(dev, 0xC3, 0x80);
	_panel_transmit_p1(dev, 0xC4, 0x10);
	_panel_transmit_p1(dev, 0xC5, 0x37);
	_panel_transmit_p1(dev, 0xC6, 0xA9);
	_panel_transmit_p1(dev, 0xC7, 0x41);
	_panel_transmit_p1(dev, 0xC8, 0x01);
	_panel_transmit_p1(dev, 0xC9, 0xA9);
	_panel_transmit_p1(dev, 0xCA, 0x41);
	_panel_transmit_p1(dev, 0xCB, 0x01);
	_panel_transmit_p1(dev, 0xCC, 0x7F);

	_panel_transmit_p1(dev, 0xCD, 0x7F);
	_panel_transmit_p1(dev, 0xCE, 0xFF);
	_panel_transmit_p1(dev, 0xD0, 0x91);
	_panel_transmit_p1(dev, 0xD1, 0x68);
	_panel_transmit_p1(dev, 0xD2, 0x68);

	const uint8_t data_f5[] = { 0xa5, 0x00 };
	_panel_transmit(dev, 0xF5, data_f5, sizeof(data_f5));
	_panel_transmit_p1(dev, 0xF1, 0x10);
	_panel_transmit_p1(dev, 0xF0, 0x00);
	_panel_transmit_p1(dev, 0xF0, 0x02);

	const uint8_t data_e0[] = { 0xf0, 0x0a, 0x12, 0x0b, 0x0B, 0x08, 0x3E, 0x44, 0x56, 0x0A, 0x18, 0x18, 0x37, 0x3B };
	_panel_transmit(dev, 0xE0, data_e0, sizeof(data_e0));

	const uint8_t data_e1[] = { 0xF0, 0x05, 0x0B, 0x08, 0x08, 0x06, 0x34, 0x33, 0x4C, 0x09, 0x16, 0x16, 0x30, 0x36 };
	_panel_transmit(dev, 0xE1, data_e1, sizeof(data_e1));

	_panel_transmit_p1(dev, 0xF0, 0x10);
	_panel_transmit_p1(dev, 0xF3, 0x10);
	_panel_transmit_p1(dev, 0xE0, 0x0C);
	_panel_transmit_p1(dev, 0xE1, 0x00);
	_panel_transmit_p1(dev, 0xE2, 0x05);
	_panel_transmit_p1(dev, 0xE3, 0x00);
	_panel_transmit_p1(dev, 0xE4, 0xE0);
	_panel_transmit_p1(dev, 0xE5, 0x06);
	_panel_transmit_p1(dev, 0xE6, 0x21);
	_panel_transmit_p1(dev, 0xE7, 0x80);
	_panel_transmit_p1(dev, 0xE8, 0x05);
	_panel_transmit_p1(dev, 0xE9, 0x82);
	_panel_transmit_p1(dev, 0xEA, 0xE4);
	_panel_transmit_p1(dev, 0xEB, 0x80);
	_panel_transmit_p1(dev, 0xEC, 0x00);
	_panel_transmit_p1(dev, 0xED, 0x14);
	_panel_transmit_p1(dev, 0xEE, 0xFF);
	_panel_transmit_p1(dev, 0xEF, 0x00);

	_panel_transmit_p1(dev, 0xF8, 0xFF);
	_panel_transmit_p1(dev, 0xF9, 0x00);
	_panel_transmit_p1(dev, 0xFA, 0x00);
	_panel_transmit_p1(dev, 0xFB, 0x30);
	_panel_transmit_p1(dev, 0xFC, 0x00);
	_panel_transmit_p1(dev, 0xFD, 0x00);
	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_p1(dev, 0xFF, 0x00);

	_panel_transmit_p1(dev, 0x60, 0x40);
	_panel_transmit_p1(dev, 0x61, 0x03);
	_panel_transmit_p1(dev, 0x62, 0x05);
	_panel_transmit_p1(dev, 0x63, 0x40);
	_panel_transmit_p1(dev, 0x64, 0x05);
	_panel_transmit_p1(dev, 0x65, 0x05);

	_panel_transmit_p1(dev, 0x66, 0x00);
	_panel_transmit_p1(dev, 0x67, 0x00);
	_panel_transmit_p1(dev, 0x68, 0x00);
	_panel_transmit_p1(dev, 0x69, 0x00);
	_panel_transmit_p1(dev, 0x6A, 0x00);
	_panel_transmit_p1(dev, 0x6B, 0x00);
	_panel_transmit_p1(dev, 0x70, 0x40);
	_panel_transmit_p1(dev, 0x71, 0x02);
	_panel_transmit_p1(dev, 0x72, 0x05);
	_panel_transmit_p1(dev, 0x73, 0x40);
	_panel_transmit_p1(dev, 0x74, 0x04);
	_panel_transmit_p1(dev, 0x75, 0x05);
	_panel_transmit_p1(dev, 0x76, 0x00);
	_panel_transmit_p1(dev, 0x77, 0x00);
	_panel_transmit_p1(dev, 0x78, 0x00);
	_panel_transmit_p1(dev, 0x79, 0x00);
	_panel_transmit_p1(dev, 0x7A, 0x00);
	_panel_transmit_p1(dev, 0x7B, 0x00);
	_panel_transmit_p1(dev, 0x80, 0x00);
	_panel_transmit_p1(dev, 0x81, 0x00);
	_panel_transmit_p1(dev, 0x82, 0x07);
	_panel_transmit_p1(dev, 0x83, 0x02);
	_panel_transmit_p1(dev, 0x84, 0xDF);
	_panel_transmit_p1(dev, 0x85, 0x00);
	_panel_transmit_p1(dev, 0x86, 0x00);
	_panel_transmit_p1(dev, 0x87, 0x00);
	_panel_transmit_p1(dev, 0x88, 0x48);
	_panel_transmit_p1(dev, 0x89, 0x00);
	_panel_transmit_p1(dev, 0x8A, 0x09);
	_panel_transmit_p1(dev, 0x8B, 0x02);
	_panel_transmit_p1(dev, 0x8C, 0xE1);
	_panel_transmit_p1(dev, 0x8D, 0x00);
	_panel_transmit_p1(dev, 0x8E, 0x00);
	_panel_transmit_p1(dev, 0x8F, 0x00);
	_panel_transmit_p1(dev, 0x90, 0x48);
	_panel_transmit_p1(dev, 0x91, 0x00);
	_panel_transmit_p1(dev, 0x92, 0x0B);
	_panel_transmit_p1(dev, 0x93, 0x02);
	_panel_transmit_p1(dev, 0x94, 0xE3);
	_panel_transmit_p1(dev, 0x95, 0x00);
	_panel_transmit_p1(dev, 0x96, 0x00);
	_panel_transmit_p1(dev, 0x97, 0x00);
	_panel_transmit_p1(dev, 0x98, 0x48);
	_panel_transmit_p1(dev, 0x99, 0x00);
	_panel_transmit_p1(dev, 0x9A, 0x0D);
	_panel_transmit_p1(dev, 0x9B, 0x02);
	_panel_transmit_p1(dev, 0x9C, 0xE5);
	_panel_transmit_p1(dev, 0x9D, 0x00);
	_panel_transmit_p1(dev, 0x9E, 0x00);
	_panel_transmit_p1(dev, 0x9F, 0x00);
	_panel_transmit_p1(dev, 0xA0, 0x48);
	_panel_transmit_p1(dev, 0xA1, 0x00);
	_panel_transmit_p1(dev, 0xA2, 0x06);
	_panel_transmit_p1(dev, 0xA3, 0x02);
	_panel_transmit_p1(dev, 0xA4, 0xDE);
	_panel_transmit_p1(dev, 0xA5, 0x00);
	_panel_transmit_p1(dev, 0xA6, 0x00);
	_panel_transmit_p1(dev, 0xA7, 0x00);
	_panel_transmit_p1(dev, 0xA8, 0x48);
	_panel_transmit_p1(dev, 0xA9, 0x00);
	_panel_transmit_p1(dev, 0xAA, 0x08);
	_panel_transmit_p1(dev, 0xAB, 0x02);
	_panel_transmit_p1(dev, 0xAC, 0xE0);
	_panel_transmit_p1(dev, 0xAD, 0x00);
	_panel_transmit_p1(dev, 0xAE, 0x00);
	_panel_transmit_p1(dev, 0xAF, 0x00);
	_panel_transmit_p1(dev, 0xB0, 0x48);
	_panel_transmit_p1(dev, 0xB1, 0x00);
	_panel_transmit_p1(dev, 0xB2, 0x0A);
	_panel_transmit_p1(dev, 0xB3, 0x02);
	_panel_transmit_p1(dev, 0xB4, 0xE2);
	_panel_transmit_p1(dev, 0xB5, 0x00);
	_panel_transmit_p1(dev, 0xB6, 0x00);
	_panel_transmit_p1(dev, 0xB7, 0x00);
	_panel_transmit_p1(dev, 0xB8, 0x48);
	_panel_transmit_p1(dev, 0xB9, 0x00);
	_panel_transmit_p1(dev, 0xBA, 0x0C);
	_panel_transmit_p1(dev, 0xBB, 0x02);
	_panel_transmit_p1(dev, 0xBC, 0xE4);
	_panel_transmit_p1(dev, 0xBD, 0x00);
	_panel_transmit_p1(dev, 0xBE, 0x00);
	_panel_transmit_p1(dev, 0xBF, 0x00);
	_panel_transmit_p1(dev, 0xC0, 0x01);
	_panel_transmit_p1(dev, 0xC1, 0x10);
	_panel_transmit_p1(dev, 0xC2, 0xAA);
	_panel_transmit_p1(dev, 0xC3, 0x65);
	_panel_transmit_p1(dev, 0xC4, 0x74);
	_panel_transmit_p1(dev, 0xC5, 0x47);
	_panel_transmit_p1(dev, 0xC6, 0x56);
	_panel_transmit_p1(dev, 0xC7, 0xBB);
	_panel_transmit_p1(dev, 0xC8, 0x88);
	_panel_transmit_p1(dev, 0xC9, 0x99);
	_panel_transmit_p1(dev, 0xD0, 0x01);
	_panel_transmit_p1(dev, 0xD1, 0x10);
	_panel_transmit_p1(dev, 0xD2, 0xAA);
	_panel_transmit_p1(dev, 0xD3, 0x65);
	_panel_transmit_p1(dev, 0xD4, 0x74);
	_panel_transmit_p1(dev, 0xD5, 0x47);
	_panel_transmit_p1(dev, 0xD6, 0x56);
	_panel_transmit_p1(dev, 0xD7, 0xBB);
	_panel_transmit_p1(dev, 0xD8, 0x88);
	_panel_transmit_p1(dev, 0xD9, 0x99);

	_panel_transmit_p1(dev, 0xF3, 0x01);
	_panel_transmit_p1(dev, 0xF0, 0x00);
	_panel_transmit_p1(dev, 0x21, 0x00);

#if 0
	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		_panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x55); /* rgb565 */
	} else {
		_panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x66); /* rgb888 */
	}
#endif

	/* TE on */
	_panel_transmit_cmd(dev, 0x35);

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
	_panel_transmit(dev, DDIC_CMD_CASET << 8, (uint8_t *)&cmd_data[0], 4);

	cmd_data[0] = sys_cpu_to_be16(y);
	cmd_data[1] = sys_cpu_to_be16(y + h - 1);
	_panel_transmit(dev, DDIC_CMD_RASET << 8, (uint8_t *)&cmd_data[0], 4);

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
	k_msleep(120);

	return 0;
}

static int _panel_lowpower_enter(const struct device *dev)
{
	_panel_transmit_cmd(dev, DDIC_CMD_IDMON);
	return 0;
}

static int _panel_lowpower_exit(const struct device *dev)
{
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

const struct lcd_panel_config lcd_panel_st77916_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWR),
	.cmd_ramwc = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_WRMEMC),
	.tw_reset = 10,
	.ts_reset = 120,
};
