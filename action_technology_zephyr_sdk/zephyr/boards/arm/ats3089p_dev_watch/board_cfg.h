/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BOARD_CFG_H
#define __BOARD_CFG_H

#define LCD_PADDRV_LEVEL (1)
#define LCD_CLK_PADDRV_LEVEL (1)

#include <drivers/cfg_drv/dev_config.h>
#include <soc.h>

/*
 *  The device module enables the definition, If 1, the corresponding module   is opened, the GPIO configuration is enabled,
 *		If 0, the corresponding module is closed, and the GPIO configuration is turned off
 */
#define CONFIG_GPIO_A							1
#define CONFIG_GPIO_A_NAME      			"GPIOA"
#define CONFIG_GPIO_B							1
#define CONFIG_GPIO_B_NAME      			"GPIOB"
#define CONFIG_GPIO_C							1
#define CONFIG_GPIO_C_NAME      			"GPIOC"
#define CONFIG_WIO      						1
#define CONFIG_WIO_NAME         			"WIO"
#define CONFIG_EXTEND_GPIO						0
#define CONFIG_EXTEND_GPIO_NAME             "GPIOD"

#define CONFIG_GPIO_PIN2NAME(x)          (((x) < 32) ? CONFIG_GPIO_A_NAME : (((x) < 64) ? CONFIG_GPIO_B_NAME : CONFIG_GPIO_C_NAME))

#define CONFIG_SPI_FLASH_0						1
#define CONFIG_SPI_FLASH_NAME      			"spi_flash"
#define CONFIG_SPI_FLASH_1                      0
#define CONFIG_SPI_FLASH_1_NAME      		"spi_flash_1"
#define CONFIG_SPI_FLASH_2                      0
#define CONFIG_SPI_FLASH_2_NAME      		"spi_flash_2"
#define CONFIG_SIM_FLASH                        0
#define CONFIG_SIM_FLASH_NAME               "sim_flash"
#define CONFIG_ACTLOG_STORAGE_NAME           CONFIG_SPI_FLASH_NAME
#define CONFIG_BLOCK_DEV_FLASH                  1
#define CONFIG_BLOCK_DEV_FLASH_NAME         "spinand_flash"

#define CONFIG_SPINAND_3                    	1
#define CONFIG_SPINAND_FLASH_NAME      	    "spinand"

#define CONFIG_MMC_0                			0
#define CONFIG_MMC_0_NAME           		"MMC_0"
#define CONFIG_MMC_1                			0
#define CONFIG_MMC_1_NAME           		"MMC_1"

#define CONFIG_SD                   			1
#define CONFIG_SD_NAME              		"sd"

#define CONFIG_UART_0           				1
#define CONFIG_UART_0_NAME      			"UART_0"
#define CONFIG_UART_1           				0
#define CONFIG_UART_1_NAME      			"UART_1"
#define CONFIG_UART_2           				0
#define CONFIG_UART_2_NAME      			"UART_2"
#define CONFIG_UART_3           				0
#define CONFIG_UART_3_NAME      			"UART_3"
#define CONFIG_UART_4           				0
#define CONFIG_UART_4_NAME      			"UART_4"

#define CONFIG_PWM          					1
#define CONFIG_PWM_NAME      				"PWM"

#define CONFIG_I2C_0           					0
#define CONFIG_I2C_0_NAME      				"I2C_0"
#define CONFIG_I2C_1           					1
#define CONFIG_I2C_1_NAME      				"I2C_1"
#define CONFIG_I2C_2           					1
#define CONFIG_I2C_2_NAME      				"I2C_2"
#define CONFIG_I2C_3           					0
#define CONFIG_I2C_3_NAME      				"I2C_3"

#define CONFIG_SPI_1           					0
#define CONFIG_SPI_1_NAME      				"SPI_1"
#define CONFIG_SPI_2           					0
#define CONFIG_SPI_2_NAME      				"SPI_2"
#define CONFIG_SPI_3           					0
#define CONFIG_SPI_3_NAME      				"SPI_3"

#define CONFIG_I2CMT_0           				1
#define CONFIG_I2CMT_0_NAME      			"I2CMT_0"
#define CONFIG_I2CMT_1           				1
#define CONFIG_I2CMT_1_NAME      			"I2CMT_1"

#define CONFIG_SPIMT_0           				0
#define CONFIG_SPIMT_0_NAME      			"SPIMT_0"
#define CONFIG_SPIMT_1           				0
#define CONFIG_SPIMT_1_NAME      			"SPIMT_1"


#define CONFIG_AUDIO_DAC_0                      1
#define CONFIG_AUDIO_DAC_0_NAME             "DAC_0"
#define CONFIG_AUDIO_ADC_0						          1
#define CONFIG_AUDIO_ADC_0_NAME             "ADC_0"
#define CONFIG_AUDIO_I2STX_0					          0
#define CONFIG_AUDIO_I2STX_0_NAME           "I2STX_0"
#define CONFIG_AUDIO_I2SRX_0					          0
#define CONFIG_AUDIO_I2SRX_0_NAME           "I2SRX_0"
#define CONFIG_AUDIO_SPDIFRX_0					0
#define CONFIG_AUDIO_SPDIFRX_0_NAME         "SPDIFRX_0"
#define CONFIG_AUDIO_SPDIFTX_0                  0
#define CONFIG_AUDIO_SPDIFTX_0_NAME         "SPDIFTX_0"

#define CONFIG_PANEL							1
#define CONFIG_LCD_DISPLAY_DEV_NAME			"lcd_panel"

#define CONFIG_DISPLAY_ENGINE_DEV				1
#define CONFIG_DISPLAY_ENGINE_DEV_NAME		"de_acts"

#define CONFIG_DMA2D_LITE_DEV					1
#define CONFIG_DMA2D_LITE_DEV_NAME			"dma2d_lite_acts"

#define CONFIG_JPEG_HW_DEV                   1
#define CONFIG_JPEG_HW_DEV_NAME          	"jpeg_hw_acts"

