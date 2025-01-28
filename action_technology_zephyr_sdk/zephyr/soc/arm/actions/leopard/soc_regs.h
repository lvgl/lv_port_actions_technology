/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file register address define for Actions SoC
 */

#ifndef	_ACTIONS_SOC_REGS_H_
#define	_ACTIONS_SOC_REGS_H_
#include "brom_interface.h"

#define RMU_REG_BASE				0x40000000
#define CMUD_REG_BASE				0x40001000
#define CMUA_REG_BASE				0x40000100
#define PMU_REG_BASE				0x40004000
#define GPIO_REG_BASE				0x40068000
#define DMA_REG_BASE				0x4001c000
#define SDMA_REG_BASE				0x40078000
#define SDMA0_REG_BASE				0x40078100
#define SDMA1_REG_BASE				0x40078200
#define SDMA2_REG_BASE				0x40078300
#define SDMA3_REG_BASE				0x40078400
#define SDMA4_REG_BASE				0x40078500
#define SDMA_LINE0_REG_BASE			0x40078C00
#define SDMA_LINE1_REG_BASE			0x40078C20
#define SDMA_LINE2_REG_BASE			0x40078C40
#define SDMA_LINE3_REG_BASE			0x40078C60
#define SDMA_LINE4_REG_BASE			0x40078C80
#define UART0_REG_BASE				0x40038000
#define UART1_REG_BASE				0x4003C000
#define UART2_REG_BASE				0x40040000

#define MEM_REG_BASE                0x40010000
#define SE_REG_BASE                 0x40020000

#define I2C0_REG_BASE				0x40044000
#define I2C1_REG_BASE				0x40048000
#define I2C2_REG_BASE				0x4004c000
#define I2C3_REG_BASE				0x40074000

#define SPI0_REG_BASE				0x40028000
#define SPI1_REG_BASE				0x4002c000
#define SPI2_REG_BASE				0x40030000
#define SPI3_REG_BASE				0x40034000

#define I2CMT0_REG_BASE				0x40088000
#define I2CMT1_REG_BASE				0x4008c000

#define WIC_BASE                    0x40090000
#define PPI_BASE                    0x40090000
#define DE_REG_BASE                 0x4006C000
#define LCDC_REG_BASE               0x40064000
#define GPU_REG_BASE                0x40070000
#define SPIMT0_REG_BASE				0x40080000
#define SPIMT1_REG_BASE				0x40084000
#define JPEG_REG_BASE               0x40094000
#define BTC_REG_BASE                0x01100000
#define SD0_REG_BASE				0x40054000
#define SD1_REG_BASE				0x40058000
#define PWM_REG_BASE                0x40060000
#define PWM_CLK0_BASE               0x40001068





#define HOSC_CTL					(CMUA_REG_BASE + 0x00)
#define HOSCLDO_CTL					(CMUA_REG_BASE + 0x04)
#define CK64M_CTL					(CMUA_REG_BASE + 0x08)
#define LOSC_CTL					(CMUA_REG_BASE + 0x10)
#define RC4M_CTL					(CMUA_REG_BASE + 0x14)
#define RC192M_CTL                  (CMUA_REG_BASE + 0x18)
#define AVDDLDO_CTL					(CMUA_REG_BASE + 0x1c)
#define COREPLL_CTL					(CMUA_REG_BASE + 0x20)

#define RC32K_CTL					(CMUA_REG_BASE + 0x64)
#define RC32K_CAL					(CMUA_REG_BASE + 0x68)
#define RC32K_COUNT					(CMUA_REG_BASE + 0x6C)


/*fix build err, dsp pll not exist*/
#define SPLL_CTL					(CMUA_REG_BASE + 0x24)
#define DISPLAYPLL_CTL				(CMUA_REG_BASE + 0x30)


#define AUDIOLDO_CTL				(CMUA_REG_BASE + 0x1C)
#define AUDIO_PLL0_CTL				(CMUA_REG_BASE + 0x28)
#define AUDIO_PLL1_CTL				(CMUA_REG_BASE + 0x2C)

#define HOSCOK_CTL					(CMUA_REG_BASE + 0x70)
#define HGLFCLK_CTL                 (CMUA_REG_BASE + 0x74)






