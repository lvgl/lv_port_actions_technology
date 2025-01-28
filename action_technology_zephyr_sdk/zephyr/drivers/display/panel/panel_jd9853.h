/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_JD9853_DRIVER_H__
#define PANEL_JD9853_DRIVER_H__

#define DDIC_CMD_NOP			0x00 /* No Operation */
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
#define DDIC_CMD_PTRON			0x12 /* Partial Display Mode On */
#define DDIC_CMD_NORON			0x13 /* Normal Display Mode On */
#define DDIC_CMD_INVOFF			0x20 /* Display Inversion Off */
#define DDIC_CMD_INVON			0x21 /* Display Inversion On */
#define DDIC_CMD_ALLPOFF		0x22 /* All Pixel Off */
#define DDIC_CMD_ALLPON			0x23 /* All Pixel On */
#define DDIC_CMD_DISPOFF		0x28 /* Display Off */
#define DDIC_CMD_DISPON			0x29 /* Display On */
#define DDIC_CMD_CASET			0x2A /* Column Start Address Set */
#define DDIC_CMD_RASET			0x2B /* Row Start Address Set */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write Start */
#define DDIC_CMD_RAMRD			0x2E /* Memory Read Start */
#define DDIC_CMD_PTLAR			0x30 /* Partial Area Row Set */
#define DDIC_CMD_PTLAR_H		0x31 /* Partial Area Column Set */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect ON */
#define DDIC_CMD_MADCTL			0x36 /* Memory Data Access Control */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Control Interface Pixel Format */
#define DDIC_CMD_RAMWRC			0x3C /* Memory Write Continue */
#define DDIC_CMD_RAMRDC			0x3E /* Memory Read Continue */
#define DDIC_CMD_STESL			0x44 /* Set Tearing Effect Scan Line */
#define DDIC_CMD_GSL			0x45 /* Get Scan Line */
#define DDIC_CMD_DSTBON			0x4F /* Deep Standby Mode On */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness Value */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness Value */
#define DDIC_CMD_WRCTRLD		0x53 /* Write CTRL Display */
#define DDIC_CMD_RDCTRLD		0x54 /* Read CTRL Display */
#define DDIC_CMD_WRACL			0x55 /* Write ACL Control */
#define DDIC_CMD_RDACL			0x56 /* Read ACL Control */
#define DDIC_CMD_WRIMGEHCCTR	0x58 /* Write Color Enhance Control */
#define DDIC_CMD_RDIMGEHCCTR	0x59 /* Read Color Enhance Control */
#define DDIC_CMD_WRHBMDISBV		0x63 /* Write HBM Display Brightness Value */
#define DDIC_CMD_RDHBMDISBV		0x64 /* Read HBM Display Brightness Value */
#define DDIC_CMD_HBMCTL			0x66 /* Set HBM Mode */
#define DDIC_CMD_COLSET0		0x70 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET1		0x71 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET2		0x72 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET3		0x73 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET4		0x74 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET5		0x75 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET6		0x76 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET7		0x77 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET8		0x78 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET9		0x79 /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET10		0x7A /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET11		0x7B /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET12		0x7C /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET13		0x7D /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET14		0x7E /* Interface Pixel Format Set */
#define DDIC_CMD_COLSET15		0x7F /* Interface Pixel Format Set */
#define DDIC_CMD_COLOPT			0x80 /* Interface Pixel Format Option */
#define DDIC_CMD_RDDDBS			0xA1 /* Read DDB Start */
#define DDIC_CMD_RDDDBC			0xA8 /* Read DDB Continue */
#define DDIC_CMD_RDFCS			0xAA /* Read First Checksum */
#define DDIC_CMD_RDCCS			0xAF /* Read Continue Checksum */
#define DDIC_CMD_SETDISPMODE	0xC2 /* Set Display Mode */
#define DDIC_CMD_SETDSPIMODE	0xC4 /* Set Dual SPI Mode */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */
#define DDIC_CMD_MSC			0xFE /* CMD Page Switch */

#define DDIC_QSPI_CMD_RD(cmd)		((0x03 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_WR(cmd)		((0x02 << 24) | ((uint32_t)(cmd) << 8))
#define DDIC_QSPI_CMD_RAMWR(cmd)	((0x32 << 24) | ((uint32_t)(cmd) << 8))

#if 0
#define DDIC_CMD_DELAY			0xFF

typedef struct panel_regcfg {
	uint8_t cmd;    /* (write) command */
	uint8_t len;    /* parameter length or delay milliseconds */
	uint8_t dat[4]; /* parameter list */
} panel_regcfg_t;

static inline void panel_apply_config(
		const struct device *lcdc_dev, panel_regcfg_t *cfg, int num)
{
	for (; num > 0; num--, cfg++) {
		if (cfg->cmd == DDIC_CMD_DELAY) {
			k_msleep(cfg->len);
		} else {
			display_controller_write_config(lcdc_dev,
					DDIC_QSPI_CMD_WR(cfg->cmd), cfg->dat, cfg->len);
		}
	}
}
#endif

#endif /* PANEL_JD9853_DRIVER_H__ */
