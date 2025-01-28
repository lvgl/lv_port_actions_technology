/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio ADC physical implementation
 */

/*
 * Features
 *	- Support 4 independent channels with high performance ADC.
 *	- ADC0/ADC1 support ANC
 *    - 1 x  FIFOs (ADC0/ADC1 uses FIFO0(4 * 16 level * 24bits))
 *    - Support single ended and full differential input
 *    - Support 4 DMIC input
 *	- Programmable HPF
 *	- Support 3 different kinds of frequency response curves
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- INPUT0P: Analog input for ADC0 or differential input ADC0 INPUT0N
 *	- INPUT0N: Analog input to ADC1 or differential input ADC0 INPUT0P
 *	- INPUT1P: Analog input to ADC0/1 or differential input ADC1 INPUT1N
 *	- INPUT1N: Analog input to ADC1 or differential input ADC1 INPUT1P
 *	- INPUT2P: Analog input to ADC0/2 or differential input ADC2 INPUT2N
 *	- INPUT2N: Analog input to ADC1/2 or differential input ADC2 INPUT2P
 *	- INPUT3P: Analog input to ADC3 or differential input ADC3 INPUT3P
 *	- INPUT3N: Analog input to ADC3 or differential input ADC3 INPUT3P
 *	- DMIC_CLK: DMIC clk output
 *	- DMIC_DATA: DMIC data input
 */

#include <kernel.h>
#include <device.h>
#include <ksched.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include <drivers/audio/audio_in.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(adc0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * ADC_DIGCTL
 */
#define ADC_DIGCTL_ADC_OVFS_SHIFT                               (16)
#define ADC_DIGCTL_ADC_OVFS_MASK                                (0x3 << ADC_DIGCTL_ADC_OVFS_SHIFT)
#define ADC_DIGCTL_ADC_OVFS(x)                                  ((x) << ADC_DIGCTL_ADC_OVFS_SHIFT)
#define ADC_DIGCTL_ADC3_DIG_EN                                  BIT(15) /* ADC3 digital enable */
#define ADC_DIGCTL_ADC2_DIG_EN                                  BIT(14) /* ADC2 digital enable */
#define ADC_DIGCTL_ADC1_DIG_EN                                  BIT(13) /* ADC1 digital enable */
#define ADC_DIGCTL_ADC0_DIG_EN                                  BIT(12) /* ADC0 digital enable */
#define ADC_DIGCTL_ADC_DIG_SHIFT                                (12)
#define ADC_DIGCTL_ADC_DIG_MASK                                 ((0xF) << ADC_DIGCTL_ADC_DIG_SHIFT)
#define ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT                          (8)
#define ADC_DIGCTL_DMIC_PRE_GAIN_MASK                           (0x7 << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)
#define ADC_DIGCTL_DMIC_PRE_GAIN(x)                             ((x) << ADC_DIGCTL_DMIC_PRE_GAIN_SHIFT)
#define ADC_DIGCTL_DMIC23_CHS                                   BIT(7) /* 0: DMIC23 channel select L/R; 1: DMIC23 channel select R/L*/
#define ADC_DIGCTL_DMIC01_CHS                                   BIT(6)
#define ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT                         (4)
#define ADC_DIGCTL_ADC_FIR_MD_SEL_MASK                          (0x3 << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)
#define ADC_DIGCTL_ADC_FIR_MD_SEL(x)                            ((x) << ADC_DIGCTL_ADC_FIR_MD_SEL_SHIFT)
#define ADC_DIGCTL_ADDEN                                        BIT(3)
#define ADC_DIGCTL_AADEN                                        BIT(2)

/***************************************************************************************************
 * CH0_DIGCTL
 */
#define CH0_DIGCTL_MIC_SEL                                      BIT(17) /* select ADC analog or digital MIC */
#define CH0_DIGCTL_DAT_OUT_EN                                   BIT(15) /* 0: no output on ADC0 */
#define CH0_DIGCTL_HPF_AS_TS_SHIFT                              (13)
#define CH0_DIGCTL_HPF_AS_TS_MASK                               (0x3 << CH0_DIGCTL_HPF_AS_TS_SHIFT)
#define CH0_DIGCTL_HPF_AS_TS(x)                                 ((x) << CH0_DIGCTL_HPF_AS_TS_SHIFT)
#define CH0_DIGCTL_HPF_AS_EN                                    BIT(12) /* HPF auto-set enable */
#define CH0_DIGCTL_HPFEN                                        BIT(11)
#define CH0_DIGCTL_HPF_S                                        BIT(10)
#define CH0_DIGCTL_HPF_N_SHIFT                                  (4)
#define CH0_DIGCTL_HPF_N_MASK                                   (0x3F << CH0_DIGCTL_HPF_N_SHIFT)
#define CH0_DIGCTL_HPF_N(x)                                     ((x) << CH0_DIGCTL_HPF_N_SHIFT)
#define CH0_DIGCTL_ADCGC_SHIFT                                  (0)
#define CH0_DIGCTL_ADCGC_MASK                                   (0xF << CH0_DIGCTL_ADCGC_SHIFT)
#define CH0_DIGCTL_ADCGC(x)                                     ((x) << CH0_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * CH1_DIGCTL
 */
#define CH1_DIGCTL_MIC_SEL                                      BIT(17)
#define CH1_DIGCTL_DAT_OUT_EN                                   BIT(15)
#define CH1_DIGCTL_HPF_AS_TS_SHIFT                              (13)
#define CH1_DIGCTL_HPF_AS_TS_MASK                               (0x3 << CH1_DIGCTL_HPF_AS_TS_SHIFT)
#define CH1_DIGCTL_HPF_AS_TS(x)                                 ((x) << CH1_DIGCTL_HPF_AS_TS_SHIFT)
#define CH1_DIGCTL_HPF_AS_EN                                    BIT(12)
#define CH1_DIGCTL_HPFEN                                        BIT(11)
#define CH1_DIGCTL_HPF_S                                        BIT(10)
#define CH1_DIGCTL_HPF_N_SHIFT                                  (4)
#define CH1_DIGCTL_HPF_N_MASK                                   (0x3F << CH1_DIGCTL_HPF_N_SHIFT)
#define CH1_DIGCTL_HPF_N(x)                                     ((x) << CH1_DIGCTL_HPF_N_SHIFT)
#define CH1_DIGCTL_ADCGC_SHIFT                                  (0)
#define CH1_DIGCTL_ADCGC_MASK                                   (0xF << CH1_DIGCTL_ADCGC_SHIFT)
#define CH1_DIGCTL_ADCGC(x)                                     ((x) << CH1_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * CH2_DIGCTL
 */
#define CH2_DIGCTL_MIC_SEL                                      BIT(17)
#define CH2_DIGCTL_DAT_OUT_EN                                   BIT(15)
#define CH2_DIGCTL_HPF_AS_TS_SHIFT                              (13)
#define CH2_DIGCTL_HPF_AS_TS_MASK                               (0x3 << CH2_DIGCTL_HPF_AS_TS_SHIFT)
#define CH2_DIGCTL_HPF_AS_TS(x)                                 ((x) << CH2_DIGCTL_HPF_AS_TS_SHIFT)
#define CH2_DIGCTL_HPF_AS_EN                                    BIT(12)
#define CH2_DIGCTL_HPFEN                                        BIT(11)
#define CH2_DIGCTL_HPF_S                                        BIT(10)
#define CH2_DIGCTL_HPF_N_SHIFT                                  (4)
#define CH2_DIGCTL_HPF_N_MASK                                   (0x3F << CH2_DIGCTL_HPF_N_SHIFT)
#define CH2_DIGCTL_HPF_N(x)                                     ((x) << CH2_DIGCTL_HPF_N_SHIFT)
#define CH2_DIGCTL_ADCGC_SHIFT                                  (0)
#define CH2_DIGCTL_ADCGC_MASK                                   (0xF << CH2_DIGCTL_ADCGC_SHIFT)
#define CH2_DIGCTL_ADCGC(x)                                     ((x) << CH2_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * CH3_DIGCTL
 */
#define CH3_DIGCTL_MIC_SEL                                       BIT(17)
#define CH3_DIGCTL_DAT_OUT_EN                                    BIT(15)
#define CH3_DIGCTL_HPF_AS_TS_SHIFT                               (13)
#define CH3_DIGCTL_HPF_AS_TS_MASK                                (0x3 << CH3_DIGCTL_HPF_AS_TS_SHIFT)
#define CH3_DIGCTL_HPF_AS_TS(x)                                  ((x) << CH3_DIGCTL_HPF_AS_TS_SHIFT)
#define CH3_DIGCTL_HPF_AS_EN                                     BIT(12)
#define CH3_DIGCTL_HPFEN                                         BIT(11)
#define CH3_DIGCTL_HPF_S                                         BIT(10)
#define CH3_DIGCTL_HPF_N_SHIFT                                   (4)
#define CH3_DIGCTL_HPF_N_MASK                                    (0x3F << CH3_DIGCTL_HPF_N_SHIFT)
#define CH3_DIGCTL_HPF_N(x)                                      ((x) << CH3_DIGCTL_HPF_N_SHIFT)
#define CH3_DIGCTL_ADCGC_SHIFT                                   (0)
#define CH3_DIGCTL_ADCGC_MASK                                    (0xF << CH3_DIGCTL_ADCGC_SHIFT)
#define CH3_DIGCTL_ADCGC(x)                                      ((x) << CH3_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * ADC_FIFOCTL
 */
#define ADC_FIFOCTL_DRQ_LEVEL_SHIFT                              (8)
#define ADC_FIFOCTL_DRQ_LEVEL_MASK                               (0x3F << ADC_FIFOCTL_DRQ_LEVEL_SHIFT)
#define ADC_FIFOCTL_DRQ_LEVEL(x)                                 ((x) << ADC_FIFOCTL_DRQ_LEVEL_SHIFT)
#define ADC_FIFOCTL_ADCFIFO0_DMAWIDTH                            BIT(7)
#define ADC_FIFOCTL_ADF0OS_SHIFT                                 (4)
#define ADC_FIFOCTL_ADF0OS_MASK                                  (0x3 << ADC_FIFOCTL_ADF0OS_SHIFT)
#define ADC_FIFOCTL_ADF0OS(x)                                    ((x) << ADC_FIFOCTL_ADF0OS_SHIFT)
#define ADC_FIFOCTL_ADF0FIE                                      BIT(2)
#define ADC_FIFOCTL_ADF0FDE                                      BIT(1)
#define ADC_FIFOCTL_ADF0RT                                       BIT(0)

/***************************************************************************************************
 * ADC_STAT
 */
#define ADC_STAT_FIFO0_ER                                        BIT(9)
#define ADC_STAT_ADF0EF                                          BIT(8)
#define ADC_STAT_ADF0IP                                          BIT(7)
#define ADC_STAT_ADF0S_SHIFT                                     (0)
#define ADC_STAT_ADF0S_MASK                                      (0x7F << ADC_STAT_ADF0S_SHIFT)

/***************************************************************************************************
 * ADC_FIFO0_DAT
 */
#define ADC_FIFO0_DAT_ADDAT_SHIFT                                (8)
#define ADC_FIFO0_DAT_ADDAT_MASK                                 (0xFFFFFF << ADC_FIFO0_DAT_ADDAT_SHIFT)
#define ADC_FIFO0_DAT_ADDAT(x)                                   ((x) << ADC_FIFO0_DAT_ADDAT_SHIFT)

/***************************************************************************************************
 * AAL0_CTL
 */
#define AAL0_CTL_AAL_EN                                         BIT(17)
#define AAL0_CTL_AAL_RECOVER_MODE                               BIT(16)
#define AAL0_CTL_AAL_FL_SHIFT                                   (14)
#define AAL0_CTL_AAL_FL_MASK                                    (0x3 << AAL0_CTL_AAL_FL_SHIFT)
#define AAL0_CTL_AAL_FL(x)                                      ((x) << AAL0_CTL_AAL_FL_SHIFT)
#define AAL0_CTL_AAL_VT1_SHIFT                                  (12)
#define AAL0_CTL_AAL_VT1_MASK                                   (0x3 << AAL0_CTL_AAL_VT1_SHIFT)
#define AAL0_CTL_AAL_VT1(x)                                     ((x) << AAL0_CTL_AAL_VT1_SHIFT)
#define AAL0_CTL_AAL_VT0_SHIFT                                  (8)
#define AAL0_CTL_AAL_VT0_MASK                                   (0x7 << AAL0_CTL_AAL_VT0_SHIFT)
#define AAL0_CTL_AAL_VT0(x)                                     ((x) << AAL0_CTL_AAL_VT0_SHIFT)
#define AAL0_CTL_AAL_MAX_SHIFT                                  (4)
#define AAL0_CTL_AAL_MAX_MASK                                   (0xF << AAL0_CTL_AAL_MAX_SHIFT)
#define AAL0_CTL_AAL_MAX(x)                                     ((x) << AAL0_CTL_AAL_MAX_SHIFT)
#define AAL0_CTL_AAL_CNT_SHIFT                                  (0)
#define AAL0_CTL_AAL_CNT_MASK                                   (0xF << AAL0_CTL_AAL_CNT_SHIFT)
#define AAL0_CTL_AAL_CNT(x)                                     ((x) << AAL0_CTL_AAL_CNT_SHIFT)

/***************************************************************************************************
 * AAL1_CTL
 */
#define AAL1_CTL_AAL_EN                                         BIT(17)
#define AAL1_CTL_AAL_RECOVER_MODE                               BIT(16)
#define AAL1_CTL_AAL_FL_SHIFT                                   (14)
#define AAL1_CTL_AAL_FL_MASK                                    (0x3 << AAL1_CTL_AAL_FL_SHIFT)
#define AAL1_CTL_AAL_FL(x)                                      ((x) << AAL1_CTL_AAL_FL_SHIFT)
#define AAL1_CTL_AAL_VT1_SHIFT                                  (12)
#define AAL1_CTL_AAL_VT1_MASK                                   (0x3 << AAL1_CTL_AAL_VT1_SHIFT)
#define AAL1_CTL_AAL_VT1(x)                                     ((x) << AAL1_CTL_AAL_VT1_SHIFT)
#define AAL1_CTL_AAL_VT0_SHIFT                                  (8)
#define AAL1_CTL_AAL_VT0_MASK                                   (0x7 << AAL1_CTL_AAL_VT0_SHIFT)
#define AAL1_CTL_AAL_VT0(x)                                     ((x) << AAL1_CTL_AAL_VT0_SHIFT)
#define AAL1_CTL_AAL_MAX_SHIFT                                  (4)
#define AAL1_CTL_AAL_MAX_MASK                                   (0xF << AAL1_CTL_AAL_MAX_SHIFT)
#define AAL1_CTL_AAL_MAX(x)                                     ((x) << AAL1_CTL_AAL_MAX_SHIFT)
#define AAL1_CTL_AAL_CNT_SHIFT                                  (0)
#define AAL1_CTL_AAL_CNT_MASK                                   (0xF << AAL1_CTL_AAL_CNT_SHIFT)
#define AAL1_CTL_AAL_CNT(x)                                     ((x) << AAL1_CTL_AAL_CNT_SHIFT)

/***************************************************************************************************
 * AAL2_CTL
 */
#define AAL2_CTL_AAL_EN                                         BIT(17)
#define AAL2_CTL_AAL_RECOVER_MODE                               BIT(16)
#define AAL2_CTL_AAL_FL_SHIFT                                   (14)
#define AAL2_CTL_AAL_FL_MASK                                    (0x3 << AAL2_CTL_AAL_FL_SHIFT)
#define AAL2_CTL_AAL_FL(x)                                      ((x) << AAL2_CTL_AAL_FL_SHIFT)
#define AAL2_CTL_AAL_VT1_SHIFT                                  (12)
#define AAL2_CTL_AAL_VT1_MASK                                   (0x3 << AAL2_CTL_AAL_VT1_SHIFT)
#define AAL2_CTL_AAL_VT0_SHIFT                                  (8)
#define AAL2_CTL_AAL_VT0_MASK                                   (0x7 << AAL2_CTL_AAL_VT0_SHIFT)
#define AAL2_CTL_AAL_VT0(x)                                     ((x) << AAL2_CTL_AAL_VT0_SHIFT)
#define AAL2_CTL_AAL_MAX_SHIFT                                  (4)
#define AAL2_CTL_AAL_MAX_MASK                                   (0xF << AAL2_CTL_AAL_MAX_SHIFT)
#define AAL2_CTL_AAL_MAX(x)                                     ((x) << AAL2_CTL_AAL_MAX_SHIFT)
#define AAL2_CTL_AAL_CNT_SHIFT                                  (0)
#define AAL2_CTL_AAL_CNT_MASK                                   (0xF << AAL2_CTL_AAL_CNT_SHIFT)
#define AAL2_CTL_AAL_CNT(x)                                     ((x) << AAL2_CTL_AAL_CNT_SHIFT)

/***************************************************************************************************
 * AAL3_CTL
 */
#define AAL3_CTL_AAL_EN                                         BIT(17)
#define AAL3_CTL_AAL_RECOVER_MODE                               BIT(16)
#define AAL3_CTL_AAL_FL_SHIFT                                   (14)
#define AAL3_CTL_AAL_FL_MASK                                    (0x3 << AAL3_CTL_AAL_FL_SHIFT)
#define AAL3_CTL_AAL_FL(x)                                      ((x) << AAL3_CTL_AAL_FL_SHIFT)
#define AAL3_CTL_AAL_VT1_SHIFT                                  (12)
#define AAL3_CTL_AAL_VT1_MASK                                   (0x3 << AAL3_CTL_AAL_VT1_SHIFT)
#define AAL3_CTL_AAL_VT0_SHIFT                                  (8)
#define AAL3_CTL_AAL_VT0_MASK                                   (0x7 << AAL3_CTL_AAL_VT0_SHIFT)
#define AAL3_CTL_AAL_VT0(x)                                     ((x) << AAL3_CTL_AAL_VT0_SHIFT)
#define AAL3_CTL_AAL_MAX_SHIFT                                  (4)
#define AAL3_CTL_AAL_MAX_MASK                                   (0xF << AAL3_CTL_AAL_MAX_SHIFT)
#define AAL3_CTL_AAL_MAX(x)                                     ((x) << AAL3_CTL_AAL_MAX_SHIFT)
#define AAL3_CTL_AAL_CNT_SHIFT                                  (0)
#define AAL3_CTL_AAL_CNT_MASK                                   (0xF << AAL3_CTL_AAL_CNT_SHIFT)
#define AAL3_CTL_AAL_CNT(x)                                     ((x) << AAL3_CTL_AAL_CNT_SHIFT)

/***************************************************************************************************
 * ADC_CTL0
 */
#define ADC0_CTL_VRDA0_EN                                       BIT(20) /* VRAD0 enable */
#define ADC0_CTL_ADC0_BINV                                      BIT(19) /* ADC0 channel output phase invert */
#define ADC0_CTL_FDBUF0_IRS_SHIFT                               (16) /* FDBUF0 input resistor select */
#define ADC0_CTL_FDBUF0_IRS_MASK                                (0x7 << ADC0_CTL_FDBUF0_IRS_SHIFT)
#define ADC0_CTL_FDBUF0_IRS(x)                                  ((x) << ADC0_CTL_FDBUF0_IRS_SHIFT)
#define ADC0_CTL_PREAM0_PG_SHIFT                                (12) /* PREAMP0 OP feedback resistor select */
#define ADC0_CTL_PREAM0_PG_MASK                                 (0xF << ADC0_CTL_PREAM0_PG_SHIFT)
#define ADC0_CTL_PREAM0_PG(x)                                   ((x) << ADC0_CTL_PREAM0_PG_SHIFT)
#define ADC0_CTL_ADC0_CAPFC_EN                                  BIT(11) /* input cap to ADC 0 channel fast charge enable */
#define ADC0_CTL_FDBUF0_EN                                      BIT(10) /* FD BUF OP 0 enable */
#define ADC0_CTL_PREOP0_EN                                      BIT(9) /* PREOP 0 enable */
#define ADC0_CTL_ADC0_EN                                        BIT(8) /* ADC0 channel sdm enable */
#define ADC0_CTL_INPUT0N_EN_SHIFT                               (6) /* INPUT0N pad to ADC0/2 channel input enable */
#define ADC0_CTL_INPUT0N_EN_MASK                                (0x3 << ADC0_CTL_INPUT0N_EN_SHIFT)
#define ADC0_CTL_INPUT0N_EN(x)                                  ((x) << ADC0_CTL_INPUT0N_EN_SHIFT)
#define ADC0_CTL_INPUT0P_EN_SHIFT                               (4) /* INPUT0P pad to ADC0 channel input enable */
#define ADC0_CTL_INPUT0P_EN_MASK                                (0x3 << ADC0_CTL_INPUT0P_EN_SHIFT)
#define ADC0_CTL_INPUT0P_EN(x)                                  ((x) << ADC0_CTL_INPUT0P_EN_SHIFT)
#define ADC0_CTL_INPUT0_IN_MODE                                 BIT(2) /* 0: differential input mode; 1: single end input mode */
#define ADC0_CTL_INPUT0_IRS_SHIFT                               (0) /* INPUT0 input resistor select */
#define ADC0_CTL_INPUT0_IRS_MASK                                (0x3 << ADC0_CTL_INPUT0_IRS_SHIFT)
#define ADC0_CTL_INPUT0_IRS(x)                                  ((x) << ADC0_CTL_INPUT0_IRS_SHIFT)