#define CMU_SYSCLK                                                        (CMUD_REG_BASE+0x0000)
#define CMU_DEVCLKEN0                                                     (CMUD_REG_BASE+0x0004)
#define CMU_DEVCLKEN1                                                     (CMUD_REG_BASE+0x0008)
#define CMU_SD0CLK                                                        (CMUD_REG_BASE+0x0010)
#define CMU_SD1CLK                                                        (CMUD_REG_BASE+0x0014)
#define CMU_OTFDCLK                                                       (CMUD_REG_BASE+0x0018)
#define CMU_JPEGCLK                                                       (CMUD_REG_BASE+0x001C)
#define CMU_SPI0CLK                                                       (CMUD_REG_BASE+0x0020)
#define CMU_SPI1CLK                                                       (CMUD_REG_BASE+0x0024)
#define CMU_SPI2CLK                                                       (CMUD_REG_BASE+0x0028)
#define CMU_SPI3CLK                                                       (CMUD_REG_BASE+0x002C)
#define CMU_DECLK                                                         (CMUD_REG_BASE+0x0030)
#define CMU_LCDCLK                                                        (CMUD_REG_BASE+0x0034)
#define CMU_GPUCLK                                                        (CMUD_REG_BASE+0x0038)
#define CMU_SECCLK                                                        (CMUD_REG_BASE+0x003C)
#define CMU_TIMER0CLK                                                     (CMUD_REG_BASE+0x0040)
#define CMU_TIMER1CLK                                                     (CMUD_REG_BASE+0x0044)
#define CMU_TIMER2CLK                                                     (CMUD_REG_BASE+0x0048)
#define CMU_TIMER3CLK                                                     (CMUD_REG_BASE+0x004c)
#define CMU_TIMER4CLK                                                     (CMUD_REG_BASE+0x0050)
#define CMU_TIMER5CLK                                                     (CMUD_REG_BASE+0x0054)
#define CMU_PWM0CLK                                                       (CMUD_REG_BASE+0x0068)
#define CMU_PWM1CLK                                                       (CMUD_REG_BASE+0x006C)
#define CMU_PWM2CLK                                                       (CMUD_REG_BASE+0x0070)
#define CMU_PWM3CLK                                                       (CMUD_REG_BASE+0x0074)
#define CMU_LRADCCLK                                                      (CMUD_REG_BASE+0x0078)
#define CMU_I2C0CLK                                                       (CMUD_REG_BASE+0x0080)
#define CMU_I2C1CLK                                                       (CMUD_REG_BASE+0x0084)
#define CMU_I2C2CLK                                                       (CMUD_REG_BASE+0x0088)
#define CMU_I2C3CLK                                                       (CMUD_REG_BASE+0x008C)
#define CMU_DSPCLK                                                        (CMUD_REG_BASE+0x0090)
#define CMU_DACCLK                                                        (CMUD_REG_BASE+0x0098)
#define CMU_ADCCLK                                                        (CMUD_REG_BASE+0x009c)
#define CMU_I2STXCLK                                                      (CMUD_REG_BASE+0x00A0)
#define CMU_I2SRXCLK                                                      (CMUD_REG_BASE+0x00A4)
#define CMU_MEMCLKEN0                                                     (CMUD_REG_BASE+0x00B0)
#define CMU_MEMCLKEN1                                                     (CMUD_REG_BASE+0x00B4)
#define CMU_MEMCLKSRC0                                                    (CMUD_REG_BASE+0x00C0)
#define CMU_MEMCLKSRC1                                                    (CMUD_REG_BASE+0x00C4)
#define CMU_S1CLKCTL                                                      (CMUD_REG_BASE+0x00D0)
#define CMU_S1BTCLKCTL                                                    (CMUD_REG_BASE+0x00D4)
#define CMU_S2SCLKCTL                                                      (CMUD_REG_BASE+0x00DC)
#define CMU_S3CLKCTL                                                      (CMUD_REG_BASE+0x00E0)
#define CMU_WDCLK                                                         (CMUD_REG_BASE+0x00E4)
#define CMU_GPIOCLKCTL                                                    (CMUD_REG_BASE+0x00E8)
#define CMU_PMUWKUPCLKCTL                                                 (CMUD_REG_BASE+0x00EC)
#define CMU_DIGITALDEBUG                                                  (CMUD_REG_BASE+0x00F0)
#define CMU_TST_CTL                                                       (CMUD_REG_BASE+0x00F4)
#define CMU_SPIMT0CLK                                                     (CMUD_REG_BASE+0x0100)
#define CMU_SPIMT1CLK                                                     (CMUD_REG_BASE+0x0104)
#define CMU_I2CMT0CLK                                                     (CMUD_REG_BASE+0x0110)
#define CMU_I2CMT1CLK                                                     (CMUD_REG_BASE+0x0114)

