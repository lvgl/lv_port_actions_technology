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
#define PWM_CLK0_BASE               0x40001054





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
#define DSPPLL_CTL					(CMUA_REG_BASE + 0x24)
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
#define JTAG_CTL					(GPIO_REG_BASE + 0x400)
#define DEBUGIE0					(GPIO_REG_BASE + 0x420)
#define DEBUGOE0					(GPIO_REG_BASE + 0x430)


// brom api address
//fpga
//#define SPINOR_API_ADDR				(0x00005860) //(0x00005814)


#define SPINOR_API_ADDR				(p_brom_api->p_spinor_api)


// Interaction RAM
#define INTER_RAM_ADDR				(0x2FF18000)
#define DSP_SHARE_RAM_START_ADDR	(0x2FF1A000)		/* Confirm with DSP, not reflect in linker.ld, can't change */
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
//--------------Register Address---------------------------------------//
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
#define     EFUSE_DATA12                                                      (EFUSE_BASE+0x40)
#define     EFUSE_DATA13                                                      (EFUSE_BASE+0x44)
#define     EFUSE_DATA14                                                      (EFUSE_BASE+0x48)
#define     EFUSE_DATA15                                                      (EFUSE_BASE+0x4C)
#define     EFUSE_DATA16                                                      (EFUSE_BASE+0x50)
#define     EFUSE_DATA17                                                      (EFUSE_BASE+0x54)
#define     EFUSE_DATA18                                                      (EFUSE_BASE+0x58)
#define     EFUSE_DATA19                                                      (EFUSE_BASE+0x5C)
#define     EFUSE_DATA20                                                      (EFUSE_BASE+0x60)
#define     EFUSE_DATA21                                                      (EFUSE_BASE+0x64)
#define     EFUSE_DATA22                                                      (EFUSE_BASE+0x68)
#define     EFUSE_DATA23                                                      (EFUSE_BASE+0x6C)
#define     EFUSE_DATA24                                                      (EFUSE_BASE+0x70)
#define     EFUSE_DATA25                                                      (EFUSE_BASE+0x74)
#define     EFUSE_DATA26                                                      (EFUSE_BASE+0x78)
#define     EFUSE_DATA27                                                      (EFUSE_BASE+0x7C)
#define     EFUSE_DATA28                                                      (EFUSE_BASE+0x80)
#define     EFUSE_DATA29                                                      (EFUSE_BASE+0x84)
#define     EFUSE_DATA30                                                      (EFUSE_BASE+0x88)
#define     EFUSE_DATA31                                                      (EFUSE_BASE+0x8C)

//--------------Bits Location------------------------------------------//
#define     EFUSE_CTL0_RE_LOAD_AUTO                                           6
#define     EFUSE_CTL0_CHIPID_CLKD_e                                          5
#define     EFUSE_CTL0_CHIPID_CLKD_SHIFT                                      3
#define     EFUSE_CTL0_CHIPID_CLKD_MASK                                       (0x7<<3)
#define     EFUSE_CTL0_RE_LOAD                                                2
#define     EFUSE_CTL0_RD_PSD_OK                                              1
#define     EFUSE_CTL0_DATA_READY                                             0

#define     EFUSE_CTL1_RDEN                                                   23
#define     EFUSE_CTL1_EFUSE25_VOL_e                                          22
#define     EFUSE_CTL1_EFUSE25_VOL_SHIFT                                      20
#define     EFUSE_CTL1_EFUSE25_VOL_MASK                                       (0x7<<20)
#define     EFUSE_CTL1_PGM_TIMER_e                                            19
#define     EFUSE_CTL1_PGM_TIMER_SHIFT                                        17
#define     EFUSE_CTL1_PGM_TIMER_MASK                                         (0x7<<17)
#define     EFUSE_CTL1_PGM_AUTO_EN                                            16
#define     EFUSE_CTL1_EFUSE25_PD_e                                           15
#define     EFUSE_CTL1_EFUSE25_PD_SHIFT                                       14
#define     EFUSE_CTL1_EFUSE25_PD_MASK                                        (0x3<<14)
#define     EFUSE_CTL1_EN_EFUSE25                                             13
#define     EFUSE_CTL1_WR_PSD_OK                                              12
#define     EFUSE_CTL1_AEN                                                    11
#define     EFUSE_CTL1_PGMEN                                                  10
#define     EFUSE_CTL1_ADDRESS_e                                              9
#define     EFUSE_CTL1_ADDRESS_SHIFT                                          0
#define     EFUSE_CTL1_ADDRESS_MASK                                           (0x3FF<<0)