/***************************************************************************************************
 * ADC_CTL1
 */
#define ADC1_CTL_VRDA1_EN                                       BIT(20) /* VRAD1 enable */
#define ADC1_CTL_ADC1_BINV                                      BIT(19) /* ADC1 channel output phase invert */
#define ADC1_CTL_FDBUF1_IRS_SHIFT                               (16) /* FDBUF1 input resistor select */
#define ADC1_CTL_FDBUF1_IRS_MASK                                (0x7 << ADC1_CTL_FDBUF1_IRS_SHIFT)
#define ADC1_CTL_FDBUF1_IRS(x)                                  ((x) << ADC1_CTL_FDBUF1_IRS_SHIFT)
#define ADC1_CTL_PREAM1_PG_SHIFT                                (12) /* PREAMP1 OP feedback resistor select */
#define ADC1_CTL_PREAM1_PG_MASK                                 (0xF << ADC1_CTL_PREAM1_PG_SHIFT)
#define ADC1_CTL_PREAM1_PG(x)                                   ((x) << ADC1_CTL_PREAM1_PG_SHIFT)
#define ADC1_CTL_ADC1_CAPFC_EN                                  BIT(11) /* input cap to ADC 1 channel fast charge enable */
#define ADC1_CTL_FDBUF1_EN                                      BIT(10) /* FD BUF OP 1 enable */
#define ADC1_CTL_PREOP1_EN                                      BIT(9) /* PREOP 1 enable */
#define ADC1_CTL_ADC1_EN                                        BIT(8) /* ADC1 channel sdm enable */
#define ADC1_CTL_INPUT1N_EN_SHIFT                               (6) /* INPUT1N pad to ADC1/3 channel input enable */
#define ADC1_CTL_INPUT1N_EN_MASK                                (0x3 << ADC0_CTL_INPUT0N_EN_SHIFT)
#define ADC1_CTL_INPUT1N_EN(x)                                  ((x) << ADC0_CTL_INPUT0N_EN_SHIFT)
#define ADC1_CTL_INPUT1P_EN_SHIFT                               (4) /* INPUT1P pad to ADC1 channel input enable */
#define ADC1_CTL_INPUT1P_EN_MASK                                (0x3 << ADC0_CTL_INPUT0P_EN_SHIFT)
#define ADC1_CTL_INPUT1P_EN(x)                                  ((x) << ADC0_CTL_INPUT0P_EN_SHIFT)
#define ADC1_CTL_INPUT1_IN_MODE                                 BIT(2) /* 0: differential input mode; 1: single end input mode */
#define ADC1_CTL_INPUT1_IRS_SHIFT                               (0) /* INPUT1 input resistor select */
#define ADC1_CTL_INPUT1_IRS_MASK                                (0x3 << ADC0_CTL_INPUT0_IRS_SHIFT)
#define ADC1_CTL_INPUT1_IRS(x)                                  ((x) << ADC0_CTL_INPUT0_IRS_SHIFT)

/***************************************************************************************************
 * ADC_CTL2
 */
#define ADC2_CTL_MIX2AD2FDSE                                    BIT(28) /* mix out to ADC2 input mode select */
#define ADC2_CTL_PAR2AD2_EN                                     BIT(27) /* PA OUTR mix to ADC2 channel */
#define ADC2_CTL_PAL2AD2_EN                                     BIT(26) /* PA OUTL mix to ADC2 channel */
#define ADC2_CTL_PAR2AD2_PD_EN                                  BIT(25) /* PA OUTR mix to ADC2 channel pull down */
#define ADC2_CTL_PAL2AD2_PD_EN                                  BIT(24) /* PA OUTL mix to ADC2 channel pull down */
#define ADC2_CTL_VRDA2_EN                                       BIT(20) /* VRDA2 enable */
#define ADC2_CTL_ADC2_BINV                                      BIT(19) /* ADC2 channel output phase invert */
#define ADC2_CTL_FDBUF2_IRS_SHIFT                               (16) /* FDBUF2 input resistor select */
#define ADC2_CTL_FDBUF2_IRS_MASK                                (0x7 << ADC2_CTL_FDBUF2_IRS_SHIFT)
#define ADC2_CTL_FDBUF2_IRS(x)                                  ((x) << ADC2_CTL_FDBUF2_IRS_SHIFT)
#define ADC2_CTL_PREAM2_PG_SHIFT                                (12) /* PREAMP2 OP feedback resistor select */
#define ADC2_CTL_PREAM2_PG_MASK                                 (0xF << ADC2_CTL_PREAM2_PG_SHIFT)
#define ADC2_CTL_ADC2_CAPFC_EN                                  BIT(11) /* input cap to ADC2 channel fast charge enable */
#define ADC2_CTL_FDBUF2_EN                                      BIT(10) /* FD BUF OP2 enable for SE mode */
#define ADC2_CTL_PREOP2_EN                                      BIT(9) /* PREOP2 enable */
#define ADC2_CTL_ADC2_EN                                        BIT(8) /* ADC2 channel sdm enable */
#define ADC2_CTL_INPUT2N_EN_SHIFT                               (6) /* INPUT2N pad to ADC2/3 channel input enable */
#define ADC2_CTL_INPUT2N_EN_MASK                                (0x3 << ADC2_CTL_INPUT2N_EN_SHIFT)
#define ADC2_CTL_INPUT2N_EN(x)                                  ((x) << ADC2_CTL_INPUT2N_EN_SHIFT)
#define ADC2_CTL_INPUT2P_EN_SHIFT                               (4) /* INPUT2P pad to ADC2 channel input enable */
#define ADC2_CTL_INPUT2P_EN_MASK                                (0x3 << ADC2_CTL_INPUT2P_EN_SHIFT)
#define ADC2_CTL_INPUT2P_EN(x)                                  ((x) << ADC2_CTL_INPUT2P_EN_SHIFT)
#define ADC2_CTL_INPUT2_IN_MODE                                 BIT(2) /* 0: differential input mode; 1: single end input mode */
#define ADC2_CTL_INPUT2_IRS_SHIFT                               (0) /* INPUT2 input resistor select */
#define ADC2_CTL_INPUT2_IRS_MASK                                (0x3 << ADC2_CTL_INPUT2_IRS_SHIFT)
#define ADC2_CTL_INPUT2_IRS(x)                                  ((x) << ADC2_CTL_INPUT2_IRS_SHIFT)

/***************************************************************************************************
 * ADC_CTL3
 */
#define ADC3_CTL_MIX2AD3FDSE                                    BIT(28)
#define ADC3_CTL_PAR2AD3_EN                                     BIT(27)
#define ADC3_CTL_PAL2AD3_EN                                     BIT(26)
#define ADC3_CTL_PAR2AD3_PD_EN                                  BIT(25)
#define ADC3_CTL_PAL2AD3_PD_EN                                  BIT(24)
#define ADC3_CTL_VRDA3_EN                                       BIT(20)
#define ADC3_CTL_ADC3_BINV                                      BIT(19)
#define ADC3_CTL_FDBUF3_IRS_SHIFT                               (16)
#define ADC3_CTL_FDBUF3_IRS_MASK                                (0x7 << ADC3_CTL_FDBUF3_IRS_SHIFT)
#define ADC3_CTL_FDBUF3_IRS(x)                                  ((x) << ADC3_CTL_FDBUF3_IRS_SHIFT)
#define ADC3_CTL_PREAM3_PG_SHIFT                                (12)
#define ADC3_CTL_PREAM3_PG_MASK                                 (0xF << ADC3_CTL_PREAM3_PG_SHIFT)
#define ADC3_CTL_PREAM3_PG(x)                                   ((x) << ADC3_CTL_PREAM3_PG_SHIFT)
#define ADC3_CTL_ADC3_CAPFC_EN                                  BIT(11)
#define ADC3_CTL_FDBUF3_EN                                      BIT(10)
#define ADC3_CTL_PREOP3_EN                                      BIT(9)
#define ADC3_CTL_ADC3_EN                                        BIT(8)
#define ADC3_CTL_INPUT3N_EN_SHIFT                               (6) /* INPUT3N pad to ADC2/3 channel input enable */
#define ADC3_CTL_INPUT3N_EN_MASK                                (0x3 << ADC3_CTL_INPUT3N_EN_SHIFT)
#define ADC3_CTL_INPUT3N_EN(x)                                  ((x) << ADC3_CTL_INPUT3N_EN_SHIFT)
#define ADC3_CTL_INPUT3P_EN_SHIFT                               (4) /* INPUT3P pad to ADC3 channel input enable */
#define ADC3_CTL_INPUT3P_EN_MASK                                (0x3 << ADC3_CTL_INPUT3P_EN_SHIFT)
#define ADC3_CTL_INPUT3P_EN(x)                                  ((x) << ADC3_CTL_INPUT3P_EN_SHIFT)
#define ADC3_CTL_INPUT3_IN_MODE                                 BIT(2)
#define ADC3_CTL_INPUT3_IRS_SHIFT                               (0) /* INPUT3 input resistor select */
#define ADC3_CTL_INPUT3_IRS_MASK                                (0x3 << ADC3_CTL_INPUT3_IRS_SHIFT)
#define ADC3_CTL_INPUT3_IRS(x)                                  ((x) << ADC3_CTL_INPUT3_IRS_SHIFT)

/***************************************************************************************************
 * ADC_BIAS
 */
#define ADC_BIAS_BIASEN                                         BIT(28) /* BIAS enable and if use ADC, this bit shall be enabled */
#define ADC_BIAS_BIASSEL_SHIFT                                  (26)
#define ADC_BIAS_BIASSEL_MASK                                   (0x3 << ADC_BIAS_BIASSEL_SHIFT)
#define ADC_BIAS_VRDA_IQS_SHIFT                                 (24)
#define ADC_BIAS_VRDA_IQS_MASK                                  (0x3 << ADC_BIAS_VRDA_IQS_SHIFT)
#define ADC_BIAS_PREOP_ODSC                                     BIT(22)
#define ADC_BIAS_PREOP_IQS_SHIFT                                (20)
#define ADC_BIAS_PREOP_IQS_MASK                                 (0x3 << ADC_BIAS_PREOP_IQS_SHIFT)
#define ADC_BIAS_OPBUF_ODSC_SHIFT                               (18)
#define ADC_BIAS_OPBUF_ODSC_MASK                                (0x3 << ADC_BIAS_OPBUF_ODSC_SHIFT)
#define ADC_BIAS_OPBUF_IQS_SHIFT                                (16)
#define ADC_BIAS_OPBUF_IQS_MASK                                 (0x3 << ADC_BIAS_OPBUF_IQS_SHIFT)
#define ADC_BIAS_IAD3_SHIFT                                     (14)
#define ADC_BIAS_IAD3_MASK                                      (0x3 << ADC_BIAS_IAD3_SHIFT)
#define ADC_BIAS_IAD2_SHIFT                                     (12)
#define ADC_BIAS_IAD2_MASK                                      (0x3 << ADC_BIAS_IAD2_SHIFT)
#define ADC_BIAS_IAD1_SHIFT                                     (8)
#define ADC_BIAS_IAD1_MASK                                      (0x7 << ADC_BIAS_IAD1_SHIFT)
#define ADC_BIAS_VRDA_IB_SHIFT                                  (5)
#define ADC_BIAS_VRDA_IB_MASK                                   (0x7 << ADC_BIAS_VRDA_IB_SHIFT)
#define ADC_BIAS_OPBUF_IB_SHIFT                                 (3)
#define ADC_BIAS_OPBUF_IB_MASK                                  (0x3 << ADC_BIAS_OPBUF_IB_SHIFT)
#define ADC_BIAS_PREOP_IB_SHIFT                                 (0)
#define ADC_BIAS_PREOP_IB_MASK                                  (0x7 << ADC_BIAS_PREOP_IB_SHIFT)

/***************************************************************************************************
 * ADC_VMIC_CTL
 */
#define ADC_VMIC_CTL_ISO_VD18                                   BIT(23) /* isolate the MFP sio of input pin */
#define ADC_VMIC_CTL_ISO_AVCC_AU                                BIT(22) /* isolation of capless ldo AVCC_AU power */
#define ADC_VMIC_CTL_VMIC2_R_SEL                                BIT(20) /* divider resistor of VMIC2 control */
#define ADC_VMIC_CTL_VMIC1_R_SEL                                BIT(19) /* divider resistor of VMIC1 control */
#define ADC_VMIC_CTL_VMIC0_R_SEL                                BIT(18) /* divider resistor of VIMIC0 control */
#define ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT                        (16) /* VMIC I bias control */
#define ADC_VMIC_CTL_VMIC_BIAS_CTL_MASK                         (0x3 << ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT)
#define ADC_VMIC_CTL_VMIC_BIAS_CTL(x)                           ((x) << ADC_VMIC_CTL_VMIC_BIAS_CTL_SHIFT)
#define ADC_VMIC_CTL_VMIC_IB_SEL                                BIT(14) /* VMIC I bias select. 0: from PMU; 1: from VMIC I bias generation */
#define ADC_VMIC_CTL_VMIC_IBGEN_CTL                             BIT(13) /* VMIC I bias generation current control */
#define ADC_VMIC_CTL_VMIC_IBGEN_EN                              BIT(12) /* VMIC I bias generation enable */
#define ADC_VMIC_CTL_VMIC2_VOL_SHIFT                            (10) /* VMIC2 output voltage control */
#define ADC_VMIC_CTL_VMIC2_VOL_MASK                             (0x3 << ADC_VMIC_CTL_VMIC2_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC2_VOL(x)                               ((x) << ADC_VMIC_CTL_VMIC2_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC2_EN_SHIFT                             (8) /* VMIC2 enable */
#define ADC_VMIC_CTL_VMIC2_EN_MASK                              (0x3 << ADC_VMIC_CTL_VMIC2_EN_SHIFT)
#define ADC_VMIC_CTL_VMIC2_EN(x)                                ((x) << ADC_VMIC_CTL_VMIC2_EN_SHIFT)
#define ADC_VMIC_CTL_VMIC1_VOL_SHIFT                            (6) /* VMIC1 output voltage control */
#define ADC_VMIC_CTL_VMIC1_VOL_MASK                             (0x3 << ADC_VMIC_CTL_VMIC1_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC1_VOL(x)                               ((x) << ADC_VMIC_CTL_VMIC1_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC1_EN_SHIFT                             (4) /* VMIC1 enable. 0x: disable VMIC1 op; 2: bypass VMIC1 op; 3: enable VMIC1 op */
#define ADC_VMIC_CTL_VMIC1_EN_MASK                              (0x3 << ADC_VMIC_CTL_VMIC1_EN_SHIFT)
#define ADC_VMIC_CTL_VMIC1_EN(x)                                ((x) << ADC_VMIC_CTL_VMIC1_EN_SHIFT)
#define ADC_VMIC_CTL_VMIC0_VOL_SHIFT                            (2) /* VMIC0 output voltage control */
#define ADC_VMIC_CTL_VMIC0_VOL_MASK                             (0x3 << ADC_VMIC_CTL_VMIC0_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC0_VOL(x)                               ((x) << ADC_VMIC_CTL_VMIC0_VOL_SHIFT)
#define ADC_VMIC_CTL_VMIC0_EN_SHIFT                             (0)
#define ADC_VMIC_CTL_VMIC0_EN_MASK                              (0x3 << ADC_VMIC_CTL_VMIC0_EN_SHIFT)
#define ADC_VMIC_CTL_VMIC0_EN(x)                                ((x) << ADC_VMIC_CTL_VMIC0_EN_SHIFT)

/***************************************************************************************************
 * ADC_REF_LDO_CTL
 */
#define ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT                      (18) /* AULDO pull down current control. 0: small; 3: large */
#define ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK                       (0x3 << ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_PD_CTL(x)                         ((x) << ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_BIAS_CTL_SHIFT                    (16) /* AUDIO I bias control */
#define ADC_REF_LDO_CTL_AULDO_BIAS_CTL_MASK                     (0x3 << ADC_REF_LDO_CTL_AULDO_BIAS_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_BIAS_CTL(x)                       ((x) << ADC_REF_LDO_CTL_AULDO_BIAS_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_IB_SEL                            BIT(14) /* AULDO I bias select */
#define ADC_REF_LDO_CTL_AULDO_IBGEN_CTL                         BIT(13) /* AULDO I bias generation current control */
#define ADC_REF_LDO_CTL_AULDO_IBGEN_EN                          BIT(12) /* AULDO I bias generation enable */
#define ADC_REF_LDO_CTL_AULDO_VOL_SHIFT                         (10) /* AULDO output voltage control */
#define ADC_REF_LDO_CTL_AULDO_VOL_MASK                          (0x3 << ADC_REF_LDO_CTL_AULDO_VOL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_VOL(x)                            ((x) << ADC_REF_LDO_CTL_AULDO_VOL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_EN_SHIFT                          (8) /* AULDO enable for ADC */
#define ADC_REF_LDO_CTL_AULDO_EN_MASK                           (0x3 << ADC_REF_LDO_CTL_AULDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_EN(x)                             ((x) << ADC_REF_LDO_CTL_AULDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_DALDO_EN_SHIFT                          (6) /* DALDO enable for DAC */
#define ADC_REF_LDO_CTL_DALDO_EN_MASK                           (0x3 << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_DALDO_EN(x)                             ((x) << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_VREF_RSEL_SHIFT                         (2) /* VREF voltage divide res control */
#define ADC_REF_LDO_CTL_VREF_RSEL_MASK                          (0x3 << ADC_REF_LDO_CTL_VREF_RSEL_SHIFT)
#define ADC_REF_LDO_CTL_VREF_RSEL(x)                            ((x) << ADC_REF_LDO_CTL_VREF_RSEL_SHIFT)
#define ADC_REF_LDO_CTL_VREF_FU                                 BIT(1) /* VREF fastup control */
#define ADC_REF_LDO_CTL_VREF_EN                                 BIT(0) /* VREF enable control */

/***************************************************************************************************
 * HW_TRIGGER_ADC_CTL
 */
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC3_EN                       BIT(7)
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC2_EN                       BIT(6)
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC1_EN                       BIT(5)
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC0_EN                       BIT(4)
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC_SHIFT                     (4)
#define HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK                      (0xF << HW_TRIGGER_ADC_CTL_INT_TO_ADC_SHIFT)
#define HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT                (0)
#define HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_MASK                 (0xF << HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT)
#define HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL(x)                   ((x) << HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_SHIFT)

/***************************************************************************************************
 * ADC_DEBUG
 */
#define DEBUGSEL                                               (0x40068400)
#define DEBUGIE0                                               (0x40068410)
#define DEBUGOE0                                               (0x40068420)
#define DEBUGSEL_DBGSE_SHIFT                                   (0)
#define DEBUGSEL_DBGSE_MASK                                    (0x3F << DEBUGSEL_DBGSE_SHIFT)
#define DEBUGSEL_DBGSE(x)                                      ((x) << DEBUGSEL_DBGSE_SHIFT)
#define DBGSE_ADC											   (0xd)

/***************************************************************************************************
 * ADC FEATURES CONGIURATION
 */
#define ADC_FIFO_MAX_NUMBER                                    (1)

