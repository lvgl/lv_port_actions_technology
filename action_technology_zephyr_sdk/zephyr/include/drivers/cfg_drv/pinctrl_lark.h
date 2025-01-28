/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_LARK_H_
#define PINCTRL_LARK_H_

#define PIN_MFP_SET(pin, val)				\
	{.pin_num = pin,		\
	 .mode = val}


#define GPIO_CTL_MFP_SHIFT				(0)
#define GPIO_CTL_MFP_MASK				(0x1f << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP_GPIO				(0x0 << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP(x)					(x << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_SMIT					(0x1 << 5)
#define GPIO_CTL_GPIO_OUTEN				(0x1 << 6)
#define GPIO_CTL_GPIO_INEN				(0x1 << 7)
#define GPIO_CTL_PULL_MASK				(0xf << 8)
#define GPIO_CTL_PULLUP_STRONG			(0x1 << 8)
#define GPIO_CTL_PULLDOWN				(0x1 << 9)
#define GPIO_CTL_PULLUP					(0x1 << 11)
#define GPIO_CTL_PADDRV_SHIFT			(12)
#define GPIO_CTL_PADDRV_LEVEL(x)		((x) << GPIO_CTL_PADDRV_SHIFT)
#define GPIO_CTL_PADDRV_MASK			GPIO_CTL_PADDRV_LEVEL(0x7)
#define GPIO_CTL_INTC_EN				(0x1 << 20)
#define GPIO_CTL_INC_TRIGGER_SHIFT			(21)
#define GPIO_CTL_INC_TRIGGER(x)				((x) << GPIO_CTL_INC_TRIGGER_SHIFT)
#define GPIO_CTL_INC_TRIGGER_MASK			GPIO_CTL_INC_TRIGGER(0x7)
#define GPIO_CTL_INC_TRIGGER_RISING_EDGE	GPIO_CTL_INC_TRIGGER(0x0)
#define GPIO_CTL_INC_TRIGGER_FALLING_EDGE	GPIO_CTL_INC_TRIGGER(0x1)
#define GPIO_CTL_INC_TRIGGER_DUAL_EDGE		GPIO_CTL_INC_TRIGGER(0x2)
#define GPIO_CTL_INC_TRIGGER_HIGH_LEVEL		GPIO_CTL_INC_TRIGGER(0x3)
#define GPIO_CTL_INC_TRIGGER_LOW_LEVEL		GPIO_CTL_INC_TRIGGER(0x4)
#define GPIO_CTL_INTC_MASK					(0x1 << 25)


/*********I2C mfp config*****************/
#define MFP0_I2C	8
#define MFP1_I2C	9
#define I2C_MFP_CFG(x)	(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(3))


#define gpio22_i2c0_clk_node	PIN_MFP_SET(22, I2C_MFP_CFG(MFP0_I2C))
#define gpio23_i2c0_data_node	PIN_MFP_SET(23, I2C_MFP_CFG(MFP0_I2C))

#define gpio24_i2c0_clk_node	PIN_MFP_SET(24, I2C_MFP_CFG(MFP0_I2C))
#define gpio25_i2c0_data_node	PIN_MFP_SET(25, I2C_MFP_CFG(MFP0_I2C))


#define gpio28_i2c0_clk_node	PIN_MFP_SET(28, I2C_MFP_CFG(MFP0_I2C))
#define gpio29_i2c0_data_node	PIN_MFP_SET(29, I2C_MFP_CFG(MFP0_I2C))


#define gpio20_i2c1_clk_node	PIN_MFP_SET(20, I2C_MFP_CFG(MFP1_I2C))
#define gpio21_i2c1_data_node	PIN_MFP_SET(21, I2C_MFP_CFG(MFP1_I2C))

#define gpio18_i2c1_clk_node	PIN_MFP_SET(18, I2C_MFP_CFG(MFP1_I2C))
#define gpio19_i2c1_data_node	PIN_MFP_SET(19, I2C_MFP_CFG(MFP1_I2C))

#define gpio28_i2c1_clk_node	PIN_MFP_SET(28, I2C_MFP_CFG(MFP1_I2C))
#define gpio29_i2c1_data_node	PIN_MFP_SET(29, I2C_MFP_CFG(MFP1_I2C))

#define gpio57_i2c0_clk_node	PIN_MFP_SET(57, I2C_MFP_CFG(MFP0_I2C))
#define gpio58_i2c0_data_node	PIN_MFP_SET(58, I2C_MFP_CFG(MFP0_I2C))

