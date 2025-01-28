#ifndef __SOC_REGS_H
#define __SOC_REGS_H

//--------------RMU-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     RMU_BASE                                                          0x40000000
#define     MRCR0                                                             (RMU_BASE+0x00000000)
#define     MRCR1                                                             (RMU_BASE+0x00000004)

//--------------MemoryController-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     MemoryController_BASE                                             0x40010000
#define     MEMORYCTL                                                         (MemoryController_BASE+0x00000000)
#define     CPU_ERROR_ADDR                                                    (MemoryController_BASE+0x00000008)
#define     MEMORYCTL2                                                        (MemoryController_BASE+0x00000020)
#define     DBGCTL                                                            (MemoryController_BASE+0x00000040)
#define     DSPPAGEADDR0                                                      (MemoryController_BASE+0x00000080)
#define     DSPPAGEADDR1                                                      (MemoryController_BASE+0x00000084)
#define     DSPPAGEADDR2                                                      (MemoryController_BASE+0x00000088)
#define     DSPPAGEADDR3                                                      (MemoryController_BASE+0x0000008c)
#define     MPUIE                                                             (MemoryController_BASE+0x00000100)
#define     MPUIP                                                             (MemoryController_BASE+0x00000104)
#define     MPUCTL0                                                           (MemoryController_BASE+0x00000110)
#define     MPUBASE0                                                          (MemoryController_BASE+0x00000114)
#define     MPUEND0                                                           (MemoryController_BASE+0x00000118)
#define     MPUERRADDR0                                                       (MemoryController_BASE+0x0000011c)
#define     MPUCTL1                                                           (MemoryController_BASE+0x00000120)
#define     MPUBASE1                                                          (MemoryController_BASE+0x00000124)
#define     MPUEND1                                                           (MemoryController_BASE+0x00000128)
#define     MPUERRADDR1                                                       (MemoryController_BASE+0x0000012c)
#define     MPUCTL2                                                           (MemoryController_BASE+0x00000130)
#define     MPUBASE2                                                          (MemoryController_BASE+0x00000134)
#define     MPUEND2                                                           (MemoryController_BASE+0x00000138)
#define     MPUERRADDR2                                                       (MemoryController_BASE+0x0000013c)
#define     MPUCTL3                                                           (MemoryController_BASE+0x00000140)
#define     MPUBASE3                                                          (MemoryController_BASE+0x00000144)
#define     MPUEND3                                                           (MemoryController_BASE+0x00000148)
#define     MPUERRADDR3                                                       (MemoryController_BASE+0x0000014c)
#define     BIST_EN0                                                          (MemoryController_BASE+0x00000200)
#define     BIST_FIN0                                                         (MemoryController_BASE+0x00000204)
#define     BIST_INFO0                                                        (MemoryController_BASE+0x00000208)
#define     BIST_EN1                                                          (MemoryController_BASE+0x0000020c)
#define     BIST_FIN1                                                         (MemoryController_BASE+0x00000210)
#define     BIST_INFO1                                                        (MemoryController_BASE+0x00000214)
#define     SPI_CACHE_MAPPING_ADDR0                                           (MemoryController_BASE+0x00000300)
#define     SPI_CACHE_ADDR0_ENTRY                                             (MemoryController_BASE+0x00000304)
#define     SPI_CACHE_MAPPING_ADDR1                                           (MemoryController_BASE+0x00000308)
#define     SPI_CACHE_ADDR1_ENTRY                                             (MemoryController_BASE+0x0000030c)
#define     SPI_CACHE_MAPPING_ADDR2                                           (MemoryController_BASE+0x00000310)
#define     SPI_CACHE_ADDR2_ENTRY                                             (MemoryController_BASE+0x00000314)
#define     SPI_CACHE_MAPPING_ADDR3                                           (MemoryController_BASE+0x00000318)
#define     SPI_CACHE_ADDR3_ENTRY                                             (MemoryController_BASE+0x0000031c)
#define     SPI_CACHE_MAPPING_ADDR4                                           (MemoryController_BASE+0x00000320)
#define     SPI_CACHE_ADDR4_ENTRY                                             (MemoryController_BASE+0x00000324)
#define     SPI_CACHE_MAPPING_ADDR5                                           (MemoryController_BASE+0x00000328)
#define     SPI_CACHE_ADDR5_ENTRY                                             (MemoryController_BASE+0x0000032c)
#define     SPI_CACHE_MAPPING_ADDR6                                           (MemoryController_BASE+0x00000330)
#define     SPI_CACHE_ADDR6_ENTRY                                             (MemoryController_BASE+0x00000334)
#define     SPI_CACHE_MAPPING_ADDR7                                           (MemoryController_BASE+0x00000338)
#define     SPI_CACHE_ADDR7_ENTRY                                             (MemoryController_BASE+0x0000033c)

//--------------CMU_ANALOG_REGISTER-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     CMU_ANALOG_REGISTER_BASE																					0x40000100                                          
#define     HOSC_CTL                                                          (CMU_ANALOG_REGISTER_BASE+0x00)
#define     HOSCLDO_CTL                                                       (CMU_ANALOG_REGISTER_BASE+0x04)
#define     RC64M_CTL                                                         (CMU_ANALOG_REGISTER_BASE+0x08)
#define     RC96M_CTL                                                         (CMU_ANALOG_REGISTER_BASE+0x0C)
#define     LOSC_CTL                                                          (CMU_ANALOG_REGISTER_BASE+0x10)
#define     RC4M_CTL                                                          (CMU_ANALOG_REGISTER_BASE+0x14)
#define     RC128M_CTL                                                        (CMU_ANALOG_REGISTER_BASE+0x18)
#define     AVDDLDO_CTL                                                       (CMU_ANALOG_REGISTER_BASE+0x1C)
#define     COREPLL_CTL                                                       (CMU_ANALOG_REGISTER_BASE+0x20)
#define     AUDIOPLL0_CTL                                                     (CMU_ANALOG_REGISTER_BASE+0x28)
#define     AUDIOPLL1_CTL                                                     (CMU_ANALOG_REGISTER_BASE+0x2c)
#define     COREPLL_DEBUG                                                     (CMU_ANALOG_REGISTER_BASE+0x40)
#define     AUDIOPLL0_DEBUG                                                   (CMU_ANALOG_REGISTER_BASE+0x48)
#define     AUDIOPLL1_DEBUG                                                   (CMU_ANALOG_REGISTER_BASE+0x4c)
#define     CMU_ANADEBUG                                                      (CMU_ANALOG_REGISTER_BASE+0x60)
#define     RC32K_CTL                                                         (CMU_ANALOG_REGISTER_BASE+0x64)
#define     RC32K_CAL                                                         (CMU_ANALOG_REGISTER_BASE+0x68)
#define     RC32K_COUNT                                                       (CMU_ANALOG_REGISTER_BASE+0x6C)
#define     HOSCOK_CTL                                                        (CMU_ANALOG_REGISTER_BASE+0x70)