#define CONFIG_LCDC_DEV							1
#define CONFIG_LCDC_DEV_NAME				"lcdc_acts"

#define CONFIG_GPU_DEV							1
#define CONFIG_GPU_DEV_NAME					"gpu"

#define CONFIG_ADCKEY                           0
#define CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME   "keyadc"

#define CONFIG_GPIOKEY                          1
#define CONFIG_INPUT_DEV_ACTS_GPIOKEY_NAME  "keygpio"

#define CONFIG_ONOFFKEY                         1
#define CONFIG_INPUT_DEV_ACTS_ONOFF_KEY_NAME "onoffkey"

#define CONFIG_TPKEY							1
#define CONFIG_TPKEY_DEV_NAME				 "tpkey"

#define CONFIG_ACTS_BATTERY						1
#define CONFIG_ACTS_BATTERY_DEV_NAME         "batadc"

#define CONFIG_VIBRATOR                         1
#define CONFIG_VIBRATOR_DEV_NAME             "VIBRATOR"

#define CONFIG_CEC				 				0
#define CONFIG_ACTS_BATTERY_NTC 				1

#define CONFIG_UART_0_USE_TX_DMA   1
#define CONFIG_UART_0_TX_DMA_CHAN  0x2
#define CONFIG_UART_0_TX_DMA_ID    1
#define CONFIG_UART_0_USE_RX_DMA   1
#define CONFIG_UART_0_RX_DMA_CHAN  0xff
#define CONFIG_UART_0_RX_DMA_ID    1

#define CONFIG_UART_1_USE_TX_DMA   0
#define CONFIG_UART_1_TX_DMA_CHAN  0xff
#define CONFIG_UART_1_TX_DMA_ID    2

#define CONFIG_MMC_0_USE_DMA        1
#define CONFIG_MMC_0_DMA_CHAN       0xff
#define CONFIG_MMC_0_DMA_ID         5

#define CONFIG_MMC_1_USE_DMA        0
#define CONFIG_MMC_1_DMA_CHAN       0xff
#define CONFIG_MMC_1_DMA_ID         6

#define CONFIG_PWM_USE_DMA   		1
#define CONFIG_PWM_DMA_CHAN  		0xff
#define CONFIG_PWM_DMA_ID    		21

#define CONFIG_I2C_0_USE_DMA   0
#define CONFIG_I2C_0_DMA_CHAN  0xff
#define CONFIG_I2C_0_DMA_ID    19

#define CONFIG_I2C_1_USE_DMA   0
#define CONFIG_I2C_1_DMA_CHAN  0xff
#define CONFIG_I2C_1_DMA_ID    20

#define CONFIG_I2C_2_USE_DMA   0
#define CONFIG_I2C_2_DMA_CHAN  0xff
#define CONFIG_I2C_2_DMA_ID    24

#define CONFIG_I2C_3_USE_DMA   0
#define CONFIG_I2C_3_DMA_CHAN  0xff
#define CONFIG_I2C_3_DMA_ID    25

#define CONFIG_I2CMT_0_USE_DMA   0
#define CONFIG_I2CMT_0_DMA_CHAN  0xff
#define CONFIG_I2CMT_1_USE_DMA   0
#define CONFIG_I2CMT_1_DMA_CHAN  0xff

#define CONFIG_SPI_1_USE_DMA   1
#define CONFIG_SPI_1_TXDMA_CHAN  0xff
#define CONFIG_SPI_1_RXDMA_CHAN  0xff
#define CONFIG_SPI_1_DMA_ID    8

#define CONFIG_SPI_2_USE_DMA   1
#define CONFIG_SPI_2_TXDMA_CHAN  0xff
#define CONFIG_SPI_2_RXDMA_CHAN  0xff
#define CONFIG_SPI_2_DMA_ID    9

#define CONFIG_SPI_3_USE_DMA   1
#define CONFIG_SPI_3_TXDMA_CHAN  0xff
#define CONFIG_SPI_3_RXDMA_CHAN  0xff
#define CONFIG_SPI_3_DMA_ID    10

#define CONFIG_SPIMT_0_DMA_CHAN  0xff
#define CONFIG_SPIMT_1_DMA_CHAN  0xff

/* The DMA channel for DAC FIFO0  */
#define CONFIG_AUDIO_DAC_0_FIFO0_DMA_CHAN       (0xff)
/* The DMA slot ID for DAC FIFO0 */
#define CONFIG_AUDIO_DAC_0_FIFO0_DMA_ID         (0xb)
/* The DMA channel for DAC FIFO1  */
#define CONFIG_AUDIO_DAC_0_FIFO1_DMA_CHAN       (0xff)
/* The DMA slot ID for DAC FIFO1 */
#define CONFIG_AUDIO_DAC_0_FIFO1_DMA_ID         (0xc)
/* The DMA channel for ADC FIFO0  */
#define CONFIG_AUDIO_ADC_0_FIFO0_DMA_CHAN       (0xff)
/* The DMA slot ID for ADC FIFO0 */
#define CONFIG_AUDIO_ADC_0_FIFO0_DMA_ID         (0xb)
/* The DMA channel for ADC FIFO1  */
#define CONFIG_AUDIO_ADC_0_FIFO1_DMA_CHAN       (0xff)
/* The DMA slot ID for ADC FIFO1 */
#define CONFIG_AUDIO_ADC_0_FIFO1_DMA_ID         (0xc)
/* The DMA channel for I2STX FIFO0  */
#define CONFIG_AUDIO_I2STX_0_FIFO0_DMA_CHAN     (0xff)
/* The DMA slot ID for I2STX FIFO0 */
#define CONFIG_AUDIO_I2STX_0_FIFO0_DMA_ID       (0xe)
/* The DMA channel for I2SRX FIFO0  */
#define CONFIG_AUDIO_I2SRX_0_FIFO0_DMA_CHAN     (0xff)
/* The DMA slot ID for I2SRX FIFO0 */
#define CONFIG_AUDIO_I2SRX_0_FIFO0_DMA_ID       (0xe)
/* The DMA channel for SPDIFRX FIFO0  */
#define CONFIG_AUDIO_SPDIFRX_0_FIFO0_DMA_CHAN    (0xff)
/* The DMA slot ID for SPDIFRX FIFO0 */
#define CONFIG_AUDIO_SPDIFRX_0_FIFO0_DMA_ID      (0x10)