#define ADC_FIFO_MAX_DRQ_LEVEL                                 (127)
#define ADC_FIFO_DRQ_LEVEL_DEFAULT                             (32) /* 32 level */

#define ADC_OSR_DEFAULT                                        (1) /* ADC_OSR_128FS */

#define ADC_DIGITAL_CH_GAIN_MAX                                (0xF) /* 52.5dB */
#define ADC_DIGITAL_DMIC_GAIN_MAX                              (0x7) /* 63x */

#define ADC_FEEDBACK_RES_INVALID                               (0xFF) /* invalid ADC feedback resistor */

#define ADC_MAX_CHANNELS_NUMBER                                (4) /* max ADC channels number */

#define ADC_HPF_HIGH_FREQ_HZ                                   (500) /* 500Hz for high frequency */

#define ADC_HPF_FAST_STABLE_MS                                 (50) /* 10 milliseconds for HFP fast stable */

#define ADC_FAST_CAP_CHARGE_TIME_MS                            (80) /* 80 milliseconds for CAP charging */

#define ADC_LDO_CAPACITOR_CHARGE_TIME_MS                       (5) /* Wait time for ADC input capacitor charge full */

//#define ADC_ANALOG_DEBUG_OUT_ENABLE

#ifdef CONFIG_SOC_SERIES_LARK_FPGA
#define ADC_DIGITAL_DEBUG_IN_ENABLE
#endif

#define ADC_CH2REG(base, x)                                    ((uint32_t)&((base)->ch0_digctl) + ((x) << 2))
#define ADC_CTL2REG(base, x)                                   ((uint32_t)&((base)->adc_ctl0) + ((x) << 2))

/* @brief the macro to configure the ADC channels */
#define ADC_CHANNEL_CFG(n, en) \
    { \
        if (ADC_CH_DISABLE != ch##n##_input) { \
            if (ADC_CH_DMIC == ch##n##_input) { \
                adc_hpf_config(dev, n, en); \
                __adc_digital_channel_cfg(dev, ADC_CHANNEL_##n, ADC_DMIC, en); \
            } else { \
                adc_hpf_config(dev, n, en); \
                __adc_digital_channel_cfg(dev, ADC_CHANNEL_##n, ADC_AMIC, en); \
            } \
        } \
    }

/* @brief the macro to control ADC channels to enable or disable */
#define ADC_CHANNELS_CTL(en) \
    { \
        if (en) { \
            ADC_CHANNEL_CFG(0, true); \
            ADC_CHANNEL_CFG(1, true); \
            ADC_CHANNEL_CFG(2, true); \
            ADC_CHANNEL_CFG(3, true); \
        } else { \
            ADC_CHANNEL_CFG(0, false); \
            ADC_CHANNEL_CFG(1, false); \
            ADC_CHANNEL_CFG(2, false); \
            ADC_CHANNEL_CFG(3, false); \
        } \
    }

/* @brief the macro to set the DMIC gain */
#define ADC_DMIC_GAIN_CFG(n) \
    { \
        uint8_t dmic_gain; \
        if ((ADC_CH_DMIC == ch##n##_input) && (ADC_GAIN_INVALID != ch##n##_gain)) { \
            if (adc_dmic_gain_translate(ch##n##_gain, &dmic_gain, &dig_gain)) { \
                LOG_DBG("failed to translate dmic ch%d gain %d", n, ch##n##_gain); \
                return -EFAULT; \
            } \
            __adc_digital_gain_set(dev, ADC_CHANNEL_##n, dig_gain); \
            __adc_dmic_gain_set(dev, dmic_gain); \
        } \
    }

/**
 * @struct acts_audio_adc
 * @brief ADC controller hardware register
 */
struct acts_audio_adc {
    volatile uint32_t adc_digctl;		/* ADC digital control */
    volatile uint32_t ch0_digctl;		/* channel0 digital control */
    volatile uint32_t ch1_digctl;		/* channel1 digital control */
    volatile uint32_t ch2_digctl;		/* channel2 digital control */
    volatile uint32_t ch3_digctl;		/* channel3 digital control */
    volatile uint32_t fifoctl;			/* ADC fifo control */
    volatile uint32_t stat;				/* ADC stat */
    volatile uint32_t fifo0_dat;		/* ADC FIFO0 data */
    volatile uint32_t aal0_ctl;			/* ADC0 auto amplitude limit control */
    volatile uint32_t aal1_ctl;			/* ADC1 auto amplitude limit control */
    volatile uint32_t aal2_ctl;			/* ADC2 auto amplitude limit control */
    volatile uint32_t aal3_ctl;			/* ADC3 auto amplitude limit control */
    volatile uint32_t adc_ctl0;			/* ADC control0 */
    volatile uint32_t adc_ctl1;			/* ADC control1 */
    volatile uint32_t adc_ctl2;			/* ADC control2 */
    volatile uint32_t adc_ctl3;			/* ADC control3 */
    volatile uint32_t bias;				/* ADC bias */
    volatile uint32_t vmic_ctl;			/* VMIC control */
    volatile uint32_t ref_ldo_ctl;		/* ADC reference LDO control */
    volatile uint32_t hw_trigger_ctl;	/* ADC HW trigger ADC control */
};

#ifdef CONFIG_CFG_DRV
/**
 * struct phy_adc_external_config
 * @brief The ADC external configuration which generated by configuration tool.
 */
struct phy_adc_external_config {
    cfg_uint32 ADC_Bias_Setting; /* ADC bias setting */
    cfg_uint8 DMIC01_Channel_Aligning; /* DMIC latch policy selection. 0: L/R 1:R/L */
    cfg_uint8 DMIC23_Channel_Aligning; /* DMIC latch policy selection. 2: L/R 3:R/L */
    CFG_Type_DMIC_Select_GPIO DMIC_Select_GPIO; /* DMIC GPIO pin  */
    cfg_uint8 Enable_ANC; /* ANC configuration. 0:disable; 1:AUDIO_ANC_FF; 2:AUDIO_ANC_FB; 3:AUDIO_ANC_FY */
    CFG_Type_DMIC_Select_GPIO ANCDMIC_Select_GPIO; /* DMIC GPIO pin for ANC */
    cfg_uint8 Record_Adc_Select; /* ADC type selection */
    cfg_uint8 Enable_VMIC; /* VMIC power supply enable */
    cfg_uint8 Hw_Aec_Select; /* Hardware AEC enable */
    CFG_Type_Mic_Config Mic_Config[CFG_MAX_ADC_NUM]; /* MIC configuration */
    CFG_Type_ADC_Select_INPUT ADC_Select_INPUT; /* ADC input selection */
    cfg_int16 ANC_FF_GAIN; /* ANC FF MIC gain */
    cfg_int16 ANC_FB_GAIN; /* ANC FB MIC gain */
};
#endif

/**
 * struct phy_adc_drv_data
 * @brief The software related data that used by physical adc driver.
 */
struct phy_adc_drv_data {
#ifdef CONFIG_CFG_DRV
        struct phy_adc_external_config external_config; /* ADC external configuration */
        uint8_t input_ch0; /* ADC channel0 INPUT selection after parser */
        uint8_t input_ch1; /* ADC channel1 INPUT selection after parser */
        uint8_t input_ch2; /* ADC channel2 INPUT selection after parser */
        uint8_t input_ch3; /* ADC channel3 INPUT selection after parser */
#endif

    uint8_t hw_trigger_en : 1; /* If 1 to enable hw IRQ signal to trigger ADC digital start */
    uint8_t anc_en : 1; /* If 1 to indicate ANC has been enabled */
	uint8_t audio_pll_index : 1; /* The index of audio pll */
};

/**
 * union phy_adc_features
 * @brief The infomation from DTS to control the ADC features to enable or nor.
 */
typedef union {
    uint64_t raw;
    struct {
        uint64_t adc0_hpf_time : 2; /* ADC0 HPF auto-set time */
        uint64_t adc1_hpf_time : 2; /* ADC1 HPF auto-set time */
        uint64_t adc2_hpf_time : 2; /* ADC2 HPF auto-set time */
        uint64_t adc3_hpf_time : 2; /* ADC3 HPF auto-set time */
        uint64_t adc0_hpf_fc_high: 1; /* ADC0 HPF use high frequency range */
        uint64_t adc1_hpf_fc_high: 1; /* ADC1 HPF use high frequency range */
        uint64_t adc2_hpf_fc_high: 1; /* ADC2 HPF use high frequency range */
        uint64_t adc3_hpf_fc_high: 1; /* ADC3 HPF use high frequency range */
        uint64_t adc0_frequency : 6; /* ADC0 HFP frequency */
        uint64_t adc1_frequency : 6; /* ADC1 HFP frequency */
        uint64_t adc2_frequency : 6; /* ADC2 HFP frequency */
        uint64_t adc3_frequency : 6; /* ADC2 HFP frequency */
        uint64_t ldo_voltage : 2; /* AUDIO LDO voltage */
        uint64_t fast_cap_charge : 1; /* Fast CAP charge function */
    } v;
} phy_adc_features;

#ifndef CONFIG_CFG_DRV
static uint8_t vmic_ctl_array[] = CONFIG_AUDIO_ADC_0_VMIC_CTL_ARRAY;
#else
static uint8_t adc_vmic_index_mapping_array[] = CONFIG_AUDIO_ADC_0_VMIC_MAPPING;
#endif

static uint8_t vmic_voltage_array[] = CONFIG_AUDIO_ADC_0_VMIC_VOLTAGE_ARRAY;

/**
 * struct phy_adc_config_data
 * @brief The hardware related data that used by physical adc driver.
 */
struct phy_adc_config_data {
    uint32_t reg_base; /* ADC controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
    uint8_t clk_id; /* ADC devclk id */
    uint8_t rst_id; /* ADC reset id */
    phy_adc_features features; /* ADC features */
};

struct adc_amic_aux_gain_setting {
    int16_t gain;
    uint8_t input_res;
    uint8_t feedback_res;
    uint8_t digital_gain;
};

struct adc_dmic_gain_setting {
    int16_t gain;
    uint8_t dmic_pre_gain;
    uint8_t digital_gain;
};

struct adc_anc_clk_setting {
    uint8_t sample_rate;
    uint16_t root_clk_div; /* a_mclk_type_e */
    uint8_t ovfs_clk_div; /* 0: div1; 1: div2; 2: div4 */
    uint8_t fir_clk_div; /* 0: div1; 1: div3 */
};

/**
  * @struct adc_amic_aux_gain_setting
  * @brief The gain mapping table of the analog mic and aux.
  * @note By the SD suggestion, it is suitable to ajust the analog gain when below 20dB.
  * Whereas, it is the same effect to ajust the analog or digital gian when above 20dB.
  */
static const struct adc_amic_aux_gain_setting amic_aux_gain_mapping[] = {
    {-120, 0, 0, 0},
    {-90, 0, 1, 0},
    {-60, 0, 2, 0},
    {-30, 0, 3, 0},
    {0, 0, 4, 0},
    {30, 0, 5, 0},
    {60, 0, 6, 0},
    {75, 0, 7, 0},
    {90, 0, 8, 0},
    {105, 0, 9, 0},
    {120, 0, 10, 0},
    {135, 0, 11, 0},
    {150, 0, 12, 0},
    {165, 0, 13, 0},
    {180, 0, 14, 0},
    {195, 0, 15, 0},
    {210, 1, 12, 0},
    {225, 1, 13, 0},
    {230, 0, 15, 1},
    {240, 1, 14, 0},
    {245, 1, 12, 1},
    {250, 1, 14, 2},
    {255, 1, 15, 0},
    {260, 1, 13, 1},
    {265, 0, 15, 2},
    {270, 2, 12, 0},
    {275, 1, 14, 1},
    {280, 1, 12, 2},
    {285, 2, 13, 0},
    {290, 1, 15, 1},
    {295, 1, 13, 2},
    {300, 2, 14, 0},
    {305, 2, 12, 1},
    {310, 1, 14, 2},
    {315, 2, 15, 0},
    {320, 2, 13, 1},
    {325, 1, 15, 2},
    {330, 1, 13, 3},
    {335, 2, 14, 1},
    {340, 2, 12, 1},
    {345, 1, 14, 3},
    {350, 2, 15, 1},
    {355, 2, 13, 2},
    {360, 1, 15, 3},
    {365, 1, 13, 4},
    {370, 2, 14, 2},
    {375, 2, 12, 3},
    {380, 1, 14, 4},
    {385, 2, 15, 2},
    {390, 2, 13, 3},
    {395, 1, 15, 4},
    {400, 1, 13, 5},
    {405, 2, 14, 3},
    {410, 2, 12, 4},
    {415, 1, 14, 5},
    {420, 2, 15, 3},
    {425, 2, 13, 4},
    {430, 1, 15, 5},
    {435, 1, 13, 6},
    {440, 2, 14, 4},
    {445, 2, 12, 5},
    {450, 1, 14, 6},
    {455, 2, 15, 4},
    {460, 2, 13, 5},
    {465, 1, 15, 6},
    {470, 1, 13, 7},
    {475, 2, 14, 5},
    {480, 2, 12, 6},
    {485, 1, 14, 7},
    {490, 2, 15, 5},
    {495, 2, 13, 6},
    {500, 1, 15, 7},
    {505, 1, 13, 8},
    {510, 2, 14, 6},
    {515, 2, 12, 7},
    {520, 1, 14, 8},
    {525, 2, 15, 6},
    {530, 2, 13, 7},
    {535, 1, 15, 8},
    {540, 1, 13, 9},
    {545, 2, 14, 7},
    {550, 2, 12, 8},
    {555, 1, 14, 9},
    {560, 2, 15, 7},
    {565, 2, 13, 8},
    {570, 1, 15, 9},
    {575, 1, 13, 10},
    {580, 2, 14, 8},
    {585, 2, 12, 9},
    {590, 1, 14, 10},
    {595, 2, 15, 8},
    {600, 2, 13, 9},
    {605, 1, 15, 10},
    {610, 1, 13, 11},
    {615, 2, 14, 9},
    {620, 2, 12, 10},
    {625, 1, 14, 11},
    {630, 2, 15, 9},
    {635, 2, 13, 10},
    {640, 1, 15, 11},
    {645, 1, 13, 12},
    {650, 2, 14, 10},
    {655, 2, 12, 11},
    {660, 1, 14, 12},
    {665, 2, 15, 10},
    {670, 2, 13, 11},
    {675, 1, 15, 12},
    {680, 1, 13, 13},
    {685, 2, 14, 11},
    {690, 2, 12, 12},
    {695, 1, 14, 13},
    {700, 2, 15, 11},
    {705, 2, 13, 12},
    {710, 1, 15, 13},
    {715, 1, 13, 14},
    {720, 2, 14, 12},
    {725, 2, 12, 13},
    {730, 1, 14, 14},
    {735, 2, 15, 12},
    {740, 2, 13, 13},
    {745, 1, 15, 14},
    {750, 1, 13, 15},
    {755, 2, 14, 13},
    {760, 2, 12, 14},
    {765, 1, 14, 15},
    {770, 2, 15, 13},
    {775, 2, 13, 14},
    {780, 1, 15, 15},
    {795, 2, 12, 15},
    {810, 2, 13, 15},
    {825, 2, 14, 15},
    {840, 2, 15, 15},
};

#ifdef CONFIG_ADC_DMIC
/* dB = 20 x log(x) */
static const struct adc_dmic_gain_setting dmic_gain_mapping[] = {
    {0, 0, 0},
    {60, 1, 0},
    {120, 2, 0},
    {180, 3, 0},
    {240, 4, 0},
    {300, 5, 0},
    {360, 6, 0},
    {395, 6, 1},
    {430, 6, 2},
    {465, 6, 3},
    {500, 6, 4},
    {535, 6, 5},
    {570, 6, 6},
    {605, 6, 7},
    {640, 6, 8},
    {675, 6, 9},
    {710, 6, 10},
    {745, 6, 11},
    {780, 6, 12},
    {815, 6, 13},
    {850, 6, 14},
    {885, 6, 15},
};
#endif

static const struct adc_anc_clk_setting adc_anc_clk_mapping[] = {
    {SAMPLE_RATE_16KHZ, MCLK_768FS, 1, 1},
    {SAMPLE_RATE_22KHZ, MCLK_256FS, 0, 0},
    {SAMPLE_RATE_24KHZ, MCLK_256FS, 0, 0},
    {SAMPLE_RATE_32KHZ, MCLK_768FS, 2, 1},
    {SAMPLE_RATE_44KHZ, MCLK_256FS, 1, 0},
    {SAMPLE_RATE_48KHZ, MCLK_256FS, 1, 0},
};

/**
 * enum a_adc_fifo_e
 * @brief ADC fifo index selection
 */
typedef enum {
    ADC_FIFO_0 = 0,
} a_adc_fifo_e;

/**
 * enum a_adc_ovfs_e
 * @brief ADC CIC over sample rate selection
 */
typedef enum {
    ADC_OVFS_192FS = 0,
    ADC_OVFS_128FS,
    ADC_OVFS_96FS,
    ADC_OVFS_64FS
} a_adc_ovfs_e;

/**
 * enum a_adc_fir_e
 * @brief ADC frequency response (FIR) mode selection
 */
typedef enum {
    ADC_FIR_MODE_A = 0,
    ADC_FIR_MODE_B,
    ADC_FIR_MODE_C,
} a_adc_fir_e;

/**
 * enum a_adc_ch_e
 * @beief ADC channels selection
 */
typedef enum {
    ADC_CHANNEL_0 = 0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3
} a_adc_ch_e;

typedef enum {
    ADC_AMIC = 0, /* analog mic */
    ADC_DMIC /* digital mic */
} a_adc_ch_type_e;

/*
 * enum a_hpf_time_e
 * @brief HPF(High Pass Filter) auto setting time selection
 */
typedef enum {
    HPF_TIME_0 = 0, /* 1.3ms at 48kfs*/
    HPF_TIME_1, /* 5ms at 48kfs */
    HPF_TIME_2, /* 10ms  at 48kfs*/
    HPF_TIME_3 /* 20ms at 48kfs */
} a_hpf_time_e;

/*
 * enum a_input_lr_e
 * @brief ADC input left and right selection
 */
typedef enum {
    INPUT_POSITIVE_SEL = (1 << 0), /* INPUTxP selection */
    INPUT_NEGATIVE_SEL = (1 << 1) /* INPUTxN selection */
} a_input_lr_e;

typedef struct {
    int16_t gain;
    uint8_t fb_res;
    uint8_t input_res;
} adc_gain_input_t;

/* @brief  Get the ADC controller base address */
static inline struct acts_audio_adc *get_adc_reg_base(struct device *dev)
{
    const struct phy_adc_config_data *cfg = dev->config;
    return (struct acts_audio_adc *)cfg->reg_base;
}

/* @brief Dump the ADC relative registers */
static void adc_dump_register(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    LOG_INF("** adc contoller regster **");
    LOG_INF("          BASE: %08x", (uint32_t)adc_reg);
    LOG_INF("    ADC_DIGCTL: %08x", adc_reg->adc_digctl);
    LOG_INF("    CH0_DIGCTL: %08x", adc_reg->ch0_digctl);
    LOG_INF("    CH1_DIGCTL: %08x", adc_reg->ch1_digctl);
    LOG_INF("    CH2_DIGCTL: %08x", adc_reg->ch2_digctl);
    LOG_INF("    CH3_DIGCTL: %08x", adc_reg->ch3_digctl);
    LOG_INF("   ADC_FIFOCTL: %08x", adc_reg->fifoctl);
    LOG_INF("      ADC_STAT: %08x", adc_reg->stat);
    LOG_INF(" ADC_FIFO0_DAT: %08x", adc_reg->fifo0_dat);
    LOG_INF("       AAL0_CTL: %08x", adc_reg->aal0_ctl);
    LOG_INF("       AAL1_CTL: %08x", adc_reg->aal1_ctl);
    LOG_INF("       AAL2_CTL: %08x", adc_reg->aal2_ctl);
    LOG_INF("       AAL3_CTL: %08x", adc_reg->aal3_ctl);
    LOG_INF("      ADC_CTL0: %08x", adc_reg->adc_ctl0);
    LOG_INF("      ADC_CTL1: %08x", adc_reg->adc_ctl1);
    LOG_INF("      ADC_CTL2: %08x", adc_reg->adc_ctl2);
    LOG_INF("	   ADC_CTL3: %08x", adc_reg->adc_ctl3);
    LOG_INF("      ADC_BIAS: %08x", adc_reg->bias);
    LOG_INF("      VMIC_CTL: %08x", adc_reg->vmic_ctl);
    LOG_INF("   REF_LDO_CTL: %08x", adc_reg->ref_ldo_ctl);
    LOG_INF("HW_TRIGGER_CTL: %08x", adc_reg->hw_trigger_ctl);
    LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
    LOG_INF("    CMU_ADCCLK: %08x", sys_read32(CMU_ADCCLK));
}

/* @brief disable ADC FIFO by specified FIFO index */
static void __adc_fifo_disable(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (ADC_FIFO_0 == idx) {
        adc_reg->fifoctl &= ~(ADC_FIFOCTL_ADF0RT | ADC_FIFOCTL_ADF0FDE);
        /* disable ADC FIFO0 access clock */
        sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
    }
}

/* @brief check all ADC FIFOs are idle */
static inline bool __adc_check_fifo_all_disable(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (adc_reg->fifoctl & ADC_FIFOCTL_ADF0RT)
        return false;

    return true;
}

/* @brief check whether the ADC FIFO is working now */
static bool __is_adc_fifo_working(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (ADC_FIFO_0 == idx) {
        if (adc_reg->fifoctl & ADC_FIFOCTL_ADF0RT)
            return true;
    }

    return false;
}

/* @brief enable ADC FIFO by specified FIFO index */
static int __adc_fifo_enable(struct device *dev, audio_fifouse_sel_e sel,
                            audio_dma_width_e wd, uint8_t drq_level, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;

    if ((drq_level > ADC_FIFO_MAX_DRQ_LEVEL) || (drq_level == 0))
        drq_level = ADC_FIFO_DRQ_LEVEL_DEFAULT;

    if (FIFO_SEL_ASRC == sel) {
        LOG_ERR("invalid fifo sel %d", sel);
        return -EINVAL;
    }

    if (ADC_FIFO_0 == idx) {
        reg &= ~0x3FFF; /* clear FIFO0 fields */

        if (FIFO_SEL_CPU == sel) /* enable IRQ */
            reg |= (ADC_FIFOCTL_ADF0FIE);
        else if (FIFO_SEL_DMA == sel) /* enable DRQ */
            reg |= (ADC_FIFOCTL_ADF0FDE);

        reg |= ADC_FIFOCTL_ADF0OS(sel) | ADC_FIFOCTL_ADF0RT;
        if (DMA_WIDTH_16BITS == wd) /* width 0:32bits; 1:16bits */
            reg |= ADC_FIFOCTL_ADCFIFO0_DMAWIDTH;
        reg |= ADC_FIFOCTL_DRQ_LEVEL(drq_level);

        adc_reg->fifoctl = reg;

        /* enable ADC FIFO to access ADC CLOCK */
        sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
    }

    return 0;
}

/* @brief set the ADC FIFO DRQ level */
static int __adc_fifo_drq_level_set(struct device *dev, a_adc_fifo_e idx, uint8_t level)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;

    if ((level > ADC_FIFO_MAX_DRQ_LEVEL) || (level == 0))
        return -EINVAL;

    if (ADC_FIFO_0 == idx) {
        reg &= ~ADC_FIFOCTL_DRQ_LEVEL_MASK;
        reg |= ADC_FIFOCTL_DRQ_LEVEL(level);
    } else {
        return -EINVAL;
    }

    adc_reg->fifoctl = reg;

    return 0;
}

/* @brief get the ADC FIFO DRQ level */
static int __adc_fifo_drq_level_get(struct device *dev, a_adc_fifo_e idx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->fifoctl;
    int level;

    if (ADC_FIFO_0 == idx) {
        level = (reg & ADC_FIFOCTL_DRQ_LEVEL_MASK) >> ADC_FIFOCTL_DRQ_LEVEL_SHIFT;
    } else {
        level = -EINVAL;
    }

    return level;
}

/* @brief ADC digital CIC over sample rate and FIR mode setting */
static void __adc_digital_ovfs_fir_cfg(struct device *dev, uint8_t sr)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint32_t reg;

    a_adc_fir_e fir;

    /* Configure the Programmable frequency response curve according with the sample rate */
    if (sr <= SAMPLE_RATE_16KHZ) {
        fir = ADC_FIR_MODE_B;
    } else if (sr < SAMPLE_RATE_96KHZ) {
        fir = ADC_FIR_MODE_A;
    } else {
        fir = ADC_FIR_MODE_C;
    }

    reg = adc_reg->adc_digctl;
    reg &= ~(ADC_DIGCTL_ADC_OVFS_MASK | ADC_DIGCTL_ADC_FIR_MD_SEL_MASK);

    /* ADC works without ANC */
    if (sr >= SAMPLE_RATE_96KHZ)
        reg |= ADC_DIGCTL_ADC_OVFS(3); /* 64FS */
    else
        reg |= ADC_DIGCTL_ADC_OVFS(ADC_OSR_DEFAULT); /* 128FS */

    /* ADC works with ANC */
    if (data->anc_en) {
        if (sr == SAMPLE_RATE_16KHZ) {
            reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
            reg |= ADC_DIGCTL_ADC_OVFS(0); /* ANC sample rate 16K CIC over sample rate select 192fs */
        } else if (sr == SAMPLE_RATE_24KHZ) {
            reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
            reg |= ADC_DIGCTL_ADC_OVFS(1); /* ANC sample rate 24K CIC over sample rate select 128fs */
        } else if (sr == SAMPLE_RATE_32KHZ) {
            reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
            reg |= ADC_DIGCTL_ADC_OVFS(2); /* ANC sample rate 32K CIC over sample rate select 96fs */
        } else if (sr == SAMPLE_RATE_48KHZ) {
            reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
            reg |= ADC_DIGCTL_ADC_OVFS(3); /* ANC sample rate 48K CIC over sample rate select 64fs */
        }
    }

    reg |= ADC_DIGCTL_ADC_FIR_MD_SEL(fir);

    adc_reg->adc_digctl = reg;
}

/* @brief ADC channel digital configuration */
static void __adc_digital_channel_cfg(struct device *dev, a_adc_ch_e ch, a_adc_ch_type_e type, bool out_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint32_t reg0, reg1;
    uint32_t ch_digital = ADC_CH2REG(adc_reg, ch);

    reg0 = adc_reg->adc_digctl;
    reg1 = sys_read32(ch_digital);

    LOG_DBG("channel:%d type:%d enable:%d", ch, type, out_en);

    if (out_en) {
        /* enable FIR/CIC clock */
        sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCCICEN | CMU_ADCCLK_ADCFIREN, CMU_ADCCLK);

        if (ADC_AMIC == type) {
            /* enable ANA clock */
            sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCANAEN, CMU_ADCCLK);
            reg1 &= ~CH0_DIGCTL_MIC_SEL; /* enable ADC analog part */
        } else if (ADC_DMIC == type) {
            /* enable DMIC clock */
            sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCDMICEN, CMU_ADCCLK);
            reg1 |= CH0_DIGCTL_MIC_SEL; /* enable ADC  digital MIC part */
        }
        reg1 |= CH0_DIGCTL_DAT_OUT_EN; /* channel FIFO timing slot enable */
    } else {
        reg1 &= ~CH0_DIGCTL_DAT_OUT_EN;
        reg0 &= ~(1 << (ADC_DIGCTL_ADC_DIG_SHIFT + ch)); /* channel disable */
        adc_reg->adc_digctl = reg0;
        /* check all channels disable */
        if ((!(reg0 & ADC_DIGCTL_ADC_DIG_MASK)) && (!data->anc_en)) {
            sys_write32(sys_read32(CMU_ADCCLK) & ~(CMU_ADCCLK_ADCDMICEN
                | CMU_ADCCLK_ADCANAEN | CMU_ADCCLK_ADCCICEN
                | CMU_ADCCLK_ADCFIREN), CMU_ADCCLK);
        }
    }

    sys_write32(reg1, ch_digital);
}

/* @brief ADC channels enable at the same time */
static void __adc_digital_channels_en(struct device *dev, bool ch0_en, bool ch1_en, bool ch2_en, bool ch3_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint32_t reg = adc_reg->adc_digctl;
    uint32_t reg1 = adc_reg->hw_trigger_ctl;

    //reg &= ~ADC_DIGCTL_ADC_DIG_MASK;
    reg1 &= ~HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK;

    if (ch0_en) {
        reg |= ADC_DIGCTL_ADC0_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC0_EN;
    }

    if (ch1_en) {
        reg |= ADC_DIGCTL_ADC1_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC1_EN;
    }

    if (ch2_en) {
        reg |= ADC_DIGCTL_ADC2_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC2_EN;
    }

    if (ch3_en) {
        reg |= ADC_DIGCTL_ADC3_DIG_EN;
        reg1 |= HW_TRIGGER_ADC_CTL_INT_TO_ADC3_EN;
    }

    if (data->hw_trigger_en)
        adc_reg->hw_trigger_ctl = reg1;
    else
        adc_reg->adc_digctl = reg;
}
#if 0
/* @brief ADC HPF(High Pass Filter) audo-set configuration */
static void __adc_hpf_auto_set(struct device *dev, a_adc_ch_e ch, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);
    a_hpf_time_e time = HPF_TIME_2;

    if (!enable) {
        /* disable HPF auto-set function */
        sys_write32(sys_read32(ch_digctl) & ~CH0_DIGCTL_HPF_AS_EN, ch_digctl);
        return ;
    }

    if (ADC_CHANNEL_0 == ch)
        time = PHY_DEV_FEATURE(adc0_hpf_time);
    else if (ADC_CHANNEL_1 == ch)
        time = PHY_DEV_FEATURE(adc1_hpf_time);
    else if (ADC_CHANNEL_2 == ch)
        time = PHY_DEV_FEATURE(adc2_hpf_time);
    else if (ADC_CHANNEL_3 == ch)
        time = PHY_DEV_FEATURE(adc3_hpf_time);

    reg = sys_read32(ch_digctl) & ~CH0_DIGCTL_HPF_AS_TS_MASK;
    reg |= CH0_DIGCTL_HPF_AS_TS(time);
    reg |= CH0_DIGCTL_HPF_AS_EN; /* HPF auto-set enable */

    sys_write32(reg, ch_digctl);

    LOG_DBG("%d ch@%d HPF reg:0x%x", __LINE__, ch, sys_read32(ch_digctl));
}
#endif
/* @brief ADC HPF configuration for fast stable */
static void __adc_hpf_fast_stable(struct device *dev, uint16_t input_dev, uint8_t sample_rate)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint8_t i;
    uint32_t reg, ch_digctl = 0, en_flag = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return ;
    }