#define RMU_MRCR0					(RMU_REG_BASE + 0x00)
#define RMU_MRCR1					(RMU_REG_BASE + 0x04)
#define DSP_VCT_ADDR                (RMU_REG_BASE+0x00000080)
#define DSP_STATUS_EXT_CTL          (RMU_REG_BASE+0x00000084)
#define GPU_STATUS_EXT_CTL          (RMU_REG_BASE+0x00000090)
#define	UID0                        (RMU_REG_BASE+0x000000A0)
#define	UID1                        (RMU_REG_BASE+0x000000A4)

#define INTC_BASE					0x40003000
#define INT_TO_DSP					(INTC_BASE+0x00000000)
#define INFO_TO_DSP					(INTC_BASE+0x00000004)
#define INT_TO_BT_CPU				(INTC_BASE+0x00000010)
#define PENDING_FROM_DSP			(INTC_BASE+0x00000014)
#define PENDING_FROM_BT_CPU         (INTC_BASE+0x0000001C)

#define MEMORY_CTL					(MEM_REG_BASE)
#define DSP_PAGE_ADDR0				(MEM_REG_BASE+0x00000080)
#define DSP_PAGE_ADDR1				(MEM_REG_BASE+0x00000084)
#define DSP_PAGE_ADDR2              (MEM_REG_BASE+0x00000088)
#define DSP_PAGE_ADDR3              (MEM_REG_BASE+0x0000008c)

#define WIO0_CTL					(GPIO_REG_BASE + 0x300)
#define WIO1_CTL					(GPIO_REG_BASE + 0x304)
#define WIO2_CTL					(GPIO_REG_BASE + 0x308)
#define WIO3_CTL					(GPIO_REG_BASE + 0x30c)

// brom api address
//fpga
//#define SPINOR_API_ADDR				(0x00005860) //(0x00005814)


#define SPINOR_API_ADDR				(p_brom_api->p_spinor_api)


// Interaction RAM
#define INTER_RAM_ADDR				(0x2FF18000)
#define DSP_SHARE_RAM_START_ADDR	(0x2FF1A800)		/* Confirm with DSP, not reflect in linker.ld, can't change */
#define INTER_RAM_SIZE				(DSP_SHARE_RAM_START_ADDR - INTER_RAM_ADDR)

//--------------SPICACHE_Control_Register-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     SPICACHE_Control_Register_BASE                                    0x40014000
#define     SPICACHE_CTL                                                      (SPICACHE_Control_Register_BASE+0x0000)
#define     SPICACHE_INVALIDATE                                               (SPICACHE_Control_Register_BASE+0x0004)
#define     SPICACHE_TOTAL_MISS_COUNT                                         (SPICACHE_Control_Register_BASE+0x0010)
#define     SPICACHE_TOTAL_HIT_COUNT                                          (SPICACHE_Control_Register_BASE+0x0014)
#define     SPICACHE_PROFILE_INDEX_START                                      (SPICACHE_Control_Register_BASE+0x0018)
#define     SPICACHE_PROFILE_INDEX_END                                        (SPICACHE_Control_Register_BASE+0x001C)
#define     SPICACHE_RANGE_INDEX_MISS_COUNT                                   (SPICACHE_Control_Register_BASE+0x0020)
#define     SPICACHE_RANGE_INDEX_HIT_COUNT                                    (SPICACHE_Control_Register_BASE+0x0024)
#define     SPICACHE_PROFILE_ADDR_START                                       (SPICACHE_Control_Register_BASE+0x0028)
#define     SPICACHE_PROFILE_ADDR_END                                         (SPICACHE_Control_Register_BASE+0x002C)
#define     SPICACHE_RANGE_ADDR_MISS_COUNT                                    (SPICACHE_Control_Register_BASE+0x0030)
#define     SPICACHE_RANGE_ADDR_HIT_COUNT                                     (SPICACHE_Control_Register_BASE+0x0034)
#define     SPICACHE_DMA_READ_COUNT                                           (SPICACHE_Control_Register_BASE+0x0038)
#define     SPICACHE_SDMA_READ_COUNT                                          (SPICACHE_Control_Register_BASE+0x003C)
#define     SPICACHE_DE_READ_COUNT                                            (SPICACHE_Control_Register_BASE+0x0040)
#define     MAPPING_MISS_ADDR                                                 (SPICACHE_Control_Register_BASE+0x0060)

