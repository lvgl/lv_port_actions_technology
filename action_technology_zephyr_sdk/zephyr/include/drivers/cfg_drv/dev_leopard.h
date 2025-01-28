/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEV_LEOPARD_H_
#define DEV_LEOPARD_H_

/*
dma cfg
*/
#define  CONFIG_DMA_0_NAME                  "DMA_0"
#define  CONFIG_DMA_0_PCHAN_NUM              10
#define  CONFIG_DMA_0_VCHAN_NUM              10
#define  CONFIG_DMA_0_VCHAN_PCHAN_NUM        6
#define  CONFIG_DMA_0_VCHAN_PCHAN_START      4

/*
pwm cfg
*/
#define  CONFIG_PWM_CHANS                    4

/*
DSP cfg
*/
#define  CONFIG_DSP_NAME                  "DSP"


/*
BTC cfg
*/
#define  CONFIG_BTC_NAME                  "BTC"

/*
 * Audio cfg
 */
#ifndef CONFIG_AUDIO_OUT_ACTS_DEV_NAME
#define CONFIG_AUDIO_OUT_ACTS_DEV_NAME  "audio_out"
#endif
#ifndef CONFIG_AUDIO_IN_ACTS_DEV_NAME
#define CONFIG_AUDIO_IN_ACTS_DEV_NAME   "audio_in"
#endif

/*
 * DSP cfg
 */
#ifndef CONFIG_DSP_ACTS_DEV_NAME
#define CONFIG_DSP_ACTS_DEV_NAME  "dsp_acts"
#endif


/*
PMUADC cfg
*/
#define	PMUADC_ID_CHARGI		(0)
#define	PMUADC_ID_BATV			(1)
#define	PMUADC_ID_DC5V			(2)
#define PMUADC_ID_SENSOR		(3)
#define PMUADC_ID_SVCC			(4)
#define	PMUADC_ID_LRADC1		(5)
#define	PMUADC_ID_VCCI		    (6)
#define	PMUADC_ID_LRADC2		(7)
#define	PMUADC_ID_LRADC3		(8)
#define	PMUADC_ID_LRADC4		(9)
#define	PMUADC_ID_LRADC5		(10)
#define	PMUADC_ID_LRADC6		(11)

/**
 * LCD panel cfg (sync drivers/display/display_controller.h and drivers/display.h)
 */
/* Enumeration with possible display major port type */
#define PANEL_PORT_Unknown  (0)
#define PANEL_PORT_MCU      (1)
#define PANEL_PORT_TR       (2)
#define PANEL_PORT_SPI      (4)

/* Enumeration with possible display mcu port type */
#define PANEL_MCU_8080  (0) /* Intel 8080 */
#define PANEL_MCU_6800  (1) /* Moto 6800 */

/* Enumeration with possible display spi port type */
#define PANEL_SPI_3LINE_1  (0)
#define PANEL_SPI_3LINE_2  (1)
#define PANEL_SPI_4LINE_1  (2)
#define PANEL_SPI_4LINE_2  (3)
#define PANEL_QSPI         (4)
#define PANEL_QSPI_SYNC    (5)
#define PANEL_QSPI_DDR_0   (6)
#define PANEL_QSPI_DDR_1   (7)
#define PANEL_QSPI_DDR_2   (8)

/* Enumeration of full port type */
#define PANEL_PORT_TYPE(major, minor) (((major) << 8) | (minor))

#define PANEL_PORT_MCU_8080  PANEL_PORT_TYPE(PANEL_PORT_MCU, PANEL_MCU_8080)
#define PANEL_PORT_MCU_6800  PANEL_PORT_TYPE(PANEL_PORT_MCU, PANEL_MCU_6800)

#define PANEL_PORT_SPI_3LINE_1  PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_SPI_3LINE_1)
#define PANEL_PORT_SPI_3LINE_2  PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_SPI_3LINE_2)
#define PANEL_PORT_SPI_4LINE_1  PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_SPI_4LINE_1)
#define PANEL_PORT_SPI_4LINE_2  PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_SPI_4LINE_2)
#define PANEL_PORT_QSPI         PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_QSPI)
#define PANEL_PORT_QSPI_SYNC    PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_QSPI_SYNC)
#define PANEL_PORT_QSPI_DDR_0   PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_QSPI_DDR_0)
#define PANEL_PORT_QSPI_DDR_1   PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_QSPI_DDR_1)
#define PANEL_PORT_QSPI_DDR_2   PANEL_PORT_TYPE(PANEL_PORT_SPI, PANEL_QSPI_DDR_2)

/* Enumeration with possible display TR port type */
#define PANEL_PORT_TR_LCD  PANEL_PORT_TYPE(PANEL_PORT_TR, 0)

/* Display pixel format enumeration. */
#define PANEL_PIXEL_FORMAT_RGB_888   (0x1 << 0)
#define PANEL_PIXEL_FORMAT_ARGB_8888 (0x1 << 3)
#define PANEL_PIXEL_FORMAT_RGB_565   (0x1 << 4)
#define PANEL_PIXEL_FORMAT_BGR_565   (0x1 << 5)
#define PANEL_PIXEL_FORMAT_BGR_888   (0x1 << 6)
#define PANEL_PIXEL_FORMAT_XRGB_8888 (0x1 << 7)

#endif /* DEV_LEOPARD_H_ */