#define gpio59_i2c1_clk_node	PIN_MFP_SET(59, I2C_MFP_CFG(MFP1_I2C))
#define gpio60_i2c1_data_node	PIN_MFP_SET(60, I2C_MFP_CFG(MFP1_I2C))

/*********I2CMT mfp config*****************/
#define MFP0_I2CMT	26
#define MFP1_I2CMT	27
#define I2CMT_MFP_CFG(x)	(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))

#define gpio22_i2cmt0_clk_node	PIN_MFP_SET(22, I2CMT_MFP_CFG(MFP0_I2CMT))
#define gpio23_i2cmt0_data_node	PIN_MFP_SET(23, I2CMT_MFP_CFG(MFP0_I2CMT))

#define gpio24_i2cmt0_clk_node	PIN_MFP_SET(24, I2CMT_MFP_CFG(MFP0_I2CMT))
#define gpio25_i2cmt0_data_node	PIN_MFP_SET(25, I2CMT_MFP_CFG(MFP0_I2CMT))

#define gpio49_i2cmt0_clk_node	PIN_MFP_SET(49, I2CMT_MFP_CFG(MFP0_I2CMT))
#define gpio50_i2cmt0_data_node	PIN_MFP_SET(50, I2CMT_MFP_CFG(MFP0_I2CMT))

#define gpio51_i2cmt1_clk_node	PIN_MFP_SET(51,  I2CMT_MFP_CFG(MFP1_I2CMT))
#define gpio52_i2cmt1_data_node	PIN_MFP_SET(52,  I2CMT_MFP_CFG(MFP1_I2CMT))

#define gpio59_i2cmt1_clk_node	PIN_MFP_SET(59,  I2CMT_MFP_CFG(MFP1_I2CMT))
#define gpio60_i2cmt1_data_node	PIN_MFP_SET(60,  I2CMT_MFP_CFG(MFP1_I2CMT))

/*********SPI mfp config*****************/
#define MFP_SPI0	6
#define MFP_SPI0_1	7
#define MFP_SPI1	2
#define MFP_SPI2	3
#define MFP_SPI3	4

#define SPI_MFP_CFG(x)	(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))
#define SPI2_MFP_CFG(x)	(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(5))

#define gpio24_spi1_ss_node		PIN_MFP_SET(24,  SPI_MFP_CFG(MFP_SPI1))
#define gpio25_spi1_clk_node	PIN_MFP_SET(25,  SPI_MFP_CFG(MFP_SPI1))
#define gpio26_spi1_miso_node	PIN_MFP_SET(26,  SPI_MFP_CFG(MFP_SPI1))
#define gpio27_spi1_mosi_node	PIN_MFP_SET(27,  SPI_MFP_CFG(MFP_SPI1))


#define gpio30_spi2_ss_node		PIN_MFP_SET(30,  SPI2_MFP_CFG(MFP_SPI2))
#define gpio31_spi2_clk_node	PIN_MFP_SET(31,  SPI2_MFP_CFG(MFP_SPI2))
#define gpio32_spi2_miso_node	PIN_MFP_SET(32,  SPI2_MFP_CFG(MFP_SPI2))
#define gpio33_spi2_mosi_node	PIN_MFP_SET(33,  SPI2_MFP_CFG(MFP_SPI2))

#define gpio20_spi3_ss_node		PIN_MFP_SET(20,  SPI2_MFP_CFG(MFP_SPI3))
#define gpio21_spi3_clk_node	PIN_MFP_SET(21,  SPI2_MFP_CFG(MFP_SPI3))
#define gpio22_spi3_miso_node	PIN_MFP_SET(22,  SPI2_MFP_CFG(MFP_SPI3))
#define gpio23_spi3_mosi_node	PIN_MFP_SET(23,  SPI2_MFP_CFG(MFP_SPI3))

/*********SPIMT mfp config*****************/
#define MFP0_SPIMT	24
#define MFP1_SPIMT	25

#define SPIMT_MFP_CFG(x)	(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(3))

#define gpio49_spimt0_ss_node	PIN_MFP_SET(49,  SPIMT_MFP_CFG(MFP0_SPIMT))
#define gpio50_spimt0_clk_node	PIN_MFP_SET(50,  SPIMT_MFP_CFG(MFP0_SPIMT))
#define gpio51_spimt0_miso_node	PIN_MFP_SET(51,  SPIMT_MFP_CFG(MFP0_SPIMT))
#define gpio52_spimt0_mosi_node	PIN_MFP_SET(52,  SPIMT_MFP_CFG(MFP0_SPIMT))
#define gpio61_spimt0_ss1_node	PIN_MFP_SET(61,  SPIMT_MFP_CFG(MFP0_SPIMT))

