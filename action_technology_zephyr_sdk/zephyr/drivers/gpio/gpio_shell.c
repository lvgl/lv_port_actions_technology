/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <drivers/gpio.h>
#include <board_cfg.h>
#include "gpio_utils.h"

#if defined(CONFIG_SOC_SERIES_LEOPARD)
#define MAX_MFP_PINNUM 		86
#define MAX_GPIO_PINNUM 	81
#else
#define MAX_MFP_PINNUM 		69
#define MAX_GPIO_PINNUM 	65
#endif

#define WIO_CTL_MFP_MASK	0xf

#if defined(CONFIG_SOC_SERIES_LEOPARD)
enum {
	WIO0 = 81,
	WIO1,
	WIO2,
	WIO3,
	WIO4,
};
#else
enum {
	WIO0 = 65,
	WIO1,
	WIO2,
	WIO3,
};
#endif

enum {
	UART_TX,
	UART_RX,
	UART_CTS,
	UART_RTS,
};

enum {
	I2C_CLK,
	I2C_DAT,
};

 enum {
	SPI_CS,
	SPI_CLK,
	SPI_MISO,
	SPI_MOSI,
	SPI_D2,
	SPI_D3,
	SPI_D4,
	SPI_D5,
	SPI_D6,
	SPI_D7,
	SPI_DM,
	SPI_DQS,
	SPI_CLK_POS,
	SPI_CLK_NEG,
	SPI_IO2,
	SPI_IO3,
	SPI_DQS0,
	SPI_DQS1,
	SPI_D8,
	SPI_D9,
	SPI_D10,
	SPI_D11,
	SPI_D12,
	SPI_D13,
	SPI_D14,
	SPI_D15,
};

enum {
	IICMT_CLK,
	IICMT_DAT,
};

enum {
	SPIMT_SS,
	SPIMT_SS0,
	SPIMT_SS1,
	SPIMT_CLK,
	SPIMT_MISO,
	SPIMT_MOSI,
	SPIMT_SS2,
	SPIMT_SS3,
};

enum {
	PPI_TRIG0,
	PPI_TRIG1,
	PPI_TRIG2,
	PPI_TRIG3,
	PPI_TRIG4,
	PPI_TRIG5,
	PPI_TRIG6,
	PPI_TRIG7,
	PPI_TRIG8,
	PPI_TRIG9,
	PPI_TRIG10,
	PPI_TRIG11,
};

enum {
	SD0_CMD,
	SD0_CLK0,
	SD0_D0,
	SD0_D1,
	SD0_D6,
	SD0_D7,
	SD0_D4,
	SD0_D5,
	SD0_D2,
	SD0_D3,
	SD0_CLK1,
	SD1_CMD,
	SD1_CLK0,
	SD1_D0,
	SD1_D1,
	SD1_D2,
	SD1_D3,
	SD1_D4,
	SD1_D5,
	SD1_D6,
	SD1_D7,
	SD1_CLK1,
};

enum {
	IIS_MCLK,
	IIS_LRCLK,
	IIS_BCLK,
	IIS_DOUT,
	IIS_DIN,
};

enum {
	SPDIF_TX,
	SPDIF_RX,
	SPDIF_RX_A,
};

enum {
	CEC,
};

enum {
	DMIC_CLK,
	DMIC_DAT,
};

enum {
	PWM0,
	PWM1,
	PWM2,
	PWM3,
	PWM4,
	PWM5,
	PWM6,
	PWM7,
	PWM8,
};

enum {
	LCD_D0,
	LCD_D1,
	LCD_D2,
	LCD_D3,
	LCD_D4,
	LCD_D5,
	LCD_D6,
	LCD_D7,
	LCD_D10,
	LCD_D11,
	LCD_D12,
	LCD_D13,
	LCD_D14,
	LCD_D15,
	LCD_CE0,
	LCD_CE1,
	LCD_RS,
	LCD_RDE,
	LCD_WR,
	LCD_TE,
};

enum {
	GIO14,
	GIO15,
	GIO16,
	GIO17,
	GIO30,
	GIO31,
	BT_UART_TX,
	BT_UART_RX,
	BT_REQ,
	BT_ACCESS,
	PTA_GRANT,
	ANT_SW0,
	ANT_SW1,
	ANT_SW2,
	ANT_SW3,
	ANT_SW4,
	ANT_SW5,
	ANT_SW6,
	ANT_SW7,
};

enum {
	LRADC4,
	LRADC5,
	LRADC2,
	LRADC3,
	LRADC6,
	LRADC7,
	LRADC1,
};

enum {
	TIMER2_CAP,
	TIMER3_CAP,
};

enum {
	CTK0_OUT,
	USBDM,
	USBDP,
};

enum {
	IOVCC1_OUT,
	IOVCC2_OUT,
	IOVCC4_OUT,
	WCI_TX,
	WCI_RX,
	VCC_OUT,
};

enum {
	HOSCOUT,
	LOSCOUT,
};

#if defined(CONFIG_SOC_SERIES_LEOPARD)

static const uint8_t MFP_TABLE[MAX_GPIO_PINNUM][32] = {
	{0, SPI_CS, 0, 0, 0, 0, 0, 0, 0, 0, SD0_CMD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_MOSI},
	{0, SPI_MISO, 0, 0, 0, 0, 0, 0, 0, 0, SD0_CLK0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_CLK},
	{0, SPI_CLK, 0, 0, 0, 0, 0, 0, 0, 0, SD0_D0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_CS},
	{0, SPI_MOSI, 0, 0, 0, 0, 0, 0, 0, 0, SD0_D1, 0, 0, 0, 0, 0, 0, 0, PWM0, 0, 0, 0, SPI_MISO},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, I2C_CLK, 0, SD0_D4, PPI_TRIG0, IIS_LRCLK, IIS_LRCLK, I2C_DAT, 0, 0, 0, PWM0, 0, SD1_D2, 0, 0, 0, 0, SPIMT_SS0, 0, 0, LRADC4},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_DAT, 0, SD0_D5, PPI_TRIG1, IIS_DOUT, IIS_DIN, I2C_CLK, 0, 0, 0, PWM1, 0, SD1_D3, 0, 0, 0, 0, SPIMT_CLK, 0, 0, LRADC5},
	{0, SPI_D2, 0, 0, 0, 0, UART_CTS, 0, 0, I2C_CLK, SD0_D2, 0, 0, 0, 0, 0, 0, DMIC_CLK, PWM2},
	{0, SPI_D3, 0, 0, 0, 0, UART_RTS, 0, 0, I2C_DAT, SD0_D3, 0, 0, 0, 0, 0, 0, DMIC_DAT, PWM3},
	{0, 0, 0, SPI_MISO, SPI_IO2, 0, 0, UART_TX, 0, 0, SD0_D2, PPI_TRIG2, IIS_LRCLK, IIS_LRCLK, I2C_DAT, I2C_CLK, 0, 0, PWM0, 0, SD1_CMD, 0, 0, 0, 0, SPIMT_MISO, 0, 0, LRADC2},
	{0, 0, 0, SPI_MOSI, SPI_IO3, 0, 0, UART_RX, 0, 0, SD0_D3, PPI_TRIG3, IIS_BCLK, IIS_BCLK, 0, I2C_DAT, 0, 0, PWM1, 0, SD1_CLK0, 0, 0, 0, 0, SPIMT_MOSI, 0, 0, LRADC3},
	{0, 0, 0, 0, SPI_CS, UART_RX, 0, UART_CTS, I2C_CLK, 0, SD0_CMD, 0, 0, 0, 0, 0, 0, 0, PWM2, 0, SD1_D2, IIS_DOUT, SPI_CS, GIO14, 0, SPIMT_SS1},