#define     SPI1_CACHE_REGISTER_BASE                                          0x40018000
#define     SPI1_CACHE_CTL                                                    (SPI1_CACHE_REGISTER_BASE+0x0000)
#define     SPI1_CACHE_OPERATE                                                (SPI1_CACHE_REGISTER_BASE+0x0004)
#define     CACHE_OPERATE_ADDR_START                                          (SPI1_CACHE_REGISTER_BASE+0x0008)
#define     CACHE_OPERATE_ADDR_END                                            (SPI1_CACHE_REGISTER_BASE+0x000c)
#define     SPI1_CACHE_MCPU_MISS_COUNT                                        (SPI1_CACHE_REGISTER_BASE+0x0010)
#define     SPI1_CACHE_MCPU_HIT_COUNT                                         (SPI1_CACHE_REGISTER_BASE+0x0014)
#define     SPI1_CACHE_MCPU_WRITEBACK_COUNT                                   (SPI1_CACHE_REGISTER_BASE+0x0018)
#define     SPI1_CACHE_DMA_MISS_COUNT                                         (SPI1_CACHE_REGISTER_BASE+0x0028)
#define     SPI1_CACHE_DMA_HIT_COUNT                                          (SPI1_CACHE_REGISTER_BASE+0x002c)
#define     SPI1_CACHE_DMA_WRITEBACK_COUNT                                    (SPI1_CACHE_REGISTER_BASE+0x0030)
#define     SPI1_CACHE_PROFILE_ADDR_START                                     (SPI1_CACHE_REGISTER_BASE+0x003c)
#define     SPI1_CACHE_PROFILE_ADDR_END                                       (SPI1_CACHE_REGISTER_BASE+0x0040)
#define     SPI1_CACHE_DE_READ_COUNT                                          (SPI1_CACHE_REGISTER_BASE+0x0050)
#define     SPI1_CACHE_DE_WRITE_COUNT                                         (SPI1_CACHE_REGISTER_BASE+0x0054)
#define     SPI1_CACHE_GPU_READ_COUNT                                         (SPI1_CACHE_REGISTER_BASE+0x0058)
#define     SPI1_CACHE_GPU_WRITE_COUNT                                        (SPI1_CACHE_REGISTER_BASE+0x005C)
#define     SPI1_CACHE_SDMA_READ_COUNT                                        (SPI1_CACHE_REGISTER_BASE+0x0060)
#define     SPI1_CACHE_SDMA_WRITE_COUNT                                       (SPI1_CACHE_REGISTER_BASE+0x0064)
#define     SPI1_CACHE_DMAUNCACHE_READ_COUNT                                  (SPI1_CACHE_REGISTER_BASE+0x0068)
#define     SPI1_CACHE_DMAUNCACHE_WRITE_COUNT                                 (SPI1_CACHE_REGISTER_BASE+0x006C)
#define     SPI1_PSRAM_MAPPING_MISS_ADDR                                      (SPI1_CACHE_REGISTER_BASE+0x0080)
#define     SPI1_PSRAM_MASTER_WEIGHT                                          (SPI1_CACHE_REGISTER_BASE+0x0090)
#define     SPI1_GPU_CTL                                                      (SPI1_CACHE_REGISTER_BASE+0x0094)