#endif

    if (adc_input_map.ch0_input != ADC_CH_DISABLE)
        en_flag |= BIT(0);

    if (adc_input_map.ch1_input != ADC_CH_DISABLE)
        en_flag |= BIT(1);

    if (adc_input_map.ch2_input != ADC_CH_DISABLE)
        en_flag |= BIT(2);

    if (adc_input_map.ch3_input != ADC_CH_DISABLE)
        en_flag |= BIT(3);

    for (i = 0; i < ADC_MAX_CHANNELS_NUMBER; i++) {
        if (en_flag & BIT(i)) {
            ch_digctl = ADC_CH2REG(adc_reg, i);
            reg = sys_read32(ch_digctl) & ~(0x7f << CH0_DIGCTL_HPF_N_SHIFT);
            reg |= CH0_DIGCTL_HPF_N(0x03);

            /* enable high frequency range and HPF function */
            reg |= (CH0_DIGCTL_HPF_S | CH0_DIGCTL_HPFEN );

            sys_write32(reg, ch_digctl);

        }
    }
}

#if 0
/* @brief ADC HPF(High Pass Filter) enable */
static void __adc_hpf_control(struct device *dev, a_adc_ch_e ch, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);
    bool is_high;
    uint8_t frequency;

    if (!enable) {
        sys_write32(sys_read32(ch_digctl) & ~CH0_DIGCTL_HPFEN, ch_digctl);
        return ;
    }

    if (ADC_CHANNEL_0 == ch) {
        is_high = PHY_DEV_FEATURE(adc0_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc0_frequency);
    } else if (ADC_CHANNEL_1 == ch) {
        is_high = PHY_DEV_FEATURE(adc1_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc1_frequency);
    } else if (ADC_CHANNEL_2 == ch){
        is_high = PHY_DEV_FEATURE(adc2_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc2_frequency);
    } else {
        is_high = PHY_DEV_FEATURE(adc3_hpf_fc_high);
        frequency = PHY_DEV_FEATURE(adc3_frequency);
    }

    /* clear HPF_S and HPF_N */
    reg = sys_read32(ch_digctl) & ~(0x7f << CH0_DIGCTL_HPF_N_SHIFT);
    reg |= CH0_DIGCTL_HPF_N(frequency);

    if (is_high)
        reg |= CH0_DIGCTL_HPF_S;

    /* enable HPF */
    reg |= CH0_DIGCTL_HPFEN;

    sys_write32(reg, ch_digctl);

    LOG_DBG("%d ch@%d HPF reg:0x%x", __LINE__, ch, sys_read32(ch_digctl));
}
#endif

/* @brief ADC channel digital gain setting */
static void __adc_digital_gain_set(struct device *dev, a_adc_ch_e ch, uint8_t gain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg, ch_digctl = ADC_CH2REG(adc_reg, ch);

    if (gain > ADC_DIGITAL_CH_GAIN_MAX)
        gain = ADC_DIGITAL_CH_GAIN_MAX;

    reg = sys_read32(ch_digctl) & ~CH0_DIGCTL_ADCGC_MASK;
    reg |= CH0_DIGCTL_ADCGC(gain);

    sys_write32(reg, ch_digctl);
}

#ifdef CONFIG_ADC_DMIC
/* @brief ADC digital dmic gain setting */
static void __adc_dmic_gain_set(struct device *dev, uint8_t gain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg;

    if (gain > ADC_DIGITAL_DMIC_GAIN_MAX)
        gain = ADC_DIGITAL_DMIC_GAIN_MAX;

    reg = adc_reg->adc_digctl & ~ADC_DIGCTL_DMIC_PRE_GAIN_MASK;
    reg |= ADC_DIGCTL_DMIC_PRE_GAIN(gain);

    /* by default DMIC01 latch sequence is L firstly and then R */
#ifdef CONFIG_ADC_DMIC_RL_SEQUENCE
    reg |= ADC_DIGCTL_DMIC01_CHS;
#endif

    /* by default DMIC23 latch sequence is L firstly and then R */
#ifdef CONFIG_ADC_DMIC23_RL_SEQUENCE
    reg |= ADC_DIGCTL_DMIC23_CHS;
#endif

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    if (data->external_config.DMIC01_Channel_Aligning)
        reg |= ADC_DIGCTL_DMIC01_CHS; /* DMIC01 latch sequency: R/L  */

    if (data->external_config.DMIC23_Channel_Aligning)
        reg |= ADC_DIGCTL_DMIC23_CHS; /* DMIC23 latch sequency: R/L  */
#endif

    adc_reg->adc_digctl = reg;
}
#endif

/* @brief ADC VMIC control initialization */
static void __adc_vmic_ctl_init(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    adc_reg->vmic_ctl |= (ADC_VMIC_CTL_ISO_AVCC_AU | ADC_VMIC_CTL_ISO_VD18);

    adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_VOL_MASK;
    adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC0_VOL(vmic_voltage_array[0]);

    adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_VOL_MASK;
    adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC1_VOL(vmic_voltage_array[1]);

    adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC2_VOL_MASK;
    adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC2_VOL(vmic_voltage_array[2]);

}

#ifdef CONFIG_CFG_DRV
static void __adc_vmic_ctl(struct device *dev, uint8_t adc2vmic_index, bool is_en)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    LOG_DBG("adc2vmic index:%d is_en:%d",adc2vmic_index, is_en);

    if (is_en) {
        if (adc2vmic_index == 0)
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC0_EN(3); /* enable VMIC0 OP */
        else if (adc2vmic_index == 1)
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC1_EN(3); /* enable VMIC1 OP */
        else
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC2_EN(3); /* enable VMIC2 OP */
    } else {
        if (adc2vmic_index == 0)
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_EN_MASK; /* disable VMIC0 OP */
        else if (adc2vmic_index == 1)
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_EN_MASK; /* disable VMIC1 OP */
        else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC2_EN_MASK; /* disable VMIC2 OP */
    }
}
#endif

/* @brief ADC VMIC control */
static void __adc_vmic_ctl_enable(struct device *dev, uint16_t input_dev)
{
    /* power-on MIC voltage */
#ifndef CONFIG_CFG_DRV
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if ((input_dev & AUDIO_DEV_TYPE_AMIC) || (input_dev & AUDIO_DEV_TYPE_DMIC))
#endif
    {
        /* vmic_ctl_array
          *   - 0: disable VMICx OP
          *   - 2: bypass VMICx OP
          *   - 3: enable VMICx OP
          */
#ifdef CONFIG_CFG_DRV
        struct phy_adc_drv_data *data = dev->data;
        if (data->external_config.Enable_VMIC & ADC_0)
#else
        if (vmic_ctl_array[0] <= 3)
#endif
        {
#ifdef CONFIG_CFG_DRV
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], true);
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_EN_MASK;
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC0_EN(vmic_ctl_array[0]);
#endif
        }

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Enable_VMIC & ADC_1)
#else
        if (vmic_ctl_array[1] <= 3)
#endif
        {
#ifdef CONFIG_CFG_DRV
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], true);
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_EN_MASK;
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC1_EN(vmic_ctl_array[1]);
#endif
        }

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Enable_VMIC & ADC_2)
#else
        if (vmic_ctl_array[2] <= 3)
#endif
        {
#ifdef CONFIG_CFG_DRV
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[2], true);
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC2_EN_MASK;
            adc_reg->vmic_ctl |= ADC_VMIC_CTL_VMIC2_EN(vmic_ctl_array[2]);
#endif
        }
		k_sleep(K_MSEC(1));
    }

}

/* @brief ADC VMIC control */
static void __adc_vmic_ctl_disable(struct device *dev, uint16_t input_dev)
{
    /* power-off MIC voltage */
#ifndef CONFIG_CFG_DRV
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if ((input_dev & AUDIO_DEV_TYPE_AMIC) || (input_dev & AUDIO_DEV_TYPE_DMIC))
#endif
    {
#ifdef CONFIG_CFG_DRV
        struct phy_adc_drv_data *data = dev->data;
        if (data->external_config.Enable_VMIC & ADC_0)
#else
        if (vmic_ctl_array[0] <= 3)
#endif
        {
#ifdef CONFIG_CFG_DRV
            if (!(data->anc_en && data->external_config.Enable_ANC == ANC_FF)) {
                __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], false);
            }
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC0_EN_MASK; /* disable VMIC0 OP */
#endif
        }

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Enable_VMIC & ADC_1)
#else
        if (vmic_ctl_array[1] <= 3)
#endif
        {
#ifdef CONFIG_CFG_DRV
        if (!(data->anc_en && data->external_config.Enable_ANC == ANC_FB)) {
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], false);
        }
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC1_EN_MASK; /* disable VMIC1 OP */
#endif
        }

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Enable_VMIC & ADC_2)
#else
        if (vmic_ctl_array[2])
#endif
        {
#ifdef CONFIG_CFG_DRV
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[2], false);
#else
            adc_reg->vmic_ctl &= ~ADC_VMIC_CTL_VMIC2_EN_MASK; /* disable VMIC2 OP */
#endif
        }
		k_sleep(K_MSEC(1));
    }
}

/* @brief ADC input analog gain setting */
static int __adc_input_gain_set(struct device *dev, a_adc_ch_e ch, uint8_t input_res,
                                    uint8_t feedback_res, bool is_diff, bool update_fb)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg, adc_ctl = ADC_CTL2REG(adc_reg, ch);

    /* Set FDBUFx when in single end mode */
    reg = sys_read32(adc_ctl);

    if (!is_diff) {
        reg &= ~ADC0_CTL_FDBUF0_IRS_MASK;
        reg |= ADC0_CTL_FDBUF0_IRS(input_res);
        reg |= 0x4 << ADC0_CTL_FDBUF0_IRS_SHIFT;
    }

    reg &= ~ADC0_CTL_INPUT0_IRS_MASK;
    reg |= ADC0_CTL_INPUT0_IRS(input_res);

    if (update_fb) {
        reg &= ~ADC0_CTL_PREAM0_PG_MASK;
        reg |= ADC0_CTL_PREAM0_PG(feedback_res);
    }

    sys_write32(reg, adc_ctl);
    return 0;
}

