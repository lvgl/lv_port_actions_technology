/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_CONTROLLER_LCDC_LARK_H_
#define ZEPHYR_DRIVERS_DISPLAY_CONTROLLER_LCDC_LARK_H_

#include <zephyr/types.h>

/* Reg LCD_CTL */
#define LCD_EN                    BIT(0)
#define LCD_CLK_EN                BIT(3)
#define LCD_IF_SEL(x)             ((uint32_t)(x) << 1)
#define LCD_IF_SEL_MCU_8080      LCD_IF_SEL(0)
#define LCD_IF_SEL_MCU_6880      LCD_IF_SEL(1)
#define LCD_IF_SEL_SPI            LCD_IF_SEL(3)
#define LCD_IF_MASK               LCD_IF_SEL(3)

#define LCD_OUT_FORMAT_SEL(x)      ((uint32_t)(x) << 4)
#define LCD_OUT_FORMAT_RGB565      LCD_OUT_FORMAT_SEL(0)
#define LCD_OUT_FORMAT_RGB666      LCD_OUT_FORMAT_SEL(1)
#define LCD_OUT_FORMAT_RGB888      LCD_OUT_FORMAT_SEL(2)
#define LCD_OUT_FORMAT_MASK        LCD_OUT_FORMAT_SEL(3)

#define LCD_IF_MLS_SEL(x)         ((uint32_t)(x) << 8)
#define LCD_IF_MSB_FIRST          LCD_IF_MLS_SEL(0)
#define LCD_IF_LSB_FIRST          LCD_IF_MLS_SEL(1)

#define LCD_IF_CE_SEL(x)          ((uint32_t)(x) << 9)

#define LCD_TE_MODE_SEL(x)        ((uint32_t)(x) << 10)
#define LCD_TE_MODE_HIGH_LEVEL    LCD_TE_MODE_SEL(0)
#define LCD_TE_MODE_LOW_LEVEL     LCD_TE_MODE_SEL(1)
#define LCD_TE_MODE_RISING_EDGE   LCD_TE_MODE_SEL(2)
#define LCD_TE_MODE_FALLING_EDGE  LCD_TE_MODE_SEL(3)
#define LCD_TE_MODE_MASK          LCD_TE_MODE_SEL(3)

#define LCD_TE_EN                 BIT(12)

#define LCD_HOLD_EN              BIT(16)

/* Reg LCD_DISP_SIZE */
#define LCD_DISP_SIZE(w, h)       ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg LCD_CPU_CTL */
#define LCD_CPU_START              BIT(31)
#define LCD_CPU_FEND_IRQ_EN        BIT(30)

#define LCD_CPU_SDT_SEL(type, rb_swap) (((uint32_t)(type) << 15) | ((uint32_t)(rb_swap) << 13))
#define LCD_CPU_SDT_RGB565          LCD_SPI_SDT_SEL(0, 0)
#define LCD_CPU_SDT_BGR565          LCD_SPI_SDT_SEL(0, 1)
#define LCD_CPU_SDT_RGB666          LCD_SPI_SDT_SEL(1, 0)
#define LCD_CPU_SDT_BGR666          LCD_SPI_SDT_SEL(1, 1)
#define LCD_CPU_SDT_RGB888          LCD_SPI_SDT_SEL(2, 0)
#define LCD_CPU_SDT_BGR888          LCD_SPI_SDT_SEL(2, 1)
#define LCD_CPU_SDT_ARGB8888        LCD_SPI_SDT_SEL(3, 0)
#define LCD_CPU_SDT_ABGR8888        LCD_SPI_SDT_SEL(3, 1)
#define LCD_CPU_SDT_BGRA8888        LCD_SPI_SDT_SEL(4, 0)
#define LCD_CPU_SDT_RGBA8888        LCD_SPI_SDT_SEL(4, 1)
#define LCD_CPU_SDT_MASK            LCD_SPI_SDT_SEL(7, 1)

#define LCD_CPU_HWA_EN              BIT(14)

#define LCD_CPU_RX_DELAY_SEL(x)     ((uint32_t)(x) << 8)
#define LCD_CPU_RX_DELAY_MASK       LCD_CPU_RX_DELAY_SEL(1)

#define LCD_CPU_AHB_F565_SEL(x)     ((uint32_t)(x) << 5)
#define LCD_CPU_AHB_F565_16BIT      LCD_CPU_AHB_F565_SEL(1)
#define LCD_CPU_AHB_F565_24BIT      LCD_CPU_AHB_F565_SEL(0)
#define LCD_CPU_AHB_F565_MASK       LCD_CPU_AHB_F565_SEL(1)