#define gpio53_spimt1_ss_node	PIN_MFP_SET(53,  SPIMT_MFP_CFG(MFP1_SPIMT))
#define gpio54_spimt1_clk_node	PIN_MFP_SET(54,  SPIMT_MFP_CFG(MFP1_SPIMT))
#define gpio55_spimt1_miso_node	PIN_MFP_SET(55,  SPIMT_MFP_CFG(MFP1_SPIMT))
#define gpio56_spimt1_mosi_node	PIN_MFP_SET(56,  SPIMT_MFP_CFG(MFP1_SPIMT))

/* lcdc */
#ifndef LCD_PADDRV_LEVEL
#  define LCD_PADDRV_LEVEL (3)
#endif
#define LCD_MFP_SEL                  (19 | GPIO_CTL_PADDRV_LEVEL(LCD_PADDRV_LEVEL))
#define gpio30_lcd_ce0_hsync_node    PIN_MFP_SET(30, (GPIO_CTL_MFP(LCD_MFP_SEL)|GPIO_CTL_PULLUP))
#define gpio31_lcd_ce1_node          PIN_MFP_SET(31, (GPIO_CTL_MFP(LCD_MFP_SEL)|GPIO_CTL_PULLUP))
#define gpio32_lcd_rs_vsync_sda_node PIN_MFP_SET(32, LCD_MFP_SEL)
#define gpio33_lcd_rde_lde_cdx_node  PIN_MFP_SET(33, LCD_MFP_SEL)
#define gpio34_lcd_wr_dclk_scl_node  PIN_MFP_SET(34, LCD_MFP_SEL)
#define gpio35_lcd_te_sdo_node       PIN_MFP_SET(35, LCD_MFP_SEL)

#define gpio14_lcd_d0_node           PIN_MFP_SET(14, LCD_MFP_SEL)
#define gpio15_lcd_d1_node           PIN_MFP_SET(15, LCD_MFP_SEL)
#define gpio16_lcd_d2_node           PIN_MFP_SET(16, LCD_MFP_SEL)
#define gpio17_lcd_d3_node           PIN_MFP_SET(17, LCD_MFP_SEL)
#define gpio18_lcd_d4_node           PIN_MFP_SET(18, LCD_MFP_SEL)
#define gpio19_lcd_d5_node           PIN_MFP_SET(19, LCD_MFP_SEL)
#define gpio20_lcd_d6_node           PIN_MFP_SET(20, LCD_MFP_SEL)
#define gpio21_lcd_d7_node           PIN_MFP_SET(21, LCD_MFP_SEL)

#define gpio14_lcd_ce0_hsync_node    PIN_MFP_SET(14,   (GPIO_CTL_MFP(LCD_MFP_SEL)|GPIO_CTL_PULLUP))
#define gpio15_lcd_ce1_node          PIN_MFP_SET(15,   LCD_MFP_SEL)
#define gpio16_lcd_d8_node           PIN_MFP_SET(16,   LCD_MFP_SEL)
#define gpio17_lcd_d9_node           PIN_MFP_SET(17,   LCD_MFP_SEL)
#define gpio18_lcd_d10_node          PIN_MFP_SET(18,   LCD_MFP_SEL)
#define gpio19_lcd_d11_node          PIN_MFP_SET(19,   LCD_MFP_SEL)
#define gpio20_lcd_d12_node          PIN_MFP_SET(20,   LCD_MFP_SEL)
#define gpio21_lcd_d13_node          PIN_MFP_SET(21,   LCD_MFP_SEL)
#define gpio22_lcd_d14_node          PIN_MFP_SET(22,   LCD_MFP_SEL)
#define gpio23_lcd_d15_node          PIN_MFP_SET(23,   LCD_MFP_SEL)
#define gpio24_lcd_d0_node           PIN_MFP_SET(24,   LCD_MFP_SEL)
#define gpio25_lcd_d1_node           PIN_MFP_SET(25,   LCD_MFP_SEL)
#define gpio26_lcd_d2_node           PIN_MFP_SET(26,   LCD_MFP_SEL)
#define gpio27_lcd_d3_node           PIN_MFP_SET(27,   LCD_MFP_SEL)
#define gpio28_lcd_d4_node           PIN_MFP_SET(28,   LCD_MFP_SEL)
#define gpio29_lcd_d5_node           PIN_MFP_SET(29,   LCD_MFP_SEL)
#define gpio30_lcd_d6_node           PIN_MFP_SET(30,   LCD_MFP_SEL)
#define gpio31_lcd_d7_node           PIN_MFP_SET(31,   LCD_MFP_SEL)
#define gpio32_lcd_rs_vsync_sda_node PIN_MFP_SET(32,   LCD_MFP_SEL)
#define gpio33_lcd_rde_lde_cdx_node  PIN_MFP_SET(33,   LCD_MFP_SEL)
#define gpio34_lcd_wr_dclk_scl_node  PIN_MFP_SET(34,   LCD_MFP_SEL)
#define gpio35_lcd_te_sdo_node       PIN_MFP_SET(35,   LCD_MFP_SEL)
/* lcdc fpga 1.8v*/
#define gpio30_lcd_ce0_hsync_node    PIN_MFP_SET(30,  (GPIO_CTL_MFP(LCD_MFP_SEL)|GPIO_CTL_PULLUP))
#define gpio34_lcd_wr_dclk_scl_node  PIN_MFP_SET(34,  LCD_MFP_SEL)
#define gpio14_lcd_d0_node           PIN_MFP_SET(14,  LCD_MFP_SEL)
#define gpio15_lcd_d1_node           PIN_MFP_SET(15,  LCD_MFP_SEL)
#define gpio16_lcd_d2_node           PIN_MFP_SET(16,  LCD_MFP_SEL)
#define gpio17_lcd_d3_node           PIN_MFP_SET(17,  LCD_MFP_SEL)
#define gpio18_lcd_d4_node           PIN_MFP_SET(18,  LCD_MFP_SEL)
#define gpio19_lcd_d5_node           PIN_MFP_SET(19,  LCD_MFP_SEL)
#define gpio20_lcd_d6_node           PIN_MFP_SET(20,  LCD_MFP_SEL)
#define gpio21_lcd_d7_node           PIN_MFP_SET(21,  LCD_MFP_SEL)