//--------------Bits Location------------------------------------------//
#define     HOSC_CTL_HOSCI_CAP_e                                              31
#define     HOSC_CTL_HOSCI_CAP_SHIFT                                          24
#define     HOSC_CTL_HOSCI_CAP_MASK                                           (0xFF<<24)
#define     HOSC_CTL_HOSCO_CAP_e                                              23
#define     HOSC_CTL_HOSCO_CAP_SHIFT                                          16
#define     HOSC_CTL_HOSCO_CAP_MASK                                           (0xFF<<16)
#define     HOSC_CTL_HOSC_AMPLMT_e                                            14
#define     HOSC_CTL_HOSC_AMPLMT_SHIFT                                        13
#define     HOSC_CTL_HOSC_AMPLMT_MASK                                         (0x3<<13)
#define     HOSC_CTL_HOSC_AMPLMT_EN                                           12
#define     HOSC_CTL_HGMC_e                                                   9
#define     HOSC_CTL_HGMC_SHIFT                                               8
#define     HOSC_CTL_HGMC_MASK                                                (0x3<<8)
#define     HOSC_CTL_HOSC_VDD_SEL                                             5
#define     HOSC_CTL_HOSC_BUF_SEL                                             4

#define     HOSCLDO_CTL_HOSC_CAPTUNE_DONE                                     25
#define     HOSCLDO_CTL_HOSC_CAPTUNE_S                                        24
#define     HOSCLDO_CTL_HOSC_CAP_CODE_e                                       23
#define     HOSCLDO_CTL_HOSC_CAP_CODE_SHIFT                                   16
#define     HOSCLDO_CTL_HOSC_CAP_CODE_MASK                                    (0xFF<<16)
#define     HOSCLDO_CTL_OSCVDD0_PD_BIASSET                                    5
#define     HOSCLDO_CTL_OSCVDD0_PD                                            4
#define     HOSCLDO_CTL_OSCVDD0_VSET_e                                        3
#define     HOSCLDO_CTL_OSCVDD0_VSET_SHIFT                                    2
#define     HOSCLDO_CTL_OSCVDD0_VSET_MASK                                     (0x3<<2)
#define     HOSCLDO_CTL_OSCVDD0_EN                                            0

#define     RC64M_CTL_CALDONE                                                 12
#define     RC64M_CTL_FRQSET_e                                                11
#define     RC64M_CTL_FRQSET_SHIFT                                            6
#define     RC64M_CTL_FRQSET_MASK                                             (0x3F<<6)
#define     RC64M_CTL_TUNE_MODE                                               5
#define     RC64M_CTL_EN_AUTOCAL                                              4
#define     RC64M_CTL_INRANGE_e                                               3
#define     RC64M_CTL_INRANGE_SHIFT                                           2
#define     RC64M_CTL_INRANGE_MASK                                            (0x3<<2)
#define     RC64M_CTL_REFCLKSEL                                               1

#define     RC96M_CTL_CALDONE                                                 12
#define     RC96M_CTL_FRQSET_e                                                11
#define     RC96M_CTL_FRQSET_SHIFT                                            6
#define     RC96M_CTL_FRQSET_MASK                                             (0x3F<<6)
#define     RC96M_CTL_TUNE_MODE                                               5
#define     RC96M_CTL_EN_AUTOCAL                                              4
#define     RC96M_CTL_INRANGE_e                                               3
#define     RC96M_CTL_INRANGE_SHIFT                                           2
#define     RC96M_CTL_INRANGE_MASK                                            (0x3<<2)
#define     RC96M_CTL_REFCLKSEL                                               1
#define     RC96M_CTL_RC96M_EN                                                0

#define     LOSC_CTL_LOSCI_CAP_e                                              20
#define     LOSC_CTL_LOSCI_CAP_SHIFT                                          16
#define     LOSC_CTL_LOSCI_CAP_MASK                                           (0x1F<<16)
#define     LOSC_CTL_LOSC_RDY                                                 15
#define     LOSC_CTL_LOSCO_CAP_e                                              12
#define     LOSC_CTL_LOSCO_CAP_SHIFT                                          8
#define     LOSC_CTL_LOSCO_CAP_MASK                                           (0x1F<<8)
#define     LOSC_CTL_LGMC_e                                                   6
#define     LOSC_CTL_LGMC_SHIFT                                               4
#define     LOSC_CTL_LGMC_MASK                                                (0x7<<4)
#define     LOSC_CTL_LOSC_EN                                                  0

#define     RC4M_CTL_RC4M_OPTION_e                                            31
#define     RC4M_CTL_RC4M_OPTION_SHIFT                                        7
#define     RC4M_CTL_RC4M_OPTION_MASK                                         (0x1FFFFFF<<7)
#define     RC4M_CTL_RC4M_FRQSET_e                                            6
#define     RC4M_CTL_RC4M_FRQSET_SHIFT                                        1
#define     RC4M_CTL_RC4M_FRQSET_MASK                                         (0x3F<<1)

#define     RC128M_CTL_CALDONE                                                12
#define     RC128M_CTL_FRQSET_e                                               11
#define     RC128M_CTL_FRQSET_SHIFT                                           6
#define     RC128M_CTL_FRQSET_MASK                                            (0x3F<<6)
#define     RC128M_CTL_TUNE_MODE                                              5
#define     RC128M_CTL_EN_AUTOCAL                                             4
#define     RC128M_CTL_INRANGE_e                                              3
#define     RC128M_CTL_INRANGE_SHIFT                                          2
#define     RC128M_CTL_INRANGE_MASK                                           (0x3<<2)
#define     RC128M_CTL_REFCLKSEL                                              1
#define     RC128M_CTL_RC128M_EN                                              0

#define     AVDDLDO_CTL_AVDD_PD_BIASSET                                       5
#define     AVDDLDO_CTL_AVDD_PD_EN                                            4
#define     AVDDLDO_CTL_AVDD_VSET_e                                           3
#define     AVDDLDO_CTL_AVDD_VSET_SHIFT                                       2
#define     AVDDLDO_CTL_AVDD_VSET_MASK                                        (0x3<<2)
#define     AVDDLDO_CTL_AVDD_EN                                               0

