/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_gc9c01.h"
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

static void _panel_exit_sleep(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	_panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
	k_msleep(120);

	data->in_sleep = 0;
}

static void _panel_init_te(const struct device *dev)
{
	const struct lcd_panel_config *config = dev->config;

	if (config->videomode.flags & (DISPLAY_FLAGS_TE_HIGH | DISPLAY_FLAGS_TE_LOW)) {
		uint8_t tmp[2];

		tmp[0] = 0x10;
		_panel_transmit(dev, 0x84, tmp, 1);

		tmp[0] = 0x02; /* pulse width: 3 line time */
		tmp[1] = (config->videomode.flags & DISPLAY_FLAGS_TE_LOW) ? 1 : 0;
		_panel_transmit(dev, DDIC_CMD_TECTL, tmp, 2);

		sys_put_be16(CONFIG_PANEL_TE_SCANLINE, tmp);
		_panel_transmit(dev, DDIC_CMD_STESL, tmp, 2);

		tmp[0] = 0; /* only output vsync */
		_panel_transmit(dev, DDIC_CMD_TEON, tmp, 1);
	} else {
		_panel_transmit(dev, DDIC_CMD_TEOFF, NULL, 0);
	}
}

static int _panel_init(const struct device *dev)
{
	_panel_exit_sleep(dev);

	// internal reg enable
	_panel_transmit(dev, DDIC_CMD_INTERREG_EN1, NULL, 0);
	_panel_transmit(dev, DDIC_CMD_INTERREG_EN2, NULL, 0);

	//reg_en for 70\74
	_panel_transmit_p1(dev, 0x80, 0x11);
	//reg_en for 7C\7D\7E
	_panel_transmit_p1(dev, 0x81, 0x70);
	//reg_en for 90\93
	_panel_transmit_p1(dev, 0x82, 0x09);
	//reg_en for 98\99
	_panel_transmit_p1(dev, 0x83, 0x03);
	//reg_en for B5
	_panel_transmit_p1(dev, 0x84, 0x20);
	//reg_en for B9\BE
	_panel_transmit_p1(dev, 0x85, 0x42);
	//reg_en for C2~7
	_panel_transmit_p1(dev, 0x86, 0xfc);
	//reg_en for C8\CB
	_panel_transmit_p1(dev, 0x87, 0x09);
	//reg_en for EC
	_panel_transmit_p1(dev, 0x89, 0x10);
	//reg_en for F0~3\F6
	_panel_transmit_p1(dev, 0x8A, 0x4f);
	//reg_en for 60\63\64\66
	_panel_transmit_p1(dev, 0x8C, 0x59);
	//reg_en for 68\6C\6E
	_panel_transmit_p1(dev, 0x8D, 0x51);
	//reg_en for A1~3\A5\A7
	_panel_transmit_p1(dev, 0x8E, 0xae);
	//reg_en for AC~F\A8\A9
	_panel_transmit_p1(dev, 0x8F, 0xf3);

	_panel_transmit_p1(dev, DDIC_CMD_MADCTL, 0x00);
	// 565 frame
	_panel_transmit_p1(dev, DDIC_CMD_COLMOD, 0x05);
	// 2COL
	_panel_transmit_p1(dev, DDIC_CMD_INVERSION, 0x77);

	// rtn 60Hz
	const uint8_t rtn_data[] = { 0x01, 0x80, 0x00, 0x00, 0x00, 0x00 };
	_panel_transmit(dev, 0x74, rtn_data, sizeof(rtn_data));

	// bvdd 3x
	_panel_transmit_p1(dev, 0x98, 0x3E);
	// bvee -2x
	_panel_transmit_p1(dev, 0x99, 0x3E);

	// VBP
	//_panel_transmit_p1(p_gc9c01, 0xC3, 0x2A);
	_panel_transmit_p1(dev, 0xC3, 0x3A + 4);
	// VBN
	//_panel_transmit_p1(p_gc9c01, 0xC4, 0x18);
	_panel_transmit_p1(dev, 0xC4, 0x16 + 4);

	const uint8_t data_0xA1A2[] = { 0x01, 0x04 };
	_panel_transmit(dev, 0xA1, data_0xA1A2, sizeof(data_0xA1A2));
	_panel_transmit(dev, 0xA2, data_0xA1A2, sizeof(data_0xA1A2));

	// IREF
	_panel_transmit_p1(dev, 0xA9, 0x1C);

	const uint8_t data_0xA5[] = { 0x11, 0x09 }; //VDDMA,VDDML
	_panel_transmit(dev, 0xA5, data_0xA5, sizeof(data_0xA5));

	// RTERM
	_panel_transmit_p1(dev, 0xB9, 0x8A);
	//VBG_BUF, DVDD
	_panel_transmit_p1(dev, 0xA8, 0x5E);
	_panel_transmit_p1(dev, 0xA7, 0x40);
	//VDDSOU ,VDDGM
	_panel_transmit_p1(dev, 0xAF, 0x73);
	//VREE,VRDD
	_panel_transmit_p1(dev, 0xAE, 0x44);
	//VRGL ,VDDSF
	_panel_transmit_p1(dev, 0xAD, 0x38);
	//VRGL ,VDDSF (adjust refresh rate about 60.1~60.6 fps)
	_panel_transmit_p1(dev, 0xA3, 0x67);
	//VREG_VREF
	_panel_transmit_p1(dev, 0xC2, 0x02);
	//VREG1A
	_panel_transmit_p1(dev, 0xC5, 0x11);
	//VREG1B
	_panel_transmit_p1(dev, 0xC6, 0x0E);
	//VREG2A
	_panel_transmit_p1(dev, 0xC7, 0x13);
	//VREG2B
	_panel_transmit_p1(dev, 0xC8, 0x0D);

	//bvdd ref_ad
	_panel_transmit_p1(dev, 0xCB, 0x02);

	const uint8_t data_0x7C[] = { 0xB6, 0x26 };
	_panel_transmit(dev, 0x7C, data_0x7C, sizeof(data_0x7C));

	//VGLO
	_panel_transmit_p1(dev, 0xAC, 0x24);
	//EPF=2
	_panel_transmit_p1(dev, 0xF6, 0x80);

	//gip start
	const uint8_t data_0xB5[] = { 0x09, 0x09 }; //VFP VBP
	_panel_transmit(dev, 0xB5, data_0xB5, sizeof(data_0xB5));

	const uint8_t data_0x60[] = { 0x38, 0x0B, 0x5B, 0x56 }; //STV1&2
	_panel_transmit(dev, 0x60, data_0x60, sizeof(data_0x60));

	const uint8_t data_0x63[] = { 0x3A, 0xE0, 0x5B, 0x56 }; //STV3&4
	_panel_transmit(dev, 0x63, data_0x63, sizeof(data_0x63));

	const uint8_t data_0x64[] = { 0x38, 0x0D, 0x72, 0xDD, 0x5B, 0x56 }; //CLK_group1
	_panel_transmit(dev, 0x64, data_0x64, sizeof(data_0x64));

	const uint8_t data_0x66[] = { 0x38, 0x11, 0x72, 0xE1, 0x5B, 0x56 }; //CLK_group1
	_panel_transmit(dev, 0x66, data_0x66, sizeof(data_0x66));

	const uint8_t data_0x68[] = { 0x3B, 0x08, 0x08, 0x00, 0x08, 0x29, 0x5B }; //FLC&FLV 1~2
	_panel_transmit(dev, 0x68, data_0x68, sizeof(data_0x68));

	const uint8_t data_0x6E[] = {
		0x00, 0x00, 0x00, 0x07, 0x01, 0x13, 0x11, 0x0B, 0x09, 0x16, 0x15, 0x1D,
		0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1D, 0x15, 0x16, 0x0A,
		0x0C, 0x12, 0x14, 0x02, 0x08, 0x00, 0x00, 0x00,
	};
	_panel_transmit(dev, 0x6E, data_0x6E, sizeof(data_0x6E));

	//gip end

	//SOU_BIAS_FIX
	_panel_transmit_p1(dev, 0xBE, 0x11);

	const uint8_t data_0x6C[] = { 0xCC, 0x0C, 0xCC, 0x84, 0xCC, 0x04, 0x50 }; //precharge GATE
	_panel_transmit(dev, 0x6C, data_0x6C, sizeof(data_0x6C));

	_panel_transmit_p1(dev, 0x7D, 0x72);
	_panel_transmit_p1(dev, 0x7E, 0x38);

	const uint8_t data_0x70[] = { 0x02, 0x03, 0x09, 0x05, 0x0C, 0x06, 0x09, 0x05, 0x0C, 0x06 };
	_panel_transmit(dev, 0x70, data_0x70, sizeof(data_0x70));

	const uint8_t data_0x90[] = { 0x06, 0x06, 0x05, 0x06 };
	_panel_transmit(dev, 0x90, data_0x90, sizeof(data_0x90));

	const uint8_t data_0x93[] = { 0x45, 0xFF, 0x00 };
	_panel_transmit(dev, 0x93, data_0x93, sizeof(data_0x93));

	const uint8_t data_0xF0[] = { 0x46, 0x0B, 0x0F, 0x0A, 0x10, 0x3F };
	_panel_transmit(dev, DDIC_CMD_GAMSET1, data_0xF0, sizeof(data_0xF0));

	const uint8_t data_0xF1[] = { 0x52, 0x9A, 0x4F, 0x36, 0x37, 0xFF };
	_panel_transmit(dev, DDIC_CMD_GAMSET2, data_0xF1, sizeof(data_0xF1));

	const uint8_t data_0xF2[] = { 0x46, 0x0B, 0x0F, 0x0A, 0x10, 0x3F };
	_panel_transmit(dev, DDIC_CMD_GAMSET3, data_0xF2, sizeof(data_0xF2));

	const uint8_t data_0xF3[] = { 0x52, 0x9A, 0x4F, 0x36, 0x37, 0xFF };
	_panel_transmit(dev, DDIC_CMD_GAMSET4, data_0xF3, sizeof(data_0xF3));

	_panel_init_te(dev);
	return 0;
}

static int _panel_set_mem_area(const struct device *dev, uint16_t x, uint16_t y, uint16_t w,
				uint16_t h)
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
	k_msleep(120);

	return 0;
}

static const struct lcd_panel_ops lcd_panel_ops = {
	.init = _panel_init,
	.blanking_on = _panel_blanking_on,
	.blanking_off = _panel_blanking_off,
	.write_prepare = _panel_set_mem_area,
};

const struct lcd_panel_config lcd_panel_gc9c01_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = (0x32 << 24 | DDIC_CMD_RAMWR << 8),
	.cmd_ramwc = (0x32 << 24 | DDIC_CMD_RAMWRC << 8),
	.tw_reset = 10,
	.ts_reset = 120,
};