// 11
	{0, 0, 0, 0, SPI_CLK, UART_RX, 0, UART_RTS, I2C_DAT, 0, SD0_D0, PPI_TRIG5, 0, 0, I2C_DAT, 0, 0, 0, PWM3, 0, SD1_CLK0, 0, SPI_CLK, GIO15, SPIMT_SS3, SPIMT_SS2},
	{0, 0, 0, 0, SPI_MISO, UART_CTS, UART_TX, 0, 0, I2C_CLK, SD0_CLK0, PPI_TRIG6, 0, 0, I2C_CLK, 0, 0, DMIC_CLK, PWM1, 0, SD1_D3, IIS_DOUT, SPI_MISO, GIO16, SPIMT_SS2, SPIMT_SS3},
	{0, 0, 0, 0, SPI_MOSI, UART_RTS, UART_RX, 0, 0, I2C_DAT, SD0_D1, PPI_TRIG7, 0, IIS_DIN, 0, I2C_CLK, 0, DMIC_DAT, 0, 0, SD1_D0, 0, SPI_MOSI, GIO17, SPIMT_SS1, 0, 0, 0, 0, 0, 0, WCI_TX},
	{0, 0, 0, 0, 0, UART_TX, 0, 0, I2C_CLK, 0, 0, PPI_TRIG0, IIS_DOUT, 0, 0, 0, 0, 0, PWM0, LCD_D0, 0, 0, 0, BT_UART_TX, SPIMT_SS3, SPIMT_MISO, IICMT_CLK},
	{0, 0, 0, 0, 0, UART_RX, 0, 0, I2C_DAT, 0, 0, PPI_TRIG1, 0, IIS_DIN, 0, 0, 0, 0, PWM1, LCD_D1, 0, 0, 0, BT_UART_RX, SPIMT_SS2, SPIMT_MOSI, IICMT_DAT},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, 0, I2C_DAT, 0, PPI_TRIG2, IIS_MCLK, IIS_MCLK, 0, 0, 0, 0, PWM2, LCD_D2, 0, 0, 0, BT_REQ, 0, SPIMT_SS1, 0, IICMT_DAT},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, 0, I2C_CLK, 0, PPI_TRIG3, IIS_BCLK, IIS_BCLK, 0, I2C_CLK, 0, 0, PWM3, LCD_D3, 0, 0, 0, BT_ACCESS, 0, SPIMT_SS0, 0, IICMT_CLK},
	{0, 0, 0, SPI_MISO, SPI_MISO, 0, UART_CTS, 0, I2C_CLK, 0, SD0_D0, PPI_TRIG4, IIS_DOUT, IIS_DIN, 0, 0, 0, 0, PWM1, LCD_D4, SD1_D2, IIS_DOUT, 0, PTA_GRANT, SPIMT_MISO, SPIMT_MISO, IICMT_CLK, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, 0, UART_RTS, 0, I2C_DAT, 0, SD0_D1, PPI_TRIG5, IIS_MCLK, IIS_MCLK, 0, 0, 0, 0, PWM2, LCD_D5, SD1_D3, 0, 0, GIO14, SPIMT_MOSI, SPIMT_MOSI, IICMT_DAT, IICMT_DAT, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_CS, SPI_CS, 0, UART_TX, UART_TX, 0, I2C_CLK, SD0_D2, PPI_TRIG6, IIS_BCLK, IIS_BCLK, 0, 0, 0, 0, PWM3, LCD_D6, 0, 0, 0, GIO15, SPIMT_SS0, SPIMT_SS0, 0, IICMT_CLK, LRADC2},
// 21
	{0, 0, 0, SPI_CLK, SPI_CLK, 0, UART_RX, UART_RX, I2C_CLK, I2C_DAT, SD0_D3, PPI_TRIG7, IIS_LRCLK, IIS_LRCLK, 0, 0, 0, 0, PWM0, LCD_D7, 0, 0, 0, GIO16, SPIMT_CLK, SPIMT_CLK, 0, IICMT_DAT, LRADC3},
	{0, 0, 0, 0, SPI_MISO, 0, 0, UART_CTS, I2C_DAT, 0, SD0_D6, PPI_TRIG8, 0, 0, I2C_CLK, 0, 0, 0, PWM1, 0, SD1_D0, IIS_DOUT, 0, GIO17, SPIMT_SS3, SPIMT_MISO, IICMT_CLK, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, 0, SPI_MOSI, 0, 0, UART_RTS, 0, I2C_CLK, SD0_D7, PPI_TRIG9, IIS_DOUT, IIS_DIN, I2C_DAT, 0, 0, 0, 0, 0, SD1_D1, 0, 0, ANT_SW0, SPIMT_SS2, SPIMT_MOSI, IICMT_DAT, 0, LRADC2, 0, 0, WCI_TX},
	{0, 0, 0, 0, SPI_CLK, 0, UART_TX, 0, I2C_CLK, I2C_DAT, SD0_CLK0, PPI_TRIG10, IIS_MCLK, IIS_MCLK, 0, I2C_CLK, 0, 0, 0, LCD_D4, 0, 0, 0, ANT_SW1, SPIMT_SS1, SPIMT_SS0, IICMT_CLK, 0, 0, 0, 0, WCI_RX},
	{0, 0, 0, 0, SPI_IO2, 0, UART_RX, 0, I2C_DAT, 0, SD0_CMD, PPI_TRIG11, IIS_BCLK, IIS_BCLK, I2C_CLK, I2C_DAT, 0, 0, 0, LCD_D5, 0, 0, 0, ANT_SW2, 0, SPIMT_CLK, IICMT_DAT, 0, LRADC5, HOSCOUT, LOSCOUT, WCI_TX},
	{0, 0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, I2C_CLK, SD0_D4, PPI_TRIG0, IIS_LRCLK, IIS_LRCLK, I2C_DAT, 0, 0, 0, 0, LCD_D6, SD1_CMD, 0, 0, ANT_SW3, 0, SPIMT_MISO, 0, IICMT_CLK, 0, 0, 0, VCC_OUT},
	{0, 0, 0, 0, SPI_IO3, 0, 0, UART_RX, 0, I2C_DAT, SD0_D5, PPI_TRIG1, IIS_DOUT, 0, 0, I2C_CLK, 0, 0, 0, LCD_D7, SD1_CLK0, 0, 0, ANT_SW4, 0, SPIMT_MOSI, 0, IICMT_DAT, LRADC6, 0, 0, VCC_OUT},
	{0, 0, 0, 0, SPI_IO2, UART_TX, 0, 0, I2C_CLK, 0, SD0_D6, PPI_TRIG2, IIS_DOUT, 0, I2C_CLK, 0, 0, 0, 0, 0, SD1_D1, 0, 0, BT_UART_TX, 0, SPIMT_SS0, IICMT_CLK},
	{0, 0, 0, SPI_CS, SPI_IO3, UART_RX, 0, 0, I2C_DAT, 0, SD0_D7, PPI_TRIG3, 0, IIS_DIN, I2C_DAT, 0, 0, 0, 0, 0, SD1_D0, IIS_DOUT, 0, BT_UART_RX, 0, SPIMT_SS1, IICMT_DAT},
	{0, 0, 0, SPI_CS, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG0, IIS_LRCLK, IIS_LRCLK, I2C_DAT, I2C_DAT, 0, DMIC_CLK, 0, LCD_CE0, 0, 0, 0, ANT_SW7, 0, SPIMT_CLK},