#define     COREPLL_CTL_COREPLLPMD                                            8
#define     COREPLL_CTL_COREPLLEN                                             7
#define     COREPLL_CTL_COREPLLSCORE_e                                        5
#define     COREPLL_CTL_COREPLLSCORE_SHIFT                                    0
#define     COREPLL_CTL_COREPLLSCORE_MASK                                     (0x3F<<0)

#define     AUDIOPLL0_CTL_AUDPLL0PMD                                          8
#define     AUDIOPLL0_CTL_AUDPLL0EN                                           4
#define     AUDIOPLL0_CTL_APS0_e                                              3
#define     AUDIOPLL0_CTL_APS0_SHIFT                                          0
#define     AUDIOPLL0_CTL_APS0_MASK                                           (0xF<<0)

#define     AUDIOPLL1_CTL_AUDPLL1PMD                                          8
#define     AUDIOPLL1_CTL_AUDPLL1EN                                           4
#define     AUDIOPLL1_CTL_APS1_e                                              3
#define     AUDIOPLL1_CTL_APS1_SHIFT                                          0
#define     AUDIOPLL1_CTL_APS1_MASK                                           (0xF<<0)

#define     RC32K_CTL_RC32K_FRQSET_e                                          6
#define     RC32K_CTL_RC32K_FRQSET_SHIFT                                      1
#define     RC32K_CTL_RC32K_FRQSET_MASK                                       (0x3F<<1)

#define     RC32K_CAL_CALCYCLE_e                                              23
#define     RC32K_CAL_CALCYCLE_SHIFT                                          8
#define     RC32K_CAL_CALCYCLE_MASK                                           (0xFFFF<<8)
#define     RC32K_CAL_CALDONE                                                 4
#define     RC32K_CAL_CALEN                                                   0

#define     RC32K_COUNT_CALCOUNT_e                                            31
#define     RC32K_COUNT_CALCOUNT_SHIFT                                        0
#define     RC32K_COUNT_CALCOUNT_MASK                                         (0xFFFFFFFF<<0)

#define     HOSCOK_CTL_OSCVDD_OK                                              17
#define     HOSCOK_CTL_HOSC_OK                                                16
#define     HOSCOK_CTL_16M_COUNT_DELAY_e                                      12
#define     HOSCOK_CTL_16M_COUNT_DELAY_SHIFT                                  10
#define     HOSCOK_CTL_16M_COUNT_DELAY_MASK                                   (0x7<<10)
#define     HOSCOK_CTL_16M_COUNT_INTERVAL_e                                   9
#define     HOSCOK_CTL_16M_COUNT_INTERVAL_SHIFT                               8
#define     HOSCOK_CTL_16M_COUNT_INTERVAL_MASK                                (0x3<<8)
#define     HOSCOK_CTL_16M_COUNT_REPEAT_e                                     7
#define     HOSCOK_CTL_16M_COUNT_REPEAT_SHIFT                                 6
#define     HOSCOK_CTL_16M_COUNT_REPEAT_MASK                                  (0x3<<6)
#define     HOSCOK_CTL_16M_COUNT_DIFFER_e                                     5
#define     HOSCOK_CTL_16M_COUNT_DIFFER_SHIFT                                 4
#define     HOSCOK_CTL_16M_COUNT_DIFFER_MASK                                  (0x3<<4)
#define     HOSCOK_CTL_HOSCOK_DELAY_e                                         3
#define     HOSCOK_CTL_HOSCOK_DELAY_SHIFT                                     2
#define     HOSCOK_CTL_HOSCOK_DELAY_MASK                                      (0x3<<2)
#define     HOSCOK_CTL_HOSCOK_SRC                                             0

//--------------CMU_DIGITAL_REGISTER-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     CMU_DIGITAL_REGISTER_BASE                                         0x40001000
#define     CMU_SYSCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x0000)
#define     CMU_DEVCLKEN0                                                     (CMU_DIGITAL_REGISTER_BASE+0x0004)
#define     CMU_DEVCLKEN1                                                     (CMU_DIGITAL_REGISTER_BASE+0x0008)
#define     CMU_SD0CLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x0010)
#define     CMU_SD1CLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x0014)
#define     CMU_SPI0CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0020)
#define     CMU_SPI1CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0024)
#define     CMU_SPI2CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0028)
#define     CMU_SPI3CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x002C)
#define     CMU_DECLK                                                         (CMU_DIGITAL_REGISTER_BASE+0x0030)
#define     CMU_LCDCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x0034)
#define     CMU_SECCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x003C)
#define     CMU_TIMER0CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0040)
#define     CMU_TIMER1CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0044)
#define     CMU_TIMER2CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0048)
#define     CMU_TIMER3CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x004c)
#define     CMU_TIMER4CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0050)
#define     CMU_PWM0CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0054)
#define     CMU_PWM1CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0058)
#define     CMU_PWM2CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x005C)
#define     CMU_PWM3CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0060)
#define     CMU_PWM4CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0064)
#define     CMU_PWM5CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0068)
#define     CMU_PWM6CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x006C)
#define     CMU_PWM7CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0070)
#define     CMU_PWM8CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0074)
#define     CMU_LRADCCLK                                                      (CMU_DIGITAL_REGISTER_BASE+0x0078)
#define     CMU_CECCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x007C)
#define     CMU_I2C0CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0080)
#define     CMU_I2C1CLK                                                       (CMU_DIGITAL_REGISTER_BASE+0x0084)
#define     CMU_DACCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x0098)
#define     CMU_ADCCLK                                                        (CMU_DIGITAL_REGISTER_BASE+0x009c)
#define     CMU_I2STXCLK                                                      (CMU_DIGITAL_REGISTER_BASE+0x00A0)
#define     CMU_I2SRXCLK                                                      (CMU_DIGITAL_REGISTER_BASE+0x00A4)
#define     CMU_SPDIFTXCLK                                                    (CMU_DIGITAL_REGISTER_BASE+0x00A8)
#define     CMU_SPDIFRXCLK                                                    (CMU_DIGITAL_REGISTER_BASE+0x00AC)
#define     CMU_MEMCLKEN0                                                     (CMU_DIGITAL_REGISTER_BASE+0x00B0)
#define     CMU_MEMCLKEN1                                                     (CMU_DIGITAL_REGISTER_BASE+0x00B4)
#define     CMU_MEMCLKSRC0                                                    (CMU_DIGITAL_REGISTER_BASE+0x00C0)
#define     CMU_MEMCLKSRC1                                                    (CMU_DIGITAL_REGISTER_BASE+0x00C4)
#define     CMU_S1CLKCTL                                                      (CMU_DIGITAL_REGISTER_BASE+0x00D0)
#define     CMU_S1BTCLKCTL                                                    (CMU_DIGITAL_REGISTER_BASE+0x00D4)
#define     CMU_S2CLKCTL                                                      (CMU_DIGITAL_REGISTER_BASE+0x00DC)
#define     CMU_S3CLKCTL                                                      (CMU_DIGITAL_REGISTER_BASE+0x00E0)
#define     CMU_PMUWKUPCLKCTL                                                 (CMU_DIGITAL_REGISTER_BASE+0x00EC)
#define     CMU_DIGITALDEBUG                                                  (CMU_DIGITAL_REGISTER_BASE+0x00F0)
#define     CMU_TST_CTL                                                       (CMU_DIGITAL_REGISTER_BASE+0x00F4)
#define     CMU_SPIMT0CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0100)
#define     CMU_SPIMT1CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0104)
#define     CMU_I2CMT0CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0110)
#define     CMU_I2CMT1CLK                                                     (CMU_DIGITAL_REGISTER_BASE+0x0114)

