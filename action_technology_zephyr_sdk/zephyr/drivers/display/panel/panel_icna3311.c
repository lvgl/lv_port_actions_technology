/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_icna3311.h"
#include "panel_device.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(lcd_panel, CONFIG_DISPLAY_LOG_LEVEL);

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
static void _panel_read(const struct device *dev, uint32_t cmd,
		uint8_t *rx_data, size_t rx_count)
{
	struct lcd_panel_data *data = dev->data;

	assert(data->transfering == 0);

	display_controller_read_config(data->lcdc_dev, DDIC_QSPI_CMD_RD(cmd), rx_data, rx_count);
}

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
	const struct lcd_panel_config *config = dev->config;

	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_p1(dev, 0xC4, 0x80);
	//_panel_transmit_p1(dev, 0x35, 0x00);
	_panel_transmit_p1(dev, 0x53, 0x20);
	_panel_transmit_p1(dev, 0x51, 0xFF);
	_panel_transmit_p1(dev, 0x63, 0xFF);

	/* te scanline */
	const uint8_t data_44[] = { CONFIG_PANEL_TE_SCANLINE >> 8, CONFIG_PANEL_TE_SCANLINE & 0xFF };
	_panel_transmit(dev, DDIC_CMD_STESL, data_44, sizeof(data_44));
	_panel_transmit_p1(dev, DDIC_CMD_TEON, 0x00);

	if (config->videomode.pixel_format == PIXEL_FORMAT_RGB_888) {
		_panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x77); /* RGB-888 */
	} else {
		_panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x55); /* RGB-565 */
	}

	//const uint8_t data_2A[] = { 0x00, 0x08, 0x01, 0x07 };
	//_panel_transmit(dev, 0x2A, data_2A, sizeof(data_2A));

	//const uint8_t data_2B[] = { 0x00, 0x00, 0x01, 0x91 };
	//_panel_transmit(dev, 0x2B, data_2B, sizeof(data_2B));

	/* Sleep Out */
	_panel_exit_sleep(dev);

	/* Display on */
	//_panel_transmit_cmd(dev, DDIC_CMD_DISPON);

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

static int _panel_esc_check(const struct device *dev)
{
	uint8_t regv = 0;

	_panel_read(dev, 0x0a, &regv, 1);
	if (regv != 0x9c) {
		LOG_ERR("0x0A: %02x", regv);
		return -EIO;
	}

	return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
	.blanking_on = _panel_blanking_on,
	.blanking_off = _panel_blanking_off,
	.lowpower_enter = _panel_lowpower_enter,
	.lowpower_exit = _panel_lowpower_exit,
	.check_esd = _panel_esc_check,
	.set_brightness = _panel_set_brightness,
	.write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_icna3311_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWR),
	.cmd_ramwc = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWRC),
	.tw_reset = 10,
	.ts_reset = 120,
};