//#define gpio24_lcd_d10_node          PIN_MFP_SET(24, LCD_MFP_SEL)
//#define gpio25_lcd_d11_node          PIN_MFP_SET(25, LCD_MFP_SEL)
//#define gpio26_lcd_d12_node          PIN_MFP_SET(26, LCD_MFP_SEL)
//#define gpio27_lcd_d13_node          PIN_MFP_SET(27, LCD_MFP_SEL)
//#define gpio28_lcd_d14_node          PIN_MFP_SET(28, LCD_MFP_SEL)
//#define gpio29_lcd_d15_node          PIN_MFP_SET(29, LCD_MFP_SEL)

/* sd0 */
#define SDC0_MFP_SEL             10
#define SDC0_MFP_CFG_VAL	(GPIO_CTL_MFP(SDC0_MFP_SEL)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(5))

//#define gpio0_sdc0_cmd_node  PIN_MFP_SET(0, sdc0, cmd,  SDC0_MFP_CFG_VAL)
//#define gpio1_sdc0_clk_node  PIN_MFP_SET(1, sdc0, clk,  (GPIO_CTL_MFP(SDC0_MFP_SEL)|GPIO_CTL_PADDRV_LEVEL(3)))
//#define gpio2_sdc0_d0_node   PIN_MFP_SET(2, sdc0, d0,   SDC0_MFP_CFG_VAL)
//#define gpio3_sdc0_d1_node   PIN_MFP_SET(3, sdc0, d1,   SDC0_MFP_CFG_VAL)
//#define gpio4_sdc0_d2_node   PIN_MFP_SET(4, sdc0, d2,   SDC0_MFP_CFG_VAL)
//#define gpio5_sdc0_d3_node   PIN_MFP_SET(5, sdc0, d3,   SDC0_MFP_CFG_VAL)
//#define gpio6_sdc0_d4_node   PIN_MFP_SET(6, sdc0, d4,   SDC0_MFP_CFG_VAL)
//#define gpio7_sdc0_d5_node   PIN_MFP_SET(7, sdc0, d5,   SDC0_MFP_CFG_VAL)
//#define gpio8_sdc0_d6_node   PIN_MFP_SET(8, sdc0, d6,   SDC0_MFP_CFG_VAL)
//#define gpio9_sdc0_d7_node   PIN_MFP_SET(9, sdc0, d7,   SDC0_MFP_CFG_VAL)

#define gpio11_sdc0_cmd_node  PIN_MFP_SET(11,   SDC0_MFP_CFG_VAL)
#define gpio10_sdc0_clk_node  PIN_MFP_SET(10,  (GPIO_CTL_MFP(SDC0_MFP_SEL)|GPIO_CTL_PADDRV_LEVEL(3)))
#define gpio12_sdc0_d0_node   PIN_MFP_SET(12,   SDC0_MFP_CFG_VAL)
#define gpio13_sdc0_d1_node   PIN_MFP_SET(13,    SDC0_MFP_CFG_VAL)
#define gpio8_sdc0_d2_node   PIN_MFP_SET(8,    SDC0_MFP_CFG_VAL)
#define gpio9_sdc0_d3_node   PIN_MFP_SET(9,    SDC0_MFP_CFG_VAL)