//31
	{0, 0, 0, SPI_CLK, SPI_CLK, 0, 0, 0, 0, 0, 0, PPI_TRIG4, IIS_MCLK, IIS_MCLK, 0, I2C_DAT, 0, DMIC_DAT, 0, 0, SD1_D2, 0, 0, ANT_SW5, 0, SPIMT_CLK, 0, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_MISO, 0, 0, 0, 0, I2C_CLK, 0, 0, PPI_TRIG1, 0, 0, I2C_CLK, 0, 0, 0, 0, LCD_RS, 0, IIS_DOUT, 0, ANT_SW6, SPIMT_MISO, 0, IICMT_CLK, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_MOSI, 0, 0, 0, 0, I2C_DAT, 0, 0, PPI_TRIG2, 0, 0, 0, 0, 0, 0, 0, LCD_RDE, 0, 0, 0, GIO30, SPIMT_MOSI, 0, IICMT_DAT, 0, 0, 0, 0, IOVCC2_OUT},
	{0, 0, 0, SPI_CLK, 0, 0, 0, 0, 0, I2C_CLK, 0, PPI_TRIG3, 0, 0, 0, 0, 0, DMIC_CLK, 0, LCD_WR, 0, 0, 0, BT_UART_TX, SPIMT_CLK, 0, 0, IICMT_CLK},
	{0, 0, 0, SPI_MISO, 0, 0, 0, 0, 0, I2C_DAT, 0, PPI_TRIG4, 0, 0, 0, I2C_CLK, 0, DMIC_DAT, 0, LCD_TE, 0, 0, 0, BT_UART_RX, SPIMT_SS1, 0, IICMT_CLK, IICMT_DAT, LRADC4},
	{0, 0, SPI_MOSI, 0, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG0, IIS_MCLK, 0, 0, I2C_CLK, 0, 0, PWM0},
	{0, 0, SPI_MISO, 0, 0, UART_TX, 0, 0, 0, 0, 0, PPI_TRIG1, IIS_BCLK, 0, 0, I2C_DAT, 0, 0, PWM1},
	{0, 0, SPI_D2, 0, 0, UART_RX, 0, 0, 0, 0, 0, PPI_TRIG2, IIS_LRCLK, 0, 0, 0, 0, 0, PWM2},
	{0, 0, SPI_D3, 0, 0, 0, UART_TX, 0, I2C_CLK, 0, 0, PPI_TRIG3, IIS_DOUT, 0, 0, 0, 0, 0, PWM3, 0, 0, 0, 0, 0, 0, 0, IICMT_CLK, IICMT_CLK},
	{0, 0, SPI_CS, SPI_MISO, 0, 0, UART_RX, 0, I2C_DAT, 0, 0, PPI_TRIG4, 0, IIS_MCLK, 0, 0, 0, 0, PWM0, 0, 0, 0, 0, 0, 0, SPIMT_MISO, IICMT_DAT, IICMT_DAT},
//41
	{0, 0, SPI_CLK_POS, SPI_MOSI, 0, 0, 0, UART_TX, 0, I2C_CLK, 0, PPI_TRIG5, 0, IIS_BCLK, 0, 0, 0, 0, PWM1, 0, SD1_D2, 0, 0, 0, 0, SPIMT_MOSI},
	{0, 0, SPI_CLK_NEG, SPI_CLK, 0, 0, 0, UART_RX, 0, I2C_DAT, 0, PPI_TRIG6, 0, IIS_LRCLK, 0, 0, 0, 0, PWM2, 0, SD1_D3, 0, 0, 0, 0, SPIMT_CLK},
	{0, 0, SPI_D4, SPI_CS, 0, UART_CTS, 0, 0, 0, 0, 0, PPI_TRIG7, 0, IIS_DIN, I2C_DAT, 0, 0, 0, PWM3, 0, SD1_CLK0, 0, 0, 0, 0, SPIMT_SS0},
	{0, 0, SPI_D5, 0, 0, UART_RTS, 0, 0, 0, 0, 0, PPI_TRIG8, 0, IIS_MCLK, I2C_CLK, 0, 0, 0, PWM1, 0, SD1_CMD, 0, 0, 0, 0, SPIMT_SS1},
	{0, 0, SPI_D6, 0, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG9, 0, IIS_BCLK, 0, 0, 0, 0, PWM2, 0, SD1_D0, 0, 0, 0, 0, SPIMT_SS2},
	{0, 0, SPI_D7, 0, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG10, 0, IIS_LRCLK, 0, 0, 0, 0, PWM3, 0, SD1_D1, 0, 0, 0, 0, SPIMT_SS3},
	{0, 0, SPI_DM, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, I2C_CLK, 0, 0, 0, 0, 0, IIS_DOUT, 0, 0, 0, 0, 0, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, SPI_DQS, 0, 0, 0, 0, 0, 0, 0, SD0_D7, PPI_TRIG11, 0, 0, 0, I2C_DAT, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPIMT_CLK, 0, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, I2C_CLK, 0, 0, PPI_TRIG0, 0, 0, I2C_CLK, 0, 0, 0, PWM0, 0, SD1_CMD, 0, 0, BT_REQ, SPIMT_SS0, 0, IICMT_CLK},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_DAT, 0, 0, PPI_TRIG1, 0, 0, I2C_DAT, 0, 0, 0, PWM1, 0, SD1_CLK0, 0, 0, BT_ACCESS, SPIMT_CLK, 0, IICMT_DAT},