/*
 *  Device module interrupt priority definition
 */
#define CONFIG_BTC_IRQ_PRI                		0

#define CONFIG_TWS_IRQ_PRI                		0

#define CONFIG_UART_0_IRQ_PRI   				0

#define CONFIG_UART_1_IRQ_PRI   				0

#define CONFIG_MMC_0_IRQ_PRI        			0

#define CONFIG_MMC_1_IRQ_PRI        			0

#define CONFIG_DMA_IRQ_PRI                      0

#define CONFIG_MPU_IRQ_PRI                      0

#define CONFIG_GPIO_IRQ_PRI                     0

#define CONFIG_I2C_0_IRQ_PRI   					0

#define CONFIG_I2C_1_IRQ_PRI   					0

#define CONFIG_I2C_2_IRQ_PRI   					0

#define CONFIG_I2C_3_IRQ_PRI   					0

#define CONFIG_SPI_1_IRQ_PRI   					0

#define CONFIG_SPI_2_IRQ_PRI  					0

#define CONFIG_SPI_3_IRQ_PRI   					0

#define CONFIG_I2CMT_0_IRQ_PRI   			    0

#define CONFIG_I2CMT_1_IRQ_PRI   				0

#define CONFIG_SPIMT_0_IRQ_PRI   				0

#define CONFIG_SPIMT_1_IRQ_PRI   				0

#define CONFIG_AUDIO_DAC_0_IRQ_PRI              0

#define CONFIG_AUDIO_ADC_0_IRQ_PRI              0

#define CONFIG_AUDIO_I2STX_0_IRQ_PRI            0

#define CONFIG_AUDIO_I2SRX_0_IRQ_PRI            0

#define CONFIG_AUDIO_SPDIFRX_0_IRQ_PRI          0

#define CONFIG_PMU_IRQ_PRI                      0

#define CONFIG_RTC_IRQ_PRI                      0

#define CONFIG_WDT_0_IRQ_PRI                    0

#define CONFIG_DSP_IRQ_PRI                		0

#define CONFIG_PMUADC_IRQ_PRI                   0

/*
spi nor flash cfg
*/
#define CONFIG_SPI_FLASH_CHIP_SIZE      0x2000000
#define CONFIG_SPI_FLASH_BUS_WIDTH      4
#define CONFIG_SPI_FLASH_DELAY_CHAIN    (11*3)  //unit:0.25ns
#define CONFIG_SPI_FLASH_NO_IRQ_LOCK    1
#define CONFIG_SPI_FLASH_FREQ_MHZ    	100

//#define CONFIG_SPI0_NOR_DTR_MODE
//#define CONFIG_SPI0_NOR_QPI_MODE

#define CONFIG_SPI_XIP_READ
#define CONFIG_SPI_XIP_VADDR 0x12000000    /*max 32MB xip read*/



/*
spi nand flash cfg
*/
#define CONFIG_SPINAND_USE_SPICONTROLER     3
#define CONFIG_SPINAND_FLASH_BUS_WIDTH      4
#define CONFIG_SPINAND_FLASH_FREQ_MHZ    	96
//#define CONFIG_SPINAND_POWER_CONTROL_SECONDS 5

/*
mmc board cfg
*/

#define CONFIG_MMC_0_BUS_WIDTH      4
#define CONFIG_MMC_0_CLKSEL         0     /*0 or 1, config by pinctrls*/
#define CONFIG_MMC_0_DATA_REG_WIDTH 4

#define CONFIG_MMC_0_USE_GPIO_IRQ   0
#define CONFIG_MMC_0_GPIO_IRQ_DEV   CONFIG_GPIO_A_NAME  /*CONFIG_GPIO_A_NAME&CONFIG_GPIO_B_NAME&CONFIG_GPIO_C_NAME*/
#define CONFIG_MMC_0_GPIO_IRQ_NUM   10    /*GPIOA10*/
#define CONFIG_MMC_0_GPIO_IRQ_FLAG  0    /*0=GPIO_ACTIVE_HIGH OR 1=GPIO_ACTIVE_LOW*/
#define CONFIG_MMC_0_ENABLE_SDIO_IRQ 0   /* If 1 to enable SD0 SDIO IRQ */


#define CONFIG_MMC_1_BUS_WIDTH      4
#define CONFIG_MMC_1_CLKSEL         0
#define CONFIG_MMC_1_DATA_REG_WIDTH 1
#define CONFIG_MMC_1_MFP
#define CONFIG_MMC_1_USE_GPIO_IRQ   0
#define CONFIG_MMC_1_ENABLE_SDIO_IRQ 0   /* If 1 to enable SD1 SDIO IRQ */

#define CONFIG_MMC_ACTS_ERROR_DETAIL    1 /* If 1 to print detail information when error occured */
#define CONFIG_MMC_WAIT_DAT1_BUSY       1 /* If 1 to wait SD/MMC card data 1 pin busy */
#define CONFIG_MMC_YIELD_WAIT_DMA_DONE  1 /* If  1 to yield task to wait DMA done */
#define CONFIG_MMC_SD0_FIFO_WIDTH_8BITS 0 /* If 1 to enable SD0 FIFO width 8 bits transfer */
#define CONFIG_MMC_STATE_FIFO           0 /* If 1 to enable using FIFO state for CPU read/write operations */

/*
sd board cfg
*/
#define CONFIG_SD_MMC_DEV           CONFIG_MMC_0_NAME /*CONFIG_MMC_0_NAME or CONFIG_MMC_1_NAME*/


