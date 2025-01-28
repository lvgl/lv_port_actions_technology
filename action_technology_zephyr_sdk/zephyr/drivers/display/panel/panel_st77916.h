/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_ST77916_DRIVER_H__
#define PANEL_ST77916_DRIVER_H__

/* Page Set Table */
#define DDIC_CMD_CSC1			0xF0 /* Command Set Ctrl 1 */
#define DDIC_CMD_CSC2			0xF1 /* Command Set Ctrl 2 */
#define DDIC_CMD_CSC3			0xF2 /* Command Set Ctrl 3 */
#define DDIC_CMD_CSC4			0xF3 /* Command Set Ctrl 4 */
#define DDIC_CMD_SPIOR			0xF3 /* SPI Others Read */

/* Command Table 1 */
#define DDIC_CMD_NOP			0x00 /* No Operation */
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDDST			0x09 /* Read Display Status */
#define DDIC_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define DDIC_CMD_RDDMADCTL		0x0B /* Read Display MADCTL */
#define DDIC_CMD_RDDCOLMOD		0x0C /* Read Display Pixel Format */
#define DDIC_CMD_RDDIM			0x0D /* Read Display Image Mode */
#define DDIC_CMD_RDDSM			0x0E /* Read Display Signal Mode */
#define DDIC_CMD_RDBST			0x0F /* Read Busy Status */
#define DDIC_CMD_SLPIN			0x10 /* Sleep In */
#define DDIC_CMD_SLPOUT			0x11 /* Sleep Out */
#define DDIC_CMD_NOROFF			0x12 /* Normal Out */
#define DDIC_CMD_NORON			0x13 /* Normal On */
#define DDIC_CMD_INVOFF			0x20 /* Display Inversion Off */
#define DDIC_CMD_INVON			0x21 /* Display Inversion On */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_CASET			0x2A /* Column Addresss Set */
#define DDIC_CMD_RASET			0x2B /* Row Address Set */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write */
#define DDIC_CMD_RAMRD			0x2E /* Memory Read */
#define DDIC_CMD_VSCRDEF		0x33 /* Vertical Scrolling Definition */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line Off */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line On */
#define DDIC_CMD_MADCTL			0x36 /* Memory Data Access Control */
#define DDIC_CMD_VSCSAD			0x37 /* Vertical Scroll Start Address of RAM */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Interface Pixel Format */
#define DDIC_CMD_WRMEMC			0x3C /* Write Memory Continue */
#define DDIC_CMD_RDMEMC			0x3E /* Read Memory Continue */
#define DDIC_CMD_HSCRDEF		0x43 /* Horizontal Scrolling Definition */
#define DDIC_CMD_TESLWR			0x44 /* Write Tear Scanline */
#define DDIC_CMD_TESLRD			0x45 /* Read Tear Scanline */
#define DDIC_CMD_HSCSAD			0x47 /* Horizontal Scroll Start Address of RAM */
#define DDIC_CMD_CPON			0x4A /* Compress On */
#define DDIC_CMD_CPOFF			0x4B /* Compress Off */
#define DDIC_CMD_RAMCLACT		0x4C /* Memory Clear Act */
#define DDIC_CMD_RAMCLSETR		0x4D /* Memory Clear Set R */
#define DDIC_CMD_RAMCLSETG		0x4E /* Memory Clear Set G */
#define DDIC_CMD_RAMCLSETB		0x4F /* Memory Clear Set B */
#define DDIC_CMD_CDCCTR			0x50 /* CDC Control */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness */
#define DDIC_CMD_WRCTRLD		0x53 /* Write CTRL Display */
#define DDIC_CMD_RDCTRLD		0x54 /* Read CTRL Display */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */

/* Command Table 2 */
#define DDIC_CMD_VRHPS			0xB0 /* VRHP Set */
#define DDIC_CMD_VRHNS			0xB1 /* VRHN Set */
#define DDIC_CMD_VCOMS			0xB2 /* VCOM GND Set */
#define DDIC_CMD_STEP14S		0xB5 /* STEP SET1 */
#define DDIC_CMD_STEP23S		0xB6 /* STEP SET2 */
#define DDIC_CMD_SBSTS			0xB7 /* SVDD_SVCL_SET */
#define DDIC_CMD_TCONS			0xBA /* TCON_SET */
#define DDIC_CMD_RGBVBP			0xBB /* RGB_VBP */
#define DDIC_CMD_RGBHBP			0xBC /* RGB_HBP */
#define DDIC_CMD_RGBSET			0xBD /* RGB_SET */
#define DDIC_CMD_FRCTRA1		0xC0 /* Frame Rate Control A1 in Normal Mode */
#define DDIC_CMD_FRCTRA2		0xC1 /* Frame Rate Control A2 in Normal Mode */
#define DDIC_CMD_FRCTRA3		0xC2 /* Frame Rate Control A3 in Normal Mode */
#define DDIC_CMD_FRCTRB1		0xC3 /* Frame Rate Control B1 in Idle Mode */
#define DDIC_CMD_FRCTRB2		0xC4 /* Frame Rate Control B2 in Idle Mode */
#define DDIC_CMD_FRCTRB3		0xC5 /* Frame Rate Control B3 in Idle Mode */
#define DDIC_CMD_PWRCTRA1		0xC6 /* Power Control A1 in Normal Mode */
#define DDIC_CMD_PWRCTRA2		0xC7 /* Power Control A2 in Normal Mode */
#define DDIC_CMD_PWRCTRA3		0xC8 /* Power Control A3 in Normal Mode */
#define DDIC_CMD_PWRCTRB1		0xC9 /* Power Control B1 in Idle Mode */
#define DDIC_CMD_PWRCTRB2		0xCA /* Power Control B2 in Idle Mode */
#define DDIC_CMD_PWRCTRB3		0xCB /* Power Control B3 in Idle Mode */
#define DDIC_CMD_DSTBDSLP		0xCF /* DSTB_DSLP */
#define DDIC_CMD_RESSET1		0xD0 /* Resolution Set 1 */
#define DDIC_CMD_RESSET2		0xD1 /* Resolution Set 2 */
#define DDIC_CMD_RESSET3		0xD2 /* Resolution Set 3 */
#define DDIC_CMD_VCMOFSET		0xDD /* VCOM OFFSET Set */
#define DDIC_CMD_VCMOFNSET		0xDE /* VCOM OFFSET NEW Set */
#define DDIC_CMD_GAMCTRP1		0xE0 /* Positive Voltage Gamma Control */
#define DDIC_CMD_GAMCTRN1		0xE1 /* Negative Voltage Gamma Control */

#define DDIC_QSPI_CMD_RD(cmd)		((0x0B << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_WR(cmd)		((0x02 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_RAMWR(cmd)	((0x32 << 24) | ((uint32_t)(cmd) << 8))

#endif /* PANEL_ST77916_DRIVER_H__ */
