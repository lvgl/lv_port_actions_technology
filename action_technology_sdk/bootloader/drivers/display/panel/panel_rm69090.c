/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include "panel_rm69090.h"
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

static void _panel_transmit_p4(const struct device *dev, uint32_t cmd,
		uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4)
{
	uint8_t data_array[4] = { data1, data2, data3, data4 };

	_panel_transmit(dev, cmd, data_array, 4);
}

static void _panel_exit_sleep(const struct device *dev)
{
	struct lcd_panel_data *data = dev->data;

	_panel_transmit_cmd(dev, DDIC_CMD_SLPOUT);
	k_msleep(150);

	data->in_sleep = 0;
}

static void _panel_init_te(const struct device *dev)
{
	const struct lcd_panel_config *config = dev->config;

	if (config->videomode.flags & (DISPLAY_FLAGS_TE_HIGH | DISPLAY_FLAGS_TE_LOW)) {
		uint8_t tmp[2];

		sys_put_be16(CONFIG_PANEL_TE_SCANLINE, tmp);
		_panel_transmit(dev, DDIC_CMD_STESL, tmp, 2);

		tmp[0] = 0x02; /* only output vsync */
		_panel_transmit(dev, DDIC_CMD_TEON, tmp, 1);
	} else {
		_panel_transmit(dev, DDIC_CMD_TEOFF, NULL, 0);
	}
}