//--------------PMUVDD-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     PMUVDD_BASE                                                       0x40004000
#define     VOUT_CTL0                                                         (PMUVDD_BASE+0x00)
#define     VOUT_CTL1_S1                                                      (PMUVDD_BASE+0x04)
#define     VOUT_CTL1_S2                                                      (PMUVDD_BASE+0x08)
#define     VOUT_CTL1_S3                                                      (PMUVDD_BASE+0X0C)
#define     PMU_DET                                                           (PMUVDD_BASE+0x10)
#define     VOUT_CTL1_S1M                                                     (PMUVDD_BASE+0x14)
#define     DCDC_VC18_CTL                                                     (PMUVDD_BASE+0X20)
#define     DCDC_VD12_CTL                                                     (PMUVDD_BASE+0X24)
// #define     DCDC_VDD_CTL                                                      (PMUVDD_BASE+0X28)
#define     PWRGATE_DIG                                                       (PMUVDD_BASE+0X30)
#define     PWRGATE_DIG_ACK                                                   (PMUVDD_BASE+0X34)
#define     PWRGATE_RAM                                                       (PMUVDD_BASE+0X38)
// #define     PWRGATE_RAM_ACK                                                   (PMUVDD_BASE+0X3C)
#define     PMU_INTMASK                                                       (PMUVDD_BASE+0X40)
#define     RAM_DEEPSLEEP                                                     (PMUVDD_BASE+0x44)
#define     RAM_LIGHTSLEEP                                                    (PMUVDD_BASE+0x48)

//--------------PMUSVCC-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     PMUSVCC_BASE                                                      0x40004000
#define     CHG_CTL_SVCC                                                      (PMUSVCC_BASE+0X100)
#define     BDG_CTL_SVCC                                                      (PMUSVCC_BASE+0x104)
#define     SYSTEM_SET_SVCC                                                   (PMUSVCC_BASE+0x108)
#define     POWER_CTL_SVCC                                                    (PMUSVCC_BASE+0x10C)
#define     WKEN_CTL_SVCC                                                     (PMUSVCC_BASE+0x110)
#define     WAKE_PD_SVCC                                                      (PMUSVCC_BASE+0x114)
#define     COUNTER8HZ_SVCC                                                   (PMUSVCC_BASE+0x118)
#define     ALARM8HZ_SVCC                                                     (PMUSVCC_BASE+0x11C)
#define     REG_BKUP_SVCC                                                     (PMUSVCC_BASE+0x120)


#define     PMUADC_BASE                                                      0x40004000
#define 	PMUADC_CTL 	  													(PMUADC_BASE+0x200)
#define     PMUADC_INTMASK                                                  (PMUADC_BASE+0X204)
#define     PMUADC_PD                                                       (PMUADC_BASE+0X208)
#define     PMUADCDIG_CTL                                                   (PMUADC_BASE+0X20C)
#define     CHARGI_DATA                                                     (PMUADC_BASE+0x210)
#define     BATADC_DATA                                                     (PMUADC_BASE+0x214)
#define     DC5VADC_DATA                                                    (PMUADC_BASE+0x218)
#define     SENSADC_DATA                                                    (PMUADC_BASE+0x21C)
#define     SVCCADC_DATA                                                    (PMUADC_BASE+0x220)
#define     LRADC1_DATA                                                     (PMUADC_BASE+0x224)
#define     VCCI_DATA                                                       (PMUADC_BASE+0x228)
#define     LRADC2_DATA                                                     (PMUADC_BASE+0x22C)
#define     LRADC3_DATA                                                     (PMUADC_BASE+0x230)
#define     LRADC4_DATA                                                     (PMUADC_BASE+0x234)
#define     LRADC5_DATA                                                     (PMUADC_BASE+0x238)
#define     LRADC6_DATA                                                     (PMUADC_BASE+0x23C)


//--------------EFUSE-------------------------------------------//

#define     EFUSE_BASE                                                        0x40008000
#define     EFUSE_CTL0                                                        (EFUSE_BASE+0x00)
#define     EFUSE_CTL1                                                        (EFUSE_BASE+0x04)
#define     EFUSE_CTL2                                                        (EFUSE_BASE+0x08)
#define     EFUSE_DATA0                                                       (EFUSE_BASE+0x10)
#define     EFUSE_DATA1                                                       (EFUSE_BASE+0x14)
#define     EFUSE_DATA2                                                       (EFUSE_BASE+0x18)
#define     EFUSE_DATA3                                                       (EFUSE_BASE+0x1C)
#define     EFUSE_DATA4                                                       (EFUSE_BASE+0x20)
#define     EFUSE_DATA5                                                       (EFUSE_BASE+0x24)
#define     EFUSE_DATA6                                                       (EFUSE_BASE+0x28)
#define     EFUSE_DATA7                                                       (EFUSE_BASE+0x2C)
#define     EFUSE_DATA8                                                       (EFUSE_BASE+0x30)
#define     EFUSE_DATA9                                                       (EFUSE_BASE+0x34)
#define     EFUSE_DATA10                                                      (EFUSE_BASE+0x38)
#define     EFUSE_DATA11                                                      (EFUSE_BASE+0x3C)