//51
	{0, 0, 0, SPI_MISO, 0, 0, UART_CTS, 0, 0, I2C_CLK, 0, PPI_TRIG2, 0, 0, 0, I2C_CLK, 0, 0, PWM2, 0, SD1_D1, 0, 0, PTA_GRANT, SPIMT_MISO, 0, 0, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, 0, 0, UART_RTS, 0, I2C_CLK, I2C_DAT, 0, PPI_TRIG3, IIS_DOUT, 0, 0, I2C_DAT, 0, 0, PWM3, 0, SD1_D0, IIS_DOUT, 0, GIO14, SPIMT_MOSI, 0, 0, IICMT_DAT},
	{0, 0, 0, SPI_CS, SPI_CS, 0, 0, UART_TX, I2C_DAT, 0, SD0_D4, PPI_TRIG4, 0, IIS_MCLK, I2C_CLK, 0, 0, 0, PWM0, 0, SD1_D2, 0, 0, GIO15, 0, SPIMT_SS0, IICMT_CLK, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_CLK, SPI_CLK, 0, 0, UART_RX, 0, I2C_CLK, SD0_D5, PPI_TRIG5, 0, IIS_BCLK, I2C_DAT, 0, 0, 0, PWM1, 0, SD1_D3, 0, 0, GIO16, 0, SPIMT_CLK, IICMT_DAT},
	{0, 0, 0, SPI_MISO, SPI_MISO, 0, 0, UART_CTS, 0, I2C_DAT, SD0_D6, PPI_TRIG6, 0, IIS_LRCLK, 0, I2C_CLK, 0, 0, PWM2, 0, SD1_CLK0, 0, 0, GIO17, 0, SPIMT_MISO, 0, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, 0, 0, UART_RTS, 0, 0, SD0_D7, PPI_TRIG7, IIS_DOUT, IIS_DIN, I2C_CLK, I2C_DAT, 0, 0, PWM3, 0, 0, 0, 0, ANT_SW0, 0, SPIMT_MOSI, 0, IICMT_DAT},
	{0, 0, 0, SPI_CS, 0, 0, 0, 0, I2C_CLK, 0, SD0_D2, PPI_TRIG8, 0, 0, I2C_DAT, 0, 0, 0, PWM1, 0, 0, IIS_DOUT, 0, ANT_SW1, SPIMT_SS2, 0, IICMT_CLK, 0, 0, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_CLK, 0, 0, 0, 0, I2C_DAT, 0, SD0_D3, PPI_TRIG9, IIS_DOUT, 0, 0, I2C_CLK, 0, 0, 0, 0, 0, 0, 0, ANT_SW2, SPIMT_SS3, 0, IICMT_DAT, 0, 0, 0, 0, WCI_TX},
	{0, 0, 0, SPI_MISO, 0, 0, 0, 0, 0, I2C_CLK, SD0_CLK1, PPI_TRIG10, IIS_MCLK, IIS_MCLK, 0, I2C_DAT, 0, 0, 0, 0, 0, 0, 0, ANT_SW3, SPIMT_SS1, 0, 0, IICMT_CLK, 0, 0, 0, WCI_RX},
	{0, 0, 0, SPI_MOSI, 0, 0, 0, 0, 0, I2C_DAT, SD0_CLK0, PPI_TRIG11, IIS_BCLK, IIS_BCLK, 0, I2C_CLK, 0, 0, 0, 0, 0, 0, 0, ANT_SW4, 0, 0, 0, IICMT_DAT, 0, HOSCOUT, LOSCOUT},
//61
	{0, 0, 0, 0, SPI_IO2, 0, 0, 0, 0, 0, SD0_D0, PPI_TRIG4, IIS_LRCLK, IIS_LRCLK, I2C_CLK, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW5, 0, SPIMT_SS3, 0, 0, LRADC5, HOSCOUT, LOSCOUT},
	{0, 0, 0, 0, SPI_IO3, UART_RX, UART_RX, UART_RX, 0, 0, SD0_D1, PPI_TRIG5, IIS_DOUT, IIS_DIN, I2C_DAT, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW6, 0, SPIMT_SS1, 0, 0, LRADC4},
	{0, 0, 0, SPI_MISO, SPI_MISO, UART_TX, UART_TX, UART_TX, 0, 0, SD0_CMD, PPI_TRIG6, IIS_MCLK, IIS_MCLK, 0, I2C_DAT, 0, 0, 0, 0, 0, IIS_DOUT, 0, ANT_SW7, 0, SPIMT_SS2, 0, 0, LRADC3, HOSCOUT, LOSCOUT, IOVCC4_OUT},
	{0, 0, 0, SPI_MISO, SPI_MISO, UART_TX, UART_TX, UART_TX, 0, 0, SD0_D7, PPI_TRIG11, IIS_MCLK, IIS_MCLK, I2C_CLK, I2C_DAT, 0, 0, PWM0, 0, SD1_D1, IIS_DOUT, 0, GIO31, SPIMT_SS0, 0, 0, IICMT_DAT, 0, HOSCOUT, LOSCOUT, IOVCC1_OUT},
	{0, 0, SPI_DQS1, SPI_MISO, 0, 0, UART_TX, 0, I2C_CLK, 0, SD0_CMD, 0, 0, IIS_DIN, 0, 0, 0, 0, PWM0, 0, 0, 0, 0, BT_REQ, SPIMT_SS0, 0, IICMT_CLK, 0, 0, 0, 0, WCI_TX},
	{0, 0, SPI_D8, SPI_MOSI, 0, 0, UART_RX, 0, I2C_DAT, 0, SD0_CLK0, PPI_TRIG0, IIS_MCLK, IIS_MCLK, 0, 0, 0, 0, PWM1, 0, 0, 0, 0, BT_ACCESS, SPIMT_CLK, 0, IICMT_DAT, 0, 0, 0, 0, WCI_RX},
	{0, 0, SPI_D9, SPI_CLK, 0, UART_TX, UART_CTS, 0, 0, I2C_CLK, SD0_D0, PPI_TRIG1, IIS_BCLK, IIS_BCLK, 0, 0, 0, 0, PWM2, 0, 0, 0, 0, PTA_GRANT, SPIMT_MISO, 0, 0, IICMT_CLK},
	{0, 0, SPI_D10, SPI_CS, 0, UART_RX, UART_RTS, 0, I2C_CLK, I2C_DAT, SD0_D1, PPI_TRIG2, IIS_LRCLK, IIS_LRCLK, 0, 0, 0, DMIC_CLK, PWM3, 0, 0, 0, 0, GIO14, SPIMT_MOSI, SPIMT_SS2, 0, IICMT_DAT},
	{0, 0, SPI_D11, 0, SPI_CS, 0, 0, UART_TX, I2C_DAT, 0, SD0_D2, PPI_TRIG3, IIS_DOUT, IIS_DIN, I2C_CLK, 0, 0, DMIC_DAT, PWM0, 0, 0, 0, 0, GIO15, SPIMT_SS1, SPIMT_SS3, IICMT_CLK},
	{0, 0, SPI_D12, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_CLK, SD0_D3, PPI_TRIG4, 0, 0, I2C_DAT, 0, 0, DMIC_CLK, PWM1, 0, 0, IIS_DOUT, 0, GIO16, SPIMT_SS2, SPIMT_SS1, IICMT_DAT, 0, 0, HOSCOUT, LOSCOUT},
