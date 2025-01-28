/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_GC9C01_DRIVER_H__
#define PANEL_GC9C01_DRIVER_H__

/* Decription of Level 1 Command */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDDSTA			0x09 /* Read Display Status */
#define DDIC_CMD_SLPIN			0x10 /* Sleep In */
#define DDIC_CMD_SLPOUT			0x11 /* Sleep Out */
#define DDIC_CMD_PTLON			0x12 /* Partial Display Mode On */
#define DDIC_CMD_NORON			0x13 /* Normal Display Mode On */
#define DDIC_CMD_INVOFF			0x20  /* Display Inversion Off */
#define DDIC_CMD_INVON			0x21 /* Display Inversion On */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_CASET			0x2A /* Set Column Start Address */
#define DDIC_CMD_RASET			0x2B /* Set Row Start Address */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write */
#define DDIC_CMD_PTLAR			0x30 /* Partial Area */
#define DDIC_CMD_VERTSCROLL		0x33 /* Vertical Scrolling Definition */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line ON */

#define DDIC_CMD_MADCTL			0x36 /* Memory Access Control */
#define DDIC_MADCTL_ROWCOL_SWAP	(0x1 << 5)
#define DDIC_MADCTL_COL_INV		(0x1 << 6)
#define DDIC_MADCTL_ROW_INV		(0x1 << 7)

#define DDIC_CMD_VERTSCROLL_ADDR	0x37 /* Vertical Scrolling Start Address */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */

#define DDIC_CMD_COLMOD			0x3A /* Pixel Format Set */
#define DDIC_COLMOD_RGB_16bit	(0x5 << 4)
#define DDIC_COLMOD_RGB_18bit	(0x6 << 4)
#define DDIC_COLMOD_MCU_gray	(0)
#define DDIC_COLMOD_MCU_3bit	(1)
#define DDIC_COLMOD_MCU_8bit	(2)
#define DDIC_COLMOD_MCU_12bit	(3)
#define DDIC_COLMOD_MCU_16bit	(5)
#define DDIC_COLMOD_MCU_18bit	(6)
#define DDIC_COLMOD_MCU_24bit	(7)

#define DDIC_CMD_RAMWRC			0x3C /* Write Memory Continue */
#define DDIC_CMD_STESL			0x44 /* Set Tear Scanline */
#define DDIC_CMD_GSL			0x45 /* Get Scanline */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness */
#define DDIC_CMD_WRCTRLD		0x53 /* Write Display Control */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */

/* Decription of Level 2 Command */
#define DDIC_CMD_RGBCTL			0xB0 /* RGB Interface Signal Control */
#define DDIC_CMD_PORCHCTL		0xB5 /* Blanking Porch Control */
#define DDIC_CMD_DISPFUNCTL		0xB6 /* Display Function Control */
#define DDIC_CMD_TECTL			0xB4 /* Tearing Effect Control */
#define DDIC_CMD_IFCTL			0xF6 /* Interface Control */

/* Decription of Level 3 Command */
#define DDIC_CMD_INVERSION		0xEC /* Inversion */
#define DDIC_CMD_SPI2DCTL		0xB1 /* SPI 2Data Control */
#define DDIC_CMD_PWRCTL1		0xC1 /* Power Control 1 */
#define DDIC_CMD_PWRCTL2		0xC3 /* Power Control 2 */
#define DDIC_CMD_PWRCTL3		0xC4 /* Power Control 3 */
#define DDIC_CMD_PWRCTL4		0xC9 /* Power Control 4 */
#define DDIC_CMD_INTERREG_EN1	0xFE /* Inter Register Enable 1 */
#define DDIC_CMD_INTERREG_EN2	0xEF /* Inter Register Enable 2 */
#define DDIC_CMD_GAMSET1		0xF0 /* Set Gamma 1 */
#define DDIC_CMD_GAMSET2		0xF1 /* Set Gamma 2 */
#define DDIC_CMD_GAMSET3		0xF2 /* Set Gamma 3 */
#define DDIC_CMD_GAMSET4		0xF3 /* Set Gamma 4 */

#endif /* PANEL_GC9C01_DRIVER_H__ */
