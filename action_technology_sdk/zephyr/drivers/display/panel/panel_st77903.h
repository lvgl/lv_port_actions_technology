/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_ST77903_DRIVER_H__
#define PANEL_ST77903_DRIVER_H__

/* Command Table 1 */
#define DDIC_CMD_NOP			0x00
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDDST			0x09 /* Read Display Status */
#define DDIC_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define DDIC_CMD_RDDMADCTL		0x0B /* Read Display MADCTL */
#define DDIC_CMD_RDDCOLMOD		0x0C /* Read Display Pixel Format */
#define DDIC_CMD_RDDIM			0x0D /* Read Display Image Mode */
#define DDIC_CMD_RDDSM			0x0E /* Read Display Signal Mode */
#define DDIC_CMD_RDDSDR			0x0F /* Read Display Self-Diagnostic Result */
#define DDIC_CMD_SLPIN			0x10 /* Sleep In */
#define DDIC_CMD_SLPOUT			0x11 /* Sleep Out */
#define DDIC_CMD_INVOFF			0x20 /* Display Inversion Off */
#define DDIC_CMD_INVON			0x21 /* Display Inversion On */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line ON */
#define DDIC_CMD_MADCTL			0x36 /* Memory Data Access Control */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Interface Pixel Format */
#define DDIC_CMD_WRMEMC			0x3C /* Memory Continuous Write */
#define DDIC_CMD_STE			0x44 /* Set Tear Scanline */
#define DDIC_CMD_TESLRD			0x45 /* Read Scanline */
#define DDIC_CMD_HS				0x60 /* Horizontal SYNC Command */
#define DDIC_CMD_VS				0x61 /* Vertical SYNC Command */
#define DDIC_CMD_WRIDMC			0x90 /* Write two-color idle Mode color */
#define DDIC_CMD_RDIDMC			0x91 /* Read two-color idle Mode color */
#define DDIC_CMD_RDFCS			0xAA /* Read First Checksum */
#define DDIC_CMD_RDCFCS			0xAF /* Read Continue Checksum */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */

/* Command Table 2 */
#define DDIC_CMD_CK				0xF0 /* Command Key */
#define DDIC_CMD_ECFC			0xB0 /* Entry Code Function Control */
#define DDIC_CMD_FRC1			0xB1 /* Frame Rate Control 1 */
#define DDIC_CMD_GSC			0xB2 /* Gate Scan Control */
#define DDIC_CMD_VDMDC			0xB3 /* Video Mode Display Control */
#define DDIC_CMD_TCMDC			0xB4 /* Two color Mode Display Control */
#define DDIC_CMD_BPC			0xB5 /* Blank Porch Control */
#define DDIC_CMD_DISCN			0xB6 /* Display Function Control */
#define DDIC_CMD_EMSET			0xB7 /* Entry Mode Set */
#define DDIC_CMD_PWR			0xC0 /* Power Control */
#define DDIC_CMD_PWR1			0xC1 /* Power Control 1 */
#define DDIC_CMD_PWR2			0xC2 /* Power Control 2 */
#define DDIC_CMD_PWR3			0xC3 /* Power Control 3 */
#define DDIC_CMD_VCOMCTL		0xC5 /* Vcom Control */
#define DDIC_CMD_VMF1OFS		0xD6 /* Vcom Offset 1 */
#define DDIC_CMD_VMF2OFS		0xD7 /* Vcom Offset 2 */
#define DDIC_CMD_PGC			0xE0 /* Positive Gamma Control */
#define DDIC_CMD_NGC			0xE1 /* Negative Gamma Control */
#define DDIC_CMD_ANAMODE		0xE5 /* Analog System Control */
#define DDIC_CMD_DTRCON			0xD9 /* Dithering Control */
#define DDIC_CMD_SRECON			0xDE /* SRE Control */
#define DDIC_CMD_RLCMODE		0xC8 /* Run-length Control */

#define DDIC_CMD_RGBIF			0xA0 /* RGB Interface Control */

#define ST77903_RD_CMD(cmd)			((0xDD << 24) | ((uint32_t)(cmd) << 8))
#define ST77903_WR_CMD(cmd)			((0xDE << 24) | ((uint32_t)(cmd) << 8))

#endif /* PANEL_ST77903_DRIVER_H__ */