#define     InterruptController_BASE                                          0xe000e000
#define     NVIC_ISER0                                                        (InterruptController_BASE+0x00000100)
#define     NVIC_ISER1                                                        (InterruptController_BASE+0x00000104)
#define     NVIC_ICER0                                                        (InterruptController_BASE+0x00000180)
#define     NVIC_ICER1                                                        (InterruptController_BASE+0x00000184)
#define     NVIC_ISPR0                                                        (InterruptController_BASE+0x00000200)
#define     NVIC_ISPR1                                                        (InterruptController_BASE+0x00000204)
#define     NVIC_ICPR0                                                        (InterruptController_BASE+0x00000280)
#define     NVIC_ICPR1                                                        (InterruptController_BASE+0x00000284)
#define     NVIC_IABR0                                                        (InterruptController_BASE+0x00000300)
#define     NVIC_IABR1                                                        (InterruptController_BASE+0x00000304)
#define     NVIC_IPR0                                                         (InterruptController_BASE+0x00000400)
#define     NVIC_IPR1                                                         (InterruptController_BASE+0x00000404)
#define     NVIC_IPR2                                                         (InterruptController_BASE+0x00000408)
#define     NVIC_IPR3                                                         (InterruptController_BASE+0x0000040c)
#define     NVIC_IPR4                                                         (InterruptController_BASE+0x00000410)
#define     NVIC_IPR5                                                         (InterruptController_BASE+0x00000414)
#define     NVIC_IPR6                                                         (InterruptController_BASE+0x00000418)
#define     NVIC_IPR7                                                         (InterruptController_BASE+0x0000041c)
#define     NVIC_IPR8                                                         (InterruptController_BASE+0x00000420)
#define     NVIC_IPR9                                                         (InterruptController_BASE+0x00000424)


#define     TIMER_REGISTER_BASE                                               0x4000C100

/* For sys tick used */
#define     T0_CTL                                                            (TIMER_REGISTER_BASE+0x00)
#define     T0_VAL                                                            (TIMER_REGISTER_BASE+0x04)
#define     T0_CNT                                                            (TIMER_REGISTER_BASE+0x08)

/* For hrtimer used */
#define     T1_CTL                                                            (TIMER_REGISTER_BASE+0x20)
#define     T1_VAL                                                            (TIMER_REGISTER_BASE+0x24)
#define     T1_CNT                                                            (TIMER_REGISTER_BASE+0x28)

/* For system cycle used */
#define     T2_CTL                                                            (TIMER_REGISTER_BASE+0x40)
#define     T2_VAL                                                            (TIMER_REGISTER_BASE+0x44)
#define     T2_CNT                                                            (TIMER_REGISTER_BASE+0x48)

/* For T3 used */
#define     T3_CTL                                                            (TIMER_REGISTER_BASE+0x60)
#define     T3_VAL                                                            (TIMER_REGISTER_BASE+0x64)
#define     T3_CNT                                                            (TIMER_REGISTER_BASE+0x68)

/* For tws used */
#define     T4_CTL                                                            (TIMER_REGISTER_BASE+0x80)
#define     T4_VAL                                                            (TIMER_REGISTER_BASE+0x84)
#define     T4_CNT                                                            (TIMER_REGISTER_BASE+0x88)

#define     T5_CTL                                                            (TIMER_REGISTER_BASE+0xA0)
#define     T5_VAL                                                            (TIMER_REGISTER_BASE+0xA4)
#define     T5_CNT                                                            (TIMER_REGISTER_BASE+0xA8)