/* @brief ADC channel0 analog control */
static int __adc_ch0_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc_ctl0;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx != ADC_CH_INPUT0P) && (inputx != ADC_CH_INPUT0NP_DIFF)) {
        LOG_ERR("invalid input:0x%x for ADC0", inputx);
        return -EINVAL;
    }

    reg &= ~ADC0_CTL_INPUT0_IRS_MASK; /* clear input resistor */
    reg &= ~ADC0_CTL_INPUT0_IN_MODE;
    reg &= ~(0xf << ADC0_CTL_INPUT0P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    /* ADC0 channel sdm / PREOP 0 / FD BUF OP 0 diable*/
    reg &= ~(ADC0_CTL_ADC0_EN | ADC0_CTL_PREOP0_EN | ADC0_CTL_FDBUF0_EN);
    reg &= ~ADC0_CTL_PREAM0_PG_MASK;
    reg &= ~ADC0_CTL_FDBUF0_IRS_MASK;

    reg |= ADC0_CTL_VRDA0_EN; /* VRDA0 enable */
    reg |= ADC0_CTL_FDBUF0_EN; /* FD BUF OP 0 enable for SE mode */
    reg |= ADC0_CTL_PREOP0_EN; /* PREOP 0 enable */
    reg |= ADC0_CTL_ADC0_EN; /* ADC0 channel enable */

    /* by default to enable single end input mode: INPUT0P=>ADC0, INPUT0N=>ADC2 */
    reg |= ADC0_CTL_INPUT0_IN_MODE;

    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx) {
        reg &= ~ADC0_CTL_INPUT0_IN_MODE; /* enable differential input mode */
        reg &= ~ADC0_CTL_FDBUF0_EN;	/* when sel diff mode, disable buf */
        reg &= ~ADC0_CTL_FDBUF0_IRS_MASK; /* when select diff mode to disable connect buf */
    }

    /* enable INPUT0N pad to ADC0/2 channel */
    if (inputx & ADC_CH_INPUT0N)
        reg |= ADC0_CTL_INPUT0N_EN(3);

    /* enable INPUT0P pad to ADC0 channel */
    if (inputx & ADC_CH_INPUT0P)
        reg |= ADC0_CTL_INPUT0P_EN(3);

    /*input cap to ADC0 channel fast charge enable*/
    reg |= ADC0_CTL_ADC0_CAPFC_EN;
    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx)
        reg |= ADC0_CTL_FDBUF0_IRS(0x0);
    else
        reg |= ADC0_CTL_FDBUF0_IRS(0x7);
    reg |= ADC0_CTL_INPUT0_IRS(0x3);

    adc_reg->adc_ctl0 = reg;

    k_sleep(K_MSEC(50));
    /*disable fast charge*/
    reg &= ~ADC0_CTL_ADC0_CAPFC_EN;
    reg &= ~ADC0_CTL_FDBUF0_IRS_MASK;
    reg &= ~ADC0_CTL_INPUT0_IRS_MASK;
    adc_reg->adc_ctl0 = reg;
    return 0;
}

/* @brief ADC channel1 analog control */
static int __adc_ch1_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc_ctl1;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx != ADC_CH_INPUT1P) && (inputx != ADC_CH_INPUT1NP_DIFF)) {
        LOG_ERR("invalid input:0x%x for ADC1", inputx);
        return -EINVAL;
    }

    reg &= ~ADC1_CTL_INPUT1_IRS_MASK; /* clear input resistor */
    reg &= ~ADC1_CTL_INPUT1_IN_MODE;
    reg &= ~(0xf << ADC1_CTL_INPUT1P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    /* ADC1 channel sdm / PREOP 1 / FD BUF OP 1 diable*/
    reg &= ~(ADC1_CTL_ADC1_EN | ADC1_CTL_PREOP1_EN | ADC1_CTL_FDBUF1_EN);
    reg &= ~ADC1_CTL_PREAM1_PG_MASK;
    reg &= ~ADC1_CTL_FDBUF1_IRS_MASK;

    reg |= ADC1_CTL_VRDA1_EN; /* VRDA1 enable */
    reg |= ADC1_CTL_FDBUF1_EN; /* FD BUF OP 1 enable for SE mode */
    reg |= ADC1_CTL_PREOP1_EN; /* PREOP 1 enable */
    reg |= ADC1_CTL_ADC1_EN; /* ADC1 channel enable */

    /* by default to enable single end input mode: INPUT1P=>ADC1, INPUT1N=>ADC3 */
    reg |= ADC1_CTL_INPUT1_IN_MODE;

    /* input differential mode */
    if (ADC_CH_INPUT1NP_DIFF == inputx) {
        reg &= ~ADC1_CTL_INPUT1_IN_MODE; /* enable differential input mode */
        reg &= ~ADC1_CTL_FDBUF1_EN;	/* when sel diff mode, disable buf */
        reg &= ~ADC1_CTL_FDBUF1_IRS_MASK; /* when select diff mode to disable connect buf */
    }

    /* enable INPUT1N pad to ADC1/3 channel */
    if (inputx & ADC_CH_INPUT1N)
        reg |= ADC1_CTL_INPUT1N_EN(3);

    /* enable INPUT1P pad to ADC1 channel */
    if (inputx & ADC_CH_INPUT1P)
        reg |= ADC1_CTL_INPUT1P_EN(3);

    /*input cap to ADC1 channel fast charge enable*/
    reg |= ADC1_CTL_ADC1_CAPFC_EN;
    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx)
        reg |= ADC1_CTL_FDBUF1_IRS(0x0);
    else
        reg |= ADC1_CTL_FDBUF1_IRS(0x7);
    reg |= ADC1_CTL_INPUT1_IRS(0x3);

    adc_reg->adc_ctl1 = reg;

    /*disable fast charge*/
    k_sleep(K_MSEC(50));
    reg &= ~ADC1_CTL_ADC1_CAPFC_EN;
    reg &= ~ADC1_CTL_FDBUF1_IRS_MASK;
    reg &= ~ADC1_CTL_INPUT1_IRS_MASK;
    adc_reg->adc_ctl1 = reg;

    return 0;
}

/* @brief ADC channel2 analog control */
static int __adc_ch2_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc_ctl2;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx & ADC_CH_INPUT2N) && !(inputx & ADC_CH_INPUT2P)) {
        LOG_ERR("ADC2 does not support INPUT2N in SE mode");
        return -EINVAL;
    }

    if ((inputx & ADC_CH_INPUT1N) || (inputx & ADC_CH_INPUT3P)) {
        LOG_ERR("Invalid input:0x%x for ADC2", inputx);
        return -EINVAL;
    }

    reg &= ~ADC2_CTL_INPUT2_IRS_MASK; /* clear input resistor */
    reg &= ~ADC2_CTL_INPUT2_IN_MODE;
    reg &= ~(0xf << ADC2_CTL_INPUT2P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    /* ADC2 channel sdm / PREOP 2 / FD BUF OP 2 diable*/
    reg &= ~(ADC2_CTL_ADC2_EN | ADC2_CTL_PREOP2_EN | ADC2_CTL_FDBUF2_EN);
    reg &= ~ADC2_CTL_PREAM2_PG_MASK;
    reg &= ~ADC2_CTL_FDBUF2_IRS_MASK;

    reg |= ADC2_CTL_VRDA2_EN; /* VRDA2 enable */
    reg |= ADC2_CTL_FDBUF2_EN;	/* FD BUF OP 2 enable for SE mode */
    reg |= ADC2_CTL_PREOP2_EN; /* PREOP 2 enable */
    reg |= ADC2_CTL_ADC2_EN; /* ADC2 channel enable */

    /* by default to enable single end input mode: INPUT2P=>ADC2, INPUT2N=>ADC3 */
    reg |= ADC2_CTL_INPUT2_IN_MODE;

    /* input differential mode */
    if (ADC_CH_INPUT2NP_DIFF == inputx) {
        reg &= ~ADC2_CTL_INPUT2_IN_MODE; /* enable differential input mode */
        reg &= ~ADC2_CTL_FDBUF2_EN;	/* when sel diff mode, disable buf */
        reg &= ~ADC2_CTL_FDBUF2_IRS_MASK; /* when select diff mode to disable connect buf */
    }

    if (inputx & ADC_CH_INPUT2N)
        reg |= ADC2_CTL_INPUT2N_EN(3);

    if (inputx & ADC_CH_INPUT2P)
        reg |= ADC2_CTL_INPUT2P_EN(3);

    /*input cap to ADC2 channel fast charge enable*/
    reg |= ADC2_CTL_ADC2_CAPFC_EN;
    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx)
        reg |= ADC2_CTL_FDBUF2_IRS(0x0);
    else
        reg |= ADC2_CTL_FDBUF2_IRS(0x7);
    reg |= ADC2_CTL_INPUT2_IRS(0x3);

    adc_reg->adc_ctl2 = reg;

    /*disable fast charge*/
    k_sleep(K_MSEC(50));
    reg &= ~ADC2_CTL_ADC2_CAPFC_EN;
    reg &= ~ADC2_CTL_FDBUF2_IRS_MASK;
    reg &= ~ADC2_CTL_INPUT2_IRS_MASK;
    adc_reg->adc_ctl2 = reg;

    /* in case of the INPUT0N pad to ADC2 */
    if (inputx & ADC_CH_INPUT0N) {
        adc_reg->adc_ctl0 |= ADC0_CTL_INPUT0N_EN(3); /* enable INPUT0N to ADC0 channel */
        adc_reg->adc_ctl0 |= ADC0_CTL_INPUT0_IN_MODE; /* enable ADC0 single end mode */
    }

    /* in case of the INPUT3N pad to ADC2 */
    if (inputx & ADC_CH_INPUT3N) {

        adc_reg->adc_ctl3 |= ADC3_CTL_INPUT3N_EN(3); /* enable INPUT3N to ADC3 channel */
        adc_reg->adc_ctl3 |= ADC3_CTL_INPUT3_IN_MODE; /* enable ADC3 single end mode */
    }

    return 0;
}

/* @brief ADC channel3 analog control */
static int __adc_ch3_analog_control(struct device *dev, uint8_t inputx)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint32_t reg = adc_reg->adc_ctl3;

    /* ADC channel disable or DMIC does not need to set ADCx_ctl */
    if ((inputx == ADC_CH_DISABLE) || (inputx == ADC_CH_DMIC))
        return 0;

    if ((inputx & ADC_CH_INPUT3N) && !(inputx & ADC_CH_INPUT3P)) {
        LOG_ERR("ADC2 does not support INPUT3N in SE mode");
        return -EINVAL;
    }

    if ((inputx & ADC_CH_INPUT0N) || (inputx & ADC_CH_INPUT2P)) {
        LOG_ERR("Invalid input:0x%x for ADC3", inputx);
        return -EINVAL;
    }

    reg &= ~ADC3_CTL_INPUT3_IRS_MASK; /* clear input resistor */
    reg &= ~ADC3_CTL_INPUT3_IN_MODE;
    reg &= ~(0xf << ADC3_CTL_INPUT3P_EN_SHIFT); /* clear INPUTN/P pad to ADC channel input enable */
    /* ADC3 channel sdm / PREOP 3 / FD BUF OP 3 diable*/
    reg &= ~(ADC3_CTL_ADC3_EN | ADC3_CTL_PREOP3_EN | ADC3_CTL_FDBUF3_EN);
    reg &= ~ADC3_CTL_PREAM3_PG_MASK;
    reg &= ~ADC3_CTL_FDBUF3_IRS_MASK;

    reg |= ADC3_CTL_VRDA3_EN; /* VRDA3 enable */
    reg |= ADC3_CTL_FDBUF3_EN; /* FD BUF OP 3 enable for SE mode */
    reg |= ADC3_CTL_PREOP3_EN; /* PREOP 3 enable */
    reg |= ADC3_CTL_ADC3_EN; /* ADC3 channel enable */

    /* by default to enable single end input mode: INPUT3P=>ADC3, INPUT3N=>ADC2 */
    reg |= ADC3_CTL_INPUT3_IN_MODE;

    /* input differential mode */
    if(ADC_CH_INPUT3NP_DIFF == inputx) {
        reg &= ~ADC3_CTL_INPUT3_IN_MODE; /* enable differential input mode */
        reg &= ~ADC3_CTL_FDBUF3_EN; /* when sel diff mode, disable buf */
        reg &= ~ADC3_CTL_FDBUF3_IRS_MASK; /* when select diff mode to disable connect buf */
    }

    if (inputx & ADC_CH_INPUT3N)
        reg |= ADC3_CTL_INPUT3N_EN(3);

    if (inputx & ADC_CH_INPUT3P)
        reg |= ADC3_CTL_INPUT3P_EN(3);

    /*input cap to ADC3 channel fast charge enable*/
    reg |= ADC3_CTL_ADC3_CAPFC_EN;
    /* input differential mode */
    if (ADC_CH_INPUT0NP_DIFF == inputx)
        reg |= ADC3_CTL_FDBUF3_IRS(0x0);
    else
        reg |= ADC3_CTL_FDBUF3_IRS(0x7);
    reg |= ADC3_CTL_INPUT3_IRS(0x3);

    adc_reg->adc_ctl3 = reg;

    /*disable fast charge*/
    k_sleep(K_MSEC(50));
    reg &= ~ADC3_CTL_ADC3_CAPFC_EN;
    reg &= ~ADC3_CTL_FDBUF3_IRS_MASK;
    reg &= ~ADC3_CTL_INPUT3_IRS_MASK;
    adc_reg->adc_ctl3 = reg;

    /* in case of the INPUT1N pad to ADC3 */
    if (ADC_CH_INPUT1N == inputx) {
        adc_reg->adc_ctl1 |= ADC1_CTL_INPUT1N_EN(3); /* enable INPUT1N to ADC0 channel */
        adc_reg->adc_ctl1 |= ADC1_CTL_INPUT1_IN_MODE; /* enable ADC1 single end mode */
    }

    /* in case of the INPUT2N pad to ADC3 */
    if(ADC_CH_INPUT2N == inputx) {
        adc_reg->adc_ctl2 |= ADC2_CTL_INPUT2N_EN(3); /* enable INPUT1N to ADC0 channel */
        adc_reg->adc_ctl2 |= ADC2_CTL_INPUT2_IN_MODE; /* enable ADC1 single end mode */
    }

    return 0;
}

#ifdef ADC_DIGITAL_DEBUG_IN_ENABLE
static void __adc_digital_debug_in(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    /* set ADC clock divisor to '1' */
    sys_write32(sys_read32(CMU_ADCCLK) & (~0x7), CMU_ADCCLK);

    /* switch ADC debug clock to external PAD */
    sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCDEBUGEN, CMU_ADCCLK);

    /* ADC Digital debug enable */
    adc_reg->adc_digctl |= ADC_DIGCTL_ADDEN;
}
#endif

#ifdef ADC_ANALOG_DEBUG_OUT_ENABLE
static void __adc_analog_debug_out(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DBGSE_ADC << DEBUGSEL_DBGSE_SHIFT;
    sys_write32(reg, DEBUGSEL);

    sys_write32(sys_read32(DEBUGIE0) & (~0x107fe000), DEBUGIE0);
    sys_write32(sys_read32(DEBUGOE0) | 0x107fe000, DEBUGOE0);

    adc_reg->adc_digctl |= ADC_DIGCTL_AADEN;
}
#endif

/* @brief set the external trigger source for DAC digital start */
static int __adc_external_trigger_enable(struct device *dev, uint8_t trigger_src)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;

    if (trigger_src > 6) {
        LOG_ERR("Invalid ADC trigger source %d", trigger_src);
        return -EINVAL;
    }

    adc_reg->hw_trigger_ctl &= ~ HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL_MASK;
    adc_reg->hw_trigger_ctl |= HW_TRIGGER_ADC_CTL_TRIGGER_SRC_SEL(trigger_src);

    data->hw_trigger_en = 1;

    return 0;
}

/* @brief disable the external irq signal to start ADC digital function */
static void __adc_external_trigger_disable(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;

    if (adc_reg->hw_trigger_ctl & HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK)
        adc_reg->hw_trigger_ctl &= ~HW_TRIGGER_ADC_CTL_INT_TO_ADC_MASK;

    data->hw_trigger_en = 0;
}

/* @brief ADC fast capacitor charge function */
static void adc_fast_cap_charge(struct device *dev, uint16_t input_dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

    if (!PHY_DEV_FEATURE(fast_cap_charge))
        return ;

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return ;
    }
#endif

    /* set input resistor to use 3k */
    if (ADC_CH_DISABLE != adc_input_map.ch0_input) {
        /* in single-end mode */
        if ((adc_input_map.ch0_input & ADC_CH_INPUT0NP_DIFF) != ADC_CH_INPUT0NP_DIFF) {
            adc_reg->adc_ctl0 &= ~ADC0_CTL_FDBUF0_IRS_MASK;
            adc_reg->adc_ctl0 |= ADC0_CTL_FDBUF0_IRS(0x7);
        }
        adc_reg->adc_ctl0 &= ~ADC0_CTL_INPUT0_IRS_MASK;
        adc_reg->adc_ctl0 |= ADC0_CTL_INPUT0_IRS(0x3);
        /*Enable fast charge function of input cap to ADC L/R channel 0 */
        adc_reg->adc_ctl0 |= ADC0_CTL_ADC0_CAPFC_EN;
    }

    if (ADC_CH_DISABLE != adc_input_map.ch1_input) {
        /* in single-end mode */
        if ((adc_input_map.ch1_input & ADC_CH_INPUT1NP_DIFF) != ADC_CH_INPUT1NP_DIFF) {
            adc_reg->adc_ctl1 &= ~ADC1_CTL_FDBUF1_IRS_MASK;
            adc_reg->adc_ctl1 |= ADC1_CTL_FDBUF1_IRS(0x7);
        }
        adc_reg->adc_ctl1 &= ~ADC1_CTL_INPUT1_IRS_MASK;
        adc_reg->adc_ctl1 |= ADC1_CTL_INPUT1_IRS(0x3);
        /*Enable fast charge function of input cap to ADC L/R channel 1 */
        adc_reg->adc_ctl1 |= ADC1_CTL_ADC1_CAPFC_EN;
    }

    if (ADC_CH_DISABLE != adc_input_map.ch2_input) {
        /* in single-end mode */
        if ((adc_input_map.ch2_input & ADC_CH_INPUT2NP_DIFF) != ADC_CH_INPUT2NP_DIFF) {
            adc_reg->adc_ctl2 &= ~ADC2_CTL_FDBUF2_IRS_MASK;
            adc_reg->adc_ctl2 |= ADC2_CTL_FDBUF2_IRS(0x7);
        }
        adc_reg->adc_ctl2 &= ~ADC2_CTL_INPUT2_IRS_MASK;
        adc_reg->adc_ctl2 |= ADC2_CTL_INPUT2_IRS(0x3);
        /*Enable fast charge function of input cap to ADC L/R channel 2 */
        adc_reg->adc_ctl2 |= ADC2_CTL_ADC2_CAPFC_EN;
    }

    if (ADC_CH_DISABLE != adc_input_map.ch3_input) {
        /* in single-end mode */
        if ((adc_input_map.ch3_input & ADC_CH_INPUT3NP_DIFF) != ADC_CH_INPUT3NP_DIFF) {
            adc_reg->adc_ctl3 &= ~ADC3_CTL_FDBUF3_IRS_MASK;
            adc_reg->adc_ctl3 |= ADC3_CTL_FDBUF3_IRS(0x7);
        }
        adc_reg->adc_ctl3 &= ~ADC3_CTL_INPUT3_IRS_MASK;
        adc_reg->adc_ctl3 |= ADC3_CTL_INPUT3_IRS(0x3);
        /*Enable fast charge function of input cap to ADC L/R channel 3 */
        adc_reg->adc_ctl3 |= ADC3_CTL_ADC3_CAPFC_EN;
    }

    /* 3. wait for charging */
    k_sleep(K_MSEC(ADC_FAST_CAP_CHARGE_TIME_MS));

    /* disable fast charge of input CAP to ADC channels */
    if (ADC_CH_DISABLE != adc_input_map.ch0_input)
        adc_reg->adc_ctl0 &= ~ADC0_CTL_ADC0_CAPFC_EN;

    if (ADC_CH_DISABLE != adc_input_map.ch1_input)
        adc_reg->adc_ctl1 &= ~ADC1_CTL_ADC1_CAPFC_EN;

    if (ADC_CH_DISABLE != adc_input_map.ch2_input)
        adc_reg->adc_ctl2 &= ~ADC2_CTL_ADC2_CAPFC_EN;

    if (ADC_CH_DISABLE != adc_input_map.ch3_input)
        adc_reg->adc_ctl3 &= ~ADC3_CTL_ADC3_CAPFC_EN;
}