#define     EFUSE_CTL2_PASSWORD_e                                             31
#define     EFUSE_CTL2_PASSWORD_SHIFT                                         0
#define     EFUSE_CTL2_PASSWORD_MASK                                          (0xFFFFFFFF<<0)

#define     EFUSE_DATA0_EFUSE_DATA0_e                                         31
#define     EFUSE_DATA0_EFUSE_DATA0_SHIFT                                     0
#define     EFUSE_DATA0_EFUSE_DATA0_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA1_EFUSE_DATA1_e                                         31
#define     EFUSE_DATA1_EFUSE_DATA1_SHIFT                                     0
#define     EFUSE_DATA1_EFUSE_DATA1_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA2_EFUSE_DATA2_e                                         31
#define     EFUSE_DATA2_EFUSE_DATA2_SHIFT                                     0
#define     EFUSE_DATA2_EFUSE_DATA2_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA3_EFUSE_DATA3_e                                         31
#define     EFUSE_DATA3_EFUSE_DATA3_SHIFT                                     0
#define     EFUSE_DATA3_EFUSE_DATA3_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA4_EFUSE_DATA4_e                                         31
#define     EFUSE_DATA4_EFUSE_DATA4_SHIFT                                     0
#define     EFUSE_DATA4_EFUSE_DATA4_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA5_EFUSE_DATA5_e                                         31
#define     EFUSE_DATA5_EFUSE_DATA5_SHIFT                                     0
#define     EFUSE_DATA5_EFUSE_DATA5_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA6_EFUSE_DATA6_e                                         31
#define     EFUSE_DATA6_EFUSE_DATA6_SHIFT                                     0
#define     EFUSE_DATA6_EFUSE_DATA6_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA7_EFUSE_DATA7_e                                         31
#define     EFUSE_DATA7_EFUSE_DATA7_SHIFT                                     0
#define     EFUSE_DATA7_EFUSE_DATA7_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA8_EFUSE_DATA8_e                                         31
#define     EFUSE_DATA8_EFUSE_DATA8_SHIFT                                     0
#define     EFUSE_DATA8_EFUSE_DATA8_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA9_EFUSE_DATA9_e                                         31
#define     EFUSE_DATA9_EFUSE_DATA9_SHIFT                                     0
#define     EFUSE_DATA9_EFUSE_DATA9_MASK                                      (0xFFFFFFFF<<0)