//--------------Bits Location------------------------------------------//
#define     CMU_SYSCLK_CORE_DEEPSLP_DIS                                       25
#define     CMU_SYSCLK_CPUTIMERCLKSRC                                         24
#define     CMU_SYSCLK_AHBCLKDIV_e                                            9
#define     CMU_SYSCLK_AHBCLKDIV_SHIFT                                        8
#define     CMU_SYSCLK_AHBCLKDIV_MASK                                         (0x3<<8)
#define     CMU_SYSCLK_CPUCLKDIV_e                                            7
#define     CMU_SYSCLK_CPUCLKDIV_SHIFT                                        4
#define     CMU_SYSCLK_CPUCLKDIV_MASK                                         (0xF<<4)
#define     CMU_SYSCLK_CORECLKSRC_e                                           2
#define     CMU_SYSCLK_CORECLKSRC_SHIFT                                       0
#define     CMU_SYSCLK_CORECLKSRC_MASK                                        (0x7<<0)

#define     CMU_DEVCLKEN0_EFUSECLKEN                                          31
#define     CMU_DEVCLKEN0_EXINTCLKEN                                          30
#define     CMU_DEVCLKEN0_I2C1CLKEN                                           28
#define     CMU_DEVCLKEN0_I2C0CLKEN                                           27
#define     CMU_DEVCLKEN0_UART2CLKEN                                          26
#define     CMU_DEVCLKEN0_UART1CLKEN                                          25
#define     CMU_DEVCLKEN0_UART0CLKEN                                          24
#define     CMU_DEVCLKEN0_CECCLKEN                                            23
#define     CMU_DEVCLKEN0_CPUTIMERCLKEN                                       20
#define     CMU_DEVCLKEN0_LRADCCLKEN                                          19
#define     CMU_DEVCLKEN0_TIMERCLKEN                                          18
#define     CMU_DEVCLKEN0_PWMCLKEN                                            17
#define     CMU_DEVCLKEN0_SECLKEN                                             16
#define     CMU_DEVCLKEN0_LCDCLKEN                                            14
#define     CMU_DEVCLKEN0_DECLKEN                                             12
#define     CMU_DEVCLKEN0_USBCLKEN                                            10
#define     CMU_DEVCLKEN0_SPI1DCACHECLKEN                                     9
#define     CMU_DEVCLKEN0_SPI0CACHECLKEN                                      8
#define     CMU_DEVCLKEN0_SPI3CLKEN                                           7
#define     CMU_DEVCLKEN0_SPI2CLKEN                                           6
#define     CMU_DEVCLKEN0_SPI1CLKEN                                           5
#define     CMU_DEVCLKEN0_SPI0CLKEN                                           4
#define     CMU_DEVCLKEN0_SD1CLKEN                                            2
#define     CMU_DEVCLKEN0_SD0CLKEN                                            1
#define     CMU_DEVCLKEN0_DMACLKEN                                            0

#define     CMU_DEVCLKEN1_BTHUB_RC32KEN                                       28
#define     CMU_DEVCLKEN1_BTHUB_RC64MEN                                       27
#define     CMU_DEVCLKEN1_BTHUB_HOSCEN                                        26
#define     CMU_DEVCLKEN1_BTHUB_RC4MEN                                        25
#define     CMU_DEVCLKEN1_BTHUB_LOSCEN                                        24
#define     CMU_DEVCLKEN1_I2CMT1CLKEN                                         19
#define     CMU_DEVCLKEN1_I2CMT0CLKEN                                         18
#define     CMU_DEVCLKEN1_SPIMT1CLKEN                                         17
#define     CMU_DEVCLKEN1_SPIMT0CLKEN                                         16
#define     CMU_DEVCLKEN1_DACANACLKEN                                         15
#define     CMU_DEVCLKEN1_SPDIFRXCLKEN                                        9
#define     CMU_DEVCLKEN1_SPDIFTXCLKEN                                        8
#define     CMU_DEVCLKEN1_I2SHCLKEN                                           7
#define     CMU_DEVCLKEN1_I2SSRDCLKEN                                         6
#define     CMU_DEVCLKEN1_I2SG1CLKEN                                          5
#define     CMU_DEVCLKEN1_I2SG0CLKEN                                          4
#define     CMU_DEVCLKEN1_ADCCLKEN                                            3
#define     CMU_DEVCLKEN1_DACCLKEN                                            2
#define     CMU_DEVCLKEN1_AUDDSPTIMERCLKEN                                    1
#define     CMU_DEVCLKEN1_AUDDSPCLKEN                                         0

#define     CMU_SD0CLK_SD0CLKSRC_e                                            9
#define     CMU_SD0CLK_SD0CLKSRC_SHIFT                                        8
#define     CMU_SD0CLK_SD0CLKSRC_MASK                                         (0x3<<8)
#define     CMU_SD0CLK_SD0CLKPOSTDIV                                          6
#define     CMU_SD0CLK_SD0CLKDIV_e                                            3
#define     CMU_SD0CLK_SD0CLKDIV_SHIFT                                        0
#define     CMU_SD0CLK_SD0CLKDIV_MASK                                         (0xF<<0)

#define     CMU_SD1CLK_SD1CLKSRC_e                                            9
#define     CMU_SD1CLK_SD1CLKSRC_SHIFT                                        8
#define     CMU_SD1CLK_SD1CLKSRC_MASK                                         (0x3<<8)
#define     CMU_SD1CLK_SD1CLKPOSTDIV                                          6
#define     CMU_SD1CLK_SD1CLKDIV_e                                            3
#define     CMU_SD1CLK_SD1CLKDIV_SHIFT                                        0
#define     CMU_SD1CLK_SD1CLKDIV_MASK                                         (0xF<<0)

