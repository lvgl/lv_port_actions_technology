/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_FT2308_DRIVER_H__
#define PANEL_FT2308_DRIVER_H__

#define DDIC_CMD_NOP			0x00 /* No Operation */
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDNUMED		0x05 /* Read Number of Errors on DSI */
#define DDIC_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define DDIC_CMD_RDDCOLMOD		0x0C /* Read Display Pixel Format */
#define DDIC_CMD_RDDIM			0x0D /* Read Display Image Mode */
#define DDIC_CMD_RDDSM			0x0E /* Read Display Signal Mode */
#define DDIC_CMD_RDDSDR			0x0F /* Read Display Self-Diagnostic Result */
#define DDIC_CMD_SLPIN			0x10 /* Sleep In */
#define DDIC_CMD_SLPOUT			0x11 /* Sleep Out */
#define DDIC_CMD_PTLON			0x12 /* Partial Display Mode On */
#define DDIC_CMD_NORON			0x13 /* Normal Display Mode On */
#define DDIC_CMD_RAMZIP_SET		0x1C /* RAM Compression Setting */
#define DDIC_CMD_GAMSET			0x26 /* Gamma Set */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_CASET			0x2A /* Column Address Set */
#define DDIC_CMD_PASET			0x2B /* Page Address Set */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write Start */
#define DDIC_CMD_RAMRD			0x2E /* Memory Read */
#define DDIC_CMD_PTLAR			0x30 /* Partial Area Set */
#define DDIC_CMD_VPTLAR			0x31 /* Vertical Partial Area Set */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line ON */
#define DDIC_CMD_MADCTL			0x36 /* Memory Data Access Control */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Control Interface Pixel Format */
#define DDIC_CMD_RAMWRCNT		0x3C /* Memory Write Continue */
#define DDIC_CMD_RAMRDCNT		0x3E /* Memory Read Continue */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness Value */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness Value */
#define DDIC_CMD_WRCTRLD		0x53 /* Write CTRL Display */
#define DDIC_CMD_RDCTRLD		0x54 /* Read CTRL Display */
#define DDIC_CMD_WRACL			0x55 /* Auto Current Limit Control */
#define DDIC_CMD_RDACL			0x56 /* Read Auto Current Limit */
#define DDIC_CMD_FR_MANUAL		0x67 /* Frame Rate Control for Manual Mode */
#define DDIC_CMD_FR_AUTO		0x68 /* Frame Rate Control for Auto Mode */
#define DDIC_CMD_FR_LPF			0x69 /* Low Power Frame Setting */
#define DDIC_CMD_FCC_WA			0x94 /* Focal CleverColor - White Balance Adjustment */
#define DDIC_CMD_FCC_AOD_CLK	0x95 /* Focal CleverColor - AOD Clock */
#define DDIC_CMD_FCC_CGM		0x96 /* Focal CleverColor - CGM */
#define DDIC_CMD_RDDDBSTR		0xA1 /* Read DDB Start */
#define DDIC_CMD_RDDDBCNT		0xA8 /* Read DDB Continue */
#define DDIC_CMD_RESDEF			0xAC /* Read ESD Event Flag */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */

#define DDIC_QSPI_CMD_RD(cmd)		((0x03 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_WR(cmd)		((0x02 << 24) | ((uint32_t)(cmd)))
#define DDIC_QSPI_CMD_RAMWR(cmd)	((0x32 << 24) | ((uint32_t)(cmd) << 8))

#endif /* PANEL_FT2308_DRIVER_H__ */