#define     EFUSE_DATA10_EFUSE_DATA10_e                                       31
#define     EFUSE_DATA10_EFUSE_DATA10_SHIFT                                   0
#define     EFUSE_DATA10_EFUSE_DATA10_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA11_EFUSE_DATA11_e                                       31
#define     EFUSE_DATA11_EFUSE_DATA11_SHIFT                                   0
#define     EFUSE_DATA11_EFUSE_DATA11_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA12_EFUSE_DATA12_e                                       31
#define     EFUSE_DATA12_EFUSE_DATA12_SHIFT                                   0
#define     EFUSE_DATA12_EFUSE_DATA12_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA13_EFUSE_DATA13_e                                       31
#define     EFUSE_DATA13_EFUSE_DATA13_SHIFT                                   0
#define     EFUSE_DATA13_EFUSE_DATA13_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA14_EFUSE_DATA14_e                                       31
#define     EFUSE_DATA14_EFUSE_DATA14_SHIFT                                   0
#define     EFUSE_DATA14_EFUSE_DATA14_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA15_EFUSE_DATA15_e                                       31
#define     EFUSE_DATA15_EFUSE_DATA15_SHIFT                                   0
#define     EFUSE_DATA15_EFUSE_DATA15_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA16_EFUSE_DATA16_e                                       31
#define     EFUSE_DATA16_EFUSE_DATA16_SHIFT                                   0
#define     EFUSE_DATA16_EFUSE_DATA16_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA17_EFUSE_DATA17_e                                       31
#define     EFUSE_DATA17_EFUSE_DATA17_SHIFT                                   0
#define     EFUSE_DATA17_EFUSE_DATA17_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA18_EFUSE_DATA18_e                                       31
#define     EFUSE_DATA18_EFUSE_DATA18_SHIFT                                   0
#define     EFUSE_DATA18_EFUSE_DATA18_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA19_EFUSE_DATA19_e                                       31
#define     EFUSE_DATA19_EFUSE_DATA19_SHIFT                                   0
#define     EFUSE_DATA19_EFUSE_DATA19_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA20_EFUSE_DATA20_e                                       31
#define     EFUSE_DATA20_EFUSE_DATA20_SHIFT                                   0
#define     EFUSE_DATA20_EFUSE_DATA20_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA21_EFUSE_DATA21_e                                       31
#define     EFUSE_DATA21_EFUSE_DATA21_SHIFT                                   0
#define     EFUSE_DATA21_EFUSE_DATA21_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA22_EFUSE_DATA22_e                                       31
#define     EFUSE_DATA22_EFUSE_DATA22_SHIFT                                   0
#define     EFUSE_DATA22_EFUSE_DATA22_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA23_EFUSE_DATA23_e                                       31
#define     EFUSE_DATA23_EFUSE_DATA23_SHIFT                                   0
#define     EFUSE_DATA23_EFUSE_DATA23_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA24_EFUSE_DATA24_e                                       31
#define     EFUSE_DATA24_EFUSE_DATA24_SHIFT                                   0
#define     EFUSE_DATA24_EFUSE_DATA24_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA25_EFUSE_DATA25_e                                       31
#define     EFUSE_DATA25_EFUSE_DATA25_SHIFT                                   0
#define     EFUSE_DATA25_EFUSE_DATA25_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA26_EFUSE_DATA26_e                                       31
#define     EFUSE_DATA26_EFUSE_DATA26_SHIFT                                   0
#define     EFUSE_DATA26_EFUSE_DATA26_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA27_EFUSE_DATA27_e                                       31
#define     EFUSE_DATA27_EFUSE_DATA27_SHIFT                                   0
#define     EFUSE_DATA27_EFUSE_DATA27_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA28_EFUSE_DATA28_e                                       31
#define     EFUSE_DATA28_EFUSE_DATA28_SHIFT                                   0
#define     EFUSE_DATA28_EFUSE_DATA28_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA29_EFUSE_DATA29_e                                       31
#define     EFUSE_DATA29_EFUSE_DATA29_SHIFT                                   0
#define     EFUSE_DATA29_EFUSE_DATA29_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA30_EFUSE_DATA30_e                                       31
#define     EFUSE_DATA30_EFUSE_DATA30_SHIFT                                   0
#define     EFUSE_DATA30_EFUSE_DATA30_MASK                                    (0xFFFFFFFF<<0)

#define     EFUSE_DATA31_EFUSE_DATA31_e                                       31
#define     EFUSE_DATA31_EFUSE_DATA31_SHIFT                                   0
#define     EFUSE_DATA31_EFUSE_DATA31_MASK                                    (0xFFFFFFFF<<0)


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

/*spi1*/
#define     SPI1_REGISTER_BASE                                                0x4002C000
#define     SPI1_CTL                                                          (SPI1_REGISTER_BASE+0x0000)
#define     SPI1_DDR_MODE_CTL                                                 (SPI1_REGISTER_BASE+0x0018)
#define     SPI1_CLKGATING                                                    (SPI1_REGISTER_BASE+0x0038)
#define     SPI1_DELAYCHAIN                                                   (SPI1_REGISTER_BASE+0x003C)

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

#define UART2_CTL			(UART2_REG_BASE+0x00)
#define UART2_RXDAT			(UART2_REG_BASE+0x04)
#define UART2_TXDAT			(UART2_REG_BASE+0x08)
#define UART2_STA			(UART2_REG_BASE+0x0c)
#define UART2_BR			(UART2_REG_BASE+0x10)