#define LCD_CPU_RS(x)               ((uint32_t)(x) << 4)
#define LCD_CPU_RS_LOW              LCD_CPU_RS(0)
#define LCD_CPU_RS_HIGH             LCD_CPU_RS(1)
#define LCD_CPU_RS_MASK             LCD_CPU_RS(1)

#define LCD_CPU_SRC_SEL(x)          ((uint32_t)(x) << 2)
#define LCD_CPU_SRC_SEL_AHB         LCD_CPU_SRC_SEL(0)
#define LCD_CPU_SRC_SEL_DE          LCD_CPU_SRC_SEL(1)
#define LCD_CPU_SRC_SEL_DMA         LCD_CPU_SRC_SEL(2)
#define LCD_CPU_SRC_MASK            LCD_CPU_SRC_SEL(3)

#define LCD_CPU_AHB_DATA_SEL(x)       ((uint32_t)(x) << 1)
#define LCD_CPU_AHB_DATA_SEL_IMG      LCD_CPU_AHB_DATA_SEL(0)
#define LCD_CPU_AHB_DATA_SEL_CFG      LCD_CPU_AHB_DATA_SEL(1)
#define LCD_CPU_AHB_DATA_MASK         LCD_CPU_AHB_DATA_SEL(1)

#define LCD_CPU_AHB_CSX(x)            ((uint32_t)(x) << 0)
#define LCD_CPU_AHB_CSX_MASK          LCD_CPU_AHB_CSX(1)

/* Reg LCD_CPU_CLK */
#define LCD_CPU_CLK(hdu, ldu, ldu2) \
	(((uint32_t)(((ldu2) - 1) & 0x3f) << 16) |  \
	((uint32_t)(((ldu) - 1) & 0x3f) << 8) | (((hdu) - 1) & 0x3f))

/* Reg LCD_SPI_CTL */
#define LCD_SPI_TYPE_SEL(x)           ((x))
#define LCD_SPI_3WIRE_TYPE1           LCD_SPI_TYPE_SEL(0)
#define LCD_SPI_3WIRE_TYPE2           LCD_SPI_TYPE_SEL(1)
#define LCD_SPI_4WIRE_TYPE1           LCD_SPI_TYPE_SEL(2)
#define LCD_SPI_4WIRE_TYPE2           LCD_SPI_TYPE_SEL(3)
#define LCD_QSPI                      LCD_SPI_TYPE_SEL(4)
#define LCD_QSPI_SYNC                 LCD_SPI_TYPE_SEL(5)
#define LCD_SPI_TYPE_MASK             LCD_SPI_TYPE_SEL(7)

#define LCD_SPI_CDX(x)                ((uint32_t)(x) << 3)
#define LCD_SPI_CDX_MASK              LCD_SPI_CDX(0x1)

#define LCD_SPI_AHB_CSX(x)            ((uint32_t)(x) << 5)
#define LCD_SPI_AHB_CSX_MASK          LCD_SPI_AHB_CSX(1)

/* sample edge: 0-rising edge; 1-falling edge */
#define LCD_SPI_SCLK_POL(x)           ((uint32_t)((x)) << 6)

#define LCD_SPI_DCP_SEL(x)            ((uint32_t)(x) << 7)
#define LCD_SPI_DCP_SEL_NORMAL        LCD_SPI_DCP_SEL(0)
#define LCD_SPI_DCP_SEL_COMPAT        LCD_SPI_DCP_SEL(1)
#define LCD_SPI_DCP_MASK              LCD_SPI_DCP_SEL(1)

#define LCD_SPI_RDLC_SEL(x)           ((uint32_t)((x) & 0x3) << 8)
#define LCD_SPI_RDLC_MASK             LCD_SPI_RDLC_SEL(0x3)

#define LCD_SPI_SRC_SEL(x)            ((uint32_t)((x) & 0x3) << 10)
#define LCD_SPI_SRC_SEL_AHB           LCD_SPI_SRC_SEL(0)
#define LCD_SPI_SRC_SEL_DE            LCD_SPI_SRC_SEL(1)
#define LCD_SPI_SRC_SEL_DMA           LCD_SPI_SRC_SEL(2)
#define LCD_SPI_SRC_MASK              LCD_SPI_SRC_SEL(3)