/* @brief ADC input channel analog configuration */
static int adc_input_config(struct device *dev, uint16_t input_dev)
{
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
    int ret;

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map))
        return -ENOENT;
#endif

    LOG_INF("ADC channel {dev:0x%x, [0x%x, 0x%x, 0x%x, 0x%x]}",
            input_dev, adc_input_map.ch0_input, adc_input_map.ch1_input,
            adc_input_map.ch2_input, adc_input_map.ch3_input);

    // adc_fast_cap_charge(dev, input_dev);

    ret = __adc_ch0_analog_control(dev, adc_input_map.ch0_input);
    if (ret)
        return ret;

    ret = __adc_ch1_analog_control(dev, adc_input_map.ch1_input);
    if (ret)
        return ret;

    ret = __adc_ch2_analog_control(dev, adc_input_map.ch2_input);
    if (ret)
        return ret;

    ret = __adc_ch3_analog_control(dev, adc_input_map.ch3_input);
    if (ret)
        return ret;

    return 0;
}

/* @brief ADC HPF (High Pass Filter) configuration */
static void adc_hpf_config(struct device *dev, a_adc_ch_e ch, bool enable)
{
    //__adc_hpf_auto_set(dev, ch, enable);
    // __adc_hpf_control(dev, ch, enable);
    return;
}

/* @brief ADC channels enable according to the input audio device */
static int adc_channels_enable(struct device *dev, uint16_t input_dev)
{
    uint8_t ch0_input = 0, ch1_input = 0, ch2_input = 0, ch3_input = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return -ENOENT;
    }
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;
    ch2_input = adc_input_map.ch2_input;
    ch3_input = adc_input_map.ch3_input;

    ADC_CHANNELS_CTL(true);

    return 0;
}

/* @brief ADC channels disable according to the input audio device */
static int adc_channels_disable(struct device *dev, uint16_t input_dev)
{
    uint8_t ch0_input = 0, ch1_input = 0, ch2_input = 0, ch3_input = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map)) {
        LOG_ERR("invalid input device:0x%x", input_dev);
        return -ENOENT;
    }
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;
    ch2_input = adc_input_map.ch2_input;
    ch3_input = adc_input_map.ch3_input;

    ADC_CHANNELS_CTL(false);

    return 0;
}

/* @brief  Enable the ADC digital function */
static int adc_digital_enable(struct device *dev, uint16_t input_dev, uint8_t sample_rate)
{
	int ret;

    /* configure OVFS and FIR */
    __adc_digital_ovfs_fir_cfg(dev, sample_rate);
    adc_fast_cap_charge(dev, input_dev);
    /* set HPF high frequency range for fast stable */
    __adc_hpf_fast_stable(dev, input_dev, sample_rate);

    ret = adc_channels_enable(dev, input_dev);
	return ret;
}

/* @brief  Disable the ADC digital function */
static void adc_digital_disable(struct device *dev, uint16_t input_dev)
{
    adc_channels_disable(dev, input_dev);
}

/* @brief Translate the AMIC/AUX gain from dB fromat to hardware register value */
static int adc_aux_amic_gain_translate(int16_t gain, uint8_t *input_res, uint8_t *fd_res, uint8_t *dig_gain)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(amic_aux_gain_mapping); i++) {
        if (gain <= amic_aux_gain_mapping[i].gain) {
            *input_res = amic_aux_gain_mapping[i].input_res;
            *fd_res = amic_aux_gain_mapping[i].feedback_res;
            *dig_gain = amic_aux_gain_mapping[i].digital_gain;
            LOG_INF("gain:%d map [%d %d %d]",
                gain, *input_res, *fd_res, *dig_gain);
            break;
        }
    }

    if (i == ARRAY_SIZE(amic_aux_gain_mapping)) {
        LOG_ERR("can not find out gain map %d", gain);
        return -ENOENT;
    }

    return 0;
}

#ifdef CONFIG_ADC_DMIC
/* @brief Translate the DMIC gain from dB fromat to hardware register value */
static int adc_dmic_gain_translate(int16_t gain, uint8_t *dmic_gain, uint8_t *dig_gain)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(dmic_gain_mapping); i++) {
        if (gain <= dmic_gain_mapping[i].gain) {
            *dmic_gain = dmic_gain_mapping[i].dmic_pre_gain;
            *dig_gain = dmic_gain_mapping[i].digital_gain;
            LOG_DBG("gain:%d map [%d %d]",
                gain, *dmic_gain, *dig_gain);
            break;
        }
    }

    if (i == ARRAY_SIZE(dmic_gain_mapping)) {
        LOG_ERR("can not find out gain map %d", gain);
        return -ENOENT;
    }

    return 0;
}
#endif

/* @brief ADC config channel0 gain */
static int adc_ch0_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    uint8_t dig_gain, input_res, fd_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &input_res, &fd_res, &dig_gain)) {
            LOG_ERR("failed to translate amic_aux ch0 gain %d", ch_gain);
        } else {
            __adc_digital_gain_set(dev, ADC_CHANNEL_0, dig_gain);
            if ((ADC_CH_INPUT0NP_DIFF & ch_input) == ADC_CH_INPUT0NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_0, input_res, fd_res, true, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_0, input_res, fd_res, false, true);
        }
    }

    return 0;
}

/* @brief ADC config channel1 gain */
static int adc_ch1_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    uint8_t dig_gain, input_res, fd_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &input_res, &fd_res, &dig_gain)) {
            LOG_ERR("failed to translate amic_aux ch1 gain %d", ch_gain);
        } else {
            __adc_digital_gain_set(dev, ADC_CHANNEL_1, dig_gain);
            if ((ADC_CH_INPUT1NP_DIFF & ch_input) == ADC_CH_INPUT1NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_1, input_res, fd_res, true, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_1, input_res, fd_res, false, true);
        }
    }

    return 0;
}

/* @brief ADC config channel2 gain */
static int adc_ch2_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint8_t dig_gain, input_res, fd_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &input_res, &fd_res, &dig_gain)) {
            LOG_ERR("failed to translate amic_aux ch2 gain %d", ch_gain);
        } else {
            __adc_digital_gain_set(dev, ADC_CHANNEL_2, dig_gain);
            if ((ADC_CH_INPUT2NP_DIFF & ch_input) == ADC_CH_INPUT2NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_2, input_res, fd_res, true, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_2, input_res, fd_res, false, true);

            /* In case of the INPUT0N pad to ADC2 */
            if (ADC_CH_INPUT0N & ch_input) {
                /* check if INPUT0P is disabled or not */
                if (!(adc_reg->adc_ctl0 & ADC0_CTL_INPUT0P_EN_MASK)) {
                    /* if INPUT0P is disabled, update the ADC0 input resistor */
                    __adc_input_gain_set(dev, ADC_CHANNEL_0, input_res, fd_res, false, false);
                }
            }

            /* In case of the INPUT0N pad to ADC2 */
            if (ADC_CH_INPUT3N & ch_input) {
                /* check if INPUT3P is disabled or not */
                if (!(adc_reg->adc_ctl3 & ADC3_CTL_INPUT3P_EN_MASK)) {
                    /* if INPUT3P is disabled, update the ADC3 input resistor */
                    __adc_input_gain_set(dev, ADC_CHANNEL_3, input_res, fd_res, false, false);
                }
            }
        }
    }

    return 0;
}

/* @brief ADC config channel3 gain */
static int adc_ch3_gain_config(struct device *dev, uint8_t ch_input, int16_t ch_gain)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    uint8_t dig_gain, input_res, fd_res;

    if ((ADC_CH_DISABLE != ch_input) && (ADC_CH_DMIC != ch_input)) {
        if (adc_aux_amic_gain_translate(ch_gain, &input_res, &fd_res, &dig_gain)) {
            LOG_ERR("failed to translate amic_aux ch3 gain %d", ch_gain);
        } else {
            __adc_digital_gain_set(dev, ADC_CHANNEL_3, dig_gain);
            if ((ADC_CH_INPUT3NP_DIFF & ch_input) == ADC_CH_INPUT3NP_DIFF)
                __adc_input_gain_set(dev, ADC_CHANNEL_3, input_res, fd_res, true, true);
            else
                __adc_input_gain_set(dev, ADC_CHANNEL_3, input_res, fd_res, false, true);

            /* In case of the INPUT1N pad to ADC3 */
            if (ADC_CH_INPUT1N & ch_input) {
                /* check if INPUT1P is disabled or not */
                if (!(adc_reg->adc_ctl1 & ADC1_CTL_INPUT1P_EN_MASK)) {
                    /* if INPUT1P is disabled, update the ADC1 input resistor */
                    __adc_input_gain_set(dev, ADC_CHANNEL_1, input_res, fd_res, false, false);
                }
            }

            /* In case of the INPUT2N pad to ADC3 */
            if (ADC_CH_INPUT2N & ch_input) {
                /* check if INPUT2P is disabled or not */
                if (!(adc_reg->adc_ctl2 & ADC2_CTL_INPUT2P_EN_MASK)) {
                    /* if INPUT2P is disabled, update the ADC2 input resistor */
                    __adc_input_gain_set(dev, ADC_CHANNEL_2, input_res, fd_res, false, false);
                }
            }
        }
    }

    return 0;
}

/* @brief ADC gain configuration */
static int adc_gain_config(struct device *dev, uint16_t input_dev, adc_gain *gain)
{
    uint8_t ch0_input, ch1_input, ch2_input, ch3_input;
    int16_t ch0_gain = 0, ch1_gain = 0, ch2_gain = 0, ch3_gain = 0;
    audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_input_map.ch0_input = data->input_ch0;
    adc_input_map.ch1_input = data->input_ch1;
    adc_input_map.ch2_input = data->input_ch2;
    adc_input_map.ch3_input = data->input_ch3;
#else
    if (board_audio_device_mapping(&adc_input_map))
        return -ENOENT;
#endif

    ch0_input = adc_input_map.ch0_input;
    ch1_input = adc_input_map.ch1_input;
    ch2_input = adc_input_map.ch2_input;
    ch3_input = adc_input_map.ch3_input;

    /* gain set when input channel enabled */
    if (ADC_CH_DISABLE != ch0_input)
        ch0_gain = gain->ch_gain[0];

    if (ADC_CH_DISABLE != ch1_input)
        ch1_gain = gain->ch_gain[1];

    if (ADC_CH_DISABLE != ch2_input)
        ch2_gain = gain->ch_gain[2];

    if (ADC_CH_DISABLE != ch3_input)
        ch3_gain = gain->ch_gain[3];

    LOG_INF("ADC channel gain {%d, %d, %d, %d}",
            ch0_gain, ch1_gain, ch2_gain, ch3_gain);

#ifdef CONFIG_ADC_DMIC
    ADC_DMIC_GAIN_CFG(0);
    ADC_DMIC_GAIN_CFG(1);
    ADC_DMIC_GAIN_CFG(2);
    ADC_DMIC_GAIN_CFG(3);
#endif

    /* single-end is 6db minus than diff mode */
    if ((ch0_input & ADC_CH_INPUT0NP_DIFF) != ADC_CH_INPUT0NP_DIFF)
        ch0_gain += 60;

    if ((ch1_input & ADC_CH_INPUT1NP_DIFF) != ADC_CH_INPUT1NP_DIFF)
        ch1_gain += 60;

    if ((ch2_input & ADC_CH_INPUT2NP_DIFF) != ADC_CH_INPUT2NP_DIFF)
        ch2_gain += 60;

    if ((ch3_input & ADC_CH_INPUT3NP_DIFF) != ADC_CH_INPUT3NP_DIFF)
        ch3_gain += 60;

    /* config channel0 gain */
    adc_ch0_gain_config(dev, ch0_input, ch0_gain);

    /* config channel1 gain */
    adc_ch1_gain_config(dev, ch1_input, ch1_gain);

    /* config channel2 gain */
    adc_ch2_gain_config(dev, ch2_input, ch2_gain);

    /* config channel3 gain */
    adc_ch3_gain_config(dev, ch3_input, ch3_gain);

    return 0;
}

/* @brief ADC sample rate config */
static int adc_sample_rate_set(struct device *dev, audio_sr_sel_e sr_khz)
{
	struct phy_adc_drv_data *data = dev->data;
    int ret;
    uint8_t pre_div, clk_div, series, pll_index;
    uint32_t reg;
    uint16_t mclk = MCLK_256FS;
    uint8_t ovfs_clk_div = 0, fir_clk_div = 0;
    uint8_t i;

    for (i = 0; i < ARRAY_SIZE(adc_anc_clk_mapping); i++) {
        if (adc_anc_clk_mapping[i].sample_rate == sr_khz) {
            mclk = adc_anc_clk_mapping[i].root_clk_div;
            ovfs_clk_div = adc_anc_clk_mapping[i].ovfs_clk_div;
            fir_clk_div = adc_anc_clk_mapping[i].fir_clk_div;
            break;
        }
    }

    /* Get audio PLL setting  */
    ret = audio_get_pll_setting(sr_khz, mclk, /* ADC clock source is fixed 256FS */
            &pre_div, &clk_div, &series);
    if (ret) {
        LOG_DBG("get pll setting error:%d", ret);
        return ret;
    }

    /* Check the pll usage and then config */
    ret = audio_pll_check_config(series, &pll_index);
    if (ret) {
        LOG_DBG("check pll config error:%d", ret);
        return ret;
    }

    reg = sys_read32(CMU_ADCCLK) & ~0x1FF;

    /* Select pll0 or pll1 */
    reg |= (pll_index & 0x1) << CMU_ADCCLK_ADCCLKSRC;

    reg |= (pre_div << CMU_ADCCLK_ADCCLKPREDIV) | (clk_div << CMU_ADCCLK_ADCCLKDIV_SHIFT);

    /* ADC OVFS clock divisor */
    reg &= ~CMU_ADCCLK_ADCOVFSCLKDIV_MASK;
    reg |= CMU_ADCCLK_ADCOVFSCLKDIV(ovfs_clk_div);

    /* ADC FIR clock divisor */
    if (fir_clk_div)
        reg |= CMU_ADCCLK_ADCFIRCLKDIV;
    else
        reg &= ~CMU_ADCCLK_ADCFIRCLKDIV;


    LOG_DBG("sr:%d pll_index:%d pre_div:%d clk_div:%d", sr_khz, pll_index, pre_div, clk_div);

	data->audio_pll_index = pll_index;

    sys_write32(reg, CMU_ADCCLK);

    return 0;
}

/* @brief Get the sample rate from the ADC config */
static int adc_sample_rate_get(struct device *dev)
{
    uint8_t pre_div, clk_div, pll_index;
    uint32_t reg;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);

    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC)) >> CMU_ADCCLK_ADCCLKSRC;
    pre_div = (reg & (1 << CMU_ADCCLK_ADCCLKPREDIV)) >> CMU_ADCCLK_ADCCLKPREDIV;
    clk_div = reg & CMU_ADCCLK_ADCCLKDIV_MASK;

    return audio_get_pll_sample_rate(MCLK_256FS, pre_div, clk_div, pll_index);
}

/* @brief Get the AUDIO_PLL APS used by ADC */
static int adc_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC)) >> CMU_ADCCLK_ADCCLKSRC;

    return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by ADC */
static int adc_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    pll_index = (reg & (1 << CMU_ADCCLK_ADCCLKSRC)) >> CMU_ADCCLK_ADCCLKSRC;

    return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief Set the AUDIO_PLL index that used by ADC */
static int adc_get_pll_index(struct device *dev, uint8_t *idx)
{
    uint32_t reg;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_ADCCLK);
    *idx = (reg & (1 << CMU_ADCCLK_ADCCLKSRC)) >> CMU_ADCCLK_ADCCLKSRC;

    return 0;
}

/* @brief ADC FIFO DRQ level set by input device */
static int adc_fifo_drq_level_set(struct device *dev, uint8_t level)
{
    return __adc_fifo_drq_level_set(dev, ADC_FIFO_0, level);
}

/* @brief ADC FIFO DRQ level get by input device */
static int adc_fifo_drq_level_get(struct device *dev, uint8_t *level)
{
    int ret;

    ret = __adc_fifo_drq_level_get(dev, ADC_FIFO_0);
    if (ret < 0) {
        LOG_ERR("Failed to get ADC FIFO DRQ level err=%d", ret);
        return ret;
    }

    *level = ret;

    LOG_DBG("ADC DRQ level %d", *level);

    return 0;
}

/* @brief check the FIFO is busy or not */
static int adc_check_fifo_busy(struct device *dev)
{
    if (__is_adc_fifo_working(dev, ADC_FIFO_0)) {
        LOG_INF("ADC FIFO0 now is working");
        return -EBUSY;
    }

    return 0;
}

/* @brief ADC FIFO enable on the basic of the input audio device usage */
static int adc_fifo_enable(struct device *dev, ain_param_t *in_param)
{
    audio_dma_width_e wd = (in_param->channel_width != CHANNEL_WIDTH_16BITS)
                            ? DMA_WIDTH_32BITS : DMA_WIDTH_16BITS;

    __adc_fifo_disable(dev, ADC_FIFO_0);
    return __adc_fifo_enable(dev, FIFO_SEL_DMA, wd,
                            ADC_FIFO_DRQ_LEVEL_DEFAULT, ADC_FIFO_0);;
}

/* @brief ADC FIFO enable on the basic of the input audio device usage */
static int adc_fifo_disable(struct device *dev)
{
    __adc_fifo_disable(dev, ADC_FIFO_0);;

    return 0;
}

/* @brief ADC BIAS setting for power saving */
static void adc_bias_setting(struct device *dev, uint16_t input_dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

#ifdef CONFIG_CFG_DRV
    struct phy_adc_drv_data *data = dev->data;
    adc_reg->bias = data->external_config.ADC_Bias_Setting;
#endif

    /* BIAS enable */
    adc_reg->bias |= ADC_BIAS_BIASEN;
}

/* @brief AUDIO LDO initialization */
static void adc_ldo_init(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    const struct phy_adc_config_data *cfg = dev->config;

    /* set the AUDIO LDO output voltage */
    adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_AULDO_VOL_MASK;
    adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_AULDO_VOL(PHY_DEV_FEATURE(ldo_voltage));
}

/* @brief Power control(enable or disable) by ADC LDO */
static void adc_ldo_power_control(struct device *dev, bool enable)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    uint32_t reg = adc_reg->ref_ldo_ctl;

    if (enable) {
        /** FIXME: HW issue
         * ADC LDO shall be enabled when use DAC individually, otherwise VREF_ADD will get low voltage.
         */

        /* AULDO pull down current control */
        reg &= ~ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK;
        reg |= ADC_REF_LDO_CTL_AULDO_PD_CTL(2);

        /* VREF voltage divide res control */
        reg &= ~ADC_REF_LDO_CTL_VREF_RSEL_MASK;
        reg |= ADC_REF_LDO_CTL_VREF_RSEL(0);

        reg |= ADC_REF_LDO_CTL_AULDO_EN(3);
        adc_reg->ref_ldo_ctl = reg;
        adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_VREF_EN;

        if (!(reg & ADC_REF_LDO_CTL_VREF_EN)) {
            LOG_INF("ADC wait for capacitor charge full");
            /* enable VREF fast charge */
            adc_reg->ref_ldo_ctl |= ADC_REF_LDO_CTL_VREF_FU;

            if (!z_is_idle_thread_object(_current))
                k_sleep(K_MSEC(ADC_LDO_CAPACITOR_CHARGE_TIME_MS));
            else
                k_busy_wait(ADC_LDO_CAPACITOR_CHARGE_TIME_MS * 1000UL);

            /* disable LDO fast charge */
            adc_reg->ref_ldo_ctl &= ~ADC_REF_LDO_CTL_VREF_FU;
        }

        /* Wait for AULDO stable */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
        else
            k_busy_wait(1000);

        /* reduce AULDO static power consume */
        uint32_t reg1 = adc_reg->ref_ldo_ctl;
        reg1 &= ~ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK;
        reg1 &= ~ADC_REF_LDO_CTL_VREF_RSEL_MASK;
        reg1 |= ADC_REF_LDO_CTL_VREF_RSEL(3);
        adc_reg->ref_ldo_ctl = reg1;
    } else {
        /* check DAC LDO status */
        uint32_t key = irq_lock();
        if (!(reg & ADC_REF_LDO_CTL_DALDO_EN_MASK)) {
            reg &= ~ADC_REF_LDO_CTL_AULDO_EN_MASK;
            reg &= ~ADC_REF_LDO_CTL_VREF_EN;
            adc_reg->ref_ldo_ctl = reg;
        }
        irq_unlock(key);
    }
}