/* sd1 */
#define SDC1_MFP_SEL             10
#define SDC1_MFP_CFG_VAL	(GPIO_CTL_MFP(SDC1_MFP_SEL)|GPIO_CTL_PULLUP|GPIO_CTL_PADDRV_LEVEL(5))

#define gpio44_sdc1_cmd_node  PIN_MFP_SET(44,   SDC1_MFP_SEL)
#define gpio43_sdc1_clk_node  PIN_MFP_SET(43,   (GPIO_CTL_MFP(SDC1_MFP_SEL)|GPIO_CTL_PADDRV_LEVEL(3)))
#define gpio45_sdc1_d0_node   PIN_MFP_SET(45,   SDC1_MFP_CFG_VAL)
#define gpio46_sdc1_d1_node   PIN_MFP_SET(46,   SDC1_MFP_CFG_VAL)
#define gpio41_sdc1_d2_node   PIN_MFP_SET(41,   SDC1_MFP_CFG_VAL)
#define gpio42_sdc1_d3_node   PIN_MFP_SET(42,   SDC1_MFP_CFG_VAL)


/* uart0  */
#define UART0_MFP_SEL           5
#define UART0_MFP_CFG (GPIO_CTL_MFP(UART0_MFP_SEL)|GPIO_CTL_SMIT|GPIO_CTL_PULLUP_STRONG|GPIO_CTL_PADDRV_LEVEL(4))
#define gpio10_uart0_tx_node    PIN_MFP_SET(10,  UART0_MFP_CFG)
#define gpio11_uart0_rx_node    PIN_MFP_SET(11,  UART0_MFP_CFG)
#define gpio28_uart0_tx_node    PIN_MFP_SET(28,  UART0_MFP_CFG)
#define gpio29_uart0_rx_node    PIN_MFP_SET(29,  UART0_MFP_CFG)
#define gpio37_uart0_tx_node    PIN_MFP_SET(37,  UART0_MFP_CFG)
#define gpio38_uart0_rx_node    PIN_MFP_SET(38,  UART0_MFP_CFG)
#define gpio63_uart0_tx_node    PIN_MFP_SET(63,  UART0_MFP_CFG)
#define gpio62_uart0_rx_node    PIN_MFP_SET(62,  UART0_MFP_CFG)

/* uart1 */
#define UART1_MFP_SEL			6
#define UART1_MFP_CFG (GPIO_CTL_MFP(UART1_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PULLUP_STRONG | GPIO_CTL_PADDRV_LEVEL(4))
#define gpio16_uart1_tx_node		PIN_MFP_SET(16, UART1_MFP_CFG)
#define gpio17_uart1_rx_node		PIN_MFP_SET(17, UART1_MFP_CFG)

/* uart2 */
#define UART2_MFP_SEL			7
#define UART2_MFP_CFG (GPIO_CTL_MFP(UART2_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PULLUP_STRONG | GPIO_CTL_PADDRV_LEVEL(4))
#define gpio53_uart2_tx_node		PIN_MFP_SET(53, UART2_MFP_CFG)
#define gpio54_uart2_rx_node		PIN_MFP_SET(54, UART2_MFP_CFG)


/* SPDIFTX */
#define SPDIFTX_MFP_SEL           14
#define SPDIFTX_MFP_CFG	(GPIO_CTL_MFP(SPDIFTX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio9_spdiftx0_d0_node   PIN_MFP_SET(9, SPDIFTX_MFP_CFG)
#define gpio62_spdiftx0_d0_node   PIN_MFP_SET(62, SPDIFTX_MFP_CFG)

/* SPDIFRX */
#define SPDIFRX_MFP_SEL           15
#define SPDIFRX_MFP_CFG	(GPIO_CTL_MFP(SPDIFRX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio13_spdifrx0_d0_node   PIN_MFP_SET(13,  SPDIFRX_MFP_CFG)
#define gpio63_spdifrx0_d0_node   PIN_MFP_SET(63,  SPDIFRX_MFP_CFG)

/* I2STX */
#define I2STX_MFP_SEL             12
#define I2STX_MFP_CFG (GPIO_CTL_MFP(I2STX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio39_i2stx0_d0_node     PIN_MFP_SET(39,   I2STX_MFP_CFG)
#define gpio16_i2stx0_mclk_node   PIN_MFP_SET(16,   I2STX_MFP_CFG)
#define gpio17_i2stx0_bclk_node   PIN_MFP_SET(17,   I2STX_MFP_CFG)
#define gpio18_i2stx0_lrclk_node  PIN_MFP_SET(18,   I2STX_MFP_CFG)