#define LCD_SPI_DELAY_CHAIN_SEL(x)    ((uint32_t)((x) & 0xf) << 12)
#define LCD_SPI_DELAY_CHAIN_MASK      LCD_SI_DELAY_CHAIN_SEL(0xf)

#define LCD_SPI_DUAL_LANE_SEL(x)      ((uint32_t)(x) << 16)
#define LCD_SPI_DUAL_LANE_SEL_SINGLE  LCD_SPI_DUAL_LANE_SEL(0)
#define LCD_SPI_DUAL_LANE_SEL_DUAL    LCD_SPI_DUAL_LANE_SEL(1)
#define LCD_SPI_DUAL_LANE_MASK        LCD_SPI_DUAL_LANE_SEL(1)

#define LCD_SPI_AHB_F565_SEL(x)       ((uint32_t)(x) << 17)
#define LCD_SPI_AHB_F565_16BIT        LCD_SPI_AHB_F565_SEL(1)
#define LCD_SPI_AHB_F565_24BIT        LCD_SPI_AHB_F565_SEL(0)
#define LCD_SPI_AHB_F565_MASK         LCD_SPI_AHB_F565_SEL(1)

#define LCD_SPI_RWL(x)                ((uint32_t)(x) << 18)
#define LCD_SPI_RWL_MASK              LCD_SPI_RWL(0x3f)

#define LCD_SPI_SDT_SEL(type, rb_swap) (((uint32_t)(type) << 24) | ((uint32_t)(rb_swap) << 4))
#define LCD_SPI_SDT_RGB565          LCD_SPI_SDT_SEL(0, 0)
#define LCD_SPI_SDT_BGR565          LCD_SPI_SDT_SEL(0, 1)
#define LCD_SPI_SDT_RGB666          LCD_SPI_SDT_SEL(1, 0)
#define LCD_SPI_SDT_BGR666          LCD_SPI_SDT_SEL(1, 1)
#define LCD_SPI_SDT_RGB888          LCD_SPI_SDT_SEL(2, 0)
#define LCD_SPI_SDT_BGR888          LCD_SPI_SDT_SEL(2, 1)
#define LCD_SPI_SDT_ARGB8888        LCD_SPI_SDT_SEL(3, 0)
#define LCD_SPI_SDT_ABGR8888        LCD_SPI_SDT_SEL(3, 1)
#define LCD_SPI_SDT_BGRA8888        LCD_SPI_SDT_SEL(4, 0)
#define LCD_SPI_SDT_RGBA8888        LCD_SPI_SDT_SEL(4, 1)
#define LCD_SPI_SDT_MASK            LCD_SPI_SDT_SEL(7, 1)

#define LCD_SPI_HWA_EN                BIT(27)
#define LCD_SPI_AHB_CFG_DATA          BIT(28)
#define LCD_SPI_FEND_PD_EN            BIT(29)
#define LCD_SPI_FTC_IRQ_EN            BIT(30)
#define LCD_SPI_START                 BIT(31)

/* Reg LCD_QSPI_CMD{1} */
#define LCD_QSPI_CMD(cmd, addr)       (((uint32_t)(cmd) << 24) | ((addr) & 0xffffff))

/* Reg LCD_QSI_QSYNC_TIM (vfp and vbp have inverse meanings with the normal lcd RGB timing) */
#define LCD_QSPI_SYNC_TIM(hp, vfp, vbp) \
		(((uint32_t)((hp) & 0xffff) << 16) |  \
		((uint32_t)(((vbp) - 1) & 0xff) << 8) | (((vfp) - 1) & 0xff))

/* Reg LCD_QSPI_IMG_SIZE */
#define LCD_QSPI_IMG_SIZE(w, h)       ((((uint32_t)(h) - 1) << 16) | ((w) - 1))

/* Reg LCD_QSPI_DTAS */
#define LCD_QSPI_DTAS(clk_en_in_delay, delay_cycles) \
		((((uint32_t)clk_en_in_delay) << 16) | ((delay_cycles) & 0xffff))

/* Reg DE_INTERFACE_CTL */
#define LCD_DE_CTL_UNIT(x)    (((uint32_t)(x) & 0x3FF) << 20)
#define LCD_DE_VFP(x)         (((uint32_t)(x) & 0x3FF) << 10)
#define LCD_DE_VSW(x)         (((uint32_t)(x) & 0x3FF) << 0)

