/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_st77903.h"
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

	display_controller_write_config(data->lcdc_dev, ST77903_WR_CMD(cmd), tx_data, tx_count);
}

static inline void _panel_transmit_cmd(const struct device *dev, uint32_t cmd)
{
	_panel_transmit(dev, cmd, NULL, 0);
}

static inline void _panel_transmit_p1(const struct device *dev, uint32_t cmd, uint8_t tx_data)
{
	_panel_transmit(dev, cmd, &tx_data, 1);
}

static int _panel_init(const struct device *dev)
{
	const struct lcd_panel_config *config = dev->config;

	_panel_transmit_p1(dev, 0xf0, 0xc3);
	_panel_transmit_p1(dev, 0xf0, 0x96);
	_panel_transmit_p1(dev, 0xf0, 0xa5);
	_panel_transmit_p1(dev, 0xe9, 0x20);

	const uint8_t data_0xe7[] = { 0x80, 0x77, 0x1f, 0xcc, };
	_panel_transmit(dev, 0xe7, data_0xe7, sizeof(data_0xe7));

	const uint8_t data_0xc1[] = { 0x77, 0x07, 0xc2, 0x07, };
	_panel_transmit(dev, 0xc1, data_0xc1, sizeof(data_0xc1));

	const uint8_t data_0xc2[] = { 0x77, 0x07, 0xc2, 0x07, };
	_panel_transmit(dev, 0xc2, data_0xc2, sizeof(data_0xc2));

	const uint8_t data_0xc3[] = { 0x22, 0x02, 0x22, 0x04, };
	_panel_transmit(dev, 0xc3, data_0xc3, sizeof(data_0xc3));

	const uint8_t data_0xc4[] = { 0x22, 0x02, 0x22, 0x04, };
	_panel_transmit(dev, 0xc4, data_0xc4, sizeof(data_0xc4));

	_panel_transmit_p1(dev, 0xc5, 0x71);

	const uint8_t data_0xe0[] = {
		0x87, 0x09, 0x0c, 0x06, 0x05, 0x03, 0x29, 0x32,
		0x49, 0x0f, 0x1b, 0x17, 0x2a, 0x2f,
	};
	_panel_transmit(dev, 0xe0, data_0xe0, sizeof(data_0xe0));

	const uint8_t data_0xe1[] = {
		0x87, 0x09, 0x0c, 0x06, 0x05, 0x03, 0x29, 0x32,
		0x49, 0x0f, 0x1b, 0x17, 0x2a, 0x2f,
	};
	_panel_transmit(dev, 0xe1, data_0xe1, sizeof(data_0xe1));

	const uint8_t data_0xe5[] = {
		0xb2, 0xf5, 0xbd, 0x24, 0x22, 0x25, 0x10, 0x22,
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	};
	_panel_transmit(dev, 0xe5, data_0xe5, sizeof(data_0xe5));

	const uint8_t data_0xe6[] = {
		0xb2, 0xf5, 0xbd, 0x24, 0x22, 0x25, 0x10, 0x22,
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
	};
	_panel_transmit(dev, 0xe6, data_0xe6, sizeof(data_0xe6));

	const uint8_t data_0xec[] = { 0x40, 0x03, };
	_panel_transmit(dev, 0xec, data_0xec, sizeof(data_0xec));

	_panel_transmit_p1(dev, 0x36, 0x0c);

	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		_panel_transmit_p1(dev, 0x3a, 0x05);
	} else { /* RGB_888 */
		_panel_transmit_p1(dev, 0x3a, 0x07);
	}

	_panel_transmit_p1(dev, 0xb2, 0x00);
	_panel_transmit_p1(dev, 0xb3, 0x00); // ???
	_panel_transmit_p1(dev, 0xb4, 0x00);

	/* Set VFP(8) and VBP(8) in idle mode */
	const uint8_t data_0xb5[] = { 0x00, 0x08, 0x00, 0x8 };
	_panel_transmit(dev, 0xb5, data_0xb5, sizeof(data_0xb5));

	/* Resolution 400x400 */
	const uint8_t data_0xb6[] = { 0xc7, 0x31, };
	_panel_transmit(dev, 0xb6, data_0xb6, sizeof(data_0xb6));

	const uint8_t data_0xa5[] = {
		0x00, 0x00, 0x00, 0x00, 0x20, 0x15, 0x2a, 0x8a,
		0x02
	};
	_panel_transmit(dev, 0xa5, data_0xa5, sizeof(data_0xa5));

	const uint8_t data_0xa6[] = {
		0x00, 0x00, 0x00, 0x00, 0x20, 0x15, 0x2a, 0x8a,
		0x02
	};
	_panel_transmit(dev, 0xa6, data_0xa6, sizeof(data_0xa6));

	const uint8_t data_0xba[] = {
		0x0a, 0x5a, 0x23, 0x10, 0x25, 0x02, 0x00
	};
	_panel_transmit(dev, 0xba, data_0xba, sizeof(data_0xba));

	const uint8_t data_0xbb[] = {
		0x00, 0x30, 0x00, 0x29, 0x88, 0x87, 0x18, 0x00,
	};
	_panel_transmit(dev, 0xbb, data_0xbb, sizeof(data_0xbb));

	const uint8_t data_0xbc[] = {
		0x00, 0x30, 0x00, 0x29, 0x88, 0x87, 0x18, 0x00,
	};
	_panel_transmit(dev, 0xbc, data_0xbc, sizeof(data_0xbc));

	const uint8_t data_0xbd[] = {
		0xa1, 0xb2, 0x2b, 0x1a, 0x56, 0x43, 0x34, 0x65,
		0xff, 0xff, 0x0f,
	};
	_panel_transmit(dev, 0xbd, data_0xbd, sizeof(data_0xbd));

	/* TE on */
	_panel_transmit_p1(dev, 0x35, 0x00);

	_panel_transmit_cmd(dev, 0x21);
	//_panel_transmit_cmd(dev, 0x38);	 /* idle mode off */

	_panel_transmit_cmd(dev, 0x11); /* Sleep Out */
	k_msleep(120);

	_panel_transmit_cmd(dev, 0x29); /* Display On */
	k_msleep(120);

	return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
};

/* RGB-565 */
static uint8_t lcd_framebuffer_mem[CONFIG_PANEL_HOR_RES * CONFIG_PANEL_VER_RES * 2] __aligned(4);

const struct lcd_panel_config lcd_panel_st77903_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.fb_mem = lcd_framebuffer_mem,
	.cmd_ramwr = ST77903_WR_CMD(DDIC_CMD_VS),
	.cmd_hsync = ST77903_WR_CMD(DDIC_CMD_HS),
	.tw_reset = 10,
	.ts_reset = 120,
};