/*
uart board cfg
*/
#define CONFIG_UART_0_SPEED     2000000
#define CONFIG_UART_1_SPEED     115200

/*
pwm board cfg
*/
#define CONFIG_PWM_CYCLE     8000


/*
I2C board cfg
*/
#define CONFIG_I2C_0_CLK_FREQ  100000
#define CONFIG_I2C_0_MAX_ASYNC_ITEMS 10

#define CONFIG_I2C_1_CLK_FREQ  100000
#define CONFIG_I2C_1_MAX_ASYNC_ITEMS 3

#define CONFIG_I2C_2_CLK_FREQ  100000
#define CONFIG_I2C_2_MAX_ASYNC_ITEMS 8

#define CONFIG_I2C_3_CLK_FREQ  100000
#define CONFIG_I2C_3_MAX_ASYNC_ITEMS 3

/*
SPI board cfg
*/

/*
I2CMT cfg
*/
#define CONFIG_I2CMT_0_CLK_FREQ  400000

#define CONFIG_I2CMT_1_CLK_FREQ  400000



/*
SPIMT cfg
*/

/*
tp board cfg
*/
#define CONFIG_TP_RESET_GPIO  1
#define CONFIG_TP_RESET_GPIO_NAME   CONFIG_GPIO_B_NAME
#define CONFIG_TP_RESET_GPIO_NUM    17
#define CONFIG_TP_RESET_GPIO_FLAG   GPIO_ACTIVE_LOW

/*
audio board cfg
*/

/**
 * The DAC working mode in dedicated PCB layout.
 * - 0: single-end(non-direct drive) mode
 * - 1: single-end(direct drive VRO) mode
 * - 2: differential mode
 */
#define CONFIG_AUDIO_DAC_0_LAYOUT               (2)

/* If 1 to enable DAC high performance which only works in differential layout */
#define CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN (1)

#if (CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN == 1)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_PW      (0xc8)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_SET     (0xe1)
#define CONFIG_AUDIO_DAC_HIGH_PERFORMANCE_SHCL_CURBIAS (0)
#endif

/**
 * The LEOPARD DAC PA gain setting as below:
 * The LEOPARD DAC PA gain setting as below:
 * PA gain    DARSET = 0    DARSET = 1
 *   7          2.72VPP       ------
 *   6	        2.23VPP	      ------
 *   5	        1.74VPP       ------
 *   4	        1.20VPP	      2.71VPP
 *   3	        0.98VPP	      2.22VPP
 *   2	        0.76VPP	      1.72VPP
 *   1	        0.54VPP	      1.23VPP
 *   0	        0.32VPP	      0.74VPP
 */
#define CONFIG_AUDIO_DAC_0_PA_VOL               (4)

/* Enable DAC left and right channels mix function. */
#define CONFIG_AUDIO_DAC_0_LR_MIX               (0)

/* Enable DAC SDM(noise detect mute) function. */
#define CONFIG_AUDIO_DAC_0_NOISE_DETECT_MUTE    (1)

/* SDM mute counter configuration. */
#define CONFIG_AUDIO_DAC_0_SDM_CNT              (0x1000)

/* SDM noise dectection threshold */
#define CONFIG_AUDIO_DAC_0_SDM_THRES            (0x800)

/* Enable DAC automute function when continuously output 512x(configurable) samples 0 data. */
#define CONFIG_AUDIO_DAC_0_AUTOMUTE             (0)

/* Enable ADC loopback to DAC function. */
#define CONFIG_AUDIO_DAC_0_LOOPBACK             (0)

/* If 1 to mute the DAC left channel. */
#define CONFIG_AUDIO_DAC_0_LEFT_MUTE            (0)

/* If 1 to mute the DAC right channel. */
#define CONFIG_AUDIO_DAC_0_RIGHT_MUTE           (0)

/* Auto mute counter configuration. */
#define CONFIG_AUDIO_DAC_0_AM_CNT               (0x1000)

/* Auto noise dectection threshold */
#define CONFIG_AUDIO_DAC_0_AM_THRES             (0)

/* If 1 to enable DAC automute IRQ function. */
#define CONFIG_AUDIO_DAC_0_AM_IRQ               (0)

/* The threshold to generate half empty IRQ signal. */
#define CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES      (0xE0)

/* The threshold to generate half full IRQ signal. */
#define CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES      (0xF0)

/* If 1 to open external PA when power on */
#define CONFIG_POWERON_OPEN_EXTERNAL_PA         (0)

/* If 1 to enable DAC power perfered  */
#define CONFIG_AUDIO_DAC_POWER_PREFERRED        (1)

/* If 1 to wait for writting PCMBUF completly */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH     (1)

/* The timeout out of writing PCMBUF in microsecond */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_TIMEOUT_US (1000000)

/* The sleep time in millisecond to of writing PCMBUF */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_SLEEP_MS   (0)

#if (CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_FINISH != 0)
/* Wait until next time writing pcmbuf to write previous one completely */
#define CONFIG_AUDIO_DAC_WAIT_WRITE_PCMBUF_NEXT_TIME  (1)
#endif

/********************************** I2STX CONFIGURATION **********************************/

/**
 * I2STX channel number selection.
 *   - 2: 2 channels
 *   - 4: 4 channels(TDM)
 *   - 8: 8 channels(TDM)
 */
#define CONFIG_AUDIO_I2STX_0_CHANNEL_NUM        (2)

/**
 * I2STX transfer format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 *   - 2: right-justified format
 *   - 3: TDM format
 */
#define CONFIG_AUDIO_I2STX_0_FORMAT             (0)

/**
 * I2STX BCLK data width.
 *   - 0: 32bits
 *   - 1: 16bits
 */
#define CONFIG_AUDIO_I2STX_0_BCLK_WIDTH         (0)