#define     RTC_REG_BASE      												  0x4000C000
#define     WD_CTL        													  (RTC_REG_BASE+0x20)
#define     HCL_CTL                                                           (RTC_REG_BASE+0x1D0)
#define     RTC_REMAIN0                                                       (RTC_REG_BASE+0x30)
#define     RTC_REMAIN1                                                       (RTC_REG_BASE+0x34)
#define     RTC_REMAIN2                                                       (RTC_REG_BASE+0x38)
#define     RTC_REMAIN3                                                       (RTC_REG_BASE+0x3C)
#define     RTC_REMAIN4                                                       (RTC_REG_BASE+0x40)
#define     RTC_REMAIN5                                                       (RTC_REG_BASE+0x44)

/* DAC control register */
#define     AUDIO_DAC_REG_BASE                                                0x4005C000

/* ADC control register  */
#define     AUDIO_ADC_REG_BASE                                                0x4005C100
#define     ADC_REF_LDO_CTL                                                   (AUDIO_ADC_REG_BASE + 0x48)

/* I2STX control register */
#define     AUDIO_I2STX0_REG_BASE                                             0x4005C400

/* I2SRX control register */
#define     AUDIO_I2SRX0_REG_BASE                                             0x4005C500

/*spi0*/
#define     SPI0_REGISTER_BASE                                                0x40028000
#define     SPI0_CTL                                                          (SPI0_REGISTER_BASE+0x0000)
#define     SPI0_DELAYCHAIN                                                   (SPI0_REGISTER_BASE+0x0024)

/*spi1*/
#define     SPI1_REGISTER_BASE                                                0x4002C000
#define     SPI1_CTL                                                          (SPI1_REGISTER_BASE+0x0000)
#define     SPI1_DDR_MODE_CTL                                                 (SPI1_REGISTER_BASE+0x0018)
#define     SPI1_CLKGATING                                                    (SPI1_REGISTER_BASE+0x0038)
#define     SPI1_DELAYCHAIN                                                   (SPI1_REGISTER_BASE+0x003C)
#define     SPI1_DQS1_DELAYCHAIN                                              (SPI1_REGISTER_BASE+0x0040)

#define     CTK0_BASE                                                         0x4007C000
#define     CTK_CTL                                                           (CTK0_BASE+0x00)

/* uart */
#define UART0_CTL			(UART0_REG_BASE+0x00)
#define UART0_RXDAT			(UART0_REG_BASE+0x04)
#define UART0_TXDAT			(UART0_REG_BASE+0x08)
#define UART0_STA			(UART0_REG_BASE+0x0c)
#define UART0_BR			(UART0_REG_BASE+0x10)

#define UART1_CTL			(UART1_REG_BASE+0x00)
#define UART1_RXDAT			(UART1_REG_BASE+0x04)
#define UART1_TXDAT			(UART1_REG_BASE+0x08)
#define UART1_STA			(UART1_REG_BASE+0x0c)
#define UART1_BR			(UART1_REG_BASE+0x10)

//--------------Bits Location------------------------------------------//
#define     HOSC_CTL_HOSC_CAPTUNE_DONE                                        30
#define     HOSC_CTL_HOSC_GMTUNE_DONE                                         29
#define     HOSC_CTL_HOSC_READY                                               28
#define     HOSC_CTL_HOSC_READY_TIME_SET_e                                    27
#define     HOSC_CTL_HOSC_READY_TIME_SET_SHIFT                                26
#define     HOSC_CTL_HOSC_READY_TIME_SET_MASK                                 (0x3<<26)
#define     HOSC_CTL_HOSC_READY_EN                                            25
#define     HOSC_CTL_HOSC_CAP_e                                               24
#define     HOSC_CTL_HOSC_CAP_SHIFT                                           17
#define     HOSC_CTL_HOSC_CAP_MASK                                            (0xFF<<17)
#define     HOSC_CTL_HOSC_CAPTUNE_EN                                          16
#define     HOSC_CTL_HOSC_INJECT_EN                                           15
#define     HOSC_CTL_HOSC_AMPLMT_e                                            14
#define     HOSC_CTL_HOSC_AMPLMT_SHIFT                                        13
#define     HOSC_CTL_HOSC_AMPLMT_MASK                                         (0x3<<13)
#define     HOSC_CTL_HOSC_AMPLMT_EN                                           12
#define     HOSC_CTL_HOSC_GM_e                                                11
#define     HOSC_CTL_HOSC_GM_SHIFT                                            9
#define     HOSC_CTL_HOSC_GM_MASK                                             (0x7<<9)
#define     HOSC_CTL_HOSC_GMTUNE_EN                                           8
#define     HOSC_CTL_HOSC_INJECT_TIME_SET_e                                   7
#define     HOSC_CTL_HOSC_INJECT_TIME_SET_SHIFT                               6
#define     HOSC_CTL_HOSC_INJECT_TIME_SET_MASK                                (0x3<<6)
#define     HOSC_CTL_HOSC_VDD_SEL                                             5
#define     HOSC_CTL_HOSC_BUF_SEL                                             4
#define     HOSC_CTL_HOSC_GMTUNE_TIME_SET_e                                   3
#define     HOSC_CTL_HOSC_GMTUNE_TIME_SET_SHIFT                               2
#define     HOSC_CTL_HOSC_GMTUNE_TIME_SET_MASK                                (0x3<<2)
#define     HOSC_CTL_HOSC_CTL1                                                1
#define     HOSC_CTL_HOSC_CTL0                                                0