static int _panel_init(const struct device *dev)
{
	const struct lcd_panel_config *config = dev->config;
	struct lcd_panel_data *data = dev->data;

	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x05, 0x00);
	_panel_transmit_p1(dev, 0x06, 0x72);
	_panel_transmit_p1(dev, 0x0D, 0x00);
	_panel_transmit_p1(dev, 0x0E, 0x81);//AVDD=6V
	_panel_transmit_p1(dev, 0x0F, 0x81);
	_panel_transmit_p1(dev, 0x10, 0x11);//AVDD=3VCI
	_panel_transmit_p1(dev, 0x11, 0x81);//VCL=-VCI
	_panel_transmit_p1(dev, 0x12, 0x81);
	_panel_transmit_p1(dev, 0x13, 0x80);//VGH=AVDD
	_panel_transmit_p1(dev, 0x14, 0x80);
	_panel_transmit_p1(dev, 0x15, 0x81);//VGL=
	_panel_transmit_p1(dev, 0x16, 0x81);
	_panel_transmit_p1(dev, 0x18, 0x66);//VGHR=6V
	_panel_transmit_p1(dev, 0x19, 0x88);//VGLR=-6V
	_panel_transmit_p1(dev, 0x5B, 0x10);//VREFPN5 Regulator Enable
	_panel_transmit_p1(dev, 0x5C, 0x55);
	_panel_transmit_p1(dev, 0x62, 0x19);//Normal VREFN
	_panel_transmit_p1(dev, 0x63, 0x19);//Idle VREFN
	_panel_transmit_p1(dev, 0x70, 0x54);
	_panel_transmit_p1(dev, 0x74, 0x0C);
	_panel_transmit_p1(dev, 0xC5, 0x10);// NOR=IDLE=GAM1 // HBM=GAM2

	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x25, 0x03);
	_panel_transmit_p1(dev, 0x26, 0x32);
	_panel_transmit_p1(dev, 0x27, 0x0A);
	_panel_transmit_p1(dev, 0x28, 0x08);
	_panel_transmit_p1(dev, 0x2A, 0x03);
	_panel_transmit_p1(dev, 0x2B, 0x32);
	_panel_transmit_p1(dev, 0x2D, 0x0A);
	_panel_transmit_p1(dev, 0x2F, 0x08);
	_panel_transmit_p1(dev, 0x30, 0x43);//43: 15Hz

	_panel_transmit_p1(dev, 0x66, 0x90);
	_panel_transmit_p1(dev, 0x72, 0x1A);//OVDD  4.6V
	_panel_transmit_p1(dev, 0x73, 0x13);//OVSS  -2.2V

	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x6A, 0x17);//RT4723  daz20013  0x17=-2.2

	_panel_transmit_p1(dev, 0x1B, 0x00);
	//VSR power saving
	_panel_transmit_p1(dev, 0x1D, 0x03);
	_panel_transmit_p1(dev, 0x1E, 0x03);
	_panel_transmit_p1(dev, 0x1F, 0x03);
	_panel_transmit_p1(dev, 0x20, 0x03);
	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x36, 0x00);
	_panel_transmit_p1(dev, 0x6C, 0x80);
	_panel_transmit_p1(dev, 0x6D, 0x19);//VGMP VGSP turn off at idle mode

	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x63, 0x00);
	_panel_transmit_p1(dev, 0x64, 0x0E);
	//Gamma1 - AOD/Normal
	_panel_transmit_p1(dev, 0xFE, 0x02);
	_panel_transmit_p1(dev, 0xA9, 0x30);//5.8V VGMPS
	_panel_transmit_p1(dev, 0xAA, 0xB9);//2.5V VGSP
	_panel_transmit_p1(dev, 0xAB, 0x01);
	//Gamma2 - HBM
	_panel_transmit_p1(dev, 0xFE, 0x03);//page2
	_panel_transmit_p1(dev, 0xA9, 0x30);//5.8V VGMP
	_panel_transmit_p1(dev, 0xAA, 0x90);//2V VGSP
	_panel_transmit_p1(dev, 0xAB, 0x01);
	//SW MAPPING
	_panel_transmit_p1(dev, 0xFE, 0x0C);
	_panel_transmit_p1(dev, 0x07, 0x1F);
	_panel_transmit_p1(dev, 0x08, 0x2F);
	_panel_transmit_p1(dev, 0x09, 0x3F);
	_panel_transmit_p1(dev, 0x0A, 0x4F);
	_panel_transmit_p1(dev, 0x0B, 0x5F);
	_panel_transmit_p1(dev, 0x0C, 0x6F);
	_panel_transmit_p1(dev, 0x0D, 0xFF);
	_panel_transmit_p1(dev, 0x0E, 0xFF);
	_panel_transmit_p1(dev, 0x0F, 0xFF);
	_panel_transmit_p1(dev, 0x10, 0xFF);

	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x42, 0x14);
	_panel_transmit_p1(dev, 0x43, 0x41);
	_panel_transmit_p1(dev, 0x44, 0x25);
	_panel_transmit_p1(dev, 0x45, 0x52);
	_panel_transmit_p1(dev, 0x46, 0x36);
	_panel_transmit_p1(dev, 0x47, 0x63);

	_panel_transmit_p1(dev, 0x48, 0x41);
	_panel_transmit_p1(dev, 0x49, 0x14);
	_panel_transmit_p1(dev, 0x4A, 0x52);
	_panel_transmit_p1(dev, 0x4B, 0x25);
	_panel_transmit_p1(dev, 0x4C, 0x63);
	_panel_transmit_p1(dev, 0x4D, 0x36);

	_panel_transmit_p1(dev, 0x4E, 0x16);
	_panel_transmit_p1(dev, 0x4F, 0x61);
	_panel_transmit_p1(dev, 0x50, 0x25);
	_panel_transmit_p1(dev, 0x51, 0x52);
	_panel_transmit_p1(dev, 0x52, 0x34);
	_panel_transmit_p1(dev, 0x53, 0x43);

	_panel_transmit_p1(dev, 0x54, 0x61);
	_panel_transmit_p1(dev, 0x55, 0x16);
	_panel_transmit_p1(dev, 0x56, 0x52);
	_panel_transmit_p1(dev, 0x57, 0x25);
	_panel_transmit_p1(dev, 0x58, 0x43);
	_panel_transmit_p1(dev, 0x59, 0x34);
	//Switch Timing Control
	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x3A, 0x00);
	_panel_transmit_p1(dev, 0x3B, 0x00);
	_panel_transmit_p1(dev, 0x3D, 0x12);
	_panel_transmit_p1(dev, 0x3F, 0x37);
	_panel_transmit_p1(dev, 0x40, 0x12);
	_panel_transmit_p1(dev, 0x41, 0x0F);
	_panel_transmit_p1(dev, 0x37, 0x0C);

	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x5D, 0x01);
	_panel_transmit_p1(dev, 0x75, 0x08);
	//VSR Marping command---L
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x5E, 0x0F);
	_panel_transmit_p1(dev, 0x5F, 0x12);
	_panel_transmit_p1(dev, 0x60, 0xFF);
	_panel_transmit_p1(dev, 0x61, 0xFF);
	_panel_transmit_p1(dev, 0x62, 0xFF);
	//VSR Marping command---R
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x76, 0xFF);
	_panel_transmit_p1(dev, 0x77, 0xFF);
	_panel_transmit_p1(dev, 0x78, 0x49);
	_panel_transmit_p1(dev, 0x79, 0xF3);
	_panel_transmit_p1(dev, 0x7A, 0xFF);
	 //VSR1-STV
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x00, 0x9D);
	_panel_transmit_p1(dev, 0x01, 0x00);
	_panel_transmit_p1(dev, 0x02, 0x00);
	_panel_transmit_p1(dev, 0x03, 0x00);
	_panel_transmit_p1(dev, 0x04, 0x00);
	_panel_transmit_p1(dev, 0x05, 0x01);
	_panel_transmit_p1(dev, 0x06, 0x01);
	_panel_transmit_p1(dev, 0x07, 0x01);
	_panel_transmit_p1(dev, 0x08, 0x00);
	//VSR2-SCK1
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x09, 0xDC);
	_panel_transmit_p1(dev, 0x0A, 0x00);
	_panel_transmit_p1(dev, 0x0B, 0x02);
	_panel_transmit_p1(dev, 0x0C, 0x00);
	_panel_transmit_p1(dev, 0x0D, 0x08);
	_panel_transmit_p1(dev, 0x0E, 0x01);
	_panel_transmit_p1(dev, 0x0F, 0xCE);
	_panel_transmit_p1(dev, 0x10, 0x16);
	_panel_transmit_p1(dev, 0x11, 0x00);
	//VSR3-SCK2
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x12, 0xDC);
	_panel_transmit_p1(dev, 0x13, 0x00);
	_panel_transmit_p1(dev, 0x14, 0x02);
	_panel_transmit_p1(dev, 0x15, 0x00);
	_panel_transmit_p1(dev, 0x16, 0x08);
	_panel_transmit_p1(dev, 0x17, 0x02);
	_panel_transmit_p1(dev, 0x18, 0xCE);
	_panel_transmit_p1(dev, 0x19, 0x16);
	_panel_transmit_p1(dev, 0x1A, 0x00);
	//VSR4-ECK1
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x1B, 0xDC);
	_panel_transmit_p1(dev, 0x1C, 0x00);
	_panel_transmit_p1(dev, 0x1D, 0x02);
	_panel_transmit_p1(dev, 0x1E, 0x00);
	_panel_transmit_p1(dev, 0x1F, 0x08);
	_panel_transmit_p1(dev, 0x20, 0x01);
	_panel_transmit_p1(dev, 0x21, 0xCE);
	_panel_transmit_p1(dev, 0x22, 0x16);
	_panel_transmit_p1(dev, 0x23, 0x00);
	//VSR5-ECK2
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x24, 0xDC);
	_panel_transmit_p1(dev, 0x25, 0x00);
	_panel_transmit_p1(dev, 0x26, 0x02);
	_panel_transmit_p1(dev, 0x27, 0x00);
	_panel_transmit_p1(dev, 0x28, 0x08);
	_panel_transmit_p1(dev, 0x29, 0x02);
	_panel_transmit_p1(dev, 0x2A, 0xCE);
	_panel_transmit_p1(dev, 0x2B, 0x16);
	_panel_transmit_p1(dev, 0x2D, 0x00);
	//VEN EM-STV
	_panel_transmit_p1(dev, 0xFE, 0x04);
	_panel_transmit_p1(dev, 0x53, 0x8A);
	_panel_transmit_p1(dev, 0x54, 0x00);
	_panel_transmit_p1(dev, 0x55, 0x03);
	_panel_transmit_p1(dev, 0x56, 0x01);
	_panel_transmit_p1(dev, 0x58, 0x01);
	_panel_transmit_p1(dev, 0x59, 0x00);
	_panel_transmit_p1(dev, 0x65, 0x76);
	_panel_transmit_p1(dev, 0x66, 0x19);
	_panel_transmit_p1(dev, 0x67, 0x00);

	_panel_transmit_p1(dev, 0xFE, 0x07);
	_panel_transmit_p1(dev, 0x15, 0x04);

	_panel_transmit_p1(dev, 0xFE, 0x05);
	_panel_transmit_p1(dev, 0x4C, 0x01);
	_panel_transmit_p1(dev, 0x4D, 0x82);
	_panel_transmit_p1(dev, 0x4E, 0x04);
	_panel_transmit_p1(dev, 0x4F, 0x00);
	_panel_transmit_p1(dev, 0x50, 0x20);
	_panel_transmit_p1(dev, 0x51, 0x10);
	_panel_transmit_p1(dev, 0x52, 0x04);
	_panel_transmit_p1(dev, 0x53, 0x41);
	_panel_transmit_p1(dev, 0x54, 0x0A);
	_panel_transmit_p1(dev, 0x55, 0x08);
	_panel_transmit_p1(dev, 0x56, 0x00);
	_panel_transmit_p1(dev, 0x57, 0x28);
	_panel_transmit_p1(dev, 0x58, 0x00);
	_panel_transmit_p1(dev, 0x59, 0x80);
	_panel_transmit_p1(dev, 0x5A, 0x04);
	_panel_transmit_p1(dev, 0x5B, 0x10);
	_panel_transmit_p1(dev, 0x5C, 0x20);
	_panel_transmit_p1(dev, 0x5D, 0x00);
	_panel_transmit_p1(dev, 0x5E, 0x04);
	_panel_transmit_p1(dev, 0x5F, 0x0A);
	_panel_transmit_p1(dev, 0x60, 0x01);
	_panel_transmit_p1(dev, 0x61, 0x08);
	_panel_transmit_p1(dev, 0x62, 0x00);
	_panel_transmit_p1(dev, 0x63, 0x20);
	_panel_transmit_p1(dev, 0x64, 0x40);
	_panel_transmit_p1(dev, 0x65, 0x04);
	_panel_transmit_p1(dev, 0x66, 0x02);
	_panel_transmit_p1(dev, 0x67, 0x48);
	_panel_transmit_p1(dev, 0x68, 0x4C);
	_panel_transmit_p1(dev, 0x69, 0x02);
	_panel_transmit_p1(dev, 0x6A, 0x12);
	_panel_transmit_p1(dev, 0x6B, 0x00);
	_panel_transmit_p1(dev, 0x6C, 0x48);
	_panel_transmit_p1(dev, 0x6D, 0xA0);
	_panel_transmit_p1(dev, 0x6E, 0x08);
	_panel_transmit_p1(dev, 0x6F, 0x04);
	_panel_transmit_p1(dev, 0x70, 0x05);
	_panel_transmit_p1(dev, 0x71, 0x92);
	_panel_transmit_p1(dev, 0x72, 0x00);
	_panel_transmit_p1(dev, 0x73, 0x18);
	_panel_transmit_p1(dev, 0x74, 0xA0);
	_panel_transmit_p1(dev, 0x75, 0x00);
	_panel_transmit_p1(dev, 0x76, 0x00);
	_panel_transmit_p1(dev, 0x77, 0xE4);
	_panel_transmit_p1(dev, 0x78, 0x00);
	_panel_transmit_p1(dev, 0x79, 0x04);
	_panel_transmit_p1(dev, 0x7A, 0x02);
	_panel_transmit_p1(dev, 0x7B, 0x01);
	_panel_transmit_p1(dev, 0x7C, 0x00);
	_panel_transmit_p1(dev, 0x7D, 0x00);
	_panel_transmit_p1(dev, 0x7E, 0x24);
	_panel_transmit_p1(dev, 0x7F, 0x4C);
	_panel_transmit_p1(dev, 0x80, 0x04);
	_panel_transmit_p1(dev, 0x81, 0x0A);
	_panel_transmit_p1(dev, 0x82, 0x02);
	_panel_transmit_p1(dev, 0x83, 0xC1);
	_panel_transmit_p1(dev, 0x84, 0x02);
	_panel_transmit_p1(dev, 0x85, 0x18);
	_panel_transmit_p1(dev, 0x86, 0x90);
	_panel_transmit_p1(dev, 0x87, 0x60);
	_panel_transmit_p1(dev, 0x88, 0x88);
	_panel_transmit_p1(dev, 0x89, 0x02);
	_panel_transmit_p1(dev, 0x8A, 0x09);
	_panel_transmit_p1(dev, 0x8B, 0x0C);
	_panel_transmit_p1(dev, 0x8C, 0x18);
	_panel_transmit_p1(dev, 0x8D, 0x90);
	_panel_transmit_p1(dev, 0x8E, 0x10);
	_panel_transmit_p1(dev, 0x8F, 0x08);
	_panel_transmit_p1(dev, 0x90, 0x00);
	_panel_transmit_p1(dev, 0x91, 0x10);
	_panel_transmit_p1(dev, 0x92, 0xA8);
	_panel_transmit_p1(dev, 0x93, 0x00);
	_panel_transmit_p1(dev, 0x94, 0x04);
	_panel_transmit_p1(dev, 0x95, 0x0A);
	_panel_transmit_p1(dev, 0x96, 0x00);
	_panel_transmit_p1(dev, 0x97, 0x08);
	_panel_transmit_p1(dev, 0x98, 0x10);
	_panel_transmit_p1(dev, 0x99, 0x28);
	_panel_transmit_p1(dev, 0x9A, 0x08);
	_panel_transmit_p1(dev, 0x9B, 0x04);
	_panel_transmit_p1(dev, 0x9C, 0x02);
	_panel_transmit_p1(dev, 0x9D, 0x03);

	_panel_transmit_p1(dev, 0xFE, 0x0C);
	_panel_transmit_p1(dev, 0x25, 0x00);
	_panel_transmit_p1(dev, 0x31, 0xEF);
	_panel_transmit_p1(dev, 0x32, 0xE3);
	_panel_transmit_p1(dev, 0x33, 0x00);
	_panel_transmit_p1(dev, 0x34, 0xE3);
	_panel_transmit_p1(dev, 0x35, 0xE3);
	_panel_transmit_p1(dev, 0x36, 0x80);
	_panel_transmit_p1(dev, 0x37, 0x00);
	_panel_transmit_p1(dev, 0x38, 0x79);
	_panel_transmit_p1(dev, 0x39, 0x00);
	_panel_transmit_p1(dev, 0x3A, 0x00);
	_panel_transmit_p1(dev, 0x3B, 0x00);
	_panel_transmit_p1(dev, 0x3D, 0x00);
	_panel_transmit_p1(dev, 0x3F, 0x00);
	_panel_transmit_p1(dev, 0x40, 0x00);
	_panel_transmit_p1(dev, 0x41, 0x00);
	_panel_transmit_p1(dev, 0x42, 0x00);
	_panel_transmit_p1(dev, 0x43, 0x01);

	if (config->videomode.refresh_rate == 50) {
		_panel_transmit_p1(dev, 0xFE, 0x01);
		_panel_transmit_p1(dev, 0x28, 0x64);
	} else if (config->videomode.refresh_rate == 30) {
		_panel_transmit_p1(dev, 0xFE, 0x01);
		_panel_transmit_p1(dev, 0x25, 0x07);
	}

	//_panel_transmit_p1(dev, 0xFE,0x04);
	//_panel_transmit_p1(dev, 0x05,0x08);

	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x6e, 0x0a); // 这两句把灭屏功耗降低一倍 (灭屏电流，LCD部分为110uA左右)

	_panel_transmit_p1(dev, 0xFE, 0x00);
	_panel_transmit_p1(dev, 0x36, 0x00); // 切记 RM69090 不能设置翻转 否则 撕裂

	_panel_transmit_p1(dev, 0x35, 0x00);
	_panel_transmit_p1(dev, 0xC4, 0x80);
	_panel_transmit_p1(dev, 0x51, data->brightness); //_panel_transmit_p1(dev, 0x51,0xFF);
	_panel_transmit_p4(dev, 0x2A, 0x00, 0x0C, 0x01, 0xD1);
	_panel_transmit_p4(dev, 0x2B, 0x00, 0x00, 0x01, 0xC5);

	if (config->videomode.pixel_format == PIXEL_FORMAT_BGR_565) {
		_panel_transmit_p1(dev, 0x3A, 0x55);//0x55:RGB565   0x77:RGB888
	} else {
		_panel_transmit_p1(dev, 0x3A, 0x77);//0x55:RGB565   0x77:RGB888
	}

	/* Sleep Out */
	_panel_exit_sleep(dev);

	_panel_init_te(dev);
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

	x += CONFIG_PANEL_MEM_OFFSET_X; // add 12 pixel offset ?
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
	k_msleep(80);

	return 0;
}

static int _panel_lowpower_enter(const struct device *dev)
{
	_panel_transmit_p1(dev, 0xFE, 0x01);
	/* 0x01 60Hz, 0x41 30Hz, 0x43 15Hz, 0x4B 5Hz, 0x7B 1Hz */
	_panel_transmit_p1(dev, 0x29, 0x4B);
	_panel_transmit_p1(dev, 0xFE, 0x00);
	return 0;
}

static int _panel_lowpower_exit(const struct device *dev)
{
	_panel_transmit_p1(dev, 0xFE, 0x01);
	_panel_transmit_p1(dev, 0x29, 0x01);
	_panel_transmit_p1(dev, 0xFE, 0x00);
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

const struct lcd_panel_config lcd_panel_rm69090_config = {
	.videoport = PANEL_VIDEO_PORT_INITIALIZER,
	.videomode = PANEL_VIDEO_MODE_INITIALIZER,
	.ops = &lcd_panel_ops,
	.cmd_ramwr = (0x32 << 24 | DDIC_CMD_RAMWR << 8),
	.cmd_ramwc = (0x32 << 24 | DDIC_CMD_RAMWRC << 8),
	.tw_reset = 10,
	.ts_reset = 120,
};