/* Enable the SRD(sample rate detect) function. */
#define CONFIG_AUDIO_I2STX_0_SRD_EN             (0)

/**
 * I2STX master or slaver mode selection.
 *   - 0: master
 *   - 1: slaver
 */
#define CONFIG_AUDIO_I2STX_0_MODE               (0)

/* Enable in slave mode MCLK to use internal clock. */
#define CONFIG_AUDIO_I2STX_0_SLAVE_INTERNAL_CLK (0)

/**
 * I2STX LRCLK process selection.
 *   - 0: 50% duty for I2S
 *   - 1: 1 BCLK
 */
#define CONFIG_AUDIO_I2STX_0_LRCLK_PROC         (0)

/**
 * I2STX MCLK reverse selection.
 *   - 0: normal
 *   - 1: reverse
 */
#define CONFIG_AUDIO_I2STX_0_MCLK_REVERSE       (0)

/* Enable I2STX channel BCLK/LRCLK alway existed which used in master mode. */
#define CONFIG_AUDIO_I2STX_0_ALWAYS_OPEN        (0)

/**
 * I2STX transfer TDM format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 */
#define CONFIG_AUDIO_I2STX_0_TDM_FORMAT         (0)

/**
 *  I2STX TDM frame start position selection.
 *   - 0: the rising edge of LRCLK with a pulse.
 *   - 1: the rising edge of LRCLK with a 50% duty cycle.
 *   - 2: the falling edge of LRCLK with a 50% duty cycle.
 */
#define CONFIG_AUDIO_I2STX_0_TDM_FRAME          (0)

/**
 * I2STX data output delay selection.
 *   - 0: 2 mclk cycles after the bclk rising edge.
 *   - 1: 3 mclk cycles after the bclk rising edge.
 *   - 2: 4 mclk cycles after the bclk rising edge.
 *   - 3: 5 mclk cycles after the bclk rising edge.
 */
#define CONFIG_AUDIO_I2STX_0_TX_DELAY           (0)

/**
*   PCM enable
*   selet pcm mode
*   0:disable pcm enable i2s
*   1:enable pcm disable i2s
*/
#define CONFIG_AUDIO_PCMTX_0_EN                    (0)

/**
*   first PCM enable
*   set  pcm format
*   0: short frame
*   1: long frame
*/
#define CONFIG_AUDIO_PCMTX_0_FORMART               (1)

/**
*   first PCM enable
*   set  pcm slot
*   0: slot 1
*   1: slot 2
*   2: slot 4
*   3: slot 8
*/
#define CONFIG_AUDIO_PCMTX_0_SLOT                    (1)


/********************************** SPDIFTX CONFIGURATION **********************************/

/* Enable the clock of SPDIFTX source from I2STX div2 clock. */
#define CONFIG_AUDIO_SPDIFTX_0_CLK_I2STX_DIV2       (0)


/********************************** ADC CONFIGURATION **********************************/
/* The end address of SRAM which used by DMA interleaved mode */
#define AUDIO_IN_DMA_RESERVED_ADDRESS           (0x02000000)



/**
 * ADC0 channel HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH0_HPF_TIME         (1)

/* ADC channel0 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH0_FREQUENCY        (0)

/* If 1 to enable ADC channel0 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH0_HPF_FC_HIGH      (0)

/**
 * ADC channel1 HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH1_HPF_TIME         (1)

/* ADC channel1 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH1_FREQUENCY        (0)

/* If 1 to enable ADC channel1 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH1_HPF_FC_HIGH      (0)

/**
 * ADC channel2 HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH2_HPF_TIME         (1)

/* ADC channel2 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH2_FREQUENCY        (0)

/* If 1 to enable ADC channel2 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH2_HPF_FC_HIGH      (0)

/**
 * ADC channel3 HPF auto-set time selection.
 *   - 0: 1.3ms in 48kfs
 *   - 1: 5ms in 48kfs
 *   - 2: 10ms in 48kfs
 *   - 3: 20ms in 48kfs
 */
#define CONFIG_AUDIO_ADC_0_CH3_HPF_TIME         (1)

/* ADC channel3 frequency which range from 0 ~ 111111b. */
#define CONFIG_AUDIO_ADC_0_CH3_FREQUENCY        (0)

/* If 1 to enable ADC channel3 HPF high frequency range. */
#define CONFIG_AUDIO_ADC_0_CH3_HPF_FC_HIGH      (0)

/**
 * Audio LDO output voltage selection.
 *   - 0: 1.6v
 *   - 1: 1.7v
 *   - 2: 1.8v
 *   - 3: 1.9v
 */
#define CONFIG_AUDIO_ADC_0_LDO_VOLTAGE          (1)

/*
 * Audio VMIC control MIC power as <vmic-ctl0, vmic-ctl1, vmic-ctl2>.
 *   - 0x: disable VMIC OP
 *   - 2: bypass VMIC OP
 *   - 3: enable VMIC OP
 */
#define CONFIG_AUDIO_ADC_0_VMIC_CTL_ARRAY       {3, 3, 3}

/**
 * Audio VMIC control the MIC voltage as <vmic-vol0, vmic-vol1>.
 *   - 0: 0.8 AVCC
 *   - 1: 0.85 AVCC
 *   - 2: 0.9 AVCC
 *   - 3: 0.95 AVCC
 */
#define CONFIG_AUDIO_ADC_0_VMIC_VOLTAGE_ARRAY   {2, 2, 2}

/* Enable ADC fast capacitor charge function. */
#define CONFIG_AUDIO_ADC_0_FAST_CAP_CHARGE      (0)

/**
 * ADC channels selection for AEC.
 *   - 0: select ADC0 for AEC
 *   - 1: select ADC1 for AEC
 */
#define CONFIG_AUDIO_ADC_0_AEC_SEL              (1)
/********************************** I2SRX CONFIGURATION **********************************/
/**
 * I2SRX channel number selection.
 *   - 2: 2 channels
 *   - 4: 4 channels(TDM)
 *   - 8: 8 channels(TDM)
 */