#define     HOSCLDO_CTL_OSC32M_FRQCAL_e                                       31
#define     HOSCLDO_CTL_OSC32M_FRQCAL_SHIFT                                   22
#define     HOSCLDO_CTL_OSC32M_FRQCAL_MASK                                    (0x3FF<<22)
#define     HOSCLDO_CTL_OSC32M_CALDONE                                        21
#define     HOSCLDO_CTL_OSC32M_FRQMSET_e                                      20
#define     HOSCLDO_CTL_OSC32M_FRQMSET_SHIFT                                  11
#define     HOSCLDO_CTL_OSC32M_FRQMSET_MASK                                   (0x3FF<<11)
#define     HOSCLDO_CTL_OSC32M_CTL_e                                          10
#define     HOSCLDO_CTL_OSC32M_CTL_SHIFT                                      9
#define     HOSCLDO_CTL_OSC32M_CTL_MASK                                       (0x3<<9)
#define     HOSCLDO_CTL_OSC32M_CALMODE                                        8
#define     HOSCLDO_CTL_OSC32M_CALEN                                          7
#define     HOSCLDO_CTL_OSC32M_EN                                             6
#define     HOSCLDO_CTL_OSCVDD0_PD_BIASSET                                    5
#define     HOSCLDO_CTL_OSCVDD0_PD                                            4
#define     HOSCLDO_CTL_OSCVDD0_VSET_e                                        3
#define     HOSCLDO_CTL_OSCVDD0_VSET_SHIFT                                    2
#define     HOSCLDO_CTL_OSCVDD0_VSET_MASK                                     (0x3<<2)
#define     HOSCLDO_CTL_OSCVDD0_EN                                            0

#define     AVDDLDO_CTL_AVDD_PD_EN                                            4
#define     AVDDLDO_CTL_AVDD_EN                                               0

#define     PWRGATE_DIG_MAINCPU_PG                                            31
#define     PWRGATE_DIG_RAM0_MAIN_FORCE                                       30
#define     PWRGATE_DIG_RAM_BT_FORCE                                          29
#define     PWRGATE_DIG_BT_PG                                                 28
#define     PWRGATE_DIG_BT_FORCE                                              27
#define     PWRGATE_DIG_DSP_AU_PG                                             26
#define     PWRGATE_DIG_GPU_PG                                                25
#define     PWRGATE_DIG_SHARERAM_FORCE                                        24
#define     PWRGATE_DIG_RAM_AUTOSLEEP                                         23
#define     PWRGATE_DIG_RAM_FASTWAKE                                          22
#define     PWRGATE_DIG_DISPLAY_PG                                            21

#define     PWRGATE_DIG_ACK_MAINCPU                                           31
#define     PWRGATE_DIG_ACK_BT                                                28
#define     PWRGATE_DIG_ACK_DSP_AU                                            26
#define     PWRGATE_DIG_ACK_GPU                                               25
#define     PWRGATE_DIG_ACK_DISPLAY                                           21
#define     PWRGATE_DIG_ACK_DSP_GPU_DISP_MASK                                 (0x31<<21)
#define     PWRGATE_DIG_ACK_BT_MASK                                           (0x1<<28)

#endif /* _ACTIONS_SOC_REGS_H_	*/