#define gpio9_i2stx0_d0_node     PIN_MFP_SET(9,  I2STX_MFP_CFG)
#define gpio6_i2stx0_mclk_node   PIN_MFP_SET(6,  I2STX_MFP_CFG)
#define gpio7_i2stx0_bclk_node   PIN_MFP_SET(7,  I2STX_MFP_CFG)
#define gpio8_i2stx0_lrclk_node  PIN_MFP_SET(8,  I2STX_MFP_CFG)

#define gpio49_i2stx0_mclk_node  PIN_MFP_SET(49, I2STX_MFP_CFG)
#define gpio50_i2stx0_bclk_node  PIN_MFP_SET(50, I2STX_MFP_CFG)
#define gpio51_i2stx0_lrclk_node PIN_MFP_SET(51, I2STX_MFP_CFG)
#define gpio52_i2stx0_d0_node    PIN_MFP_SET(52, I2STX_MFP_CFG)

/* I2SRX */
#define I2SRX_MFP_SEL             13
#define I2SRX_MFP_CFG (GPIO_CTL_MFP(I2SRX_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio43_i2srx0_d0_node     PIN_MFP_SET(43,  I2SRX_MFP_CFG)
#define gpio19_i2srx0_mclk_node   PIN_MFP_SET(19,  I2SRX_MFP_CFG)
#define gpio20_i2srx0_bclk_node   PIN_MFP_SET(20,  I2SRX_MFP_CFG)
#define gpio21_i2srx0_lrclk_node  PIN_MFP_SET(21,  I2SRX_MFP_CFG)

#define gpio9_i2srx0_d0_node      PIN_MFP_SET(9,   I2SRX_MFP_CFG)
#define gpio40_i2srx0_mclk_node   PIN_MFP_SET(40,  I2SRX_MFP_CFG)
#define gpio6_i2srx0_bclk_node    PIN_MFP_SET(6,   I2SRX_MFP_CFG)
#define gpio7_i2srx0_lrclk_node   PIN_MFP_SET(7,   I2SRX_MFP_CFG)

#define gpio53_i2srx0_mclk_node   PIN_MFP_SET(53,   I2SRX_MFP_CFG)
#define gpio54_i2srx0_bclk_node   PIN_MFP_SET(54,   I2SRX_MFP_CFG)
#define gpio55_i2srx0_lrclk_node  PIN_MFP_SET(55,   I2SRX_MFP_CFG)
#define gpio56_i2srx0_d0_node     PIN_MFP_SET(56,   I2SRX_MFP_CFG)

/* TP KEY */

#define gpio12_tp_rst_node	PIN_MFP_SET(12,  GPIO_CTL_GPIO_OUTEN)
#define gpio30_tp_rst_node	PIN_MFP_SET(30,  GPIO_CTL_GPIO_OUTEN)

#define gpio13_tp_isr_node	PIN_MFP_SET(13,  GPIO_CTL_GPIO_OUTEN)
#define gpio31_tp_isr_node	PIN_MFP_SET(31,  GPIO_CTL_GPIO_OUTEN)



/* GPIO KEY */

#define GPIOKEY_MFP_PU_CFG (GPIO_CTL_SMIT | GPIO_CTL_PULLUP | GPIO_CTL_GPIO_INEN | GPIO_CTL_INTC_EN | GPIO_CTL_INC_TRIGGER_DUAL_EDGE)
#define GPIOKEY_MFP_CFG (GPIO_CTL_SMIT | GPIO_CTL_GPIO_INEN | GPIO_CTL_INTC_EN | GPIO_CTL_INC_TRIGGER_DUAL_EDGE)

#define gpio17_keygpio_key0_node   PIN_MFP_SET(17,   GPIOKEY_MFP_CFG)
#define gpio18_keygpio_key1_node   PIN_MFP_SET(18,   GPIOKEY_MFP_CFG)
#define gpio19_keygpio_key2_node   PIN_MFP_SET(19,   GPIOKEY_MFP_CFG)
#define gpio20_keygpio_key3_node   PIN_MFP_SET(20,   GPIOKEY_MFP_CFG)
#define gpio21_keygpio_key4_node   PIN_MFP_SET(21,   GPIOKEY_MFP_CFG)
#define gpio22_keygpio_key5_node   PIN_MFP_SET(22,   GPIOKEY_MFP_CFG)
#define gpio23_keygpio_key6_node   PIN_MFP_SET(23,   GPIOKEY_MFP_CFG)
#define gpio24_keygpio_key7_node   PIN_MFP_SET(24,   GPIOKEY_MFP_CFG)
#define gpio25_keygpio_key8_node   PIN_MFP_SET(25,   GPIOKEY_MFP_CFG)
#define gpio26_keygpio_key9_node   PIN_MFP_SET(26,   GPIOKEY_MFP_CFG)