#define     CMU_SPI0CLK_SPI0CLKSRC_e                                          9
#define     CMU_SPI0CLK_SPI0CLKSRC_SHIFT                                      8
#define     CMU_SPI0CLK_SPI0CLKSRC_MASK                                       (0x3<<8)
#define     CMU_SPI0CLK_SPI0CLKDIV_e                                          3
#define     CMU_SPI0CLK_SPI0CLKDIV_SHIFT                                      0
#define     CMU_SPI0CLK_SPI0CLKDIV_MASK                                       (0xF<<0)

#define     CMU_SPI1CLK_SPI1CLKSRC_e                                          9
#define     CMU_SPI1CLK_SPI1CLKSRC_SHIFT                                      8
#define     CMU_SPI1CLK_SPI1CLKSRC_MASK                                       (0x3<<8)
#define     CMU_SPI1CLK_SPI1CLKDIV_e                                          3
#define     CMU_SPI1CLK_SPI1CLKDIV_SHIFT                                      0
#define     CMU_SPI1CLK_SPI1CLKDIV_MASK                                       (0xF<<0)

#define     CMU_SPI2CLK_SPI2CLKSRC_e                                          9
#define     CMU_SPI2CLK_SPI2CLKSRC_SHIFT                                      8
#define     CMU_SPI2CLK_SPI2CLKSRC_MASK                                       (0x3<<8)
#define     CMU_SPI2CLK_SPI2CLKDIV_e                                          3
#define     CMU_SPI2CLK_SPI2CLKDIV_SHIFT                                      0
#define     CMU_SPI2CLK_SPI2CLKDIV_MASK                                       (0xF<<0)

#define     CMU_SPI3CLK_SPI3CLKSRC_e                                          9
#define     CMU_SPI3CLK_SPI3CLKSRC_SHIFT                                      8
#define     CMU_SPI3CLK_SPI3CLKSRC_MASK                                       (0x3<<8)
#define     CMU_SPI3CLK_SPI3CLKDIV_e                                          3
#define     CMU_SPI3CLK_SPI3CLKDIV_SHIFT                                      0
#define     CMU_SPI3CLK_SPI3CLKDIV_MASK                                       (0xF<<0)

#define     CMU_DECLK_DECLKSRC_e                                              9
#define     CMU_DECLK_DECLKSRC_SHIFT                                          8
#define     CMU_DECLK_DECLKSRC_MASK                                           (0x3<<8)
#define     CMU_DECLK_DECLKDIV_e                                              3
#define     CMU_DECLK_DECLKDIV_SHIFT                                          0
#define     CMU_DECLK_DECLKDIV_MASK                                           (0xF<<0)

#define     CMU_LCDCLK_LCDCLKSRC_e                                            9
#define     CMU_LCDCLK_LCDCLKSRC_SHIFT                                        8
#define     CMU_LCDCLK_LCDCLKSRC_MASK                                         (0x3<<8)
#define     CMU_LCDCLK_LCDCLKPREDIV                                           4
#define     CMU_LCDCLK_LCDCLKDIV_e                                            3
#define     CMU_LCDCLK_LCDCLKDIV_SHIFT                                        0
#define     CMU_LCDCLK_LCDCLKDIV_MASK                                         (0xF<<0)

#define     CMU_SECCLK_SECCLKSRC_e                                            9
#define     CMU_SECCLK_SECCLKSRC_SHIFT                                        8
#define     CMU_SECCLK_SECCLKSRC_MASK                                         (0x3<<8)
#define     CMU_SECCLK_SECCLKDIV_e                                            1
#define     CMU_SECCLK_SECCLKDIV_SHIFT                                        0
#define     CMU_SECCLK_SECCLKDIV_MASK                                         (0x3<<0)

#define     CMU_TIMER0CLK_TIMER0CLKSRC_e                                      1
#define     CMU_TIMER0CLK_TIMER0CLKSRC_SHIFT                                  0
#define     CMU_TIMER0CLK_TIMER0CLKSRC_MASK                                   (0x3<<0)

#define     CMU_TIMER1CLK_TIMER1CLKSRC_e                                      1
#define     CMU_TIMER1CLK_TIMER1CLKSRC_SHIFT                                  0
#define     CMU_TIMER1CLK_TIMER1CLKSRC_MASK                                   (0x3<<0)

#define     CMU_TIMER2CLK_TIMER2CLKSRC_e                                      1
#define     CMU_TIMER2CLK_TIMER2CLKSRC_SHIFT                                  0
#define     CMU_TIMER2CLK_TIMER2CLKSRC_MASK                                   (0x3<<0)

#define     CMU_TIMER3CLK_TIMER3CLKSRC_e                                      1
#define     CMU_TIMER3CLK_TIMER3CLKSRC_SHIFT                                  0
#define     CMU_TIMER3CLK_TIMER3CLKSRC_MASK                                   (0x3<<0)

#define     CMU_TIMER4CLK_TIMER4CLKSRC_e                                      1
#define     CMU_TIMER4CLK_TIMER4CLKSRC_SHIFT                                  0
#define     CMU_TIMER4CLK_TIMER4CLKSRC_MASK                                   (0x3<<0)