/* @brief ADC physical level enable procedure */
static int phy_adc_enable(struct device *dev, void *param)
{
    const struct phy_adc_config_data *cfg = dev->config;
    ain_param_t *in_param = (ain_param_t *)param;
    adc_setting_t *adc_setting = in_param->adc_setting;
    int ret;

    if ((!in_param) || (!adc_setting)
        || (!in_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if (in_param->channel_type != AUDIO_CHANNEL_ADC) {
        LOG_ERR("Invalid channel type %d", in_param->channel_type);
        return -EINVAL;
    }

    ret = adc_check_fifo_busy(dev);
    if (ret)
        return ret;

    /* enable adc clock */
    acts_clock_peripheral_enable(cfg->clk_id);

    /* enable adc ldo */
    adc_ldo_power_control(dev, true);

    __adc_vmic_ctl_init(dev);

    __adc_vmic_ctl_enable(dev, adc_setting->device);

    /* audio_pll and adc clock setting */
    if (adc_sample_rate_set(dev, in_param->sample_rate)) {
        LOG_ERR("Failed to config sample rate %d", in_param->sample_rate);
        return -ESRCH;
    }

    /* ADC FIFO enable */
    ret = adc_fifo_enable(dev, in_param);
    if (ret)
        return ret;

    /* ADC digital enable */
    ret = adc_digital_enable(dev, adc_setting->device,
                        in_param->sample_rate);
    if (ret) {
        LOG_ERR("ADC digital enable error %d", ret);
        goto err;
    }

#ifdef ADC_DIGITAL_DEBUG_IN_ENABLE
    __adc_digital_debug_in(dev);
#endif

#ifdef ADC_ANALOG_DEBUG_OUT_ENABLE
    __adc_analog_debug_out(dev);
#endif

    /* set ADC BIAS */
    adc_bias_setting(dev, adc_setting->device);

    /* ADC analog input enable */
    ret = adc_input_config(dev, adc_setting->device);
    if (ret) {
        LOG_ERR("ADC input config error %d", ret);
        goto err;
    }

    /* ADC gain setting */
    ret = adc_gain_config(dev, adc_setting->device, &adc_setting->gain);
    if (ret) {
        LOG_ERR("ADC gain config error %d", ret);
        goto err;
    }

    return ret;

err:
    adc_fifo_disable(dev);
    return ret;
}

/* @brief ADC physical level disable procedure */
static int phy_adc_disable(struct device *dev, void *param)
{
	const struct phy_adc_config_data *cfg = dev->config;
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    uint16_t input_dev = *(uint16_t *)param;

    LOG_INF("disable input device:0x%x", input_dev);

    __adc_external_trigger_disable(dev);

    /* DAC FIFO reset */
    adc_fifo_disable(dev);

    if (__adc_check_fifo_all_disable(dev)) {
        adc_digital_disable(dev, input_dev);
        if (!data->anc_en) {
            adc_reg->adc_ctl0 = 0;
            adc_reg->adc_ctl1 = 0;
        } else {
#ifdef CONFIG_CFG_DRV
            if (data->external_config.Enable_ANC == ANC_FF)
                adc_reg->adc_ctl1 = 0;
            else if (data->external_config.Enable_ANC == ANC_FB)
                adc_reg->adc_ctl0 = 0;
#endif
        }
        adc_reg->adc_ctl2 = 0;
        adc_reg->adc_ctl3 = 0;

        /* TODO: check DAC depends on ADC issue */
        /*if (!data->anc_en)
            acts_clock_peripheral_disable(cfg->clk_id);*/

        data->hw_trigger_en = 0;

        /* disable VMIC power */
        if (!data->anc_en) {
            __adc_vmic_ctl_disable(dev, input_dev);

            /* disable ADC LDO */
            adc_ldo_power_control(dev, false);

			acts_clock_peripheral_disable(cfg->clk_id);
			audio_pll_unset(data->audio_pll_index);
        }
    }

    return 0;
}

/* @brief Get the ADC DMA information */
static int adc_get_dma_info(struct device *dev, struct audio_in_dma_info *info)
{
    const struct phy_adc_config_data *cfg = dev->config;

    /* use ADC FIFO0 */
    info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
    info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
    info->dma_info.dma_id = cfg->dma_fifo0.dma_id;

    return 0;
}

static int adc_channels_start(struct device *dev, struct aduio_in_adc_en *ctl)
{
    uint8_t i;
    bool ch0_en = false, ch1_en = false, ch2_en = false, ch3_en = false;
    audio_input_map_t adc_input_map = {0};

    if (ctl->input_dev_num > ADC_FIFO_MAX_NUMBER) {
        LOG_ERR("invalid input device number:%d", ctl->input_dev_num);
        return -EINVAL;
    }

    for (i = 0; i < ctl->input_dev_num; i++) {
        LOG_INF("start audio device 0x%x", ctl->input_dev_array[i]);
        adc_input_map.audio_dev = ctl->input_dev_array[i];
#ifdef CONFIG_CFG_DRV
        struct phy_adc_drv_data *data = dev->data;
        adc_input_map.ch0_input = data->input_ch0;
        adc_input_map.ch1_input = data->input_ch1;
        adc_input_map.ch2_input = data->input_ch2;
        adc_input_map.ch3_input = data->input_ch3;
#else
        if (board_audio_device_mapping(&adc_input_map))
            return -ENOENT;
#endif
        if (ADC_CH_DISABLE != adc_input_map.ch0_input)
            ch0_en = true;
        if (ADC_CH_DISABLE != adc_input_map.ch1_input)
            ch1_en = true;
        if (ADC_CH_DISABLE != adc_input_map.ch2_input)
            ch2_en = true;
        if (ADC_CH_DISABLE != adc_input_map.ch3_input)
            ch3_en = true;
    }

    __adc_digital_channels_en(dev, ch0_en, ch1_en, ch2_en, ch3_en);

    return 0;
}

/* @brief check ADC is busy or not */
static bool adc_is_busy(struct device *dev)
{
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
    struct phy_adc_drv_data *data = dev->data;
    bool is_busy = false;

    if (data->anc_en)
        is_busy = true;

    if (adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK)
        is_busy = true;

    return is_busy;
}

/* @brief ADC ANC configuration */
static int adc_anc_control(struct device *dev, adc_anc_ctl_t *ctl)
{
#ifdef CONFIG_CFG_DRV
    const struct phy_adc_config_data *cfg = dev->config;
    struct phy_adc_drv_data *data = dev->data;
    struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

    if (!ctl) {
        LOG_ERR("adc anc control invalid parameter");
        return -EINVAL;
    }

    acts_clock_peripheral_enable(cfg->clk_id);

    if (ctl->is_open_anc) {

        if (data->anc_en) {
            LOG_ERR("ADC ANC has already enabled");
            return -EACCES;
        }

        data->anc_en = 1;

        /* enable adc ldo */
        adc_ldo_power_control(dev, true);

        __adc_vmic_ctl_init(dev);

        /* set ADC BIAS */
        adc_bias_setting(dev, 0);

        /* audio_pll and adc clock setting */
        if (!(adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK)) {
            if (adc_sample_rate_set(dev, SAMPLE_RATE_16KHZ)) {
                    LOG_ERR("Failed to config sample rate");
                    data->anc_en = 0;
                    return -ESRCH;
            }
        }

        /* enable FIR/CIC/ANA clock */
        sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCCICEN \
            | CMU_ADCCLK_ADCFIREN | CMU_ADCCLK_ADCANAEN, CMU_ADCCLK);

        uint32_t reg = adc_reg->adc_digctl;

        if (adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK) {
            uint8_t sample_rate = adc_sample_rate_get(dev);
            if (sample_rate == SAMPLE_RATE_16KHZ) {
                reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
                reg |= ADC_DIGCTL_ADC_OVFS(0); /* ANC sample rate 16K CIC over sample rate select 192fs */
            } else if (sample_rate == SAMPLE_RATE_24KHZ) {
                reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
                reg |= ADC_DIGCTL_ADC_OVFS(1); /* ANC sample rate 24K CIC over sample rate select 128fs */
            } else if (sample_rate == SAMPLE_RATE_32KHZ) {
                reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
                reg |= ADC_DIGCTL_ADC_OVFS(2); /* ANC sample rate 32K CIC over sample rate select 96fs */
            } else if (sample_rate == SAMPLE_RATE_48KHZ) {
                reg &= ~ADC_DIGCTL_ADC_OVFS_MASK;
                reg |= ADC_DIGCTL_ADC_OVFS(3); /* ANC sample rate 48K CIC over sample rate select 64fs */
            } else {
                LOG_ERR("Current ADC sample rate:%d not support ANC", sample_rate);
                data->anc_en = 0;
                return -ENOTSUP;
            }
        }

        adc_reg->adc_digctl = reg;
    }

    LOG_INF("ANC status:%d mode:%d",
            ctl->is_open_anc, data->external_config.Enable_ANC);

    /**
    * ADC channels have fixed mapping to ANC mode as belown:
    *	ADC0 (INPUT0P) <--> ANC FF
    *	ADC1 (INPUT1P) <--> ANC FB
    *	ADC0/1 (INPUT0P/INPUT1P)<--> ANC FY
    */

    if (data->external_config.Enable_ANC == ANC_FF) {
        if (ctl->is_open_anc) {
            if (data->external_config.Mic_Config[0].Audio_In_Mode
                == AUDIO_IN_MODE_ADC_DIFF) {
                __adc_ch0_analog_control(dev, ADC_CH_INPUT0NP_DIFF);
                adc_ch0_gain_config(dev, ADC_CH_INPUT0NP_DIFF, data->external_config.ANC_FF_GAIN);
            } else {
                __adc_ch0_analog_control(dev, ADC_CH_INPUT0P);
                adc_ch0_gain_config(dev, ADC_CH_INPUT0P, (data->external_config.ANC_FF_GAIN + 60));
            }
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], true);
        } else {
            if (!(adc_reg->ch0_digctl & ADC_DIGCTL_ADC0_DIG_EN)) {
                adc_reg->adc_ctl0 = 0;
                __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], false);
            }
        }
    } else if (data->external_config.Enable_ANC == ANC_FB) {
        if (ctl->is_open_anc) {
            if (data->external_config.Mic_Config[1].Audio_In_Mode
                == AUDIO_IN_MODE_ADC_DIFF) {
                __adc_ch1_analog_control(dev, ADC_CH_INPUT1NP_DIFF);
                adc_ch1_gain_config(dev, ADC_CH_INPUT1NP_DIFF, data->external_config.ANC_FB_GAIN);
            } else {
                __adc_ch1_analog_control(dev, ADC_CH_INPUT1P);
                adc_ch1_gain_config(dev, ADC_CH_INPUT1P, (data->external_config.ANC_FB_GAIN + 60));
            }
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], true);
        } else {
            if (!(adc_reg->ch0_digctl & ADC_DIGCTL_ADC1_DIG_EN)) {
                adc_reg->adc_ctl1 = 0;
                __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], false);
            }
        }
    } else if (data->external_config.Enable_ANC == ANC_HY) {
        if (ctl->is_open_anc) {
            if (data->external_config.Mic_Config[2].Audio_In_Mode
                == AUDIO_IN_MODE_ADC_DIFF) {
                __adc_ch0_analog_control(dev, ADC_CH_INPUT0NP_DIFF);
                __adc_ch1_analog_control(dev, ADC_CH_INPUT1NP_DIFF);
                adc_ch0_gain_config(dev, ADC_CH_INPUT0NP_DIFF, data->external_config.ANC_FF_GAIN);
                adc_ch1_gain_config(dev, ADC_CH_INPUT1NP_DIFF, data->external_config.ANC_FB_GAIN);
            } else {
                __adc_ch0_analog_control(dev, ADC_CH_INPUT0P);
                __adc_ch1_analog_control(dev, ADC_CH_INPUT1P);
                adc_ch0_gain_config(dev, ADC_CH_INPUT0P, (data->external_config.ANC_FF_GAIN + 60));
                adc_ch1_gain_config(dev, ADC_CH_INPUT1P, (data->external_config.ANC_FB_GAIN + 60));
            }
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], true);
            __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], true);
        } else {
            if (!(adc_reg->ch0_digctl & ADC_DIGCTL_ADC0_DIG_EN)) {
                adc_reg->adc_ctl0 = 0;
                __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[0], false);
            }

            if (!(adc_reg->ch0_digctl & ADC_DIGCTL_ADC1_DIG_EN)) {
                adc_reg->adc_ctl1 = 0;
                __adc_vmic_ctl(dev, adc_vmic_index_mapping_array[1], false);
            }
        }
    }

    if (!ctl->is_open_anc) {

        if (!data->anc_en) {
            LOG_ERR("ADC ANC does not enable yet");
            return -EACCES;
        }

        data->anc_en = 0;
        /* TODO: check DAC depends on ADC issue */
        /*if ((adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK))
            acts_clock_peripheral_disable(cfg->clk_id);*/

        /* check all ADC channels are disabled */
        if (!(adc_reg->adc_digctl & ADC_DIGCTL_ADC_DIG_MASK)) {
            sys_write32(sys_read32(CMU_ADCCLK) & ~(CMU_ADCCLK_ADCDMICEN
                | CMU_ADCCLK_ADCANAEN | CMU_ADCCLK_ADCCICEN
                | CMU_ADCCLK_ADCFIREN), CMU_ADCCLK);

            /* all ADC channels disable to disable VMIC and ADC LDO */
            __adc_vmic_ctl_disable(dev, AUDIO_DEV_TYPE_AMIC);

            /* disable ADC LDO */
            adc_ldo_power_control(dev, false);
        }
    }

#endif
    return 0;
}

/* @brief ADC AEC configuration */
static int adc_aec_config(struct device *dev, bool is_en)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint8_t aec_sel = 0, pull_down = 0;

#if defined(CONFIG_CFG_DRV)
	struct phy_adc_drv_data *data = dev->data;
	if (data->external_config.Hw_Aec_Select == ADC_2)
		aec_sel = 0;
	else
		aec_sel = 1;
#elif defined(CONFIG_AUDIO_ADC_0_AEC_SEL)
	aec_sel = CONFIG_AUDIO_ADC_0_AEC_SEL;
#endif

#if defined(CONFIG_AUDIO_ADC_0_MIX_PULL_DOWN)
	pull_down = CONFIG_AUDIO_ADC_0_MIX_PULL_DOWN;
#endif

	if (is_en) {
		/* HW AEC only support ADC2/3 */
		if (aec_sel == 0) {
			adc_hpf_config(dev, ADC_CHANNEL_2, 1);
			__adc_digital_channel_cfg(dev, ADC_CHANNEL_2, ADC_AMIC, 1);
			__adc_ch2_analog_control(dev, ADC_CH_INPUT2NP_DIFF);
			__adc_digital_gain_set(dev, ADC_CHANNEL_2, 0);
			adc_ch2_gain_config(dev, ADC_CH_INPUT2NP_DIFF, 0);
			/* mask input*/
			adc_reg->adc_ctl2 &= ~(ADC2_CTL_INPUT2P_EN_MASK | ADC2_CTL_INPUT2N_EN_MASK);

			adc_reg->adc_ctl2 |= ADC2_CTL_PAL2AD2_EN;

			if (pull_down)
				adc_reg->adc_ctl2 |= ADC2_CTL_PAL2AD2_PD_EN;

	        adc_reg->adc_digctl |= ADC_DIGCTL_ADC2_DIG_EN;
			__adc_hpf_fast_stable(dev, ADC_CHANNEL_2, SAMPLE_RATE_16KHZ);
		} else if (aec_sel == 1) {
			adc_hpf_config(dev, ADC_CHANNEL_3, 1);
			__adc_digital_channel_cfg(dev, ADC_CHANNEL_3, ADC_AMIC, 1);
			__adc_ch3_analog_control(dev, ADC_CH_INPUT3NP_DIFF);
			adc_ch2_gain_config(dev, ADC_CH_INPUT3NP_DIFF, 0);
			__adc_digital_gain_set(dev, ADC_CHANNEL_3, 0);
			/* mask input*/
			adc_reg->adc_ctl3 &= ~(ADC3_CTL_INPUT3P_EN_SHIFT | ADC3_CTL_INPUT3N_EN_MASK);

			adc_reg->adc_ctl3 |= ADC3_CTL_PAL2AD3_EN;

			if (pull_down)
				adc_reg->adc_ctl3 |= ADC3_CTL_PAL2AD3_PD_EN;

			adc_reg->adc_digctl |= ADC_DIGCTL_ADC3_DIG_EN;
			__adc_hpf_fast_stable(dev, ADC_CHANNEL_3, SAMPLE_RATE_16KHZ);
		} else {
			LOG_ERR("Invalid HW AEC channel select:%d", aec_sel);
			return -EINVAL;
		}
	} else {
		if (aec_sel == 0) {
			__adc_digital_channel_cfg(dev, ADC_CHANNEL_2, ADC_AMIC, 0);
			adc_reg->adc_ctl2 = 0;
		} else if (aec_sel == 1) {
			__adc_digital_channel_cfg(dev, ADC_CHANNEL_3, ADC_AMIC, 0);
			adc_reg->adc_ctl3 = 0;
		}
	}

	return 0;
}

/* @brief ADC ioctl commands */
static int phy_adc_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    int ret = 0;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        adc_dump_register(dev);
        break;
    }
    case PHY_CMD_GET_AIN_DMA_INFO:
    {
        ret = adc_get_dma_info(dev, (struct audio_in_dma_info *)param);
        break;
    }
    case PHY_CMD_ADC_DIGITAL_ENABLE:
    {
        ret = adc_channels_start(dev, (struct aduio_in_adc_en *)param);
        break;
    }
    case PHY_CMD_ADC_GAIN_CONFIG:
    {
        adc_setting_t *setting = (adc_setting_t *)param;
        ret = adc_gain_config(dev, setting->device, &setting->gain);
        break;
    }
    case AIN_CMD_GET_SAMPLERATE:
    {
        ret = adc_sample_rate_get(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get ADC sample rate err=%d", ret);
            return ret;
        }
        *(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
        ret = 0;
        break;
    }
    case AIN_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = adc_sample_rate_set(dev, val);
        if (ret) {
            LOG_ERR("Failed to set ADC sample rate err=%d", ret);
            return ret;
        }
        break;
    }
    case AIN_CMD_GET_APS:
    {
        ret = adc_get_pll_aps(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get audio pll APS err=%d", ret);
            return ret;
        }
        *(audio_aps_level_e *)param = (audio_aps_level_e)ret;
        ret = 0;
        break;
    }
    case AIN_CMD_SET_APS:
    {
        audio_aps_level_e level = *(audio_aps_level_e *)param;
        ret = adc_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS err=%d", ret);
            return ret;
        }
        LOG_DBG("set new aps level %d", level);
        break;
    }
    case PHY_CMD_GET_AUDIOPLL_IDX:
    {
        ret = adc_get_pll_index(dev, (uint8_t *)param);
        break;
    }
    case PHY_CMD_FIFO_DRQ_LEVEL_GET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint16_t input_dev = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
        uint8_t level;

        ret = adc_fifo_drq_level_get(dev, &level);
        if (ret < 0)
            return ret;

        *(uint32_t *)param = PHY_FIFO_CMD(input_dev, level);
        ret = 0;

        break;
    }
    case PHY_CMD_FIFO_DRQ_LEVEL_SET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint8_t level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);

        ret = adc_fifo_drq_level_set(dev, level);

        break;
    }
    case AIN_CMD_SET_ADC_TRIGGER_SRC:
    {
        uint8_t src = *(uint8_t *)param;
        ret = __adc_external_trigger_enable(dev, src);
        break;
    }
    case AIN_CMD_ANC_CONTROL:
    {
        adc_anc_ctl_t *ctl = (adc_anc_ctl_t *)param;
        ret = adc_anc_control(dev, ctl);
        break;
    }
    case PHY_CMD_IS_ADC_BUSY:
    {
        *(uint8_t *)param = (uint8_t)adc_is_busy(dev);
        break;
    }
	case AIN_CMD_AEC_CONTROL:
	{
		bool en = *(bool *)param;
		ret = adc_aec_config(dev, en);
		break;
	}
    default:
        LOG_ERR("Unsupport command %d", cmd);
        return -ENOTSUP;
    }

    return ret;
}