//71
	{0, 0, SPI_D13, 0, SPI_MISO, 0, 0, UART_CTS, 0, I2C_DAT, SD0_D4, PPI_TRIG5, IIS_MCLK, IIS_MCLK, 0, I2C_CLK, 0, 0, PWM2, 0, SD1_CMD, 0, 0, GIO17, SPIMT_SS3, SPIMT_CLK, 0, IICMT_CLK, 0, 0, 0, WCI_TX},
	{0, 0, SPI_D14, 0, SPI_MOSI, 0, 0, UART_RTS, 0, 0, SD0_D5, PPI_TRIG6, IIS_BCLK, IIS_BCLK, I2C_CLK, I2C_DAT, 0, 0, PWM3, 0, SD1_CLK0, 0, 0, ANT_SW0, 0, SPIMT_MISO, 0, IICMT_DAT, 0, HOSCOUT, LOSCOUT},
	{0, 0, SPI_D15, 0, SPI_IO2, 0, UART_TX, UART_RX, 0, 0, SD0_D6, PPI_TRIG7, IIS_LRCLK, IIS_LRCLK, I2C_DAT, I2C_CLK, 0, 0, PWM0, 0, SD1_D0, 0, 0, ANT_SW1, 0, SPIMT_MOSI, 0, 0, 0, 0, 0, WCI_RX},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, UART_RX, 0, UART_RX, I2C_CLK, I2C_DAT, SD0_D6, PPI_TRIG4, IIS_BCLK, IIS_BCLK, I2C_DAT, I2C_CLK, 0, DMIC_CLK, PWM1, 0, SD1_D0, 0, 0, ANT_SW3, SPIMT_CLK, 0, 0, IICMT_CLK, LRADC6, HOSCOUT, LOSCOUT, WCI_RX},
	{0, 0, 0, SPI_MISO, SPI_MISO, UART_TX, 0, UART_RX, 0, 0, 0, 0, IIS_BCLK, IIS_BCLK, I2C_CLK, 0, USBDP, DMIC_CLK, PWM2, 0, SD1_D3, 0, 0, BT_UART_TX, 0, SPIMT_SS3, IICMT_CLK, IICMT_CLK, 0, 0, 0, WCI_TX},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, UART_RX, 0, UART_TX, 0, 0, 0, 0, IIS_LRCLK, IIS_LRCLK, I2C_DAT, 0, USBDM, DMIC_DAT, PWM3, 0, SD1_CLK0, 0, 0, BT_UART_RX, 0, SPIMT_SS2, IICMT_DAT, IICMT_DAT, 0, 0, 0, WCI_RX},
	{0, 0, 0, SPI_MOSI, 0, 0, 0, 0, 0, 0, SD0_CLK0, PPI_TRIG4, IIS_DOUT, IIS_DIN, 0, I2C_DAT, 0, DMIC_DAT, 0, LCD_CE1, SD1_CLK0, IIS_DOUT, 0, ANT_SW6, SPIMT_SS0, 0, IICMT_DAT, 0, LRADC6, HOSCOUT, LOSCOUT},
	{0, 0, 0, SPI_CS, SPI_CS, 0, UART_TX, 0, I2C_DAT, 0, SD0_D4, PPI_TRIG8, IIS_BCLK, IIS_BCLK, I2C_CLK, I2C_CLK, 0, DMIC_DAT, PWM2, 0, SD1_CLK0, 0, 0, ANT_SW4, SPIMT_MISO, 0, IICMT_CLK, IICMT_DAT, 0, 0, 0, WCI_TX},
	{0, 0, 0, SPI_CLK, SPI_CLK, 0, UART_RX, UART_RX, I2C_CLK, I2C_CLK, SD0_D5, PPI_TRIG9, IIS_LRCLK, IIS_LRCLK, 0, I2C_CLK, 0, DMIC_CLK, PWM3, 0, SD1_CMD, 0, 0, ANT_SW5, SPIMT_MOSI, 0, IICMT_DAT, IICMT_CLK, 0, 0, 0, WCI_RX},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, 0, UART_RX, UART_TX, 0, I2C_DAT, SD0_D7, PPI_TRIG8, IIS_DOUT, IIS_DIN, I2C_DAT, I2C_DAT, 0, DMIC_DAT, 0, 0, SD1_D0, IIS_DOUT, 0, ANT_SW2, 0, SPIMT_SS0, 0, 0, 0, HOSCOUT, LOSCOUT},
};
#else

static const uint8_t MFP_TABLE[MAX_GPIO_PINNUM][32] = {
	{0, SPI_CS, 0, 0, 0, 0, 0, 0, 0, 0, SD0_CMD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_MOSI},
	{0, SPI_MISO, 0, 0, 0, 0, 0, 0, 0, 0, SD0_CLK0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_CLK},
	{0, SPI_CLK, 0, 0, 0, 0, 0, 0, 0, 0, SD0_D0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SPI_CS},
	{0, SPI_MOSI, 0, 0, 0, 0, 0, 0, 0, 0, SD0_D1, 0, 0, IIS_MCLK, 0, 0, 0, 0, PWM0, 0, 0, 0, SPI_MISO},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, I2C_CLK, 0, SD0_D6, PPI_TRIG0, IIS_LRCLK, IIS_LRCLK, 0, 0, DMIC_CLK, 0, PWM0, 0, 0, 0, 0, 0, 0, SPIMT_SS0, 0, 0, LRADC4, TIMER2_CAP},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_DAT, 0, SD0_D7, PPI_TRIG1, IIS_DOUT, IIS_DIN, SPDIF_TX, SPDIF_RX, DMIC_DAT, 0, PWM1, 0, 0, 0, 0, 0, 0, SPIMT_CLK, 0, 0, LRADC5, TIMER3_CAP},
	{0, SPI_D2, 0, 0, 0, 0, UART_CTS, 0, 0, I2C_CLK, SD0_D4, 0, IIS_MCLK, IIS_BCLK, 0, 0, 0, DMIC_CLK, PWM2},
	{0, SPI_D3, 0, 0, 0, 0, UART_RTS, 0, 0, I2C_DAT, SD0_D5, 0, IIS_BCLK, IIS_LRCLK, 0, 0, 0, DMIC_DAT, PWM3},
	{0, 0, 0, SPI_MISO, SPI_IO2, 0, 0, UART_TX, 0, 0, SD0_D2, PPI_TRIG2, IIS_LRCLK, IIS_LRCLK, 0, 0, DMIC_CLK, 0, PWM4, 0, 0, 0, 0, 0, 0, SPIMT_MISO, 0, 0, LRADC2},
	{0, 0, 0, SPI_MOSI, SPI_IO3, 0, 0, UART_RX, 0, 0, SD0_D3, PPI_TRIG3, IIS_DOUT, IIS_DIN, SPDIF_TX, SPDIF_RX, DMIC_DAT, 0, PWM5, 0, 0, CEC, 0, 0, 0, SPIMT_MOSI, 0, 0, LRADC3},
	{0, 0, 0, 0, SPI_CS, UART_RX, 0, UART_CTS, I2C_CLK, 0, SD0_CLK1, 0, 0, 0, 0, 0, 0, 0, PWM6, 0, 0, 0, SPI_CS, GIO14, SPIMT_SS0},
	{0, 0, 0, 0, SPI_CLK, UART_RX, 0, UART_RTS, I2C_DAT, 0, SD0_CMD, PPI_TRIG5, 0, 0, 0, 0, 0, 0, PWM7, 0, 0, 0, SPI_CLK, GIO15, SPIMT_CLK},