#define CONFIG_AUDIO_I2SRX_0_CHANNEL_NUM        (2)

/**
 * I2SRX transfer format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 *   - 2: right-justified format
 *   - 3: TDM format
 */
#define CONFIG_AUDIO_I2SRX_0_FORMAT             (0)

/**
 * I2SRX BCLK data width.
 *   - 0: 32bits
 *   - 1: 16bits
 */
#define CONFIG_AUDIO_I2SRX_0_BCLK_WIDTH         (0)

/* Enable the SRD(sample rate detect) function. */
#define CONFIG_AUDIO_I2SRX_0_SRD_EN             (1)

/**
 * I2SRX master or slaver mode selection.
 *   - 0: master
 *   - 1: slaver
 */
#define CONFIG_AUDIO_I2SRX_0_MODE               (0)

/* Enable in slave mode MCLK to use internal clock. */
#define CONFIG_AUDIO_I2SRX_0_SLAVE_INTERNAL_CLK    (0)

/**
 * I2STX LRCLK process selection.
 *   - 0: 50% duty for I2S
 *   - 1: 1 BCLK
 */
#define CONFIG_AUDIO_I2SRX_0_LRCLK_PROC         (0)

/**
 * I2SRX MCLK reverse selection.
 *   - 0: normal
 *   - 1: reverse
 */
#define CONFIG_AUDIO_I2SRX_0_MCLK_REVERSE       (0)

/**
 * I2SRX transfer TDM format selection.
 *   - 0: I2S format
 *   - 1: left-justified format
 */
#define CONFIG_AUDIO_I2SRX_0_TDM_FORMAT         (0)

/**
 *  I2SRX TDM frame start position selection.
 *   - 0: the rising edge of LRCLK with a pulse.
 *   - 1: the rising edge of LRCLK with a 50% duty cycle.
 *   - 2: the falling edge of LRCLK with a 50% duty cycle.
 */
#define CONFIG_AUDIO_I2SRX_0_TDM_FRAME          (0)

/* If 1 to enable the I2SRX clock source from I2STX. */
#define CONFIG_AUDIO_I2SRX_0_CLK_FROM_I2STX     (0)

/**
*   PCM enable
*   selet pcm mode
*   0:disable pcm enable i2s
*   1:enable pcm disable i2s
*/
#define CONFIG_AUDIO_PCMRX_0_EN                    (0)

/**
*   first PCM enable
*   set  pcm format
*   0: long frame
*   1: short frame
*/
#define CONFIG_AUDIO_PCMRX_0_FORMART               (1)

/**
*   first PCM enable
*   set  pcm slot
*   0: slot 1
*   1: slot 2
*   2: slot 4
*   3: slot 8
*/
#define CONFIG_AUDIO_PCMRX_0_SLOT                    (1)


/********************************** SPDIFRX CONFIGURATION **********************************/

/* Specify minimal CORE_PLL clock for spdifrx. */
#define CONFIG_AUDIO_SPDIFRX_0_MIN_COREPLL_CLOCK (50000000)

/*
 * dma2d lite cfg
 */
#define CONFIG_DMA2D_LITE_SDMA_CHAN  1

/*
 * jpeg hw cfg
 */
#define CONFIG_JPEG_HW_INPUT_SDMA_CHAN   2

#define CONFIG_JPEG_HW_OUTPUT_SDMA_CHAN  3

#define CONFIG_JPEG_HW_COUPLE_SDMA_CHAN  4

#define CONFIG_MEM_OPT_SDMA_CHAN  		4

/*
 * jpeg cfg
 */
/* jpeg clock speed */
#define CONFIG_JPEG_CLOCK_KHZ  (200000)

/*
 * GPU cfg
 */
/* GPU clock speed */
#define CONFIG_GPU_CLOCK_KHZ  (200000)

/*
 * DE cfg
 */
/* DE clock speed */
#define CONFIG_DISPLAY_ENGINE_CLOCK_KHZ  (200000)

/*
 * LCDC cfg
 */
/* LCDC y-flip mode enabled */
#define CONFIG_LCDC_Y_FLIP  0

/*
 * panel cfg
 */
#define CONFIG_PANEL_PORT_TYPE		PANEL_PORT_QSPI
#define CONFIG_PANEL_PORT_CS		(0)
#define CONFIG_PANEL_PORT_SPI_CPOL  (1)
#define CONFIG_PANEL_PORT_SPI_CPHA  (1)
#define CONFIG_PANEL_PORT_SPI_DUAL_LANE	(1)
/* Accepted values: 1, 2, 4, 8 */
#define CONFIG_PANEL_PORT_SPI_AHB_CLK_DIVISION (2)
/* X-Resolution */
#define CONFIG_PANEL_TIMING_HACTIVE	(466)
/* Y-Resolution */
#define CONFIG_PANEL_TIMING_VACTIVE	(466)
/* Pixel transfer clock rate in KHz */
#define CONFIG_PANEL_TIMING_PIXEL_CLK_KHZ (60000)
/* Refresh rate in Hz */
#define CONFIG_PANEL_TIMING_REFRESH_RATE_HZ (60)
/* TE signal exists */
#define CONFIG_PANEL_TIMING_TE_ACTIVE	(1)

//#define CONFIG_PANEL_BACKLIGHT_PWM		PWM_CFG_MAKE(CONFIG_PWM_NAME, 7, 255, 1)
//#define CONFIG_PANEL_BACKLIGHT_GPIO	GPIO_CFG_MAKE(CONFIG_GPIO_C_NAME, 0, GPIO_ACTIVE_HIGH, 1)

#define CONFIG_PANEL_BRIGHTNESS_DELAY_PERIODS (0)

/* brightness range [0, 255] */
#define CONFIG_PANEL_BRIGHTNESS		(255)
#define CONFIG_PANEL_AOD_BRIGHTNESS (128)
#define CONFIG_PANEL_TE_SCANLINE	(300)

