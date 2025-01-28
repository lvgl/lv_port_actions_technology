/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_RM69090_DRIVER_H__
#define PANEL_RM69090_DRIVER_H__

#define DDIC_CMD_NOP			0x00
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDNUMED		0x05 /* Read Number of Errors on DSI */
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
#define DDIC_CMD_CASET			0x2A /* Set Column Start Address */
#define DDIC_CMD_RASET			0x2B /* Set Row Start Address */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write */
#define DDIC_CMD_PTLAR			0x30 /* Partial Area */
#define DDIC_CMD_VPTLAR			0x31 /* Vertical Partial Area */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect Line OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect Line ON */
#define DDIC_CMD_MADCTR			0x36 /* Scan Direction Control */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Enter Idle Mode */
#define DDIC_CMD_COLMOD			0x3A /* Interface Pixel Format */
#define DDIC_CMD_RAMWRC			0x3C /* Memory Continuous Write */

#define DDIC_CMD_STESL			0x44 /* Set Tear Scanline */
#define DDIC_CMD_GSL			0x45 /* Get Scanline */
#define DDIC_CMD_DSTBON			0x4F /* Deep Standby Mode On */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness */
#define DDIC_CMD_WRCTRLD		0x53 /* Write Display Control */
#define DDIC_CMD_RDCTRLD		0x54 /* Read Display Control */
#define DDIC_CMD_WRRADACL		0x55 /* RAD_ACL Control */
#define DDIC_CMD_SCE			0x58 /* Set Color Enhance */
#define DDIC_CMD_GCE			0x59 /* Read Color Enhance */
#define DDIC_CMD_WRHBMDISBV		0x63 /* Write HBM Display Brightness */
#define DDIC_CMD_RDHBMDISBV		0x64 /* Read HBM Display Brightness */
#define DDIC_CMD_HBM			0x66 /* Set HBM Mode */
#define DDIC_CMD_DEEPIDM		0x67 /* Set Deep Idle Mode */

#define DDIC_CMD_COLSET			0x70 /* Interface Pixel Format Set */
#define DDIC_CMD_COLOPT			0x80 /* Interface Pixel Format Option */
#define DDIC_CMD_RDDDBS			0xA1 /* Read DDB Start */
#define DDIC_CMD_RDDDBC			0xA8 /* Read DDB Continous */
#define DDIC_CMD_RDFCS			0xAA /* Read First Checksum */
#define DDIC_CMD_RDCCS			0xAF /* Read Continue Checksum */
#define DDIC_CMD_SETDISPMOD		0xC2 /* Set DISP Mode */
#define DDIC_CMD_SETDSPIMOD		0xC4 /* Set DSPI Mode */

#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */
#define DDIC_CMD_MAUCCTR		0xFE /* CMD Mode Switch */
#define DDIC_CMD_RDMAUCCTR		0xFF /* Read CMD Status */

#endif /* PANEL_RM69090_DRIVER_H__ */
