/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PANEL_SH8601Z0_DRIVER_H__
#define PANEL_SH8601Z0_DRIVER_H__

#define DDIC_CMD_NOP			0x00 /* No Operation */
#define DDIC_CMD_SWRESET		0x01 /* Software Reset */
#define DDIC_CMD_RDDID			0x04 /* Read Display ID */
#define DDIC_CMD_RDNUMED		0x05 /* Read Number of Errors on DSI */
#define DDIC_CMD_RDDPM			0x0A /* Read Display Power Mode */
#define DDIC_CMD_RDDMADCTL		0x0B /* Read Display MADCTL */
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
#define DDIC_CMD_CASET			0x2A /* Column Address Set */
#define DDIC_CMD_PASET			0x2B /* Page Address Set */
#define DDIC_CMD_RAMWR			0x2C /* Memory Write Start */
#define DDIC_CMD_PTLAR			0x30 /* Partial Area Row Set */
#define DDIC_CMD_PTLAC			0x31 /* Partial Area Column Set */
#define DDIC_CMD_TEOFF			0x34 /* Tearing Effect OFF */
#define DDIC_CMD_TEON			0x35 /* Tearing Effect ON */
#define DDIC_CMD_MADCTL			0x36 /* Memory Data Access Control */
#define DDIC_CMD_IDMOFF			0x38 /* Idle Mode Off */
#define DDIC_CMD_IDMON			0x39 /* Idle Mode On */
#define DDIC_CMD_COLMOD			0x3A /* Control Interface Pixel Format */
#define DDIC_CMD_RAMWRC			0x3C /* Memory Write Continue */
#define DDIC_CMD_TESCAN			0x44 /* Set Tear Scan Line */
#define DDIC_CMD_RDSCAN			0x45 /* Read Scan Line Number */
#define DDIC_CMD_SPI_RDOFF		0x46 /* SPI Read Off */
#define DDIC_CMD_SPI_RDON		0x47 /* SPI Read On */
#define DDIC_CMD_AODOFF			0x48 /* AOD Mode Off */
#define DDIC_CMD_AODON			0x49 /* AOD Mode On */
#define DDIC_CMD_AOD_WRDISBV	0x4A /* Write Display Brightness Value in AOD Mode */
#define DDIC_CMD_AOD_RDDISBV	0x4B /* Read Display Brightness Value in AOD Mode */
#define DDIC_CMD_DSTB			0x4F /* Deep Standby Control */
#define DDIC_CMD_WRDISBV		0x51 /* Write Display Brightness Value */
#define DDIC_CMD_RDDISBV		0x52 /* Read Display Brightness Value */
#define DDIC_CMD_WRCTRLD1		0x53 /* Write CTRL Display 1 */
#define DDIC_CMD_RDCTRLD1		0x54 /* Read CTRL Display 1 */
#define DDIC_CMD_WRCTRLD2		0x55 /* Write CTRL Display 2 */
#define DDIC_CMD_RDCTRLD2		0x56 /* Read CTRL Display 2 */
#define DDIC_CMD_WR_CE			0x58 /* Write CE (Color Enhancement) */
#define DDIC_CMD_RD_CE			0x59 /* Read CE (Color Enhancement) */
#define DDIC_CMD_HBM_WRDISBV	0x63 /* Write Display Brightness Value in HBM Mode */
#define DDIC_CMD_HBM_RDDISBV	0x64 /* Read Display Brightness Value in HBM Mode */
#define DDIC_CMD_HBMCTL			0x66 /* HBM Control */
#define DDIC_CMD_COLSET0		0x70 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET1		0x71 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET2		0x72 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET3		0x73 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET4		0x74 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET5		0x75 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET6		0x76 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET7		0x77 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET8		0x78 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET9		0x79 /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET10		0x7A /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET11		0x7B /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET12		0x7C /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET13		0x7D /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET14		0x7E /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLSET15		0x7F /* SPI 1-1-1 Pixel Format Set */
#define DDIC_CMD_COLOPT			0x80 /* SPI 1-1-1/256 Pixel Format Option */
#define DDIC_CMD_RDDDBS			0xA1 /* Read DDB Start */
#define DDIC_CMD_RDDDBC			0xA8 /* Read DDB Continue */
#define DDIC_CMD_RDFCS			0xAA /* Read First Checksum */
#define DDIC_CMD_RDCCS			0xAF /* Read Continue Checksum */
#define DDIC_CMD_SPI_MODE		0xC4 /* SPI Mode Control */
#define DDIC_CMD_RDID1			0xDA /* Read ID1 */
#define DDIC_CMD_RDID2			0xDB /* Read ID2 */
#define DDIC_CMD_RDID3			0xDC /* Read ID3 */

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

#endif /* PANEL_SH8601Z0_DRIVER_H__ */