/* fixed screen offset due to material or other issue */
#define CONFIG_PANEL_FIX_OFFSET_X (6)
#define CONFIG_PANEL_FIX_OFFSET_Y (0)
/* (logical) resolution area reported to user */
#define CONFIG_PANEL_HOR_RES	(CONFIG_PANEL_TIMING_HACTIVE)
#define CONFIG_PANEL_VER_RES	(CONFIG_PANEL_TIMING_VACTIVE)
#define CONFIG_PANEL_OFFSET_X	(0)
#define CONFIG_PANEL_OFFSET_Y	(0)
/* round panel */
#define CONFIG_PANEL_ROUND_SHAPE	(1)
/* ESD check period in milliseconds */
#define CONFIG_PANEL_ESD_CHECK_PERIOD 3000

/* Optimization:
 * At most 7 areas (3~7) will be posted for full screen refresh.
 *
 * Areas are defined as (x1, y1, x2, y2), and must be arraged from top to bottom.
 * Both their position and size must also be even.
 */
#if 1
#define CONFIG_PANEL_FULL_SCREEN_OPT_AREA \
	{ \
		{ 124 - CONFIG_PANEL_OFFSET_X,   0 - CONFIG_PANEL_OFFSET_Y, 341 - CONFIG_PANEL_OFFSET_X,  27 - CONFIG_PANEL_OFFSET_Y }, \
		{  68 - CONFIG_PANEL_OFFSET_X,  28 - CONFIG_PANEL_OFFSET_Y, 397 - CONFIG_PANEL_OFFSET_X,  67 - CONFIG_PANEL_OFFSET_Y }, \
		{  28 - CONFIG_PANEL_OFFSET_X,  68 - CONFIG_PANEL_OFFSET_Y, 437 - CONFIG_PANEL_OFFSET_X, 123 - CONFIG_PANEL_OFFSET_Y }, \
		{   0 - CONFIG_PANEL_OFFSET_X, 124 - CONFIG_PANEL_OFFSET_Y, 465 - CONFIG_PANEL_OFFSET_X, 341 - CONFIG_PANEL_OFFSET_Y }, \
		{  28 - CONFIG_PANEL_OFFSET_X, 342 - CONFIG_PANEL_OFFSET_Y, 437 - CONFIG_PANEL_OFFSET_X, 397 - CONFIG_PANEL_OFFSET_Y }, \
		{  68 - CONFIG_PANEL_OFFSET_X, 398 - CONFIG_PANEL_OFFSET_Y, 397 - CONFIG_PANEL_OFFSET_X, 437 - CONFIG_PANEL_OFFSET_Y }, \
		{ 124 - CONFIG_PANEL_OFFSET_X, 438 - CONFIG_PANEL_OFFSET_Y, 341 - CONFIG_PANEL_OFFSET_X, 465 - CONFIG_PANEL_OFFSET_Y }, \
	}
#else
#define CONFIG_PANEL_FULL_SCREEN_OPT_AREA \
	{ \
		{ 68 - CONFIG_PANEL_OFFSET_X,   0 - CONFIG_PANEL_OFFSET_Y, 397 - CONFIG_PANEL_OFFSET_X,  67 - CONFIG_PANEL_OFFSET_Y }, \
		{  0 - CONFIG_PANEL_OFFSET_X,  68 - CONFIG_PANEL_OFFSET_Y, 465 - CONFIG_PANEL_OFFSET_X, 397 - CONFIG_PANEL_OFFSET_Y }, \
		{ 68 - CONFIG_PANEL_OFFSET_X, 398 - CONFIG_PANEL_OFFSET_Y, 397 - CONFIG_PANEL_OFFSET_X, 465 - CONFIG_PANEL_OFFSET_Y }, \
	}
#endif

/*
 * tp cfg
 */
#define CONFIG_TPKEY_I2C_NAME		CONFIG_I2C_1_NAME
#define CONFIG_TPKEY_LOWPOWER		(1)


/*
PMU cfg
*/

/* If 1 to enable the ON-OFF key short press detection function. */
#define CONFIG_PMU_ONOFF_SHORT_DETECT            (1)

/* If 1 to indicates that ON-OFF key and REMOTE key use the same WIO */
#define CONFIG_PMU_ONOFF_REMOTE_SAME_WIO         (1)

/*
PMUADC cfg
*/
#define CONFIG_PMUADC_DEBOUNCE                   (1)

/** PMUADC battery channel over sampling counter
 *   - 0: disable over sampling
 *   - 1: 4 times
 *   - 2: 8 times
 *   - 3: 16 times
 */
#define CONFIG_PMUADC_BAT_AVG_CNT                (2)

/* If 1 to wait PMUADC AVG sample completely  */
#define CONFIG_PMUADC_BAT_WAIT_AVG_COMPLETE      (0)

/** PMUADC LRADC1 channel over sampling counter
 *   - 0: disable over sampling
 *   - 1: 4 times
 *   - 2: 8 times
 *   - 3: 16 times
 */
#define CONFIG_PMUADC_LRADC1_AVG                 (0)

/** PMUADC LRADC2 channel over sampling counter
 *   - 0: disable over sampling
 *   - 1: 4 times
 *   - 2: 8 times
 *   - 3: 16 times
 */
#define CONFIG_PMUADC_LRADC2_AVG                 (2)

/**
 * PMU ADC LRADC clock source selection.
 *   - 0: RC32K
 *   - 1: CK32K768
 *   - 2: RC4M/16
 *   - 3: HOSC/128
 */
#define CONFIG_PMUADC_CLOCK_SOURCE               (3)

/**
 * PMU ADC LRADC clock source divisor selection.
 *   - 0: /1
 *   - 1: /2
 *   - 2: /4
 *   - 3: /8
 */
#define CONFIG_PMUADC_CLOCK_DIV                  (0)