//12
	{0, 0, 0, 0, SPI_MISO, UART_CTS, UART_TX, 0, 0, I2C_CLK, SD0_D0, PPI_TRIG6, 0, 0, 0, 0, 0, DMIC_CLK, PWM8, 0, 0, 0, SPI_MISO, GIO16, SPIMT_MISO},
	{0, 0, 0, 0, SPI_MOSI, UART_RTS, UART_RX, 0, 0, I2C_DAT, SD0_D1, PPI_TRIG7, 0, IIS_DIN, 0, SPDIF_RX, 0, DMIC_DAT, 0, 0, 0, 0, SPI_MOSI, GIO17, SPIMT_MOSI, 0, 0, 0, 0, 0, 0, IOVCC1_OUT},
	{0, 0, 0, 0, 0, UART_TX, 0, 0, I2C_CLK, 0, 0, PPI_TRIG0, IIS_DOUT, 0, SPDIF_TX, 0, 0, 0, PWM0, LCD_D0, 0, CEC, 0, BT_UART_TX, 0, 0, IICMT_CLK},
	{0, 0, 0, 0, 0, UART_RX, 0, 0, I2C_DAT, 0, 0, PPI_TRIG1, 0, IIS_DIN, 0, SPDIF_RX, 0, 0, PWM1, LCD_D1, 0, 0, 0, BT_UART_RX, 0, 0, IICMT_DAT, 0, 0, 0, 0, IOVCC2_OUT},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, I2C_CLK, 0, 0, PPI_TRIG2, IIS_MCLK, IIS_MCLK, 0, SPDIF_RX_A, 0, 0, PWM2, LCD_D2, 0, CEC, 0, BT_REQ, SPIMT_SS, 0, IICMT_CLK},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_DAT, 0, 0, PPI_TRIG3, IIS_BCLK, IIS_BCLK, 0, 0, 0, 0, PWM3, LCD_D3, 0, 0, 0, BT_ACCESS, SPIMT_CLK, 0, IICMT_DAT},
//18
	{0, 0, 0, SPI_MISO, SPI_MISO, 0, UART_CTS, 0, I2C_CLK, I2C_CLK, SD1_CMD, PPI_TRIG4, IIS_LRCLK, IIS_DIN, 0, 0, 0, 0, PWM4, LCD_D4, 0, 0, 0, PTA_GRANT, SPIMT_MISO, SPIMT_MISO, IICMT_CLK, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, SPI_MOSI, 0, UART_RTS, 0, I2C_DAT, I2C_DAT, SD1_CLK0, PPI_TRIG5, 0, IIS_MCLK, 0, 0, 0, 0, PWM5, LCD_D5, 0, 0, 0, GIO14, SPIMT_MOSI, SPIMT_MOSI, IICMT_DAT, IICMT_DAT},
	{0, 0, 0, SPI_CS, SPI_CS, 0, UART_TX, UART_TX, 0, I2C_CLK, SD1_D0, PPI_TRIG6, 0, IIS_BCLK, 0, 0, 0, 0, PWM6, LCD_D6, 0, 0, 0, GIO15, SPIMT_SS, SPIMT_SS, 0, IICMT_CLK, LRADC2},
//21
	{0, 0, 0, SPI_CLK, SPI_CLK, 0, UART_RX, UART_RX, 0, I2C_DAT, SD1_D1, PPI_TRIG7, 0, IIS_LRCLK, 0, 0, 0, 0, PWM7, LCD_D7, 0, 0, 0, GIO16, SPIMT_CLK, SPIMT_CLK, 0, IICMT_DAT, LRADC3},
	{0, 0, 0, 0, SPI_MISO, 0, 0, UART_CTS, I2C_CLK, 0, SD1_D2, PPI_TRIG8, IIS_DOUT, 0, SPDIF_TX, 0, DMIC_CLK, 0, PWM8, 0, 0, CEC, 0, GIO17, 0, SPIMT_MISO, IICMT_CLK, 0, LRADC6, TIMER2_CAP},
	{0, 0, 0, 0, SPI_MOSI, 0, 0, UART_RTS, I2C_DAT, 0, SD1_D3, PPI_TRIG9, 0, IIS_DIN, 0, SPDIF_RX, DMIC_DAT, 0, 0, 0, 0, 0, 0, ANT_SW0, 0, SPIMT_MOSI, IICMT_DAT, 0, LRADC7, TIMER3_CAP},
	{0, 0, SPI_CS, 0, 0, 0, UART_TX, 0, I2C_CLK, 0, SD1_D4, PPI_TRIG10, IIS_MCLK, IIS_MCLK, 0, SPDIF_RX_A, 0, 0, 0, LCD_D10, 0, CEC, 0, ANT_SW1, SPIMT_SS, 0, IICMT_CLK},
	{0, 0, SPI_CLK, 0, 0, 0, UART_RX, 0, I2C_DAT, 0, SD1_D5, PPI_TRIG11, IIS_BCLK, IIS_BCLK, 0, 0, 0, 0, 0, LCD_D11, 0, CEC, 0, ANT_SW2, SPIMT_CLK, 0, IICMT_DAT},
	{0, 0, SPI_MISO, 0, 0, 0, 0, UART_TX, 0, I2C_CLK, SD1_D6, PPI_TRIG0, IIS_LRCLK, IIS_LRCLK, 0, 0, 0, 0, 0, LCD_D12, 0, 0, 0, ANT_SW3, SPIMT_MISO, 0, 0, IICMT_CLK},
	{0, 0, SPI_MOSI, 0, 0, 0, 0, UART_RX, 0, I2C_DAT, SD1_D7, PPI_TRIG1, 0, IIS_MCLK, 0, 0, 0, 0, 0, LCD_D13, 0, 0, 0, ANT_SW4, SPIMT_MOSI, 0, 0, IICMT_DAT},
	{0, 0, 0, 0, SPI_IO2, UART_TX, 0, 0, I2C_CLK, 0, SD1_CLK1, PPI_TRIG2, 0, IIS_BCLK, 0, 0, 0, 0, 0, LCD_D14, 0, 0, 0, BT_UART_TX, SPIMT_SS1, 0, IICMT_CLK},
	{0, 0, 0, 0, SPI_IO3, UART_RX, 0, 0, I2C_DAT, 0, 0, PPI_TRIG3, 0, IIS_LRCLK, 0, 0, 0, 0, 0, LCD_D15, 0, 0, 0, BT_UART_RX, 0, SPIMT_SS1, IICMT_DAT},
	{0, 0, 0, SPI_CS, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG0, 0, 0, 0, 0, 0, DMIC_CLK, 0, LCD_CE0, 0, 0, 0, ANT_SW7},
