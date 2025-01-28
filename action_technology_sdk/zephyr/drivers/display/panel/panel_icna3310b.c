/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_icna3310b.h"
#include "panel_device.h"

/*********************
 *      DEFINES
 *********************/

/**
 * For 466x466 round panel, CONFIG_PANEL_FULL_SCREEN_OPT_AREA can be defined as follows:
 * 1) 7 areas
 * #define CONFIG_PANEL_FULL_SCREEN_OPT_AREA \
 * 	{ \
 * 		{ 124,   0, 341,  27 }, \
 * 		{  68,  28, 397,  67 }, \
 * 		{  28,  68, 437, 123 }, \
 * 		{  0,  124, 465, 341 }, \
 * 		{  28, 342, 437, 397 }, \
 * 		{  68, 398, 397, 437 }, \
 * 		{ 124, 438, 341, 465 }, \
 * 	}
 *
 * 2) 3 areas
 * #define CONFIG_PANEL_FULL_SCREEN_OPT_AREA \
 * 	{ \
 * 		{ 68,   0, 397,  67 }, \
 * 		{  0,  68, 465, 397 }, \
 * 		{ 68, 398, 397, 465 }, \
 * 	}
 */

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
	const struct lcd_panel_config *config = lcd_panel_get_config(dev);

#if (CONFIG_PANEL_TIMING_REFRESH_RATE_HZ == 60)
	_panel_transmit_p1(dev, 0xFE, 0x20);
	_panel_transmit_p1(dev, 0xF4, 0x5A);
	_panel_transmit_p1(dev, 0xF5, 0x59);

	_panel_transmit_p1(dev, 0xFE, 0x40);
	_panel_transmit_p1(dev, 0x39, 0x24);
	_panel_transmit_p1(dev, 0x3D, 0x00);
	_panel_transmit_p1(dev, 0x3F, 0x00);
	_panel_transmit_p1(dev, 0x40, 0x11);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x65, 0xf1);
	_panel_transmit_p1(dev, 0x66, 0x48);
	_panel_transmit_p1(dev, 0x67, 0x80);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x8F, 0xF1);
	_panel_transmit_p1(dev, 0x90, 0x48);
	_panel_transmit_p1(dev, 0x91, 0x80);
#endif

#if (CONFIG_PANEL_TIMING_REFRESH_RATE_HZ == 50)
	_panel_transmit_p1(dev, 0xFE, 0x20);
	_panel_transmit_p1(dev, 0xF4, 0x5A);
	_panel_transmit_p1(dev, 0xF5, 0x59);

	_panel_transmit_p1(dev, 0xFE, 0x40);
	_panel_transmit_p1(dev, 0x39, 0x24);
	_panel_transmit_p1(dev, 0x3D, 0x08);
	_panel_transmit_p1(dev, 0x3F, 0x00);
	_panel_transmit_p1(dev, 0x40, 0x61);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x65, 0x22);
	_panel_transmit_p1(dev, 0x66, 0x56);
	_panel_transmit_p1(dev, 0x67, 0x90);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x8F, 0xF1);
	_panel_transmit_p1(dev, 0x90, 0x48);
	_panel_transmit_p1(dev, 0x91, 0x80);
#endif

#if (CONFIG_PANEL_TIMING_REFRESH_RATE_HZ == 40)
	_panel_transmit_p1(dev, 0xFE, 0x20);
	_panel_transmit_p1(dev, 0xF4, 0x5A);
	_panel_transmit_p1(dev, 0xF5, 0x59);

	_panel_transmit_p1(dev, 0xFE, 0x40);
	_panel_transmit_p1(dev, 0x39, 0x24);
	_panel_transmit_p1(dev, 0x3D, 0x08);
	_panel_transmit_p1(dev, 0x3F, 0x00);
	_panel_transmit_p1(dev, 0x40, 0xf1);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x65, 0x6a);
	_panel_transmit_p1(dev, 0x66, 0x6c);
	_panel_transmit_p1(dev, 0x67, 0x90);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x8F, 0xF1);
	_panel_transmit_p1(dev, 0x90, 0x48);
	_panel_transmit_p1(dev, 0x91, 0x80);
#endif

#if (CONFIG_PANEL_TIMING_REFRESH_RATE_HZ == 30)
	_panel_transmit_p1(dev, 0xFE, 0x20);
	_panel_transmit_p1(dev, 0xF4, 0x5A);
	_panel_transmit_p1(dev, 0xF5, 0x59);

	_panel_transmit_p1(dev, 0xFE, 0x40);
	_panel_transmit_p1(dev, 0x39, 0x24);
	_panel_transmit_p1(dev, 0x3D, 0x08);
	_panel_transmit_p1(dev, 0x3F, 0x01);
	_panel_transmit_p1(dev, 0x40, 0xe1);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x65, 0xf1);
	_panel_transmit_p1(dev, 0x66, 0x48);
	_panel_transmit_p1(dev, 0x67, 0x80);

	_panel_transmit_p1(dev, 0xFE, 0x70);
	_panel_transmit_p1(dev, 0x8F, 0xF1);
	_panel_transmit_p1(dev, 0x90, 0x48);
	_panel_transmit_p1(dev, 0x91, 0x80);
#endif

	_panel_transmit_p1(dev, 0xFE, 0xD0);//20230506
	_panel_transmit_p1(dev, 0x05, 0x5A);//20230506

	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_p1(dev, 0xC4, 0x80);

	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		_panel_transmit_p1(dev, 0x3A, 0x55); /* rgb565 */
	} else {
		_panel_transmit_p1(dev, 0x3A, 0x77); /* rgb888 */
	}

	/* te */
	const uint8_t data_44[] = { CONFIG_PANEL_TE_SCANLINE >> 8, CONFIG_PANEL_TE_SCANLINE & 0xFF };
	_panel_transmit(dev, DDIC_CMD_STESL, data_44, sizeof(data_44));
	_panel_transmit_p1(dev, DDIC_CMD_TEON, 0x00);

	_panel_transmit_p1(dev, 0x53, 0x20);

	/* brightness */
	//_panel_transmit_p1(dev, DDIC_CMD_WRDISBV, CONFIG_PANEL_BRIGHTNESS);
	/* HBM brightness */
	_panel_transmit_p1(dev, DDIC_CMD_WRHBMDISBV, CONFIG_PANEL_BRIGHTNESS);

//	_panel_transmit_p1(dev, 0xFE, 0x00);
//	_panel_transmit_p1(dev, 0xC4, 0x80);
//	_panel_transmit_p1(dev, 0x53, 0x20);
//	_panel_transmit_p1(dev, 0x51, 0x80);

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

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
	.blanking_on = _panel_blanking_on,
	.blanking_off = _panel_blanking_off,
	.lowpower_enter = _panel_lowpower_enter,
	.lowpower_exit = _panel_lowpower_exit,
	.set_brightness = _panel_set_brightness,
	.write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_icna3310b_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWR),
	.cmd_ramwc = DDIC_QSPI_CMD_RAMWR(DDIC_CMD_RAMWRC),
	.tw_reset = 10,
	.ts_reset = 120,
	.td_slpin = 83,
};
