/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_RESET_H_
#define	_ACTIONS_SOC_RESET_H_

#define	RESET_ID_DMA			0
#define	RESET_ID_SD0			1
#define	RESET_ID_SD1			2
#define	RESET_ID_SPI0			4
#define	RESET_ID_SPI1			5
#define	RESET_ID_SPI2			6
#define	RESET_ID_SPI3			7
#define	RESET_ID_SPI0CACHE		8
#define	RESET_ID_SPI1CACHE		9
#define	RESET_ID_USB			10
#define	RESET_ID_USB2			11
#define	RESET_ID_DE				12
#define	RESET_ID_JPEG			13
#define	RESET_ID_LCD			14
#define	RESET_ID_GPU			15
#define	RESET_ID_SE				16
#define	RESET_ID_PWM			17
#define	RESET_ID_TIMER			18
#define	RESET_ID_LRADC			19

#define	RESET_ID_SDMA			21
#define	RESET_ID_UART0			24
#define	RESET_ID_UART1			25
#define	RESET_ID_UART2			26
#define	RESET_ID_I2C0			27
#define	RESET_ID_I2C1			28
#define	RESET_ID_FFT			29

#define	RESET_ID_DSP			32
#define	RESET_ID_ASRC			33
#define	RESET_ID_DAC			34
#define	RESET_ID_ADC			35
#define	RESET_ID_I2STX			36
#define	RESET_ID_I2SRX			37
#define	RESET_ID_SPDIFTX		40
#define	RESET_ID_SPDIFRX		41
#define RESET_ID_ANC			45
#define	RESET_ID_BT				56


#define	RESET_ID_SPIMT0			48
#define	RESET_ID_SPIMT1			49
#define	RESET_ID_I2CMT0			50
#define	RESET_ID_I2CMT1			51

#define	RESET_ID_MAX_ID			63

#ifndef _ASMLANGUAGE

void acts_reset_peripheral_assert(int reset_id);
void acts_reset_peripheral_deassert(int reset_id);
void acts_reset_peripheral(int reset_id);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_RESET_H_	*/