#define     CMU_PWM0CLK_PWM0CLKSRC_e                                          10
#define     CMU_PWM0CLK_PWM0CLKSRC_SHIFT                                      9
#define     CMU_PWM0CLK_PWM0CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM0CLK_PWM0CLKDIV_e                                          8
#define     CMU_PWM0CLK_PWM0CLKDIV_SHIFT                                      0
#define     CMU_PWM0CLK_PWM0CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM1CLK_PWM1CLKSRC_e                                          10
#define     CMU_PWM1CLK_PWM1CLKSRC_SHIFT                                      9
#define     CMU_PWM1CLK_PWM1CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM1CLK_PWM1CLKDIV_e                                          8
#define     CMU_PWM1CLK_PWM1CLKDIV_SHIFT                                      0
#define     CMU_PWM1CLK_PWM1CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM2CLK_PWM2CLKSRC_e                                          10
#define     CMU_PWM2CLK_PWM2CLKSRC_SHIFT                                      9
#define     CMU_PWM2CLK_PWM2CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM2CLK_PWM2CLKDIV_e                                          8
#define     CMU_PWM2CLK_PWM2CLKDIV_SHIFT                                      0
#define     CMU_PWM2CLK_PWM2CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM3CLK_PWM3CLKSRC_e                                          10
#define     CMU_PWM3CLK_PWM3CLKSRC_SHIFT                                      9
#define     CMU_PWM3CLK_PWM3CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM3CLK_PWM3CLKDIV_e                                          8
#define     CMU_PWM3CLK_PWM3CLKDIV_SHIFT                                      0
#define     CMU_PWM3CLK_PWM3CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM4CLK_PWM4CLKSRC_e                                          10
#define     CMU_PWM4CLK_PWM4CLKSRC_SHIFT                                      9
#define     CMU_PWM4CLK_PWM4CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM4CLK_PWM4CLKDIV_e                                          8
#define     CMU_PWM4CLK_PWM4CLKDIV_SHIFT                                      0
#define     CMU_PWM4CLK_PWM4CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM5CLK_PWM5CLKSRC_e                                          10
#define     CMU_PWM5CLK_PWM5CLKSRC_SHIFT                                      9
#define     CMU_PWM5CLK_PWM5CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM5CLK_PWM5CLKDIV_e                                          8
#define     CMU_PWM5CLK_PWM5CLKDIV_SHIFT                                      0
#define     CMU_PWM5CLK_PWM5CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM6CLK_PWM6CLKSRC_e                                          10
#define     CMU_PWM6CLK_PWM6CLKSRC_SHIFT                                      9
#define     CMU_PWM6CLK_PWM6CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM6CLK_PWM6CLKDIV_e                                          8
#define     CMU_PWM6CLK_PWM6CLKDIV_SHIFT                                      0
#define     CMU_PWM6CLK_PWM6CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM7CLK_PWM7CLKSRC_e                                          10
#define     CMU_PWM7CLK_PWM7CLKSRC_SHIFT                                      9
#define     CMU_PWM7CLK_PWM7CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM7CLK_PWM7CLKDIV_e                                          8
#define     CMU_PWM7CLK_PWM7CLKDIV_SHIFT                                      0
#define     CMU_PWM7CLK_PWM7CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_PWM8CLK_PWM8CLKSRC_e                                          10
#define     CMU_PWM8CLK_PWM8CLKSRC_SHIFT                                      9
#define     CMU_PWM8CLK_PWM8CLKSRC_MASK                                       (0x3<<9)
#define     CMU_PWM8CLK_PWM8CLKDIV_e                                          8
#define     CMU_PWM8CLK_PWM8CLKDIV_SHIFT                                      0
#define     CMU_PWM8CLK_PWM8CLKDIV_MASK                                       (0x1FF<<0)

#define     CMU_LRADCCLK_LRADCCLKSRC_e                                        6
#define     CMU_LRADCCLK_LRADCCLKSRC_SHIFT                                    4
#define     CMU_LRADCCLK_LRADCCLKSRC_MASK                                     (0x7<<4)
#define     CMU_LRADCCLK_LRADCCLKDIV_e                                        1
#define     CMU_LRADCCLK_LRADCCLKDIV_SHIFT                                    0
#define     CMU_LRADCCLK_LRADCCLKDIV_MASK                                     (0x3<<0)

#define     CMU_CECCLK_CECCLKDIV_e                                            1
#define     CMU_CECCLK_CECCLKDIV_SHIFT                                        0
#define     CMU_CECCLK_CECCLKDIV_MASK                                         (0x3<<0)

#define     CMU_I2C0CLK_I2C0CLKDIV_e                                          1
#define     CMU_I2C0CLK_I2C0CLKDIV_SHIFT                                      0
#define     CMU_I2C0CLK_I2C0CLKDIV_MASK                                       (0x3<<0)

#define     CMU_I2C1CLK_I2C1CLKDIV_e                                          1
#define     CMU_I2C1CLK_I2C1CLKDIV_SHIFT                                      0
#define     CMU_I2C1CLK_I2C1CLKDIV_MASK                                       (0x3<<0)

#define     CMU_AUDDSPCLK_DSPRIEN                                             24
#define     CMU_AUDDSPCLK_DSPAPBCLKDIV_e                                      9
#define     CMU_AUDDSPCLK_DSPAPBCLKDIV_SHIFT                                  8
#define     CMU_AUDDSPCLK_DSPAPBCLKDIV_MASK                                   (0x3<<8)
#define     CMU_AUDDSPCLK_DSPCLKDIV_e                                         7
#define     CMU_AUDDSPCLK_DSPCLKDIV_SHIFT                                     4
#define     CMU_AUDDSPCLK_DSPCLKDIV_MASK                                      (0xF<<4)
#define     CMU_AUDDSPCLK_DSPCLKSRC_e                                         2
#define     CMU_AUDDSPCLK_DSPCLKSRC_SHIFT                                     0
#define     CMU_AUDDSPCLK_DSPCLKSRC_MASK                                      (0x7<<0)

#define     CMU_DACCLK_DACVOLCLKSRC                                           28
#define     CMU_DACCLK_DACFIFO1CLKEN                                          25
#define     CMU_DACCLK_DACFIFO0CLKEN                                          24
#define     CMU_DACCLK_DACCLKSRC                                              8
#define     CMU_DACCLK_DACOSCCLKDIV                                           5
#define     CMU_DACCLK_DACCLKPREDIV                                           4
#define     CMU_DACCLK_DACCLKDIV_e                                            2
#define     CMU_DACCLK_DACCLKDIV_SHIFT                                        0
#define     CMU_DACCLK_DACCLKDIV_MASK                                         (0x7<<0)

#define     CMU_ADCCLK_ADCDEBUGEN                                             31
#define     CMU_ADCCLK_ADCFIFOCLKEN                                           24
#define     CMU_ADCCLK_ADCFIREN                                               23
#define     CMU_ADCCLK_ADCCICEN                                               22
#define     CMU_ADCCLK_ADCANAEN                                               21
#define     CMU_ADCCLK_ADCDMICEN                                              20
#define     CMU_ADCCLK_ADCFIRCLKRVS                                           19
#define     CMU_ADCCLK_ADCCICCLKRVS                                           18
#define     CMU_ADCCLK_ADCANACLKRVS                                           17
#define     CMU_ADCCLK_ADCDMICCLKRVS                                          16
#define     CMU_ADCCLK_ADCFIRCLKDIV                                           15
#define     CMU_ADCCLK_ADCOVFSCLKDIV_e                                        13
#define     CMU_ADCCLK_ADCOVFSCLKDIV_SHIFT                                    12
#define     CMU_ADCCLK_ADCOVFSCLKDIV_MASK                                     (0x3<<12)
#define     CMU_ADCCLK_ADCCLKSRC                                              8
#define     CMU_ADCCLK_ADCCLKPREDIV                                           4
#define     CMU_ADCCLK_ADCCLKDIV_e                                            2
#define     CMU_ADCCLK_ADCCLKDIV_SHIFT                                        0
#define     CMU_ADCCLK_ADCCLKDIV_MASK                                         (0x7<<0)