//31
	{0, 0, 0, SPI_CLK, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG4, 0, 0, 0, 0, 0, DMIC_DAT, 0, LCD_CE1, 0, 0, 0, ANT_SW5},
	{0, 0, 0, SPI_MISO, 0, 0, 0, 0, I2C_CLK, 0, 0, PPI_TRIG1, 0, 0, 0, 0, 0, 0, 0, LCD_RS, 0, 0, 0, ANT_SW6, SPIMT_MISO, 0, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, 0, 0, 0, 0, I2C_DAT, 0, 0, PPI_TRIG2, 0, 0, 0, 0, 0, 0, 0, LCD_RDE, 0, 0, 0, GIO30, SPIMT_MOSI, 0, IICMT_DAT},
	{0, 0, 0, SPI_CLK, 0, 0, 0, 0, 0, I2C_CLK, 0, PPI_TRIG3, 0, 0, 0, 0, 0, DMIC_CLK, 0, LCD_WR, 0, 0, 0, BT_UART_TX, SPIMT_CLK, 0, 0, IICMT_CLK},
	{0, 0, 0, SPI_CS, 0, 0, 0, 0, 0, I2C_DAT, 0, PPI_TRIG4, 0, 0, 0, 0, 0, DMIC_DAT, 0, LCD_TE, 0, 0, 0, BT_UART_RX, SPIMT_SS, 0, 0, IICMT_DAT, LRADC4},
	{0, 0, SPI_MOSI, 0, 0, 0, 0, 0, 0, 0, SD1_CLK1, PPI_TRIG0, IIS_MCLK, 0, 0, 0, 0, 0, PWM0},
	{0, 0, SPI_MISO, 0, 0, UART_TX, 0, 0, 0, 0, SD1_D4, PPI_TRIG1, IIS_BCLK, 0, 0, 0, 0, 0, PWM1},
	{0, 0, SPI_D2, 0, 0, UART_RX, 0, 0, 0, 0, SD1_D5, PPI_TRIG2, IIS_LRCLK, 0, 0, 0, 0, 0, PWM2},
	{0, 0, SPI_D3, 0, 0, 0, UART_TX, 0, I2C_CLK, I2C_CLK, SD1_D6, PPI_TRIG3, IIS_DOUT, 0, 0, 0, 0, 0, PWM3, 0, 0, 0, 0, 0, 0, 0, IICMT_CLK, IICMT_CLK},
	{0, 0, SPI_CS, SPI_MISO, 0, 0, UART_RX, 0, I2C_DAT, I2C_DAT, SD1_D7, PPI_TRIG4, 0, IIS_MCLK, 0, 0, DMIC_CLK, 0, PWM4, 0, 0, 0, 0, 0, 0, SPIMT_MISO, IICMT_DAT, IICMT_DAT},
//41
	{0, 0, SPI_CLK_POS, SPI_MOSI, 0, 0, 0, UART_TX, 0, 0, SD1_D2, PPI_TRIG5, 0, IIS_BCLK, 0, 0, DMIC_DAT, 0, PWM5, 0, 0, 0, 0, 0, 0, SPIMT_MOSI},
	{0, 0, SPI_CLK_NEG, SPI_CLK, 0, 0, 0, UART_RX, 0, 0, SD1_D3, PPI_TRIG6, 0, IIS_LRCLK, 0, 0, DMIC_DAT, 0, PWM6, 0, 0, 0, 0, 0, 0, SPIMT_CLK, 0, 0, 0, TIMER2_CAP},
	{0, 0, SPI_D4, SPI_CS, 0, UART_CTS, 0, 0, 0, 0, SD1_CLK0, PPI_TRIG7, 0, IIS_DIN, 0, 0, 0, 0, PWM7, 0, 0, 0, 0, 0, 0, SPIMT_SS0},
	{0, 0, SPI_D5, 0, 0, UART_RTS, 0, 0, 0, 0, SD1_CMD, PPI_TRIG8, 0, IIS_MCLK, 0, 0, DMIC_CLK, 0, PWM8},
	{0, 0, SPI_D6, 0, 0, 0, 0, 0, 0, 0, SD1_D0, PPI_TRIG9, 0, IIS_BCLK, 0, 0, 0, 0, PWM7},
	{0, 0, SPI_D7, 0, 0, 0, 0, 0, 0, 0, SD1_D1, PPI_TRIG10, 0, IIS_LRCLK, 0, 0, 0, 0, PWM8},
	{0, 0, SPI_DM, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TIMER3_CAP},
	{0, 0, SPI_DQS, 0, 0, 0, 0, 0, 0, 0, 0, PPI_TRIG11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TIMER2_CAP},
	{0, 0, 0, SPI_CS, 0, 0, UART_TX, 0, 0, 0, 0, PPI_TRIG0, IIS_MCLK, 0, 0, 0, 0, 0, PWM0, 0, 0, 0, 0, BT_REQ, SPIMT_SS0, 0, IICMT_CLK},
	{0, 0, 0, SPI_CLK, 0, 0, UART_RX, 0, I2C_DAT, 0, 0, PPI_TRIG1, IIS_BCLK, 0, 0, 0, 0, 0, PWM1, 0, 0, 0, 0, BT_ACCESS, SPIMT_CLK, 0, IICMT_DAT},
//51
	{0, 0, 0, SPI_MISO, 0, 0, UART_CTS, 0, 0, I2C_CLK, 0, PPI_TRIG2, IIS_LRCLK, 0, 0, 0, 0, 0, PWM2, 0, 0, 0, 0, PTA_GRANT, SPIMT_MISO, 0, 0, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, 0, 0, UART_RTS, 0, 0, I2C_DAT, 0, PPI_TRIG3, IIS_DOUT, 0, 0, 0, 0, 0, PWM3, 0, 0, 0, 0, GIO14, SPIMT_MOSI, 0, 0, IICMT_DAT},
	{0, 0, 0, 0, SPI_CS, 0, 0, UART_TX, I2C_CLK, 0, 0, PPI_TRIG4, 0, IIS_MCLK, 0, 0, DMIC_CLK, 0, PWM4, 0, 0, 0, 0, GIO15, 0, SPIMT_SS0, IICMT_CLK},
	{0, 0, 0, 0, SPI_CLK, 0, 0, UART_RX, I2C_DAT, 0, 0, PPI_TRIG5, 0, IIS_BCLK, 0, 0, DMIC_DAT, 0, PWM5, 0, 0, 0, 0, GIO16, 0, SPIMT_CLK, IICMT_DAT},
	{0, 0, 0, 0, SPI_MISO, 0, 0, UART_CTS, 0, I2C_CLK, 0, PPI_TRIG6, 0, IIS_LRCLK, 0, 0, DMIC_DAT, 0, PWM6, 0, 0, 0, 0, GIO17, 0, SPIMT_MISO, 0, IICMT_CLK},
	{0, 0, 0, 0, SPI_MOSI, 0, 0, UART_RTS, 0, I2C_DAT, 0, PPI_TRIG7, 0, IIS_DIN, 0, SPDIF_RX_A, 0, 0, PWM7, 0, 0, 0, 0, ANT_SW0, 0, SPIMT_MOSI, 0, IICMT_DAT},
	{0, 0, 0, SPI_CS, 0, 0, 0, 0, I2C_CLK, 0, 0, PPI_TRIG8, 0, IIS_MCLK, 0, 0, DMIC_CLK, 0, PWM8, 0, 0, 0, 0, ANT_SW1, SPIMT_SS, 0, IICMT_CLK, 0, 0, 0, CTK0_OUT},
	{0, 0, 0, SPI_CLK, 0, 0, 0, 0, I2C_DAT, 0, 0, PPI_TRIG9, 0, IIS_BCLK, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW2, SPIMT_CLK, 0, IICMT_DAT},
	{0, 0, 0, SPI_MISO, 0, 0, 0, 0, 0, I2C_CLK, 0, PPI_TRIG10, 0, IIS_LRCLK, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW3, SPIMT_MISO, 0, 0, IICMT_CLK},
	{0, 0, 0, SPI_MOSI, 0, 0, 0, 0, 0, I2C_DAT, 0, PPI_TRIG11, 0, IIS_DIN, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW4, SPIMT_MOSI, 0, 0, IICMT_DAT},
