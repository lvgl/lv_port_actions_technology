/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_CLOCK_H_
#define	_ACTIONS_SOC_CLOCK_H_

#define	CLOCK_ID_DMA			0
#define	CLOCK_ID_SD0			1
#define	CLOCK_ID_SD1			2
#define	CLOCK_ID_OTFD			3
#define	CLOCK_ID_SPI0			4
#define	CLOCK_ID_SPI1			5
#define	CLOCK_ID_SPI2			6
#define	CLOCK_ID_SPI3			7
#define	CLOCK_ID_SPI0CACHE		8
#define	CLOCK_ID_SPI1CACHE		9
#define	CLOCK_ID_USB			10
#define	CLOCK_ID_USB2			11  //USBHCLK
#define	CLOCK_ID_DE				12
#define	CLOCK_ID_JPEG			13  //JPEGCLK
#define	CLOCK_ID_LCD			14
#define	CLOCK_ID_GPU			15
#define	CLOCK_ID_SE			    16
#define	CLOCK_ID_PWM0			17
#define	CLOCK_ID_AVS			18
//#define	CLOCK_ID_TIMER			18
#define	CLOCK_ID_LRADC			19

#define	CLOCK_ID_CPUTIMER		20
#define	CLOCK_ID_SDMA   		21

#define	CLOCK_ID_I2C3   		23

#define	CLOCK_ID_UART0			24
#define	CLOCK_ID_UART1			25
#define	CLOCK_ID_UART2			26
#define	CLOCK_ID_I2C0			27
#define	CLOCK_ID_I2C1			28
#define	CLOCK_ID_I2C2			29
#define	CLOCK_ID_EXINT			30


#define	CLOCK_ID_DSP			32
#define	CLOCK_ID_ASRC			33
#define	CLOCK_ID_AUDDSPTIMER	33
#define	CLOCK_ID_DAC			34
#define	CLOCK_ID_ADC			35
#define	CLOCK_ID_I2STX			36
#define	CLOCK_ID_I2SRX			37
#define CLOCK_ID_I2SSRDCLK      38
#define CLOCK_ID_I2SHCLKEN      39

#define CLOCK_ID_DACANACLK      (32 + 8)

#define CLOCK_ID_TIMER0        (32 + 10)
#define CLOCK_ID_TIMER1        (32 + 11)
#define CLOCK_ID_TIMER2        (32 + 12)
#define CLOCK_ID_TIMER3        (32 + 13)
#define CLOCK_ID_TIMER4        (32 + 14)
#define CLOCK_ID_TIMER5        (32 + 15)

#define	CLOCK_ID_SPIMT0			48
#define	CLOCK_ID_SPIMT1			49
#define	CLOCK_ID_I2CMT0			50
#define	CLOCK_ID_I2CMT1			51

#define	CLOCK_ID_PWM1			(32 + 20)
#define	CLOCK_ID_PWM2			(32 + 21)
#define	CLOCK_ID_PWM3			(32 + 22)

#define	CLOCK_ID_MAX_ID			63

#ifndef _ASMLANGUAGE

void acts_clock_peripheral_enable(int clock_id);
void acts_clock_peripheral_disable(int clock_id);
uint32_t clk_rate_get_corepll(void);
int clk_set_rate(int clock_id,  uint32_t rate_hz);
uint32_t clk_get_rate(int clock_id);
uint32_t clk_ahb_set(uint32_t div);
uint32_t soc_freq_get_dsp_freq(void);
uint32_t soc_freq_get_cpu_freq(void);
void soc_freq_set_cpu_clk(uint32_t dsp_mhz, uint32_t cpu_mhz);
void soc_freq_set_gpu_clk(uint32_t gpu_mhz, uint32_t de_mhz, uint32_t jpeg_mhz);
uint32_t soc_freq_calculate(uint8_t divisor, uint8_t clk_src);
uint8_t soc_freq_divisor_calculate(uint32_t freq_mhz, uint32_t max_mhz);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_CLOCK_H_	*/