#define     CMU_I2STXCLK_I2STXMCLKOEN                                         29
#define     CMU_I2STXCLK_I2SG0LRCLKPROC                                       23
#define     CMU_I2STXCLK_I2SG0LRCLKDIV_e                                      21
#define     CMU_I2STXCLK_I2SG0LRCLKDIV_SHIFT                                  20
#define     CMU_I2STXCLK_I2SG0LRCLKDIV_MASK                                   (0x3<<20)
#define     CMU_I2STXCLK_I2SG0BCLKDIV_e                                       19
#define     CMU_I2STXCLK_I2SG0BCLKDIV_SHIFT                                   18
#define     CMU_I2STXCLK_I2SG0BCLKDIV_MASK                                    (0x3<<18)
#define     CMU_I2STXCLK_I2SG0BLRCLKSRC                                       16
#define     CMU_I2STXCLK_I2SG0MCLKEXTREV                                      14
#define     CMU_I2STXCLK_I2SG0MCLKSRC_e                                       13
#define     CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT                                   12
#define     CMU_I2STXCLK_I2SG0MCLKSRC_MASK                                    (0x3<<12)
#define     CMU_I2STXCLK_I2SG0CLKSRC                                          8
#define     CMU_I2STXCLK_I2SG0CLKDIV_e                                        3
#define     CMU_I2STXCLK_I2SG0CLKDIV_SHIFT                                    0
#define     CMU_I2STXCLK_I2SG0CLKDIV_MASK                                     (0xF<<0)

#define     CMU_I2SRXCLK_I2SRXMCLKOEN                                         29
#define     CMU_I2SRXCLK_I2SRX0CLKSRC                                         28
#define     CMU_I2SRXCLK_I2SG1LRCLKPROC                                       23
#define     CMU_I2SRXCLK_I2SG1LRCLKDIV_e                                      21
#define     CMU_I2SRXCLK_I2SG1LRCLKDIV_SHIFT                                  20
#define     CMU_I2SRXCLK_I2SG1LRCLKDIV_MASK                                   (0x3<<20)
#define     CMU_I2SRXCLK_I2SG1BCLKDIV_e                                       19
#define     CMU_I2SRXCLK_I2SG1BCLKDIV_SHIFT                                   18
#define     CMU_I2SRXCLK_I2SG1BCLKDIV_MASK                                    (0x3<<18)
#define     CMU_I2SRXCLK_I2SG1BLRCLKSRC                                       16
#define     CMU_I2SRXCLK_I2SG1MCLKEXTREV                                      14
#define     CMU_I2SRXCLK_I2SG1MCLKSRC_e                                       13
#define     CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT                                   12
#define     CMU_I2SRXCLK_I2SG1MCLKSRC_MASK                                    (0x3<<12)
#define     CMU_I2SRXCLK_I2SG1CLKSRC                                          8
#define     CMU_I2SRXCLK_I2SG1CLKDIV_e                                        3
#define     CMU_I2SRXCLK_I2SG1CLKDIV_SHIFT                                    0
#define     CMU_I2SRXCLK_I2SG1CLKDIV_MASK                                     (0xF<<0)

#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_e                                    13
#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT                                12
#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_MASK                                 (0x3<<12)
#define     CMU_SPDIFTXCLK_SPDTXCLKSRC                                        8
#define     CMU_SPDIFTXCLK_SPDTXCLKPREDIV                                     4
#define     CMU_SPDIFTXCLK_SPDTXCLKDIV_e                                      2
#define     CMU_SPDIFTXCLK_SPDTXCLKDIV_SHIFT                                  0
#define     CMU_SPDIFTXCLK_SPDTXCLKDIV_MASK                                   (0x7<<0)

#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_e                                    9
#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_SHIFT                                8
#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_MASK                                 (0x3<<8)
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_e                                    1
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_SHIFT                                0
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_MASK                                 (0x3<<0)

#define     CMU_MEMCLKEN0_SPI1CACHERAMCLKEN                                   27
#define     CMU_MEMCLKEN0_SPI0CACHERAMCLKEN                                   26
#define     CMU_MEMCLKEN0_AUDDSPRAM2CLKEN                                     24
#define     CMU_MEMCLKEN0_AUDDSPRAM1CLKEN                                     22
#define     CMU_MEMCLKEN0_AUDDSPRAM0CLKEN                                     20
#define     CMU_MEMCLKEN0_AUDDSPTCRAMCLKEN                                    12
#define     CMU_MEMCLKEN0_RAM4CLKEN                                           5
#define     CMU_MEMCLKEN0_RAM3CLKEN                                           4
#define     CMU_MEMCLKEN0_RAM2CLKEN                                           3
#define     CMU_MEMCLKEN0_RAM1CLKEN                                           2
#define     CMU_MEMCLKEN0_RAM0CLKEN                                           1
#define     CMU_MEMCLKEN0_ROMCLKEN                                            0

#define     CMU_MEMCLKEN1_BTRAM1CLKEN                                         18
#define     CMU_MEMCLKEN1_BTRAM0CLKEN                                         17
#define     CMU_MEMCLKEN1_BTROMCLKEN                                          16
#define     CMU_MEMCLKEN1_USBRAMCLKEN                                         8

#define     CMU_MEMCLKSRC0_AUDDSPRAM2CLKSRC_e                                 25
#define     CMU_MEMCLKSRC0_AUDDSPRAM2CLKSRC_SHIFT                             24
#define     CMU_MEMCLKSRC0_AUDDSPRAM2CLKSRC_MASK                              (0x3<<24)
#define     CMU_MEMCLKSRC0_AUDDSPRAM1CLKSRC_e                                 23
#define     CMU_MEMCLKSRC0_AUDDSPRAM1CLKSRC_SHIFT                             22
#define     CMU_MEMCLKSRC0_AUDDSPRAM1CLKSRC_MASK                              (0x3<<22)
#define     CMU_MEMCLKSRC0_AUDDSPRAM0CLKSRC_e                                 21
#define     CMU_MEMCLKSRC0_AUDDSPRAM0CLKSRC_SHIFT                             20
#define     CMU_MEMCLKSRC0_AUDDSPRAM0CLKSRC_MASK                              (0x3<<20)
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKDIV_e                             17
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKDIV_SHIFT                         16
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKDIV_MASK                          (0x3<<16)
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKSRC_e                             15
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKSRC_SHIFT                         13
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMPRECLKSRC_MASK                          (0x7<<13)
#define     CMU_MEMCLKSRC0_AUDDSPTCRAMCLKSRC                                  12
#define     CMU_MEMCLKSRC0_RAM4CLKDIV_e                                       9
#define     CMU_MEMCLKSRC0_RAM4CLKDIV_SHIFT                                   8
#define     CMU_MEMCLKSRC0_RAM4CLKDIV_MASK                                    (0x3<<8)
#define     CMU_MEMCLKSRC0_RAM4CLKSRC_e                                       7
#define     CMU_MEMCLKSRC0_RAM4CLKSRC_SHIFT                                   5
#define     CMU_MEMCLKSRC0_RAM4CLKSRC_MASK                                    (0x7<<5)

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