/* PWM */
#define PWM_MFP_CFG (0x12 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define PWM_PIN_MFP_SET(pin, chan, val)				\
	{.pin_num = pin,		\
	 .pin_chan = chan,		\
	 .mode = val}

#define gpio03_pwm_chan0_node   PWM_PIN_MFP_SET(03, 0, PWM_MFP_CFG)
#define gpio04_pwm_chan0_node   PWM_PIN_MFP_SET(04, 0, PWM_MFP_CFG)
#define gpio14_pwm_chan0_node   PWM_PIN_MFP_SET(14, 0, PWM_MFP_CFG)
#define gpio36_pwm_chan0_node   PWM_PIN_MFP_SET(36, 0, PWM_MFP_CFG)
#define gpio49_pwm_chan0_node   PWM_PIN_MFP_SET(49, 0, PWM_MFP_CFG)

#define gpio05_pwm_chan1_node   PWM_PIN_MFP_SET(05, 1, PWM_MFP_CFG)
#define gpio15_pwm_chan1_node   PWM_PIN_MFP_SET(15, 1, PWM_MFP_CFG)
#define gpio37_pwm_chan1_node   PWM_PIN_MFP_SET(37, 1, PWM_MFP_CFG)
#define gpio50_pwm_chan1_node   PWM_PIN_MFP_SET(50, 1, PWM_MFP_CFG)

#define gpio06_pwm_chan2_node   PWM_PIN_MFP_SET(06, 2, PWM_MFP_CFG)
#define gpio21_pwm_chan2_node   PWM_PIN_MFP_SET(21, 2, PWM_MFP_CFG)
#define gpio38_pwm_chan2_node   PWM_PIN_MFP_SET(38, 2, PWM_MFP_CFG)
#define gpio51_pwm_chan2_node   PWM_PIN_MFP_SET(51, 2, PWM_MFP_CFG)

#define gpio07_pwm_chan3_node   PWM_PIN_MFP_SET(07, 3, PWM_MFP_CFG)
#define gpio17_pwm_chan3_node   PWM_PIN_MFP_SET(17, 3, PWM_MFP_CFG)
#define gpio39_pwm_chan3_node   PWM_PIN_MFP_SET(39, 3, PWM_MFP_CFG)
#define gpio52_pwm_chan3_node   PWM_PIN_MFP_SET(52, 3, PWM_MFP_CFG)

#define gpio08_pwm_chan4_node   PWM_PIN_MFP_SET(08, 4, PWM_MFP_CFG)
#define gpio18_pwm_chan4_node   PWM_PIN_MFP_SET(18, 4, PWM_MFP_CFG)
#define gpio40_pwm_chan4_node   PWM_PIN_MFP_SET(40, 4, PWM_MFP_CFG)
#define gpio53_pwm_chan4_node   PWM_PIN_MFP_SET(53, 4, PWM_MFP_CFG)

#define gpio09_pwm_chan5_node   PWM_PIN_MFP_SET(09, 5, PWM_MFP_CFG)
#define gpio19_pwm_chan5_node   PWM_PIN_MFP_SET(19, 5, PWM_MFP_CFG)
#define gpio41_pwm_chan5_node   PWM_PIN_MFP_SET(41, 5, PWM_MFP_CFG)
#define gpio54_pwm_chan5_node   PWM_PIN_MFP_SET(54, 5, PWM_MFP_CFG)

#define gpio10_pwm_chan6_node   PWM_PIN_MFP_SET(10, 6, PWM_MFP_CFG)
#define gpio20_pwm_chan6_node   PWM_PIN_MFP_SET(20, 6, PWM_MFP_CFG)
#define gpio42_pwm_chan6_node   PWM_PIN_MFP_SET(42, 6, PWM_MFP_CFG)
#define gpio55_pwm_chan6_node   PWM_PIN_MFP_SET(55, 6, PWM_MFP_CFG)