/**
 * PMU ADC previous buffer current BIAS selection.
 *   - 0: 0.25uA
 *   - 1: 0.5uA
 *   - 2: 0.75uA
 *   - 3: 1uA
 */
#define CONFIG_PMUADC_IBIAS_BUF_SEL              (0)

/**
 * PMU ADC core current BIAS selection.
 *   - 0: 0.25uA
 *   - 1: 0.5uA
 *   - 2: 0.75uA
 *   - 3: 1uA
 */
#define CONFIG_PMUADC_IBIAS_ADC_SEL              (1)

/* The timeout of sync counter8hz */
#define CONFIG_PMU_COUNTER8HZ_SYNC_TIMEOUT_US    (2000000)

/* If 1 to enable backup time when power off */
#define CONFIG_PM_BACKUP_TIME_FUNCTION_EN        (1)

#define CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME    "PM_BAK_TIME"

/*
ADCKEY cfg
*/
/* The time interval in millisecond to polling read the PMU ADC key. */
#define CONFIG_ADCKEY_POLL_INTERVAL_MS           (20)

/* The total time in millisecond to polling read the PMU ADC key. */
#define CONFIG_ADCKEY_POLL_TOTAL_MS              (1000)

/* The stable counter of sample filter. */
#define CONFIG_ADCKEY_SAMPLE_FILTER_CNT          (3)

/* The LRADC channel for ADC KEY */
#define CONFIG_ADCKEY_LRADC_CHAN                 (PMUADC_ID_LRADC3)

/*
ONOFFKEY cfg
*/
/*
 * The time threshold in millisecond to estimate the on-off key press is a long time pressed.
 *   - 0: 50ms < t < 0.125s is a short pressed key press; t >= 0.125s is a long pressed key.
 *   - 1: 50ms < t < 0.25s is a short pressed key; t >= 0.25s is a long pressed key.
 *   - 2: 50ms < t < 0.5s is a short pressed key press; t >= 0.5s is a long pressed key.
 *   - 3: 50ms < t < 1s is a short pressed key press; t >= 1s is a long pressed key.
 *   - 4: 50ms < t < 1.5s is a short pressed key press; t >= 1.5s is a long pressed key.
 *   - 5: 50ms < t < 2s is a short pressed key press; t >= 2s is a long pressed key.
 *   - 6: 50ms < t < 3s is a short pressed key press; t >= 3s is a long pressed key.
 *   - 7: 50ms < t < 4s is a short pressed key press; t >= 4s is a long pressed key.
 */
#define CONFIG_ONOFFKEY_LONG_PRESS_TIME                (3)

/*
 * ON-OFF key function selection.
 *   - 0: no function
 *   - 1: reset
 *   - 2: restart
 */
#define CONFIG_ONOFFKEY_FUNCTION                       (1)

/* The time interval in millisecond to polling read the PMU ADC key. */
#define CONFIG_ONOFFKEY_POLL_INTERVAL_MS               (20)

/* The total time in millisecond to polling onoff ADC key */
#define CONFIG_ONOFFKEY_POLL_TOTAL_MS                  (6000)

/* The stable counter for ONOFF KEY sample filter */
#define CONFIG_ONOFFKEY_SAMPLE_FILTER_CNT              (3)

/* The key code of ONOFF KEY which defined by user */
#define CONFIG_ONOFFKEY_USER_KEYCODE                   (1) /* KEY_POWER which reference to input_dev.h */

/*
GPIOKEY cfg
*/
/* The time interval in millisecond to polling read the GPIO key. */
#define CONFIG_GPIOKEY_POLL_INTERVAL_MS                (20)

/* The total time in millisecond to polling onoff GPIO key */
#define CONFIG_GPIOKEY_POLL_TOTAL_MS                   (6000)

/* The stable counter for GPIO KEY sample filter */
#define CONFIG_GPIOKEY_SAMPLE_FILTER_CNT               (3)

/* The voltage level when gpio key is pressed */
#define CONFIG_GPIOKEY_PRESSED_VOLTAGE_LEVEL           (1)

/* The key code of GPIO KEY which defined by user */
#define CONFIG_GPIOKEY_USER_KEYCODE                    (9) /* KEY_TBD which reference to input_dev.h */

/*
RTC cfg
*/
/**
 *  The RTC clock source selection.
 * - 0: RTC_CLKSRC_HOSC_4HZ
 * - 1: RTC_CLKSRC_LOSC_100HZ
 * - 2: RTC_CLKSRC_HCL_RC32K_100HZ
 */
#define CONFIG_RTC_CLK_SOURCE                    (2)

/**
 *  if 1 eanble The RTC clock calibration in power off
  RTC CONFIG_RTC_CLK_SOURCE=2  calibration can enable
 */
#define CONFIG_RTC_ENABLE_CALIBRATION              1

/*
Watchdog cfg
*/

/*
Battery cfg
*/

/* The time interval for battery voltage showing debug. */
#define CONFIG_BATTERY_DEBUG_INTERVAL_SEC        (60)

#ifdef CONFIG_ACTS_BATTERY_SUPPLY_EXT_COULOMETER
/* extern coulometer device name */
#define CONFIG_ACTS_EXT_COULOMETER_DEV_NAME     "coulometer"

/* extern coulometer use i2c device name */
#define CONFIG_COULOMETER_I2C_NAME		CONFIG_I2C_1_NAME

/* extern coulometer poll interval period ms */
#define CONFIG_COULOMETER_INTERVAL_MSEC        (1000)

#endif

#ifdef CONFIG_ACTS_LEOPARD_BATTERY_SUPPLY_EXTERNAL
/* extern charger use i2c device name */
#define CONFIG_EXT_CHARGER_I2C_NAME		CONFIG_I2C_2_NAME

#define CONFIG_EXT_CHARGER_ISR_GPIO		GPIO_CFG_MAKE(CONFIG_WIO_NAME, 1, GPIO_ACTIVE_LOW, 1)    // WIO1

#endif


#endif /* __BOARD_CFG_H */