#define     CMU_S1CLKCTL_RC64M_S1EN                                           2
#define     CMU_S1CLKCTL_HOSC_S1EN                                            1
#define     CMU_S1CLKCTL_RC4M_S1EN                                            0

#define     CMU_S1BTCLKCTL_RC64M_S1BTEN                                       2
#define     CMU_S1BTCLKCTL_HOSC_S1BTEN                                        1
#define     CMU_S1BTCLKCTL_RC4M_S1BTEN                                        0

#define     CMU_S2CLKCTL_RC64M_S2EN                                           2
#define     CMU_S2CLKCTL_HOSC_S2EN                                            1
#define     CMU_S2CLKCTL_RC4M_S2EN                                            0

#define     CMU_S3CLKCTL_HOSC_S3EN                                            1
#define     CMU_S3CLKCTL_RC4M_S3EN                                            0

#define     CMU_PMUWKUPCLKCTL_PMUWKUPCLKSRC                                   0

#define     CMU_DIGITALDEBUG_DBGEN                                            31
#define     CMU_DIGITALDEBUG_CK4MDEBUG                                        30
#define     CMU_DIGITALDEBUG_DEGSEL_e                                         7
#define     CMU_DIGITALDEBUG_DEGSEL_SHIFT                                     0
#define     CMU_DIGITALDEBUG_DEGSEL_MASK                                      (0xFF<<0)

#define     CMU_TST_CTL_RC128M_TESTCTL                                        7
#define     CMU_TST_CTL_AUDIOPLL1_TESTCTL                                     6
#define     CMU_TST_CTL_AUDIOPLL0_TESTCTL                                     5
#define     CMU_TST_CTL_RC96M_TESTCTL                                         4
#define     CMU_TST_CTL_COREPLL_TESTCTL                                       3
#define     CMU_TST_CTL_RC64M_TESTCTL                                         2
#define     CMU_TST_CTL_RC4M_TESTCTL                                          1
#define     CMU_TST_CTL_RC32K_TESTCTL                                         0

#define     CMU_SPIMT0CLK_SPIMT0CLKSRC_e                                      9
#define     CMU_SPIMT0CLK_SPIMT0CLKSRC_SHIFT                                  8
#define     CMU_SPIMT0CLK_SPIMT0CLKSRC_MASK                                   (0x3<<8)
#define     CMU_SPIMT0CLK_SPIMT0CLKDIV_e                                      3
#define     CMU_SPIMT0CLK_SPIMT0CLKDIV_SHIFT                                  0
#define     CMU_SPIMT0CLK_SPIMT0CLKDIV_MASK                                   (0xF<<0)

#define     CMU_SPIMT1CLK_SPIMT1CLKSRC_e                                      9
#define     CMU_SPIMT1CLK_SPIMT1CLKSRC_SHIFT                                  8
#define     CMU_SPIMT1CLK_SPIMT1CLKSRC_MASK                                   (0x3<<8)
#define     CMU_SPIMT1CLK_SPIMT1CLKDIV_e                                      3
#define     CMU_SPIMT1CLK_SPIMT1CLKDIV_SHIFT                                  0
#define     CMU_SPIMT1CLK_SPIMT1CLKDIV_MASK                                   (0xF<<0)

#define     CMU_I2CMT0CLK_I2CMT0CLKSRC_e                                      9
#define     CMU_I2CMT0CLK_I2CMT0CLKSRC_SHIFT                                  8
#define     CMU_I2CMT0CLK_I2CMT0CLKSRC_MASK                                   (0x3<<8)
#define     CMU_I2CMT0CLK_I2CMT0CLKDIV_e                                      1
#define     CMU_I2CMT0CLK_I2CMT0CLKDIV_SHIFT                                  0
#define     CMU_I2CMT0CLK_I2CMT0CLKDIV_MASK                                   (0x3<<0)

#define     CMU_I2CMT1CLK_I2CMT1CLKSRC_e                                      9
#define     CMU_I2CMT1CLK_I2CMT1CLKSRC_SHIFT                                  8
#define     CMU_I2CMT1CLK_I2CMT1CLKSRC_MASK                                   (0x3<<8)
#define     CMU_I2CMT1CLK_I2CMT1CLKDIV_e                                      1
#define     CMU_I2CMT1CLK_I2CMT1CLKDIV_SHIFT                                  0
#define     CMU_I2CMT1CLK_I2CMT1CLKDIV_MASK                                   (0x3<<0)

//--------------PMUVDD-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     PMUVDD_BASE                                                       0x40004000
#define     VOUT_CTL0                                                         (PMUVDD_BASE+0x00)
#define     VOUT_CTL1_S1                                                      (PMUVDD_BASE+0x04)
#define     VOUT_CTL1_S2                                                      (PMUVDD_BASE+0x08)
#define     VOUT_CTL1_S3                                                      (PMUVDD_BASE+0X0C)
#define     PMU_DET                                                           (PMUVDD_BASE+0x10)
#define     DCDC_VC18_CTL                                                     (PMUVDD_BASE+0X20)
#define     DCDC_VD12_CTL                                                     (PMUVDD_BASE+0X24)
#define     DCDC_VDD_CTL                                                      (PMUVDD_BASE+0X28)
#define     PWRGATE_DIG                                                       (PMUVDD_BASE+0X30)
#define     PWRGATE_DIG_ACK                                                   (PMUVDD_BASE+0X34)
#define     PWRGATE_RAM                                                       (PMUVDD_BASE+0X38)
#define     PWRGATE_RAM_ACK                                                   (PMUVDD_BASE+0X3C)
#define     PMU_INTMASK                                                       (PMUVDD_BASE+0X40)

#endif