/* FCC add start */
#define     CMU_MEMCLKSRC0_SRAMCLKDIV_e                                       29
#define     CMU_MEMCLKSRC0_SRAMCLKDIV_SHIFT                                   28
#define     CMU_MEMCLKSRC0_SRAMCLKDIV_MASK                                    (0x3<<28)
#define     CMU_MEMCLKSRC0_SRAMCLKSRC_e                                       27
#define     CMU_MEMCLKSRC0_SRAMCLKSRC_SHIFT                                   25
#define     CMU_MEMCLKSRC0_SRAMCLKSRC_MASK                                    (0x7<<25)
#define     CMU_MEMCLKSRC0_RAM16CLKSRC_e                                      23
#define     CMU_MEMCLKSRC0_RAM16CLKSRC_SHIFT                                  22
#define     CMU_MEMCLKSRC0_RAM16CLKSRC_MASK                                   (0x3<<22)
#define     CMU_MEMCLKSRC0_RAM15CLKSRC_e                                      21
#define     CMU_MEMCLKSRC0_RAM15CLKSRC_SHIFT                                  20
#define     CMU_MEMCLKSRC0_RAM15CLKSRC_MASK                                   (0x3<<20)
#define     CMU_MEMCLKSRC0_RAM14CLKSRC_e                                      19
#define     CMU_MEMCLKSRC0_RAM14CLKSRC_SHIFT                                  18
#define     CMU_MEMCLKSRC0_RAM14CLKSRC_MASK                                   (0x3<<18)
#define     CMU_MEMCLKSRC0_RAM13CLKSRC_e                                      17
#define     CMU_MEMCLKSRC0_RAM13CLKSRC_SHIFT                                  16
#define     CMU_MEMCLKSRC0_RAM13CLKSRC_MASK                                   (0x3<<16)
#define     CMU_MEMCLKSRC0_RAM12CLKSRC_e                                      15
#define     CMU_MEMCLKSRC0_RAM12CLKSRC_SHIFT                                  14
#define     CMU_MEMCLKSRC0_RAM12CLKSRC_MASK                                   (0x3<<14)
#define     CMU_MEMCLKSRC0_RAM11CLKSRC_e                                      13
#define     CMU_MEMCLKSRC0_RAM11CLKSRC_SHIFT                                  12
#define     CMU_MEMCLKSRC0_RAM11CLKSRC_MASK                                   (0x3<<12)
#define     CMU_MEMCLKSRC0_RAM10CLKSRC                                        11
#define     CMU_MEMCLKSRC0_RAM9CLKSRC                                         10
#define     CMU_MEMCLKSRC0_RAM8CLKSRC                                         9
#define     CMU_MEMCLKSRC0_RAM7CLKSRC                                         8
#define     CMU_MEMCLKSRC0_RAM6CLKSRC                                         7
#define     CMU_MEMCLKSRC0_RAM5CLKSRC                                         6
#define     CMU_MEMCLKSRC0_RAM4CLKSRC                                         5
#define     CMU_MEMCLKSRC0_RAM3CLKSRC                                         4
#define     CMU_MEMCLKSRC0_RAM2CLKSRC                                         3
#define     CMU_MEMCLKSRC0_RAM1CLKSRC                                         2
#define     CMU_MEMCLKSRC0_RAM0CLKSRC                                         1

#define     CMU_MEMCLKSRC1_JPEG_OUTRAM3CLKSRC                                 28
#define     CMU_MEMCLKSRC1_JPEG_OUTRAM2CLKSRC                                 27
#define     CMU_MEMCLKSRC1_JPEG_OUTRAM1CLKSRC                                 26
#define     CMU_MEMCLKSRC1_JPEG_OUTRAM0CLKSRC                                 25
#define     CMU_MEMCLKSRC1_JPEGRAMCLKSRC                                      24
#define     CMU_MEMCLKSRC1_BTRAMPRECLKDIV_e                                   21
#define     CMU_MEMCLKSRC1_BTRAMPRECLKDIV_SHIFT                               20
#define     CMU_MEMCLKSRC1_BTRAMPRECLKDIV_MASK                                (0x3<<20)
#define     CMU_MEMCLKSRC1_BTRAMPRECLKSRC_e                                   19
#define     CMU_MEMCLKSRC1_BTRAMPRECLKSRC_SHIFT                               17
#define     CMU_MEMCLKSRC1_BTRAMPRECLKSRC_MASK                                (0x7<<17)
#define     CMU_MEMCLKSRC1_BTRAMCLKSRC                                        16
#define     CMU_MEMCLKSRC1_USBRAMPRECLKDIV_e                                  13
#define     CMU_MEMCLKSRC1_USBRAMPRECLKDIV_SHIFT                              12
#define     CMU_MEMCLKSRC1_USBRAMPRECLKDIV_MASK                               (0x3<<12)
#define     CMU_MEMCLKSRC1_USBRAMPRECLKSRC_e                                  11
#define     CMU_MEMCLKSRC1_USBRAMPRECLKSRC_SHIFT                              9
#define     CMU_MEMCLKSRC1_USBRAMPRECLKSRC_MASK                               (0x7<<9)
#define     CMU_MEMCLKSRC1_USBRAMCLKSRC                                       8
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKDIV_e                             5
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKDIV_SHIFT                         4
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKDIV_MASK                          (0x3<<4)
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKSRC_e                             3
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKSRC_SHIFT                         1
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMPRECLKSRC_MASK                          (0x7<<1)
#define     CMU_MEMCLKSRC1_AUDDSPTCRAMCLKSRC                                  0

#define     MRCR1_BT_HUB_RESET                                                24
/* FCC add end */
#endif /* _ACTIONS_SOC_REGS_H_	*/