#define gpio11_pwm_chan7_node   PWM_PIN_MFP_SET(11, 7, PWM_MFP_CFG)
#define gpio21_pwm_chan7_node   PWM_PIN_MFP_SET(21, 7, PWM_MFP_CFG)
#define gpio43_pwm_chan7_node   PWM_PIN_MFP_SET(43, 7, PWM_MFP_CFG)
#define gpio45_pwm_chan7_node   PWM_PIN_MFP_SET(45, 7, PWM_MFP_CFG)
#define gpio56_pwm_chan7_node   PWM_PIN_MFP_SET(56, 7, PWM_MFP_CFG)

#define gpio12_pwm_chan7_node   PWM_PIN_MFP_SET(12, 8, PWM_MFP_CFG)
#define gpio22_pwm_chan7_node   PWM_PIN_MFP_SET(22, 8, PWM_MFP_CFG)
#define gpio44_pwm_chan7_node   PWM_PIN_MFP_SET(44, 8, PWM_MFP_CFG)
#define gpio46_pwm_chan7_node   PWM_PIN_MFP_SET(46, 8, PWM_MFP_CFG)
#define gpio57_pwm_chan7_node   PWM_PIN_MFP_SET(57, 8, PWM_MFP_CFG)


/* SPI NOR */
#define SPINOR_MFP_SEL             1
#define SPINOR_MFP_CFG (GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define SPINOR_MFP_PU_CFG (GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP)

#define SPINOR_22_MFP_SEL            22
#define SPINOR_22_MFP_CFG (GPIO_CTL_MFP(SPINOR_22_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define SPINOR_22_MFP_PU_CFG (GPIO_CTL_MFP(SPINOR_22_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP)

#define gpio0_spinor0_cs_node	PIN_MFP_SET(0,   SPINOR_MFP_CFG)
#define gpio1_spinor0_miso_node	PIN_MFP_SET(1,   SPINOR_MFP_CFG)
#define gpio2_spinor0_clk_node	PIN_MFP_SET(2,   SPINOR_MFP_CFG)
#define gpio3_spinor0_mosi_node	PIN_MFP_SET(3,   SPINOR_MFP_CFG)
#define gpio6_spinor0_io2_node	PIN_MFP_SET(6,   SPINOR_MFP_PU_CFG)
#define gpio7_spinor0_io3_node	PIN_MFP_SET(7,   SPINOR_MFP_PU_CFG)

/* HDMI CEC */
#define CEC_MFP_SEL             21
#define CEC_MFP_CFG (GPIO_CTL_MFP(CEC_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP)
#define gpio12_cec0_d0_node	PIN_MFP_SET(12,  CEC_MFP_CFG)

/* ADC KEY */
#define ADCKEY_MFP_SEL          28
#define ADCKEY_MFP_CFG (GPIO_CTL_MFP(ADCKEY_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio21_adckey_lradc3_node	PIN_MFP_SET(21,  ADCKEY_MFP_CFG)

/* SPI NAND */
#define SPINAND_MFP_SEL         4
#define SPINAND_MFP_CFG (GPIO_CTL_MFP(SPINAND_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(7))
#define SPINAND_MFP_PU_CFG (GPIO_CTL_MFP(SPINAND_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(7) | GPIO_CTL_PULLUP)
//#define SPINAND_MFP_PU_CFG (GPIO_CTL_MFP(SPINAND_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(7))
#define gpio8_spinand3_io2_node	    PIN_MFP_SET(8,   SPINAND_MFP_PU_CFG)
#define gpio9_spinand3_io3_node	    PIN_MFP_SET(9,   SPINAND_MFP_PU_CFG)
#define gpio10_spinand3_ss_node	    PIN_MFP_SET(10,  SPINAND_MFP_CFG)
#define gpio11_spinand3_clk_node	PIN_MFP_SET(11,  SPINAND_MFP_CFG)
#define gpio12_spinand3_io1_node	PIN_MFP_SET(12,  SPINAND_MFP_CFG)
#define gpio13_spinand3_io0_node	PIN_MFP_SET(13,  SPINAND_MFP_CFG)

#define gpio61_spinand3_io2_node	PIN_MFP_SET(61,  SPINAND_MFP_PU_CFG)
#define gpio62_spinand3_io3_node	PIN_MFP_SET(62,  SPINAND_MFP_PU_CFG)
#define gpio53_spinand3_ss_node     PIN_MFP_SET(53,  SPINAND_MFP_CFG)
#define gpio54_spinand3_clk_node	PIN_MFP_SET(54,  SPINAND_MFP_CFG)
#define gpio55_spinand3_io1_node	PIN_MFP_SET(55,  SPINAND_MFP_CFG)
#define gpio56_spinand3_io0_node	PIN_MFP_SET(56,  SPINAND_MFP_CFG)

#endif /* PINCTRL_LARK_H_ */