const struct phy_audio_driver_api phy_adc_drv_api = {
    .audio_enable = phy_adc_enable,
    .audio_disable = phy_adc_disable,
    .audio_ioctl = phy_adc_ioctl
};

/* dump dac device tree infomation */
static void __adc_dt_dump_info(const struct phy_adc_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     ADC BASIC INFO     **");
    LOG_INF("     BASE: %08x", cfg->reg_base);
    LOG_INF("   CLK-ID: %08x", cfg->clk_id);
    LOG_INF("   RST-ID: %08x", cfg->rst_id);
    LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
    LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
    LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

    LOG_INF("** 	ADC FEATURES	 **");
    LOG_INF("   ADC0-HPF-TIME: %d", PHY_DEV_FEATURE(adc0_hpf_time));
    LOG_INF("ADC0-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc0_hpf_fc_high));
    LOG_INF("   ADC1-HPF-TIME: %d", PHY_DEV_FEATURE(adc1_hpf_time));
    LOG_INF("ADC1-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc1_hpf_fc_high));
    LOG_INF("   ADC2-HPF-TIME: %d", PHY_DEV_FEATURE(adc2_hpf_time));
    LOG_INF("ADC2-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc2_hpf_fc_high));
    LOG_INF("   ADC3-HPF-TIME: %d", PHY_DEV_FEATURE(adc3_hpf_time));
    LOG_INF("ADC3-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc3_hpf_fc_high));
    LOG_INF("  ADC0-FREQUENCY: %d", PHY_DEV_FEATURE(adc0_frequency));
    LOG_INF("  ADC1-FREQUENCY: %d", PHY_DEV_FEATURE(adc1_frequency));
    LOG_INF("  ADC2-FREQUENCY: %d", PHY_DEV_FEATURE(adc2_frequency));
    LOG_INF("  ADC3-FREQUENCY: %d", PHY_DEV_FEATURE(adc3_frequency));
    LOG_INF(" FAST-CAP-CHARGE: %d", PHY_DEV_FEATURE(fast_cap_charge));
    LOG_INF("     LDO-VOLTAGE: %d", PHY_DEV_FEATURE(ldo_voltage));
    LOG_INF("     VMIC-CTL-EN: <%d, %d, %d>",
            vmic_ctl_array[0], vmic_ctl_array[1], vmic_ctl_array[2]);
    LOG_INF("    VMIC-CTL-VOL: <%d, %d>",
            vmic_voltage_array[0], vmic_voltage_array[1]);
#endif
}

#ifdef CONFIG_CFG_DRV
static int __adc_config_dmic_mfp(struct device *dev, uint16_t clk_pin, uint16_t dat_pin)
{
    int ret;
    uint16_t pin, mfp;

    pin = PHY_AUDIO_PIN_NUM_CFG(clk_pin);
    mfp = PHY_AUDIO_PIN_MFP_CFG(clk_pin);
    ret = acts_pinmux_set(pin, mfp);
    if (ret) {
        LOG_ERR("pin@%d config mfp error:%d", pin, mfp);
        return ret;
    }

    pin = PHY_AUDIO_PIN_NUM_CFG(dat_pin);
    mfp = PHY_AUDIO_PIN_MFP_CFG(dat_pin);
    ret = acts_pinmux_set(pin, mfp);
    if (ret) {
        LOG_ERR("pin@%d config mfp error:%d", pin, mfp);
        return ret;
    }

    return 0;
}

/* @brief Configure PINMUX for DMIC(Digital MIC) */
static int adc_config_dmic_mfp(struct device *dev, uint8_t ch0_input,
            uint8_t ch1_input, uint8_t ch2_input, uint8_t ch3_input)
{
    int ret = 0;
    struct phy_adc_drv_data *data = dev->data;

    if ((ADC_CH_DMIC == ch0_input)
        || (ADC_CH_DMIC == ch1_input)) {
        if ((data->external_config.DMIC_Select_GPIO.DMIC01_CLK != GPIO_NONE)
            && (data->external_config.DMIC_Select_GPIO.DMIC01_DAT != GPIO_NONE)) {
            ret = __adc_config_dmic_mfp(dev,
                    data->external_config.DMIC_Select_GPIO.DMIC01_CLK,
                    data->external_config.DMIC_Select_GPIO.DMIC01_DAT);
        }
    }

    if ((ADC_CH_DMIC == ch2_input)
        || (ADC_CH_DMIC == ch3_input)) {
        if ((data->external_config.DMIC_Select_GPIO.DMIC23_CLK != GPIO_NONE)
            && (data->external_config.DMIC_Select_GPIO.DMIC23_DAT != GPIO_NONE)) {
            ret = __adc_config_dmic_mfp(dev,
                    data->external_config.DMIC_Select_GPIO.DMIC23_CLK,
                    data->external_config.DMIC_Select_GPIO.DMIC23_DAT);
        }
    }

    return ret;
}

/* @brief Configure PINMUX for ANC DMIC */
static int adc_config_anc_dmic_mfp(struct device *dev)
{
    int ret = 0;
    struct phy_adc_drv_data *data = dev->data;

    if (data->external_config.Enable_ANC) {
        if ((data->external_config.ANCDMIC_Select_GPIO.DMIC01_CLK != GPIO_NONE)
            && (data->external_config.ANCDMIC_Select_GPIO.DMIC01_DAT != GPIO_NONE)) {
            ret = __adc_config_dmic_mfp(dev,
                    data->external_config.ANCDMIC_Select_GPIO.DMIC01_CLK,
                    data->external_config.ANCDMIC_Select_GPIO.DMIC01_DAT);
        }

        if ((data->external_config.ANCDMIC_Select_GPIO.DMIC23_CLK != GPIO_NONE)
            && (data->external_config.ANCDMIC_Select_GPIO.DMIC23_DAT != GPIO_NONE)) {
            ret = __adc_config_dmic_mfp(dev,
                    data->external_config.ANCDMIC_Select_GPIO.DMIC23_CLK,
                    data->external_config.ANCDMIC_Select_GPIO.DMIC23_DAT);
        }
    }

    return ret;
}

/* @brief parser ADC INPUT from  */
static int adc_input_parser(const struct device *dev)
{
    struct phy_adc_drv_data *data = dev->data;
    uint8_t input, i;

    /* validate input channel0 */
    input = data->external_config.ADC_Select_INPUT.ADC_Input_Ch0;
    if ((!(input & ADC_CH_INPUT0P)) &&
        (!(input & ADC_CH_INPUT0NP_DIFF))) {
        LOG_ERR("invalid input ch0:%d", input);
        return -EINVAL;
    }

    /* validate input channel1 */
    input = data->external_config.ADC_Select_INPUT.ADC_Input_Ch1;
    if ((!(input & ADC_CH_INPUT1P)) &&
        (!(input & ADC_CH_INPUT1NP_DIFF))) {
        LOG_ERR("invalid input ch1:%d", input);
        return -EINVAL;
    }

    /* validate input channel2 */
    input = data->external_config.ADC_Select_INPUT.ADC_Input_Ch2;
    if ((!(input & ADC_CH_INPUT0N)) && (!(input & ADC_CH_INPUT2P)) &&
        (!(input & ADC_CH_INPUT3N)) && (!(input & ADC_CH_INPUT2NP_DIFF))) {
        LOG_ERR("invalid input ch1:%d", input);
        return -EINVAL;
    }

    /* validate input channel3 */
    input = data->external_config.ADC_Select_INPUT.ADC_Input_Ch3;
    if ((!(input & ADC_CH_INPUT1N)) && (!(input & ADC_CH_INPUT2N)) &&
        (!(input & ADC_CH_INPUT3P)) && (!(input & ADC_CH_INPUT3NP_DIFF))) {
        LOG_ERR("invalid input ch3:%d", input);
        return -EINVAL;
    }

    /* channel0 input parser */
    if (data->external_config.Record_Adc_Select & ADC_0) {
        for (i = 0; i < CFG_MAX_ADC_NUM; i++) {
            if (data->external_config.Mic_Config[i].Adc_Index == ADC_0) {
                if (data->external_config.Mic_Config[i].Mic_Type == ADC_TYPE_DMIC) {
                    data->input_ch0 = ADC_CH_DMIC;
                } else {
                    if (data->external_config.Mic_Config[i].Audio_In_Mode == AUDIO_IN_MODE_ADC_DIFF) {
                        data->input_ch0 = ADC_CH_INPUT0NP_DIFF;
                    } else {
                        data->input_ch0 = data->external_config.ADC_Select_INPUT.ADC_Input_Ch0;
                    }
                }
                break;
            }
        }

        if (i == CFG_MAX_ADC_NUM) {
            data->input_ch0 = ADC_CH_DISABLE;
        }
    } else {
        data->input_ch0 = ADC_CH_DISABLE;
    }

    /* channel1 input parser */
    if (data->external_config.Record_Adc_Select & ADC_1) {
        for (i = 0; i < CFG_MAX_ADC_NUM; i++) {
            if (data->external_config.Mic_Config[i].Adc_Index == ADC_1) {
                if (data->external_config.Mic_Config[i].Mic_Type == ADC_TYPE_DMIC) {
                    data->input_ch1 = ADC_CH_DMIC;
                } else {
                    if (data->external_config.Mic_Config[i].Audio_In_Mode == AUDIO_IN_MODE_ADC_DIFF) {
                        data->input_ch1 = ADC_CH_INPUT1NP_DIFF;
                    } else {
                        data->input_ch1 = data->external_config.ADC_Select_INPUT.ADC_Input_Ch1;
                    }
                }
                break;
            }
        }

        if (i == CFG_MAX_ADC_NUM)
            data->input_ch1 = ADC_CH_DISABLE;
    } else {
        data->input_ch1 = ADC_CH_DISABLE;
    }


    /* channel2 input parser */
    if (data->external_config.Record_Adc_Select & ADC_2) {
        for (i = 0; i < CFG_MAX_ADC_NUM; i++) {
            if (data->external_config.Mic_Config[i].Adc_Index == ADC_2) {
                if (data->external_config.Mic_Config[i].Mic_Type == ADC_TYPE_DMIC) {
                    data->input_ch2 = ADC_CH_DMIC;
                } else {
                    if (data->external_config.Mic_Config[i].Audio_In_Mode == AUDIO_IN_MODE_ADC_DIFF) {
                        data->input_ch2 = ADC_CH_INPUT2NP_DIFF;
                    } else {
                        data->input_ch2 = data->external_config.ADC_Select_INPUT.ADC_Input_Ch2;
                    }
                }
                break;
            }
        }

        if (i == CFG_MAX_ADC_NUM)
            data->input_ch2 = ADC_CH_DISABLE;
    } else {
        data->input_ch2 = ADC_CH_DISABLE;
    }

    /* channel3 input parser */
    if (data->external_config.Record_Adc_Select & ADC_3) {
        for (i = 0; i < CFG_MAX_ADC_NUM; i++) {
            if (data->external_config.Mic_Config[i].Adc_Index == ADC_3) {
                if (data->external_config.Mic_Config[i].Mic_Type == ADC_TYPE_DMIC) {
                    data->input_ch3 = ADC_CH_DMIC;
                } else {
                    if (data->external_config.Mic_Config[i].Audio_In_Mode == AUDIO_IN_MODE_ADC_DIFF) {
                        data->input_ch3 = ADC_CH_INPUT3NP_DIFF;
                    } else {
                        data->input_ch3 = data->external_config.ADC_Select_INPUT.ADC_Input_Ch3;
                    }
                }
                break;
            }
        }

        if (i == CFG_MAX_ADC_NUM)
            data->input_ch3 = ADC_CH_DISABLE;
    } else {
        data->input_ch3 = ADC_CH_DISABLE;
    }

    if ((data->input_ch0 == ADC_CH_DMIC)
        || (data->input_ch1 == ADC_CH_DMIC)
        || (data->input_ch2 == ADC_CH_DMIC)
        || (data->input_ch3 == ADC_CH_DMIC)) {
        /* configure DMIC MFP */
        if (adc_config_dmic_mfp((struct device *)dev, data->input_ch0,
            data->input_ch1, data->input_ch2, data->input_ch3)) {
            LOG_ERR("DMIC MFP config error");
            return -ENXIO;
        }
        /* configure ANC DMIC MFP */
        if (adc_config_anc_dmic_mfp((struct device *)dev)) {
            LOG_ERR("ANC DMIC MFP config error");
            return -ENXIO;
        }
    }

    LOG_INF("ADC input parser ch0:%d ch1:%d ch2:%d ch3:%d",
            data->input_ch0, data->input_ch1, data->input_ch2, data->input_ch3);

    return 0;
}

/* @brief initialize ADC external configuration */
static int phy_adc_config_init(const struct device *dev)
{
    struct phy_adc_drv_data *data = dev->data;
    int ret;
    uint8_t i;

    /* CFG_Struct_Audio_Settings */
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ADC_BIAS_SETTING, ADC_Bias_Setting);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_DMIC01_CHANNEL_ALIGNING, DMIC01_Channel_Aligning);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ENABLE_ANC, Enable_ANC);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_RECORD_ADC_SELECT, Record_Adc_Select);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ENABLE_VMIC, Enable_VMIC);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_HW_AEC_SELECT, Hw_Aec_Select);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ANC_FF_GAIN_SETTING, ANC_FF_GAIN);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ANC_FB_GAIN_SETTING, ANC_FB_GAIN);

    /* CFG_Type_DMIC_Select_GPIO */
    ret = cfg_get_by_key(ITEM_AUDIO_DMIC_SELECT_GPIO,
            &data->external_config.DMIC_Select_GPIO, sizeof(data->external_config.DMIC_Select_GPIO));
    if (ret) {
        LOG_DBG("** DMIC PINMUX **");
        LOG_DBG("DMIC01_CLK:%d", data->external_config.DMIC_Select_GPIO.DMIC01_CLK);
        LOG_DBG("DMIC01_DAT:%d", data->external_config.DMIC_Select_GPIO.DMIC01_DAT);
        LOG_DBG("DMIC23_CLK:%d", data->external_config.DMIC_Select_GPIO.DMIC23_CLK);
        LOG_DBG("DMIC23_DAT:%d", data->external_config.DMIC_Select_GPIO.DMIC23_DAT);
    }

    /* CFG_Type_DMIC_Select_GPIO */
    ret = cfg_get_by_key(ITEM_AUDIO_ANCDMIC_SELECT_GPIO,
            &data->external_config.DMIC_Select_GPIO, sizeof(data->external_config.DMIC_Select_GPIO));
    if (ret) {
        LOG_DBG("** ANCDMIC PINMUX **");
        LOG_DBG("DMIC01_CLK:%d", data->external_config.DMIC_Select_GPIO.DMIC01_CLK);
        LOG_DBG("DMIC01_DAT:%d", data->external_config.DMIC_Select_GPIO.DMIC01_DAT);
        LOG_DBG("DMIC23_CLK:%d", data->external_config.DMIC_Select_GPIO.DMIC23_CLK);
        LOG_DBG("DMIC23_DAT:%d", data->external_config.DMIC_Select_GPIO.DMIC23_DAT);
    }

    /* CFG_Type_Mic_Config */
    ret = cfg_get_by_key(ITEM_AUDIO_MIC_CONFIG,
            &data->external_config.Mic_Config, sizeof(data->external_config.Mic_Config));
    if (ret) {
        for (i = 0; i < ARRAY_SIZE(data->external_config.Mic_Config); i++) {
            if (ADC_NONE != data->external_config.Mic_Config[i].Adc_Index) {
                LOG_DBG("** MIC config@%d Info **", i);
                LOG_DBG("Adc_Index:%d", data->external_config.Mic_Config[i].Adc_Index);
                LOG_DBG("Mic_Type:%d", data->external_config.Mic_Config[i].Mic_Type);
                LOG_DBG("Audio_In_Mode:%d", data->external_config.Mic_Config[i].Audio_In_Mode);
            }
        }
    }

    /* CFG_Type_ADC_Select_INPUT */
    ret = cfg_get_by_key(ITEM_AUDIO_ADC_INPUT_SELECT,
            &data->external_config.ADC_Select_INPUT, sizeof(data->external_config.ADC_Select_INPUT));
    if (!ret) {
        LOG_ERR("ADC input selection error");
        return -ENOENT;
    } else {
        LOG_INF("ADC input ch0:%d ch1:%d ch2:%d ch3:%d",
            data->external_config.ADC_Select_INPUT.ADC_Input_Ch0,
            data->external_config.ADC_Select_INPUT.ADC_Input_Ch1,
            data->external_config.ADC_Select_INPUT.ADC_Input_Ch2,
            data->external_config.ADC_Select_INPUT.ADC_Input_Ch3);
    }

    return adc_input_parser(dev);
}
#endif

static int phy_adc_init(const struct device *dev)
{
    const struct phy_adc_config_data *cfg = dev->config;
    struct phy_adc_drv_data *data = dev->data;

    __adc_dt_dump_info(cfg);

    /* reset ADC controller */
    acts_reset_peripheral(cfg->rst_id);

    memset(data, 0, sizeof(struct phy_adc_drv_data));

    adc_ldo_init((struct device *)dev);

#ifdef CONFIG_CFG_DRV
    int ret;
    ret = phy_adc_config_init(dev);
    if (ret)
        LOG_ERR("ADC external config init error:%d", ret);
#endif

    printk("ADC init successfully\n");

    return 0;
}

/* physical adc driver data */
static struct phy_adc_drv_data phy_adc_drv_data0;

/* physical adc config data */
static const struct phy_adc_config_data phy_adc_config_data0 = {
    .reg_base = AUDIO_ADC_REG_BASE,
    AUDIO_DMA_FIFO_DEF(ADC, 0),
    .clk_id = CLOCK_ID_ADC,
    .rst_id = RESET_ID_ADC,

    PHY_DEV_FEATURE_DEF(adc0_hpf_time) = CONFIG_AUDIO_ADC_0_CH0_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc0_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH0_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc1_hpf_time) = CONFIG_AUDIO_ADC_0_CH1_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc1_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH1_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc2_hpf_time) = CONFIG_AUDIO_ADC_0_CH2_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc2_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH2_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc3_hpf_time) = CONFIG_AUDIO_ADC_0_CH3_HPF_TIME,
    PHY_DEV_FEATURE_DEF(adc3_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH3_HPF_FC_HIGH,
    PHY_DEV_FEATURE_DEF(adc0_frequency) = CONFIG_AUDIO_ADC_0_CH0_FREQUENCY,
    PHY_DEV_FEATURE_DEF(adc1_frequency) = CONFIG_AUDIO_ADC_0_CH1_FREQUENCY,
    PHY_DEV_FEATURE_DEF(adc2_frequency) = CONFIG_AUDIO_ADC_0_CH2_FREQUENCY,
    PHY_DEV_FEATURE_DEF(adc3_frequency) = CONFIG_AUDIO_ADC_0_CH3_FREQUENCY,
    PHY_DEV_FEATURE_DEF(ldo_voltage) = CONFIG_AUDIO_ADC_0_LDO_VOLTAGE,
    PHY_DEV_FEATURE_DEF(fast_cap_charge) = CONFIG_AUDIO_ADC_0_FAST_CAP_CHARGE,
};

#if IS_ENABLED(CONFIG_AUDIO_ADC_0)
DEVICE_DEFINE(adc0, CONFIG_AUDIO_ADC_0_NAME, phy_adc_init, NULL,
        &phy_adc_drv_data0, &phy_adc_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_adc_drv_api);
#endif