/* Reg LCD_PENDING */
#define LCD_STAT_QSPI_SYNC_FTC BIT(5)
#define LCD_STAT_TE            BIT(4)
#define LCD_STAT_SPI_FTC       BIT(3)
#define LCD_STAT_CPU_FTC       BIT(2)
#define LCD_STAT_FTC          (LCD_STAT_QSPI_SYNC_FTC | LCD_STAT_SPI_FTC | LCD_STAT_CPU_FTC)

/**
  * @brief LCDC Module (LCDC)
  */
typedef struct {                          /*!< (@ 0x40064000) LCDC Structure                        */
	volatile uint32_t CTL;                /*!< (@ 0x00000000) LCD Control Register                  */
	const volatile uint32_t RESERVED1;
	volatile uint32_t DISP_SIZE;          /*!< (@ 0x00000008) LCD DMA Source Display Size Register  */
	const volatile uint32_t RESERVED2[4];
	volatile uint32_t CPU_CTL;            /*!< (@ 0x0000001C) LCD CPU Mode Control Register         */
	volatile uint32_t DATA;               /*!< (@ 0x00000020) LCD Data Register                     */
	volatile uint32_t CPU_CLK;            /*!< (@ 0x00000024) LCD CPU Mode Clk Control Register     */
	volatile uint32_t TPL;                /*!< (@ 0x00000028) LCD Transfer Pixel Length Register    */
	volatile uint32_t SPI_CTL;            /*!< (@ 0x0000002C) LCD Serial Interface Control Register */
	const volatile uint32_t RESERVED3;
	volatile uint32_t QSPI_CMD;           /*!< (@ 0x00000034) LCD Quad Serial Command Register      */
	volatile uint32_t QSPI_CMD1;          /*!< (@ 0x00000038) LCD Quad Serial Command Register      */
	volatile uint32_t QSPI_SYNC_TIM;      /*!< (@ 0x0000003C) LCD Quad SPI Sync Timing Register     */
	volatile uint32_t QSPI_IMG_SIZE;      /*!< (@ 0x00000040) LCD QSPI Image Size Register          */
	volatile uint32_t DE_INTERFACE_CTL;   /*!< (@ 0x00000044) DE Interface Control Register         */
	volatile uint32_t PENDING;            /*!< (@ 0x00000048) LCD Pending Register                  */
	volatile uint32_t QSPI_DTAS;          /*!< (@ 0x0000004C) QSPI_SYNC interface control register  */
	volatile uint32_t DATA_1[7];          /*!< (@ 0x00000050) LCD Data Register                     */
} LCDC_Type;                              /*!< Size = 104 (0x68)                                    */

/**
  * @brief DMA Channel Config Module (DMA_CHANCFG_Type)
  */
typedef struct {                          /*!< DMA Channel Config Structure                         */
	volatile uint32_t CTL;               /*!< (@ 0x00000000) Control Register                      */
	volatile uint32_t START;             /*!< (@ 0x00000004) Start Register                        */
	volatile uint32_t SADDR0;            /*!< (@ 0x00000008) Source Address 0 Register             */
	volatile uint32_t SADDR1;            /*!< (@ 0x0000000C) Source Address 1 Register             */
	volatile uint32_t DADDR0;            /*!< (@ 0x00000010) Destination Address 0 Register        */
	volatile uint32_t DADDR1;            /*!< (@ 0x00000014) Destination Address 0 Register        */
	volatile uint32_t BC;                /*!< (@ 0x00000018) Byte Counter Register                 */
	volatile uint32_t RC;                /*!< (@ 0x0000001C) Remain Counter Register               */
} LCDC_DMACHAN_CTL_Type;                 /*!< Size = 32 (0x20)                                         */

/**
  * @brief DMA Line Config Module (DMA_LINECFG_Type)
  */
typedef struct {                           /*!< DMA Line Config Structure                                */
	volatile uint32_t LENGTH;              /*!< (@ 0x00000000) Line Length Register                      */
	volatile uint32_t COUNT;               /*!< (@ 0x00000004) Line Count Register                       */
	volatile uint32_t STRIDE;              /*!< (@ 0x00000008) Line Stride Register                      */
	volatile uint32_t REMAIN;              /*!< (@ 0x0000000C) Line Remain Register                      */
	volatile uint32_t BYTE_REMAIN_IN_LINE; /*!< (@ 0x00000010) Byte Remain in Transmitting Line Register */
} LCDC_DMALINE_CTL_Type;                   /*!< Size = 20 (0x14)                                         */

#endif /* ZEPHYR_DRIVERS_DISPLAY_CONTROLLER_LCDC_LARK_H_ */