//61
	{0, 0, 0, 0, SPI_IO2, 0, 0, 0, 0, 0, 0, PPI_TRIG4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ANT_SW5, SPIMT_SS1, 0, 0, 0, LRADC5, TIMER2_CAP},
	{0, 0, 0, 0, SPI_IO3, UART_RX, UART_RX, UART_RX, 0, 0, 0, PPI_TRIG5, 0, 0, SPDIF_TX, SPDIF_RX, 0, 0, 0, 0, 0, CEC, 0, ANT_SW6, 0, SPIMT_SS1, 0, 0, LRADC6, TIMER3_CAP},
	{0, 0, 0, 0, 0, UART_TX, UART_TX, UART_TX, 0, 0, 0, PPI_TRIG6, 0, 0, SPDIF_TX, SPDIF_RX, 0, 0, 0, 0, 0, CEC, 0, ANT_SW7, 0, SPIMT_SS1, 0, 0, LRADC7, 0, 0, IOVCC4_OUT},
	{0, 0, 0, 0, 0, UART_TX, UART_TX, UART_TX, 0, 0, 0, PPI_TRIG11, 0, 0, SPDIF_TX, SPDIF_RX, 0, 0, PWM0, 0, 0, CEC, 0, GIO31},

};

#endif


static int cmd_gpio_check(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t i;
	uint8_t j;
	uint32_t regval;
	uint32_t v_mfp;
	uint16_t pinmux[MAX_MFP_PINNUM];
	uint16_t pinmux_high;
	uint16_t pinmux_low;
	bool warning_flag;

	for (i=0; i<MAX_MFP_PINNUM; i++)
	{
#if defined(CONFIG_SOC_SERIES_LEOPARD)
		if (i < WIO0)
		{
			regval = sys_read32(GPIO_REG_BASE + 4 * i);
			v_mfp = regval & GPIO_CTL_MFP_MASK;
		}
		else
		{
			regval = sys_read32(GPIO_REG_BASE + 444 + 4 * i);
			v_mfp = regval & WIO_CTL_MFP_MASK;
		}
#else
		if (i < WIO0)
		{
			regval = sys_read32(GPIO_REG_BASE + 4 + 4 * i);
			v_mfp = regval & GPIO_CTL_MFP_MASK;
		}
		else
		{
			regval = sys_read32(GPIO_REG_BASE + 508 + 4 * i);
			v_mfp = regval & WIO_CTL_MFP_MASK;
		}
#endif

		if (v_mfp != 0)
		{
			if (i < WIO0)
			{
				pinmux_high = (uint16_t)(v_mfp << 8);
				pinmux_low = MFP_TABLE[i][v_mfp];
				pinmux[i] = pinmux_high + pinmux_low;
			}
			else if (i == WIO0)
			{
				if(v_mfp == 2)
				{
					pinmux_high = (uint16_t)(MFP1_I2C << 8);
					pinmux_low = I2C_CLK;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 3)
				{
					pinmux_high = (uint16_t)(ADCKEY_MFP_SEL << 8);
					pinmux_low = LRADC1;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else
					pinmux[i] = 0;
			}
			else if (i == WIO1)
			{
				if(v_mfp == 2)
				{
					pinmux_high = (uint16_t)(MFP1_I2C << 8);
					pinmux_low = I2C_DAT;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 1)
				{
					pinmux_high = (uint16_t)(ADCKEY_MFP_SEL << 8);
					pinmux_low = LRADC1;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 3)
				{
					pinmux_high = (uint16_t)(ADCKEY_MFP_SEL << 8);
					pinmux_low = LRADC1;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 6)
				{
					pinmux_high = (uint16_t)(30 << 8);
					pinmux_low = LOSCOUT;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else
					pinmux[i] = 0;
			}
			else if (i == WIO2)
			{
				if(v_mfp == 2)
				{
					pinmux_high = (uint16_t)(MFP0_I2C << 8);
					pinmux_low = I2C_CLK;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else
					pinmux[i] = 0;
			}
			else if (i == WIO3)
			{
				if(v_mfp == 2)
				{
					pinmux_high = (uint16_t)(MFP0_I2C << 8);
					pinmux_low = I2C_DAT;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 6)
				{
					pinmux_high = (uint16_t)(30 << 8);
					pinmux_low = LOSCOUT;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else
					pinmux[i] = 0;
			}
			else if (i == WIO4)
			{
				if(v_mfp == 2)
				{
					pinmux_high = (uint16_t)(MFP1_I2C << 8);
					pinmux_low = I2C_CLK;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else if (v_mfp == 6)
				{
					pinmux_high = (uint16_t)(30 << 8);
					pinmux_low = LOSCOUT;
					pinmux[i] = pinmux_high + pinmux_low;
				}
				else
					pinmux[i] = 0;
			}
		}
		else
			pinmux[i] = 0;

	}

	for (i=0; i<MAX_MFP_PINNUM; i++)
	{
		warning_flag = true;
		if (pinmux[i] != 0)
		{
			for (j=i+1; j<MAX_MFP_PINNUM; j++)
			{
				if (pinmux[j] == pinmux[i])
				{
					pinmux[j] = 0;
					if (warning_flag)
					{
						if (i < WIO0)
							printk("warning: GPIO_%d ", i);
						else
							printk("warning: WIO_%d ", i);
						warning_flag = false;
					}
					if (j < WIO0)
						printk("GPIO_%d ", j);
					else
						printk("WIO_%d ", j);
				}

				if (j == MAX_MFP_PINNUM-1 && !warning_flag)
				{
					printk("have the same function of 0x%x\n", pinmux[i] >> 8);
				}

			}
		}
	}

	return 0;
}

SHELL_CMD_REGISTER(gpio_check, NULL, "gpio_check commands", cmd_gpio_check);
