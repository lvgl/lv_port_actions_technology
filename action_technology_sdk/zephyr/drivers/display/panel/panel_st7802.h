/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_ST7802_DRIVER_H__
#define PANEL_ST7802_DRIVER_H__

/* Command Table 1 */
#define DDIC_CMD_NOP			0x00 /* No Operation */
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDNUMED		0x05 /* Read Number of Errors on DSI */
#define DDIC_CMD_RDDST			0x09 /* Read Display Status */
#define DDIC_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define DDIC_CMD_RDDMADCTR		0x0B /* Read Display MADCTR */
#define DDIC_CMD_RDDCOLMOD		0x0C /* Read Display Pixel Format */
#define DDIC_CMD_RDDIM			0x0D /* Read Display Image Mode */
#define DDIC_CMD_RDDSM			0x0E /* Read Display Signal Mode */
#define DDIC_CMD_RDDSDR			0x0F /* Read Display Self-Diagnostic Result */
#define DDIC_CMD_SLPIN			0x10 /* Sleep In */
#define DDIC_CMD_SLPOUT			0x11 /* Sleep Out */
#define DDIC_CMD_PTLON			0x12 /* Partial Display Mode On */
#define DDIC_CMD_NORON			0x13 /* Normal Display Mode On */
#define DDIC_CMD_INVOFF			0x20 /* Display Inversion Off */
#define DDIC_CMD_INVON			0x21 /* Display Inversion On */
#define DDIC_CMD_ALLPOFF		0x22 /* All Pixel Off */
#define DDIC_CMD_ALLPON			0x23 /* All Pixel On */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_CASET			0x2A /* Column Addresss Set */
#define DDIC_CMD_RASET			0x2B /* Row Address Set */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write */
#define DDIC_CMD_PTLAR1			0x30 /* Partial Area 1 */
#define DDIC_CMD_PTLAR2			0x31 /* Partial Area 2 */
#define DDIC_CMD_HSCRDEF		0x32 /* Horizontal Scrolling Definition */
#define DDIC_CMD_VSCRDEF		0x33 /* Vertical Scrolling Definition */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line Off */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line On */
#define DDIC_CMD_MADCTR			0x36 /* Scan Direction Control */
#define DDIC_CMD_VSCRSADD		0x37 /* Vertical Scrolling Start Address */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Interface Pixel Format */
#define DDIC_CMD_HSCRSADD		0x3B /* Horizontal Scrolling Start Address */
#define DDIC_CMD_RAMWRC			0x3C /* Memory Continuous Write */
#define DDIC_CMD_SCGAPDEF		0x3F /* Scrolling Gap Definition */
#define DDIC_CMD_SPI_RD_EN		0x42 /* SPI Read Enable */

#define DDIC_CMD_STESL			0x44 /* Set Tear Scanline */
#define DDIC_CMD_GSL			0x45 /* Get Tear Scanline */
#define DDIC_CMD_PCD			0x47 /* Panel Crack Detection */
#define DDIC_CMD_DSTBON			0x4F /* Deep Standby Mode On */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness */
#define DDIC_CMD_WRCTRLD		0x53 /* Write Display Control */
#define DDIC_CMD_RDCTRLD		0x54 /* Read Display Control */
#define DDIC_CMD_WRACL			0x55 /* Write ACL Control */
#define DDIC_CMD_RDACL			0x56 /* Read ACL Control */
#define DDIC_CMD_SLRWR			0x58 /* Set Color Enhance */
#define DDIC_CMD_SLRRD			0x59 /* Read Color Enhance */
#define DDIC_CMD_WROPS			0x5A /* Write OPS Enable */
#define DDIC_CMD_RDOPS			0x5B /* Read OPS Enable */
#define DDIC_CMD_WRHBMBV		0x5C /* Write HBM Diaplay Brightness */
#define DDIC_CMD_RDHBMBV		0x5D /* Read HBM Diaplay Brightness */
#define DDIC_CMD_HBMEN			0x5E /* HBM Enable */
#define DDIC_CMD_DPIDLEEN		0x5F /* Deep Idle Enable */
#define DDIC_CMD_RDDDBS			0xA1 /* Read DDB */
#define DDIC_CMD_RDDDBC			0xA8 /* Read DDB Continuous */
#define DDIC_CMD_RDFCS			0xAA /* Read First Checksum */
#define DDIC_CMD_RDCCS			0xAF /* Read Continue Checksum */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */
#define DDIC_CMD_RDID4			0xDD /* Read ID4 */


#define DDIC_QSPI_CMD_RD(cmd)		((0x03 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_WR(cmd)		((0x02 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_RAMWR(cmd)	((0x32 << 24) | ((uint32_t)(cmd) << 8))

#endif /* PANEL_ST7802_DRIVER_H__ */
