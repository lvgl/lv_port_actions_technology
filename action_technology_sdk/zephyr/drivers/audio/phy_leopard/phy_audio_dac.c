/**
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DAC physical channel implementation
 */

/*
 * Features:
 *    - Build in a 24 bits input sigma-delta DAC.
 *    - 16 * 2 * 24 bits FIFO.
 *    - Support digital volume with zero cross detection.
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k.
 *	- Support antipop to restrain noise.
 */

/*
 * Signal List
 * 	- AVCC: Analog power
 *	- AGND: Analog ground
 *	- PAGND: Ground for PA
 *	- AOUTL/AOUTLP: Left output of PA / Left Positive output of PA
 *	- AOUTR/AOUTLN: Right output of PA / Left Negative output of PA
 *	- AOUTRP/VRO: Right Positive output of PA / Virtual Ground for PA
 *	- AOUTRN/VROS: Right Negative output of PA / VRO Sense for PA
 */

#include <kernel.h>
#include <device.h>
#include <ksched.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include <drivers/audio/audio_out.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#include <drivers/gpio.h>
#endif
#include <logging/log.h>
LOG_MODULE_REGISTER(dac0, LOG_LEVEL_DBG);

/***************************************************************************************************
 * DAC_DIGCTL
 */
#define DAC_DIGCTL_FIR_MODE_SHIFT                              (30)
#define DAC_DIGCTL_FIR_MODE_MASK                               (0x3 << DAC_DIGCTL_FIR_MODE_SHIFT)
#define DAC_DIGCTL_FIR_MODE(x)                                 ((x) << DAC_DIGCTL_FIR_MODE_SHIFT)
#define DAC_DIGCTL_OVDT_PD                                      (27) /*    */
#define DAC_DIGCTL_OVDT_IRQ                                     (26) /*  */
#define DAC_DIGCTL_AUMUTE_CTL                                   BIT(25)
#define DAC_DIGCTL_SDM_DITH_EN                                 BIT(24) /* MIX a DC reference to SDM input */
#define DAC_DIGCTL_DAF0M2DAEN                                  BIT(17) /* DAC FIFO0 MIX to PCMBUF */
#define DAC_DIGCTL_DADEN                                       BIT(15) /* DAC analog DEBUG enable */
#define DAC_DIGCTL_DDDEN                                       BIT(14) /* DAC digital DEBUG enable */
#define DAC_DIGCTL_AD2DALPEN_L                                 BIT(12) /* ADC0 loop back to DAC Left channel */
#define DAC_DIGCTL_AD2DALPEN_R                                 BIT(11) /* ADC1 loop back to DAC right channel */
#define DAC_DIGCTL_ADC01MIX                                    BIT(10) /* 0: ADCL/R loopback to DACL/R; 1: ADCL MIX ADCR and then loopback to DACL/R */
#define DAC_DIGCTL_DACHNUM                                     BIT(8)
#define DAC_DIGCTL_SR_SHIFT                                    (4) /* FIFO1 IRQ/DRQ level and max to 30 levels*/
#define DAC_DIGCTL_SR_MASK                                     (0xF << DAC_DIGCTL_SR_SHIFT)
#define DAC_DIGCTL_SR(x)                                       ((x) << DAC_DIGCTL_SR_SHIFT)
#define DAC_DIGCTL_CIC_RATE                                    BIT(3)
#define DAC_DIGCTL_ENDITH                                      BIT(2) /* DAC dither enable */
#define DAC_DIGCTL_DDEN                                        BIT(0) /* DAC digital enable */

/***************************************************************************************************
 * DAC_FIFOCTL
 */
#define DAC_FIFOCTL_DRQ1_LEVEL_SHIFT                           (28) /* FIFO1 IRQ/DRQ level and max to 30 levels*/
#define DAC_FIFOCTL_DRQ1_LEVEL_MASK                            (0xF << DAC_FIFOCTL_DRQ1_LEVEL_SHIFT)
#define DAC_FIFOCTL_DRQ1_LEVEL(x)                              ((x) << DAC_FIFOCTL_DRQ1_LEVEL_SHIFT)
#define DAC_FIFOCTL_FIFO1_VOL_SHIFT                            (24)
#define DAC_FIFOCTL_FIFO1_VOL_MASK                             (0xF << DAC_FIFOCTL_FIFO1_VOL_SHIFT)
#define DAC_FIFOCTL_FIFO1_VOL(x)                               ((x) << DAC_FIFOCTL_FIFO1_VOL_SHIFT)
#define DAC_FIFOCTL_DACFIFO1_DMAWIDTH                          BIT(23) /* 0: 32bits; 1: 16bits */
#define DAC_FIFOCTL_DAF1IS_SHIFT                               (20)
#define DAC_FIFOCTL_DAF1IS_MASK                                (0x3 << DAC_FIFOCTL_DAF1IS_SHIFT)
#define DAC_FIFOCTL_DAF1IS(x)                                  ((x) << DAC_FIFOCTL_DAF1IS_SHIFT)
#define DAC_FIFOCTL_DAF1EIE                                    BIT(18) /* DAC FIFO1 Half-Empty IRQ enable */
#define DAC_FIFOCTL_DAF1EDE                                    BIT(17) /* DAC FIFO1 Half-Empty DRQ enable */
#define DAC_FIFOCTL_DAF1RT                                     BIT(16) /* DAC FIFO1 reset */
#define DAC_FIFOCTL_DRQ0_LEVEL_SHIFT                           (12)
#define DAC_FIFOCTL_DRQ0_LEVEL_MASK                            (0xF << DAC_FIFOCTL_DRQ0_LEVEL_SHIFT)
#define DAC_FIFOCTL_DRQ0_LEVEL(x)                              ((x) << DAC_FIFOCTL_DRQ0_LEVEL_SHIFT)
#define DAC_FIFOCTL_DACFIFO0_DMAWIDTH                          BIT(7)
#define DAC_FIFOCTL_DAF0IS_SHIFT                               (4)
#define DAC_FIFOCTL_DAF0IS_MASK                                (0x7 << DAC_FIFOCTL_DAF0IS_SHIFT)
#define DAC_FIFOCTL_DAF0IS(x)                                  ((x) << DAC_FIFOCTL_DAF0IS_SHIFT)
#define DAC_FIFOCTL_DAF0_DSPDMA_EN                                    BIT(3)
#define DAC_FIFOCTL_DAF0EIE                                    BIT(2)
#define DAC_FIFOCTL_DAF0EDE                                    BIT(1)
#define DAC_FIFOCTL_DAF0RT                                     BIT(0)

/***************************************************************************************************
 * DAC_STAT
 */
#define DAC_STAT_FIFO1_ER                                      BIT(8) /* FIFO1 ERROR */
#define DAC_STAT_DAF0EIP                                       BIT(7)
#define DAC_STAT_DAF0F                                         BIT(6)
#define DAC_STAT_DAF0S_SHIFT                                   (0)
#define DAC_STAT_DAF0S_MASK                                    (0x1F << DAC_STAT_DAF0S_SHIFT)
#define DAC_STAT_DAF0S(x)                                      (x<<DAC_STAT_DAF0S_SHIFT)

/***************************************************************************************************
 * DAC_DAT_FIFO0
 */
#define DAC_DAT_FIFO0_DAFDAT_SHIFT                            (8)
#define DAC_DAT_FIFO0_DAFDAT_MASK                             (0xFFFFFF << DAC_DAT_FIFO0_DAFDAT_SHIFT)
#define DAC_DAT_FIFO0_DAFDAT(x)                               (((x) & DAC_DAT_FIFO0_DAFDAT_MASK) >> DAC_DAT_FIFO0_DAFDAT_SHIFT)

/***************************************************************************************************
 * DAC_DAT_FIFO1
 */
#define DAC_DAT_FIFO1_DAFDAT_SHIFT                            (8)
#define DAC_DAT_FIFO1_DAFDAT_MASK                             (0xFFFFFF << DAC_DAT_FIFO1_DAFDAT_SHIFT)
#define DAC_DAT_FIFO1_DAFDAT(x)                               (((x) & DAC_DAT_FIFO1_DAFDAT_MASK) >> DAC_DAT_FIFO1_DAFDAT_SHIFT)

/***************************************************************************************************
 * PCM_BUF_CTL
 */
#define PCM_BUF_CTL_PCMBEPIE                                   BIT(7) /* PCMBUF empty IRQ enable */
#define PCM_BUF_CTL_PCMBFUIE                                   BIT(6) /* PCMBUF full IRQ enable */
#define PCM_BUF_CTL_PCMBHEIE                                   BIT(5) /* PCMBUF half empty IRQ enable */
#define PCM_BUF_CTL_PCMBHFIE                                   BIT(4) /* PCMBUF half full IRQ enable */
#define PCM_BUF_CTL_IRQ_MASK                                   (0xF << 4)

/***************************************************************************************************
 * PCM_BUF_STAT
 */
#define PCM_BUF_STAT_PCMBEIP                                   BIT(19) /* PCMBUF empty IRQ pending */
#define PCM_BUF_STAT_PCMBFIP                                   BIT(18) /* PCMBUF full IRQ pending */
#define PCM_BUF_STAT_PCMBHEIP                                  BIT(17) /* PCMBUF half empty IRQ pending */
#define PCM_BUF_STAT_PCMBHFIP                                  BIT(16) /* PCMBUF half full IRQ pending */
#define PCM_BUF_STAT_IRQ_MASK                                  (0xF << 16)
#define PCM_BUF_STAT_PCMBS_SHIFT                               (0)
#define PCM_BUF_STAT_PCMBS_MASK                                (0x7FF << PCM_BUF_STAT_PCMBS_SHIFT) /* indicates the available samples to fill */
#define PCM_BUF_STAT_PCMBS(x)                                  (x<<PCM_BUF_STAT_PCMBS_SHIFT)

/***************************************************************************************************
 * PCM_BUF_THRES_HE
 */
#define PCM_BUF_THRES_HE_THRESHOLD_SHIFT                       (0)
#define PCM_BUF_THRES_HE_THRESHOLD_MASK                        (0xFFF << PCM_BUF_THRES_HE_THRESHOLD_SHIFT)

/***************************************************************************************************
 * PCM_BUF_THRES_HF
 */
#define PCM_BUF_THRES_HF_THRESHOLD_SHIFT                       (0)
#define PCM_BUF_THRES_HF_THRESHOLD_MASK                        (0xFFF << PCM_BUF_THRES_HF_THRESHOLD_SHIFT)

/***************************************************************************************************
 * SDM_RESET_CTL
 */
#define SDM_RESET_CTL_SDMCNT_SHIFT                             (16) /* SDM mute counter */
#define SDM_RESET_CTL_SDMCNT_MASK                              (0xFFFF << SDM_RESET_CTL_SDMCNT_SHIFT)
#define SDM_RESET_CTL_SDMCNT(x)                                ((x) << SDM_RESET_CTL_SDMCNT_SHIFT)
#define SDM_RESET_CTL_SDMNDTH_SHIFT                            (4) /* SDM noise detection threshold */
#define SDM_RESET_CTL_SDMNDTH_MASK                             (0xFFF << SDM_RESET_CTL_SDMNDTH_SHIFT)
#define SDM_RESET_CTL_SDMNDTH(x)                               ((x) << SDM_RESET_CTL_SDMNDTH_SHIFT)
#define SDM_RESET_CTL_SDMRDS_L                                 BIT(1) /* 1: SDM detect L valid */
#define SDM_RESET_CTL_SDMREEN                                  BIT(0) /* reset SDM when has detected noise to avoid IDLE TONE; enable when sample rate 8K/11K/12K/16K */

/***************************************************************************************************
 * AUTO_MUTE_CTL
 */
#define AUTO_MUTE_CTL_AMCNT_SHIFT                              (16) /* auto mute counter */
#define AUTO_MUTE_CTL_AMCNT_MASK                               (0xFFFF << AUTO_MUTE_CTL_AMCNT_SHIFT)
#define AUTO_MUTE_CTL_AMCNT(x)                                 ((x) << AUTO_MUTE_CTL_AMCNT_SHIFT)
#define AUTO_MUTE_CTL_AMTH_SHIFT                               (4) /* auto mute threshold */
#define AUTO_MUTE_CTL_AMTH_MASK                                (0xFFF << AUTO_MUTE_CTL_AMTH_SHIFT)
#define AUTO_MUTE_CTL_AMTH(x)                                  ((x) << AUTO_MUTE_CTL_AMTH_SHIFT)
#define AUTO_MUTE_CTL_AMPD_OUT                                 BIT(3) 
#define AUTO_MUTE_CTL_AMPD_IN                                  BIT(2)
#define AUTO_MUTE_CTL_AM_IRQ_EN                                BIT(1) /* auto mute IRQ enable */
#define AUTO_MUTE_CTL_AMEN                                     BIT(0) /* auto mute function enable */

/***************************************************************************************************
 * VOL_LCH
 */
#define VOL_LCH_DONE_PD			                               BIT(22)
#define VOL_LCH_VOLL_IRQ_EN                                    BIT(21)
#define VOL_LCH_TO_CNT                                         BIT(20)
#define VOL_LCH_ADJ_CNT_SHIFT                                  (12)
#define VOL_LCH_ADJ_CNT_MASK                                   (0xFF << VOL_LCH_ADJ_CNT_SHIFT) /* the same as sample rate */
#define VOL_LCH_ADJ_CNT(x)                                     ((x) << VOL_LCH_ADJ_CNT_SHIFT)
#define VOL_LCH_DONE_STA			                           BIT(11) /* If 1 to indicate that DAC volume left channel soft stepping gain reach target done */
#define VOL_LCH_SOFT_STEP_EN                                   BIT(10)
#define VOL_LCH_VOLLZCTOEN                                     BIT(9)
#define VOL_LCH_VOLLZCEN                                       BIT(8)
#define VOL_LCH_VOLL_SHIFT                                     (0)
#define VOL_LCH_VOLL_MASK                                      (0xFF << VOL_LCH_VOLL_SHIFT)
#define VOL_LCH_VOLL(x)                                        ((x) << VOL_LCH_VOLL_SHIFT)

#define VOL_LCH_SOFT_CFG_DEFAULT                               (VOL_LCH_VOLLZCEN | VOL_LCH_VOLLZCTOEN | VOL_LCH_SOFT_STEP_EN)

/***************************************************************************************************
 * VOL_RCH
 */
#define VOL_RCH_DONE_PD			                               BIT(22)
#define VOL_RCH_VOLR_IRQ_EN                                    BIT(21)
#define VOL_RCH_TO_CNT                                         BIT(20)
#define VOL_RCH_ADJ_CNT_SHIFT                                  (12)
#define VOL_RCH_ADJ_CNT_MASK                                   (0xFF << VOL_RCH_ADJ_CNT_SHIFT)
#define VOL_RCH_ADJ_CNT(x)                                     ((x) << VOL_RCH_ADJ_CNT_SHIFT)
#define VOL_RCH_DONE_STA			                           BIT(11)
#define VOL_RCH_SOFT_STEP_EN                                   BIT(10)
#define VOL_RCH_VOLRZCTOEN                                     BIT(9)
#define VOL_RCH_VOLRZCEN                                       BIT(8)
#define VOL_RCH_VOLR_SHIFT                                     (0)
#define VOL_RCH_VOLR_MASK                                      (0xFF << VOL_RCH_VOLR_SHIFT)
#define VOL_RCH_VOLR(x)                                        ((x) << VOL_RCH_VOLR_SHIFT)

#define VOL_RCH_SOFT_CFG_DEFAULT                               (VOL_RCH_VOLRZCEN | VOL_RCH_VOLRZCTOEN | VOL_RCH_SOFT_STEP_EN)

/***************************************************************************************************
 * PCM_BUF_CNT
 */
#define PCM_BUF_CNT_IP                                         BIT(18) /* overflow pending */
#define PCM_BUF_CNT_IE                                         BIT(17)
#define PCM_BUF_CNT_EN                                         BIT(16)
#define PCM_BUF_CNT_CNT_SHIFT                                  (0)
#define PCM_BUF_CNT_CNT_MASK                                   (0xFFFF << PCM_BUF_CNT_CNT_SHIFT)

/***************************************************************************************************
 * DAC_ANACTL0
 */
#define DAC_ANACTL0_OVDTOUT                                    BIT(23) /* pa/vro over load state (readonly) */
#define DAC_ANACTL0_OVCDB                                       BIT(22)
#define DAC_ANACTL0_OVCSELH_SHIFT                               (17)
#define DAC_ANACTL0_OVCSELH_MASK                                (0x1f<<DAC_ANACTL0_OVCSELH_SHIFT)
#define DAC_ANACTL0_OVCSELH(x)                                  (x<<DAC_ANACTL0_OVCSELH_SHIFT)
#define DAC_ANACTL0_OVDTEN                                      BIT(16)
#define DAC_ANACTL0_SEL_FBCAP                                   BIT(13)
#define DAC_ANACTL0_DFCEN                                       BIT(12)
#define DAC_ANACTL0_PAVOL_SHIFT                                 (9)
#define DAC_ANACTL0_PAVOL_MASK                                  (0x7<<DAC_ANACTL0_PAVOL_SHIFT)
#define DAC_ANACTL0_PAVOL(x)                                    (x<<DAC_ANACTL0_PAVOL_SHIFT)
#define DAC_ANACTL0_DARSET                                      BIT(8)
#define DAC_ANACTL0_PAOSEN                                      BIT(5)
#define DAC_ANACTL0_PAEN                                        BIT(4)
#define DAC_ANACTL0_ZERODT                                      BIT(3)
#define DAC_ANACTL0_HOLDEBUGEN                                  BIT(2)
#define DAC_ANACTL0_DAENL                                       BIT(1) /* DAC enable */
#define DAC_ANACTL0_BIASEN                                      BIT(0) /* DAC + PA current bias enable */

/***************************************************************************************************
 * DAC_ANACTL1
 */
#define DAC_ANACTL1_RAMDDEBUG                                     (26)
#define DAC_ANACTL1_RAMPDSTEP_SHIFT                               (24)
#define DAC_ANACTL1_RAMPDSTEP_MASK                                (0x3 << DAC_ANACTL1_RAMPDSTEP_SHIFT)
#define DAC_ANACTL1_RAMPDSTEP(x)                                  (x<<DAC_ANACTL1_RAMPDSTEP_SHIFT)
#define DAC_ANACTL1_RAMPDINI                                      (23)
#define DAC_ANACTL1_RAMPVOL_SHIFT                                 (20)
#define DAC_ANACTL1_RAMPVOL_MASK                                  (0x3 << DAC_ANACTL1_RAMPVOL_SHIFT)
#define DAC_ANACTL1_RAMPVOL(x)                                    (x<<DAC_ANACTL1_RAMPVOL_SHIFT)
#define DAC_ANACTL1_SMCCKS_SHIFT                                  (18)
#define DAC_ANACTL1_RAMPCLKSEL_MASK                               (0x3 << DAC_ANACTL1_SMCCKS_SHIFT)
#define DAC_ANACTL1_RAMPCLKSEL(x)                                 BIT(x<<DAC_ANACTL1_RAMPCLKSEL_SHIFT)
#define DAC_ANACTL1_RAMPOPEN                                      BIT(17)
#define DAC_ANACTL1_RAMPDEN                                       BIT(16)
#define DAC_ANACTL1_ATPRPD                                        BIT(6)
#define DAC_ANACTL1_ATPSW2                                        BIT(5)
#define DAC_ANACTL1_ATPSW1                                        BIT(4)
#define DAC_ANACTL1_BCDISCH                                       BIT(3)
#define DAC_ANACTL1_ATPRC2EN                                      BIT(2)                               
#define DAC_ANACTL1_ATPRCEN                                       BIT(1)
#define DAC_ANACTL1_L2PEN                                         BIT(0)
/***************************************************************************************************
 * DAC_ANACTL2
 */
#define DAC_ANACTL2_SHDEBUGEN                                  BIT(20)
#define DAC_ANACTL2_SHCL_SET_SHIFT                             (12) /* DAC SH clock divisor setting step2 */
#define DAC_ANACTL2_SHCL_SET_MASK                              (0xFF << DAC_ANACTL2_SHCL_SET_SHIFT)
#define DAC_ANACTL2_SHCL_SET(x)                                ((x) << DAC_ANACTL2_SHCL_SET_SHIFT)
#define DAC_ANACTL2_SHCL_PW_SHIFT                              (4) /* DAC SH clock divisor setting step1 */
#define DAC_ANACTL2_SHCL_PW_MASK                               (0xFF << DAC_ANACTL2_SHCL_PW_SHIFT)
#define DAC_ANACTL2_SHCL_PW(x)                                 ((x) << DAC_ANACTL2_SHCL_PW_SHIFT)
#define DAC_ANACTL2_SHCL_SEL_SHIFT                             (2) 
#define DAC_ANACTL2_SHCL_SEL_MASK                              (0x3 << DAC_ANACTL2_SHCL_SEL_SHIFT)
#define DAC_ANACTL2_SHCL_SEL(x)                                ((x) << DAC_ANACTL2_SHCL_SEL_SHIFT)
#define DAC_ANACTL2_SH_CLKEN                                   BIT(0) /* DAC SH clock enable for THD+N */

/***************************************************************************************************
 * SDM_SAMPLES_CNT
 */
#define SDM_SAMPLES_CNT_IP                                     BIT(30) /* SDM sample counter overflow irq pending */
#define SDM_SAMPLES_CNT_IE                                     BIT(29) /* SDM sample counter overflow irq enable */
#define SDM_SAMPLES_CNT_EN                                     BIT(28) /* SDM sample counter enable */
#define SDM_SAMPLES_CNT_CNT_SHIFT                              (0) /* SDM sample counter */
#define SDM_SAMPLES_CNT_MASK                                   (0xFFFFFFF << SDM_SAMPLES_CNT_CNT_SHIFT)

/***************************************************************************************************
 * SDM_SAMPLES_NUM
 */
#define SDM_SAMPLES_NUM_CNT_SHIFT                              (0) /* backup DAC_SDM_SAMPLES_CNT when tws/timer irq occured */
#define SDM_SAMPLES_NUM_CNT_MASK                               (0xFFFFFFF << SDM_SAMPLES_NUM_CNT_SHIFT)

/***************************************************************************************************
 * HW_TRIGGER_DAC_CTL
 */
#define HW_TRIGGER_DAC_CTL_DSPDMA_DRQ_EN                       BIT(8)
#define HW_TRIGGER_DAC_CTL_INT_TO_SDMCNT_EN                    BIT(7) /* if 1 to enable SDM counter trigger */
#define HW_TRIGGER_DAC_CTL_INT_TO_DACFIFO_EN                   BIT(6) /* if 1 to enable external signal trigger DAC FIFO */
#define HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT                      BIT(5) /* enable to backup DAC_SDM_SAMPLES_CNT */
#define HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN                       BIT(4) /* enable external irq signals to start DAC digital */
#define HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT               (0) /* external irq source selection */
#define HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SRL_MASK                (0xF << HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT)
#define HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SRL(x)                  ((x) << HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SEL_SHIFT)

/***************************************************************************************************
 * DC_REF_DAT
 */
#define DC_REF_DAT_DC_DAT_SEL_SHIFT                             (8)
#define DC_REF_DAT_DC_DAT_SEL_MASK                              (0xFFFFFF<<DC_REF_DAT_DC_DAT_SEL_SHIFT)
#define DC_REF_DAT_DC_DAT(X)                                    ((X)<<DC_REF_DAT_DC_DAT_SEL_SHIFT)

/***************************************************************************************************
 * DAC_OSCTL
 */
#define DAC_OSCTL_DAOS_L_SEL_SHIFT                             (0)
#define DAC_OSCTL_DAOS_L_SEL_MASK                              (0xFF<<DAC_OSCTL_DAOS_L_SEL_SHIFT)
#define DAC_OSCTL_DAOS_L(X)                                    ((X)<<DAC_OSCTL_DAOS_L_SEL_SHIFT)

/***************************************************************************************************
 * DAC_DEBUG
 */
#define DEBUGSEL                                               (0x40068410)
#define DEBUGIE0                                               (0x40068420)
#define DEBUGOE0                                               (0x40068430)
#define DEBUGOE1                                               (0x40068434)
#define DEBUGSEL_DBGSE_SHIFT                                   (0)
#define DEBUGSEL_DBGSE_MASK                                    (0x7F << DEBUGSEL_DBGSE_SHIFT)
#define DEBUGSEL_DBGSE(x)                                      ((x) << DEBUGSEL_DBGSE_SHIFT)
#define DBGSE_DAC											   (0xc)

/***************************************************************************************************
 * ADC_REF_LDO_CTL
 */
#define ADC_REF_LDO_CTL_BASE                                    (0x4005c130)
#define ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT                      (22) /* AULDO pull down current control. 0: small; 3: large */
#define ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK                       (0x3 << ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_PD_CTL(x)                         ((x) << ADC_REF_LDO_CTL_AULDO_PD_CTL_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_EN_SHIFT                          (18) /* AULDO enable for ADC */
#define ADC_REF_LDO_CTL_AULDO_EN_MASK                           (0x3 << ADC_REF_LDO_CTL_AULDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_AULDO_EN(x)                             ((x) << ADC_REF_LDO_CTL_AULDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_DALDO_EN_SHIFT                          (10) /* DALDO enable for DAC */
#define ADC_REF_LDO_CTL_DALDO_EN_MASK                           (0x3 << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_DALDO_EN(x)                             ((x) << ADC_REF_LDO_CTL_DALDO_EN_SHIFT)
#define ADC_REF_LDO_CTL_VREF_RSEL_SHIFT                         (2) /* VREF voltage divide res control */
#define ADC_REF_LDO_CTL_VREF_RSEL_MASK                          (0x3 << ADC_REF_LDO_CTL_VREF_RSEL_SHIFT)
#define ADC_REF_LDO_CTL_VREF_RSEL(x)                            ((x) << ADC_REF_LDO_CTL_VREF_RSEL_SHIFT)
#define ADC_REF_LDO_CTL_VREF_FU                                 BIT(1) /* VREF fastup control */
#define ADC_REF_LDO_CTL_VREF_EN                                 BIT(0) /* VREF enable control */

/***************************************************************************************************
 * ADC_DIGCTL
 */
#define ADC_DIGCTL_BASE                                         (0x4005c100)
#define ADC_DIGCTL_ADC_DIG_SHIFT                                (12)
#define ADC_DIGCTL_ADC_DIG_MASK                                 ((0xF) << ADC_DIGCTL_ADC_DIG_SHIFT)

/***************************************************************************************************
 * DAC FEATURES CONGIURATION
 */
#define DAC_FIFO_MAX_DRQ_LEVEL                                 (0xE)//reg里没有写对应值，是否为这个？
#define DAC_FIFO_DRQ_LEVEL_DEFAULT                             (0x8) /* 16 level */

#define DAC_FIFO_MAX_VOL_LEVEL                                 (0xF)
#define DAC_FIFO_VOL_LEVEL_DEFAULT                             (0x3) /* 0db */

/* DAC volume soft step to_cnt default setting(0 : 8x; 1 : 128x).  */
#define DAC_VOL_TO_CNT_DEFAULT                                 (0)

/* The minimal volume value to mute automatically */
#define VOL_MUTE_MIN_DB                                        (-800000)

#define VOL_DB_TO_INDEX(x)                                     (((x) + 374) / 375)
#define VOL_INDEX_TO_DB(x)                                     ((x) * 375)
#define VOL_LEVEL_0DB                                          (0xBE)

#define DAC_FIFO_INVALID_INDEX(x)                              (((x) != AOUT_FIFO_DAC0) && ((x) != AOUT_FIFO_DAC1))

#define DAC_FIFO_MAX_LEVEL                                     (16)

#define DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS                         (130) /* PCMBUF 2k samples spends 62.5ms in 16Kfs */

#define DAC_PCMBUF_MAX_CNT                                     (0x400)

#define DAC_PCMBUF_DEFAULT_IRQ                                 (PCM_BUF_CTL_PCMBHEIE)

#define DAC_CHANNEL_NUM_MAX                                    (4)

#define DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US                  (100000)

#define DAC_FIFO_CNT_CLEAR_PENDING_TIME_US                     (200)

//#define DAC_ANALOG_DEBUG_IN_ENABLE

#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
#define DAC_DIGITAL_DEBUG_OUT_ENABLE
#endif

#define DAC_DIGITAL_DEBUG_OUT_CHANNEL_SEL                      (2) /* 1: debug left channel; others: debug right channel */

#define DAC_LDO_CAPACITOR_CHARGE_TIME_MS                       (10) /* Wait time for AOUT L/R capacitor charge full */

#define DAC_HIGH_PERFORMANCE_WAIT_SH_TIME_MS                   (3) /* Wait time for SH establish stable state */

#define ANC_MIX_TO_DAC_WAIT_STABLE_TIME_MS                     (3) /* Wait time for DAC interpolation and OSR stable for ANC */

#define FIR_MODE_A 0x0
#define FIR_MODE_B 0x1
#define FIR_MODE_C 0x2
#define FIR_MODE_R 0x3

/*
 * @struct acts_audio_dac
 * @brief DAC controller hardware register
 */
struct acts_audio_dac {
    volatile uint32_t digctl; /* DAC digital and control */
    volatile uint32_t fifoctl; /* DAC FIFO control */
    volatile uint32_t stat; /* DAC state */
    volatile uint32_t fifo0_dat; /* DAC FIFO0 data */
    volatile uint32_t pcm_buf_ctl; /* PCM buffer control */
    volatile uint32_t pcm_buf_stat; /* PCM buffer state */
    volatile uint32_t pcm_buf_thres_he; /* PCM buffer half-empty threshold */
    volatile uint32_t pcm_buf_thres_hf; /* PCM buffer half-full threshold */
    volatile uint32_t sdm_reset_ctl; /* SDM reset control */
    volatile uint32_t auto_mute_ctl; /* Auto mute control */
    volatile uint32_t vol_lch; /* volume left channel control */
    volatile uint32_t vol_rch; /* volume right channel control */
    volatile uint32_t pcm_buf_cnt; /* PCM buffer counter */
    volatile uint32_t anactl0; /* DAC analog control register 0 */
    volatile uint32_t anactl1; /* DAC analog control register 1 */
    volatile uint32_t anactl2; /* DAC analog control register 2 */
    volatile uint32_t bias; /* DAC bias control */
    volatile uint32_t sdm_samples_cnt; /* SDM samples counter */
    volatile uint32_t sdm_samples_num; /* SDM sample number */
    volatile uint32_t hw_trigger_dac_ctl; /* HW IRQ trigger DAC control */
    volatile uint32_t dc_ref_dat;/*dac reference data*/
    volatile uint32_t dac_osctl;/*dac offset register*/
};

struct phy_dac_channel {
    uint32_t fifo_cnt; /* DAC FIFO hardware counter max value is 0xFFFF */
    uint32_t fifo_cnt_timestamp; /* Record the timestamp of DAC FIFO counter overflow irq */
    int (*callback)(void *cb_data, u32_t reason); /* PCM Buffer IRQs callback */
    void *cb_data; /* callback user data */
};

#ifdef CONFIG_CFG_DRV
/**
 * struct phy_dac_external_config
 * @brief The DAC external configuration which generated by configuration tool
 */
struct phy_dac_external_config {
    cfg_uint8 Out_Mode; /* CFG_TYPE_AUDIO_OUT_MODE */
    cfg_uint32 DAC_Bias_Setting; /* DAC bias setting */
    cfg_uint8 Keep_DA_Enabled_When_Play_Pause; /* always enable DAC analog  */
    CFG_Type_Extern_PA_Control Extern_PA_Control[2]; /* GPIO pins to control external PA */
    cfg_uint8 AntiPOP_Process_Disable; /* forbidden antipop process */
    cfg_uint8 Enable_large_current_protect; /* enable large current protect */
    cfg_uint8 Pa_Vol; /* PA gain selection */
};
#endif

/**
 * struct phy_dac_drv_data
 * @brief The software related data that used by physical dac driver.
 */
struct phy_dac_drv_data {
    struct phy_dac_channel ch[DAC_CHANNEL_NUM_MAX]; /* dac channels infomation */
    uint32_t sdm_cnt; /* SDM samples counter */
    uint32_t sdm_cnt_timestamp; /* Record the timestamp of SDM counter by overflow irq */
    uint8_t sample_rate; /* The sample rate setting refer to enum audio_sr_sel_e */
    uint8_t lr_sel; /* left and right channel selection to enable, refer to enum a_lr_chl_e */
    uint8_t layout; /* DAC hardware layout */

#ifdef CONFIG_CFG_DRV
    struct phy_dac_external_config external_config; /* DAC external configuration */
#endif

    atomic_t refcount; /* DAC resources reference counter */

    uint8_t ch_fifo0_start : 1; /* The fifo0 channel start indicator */
    uint8_t ch_fifo1_start : 1; /* The fifo1 channel start indicator */
    uint8_t vol_set_mute : 1; /* The flag of the volume setting less than #VOL_MUTE_MIN_DB event*/
    uint8_t is_anc_enable : 1; /* If 1 to indicate that ANC has enabled */
	uint8_t audio_pll_index : 1; /* The index of audio pll */
    void (*dsp_audio_set_param)(uint8_t id, uint32_t param1, uint32_t param2);
};

/**
 * union phy_dac_features
 * @brief The infomation from DTS to control the DAC features to enable or nor.
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t layout : 2; /* DAC working layout(0: single-end non-direct; 1: single-end direct(VRO); 2 differencial) */
        uint32_t dac_lr_mix : 1; /* DAC left and right channels MIX */
        uint32_t noise_detect_mute : 1; /* noise detect mute */
        uint32_t automute : 1; /* auto-mute */
        uint32_t loopback : 1; /* ADC => DAC loopback */
        uint32_t left_mute : 1; /* DAC left mute */
        uint32_t right_mute : 1; /* DAC left mute */
        uint32_t pa_vol : 3; /* DAC PA gain config */
        uint32_t am_irq : 1; /* if 1 to enable auto mute irq */
    } v;
} phy_dac_features;

/**
 * struct phy_dac_config_data
 * @brief The hardware related data that used by physical dac driver.
 */
struct phy_dac_config_data {
    uint32_t reg_base; /* DAC controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
    struct audio_dma_dt dma_fifo1; /* DMA resource for FIFO1 */
    uint8_t clk_id; /* DAC devclk id */
    uint8_t rst_id; /* DAC reset id */
    void (*irq_config)(void); /* IRQ configuration function */
    phy_dac_features features; /* DAC features */
};

/*
 * enum a_dac_fifo_e
 * @brief DAC fifo index selection
 */
typedef enum {
    DAC_FIFO_0 = 0,
    DAC_FIFO_1
} a_dac_fifo_e;

/*
 * enum a_dac_sr_e
 * @brief DAC over sample rate
 */
typedef enum {
    DAC_SR_8k = 0,
    DAC_SR_11k,//11.25k
    DAC_SR_12K,
    DAC_SR_16K,
    DAC_SR_22k,//22.05k
    DAC_SR_24K, 
    DAC_SR_32K, 
    DAC_SR_44k, //44.1k
    DAC_SR_48k, 
    DAC_SR_88k, //88.2k
    DAC_SR_96k, 
} a_dac_sr_e;

/*
 * enum a_layout_e
 * @brief The DAC working layout
 */
typedef enum {
    SINGLE_END_MODE = 0,
    SINGLE_END_VOR_MODE,
    DIFFERENTIAL_MODE
} a_layout_e;

/* @brief get the base address of DAC register */
static inline struct acts_audio_dac *get_dac_reg_base(struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    return (struct acts_audio_dac *)cfg->reg_base;
}

/* @brief dump dac controller register */
static void dac_dump_register(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    LOG_INF("** dac contoller regster **");
    LOG_INF("             BASE: %08x", (uint32_t)dac_reg);
    LOG_INF("       DAC_DIGCTL: %08x", dac_reg->digctl);
    LOG_INF("      DAC_FIFOCTL: %08x", dac_reg->fifoctl);
    LOG_INF("         DAC_STAT: %08x", dac_reg->stat);
    LOG_INF("        FIFO0_DAT: %08x", dac_reg->fifo0_dat);
    LOG_INF("      PCM_BUF_CTL: %08x", dac_reg->pcm_buf_ctl);
    LOG_INF("     PCM_BUF_STAT: %08x", dac_reg->pcm_buf_stat);
    LOG_INF(" PCM_BUF_THRES_HE: %08x", dac_reg->pcm_buf_thres_he);
    LOG_INF(" PCM_BUF_THRES_HF: %08x", dac_reg->pcm_buf_thres_hf);
    LOG_INF("    SDM_RESET_CTL: %08x", dac_reg->sdm_reset_ctl);
    LOG_INF("    AUTO_MUTE_CTL: %08x", dac_reg->auto_mute_ctl);
    LOG_INF("          VOL_LCH: %08x", dac_reg->vol_lch);
    LOG_INF("          VOL_RCH: %08x", dac_reg->vol_rch);
    LOG_INF("      PCM_BUF_CNT: %08x", dac_reg->pcm_buf_cnt);
    LOG_INF("      DAC_ANALOG0: %08x", dac_reg->anactl0);
    LOG_INF("      DAC_ANALOG1: %08x", dac_reg->anactl1);
    LOG_INF("      DAC_ANALOG2: %08x", dac_reg->anactl2);
    LOG_INF("         DAC_BIAS: %08x", dac_reg->bias);
    LOG_INF("  SDM_SAMPLES_CNT: %08x", dac_reg->sdm_samples_cnt);
    LOG_INF("  SDM_SAMPLES_NUM: %08x", dac_reg->sdm_samples_num);
    LOG_INF("   HW_TRIGGER_CTL: %08x", dac_reg->hw_trigger_dac_ctl);
    LOG_INF("    AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF("    CMU_DEVCLKEN1: %08x", sys_read32(CMU_DEVCLKEN1));
    LOG_INF("       CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
    LOG_INF("         DMA7_CTL: %08x", sys_read32(0x4001c800));
    LOG_INF("       DMA7_START: %08x", sys_read32(0x4001c804));
    LOG_INF("       DMA7_sadd0: %08x", sys_read32(0x4001c808));
    LOG_INF("       DMA7_sadd1: %08x", sys_read32(0x4001c80c));
    LOG_INF("       DMA7_dadd0: %08x", sys_read32(0x4001c810));
    LOG_INF("       DMA7_dadd1: %08x", sys_read32(0x4001c814));
    LOG_INF("          DMA7_bc: %08x", sys_read32(0x4001c818));
    LOG_INF("          DMA7_rc: %08x", sys_read32(0x4001c81c));
    LOG_INF("         DEBUGSEL: %08x", sys_read32(DEBUGSEL));
    LOG_INF("         DEBUGOE0: %08x", sys_read32(DEBUGOE0));
    LOG_INF("         DEBUGOE1: %08x", sys_read32(DEBUGOE1));
}

/* @brief disable DAC FIFO by specified FIFO index */
static void __dac_fifo_disable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    /**
      * CMU_DACCLK_DACFIFO0CLKEN/CMU_DACCLK_DACFIFO1CLKEN is a clock gate for DAC FIFO read/write for power consumption.
      * When to access DAC_VOL or PA_VOL, shall enable those bits.
      */
    if (DAC_FIFO_0 == idx) {
        dac_reg->fifoctl &= ~(DAC_FIFOCTL_DAF0RT | DAC_FIFOCTL_DAF0EDE);
        /* disable DAC FIFO0 to access clock */
        sys_write32(sys_read32(CMU_DACCLK) & ~CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);
    } else
         LOG_ERR("invalid fifo sel");
}

/* @brief enable DAC FIFO0/FIFO1 */
static int __dac_fifo_enable(struct device *dev, audio_fifouse_sel_e sel,
                            audio_dma_width_e wd, uint8_t drq_level,
                            uint8_t fifo_vol, a_dac_fifo_e idx, bool fifo1_mix_en)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;

    if (drq_level > DAC_FIFO_MAX_DRQ_LEVEL)
        drq_level = DAC_FIFO_MAX_DRQ_LEVEL;

    if (fifo_vol > DAC_FIFO_MAX_VOL_LEVEL)
        fifo_vol = DAC_FIFO_MAX_VOL_LEVEL;

    if (FIFO_SEL_ASRC == sel) {
        LOG_ERR("invalid fifo sel %d", sel);
        return -EINVAL;
    }

    if (DAC_FIFO_0 == idx) {
        reg &= ~0xFFFF; /* clear all FIFO0 fields */

        if (FIFO_SEL_CPU == sel)
            reg |= DAC_FIFOCTL_DAF0EIE; /* enable irq */
        else if (FIFO_SEL_DMA == sel)
            reg |= DAC_FIFOCTL_DAF0EDE; /* enable drq */

        reg |= DAC_FIFOCTL_DAF0IS(sel);

        if (DMA_WIDTH_16BITS == wd)
            reg |= DAC_FIFOCTL_DACFIFO0_DMAWIDTH;

        reg |= DAC_FIFOCTL_DRQ0_LEVEL(drq_level);
        // reg |= DAC_FIFOCTL_FIFO0_VOL(fifo_vol);

        reg |= DAC_FIFOCTL_DAF0RT;

        dac_reg->fifoctl = reg;

        /* DAC FIFO0 MIX to DAC enable */
        dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;

        sys_write32(sys_read32(CMU_DACCLK) | CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);

    } else
        LOG_ERR("invalid fifo sel %d", sel);

    return 0;
}
static bool __dac_fifosrc_is_dsp(struct device *dev)
{
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
	uint32_t reg = dac_reg->fifoctl;

	if ((reg & DAC_FIFOCTL_DAF0IS_MASK) == (FIFO_SEL_DSP << DAC_FIFOCTL_DAF0IS_SHIFT)) {
		return true;
	} else {
		return false;
	}
}

/* @brief update DAC FIFO0/FIFO1 src */
static int __dac_fifo_update_src(struct device *dev, dac_fifosrc_setting_t *fifosrc)
{
	struct phy_dac_drv_data *data = dev->data;
	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
	uint32_t reg = dac_reg->fifoctl;
	audio_fifouse_sel_e sel = (fifosrc->fifo_from_dsp)? (FIFO_SEL_DSP): (FIFO_SEL_DMA);
	a_dac_fifo_e idx = fifosrc->fifo_idx;
    LOG_INF("sel:%d, idx:%d\n", sel, idx);
	data->dsp_audio_set_param = fifosrc->dsp_audio_set_param;

	if (DAC_FIFO_0 == idx) {
		reg &= ~DAC_FIFOCTL_DAF0IS_MASK;
		reg |= DAC_FIFOCTL_DAF0IS(sel);

		if (sel == FIFO_SEL_DSP) {
			reg &= ~DAC_FIFOCTL_DAF0EIE; /* disable irq */
			reg &= ~DAC_FIFOCTL_DAF0EDE; /* disable drq */
            reg &= ~DAC_FIFOCTL_DAF0_DSPDMA_EN; /* disable DSP_DMA drq*/
		} else if (FIFO_SEL_CPU == sel){
			reg |= DAC_FIFOCTL_DAF0EIE; /* enable irq */
        } else if (FIFO_SEL_DMA == sel){
			reg |= DAC_FIFOCTL_DAF0EDE; /* enable drq */
        }else if (FIFO_SEL_DSP_DMA == sel){
            reg &= ~DAC_FIFOCTL_DAF0EIE; /* disable irq */
			reg &= ~DAC_FIFOCTL_DAF0EDE; /* disable drq */
        }

	} else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
	}

	dac_reg->fifoctl = reg;

	if (sel == FIFO_SEL_DSP) {
		irq_disable(IRQ_ID_DACFIFO);
		irq_disable(IRQ_ID_DAC);
	} else {
		irq_enable(IRQ_ID_DACFIFO);
		irq_enable(IRQ_ID_DAC);
	}

	return 0;
}

/*
 * @brief Check and wait the DAC FIFO is empty or not
 */
static void __wait_dac_fifo_empty(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        while (((dac_reg->stat & DAC_STAT_DAF0S_MASK) >> DAC_STAT_DAF0S_SHIFT)
                != DAC_FIFO_MAX_LEVEL) {
            ;
        }
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}

/* @brief check if the specified FIFO is working or not */
static bool __is_dac_fifo_working(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        if (dac_reg->fifoctl & DAC_FIFOCTL_DAF0RT)
            return true;
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
        return false;
    }

    return false;
}

/* @brief get the available samples to fill into PCM buffer */
static uint32_t __get_pcmbuf_avail_length(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t key, avail;

    key = irq_lock();
    avail = (dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) >> PCM_BUF_STAT_PCMBS_SHIFT;
    irq_unlock(key);

    LOG_DBG("PCMBUF free space 0x%x samples", avail);

    return avail;
}

/* @brief check if dac fifo empty */
static bool __is_dac_fifo_empty(struct device *dev, a_dac_fifo_e idx)
{
    // struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    // uint32_t fifo_status = 0;

    if (DAC_FIFO_0 == idx) {
        /* DAC FIFO0 connects with PCMBUF  */
        if (DAC_PCMBUF_MAX_CNT == __get_pcmbuf_avail_length(dev))
            return true;
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
        return false;
    }
    return false;
}

/* @brief check if all DAC FIFO resources are free */
static bool __is_dac_fifo_all_free(struct device *dev, bool check_mix)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (check_mix) {
        if (dac_reg->digctl & DAC_DIGCTL_DAF0M2DAEN)
            return false;
    } else {
        if (__is_dac_fifo_working(dev, DAC_FIFO_0))
            return false;
    }

    return true;
}

/* @brief check if there is error happened in given fifo index */
static bool __check_dac_fifo_error(struct device *dev, a_dac_fifo_e idx)
{
    // struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    /* only in case of SPDIF use DAC_FIFO1  */
    if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
        return false;
    }

    return false;
}

/* @brief clear fifo error status */
static void __dac_clear_fifo_error(struct device *dev, a_dac_fifo_e idx)
{
    // struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}


/* @brief enable the FIFO sample counter function and by default to enable overflow irq */
static void __dac_fifo_counter_enable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;
        /* By default to enbale pcm buf counter overflow IRQ */
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;
        /* clear sample counter irq pending */
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
    } else if (DAC_FIFO_1 == idx) {
         LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}

/* @brief disable the FIFO sample counter function */
static void __dac_fifo_counter_disable(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);
    } else if (DAC_FIFO_1 == idx) {
         LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}

/* @brief reset the FIFO sample counter function */
static void __dac_fifo_counter_reset(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_cnt &= ~(PCM_BUF_CNT_EN | PCM_BUF_CNT_IE);
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_EN;
        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IE;
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}

/* @brief enable the DAC SDM sample counter function */
static void __dac_sdm_counter_enable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_EN;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IE;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;
}

/* @brief disable DAC SDM sample counter function */
static void __dac_sdm_counter_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt &= ~(SDM_SAMPLES_CNT_EN | SDM_SAMPLES_CNT_IE);
}

/* @brief reset the DAC SDM sample counter function */
static void __dac_sdm_counter_reset(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->sdm_samples_cnt &= ~(SDM_SAMPLES_CNT_EN | SDM_SAMPLES_CNT_IE);
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_EN;
    dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IE;
}

/* @brief read the DAC SDM sample counter */
static uint32_t __dac_read_sdm_counter(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    val = dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_MASK;

    return val;
}

/* @brief read the DAC SDM sample stable counter */
static uint32_t __dac_read_sdm_stable_counter(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    val = dac_reg->sdm_samples_num & SDM_SAMPLES_NUM_CNT_MASK;

    return val;
}

/* @brief set the DAC FIFO DRQ level */
static int __dac_fifo_drq_level_set(struct device *dev, a_dac_fifo_e idx, uint8_t level)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;

    if (level > DAC_FIFO_MAX_DRQ_LEVEL)
        return -EINVAL;

    if (DAC_FIFO_0 == idx) {
        reg &= ~DAC_FIFOCTL_DRQ0_LEVEL_MASK;
        reg |= DAC_FIFOCTL_DRQ0_LEVEL(level);
    } else if (DAC_FIFO_1 == idx) {
         LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    } else {
        return -EINVAL;
    }

    dac_reg->fifoctl = reg;

    return 0;
}

/* @brief get the DAC FIFO DRQ level */
static int __dac_fifo_drq_level_get(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->fifoctl;
    int level = 0;

    if (DAC_FIFO_0 == idx) {
        level = (reg & DAC_FIFOCTL_DRQ0_LEVEL_MASK) >> DAC_FIFOCTL_DRQ0_LEVEL_SHIFT;
    } else if (DAC_FIFO_1 == idx) {
         LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    } else {
        level = -EINVAL;
    }

    return level;
}

/* @brief read the FIFO sample counter by specified FIFO index */
static uint32_t __dac_read_fifo_counter(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t val = 0;

    if (DAC_FIFO_0 == idx) {
        val = dac_reg->pcm_buf_cnt & PCM_BUF_CNT_CNT_MASK;
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }

    return val;
}

/* @brief PCM BUF configuration */
static int __dac_pcmbuf_config(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->pcm_buf_ctl;

#if (DT_INST_PROP(0, pcmbuf_he_thres) >= DAC_PCMBUF_MAX_CNT)
#error "Error on PCMBUF HE threshold setting"
#endif

#if (DT_INST_PROP(0, pcmbuf_hf_thres) >= DAC_PCMBUF_MAX_CNT)
#error "Error on PCMBUF HF threshold setting"
#endif

    reg &= ~PCM_BUF_CTL_IRQ_MASK;

    /* By default to enable PCMBUF half empty IRQs */
    reg |= DAC_PCMBUF_DEFAULT_IRQ;

    dac_reg->pcm_buf_ctl = reg;

    dac_reg->pcm_buf_thres_he = CONFIG_AUDIO_DAC_0_PCMBUF_HE_THRES;
    dac_reg->pcm_buf_thres_hf = CONFIG_AUDIO_DAC_0_PCMBUF_HF_THRES;

    /* Clean all pcm buf irqs pending */
    dac_reg->pcm_buf_stat |= PCM_BUF_STAT_IRQ_MASK;

    LOG_DBG("ctl:0x%x, thres_he:0x%x thres_hf:0x%x",
                dac_reg->pcm_buf_ctl, dac_reg->pcm_buf_thres_he,
                dac_reg->pcm_buf_thres_hf);

    return 0;
}

static int __dac_pcmbuf_threshold_update(struct device *dev, dac_threshold_setting_t *thres)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (!thres)
        return -EINVAL;

    if ((thres->hf_thres >= DAC_PCMBUF_MAX_CNT)
        || (thres->he_thres > thres->hf_thres)) {
        LOG_ERR("Invalid threshold hf:%d he:%d",
            thres->hf_thres, thres->he_thres);
        return -ENOEXEC;
    }

    dac_reg->pcm_buf_thres_he = thres->he_thres;
    dac_reg->pcm_buf_thres_hf = thres->hf_thres;

    LOG_INF("new dac threshold => he:0x%x hf:0x%x",
        dac_reg->pcm_buf_thres_he, dac_reg->pcm_buf_thres_hf);

    return 0;
}

/* @brief set the external trigger source for DAC digital start */
static int __dac_external_trigger_enable(struct device *dev, uint8_t trigger_src)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (trigger_src > 7) {
        LOG_ERR("Invalid DAC trigger source %d", trigger_src);
        return -EINVAL;
    }

    dac_reg->hw_trigger_dac_ctl &= ~ HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SRL_MASK;
    dac_reg->hw_trigger_dac_ctl |= HW_TRIGGER_DAC_CTL_TRIGGER_SRC_SRL(trigger_src);

    dac_reg->hw_trigger_dac_ctl |= HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN;

    LOG_INF("set DAC external trigger_src:%d", trigger_src);

    return 0;
}

/* @breif control the DAC functions that can be triggered by external signals  */
static int __dac_external_trigger_control(struct device *dev, dac_ext_trigger_ctl_t *trigger_ctl)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->hw_trigger_dac_ctl;
    bool valid = false;

    if (!trigger_ctl) {
        LOG_ERR("Invalid parameter");
        return -EINVAL;
    }
    LOG_INF("extern trigger ctl:0x%x",trigger_ctl->trigger_ctl);
    if (trigger_ctl->t.sdm_cnt_trigger_en) {
        /* disable SDM CNT until external IRQ to trigger enable */
        if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_EN)
            __dac_sdm_counter_disable(dev);

        reg |= HW_TRIGGER_DAC_CTL_INT_TO_SDMCNT_EN;

        valid = true;
        LOG_INF("enable external trigger DAC SDM_CNT enable");
    }

    if (trigger_ctl->t.sdm_cnt_lock_en) {
        reg |= HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT;

        valid = true;
        LOG_INF("enable external trigger DAC lock SDM_CNT");
    }

    if (trigger_ctl->t.dac_fifo_trigger_en) {
        /**
          * FIXME: HW ISSUE
          * DMA always work when DRQ enable regardless of FIFO enable.
          */
#if 0
        if (dac_reg->fifoctl & DAC_FIFOCTL_DAF0RT) {
            dac_reg->fifoctl &= ~(DAC_FIFOCTL_DAF0RT);
        }

        reg |= HW_TRIGGER_DAC_CTL_INT_TO_DACFIFO_EN;

        valid = true;
        LOG_INF("enable external trigger DAC FIFO enable");
#endif
    }

    if (trigger_ctl->t.dac_digital_trigger_en) {
        /* disable DAC digital until external IRQ to trigger start */
        if (dac_reg->digctl & DAC_DIGCTL_DDEN)
            dac_reg->digctl &= ~DAC_DIGCTL_DDEN;

        if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_EN)
            __dac_sdm_counter_reset(dev);

        reg |= HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN;

        valid = true;
        LOG_INF("enable external trigger DAC digital start");
    }

    if (valid)
        dac_reg->hw_trigger_dac_ctl = reg;

    return 0;
}

/* @brief disable the external irq signal to start DAC digital function */
static void __dac_external_trigger_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT)
        dac_reg->hw_trigger_dac_ctl &= ~HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT;

    if (dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN)
        dac_reg->hw_trigger_dac_ctl &= ~HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN;
}

/* @brief force DAC digital module to start */
static void __dac_digital_force_start(struct device *dev, dac_ext_trigger_ctl_t *trigger_ctl)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (trigger_ctl->t.dac_fifo_trigger_en) {
        dac_reg->fifoctl |= DAC_FIFOCTL_DAF0RT;
        LOG_INF("force enable DAC FIFO");
    }

    if (trigger_ctl->t.dac_digital_trigger_en) {
        dac_reg->digctl |= DAC_DIGCTL_DDEN;
        LOG_INF("force start DAC digital");
    }

    if (trigger_ctl->t.sdm_cnt_trigger_en) {
        __dac_sdm_counter_reset(dev);
        LOG_INF("force start DAC SDM_CNT");
    }
}

/* @brief enable DAC mono mode */
static void __dac_digital_enable_mono(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    dac_reg->digctl |= DAC_DIGCTL_DACHNUM;
}

/* @brief enable DAC digital function */
static int __dac_digital_enable(struct device *dev, a_dac_sr_e sr,
            audio_ch_mode_e type, uint8_t channel_type)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    // const struct phy_dac_config_data *cfg = dev->config;

    uint32_t reg = dac_reg->digctl;

    /* clear interpolation/OSR/digital_en etc. */
    reg &= ~(DAC_DIGCTL_DDEN | DAC_DIGCTL_ENDITH 
            | DAC_DIGCTL_DACHNUM | DAC_DIGCTL_SR_MASK
            | DAC_DIGCTL_FIR_MODE_MASK);

    if ((STEREO_MODE != type) && (MONO_MODE != type))
        return -EINVAL;
    reg |= DAC_DIGCTL_SR(sr);
    /* select fir mode*/
    switch (sr)
    {
    case DAC_SR_11k:
    case DAC_SR_22k:
    case DAC_SR_44k:
    case DAC_SR_88k:
      reg |= DAC_DIGCTL_FIR_MODE(FIR_MODE_B);
      break;
    case DAC_SR_16K:
      reg |= DAC_DIGCTL_FIR_MODE(FIR_MODE_C);
    default:
      reg |= DAC_DIGCTL_FIR_MODE(FIR_MODE_A);
      break;
    }
    /* leopard dac only support mono*/
    reg |= DAC_DIGCTL_DACHNUM;

    /* digital and dith enable */
    reg |= (DAC_DIGCTL_DDEN | DAC_DIGCTL_ENDITH);

    dac_reg->digctl = reg;

    /* disable left/right channel volume soft step function */
    dac_reg->vol_lch &= ~VOL_LCH_SOFT_CFG_DEFAULT;
    dac_reg->vol_rch &= ~VOL_RCH_SOFT_CFG_DEFAULT;

    return 0;
}

/* @brief disable digital fifo usage */
static void __dac_digital_disable_fifo(struct device *dev, a_dac_fifo_e idx)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    if (DAC_FIFO_0 == idx) {
        dac_reg->digctl &= ~DAC_DIGCTL_DAF0M2DAEN; /* disable DAC_FIFO0 MIX to PCMBUF */
    } else if (DAC_FIFO_1 == idx) {
        LOG_ERR("%s dac_fifo err :leopard no fifo_1\n",__func__);
    }
}

/* @brief disable DAC digital function */
static void __dac_digital_disable(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    const struct phy_dac_config_data *cfg = dev->config;

    /* disable SDM reset function */
    if (PHY_DEV_FEATURE(noise_detect_mute)) {
        if (dac_reg->sdm_reset_ctl & SDM_RESET_CTL_SDMREEN)
            dac_reg->sdm_reset_ctl &= ~SDM_RESET_CTL_SDMREEN;
    }

    /* disable external irq signal to start DAC */
    if (dac_reg->hw_trigger_dac_ctl & HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN) {
        dac_reg->hw_trigger_dac_ctl &= ~(HW_TRIGGER_DAC_CTL_INT_TO_DAC_EN
            | HW_TRIGGER_DAC_CTL_INT_TO_SDM_CNT);
    }

    dac_reg->digctl &= ~(DAC_DIGCTL_ENDITH | DAC_DIGCTL_DDEN);
}

/* @brief check if the DAC digital function is working */
static bool __dac_is_digital_working(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (dac_reg->digctl & DAC_DIGCTL_DDEN)
        return true;

    return false;
}

/* @brief DAC L/R channel volume setting */
static void __dac_volume_set(struct device *dev, uint8_t lr_sel, uint8_t left_v, uint8_t right_v)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg_l, reg_l_old, reg_r, reg_r_old;

    /* DAC left channel volume setting */
    if (lr_sel & LEFT_CHANNEL_SEL) {
        reg_l_old = reg_l = dac_reg->vol_lch;
        reg_l &= ~VOL_LCH_VOLL_MASK;
        reg_l |= VOL_LCH_VOLL(left_v);
        dac_reg->vol_lch = reg_l;
        LOG_INF("left volume: 0x%x => 0x%x", reg_l_old & 0xFF, left_v);
    }

    if (lr_sel & RIGHT_CHANNEL_SEL) {
        reg_r_old = reg_r = dac_reg->vol_rch;
        reg_r &= ~VOL_RCH_VOLR_MASK;
        reg_r |= VOL_RCH_VOLR(right_v);
        dac_reg->vol_rch = reg_r;
        LOG_INF("right volume: 0x%x => 0x%x", reg_r_old & 0xFF, right_v);
    }
}

/* @brief get the current dac L/R volume setting */
static uint8_t __dac_volume_get(struct device *dev, uint8_t lr_sel)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint8_t vol;

    if (lr_sel & LEFT_CHANNEL_SEL)
        vol = dac_reg->vol_lch & VOL_LCH_VOLL_MASK;
    else
        vol = dac_reg->vol_rch & VOL_RCH_VOLR_MASK;

    return vol;
}

/* @brief DAC SDM(noise detect mute) configuration */
static void __dac_sdm_mute_cfg(struct device *dev, uint8_t sr)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->sdm_reset_ctl;
    uint16_t sdm_cnt = CONFIG_AUDIO_DAC_0_SDM_CNT;
    uint16_t sdm_thres = CONFIG_AUDIO_DAC_0_SDM_THRES;

    reg &= ~(SDM_RESET_CTL_SDMNDTH_MASK | SDM_RESET_CTL_SDMCNT_MASK);

    reg |= SDM_RESET_CTL_SDMCNT(sdm_cnt);
    reg |= SDM_RESET_CTL_SDMNDTH(sdm_thres);

    /* Reset SDM after has detected noise
     * NOTE: When sample rate are 8k/11k/12k/16k shall enable this bit
     */
    reg |= SDM_RESET_CTL_SDMREEN;
    dac_reg->sdm_reset_ctl = reg;
    LOG_INF("DAC SDM function enable");
}

/* @brief DAC automute function configuration */
static void __dac_automute_cfg(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    const struct phy_dac_config_data *cfg = dev->config;
    uint32_t reg = dac_reg->auto_mute_ctl;
    uint16_t am_cnt = CONFIG_AUDIO_DAC_0_AM_CNT;
    uint16_t am_thres = CONFIG_AUDIO_DAC_0_AM_THRES;

    reg &= ~(AUTO_MUTE_CTL_AMCNT_MASK | AUTO_MUTE_CTL_AMTH_MASK);

    reg |= AUTO_MUTE_CTL_AMCNT(am_cnt);
    reg |= AUTO_MUTE_CTL_AMTH(am_thres);

    if (PHY_DEV_FEATURE(am_irq))
        reg |= AUTO_MUTE_CTL_AM_IRQ_EN; /* enable auto mute IRQ */

    /* Auto mute enable */
    reg |= AUTO_MUTE_CTL_AMEN;

    dac_reg->auto_mute_ctl = reg;

    LOG_INF("DAC automute function enable");
}

/* @brief ADC loopback to DAC function configuration */
static void __dac_loopback_cfg(struct device *dev, uint8_t lr_sel, bool dac_lr_mix)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = dac_reg->digctl;

    /* Clear  L/R mix, ADCR loopback to DAC, ADCL loopback to DAC */
    reg &= ~(DAC_DIGCTL_ADC01MIX | DAC_DIGCTL_AD2DALPEN_R | DAC_DIGCTL_AD2DALPEN_L);

    /* ADC0 L to DAC L loopback */
    if (lr_sel & LEFT_CHANNEL_SEL)
        reg |= DAC_DIGCTL_AD2DALPEN_L;

    /* ADC1 R to DAC R loopback */
    if (lr_sel & RIGHT_CHANNEL_SEL)
        reg |= DAC_DIGCTL_AD2DALPEN_R;

    if (dac_lr_mix)
        reg |= DAC_DIGCTL_ADC01MIX;
    else
        reg &= ~DAC_DIGCTL_ADC01MIX;

    dac_reg->digctl = reg;

    LOG_INF("ADDA loopback(lr:%d mix:%d) enable", lr_sel, dac_lr_mix);
}

/* @brief dac left volume soft step setting and 'adj_cnt' is the same as 'sample_rate' */
static void __dac_volume_left_softstep(struct device *dev, uint8_t adj_cnt, bool irq_flag)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

    /* Clear soft step done IRQ pending */
    if (dac_reg->vol_lch & VOL_LCH_DONE_PD)
        dac_reg->vol_lch |= VOL_LCH_DONE_PD;

    reg = dac_reg->vol_lch;

    /* Clear all VOL_LCH exclude VOLL */
    reg &= ~0x3FFF00;
    reg |= VOL_LCH_ADJ_CNT(adj_cnt);

    /* to_cnt setting */
    if (DAC_VOL_TO_CNT_DEFAULT)
        reg |= VOL_LCH_TO_CNT;

    if (irq_flag)
        reg |= VOL_LCH_VOLL_IRQ_EN;
    else
        reg &= ~VOL_LCH_VOLL_IRQ_EN;

    reg |= VOL_LCH_SOFT_CFG_DEFAULT;

    dac_reg->vol_lch = reg;
}

/* @brief dac right volume soft step setting and 'adj_cnt' is the same as 'sample_rate' */
static void __dac_volume_right_softstep(struct device *dev, uint8_t adj_cnt, bool irq_flag)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg;

    /* Clear soft step done IRQ pending */
    if (dac_reg->vol_rch & VOL_RCH_DONE_PD)
        dac_reg->vol_rch |= VOL_RCH_DONE_PD;

    reg = dac_reg->vol_rch;

    /* Clear all VOL_RCH exclude VOLR */
    reg &= ~0x3FFF00;

    reg |= VOL_RCH_ADJ_CNT(adj_cnt);

    /* to_cnt setting */
    if (DAC_VOL_TO_CNT_DEFAULT)
        reg |= VOL_RCH_TO_CNT;

    if (irq_flag)
        reg |= VOL_RCH_VOLR_IRQ_EN;
    else
        reg &= ~VOL_RCH_VOLR_IRQ_EN;

    reg |= VOL_RCH_SOFT_CFG_DEFAULT;

    dac_reg->vol_rch = reg;
}

/* @brief DAC enable mute or disable mute control. */
static void __dac_mute_control(struct device *dev, bool mute_en)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    if (mute_en)
        dac_reg->anactl0 |= DAC_ANACTL0_ZERODT; /* DAC L/R channel will output zero data */
    else
        dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT;
}

#ifdef CONFIG_CFG_DRV
/* @brief Disable DAC analog by specified channels */
static int __dac_analog_disable(struct device *dev, uint8_t lr_sel)
{
    struct phy_dac_drv_data *data = dev->data;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    dac_reg->anactl0 &= ~DAC_ANACTL0_DAENL;
    return 0;
}
#endif

/* @brief DAC works in differencial layout */
static int __dac_analog_diff_cfg(struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = 0;
    // act_or(CMU_DEVCLKEN1, 1 << CMU_DEVCLKEN1_DACANACLKEN);
    reg |=  DAC_ANACTL0_BIASEN;//en all bias
    reg |=  DAC_ANACTL0_DAENL;	 //en dac
    reg |=  DAC_ANACTL0_PAEN;	 //en pa
    reg |=  DAC_ANACTL0_PAOSEN;	 //en pa output stage

    reg |= DAC_ANACTL0_PAVOL(PHY_DEV_FEATURE(pa_vol));//pa swing
    reg |= DAC_ANACTL0_DARSET;
    reg |=  DAC_ANACTL0_DFCEN;
    dac_reg->anactl0 = reg;
    dac_reg->anactl1 |= DAC_ANACTL1_ATPSW1;
    return 0;
}

#ifdef DAC_ANALOG_DEBUG_IN_ENABLE
static void __dac_analog_dbgi(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DEBUGSEL_DBGSE(DBGSE_DAC);
    sys_write32(reg, DEBUGSEL);

    /* debug GPIO input pin13 ~ pin22 */
    sys_write32(0x7fe000, DEBUGIE0);

    reg = dac_reg->digctl & ~DAC_DIGCTL_DADEN;
    reg |= DAC_DIGCTL_DADEN;

    dac_reg->digctl = reg;
}
#endif

#ifdef DAC_DIGITAL_DEBUG_OUT_ENABLE
static void __dac_digital_dbgo(struct device *dev, uint8_t lr_sel)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    uint32_t reg = sys_read32(DEBUGSEL) & ~DEBUGSEL_DBGSE_MASK;
    reg |= DEBUGSEL_DBGSE(DBGSE_DAC);
    sys_write32(reg, DEBUGSEL);

    /* debug GPIO output pin 14-21,24-27,32-33 */
    sys_write32(0x0f3fc000, DEBUGOE0);
    sys_write32(0x00000003, DEBUGOE1);
    reg = dac_reg->digctl & ~DAC_DIGCTL_DDDEN;

    reg |= DAC_DIGCTL_DDDEN;
    dac_reg->digctl = reg;
}
#endif

#ifdef CONFIG_CFG_DRV
/* @brief Enable PA/VRO output over load detection. */
static void dac_enable_pa_overload_detect(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    /* PA/VRO over load protection function needs enable DACANCCLK */
    acts_clock_peripheral_enable(CLOCK_ID_DACANACLK);

    dac_reg->anactl0 |= DAC_ANACTL0_OVDTEN;
}
#endif

/* @brief Power control(enable or disable) by DAC LDO */
static void dac_ldo_power_control(struct device *dev, bool enable)
{
    ARG_UNUSED(dev);

    uint32_t reg = sys_read32(ADC_REF_LDO_CTL_BASE);
    if (enable) {
        /** FIXME: HW issue
         * ADC LDO shall be enabled when use DAC individually, otherwise VREF_ADD will get low voltage.
         */
        adc_reset_control(true);
        acts_clock_peripheral_enable(CLOCK_ID_ADC);

        /* AULDO pull down current control */
        reg &= ~ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK;
        reg |= ADC_REF_LDO_CTL_AULDO_PD_CTL(2);

        /* VREF voltage divide res control */
        reg &= ~ADC_REF_LDO_CTL_VREF_RSEL_MASK;
        reg |= ADC_REF_LDO_CTL_VREF_RSEL(0);

        reg |= (ADC_REF_LDO_CTL_AULDO_EN(3) | ADC_REF_LDO_CTL_DALDO_EN(3));
        sys_write32(reg, ADC_REF_LDO_CTL_BASE);

        /* ADC/DAC VREF voltage enable */
        sys_write32(sys_read32(ADC_REF_LDO_CTL_BASE) \
                    | ADC_REF_LDO_CTL_VREF_EN, ADC_REF_LDO_CTL_BASE);

        if (!(reg & ADC_REF_LDO_CTL_VREF_EN)) {
            LOG_INF("DAC wait for capacitor charge full");

            sys_write32(sys_read32(ADC_REF_LDO_CTL_BASE) | ADC_REF_LDO_CTL_VREF_FU,
                        ADC_REF_LDO_CTL_BASE);

            if (!z_is_idle_thread_object(_current))
                k_sleep(K_MSEC(DAC_LDO_CAPACITOR_CHARGE_TIME_MS));
            else
                k_busy_wait(DAC_LDO_CAPACITOR_CHARGE_TIME_MS * 1000UL);

            /* disable LDO fast charge */
            sys_write32(sys_read32(ADC_REF_LDO_CTL_BASE) & ~ADC_REF_LDO_CTL_VREF_FU,
                        ADC_REF_LDO_CTL_BASE);
        }

        /* Wait for AULDO stable */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
        else
            k_busy_wait(1000);

        /* reduce AULDO static power consume */
        uint32_t reg1 = sys_read32(ADC_REF_LDO_CTL_BASE);
        reg1 &= ~ADC_REF_LDO_CTL_AULDO_PD_CTL_MASK;
        reg1 &= ~ADC_REF_LDO_CTL_VREF_RSEL_MASK;
        reg1 |= ADC_REF_LDO_CTL_VREF_RSEL(3);
        sys_write32(reg1, ADC_REF_LDO_CTL_BASE);
    } else {
        reg &= ~ADC_REF_LDO_CTL_DALDO_EN_MASK;

        uint8_t is_busy = 0;
        struct device *adc_dev = (struct device *)device_get_binding(CONFIG_AUDIO_ADC_0_NAME);
        if (!adc_dev)
            LOG_ERR("failed to bind adc device:%s", CONFIG_AUDIO_ADC_0_NAME);

        uint32_t key = irq_lock();

        /* check ADC is busy */
        phy_audio_control(adc_dev, PHY_CMD_IS_ADC_BUSY, &is_busy);

        if (is_busy)
            LOG_INF("ADC current is using");

        /* If ADC is idle to disable ADC LDO and VREF */
        if (adc_dev && !is_busy) {
            reg &= ~ADC_REF_LDO_CTL_AULDO_EN_MASK;
            reg &= ~ADC_REF_LDO_CTL_VREF_EN;
        }

        irq_unlock(key);

        sys_write32(reg, ADC_REF_LDO_CTL_BASE);
    }
}

/* @brief Translate the volume in db format to DAC hardware volume level. */
static uint16_t dac_volume_db_to_level(int32_t vol)
{
    uint32_t level = 0;
    if (vol < 0) {
        vol = -vol;
        level = VOL_DB_TO_INDEX(vol);
        if (level > VOL_LEVEL_0DB)
            level = 0;
        else
            level = VOL_LEVEL_0DB - level;
    } else {
        level = VOL_DB_TO_INDEX(vol);
        if (level > 0x40)
            level = 0xFF;
        else
            level = VOL_LEVEL_0DB + level;
    }
    return level;
}

/* @brief Translate the DAC hardware volume level to volume in db format. */
static int32_t dac_volume_level_to_db(uint16_t level)
{
    int32_t vol = 0;
    if (level < VOL_LEVEL_0DB) {
        level = VOL_LEVEL_0DB - level;
        vol = VOL_INDEX_TO_DB(level);
        vol = -vol;
    } else {
        level = level - VOL_LEVEL_0DB;
        vol = VOL_INDEX_TO_DB(level);
    }
    return vol;
}

/* @brief DAC left/right channel volume setting */
static int dac_volume_set(struct device *dev, int32_t left_vol, int32_t right_vol, uint8_t sr, int32_t fade)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;

    uint8_t lr_sel = 0;
    uint16_t cur_vol_level, l_vol_level, r_vol_level;

    /* check left channel not mute and volume is valid */
    if (!PHY_DEV_FEATURE(left_mute) && (left_vol != AOUT_VOLUME_INVALID))
        lr_sel |= LEFT_CHANNEL_SEL;

    /* check right channel not mute and volume is valid */
    if (!PHY_DEV_FEATURE(right_mute) && (right_vol != AOUT_VOLUME_INVALID))
        lr_sel |= RIGHT_CHANNEL_SEL;

    if ((left_vol <= VOL_MUTE_MIN_DB) || (right_vol <= VOL_MUTE_MIN_DB)) {
        LOG_INF("volume [%d, %d] less than mute level %d",
        left_vol, right_vol, VOL_MUTE_MIN_DB);
        __dac_mute_control(dev, true);
        data->vol_set_mute = 1;
    } else {
        /* disable mute when volume become normal */
        if (data->vol_set_mute) {
            __dac_mute_control(dev, false);
            data->vol_set_mute = 0;
        }
    }

    l_vol_level = dac_volume_db_to_level(left_vol);
    cur_vol_level = __dac_volume_get(dev, LEFT_CHANNEL_SEL);
    if (cur_vol_level == l_vol_level) {
        LOG_DBG("ignore same left volume:%d", cur_vol_level);
        lr_sel &= ~LEFT_CHANNEL_SEL;
    }

    r_vol_level = dac_volume_db_to_level(right_vol);
    cur_vol_level = __dac_volume_get(dev, RIGHT_CHANNEL_SEL);
    if (cur_vol_level == r_vol_level) {
        LOG_DBG("ignore same right volume:%d", cur_vol_level);
        lr_sel &= ~RIGHT_CHANNEL_SEL;
    }

    __dac_volume_set(dev, lr_sel, l_vol_level, r_vol_level);

	if (__dac_fifosrc_is_dsp(dev) && data->dsp_audio_set_param) {
		data->dsp_audio_set_param(DSP_AUDIO_SET_VOLUME, l_vol_level, r_vol_level);
	}
    LOG_INF("set volume {db:[%d, %d] level:[%x, %x]}",
            left_vol, right_vol, l_vol_level, r_vol_level);

    if (__get_pcmbuf_avail_length(dev) >= DAC_PCMBUF_MAX_CNT) {
        LOG_DBG("no data in pcmbuf can not enable soft step volume");
        return 0;
    }

    if(fade) {
        if (lr_sel & LEFT_CHANNEL_SEL)
            __dac_volume_left_softstep(dev, sr, false);

        if (lr_sel & RIGHT_CHANNEL_SEL)
            __dac_volume_right_softstep(dev, sr, false);
    }

    return 0;
}

/* @brief Configure the physical layout within DAC */
static int dac_physical_layout_cfg(struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    // struct phy_dac_drv_data *data = dev->data;
    int ret = -1;
    uint8_t  layout = PHY_DEV_FEATURE(layout);

    if(layout == DIFFERENTIAL_MODE )
    {
        ret = __dac_analog_diff_cfg(dev);
    } 
    else
    {
        LOG_ERR("leapard can't support sing mode");
        ret = -1;
    }

    return ret;
}

/* @brief Enable the features that supported by DAC */
static int dac_enable_features(struct device *dev, uint8_t sr)
{
    const struct phy_dac_config_data *cfg = dev->config;

    if (PHY_DEV_FEATURE(automute))
        __dac_automute_cfg(dev);

    if (PHY_DEV_FEATURE(noise_detect_mute))
        __dac_sdm_mute_cfg(dev, sr);

    if (PHY_DEV_FEATURE(loopback))
        __dac_loopback_cfg(dev, LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL, false);

    return dac_physical_layout_cfg(dev);
    return 0;
}

/* @brief DAC SR selection according to the sample rate */
static int dac_sample_rate_to_osr(struct device *dev, audio_sr_sel_e sample_rate)
{
    int sr = -1;

    ARG_UNUSED(dev);

    switch (sample_rate) {
        case SAMPLE_RATE_8KHZ:
            sr = DAC_SR_8k;
            break;
        case SAMPLE_RATE_11KHZ:
            sr = DAC_SR_11k;
            break;
        case SAMPLE_RATE_12KHZ:
            sr = DAC_SR_12K;
            break;
        case SAMPLE_RATE_16KHZ:
            sr = DAC_SR_16K;
            break;
        case SAMPLE_RATE_22KHZ:
            sr = DAC_SR_22k;
            break;
        case SAMPLE_RATE_24KHZ:
            sr = DAC_SR_24K;
            break;
        case SAMPLE_RATE_32KHZ:
            sr = DAC_SR_32K;
            break;
        case SAMPLE_RATE_44KHZ:
            sr = DAC_SR_44k;
            break;
        case SAMPLE_RATE_48KHZ:
            sr = DAC_SR_48k;
            break;
        case SAMPLE_RATE_64KHZ:
            sr = DAC_SR_11k;
            break;
        case SAMPLE_RATE_88KHZ:
            sr = DAC_SR_88k;
            break;
        case SAMPLE_RATE_96KHZ:
            sr = DAC_SR_96k;
            break;
        default:
            sr = DAC_SR_44k;
            break;
    }

    return sr;
}

/* @brief DAC sample rate config */
static int dac_sample_rate_set(struct device *dev, audio_sr_sel_e sr_khz)
{
	struct phy_dac_drv_data *data = dev->data;
    int ret;
    uint8_t div,fir_div, fir2x_div, cic_div,series,pll_index;
    uint32_t reg;

    ARG_UNUSED(dev);

    /* Get audio dac PLL setting  */
    ret = audio_get_pll_setting_dac(sr_khz, &div, &fir_div, &fir2x_div, &cic_div, &series);
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

    reg = sys_read32(CMU_DACCLK) & ~0x1FF;

    /*enable dac audio*/
    reg |= (CMU_DACCLK_DACSDMCLOCK | CMU_DACCLK_DACCICFIRCLOCK | CMU_DACCLK_DACFIFO0CLKEN);

    /* Select audio_clk_div*/
    reg |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
    /* set clk */
    reg &= (~CMU_DACCLK_DACFIR2XCLKDIV_MASK);
    reg &= (~CMU_DACCLK_DACCLKDIV_MASK);
    reg &= ~(1 <<CMU_DACCLK_DACFIRCLKDIV);
    reg &= ~(1 <<CMU_DACCLK_DACCICCLCKDIV);
    reg |=  (CMU_DACCLK_DACCLKDIV(div)| (fir_div << CMU_DACCLK_DACFIRCLKDIV)
        | CMU_DACCLK_DACFIR2XCLKDIV(fir2x_div) | (cic_div << CMU_DACCLK_DACCICCLCKDIV));

    reg |= CMU_DACCLK_DACHUMDIV;
    
	data->audio_pll_index = pll_index;

    sys_write32(reg, CMU_DACCLK);

    return 0;
}

/* @brief Get the sample rate from the DAC config */
static int dac_sample_rate_get(struct device *dev)
{
     uint8_t div,fir_div, fir2x_div, cic_div;
    uint32_t reg = sys_read32(CMU_DACCLK);

    ARG_UNUSED(dev);
    div = (reg & CMU_DACCLK_DACCLKDIV_MASK);
    fir_div = (reg & (1<<CMU_DACCLK_DACFIRCLKDIV)>>CMU_DACCLK_DACFIRCLKDIV);
    fir2x_div = (reg & CMU_DACCLK_DACFIR2XCLKDIV_MASK)>>CMU_DACCLK_DACFIR2XCLKDIV_SHIFT;
    cic_div = (reg & (1<<CMU_DACCLK_DACCICCLCKDIV)>>CMU_DACCLK_DACCICCLCKDIV);

    return audio_get_pll_sample_rate_dac(div, fir_div, fir2x_div, cic_div, 0);
}

/* @brief Get the AUDIO_PLL APS used by DAC */
static int dac_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC)) >> CMU_DACCLK_DACCLKSRC;

    return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by DAC */
static int dac_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_DACCLK);
    pll_index = (reg & (1 << CMU_DACCLK_DACCLKSRC)) >> CMU_DACCLK_DACCLKSRC;

    return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief Get the DAC DMA information */
static int dac_get_dma_info(struct device *dev, struct audio_out_dma_info *info)
{
    const struct phy_dac_config_data *cfg = dev->config;

    if (AOUT_FIFO_DAC0 == info->fifo_type) {
        info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
        info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
        info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
    }  else {
        return -ENOENT;
    }

    return 0;
}

#ifdef CONFIG_CFG_DRV
/* @brief DAC external PA control */
static int dac_external_pa_ctl(struct device *dev, uint8_t ctrl_func)
{
    struct phy_dac_drv_data *data = dev->data;
    uint8_t i, pa_func, enable;
    gpio_flags_t flags = GPIO_OUTPUT;
    const struct device *gpio_dev = NULL;

    if ((ctrl_func != EXTERNAL_PA_ENABLE)
        && (ctrl_func != EXTERNAL_PA_DISABLE)
        && (ctrl_func != EXTERNAL_PA_MUTE)
        && (ctrl_func != EXTERNAL_PA_UNMUTE)) {
        LOG_ERR("invalid external pa ctrl:%d", ctrl_func);
        return -EINVAL;
    }

    if (ctrl_func == EXTERNAL_PA_ENABLE) {
        pa_func = EXTERN_PA_ENABLE;
        enable = true;
    } else if (ctrl_func == EXTERNAL_PA_DISABLE) {
        pa_func = EXTERN_PA_ENABLE;
        enable = false;
    }  else if (ctrl_func == EXTERNAL_PA_MUTE) {
        pa_func = EXTERN_PA_MUTE;
        enable = true;
    } else {
        pa_func = EXTERN_PA_MUTE;
        pa_func = EXTERN_PA_MUTE;
        enable = false;
    }

    for (i = 0; i < ARRAY_SIZE(data->external_config.Extern_PA_Control); i++) {
        CFG_Type_Extern_PA_Control *cfg = &data->external_config.Extern_PA_Control[i];
        if (cfg->PA_Function == pa_func && cfg->GPIO_Pin != GPIO_NONE) {
            if (cfg->Pull_Up_Down != CFG_GPIO_PULL_NONE) {
                if (cfg->Pull_Up_Down == CFG_GPIO_PULL_DOWN)
                    flags |= GPIO_PULL_DOWN;
                else
                    flags |= GPIO_CTL_PULLUP;
            }

            gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(cfg->GPIO_Pin));

            if (!gpio_dev) {
                LOG_ERR("failed to bind GPIO(%d) device", cfg->GPIO_Pin);
                return -ENODEV;
            }

            gpio_pin_configure(gpio_dev, cfg->GPIO_Pin % 32, flags);

            if (enable)
                gpio_pin_set(gpio_dev, cfg->GPIO_Pin % 32, cfg->Active_Level);
            else
                gpio_pin_set(gpio_dev, cfg->GPIO_Pin % 32, !cfg->Active_Level);
        }
    }

    return 0;
}
#endif

/* @brief prepare dac runtime resources such as clock etc. */
static int phy_dac_prepare_enable(struct device *dev, aout_param_t *out_param)
{

    dac_setting_t *dac_setting = NULL;
    const struct phy_dac_config_data *cfg = dev->config;

    if ((!out_param) || (!out_param->dac_setting)
        || (!out_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }
    dac_setting = out_param->dac_setting;
    if(dac_setting== NULL)
    {
        LOG_ERR("dac_setting is NULL");
        return -EINVAL;
    }

    if (dac_setting->channel_mode != MONO_MODE) {
        LOG_ERR("Invalid channel mode %d", dac_setting->channel_mode);
        return -EINVAL;
    }

    if (!(out_param->channel_type & AUDIO_CHANNEL_DAC)) {
        LOG_ERR("Invalid channel type %d", out_param->channel_type);
        return -EINVAL;
    }

    if ((out_param->outfifo_type != AOUT_FIFO_DAC0)
        && (out_param->outfifo_type != AOUT_FIFO_DAC1)) {
        LOG_ERR("Invalid FIFO type %d", out_param->outfifo_type);
        return -EINVAL;
    }

    if ((out_param->outfifo_type == AOUT_FIFO_DAC1)
        && !__is_dac_fifo_working(dev, DAC_FIFO_0)) {
        LOG_ERR("DAC FIFO1 depends on DAC FIFO0 enabled");
        return -EPERM;
    }

    if (out_param->reload_setting) {
        LOG_ERR("DAC FIFO does not support reload mode");
        return -EINVAL;
    }

    /* Enable DAC clock gate */
    acts_clock_peripheral_enable(cfg->clk_id);

    /* DAC main clock source is alway 256FS */
    if (dac_sample_rate_set(dev, out_param->sample_rate)) {
        LOG_ERR("Failed to config sample rate %d", out_param->sample_rate);
        return -ESRCH;
    }

    return 0;
}

static int phy_dac_disable_pa(struct device *dev)
{
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

#ifdef CONFIG_CFG_DRV
    dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_DISABLE);
#endif

    dac_reg->anactl0 = 0;
    dac_reg->anactl1 = 0;
    dac_reg->anactl2 = 0;

    dac_ldo_power_control(dev, false);

    acts_clock_peripheral_disable(CMU_DACCLK);

    return 0;
}

/* @brief ADC BIAS setting for power saving */
static void dac_bias_setting(struct device *dev)
{
#ifdef CONFIG_CFG_DRV
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    dac_reg->bias = data->external_config.DAC_Bias_Setting;
#endif
}

/* @brief Wait the DAC FIFO empty */
static int dac_wait_fifo_empty(struct device *dev, a_dac_fifo_e idx, uint32_t timeout_ms)
{
    uint32_t start_time;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

    start_time = k_cycle_get_32();

    LOG_DBG("wait DAC FIFO empty start time:%d", start_time);

    /* disable PCMBUF irqs to avoid user continuously writing data */
    if (DAC_FIFO_0 == idx) {
        dac_reg->pcm_buf_ctl &= ~DAC_PCMBUF_DEFAULT_IRQ;
    }

    while (!__is_dac_fifo_empty(dev, idx)) {
        if (k_cyc_to_us_floor32(k_cycle_get_32() - start_time)
            >= (timeout_ms * 1000)) {
            LOG_ERR("wait dac fifo(%d) empty(0x%x) timeout",
                    idx, __get_pcmbuf_avail_length(dev));
            return -ETIMEDOUT;
        }

        /* PM works in IDLE thread and not allow to sleep */
        if (!z_is_idle_thread_object(_current))
            k_sleep(K_MSEC(1));
    }

    LOG_DBG("wait DAC FIFO empty end time:%d and total use %dus",
            k_cycle_get_32(), k_cycle_get_32() - start_time);

    return 0;
}

static int phy_dac_enable(struct device *dev, void *param)
{
    struct phy_dac_drv_data *data = dev->data;
    const struct phy_dac_config_data *cfg = dev->config;
    aout_param_t *out_param = (aout_param_t *)param;
    dac_setting_t *dac_setting = out_param->dac_setting;
    int ret;
    uint8_t fifo_idx;
    int8_t sr = DAC_SR_44k;
        /* reset DAC controller */
    acts_reset_peripheral(cfg->rst_id);
    soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, true);
    /* enable DAC LDO */
    dac_ldo_power_control(dev, true);

    ret = phy_dac_prepare_enable(dev, out_param);
    if (ret) {
        LOG_ERR("Failed to prepare enable dac err=%d", ret);
        acts_reset_peripheral_assert(cfg->rst_id);
        soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
        return ret;
    }

    fifo_idx = out_param->outfifo_type;

#if 0 /* Don't check fifo status for ANC */
    if (__is_dac_fifo_working(dev, fifo_idx)) {
        LOG_ERR("The DAC FIFO@%d now is using", out_param->outfifo_type);
        return -EACCES;
    }
#endif

    ret = __dac_fifo_enable(dev, FIFO_SEL_DMA,
                    (out_param->channel_width == CHANNEL_WIDTH_16BITS)
                    ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS,
                    DAC_FIFO_DRQ_LEVEL_DEFAULT,
                    DAC_FIFO_VOL_LEVEL_DEFAULT,
                    fifo_idx, true);
    if (ret)
    {
        acts_reset_peripheral_assert(cfg->rst_id);
        soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
        return ret;
    }

    __dac_pcmbuf_config(dev);

    sr = dac_sample_rate_to_osr(dev, out_param->sample_rate);
    if (sr < 0) {
        LOG_ERR("mapping sample rate:%d to osr error", out_param->sample_rate);
        ret = -EFAULT;
        goto err;
    }

    // ret_anc = anc_disable_for_interpolation_x3(dev, out_param->channel_type);

    ret = __dac_digital_enable(dev, sr,
            dac_setting->channel_mode, out_param->channel_type);
    if (ret) {
        LOG_ERR("Failed to enable DAC digital err=%d", ret);
        goto err;
    }

    // if (ret_anc == 1)
    //     anc_enable_for_interpolation_x3(dev);

    dac_bias_setting(dev);

    ret = dac_enable_features(dev, out_param->sample_rate);
    if (ret) {
        LOG_ERR("DAC enable features error %d", ret);
        goto err;
    }

    data->sample_rate = out_param->sample_rate;

    /* Record the PCM BUF data callback */
    data->ch[fifo_idx].callback = out_param->callback;
    data->ch[fifo_idx].cb_data = out_param->cb_data;

    LOG_DBG("DAC ch@%d register callback %p cb_data %p",
        fifo_idx, data->ch[fifo_idx].callback, data->ch[fifo_idx].cb_data);

    /* Clear FIFO ERROR */
    __dac_clear_fifo_error(dev, fifo_idx);
#ifdef DAC_ANALOG_DEBUG_IN_ENABLE
    __dac_analog_dbgi(dev);
#endif
#ifdef DAC_DIGITAL_DEBUG_OUT_ENABLE
    __dac_digital_dbgo(dev, DAC_DIGITAL_DEBUG_OUT_CHANNEL_SEL);
#endif
    ret = dac_volume_set(dev, dac_setting->volume.left_volume,
                            dac_setting->volume.right_volume,
                            out_param->sample_rate, 0);
    if (ret)
        goto err;
    uint32_t key = irq_lock();
    /* set channel start flag */
    if (DAC_FIFO_0 == fifo_idx)
        data->ch_fifo0_start = 1;
    else if (DAC_FIFO_1 == fifo_idx)
        data->ch_fifo1_start = 1;
    atomic_inc(&data->refcount);
    irq_unlock(key);
    return ret;

err:
    __dac_fifo_disable(dev, fifo_idx);
    acts_reset_peripheral_assert(cfg->rst_id);
    soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
    return ret;
}

// #ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
// #if (CONFIG_AUDIO_DAC_0_LAYOUT == 1)    //direct driver
// static void dac_single_end_on_antipop(struct device *dev)
// {
// 	int ramp_data = 0, max_pcm = 0x7FFFFF, min_pcm = -8388607, i;
// 	uint32_t ramp_step = 3000, dac_fifoctl;
// 	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

// 	/* enable DAC clock gate */
// 	acts_clock_peripheral_enable(CLOCK_ID_DAC);

// 	/* set sample rate 48K */
// 	dac_sample_rate_set(dev, SAMPLE_RATE_48KHZ);

// 	/* ldo enable */
// 	dac_ldo_power_control(dev, true);

// 	dac_fifoctl = dac_reg->fifoctl;

// 	/* enable DAC digital function */
// 	__dac_digital_enable(dev, DAC_OSR_64X, STEREO_MODE, AUDIO_CHANNEL_DAC);

// 	/* switch to old plan and set to 0 db */
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_PLAN;
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_CUR_MASK;

// 	/* 1.playback mute, all bias en, dac ana/dac dig/pa en, and set to diff mode */
// 	sys_write32(sys_read32(CMU_DACCLK) | CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);

// 	/* DAC FIFO0 MIX to DAC enable and disable DAC FIFO1 MIX */
// 	dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;
// 	// dac_reg->digctl &= ~DAC_DIGCTL_DAF1M2DAEN;

// 	/* enable PA DFC option */
// 	dac_reg->anactl0 |= DAC_ANACTL0_DFCEN;

// 	/* enable DAC + PA bias */
// 	dac_reg->anactl0 |= DAC_ANACTL0_BIASEN;

// 	/* disable LN/LP play back mute */
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);

// 	/* disable DAC R play back mute */
// 	dac_reg->anactl1 &= ~DAC_ANACTL1_DPBMR;

// 	/* set differential mode */
// 	dac_reg->anactl0 |= DAC_ANACTL0_DIFFM;

// 	/* left/right channels enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_DAENL | DAC_ANACTL0_DAENR);

// 	/* LN/LP OP enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PALNEN | DAC_ANACTL0_PALPEN);

// 	/* RN/RP OP enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PARNEN | DAC_ANACTL0_PARPEN);

// 	/* output stage enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PALNOSEN | DAC_ANACTL0_PARNOSEN);
// 	dac_reg->anactl0 |= (DAC_ANACTL0_SW1LN | DAC_ANACTL0_SW1RN);


// 	/* PA VOL set 0db */
// 	dac_reg->anactl0 |= DAC_ANACTL0_PAVOL(7);

// 	/* clear left and right channels soft step volume done pending */
// 	while (dac_reg->vol_lch & VOL_LCH_DONE_PD) {
// 		dac_reg->vol_lch |= VOL_LCH_DONE_PD;
// 	}

// 	while (dac_reg->vol_rch & VOL_RCH_DONE_PD) {
// 		dac_reg->vol_rch |= VOL_RCH_DONE_PD;
// 	}

// 	/* left and right channels volume set as 0db */
// 	//dac_reg->vol_lch |= VOL_LCH_SOFT_STEP_EN;
// 	//dac_reg->vol_rch |= VOL_RCH_SOFT_STEP_EN;

// 	if ((dac_reg->vol_lch & VOL_LCH_VOLL_MASK) != 0xBF) {
// 		dac_reg->vol_lch = (dac_reg->vol_lch & ~VOL_LCH_VOLL_MASK) | 0xBF;
// 		while (!(dac_reg->vol_lch & VOL_LCH_DONE_PD));
// 		dac_reg->vol_lch |= VOL_LCH_DONE_PD;
// 	}

// 	if ((dac_reg->vol_rch & VOL_RCH_VOLR_MASK) != 0xBF) {
// 		dac_reg->vol_rch = (dac_reg->vol_rch & ~VOL_RCH_VOLR_MASK) | 0xBF;
// 		while (!(dac_reg->vol_rch & VOL_RCH_DONE_PD));
// 		dac_reg->vol_rch |= VOL_RCH_DONE_PD;
// 	}

// 	/* set DACFIFO source from CPU */
// 	dac_reg->fifoctl = 0x7301;

// 	/* left/right channel antipop ramp data compensation */
// 	dac_reg->anactl1 |= DAC_ANACTL1_ATP2RCENL;
// 	dac_reg->anactl1 |= DAC_ANACTL1_ATP2RCENR;

// 	/* send max pcm data to DAC FIFO0 and delay 2ms */
// 	for (i = 0; i < 4; i++) {
// 		dac_reg->fifo0_dat = max_pcm << 8;
// 		dac_reg->fifo0_dat = max_pcm << 8;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* enable loop2 and SW2 connect for LP/RP */
// 	dac_reg->anactl1 |= (DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2RPEN);
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPSW2LP | DAC_ANACTL1_ATPSW2RP);

// 	for (i = 0; i < 4; i++) {
// 		dac_reg->fifo0_dat = max_pcm << 8;
// 		dac_reg->fifo0_dat = max_pcm << 8;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* antipop ramp connect enable for PA */
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_RP);
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPRC2EN_LP | DAC_ANACTL1_ATPRC2EN_RP);

// 	ramp_data = 0;
// 	ramp_step = 8000;

// 	/* send ramp data */
// 	while (ramp_data > min_pcm) {
// 		/* wait pcmbuf not full */
// 		while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) < 2);

// 		/* wait dacfifo not full */
// 		while ((dac_reg->stat & DAC_STAT_DAF0S_MASK) < 2);

// 		dac_reg->fifo0_dat = ramp_data << 8;
// 		dac_reg->fifo0_dat = ramp_data << 8;

// 		ramp_data -= ramp_step;
// 	}

// 	/* wait pcmbuf empty */
// 	while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) != 0x800);

// 	for (i = 0; i < 4; i++)
// 		dac_reg->fifo0_dat = min_pcm << 8;

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* enable sw1 and pa outputstage, disable sw2 */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1RP);
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PALPOSEN | DAC_ANACTL0_PARPOSEN);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPSW2LP | DAC_ANACTL1_ATPSW2RP);

// 	/* send ramp data for disable atprcen */
// 	ramp_data = 0;
// 	ramp_step = 4000;

// 	while (ramp_data < max_pcm) {
// 		/* wait pcmbuf not full */
// 		while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) < 2);

// 		/* wait dacfifo not full */
// 		while ((dac_reg->stat & DAC_STAT_DAF0S_MASK) < 2);

// 		dac_reg->fifo0_dat = ramp_data << 8;
// 		dac_reg->fifo0_dat = ramp_data << 8;

// 		ramp_data += ramp_step;
// 	}

// 	/* wait pcmbuf empty */
// 	while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) != 0x800);

// 	for (i = 0; i < 4; i++)
// 		dac_reg->fifo0_dat = max_pcm << 8;

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* disable loop2 and atprcen and atprcen2 */
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2RPEN);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPSW2LP | DAC_ANACTL1_ATPSW2RP);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRC2EN_LP | DAC_ANACTL1_ATPRC2EN_RP);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_RP);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATP2RCENL);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATP2RCENR);

// 	for (i = 0; i < 4; i++)
// 		dac_reg->fifo0_dat = 0;

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	dac_reg->fifoctl = dac_fifoctl;

// 	dac_reg->anactl0 |= DAC_ANACTL0_ZERODT;
// 	//dac_reg->anactl0 |= DAC_ANACTL0_SEL_PLAN;
// 	//dac_reg->anactl0 |= DAC_ANACTL0_SEL_CUR_MASK;
// 	dac_reg->anactl0 &= ~(1 << 22);

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// #if (CONFIG_AUDIO_DAC_HIGH_PERFORMACE_DIFF_EN == 1)
// 	__dac_analog_diff_sh_cfg(dev, 20, 100, 0);
// #endif
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT;
// }

// static void dac_single_end_off_antipop(struct device *dev)
// {
// 	int ramp_data = 0, max_pcm = 8388607, min_pcm = -8388607, i;
// 	uint32_t ramp_step;
// 	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

// 	/* enable DAC clock gate */
// 	acts_clock_peripheral_enable(CLOCK_ID_DAC);

// 	/* set sample rate 48K */
// 	dac_sample_rate_set(dev, SAMPLE_RATE_48KHZ);

// 	/* ldo enable */
// 	dac_ldo_power_control(dev, true);

// 	/* enable DAC digital function */
// 	__dac_digital_enable(dev, DAC_OSR_64X, STEREO_MODE, AUDIO_CHANNEL_DAC);

// 	/* set DACFIFO source from CPU */
// 	dac_reg->fifoctl = 0x7301;

// 	/* enable zero data */
// 	dac_reg->anactl0 |= DAC_ANACTL0_ZERODT;

// 	/* switch to old plan */
// 	dac_reg->anactl2 = 0;
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_CUR_MASK;
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_PLAN;
// 	dac_reg->anactl0 |= DAC_ANACTL0_PAVOL(7);

// 	/* enable PA DFC option */
// 	dac_reg->anactl0 |= DAC_ANACTL0_DFCEN;

// 	/* enable DAC + PA bias */
// 	dac_reg->anactl0 |= DAC_ANACTL0_BIASEN;

// 	/* disable LN/LP play back mute */
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);

// 	/* disable DAC R play back mute */
// 	dac_reg->anactl1 &= ~DAC_ANACTL1_DPBMR;

// 	/* set differential mode */
// 	dac_reg->anactl0 |= DAC_ANACTL0_DIFFM;

// 	/* left/right channels enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_DAENL | DAC_ANACTL0_DAENR);

// 	/* LN/LP OP enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PALNEN | DAC_ANACTL0_PALPEN);

// 	/* RN/RP OP enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PARNEN | DAC_ANACTL0_PARPEN);

// 	/* output stage enable */
// 	dac_reg->anactl0 |= (DAC_ANACTL0_PALNOSEN | DAC_ANACTL0_PARNOSEN);
// 	dac_reg->anactl0 |= (DAC_ANACTL0_SW1LN | DAC_ANACTL0_SW1RN);

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(3));
// 	else
// 		k_busy_wait(3000UL);

// 	/* disable zero data */
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_ZERODT;

// 	/* enable DAC FIFO clock */
// 	sys_write32(sys_read32(CMU_DACCLK) | CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);

//     dac_reg->vol_lch = (dac_reg->vol_lch & ~VOL_LCH_VOLL_MASK) | 0xBF;
// 	while ((dac_reg->vol_lch & VOL_LCH_VOLL_MASK) != 0xBF)
// 	{
// 		;
// 	}

//     dac_reg->vol_rch = (dac_reg->vol_rch & ~VOL_RCH_VOLR_MASK) | 0xBF;
// 	while ((dac_reg->vol_rch & VOL_RCH_VOLR_MASK) != 0xBF)
// 	{
// 		;
// 	}    
// 	dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;
// 	// dac_reg->digctl &= ~DAC_DIGCTL_DAF1M2DAEN;

// 	/* enable DAC ANA clock */
// 	acts_clock_peripheral_enable(CLOCK_ID_DACANACLK);

// 	dac_reg->anactl1 |= DAC_ANACTL1_ATP2RCENL;
// 	dac_reg->anactl1 |= DAC_ANACTL1_ATP2RCENR;

// 	//dac_reg->fifoctl = 0x7301;

// 	/* send max pcm data to DAC FIFO0 and delay 2ms */
// 	for (i = 0; i < 4; i++) {
// 		dac_reg->fifo0_dat = max_pcm << 8;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* enable loop2/atprcen/atprc2en */
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_RP);
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPRC2EN_LP | DAC_ANACTL1_ATPRC2EN_RP);
// 	dac_reg->anactl1 |= (DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2RPEN);

// 	/* send data from max ramp-data to min pcm data */
// 	ramp_data = 0;
// 	ramp_step = 10000;

// 	while (ramp_data > min_pcm) {
// 		/* wait pcmbuf not full */
// 		while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) < 2)
// 		{
// 			;
// 		}

// 		/* wait dacfifo not full */
// 		while ((dac_reg->stat & DAC_STAT_DAF0S_MASK) < 2)
// 		{
// 			;
// 		}

// 		dac_reg->fifo0_dat = ramp_data << 8;
// 		dac_reg->fifo0_dat = ramp_data << 8;

// 		ramp_data -= ramp_step;
// 	}

// 	/* wait pcmbuf empty */
// 	while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) != 0x800)
// 	{
// 		;
// 	}

// 	/* enable sw2 */
// 	dac_reg->anactl1 |= (DAC_ANACTL1_ATPSW2LP | DAC_ANACTL1_ATPSW2RP);

// 	for (i = 0; i < 4; i++)
// 		dac_reg->fifo0_dat = min_pcm << 8;

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

// 	/* disable sw1 and positive end paosen */
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1RP);
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_PALPOSEN | DAC_ANACTL0_PARPOSEN);

// 	/* send data from max ramp-data to min pcm data */
// 	ramp_data = 0;
// 	ramp_step = 10000;

// 	while (ramp_data < max_pcm) {
// 		/* wait pcmbuf not full */
// 		while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) < 2)
// 		{
// 			;
// 		}			

// 		/* wait dacfifo not full */
// 		while ((dac_reg->stat & DAC_STAT_DAF0S_MASK) < 2)
// 		{
// 			;
// 		}			

// 		dac_reg->fifo0_dat = ramp_data << 8;
// 		dac_reg->fifo0_dat = ramp_data << 8;

// 		ramp_data += ramp_step;
// 	}

// 	/* wait pcmbuf empty */
// 	while ((dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK) != 0x800)
// 	{
// 		;
// 	}

// 	/* disable atprcen, atprcen2, loop2en, sw2, da, pa,negatve end pa and paost */
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_RP);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRC2EN_LP | DAC_ANACTL1_ATPRC2EN_RP);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2RPEN);
// 	dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPSW2LP | DAC_ANACTL1_ATPSW2RP);
// 	dac_reg->anactl1 &= ~DAC_ANACTL1_ATP2RCENL;
// 	dac_reg->anactl1 &= ~DAC_ANACTL1_ATP2RCENR;

// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_DAENL | DAC_ANACTL0_DAENR);
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_PALPEN | DAC_ANACTL0_PARPEN);
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_PALNEN | DAC_ANACTL0_PARNEN);
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_PALNOSEN | DAC_ANACTL0_PARNOSEN);
// 	dac_reg->anactl0 &= ~(DAC_ANACTL0_SW1LN | DAC_ANACTL0_SW1RN);

// 	dac_reg->digctl &= ~DAC_DIGCTL_DDEN;
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_BIASEN;
// 	dac_reg->fifoctl &= ~DAC_FIFOCTL_DAF0RT;

// 	sys_write32(sys_read32(CMU_DACCLK) & ~CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);
// 	dac_reg->digctl &= ~DAC_DIGCTL_DAF0M2DAEN;
// }
// #else
// static void dac_single_end_on_antipop(struct device *dev)
// {
//     int ramp_data, max_pcm, min_pcm, data_cnt, i;
//     uint32_t ramp_step;
//     max_pcm = 0x7fffffff;//524287
//     min_pcm = 0x80000001;//-524287
//     data_cnt = 4800*3;//9600 samples each channel equals 200ms under 48kfs
//     ramp_step = 0xffffffff / data_cnt;
//     uint32_t dac_fifoctl;
// 	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);

// 	/* enable DAC clock gate */
// 	acts_clock_peripheral_enable(CLOCK_ID_DAC);

// 	/* set sample rate 48K */
// 	dac_sample_rate_set(dev, SAMPLE_RATE_48KHZ);

// 	/* ldo enable */
// 	dac_ldo_power_control(dev, true);

// 	/* enable DAC digital function */
// 	__dac_digital_enable(dev, DAC_OSR_64X, STEREO_MODE, AUDIO_CHANNEL_DAC);
// 	sys_write32(sys_read32(CMU_DACCLK) | CMU_DACCLK_DACFIFO0CLKEN, CMU_DACCLK);

// 	/* DAC FIFO0 MIX to DAC enable and disable DAC FIFO1 MIX */
// 	dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;
// 	// dac_reg->digctl &= ~DAC_DIGCTL_DAF1M2DAEN;

    
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_PLAN;
// 	dac_reg->anactl0 &= ~DAC_ANACTL0_SEL_CUR_MASK;

//     dac_reg->anactl0 &= ~DAC_ANACTL0_PAVOL_MASK;
//     dac_reg->anactl0 |= DAC_ANACTL0_PAVOL(0x7);  //pavol 0db
//     dac_reg->anactl0 |=  DAC_ANACTL0_DFCEN;  //enable dfc
//     dac_reg->anactl0 |=  DAC_ANACTL0_BIASEN;  //enable bias

//     //1.playback mute, sw1/dac ana/dac dig/pa en
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);
//     dac_reg->anactl0 |= (DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1LN);
//     dac_reg->anactl0 |= (DAC_ANACTL0_DAENL | DAC_ANACTL0_DAENR); //en dac lr
//     dac_reg->anactl0 |= (DAC_ANACTL0_DAINVENL | DAC_ANACTL0_DAINVENR); //en dac INV lr
//     dac_reg->anactl0 |= (DAC_ANACTL0_PALPEN | DAC_ANACTL0_PALNEN);
//     dac_reg->digctl  |= DAC_DIGCTL_DDEN;
    
//     //2.switch dacfifo0 input to cpu
//     dac_fifoctl = dac_reg->fifoctl;
//     //enable fifo0
//     //disable drq irq
//     //set to cpu
// 	dac_reg->fifoctl = 0x7301;

//     //3.send max pcm data to fifo0 and delay2ms
// 	for (i = 0; i < 8; i++) {
// 		dac_reg->fifo0_dat = max_pcm;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

//     //4.loop2/atprcen enable
//     dac_reg->anactl1 |= (DAC_ANACTL1_ATP2RCENL | DAC_ANACTL1_ATP2RCENR);
//     dac_reg->anactl1 |= (DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2LNEN);
//     dac_reg->anactl1 |= (DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_LN);

// //    dac_dump_register(dev);
    
//     //5.dac digital begain send max ramp-data untill min
//     ramp_data = max_pcm;
    
//     while(data_cnt > 0)
//     {
//        while( (dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK ) < 0x7fe ); 
//        dac_reg->fifo0_dat = ramp_data; //effective bit is 20,L
//        dac_reg->fifo0_dat = ramp_data; //R
//        ramp_data -= ramp_step;
//        if(data_cnt < 480)
//         ramp_data = min_pcm;
//        data_cnt--;
//     }
        
//     //6.after delay 200ms, enable pa outputstage
//     //ramp_data += ramp_step; //The while structure upside minus ramp_data one more

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(200));
// 	else
// 		k_busy_wait(200000UL);
    
//     dac_reg->anactl0 |= (DAC_ANACTL0_PALPOSEN | DAC_ANACTL0_PALNOSEN);
    
//     if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(5));
// 	else
// 		k_busy_wait(5000UL);
    
//     //7.after delay 2ms,disable loop2 and atprcen

//     dac_reg->anactl1 &= ~(DAC_ANACTL1_LP2LPEN | DAC_ANACTL1_LP2LNEN);
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_LN);
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_ATP2RCENL | DAC_ANACTL1_ATP2RCENR);    

//     //8.dac digital send 0, after delay 2ms, star soft-mute to enable playback
// 	for (i = 0; i < 8; i++) {
// 		dac_reg->fifo0_dat = 0;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

//     sys_write32(sys_read32(CMU_DEVCLKEN1) | (1 << 15), CMU_DEVCLKEN1);   //DACANACLKEN
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_SMCCKS_MASK);
//     dac_reg->anactl1 |= DAC_ANACTL1_SMCCKS(0x1);  //250hz
//     dac_reg->anactl1 |= DAC_ANACTL1_SMCEN;
//     dac_reg->anactl1 |= (DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(50));
// 	else
// 		k_busy_wait(50000UL);

//     dac_reg->anactl1 &= ~(DAC_ANACTL1_SMCEN);

//     dac_reg->fifoctl = dac_fifoctl;


// }

// static void dac_single_end_off_antipop(struct device *dev)
// {
//     int ramp_data, max_pcm, min_pcm, data_cnt, i;
//     uint32_t ramp_step;
//     max_pcm = 0x7fffffff;//524287
//     min_pcm = 0x80000001;//-524287
//     data_cnt = 4800*3;//9600 samples each channel equals 200ms under 48kfs
//     ramp_step = ((uint32_t)(0xffffffff)) / data_cnt;
//     ramp_data = max_pcm;
// 	struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
//     uint32_t dac_fifoctl;


// 	/* enable DAC clock gate */
// 	acts_clock_peripheral_enable(CLOCK_ID_DAC);

// 	/* set sample rate 48K */
// 	dac_sample_rate_set(dev, SAMPLE_RATE_48KHZ);

// 	/* ldo enable */
// 	dac_ldo_power_control(dev, true);

// 	/* DAC FIFO0 MIX to DAC enable and disable DAC FIFO1 MIX */
// 	dac_reg->digctl |= DAC_DIGCTL_DAF0M2DAEN;
// 	// dac_reg->digctl &= ~DAC_DIGCTL_DAF1M2DAEN;

//     //1.after delay 2ms,disable playback with soft-mute
// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);

//     sys_write32(sys_read32(CMU_DEVCLKEN1) | (1 << 15), CMU_DEVCLKEN1);  //DACANACLKEN
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_SMCCKS_MASK);
//     dac_reg->anactl1 |= DAC_ANACTL1_SMCCKS(0x2);  //1khz
//     dac_reg->anactl1 |= DAC_ANACTL1_SMCEN;
    
// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);


//     dac_reg->anactl1 &= ~(DAC_ANACTL1_DPBMLP | DAC_ANACTL1_DPBMLN);

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(30));
// 	else
// 		k_busy_wait(30000UL);
    
//     dac_reg->digctl|= DAC_DIGCTL_DDEN;

//     dac_fifoctl = dac_reg->fifoctl;

// 	/* set DACFIFO source from CPU */
// 	dac_reg->fifoctl = 0x7301;

//     //3.disable paosen, enable bcdisch and atprcen
//     dac_reg->anactl0 &= ~(DAC_ANACTL0_PALPOSEN | DAC_ANACTL0_PALNOSEN);
//     dac_reg->anactl0 &= ~(DAC_ANACTL0_SW1LP | DAC_ANACTL0_SW1LN);
    
// 	for (i = 0; i < 10; i++) {
// 		dac_reg->fifo0_dat = max_pcm;
// 	}

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(5));
// 	else
// 		k_busy_wait(5000UL);
    
//     dac_reg->anactl1 |= (DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_LN);
//     dac_reg->anactl1 |= (DAC_ANACTL1_BCDISCH_LP | DAC_ANACTL1_BCDISCH_LN);

// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(2));
// 	else
// 		k_busy_wait(2000UL);
          
//     //4.dac digital begain send max ramp-data untill min
//     //jump_cnt = 4800 * 1;
//     //ramp_data = max_pcm - (ramp_step * jump_cnt);
//     //data_cnt -= jump_cnt;
//     //while( data_cnt > (4800 * 5 ))
//     while(data_cnt > 0)
//     {
//        while( (dac_reg->pcm_buf_stat & PCM_BUF_STAT_PCMBS_MASK ) < 0x7fe ); //wait IRQ PD
//        dac_reg->fifo0_dat = ramp_data; //effective bit is 20,L
//        dac_reg->fifo0_dat = ramp_data; //R
//        ramp_data -= ramp_step;
//        data_cnt--;
//     }

//     //5.delay 100ms
//     //ramp_data += ramp_step;
// 	if (!z_is_idle_thread_object(_current))
// 		k_sleep(K_MSEC(100));
// 	else
// 		k_busy_wait(100000UL);
    
//     //6.disable pa,bcdisch,atprcen,sw1,dac ana,dac dig
//     dac_reg->anactl0 &= ~(DAC_ANACTL0_PALPEN | DAC_ANACTL0_PALNEN);
//     dac_reg->anactl0 &= ~(DAC_ANACTL0_DAENR | DAC_ANACTL0_DAENL);

//     dac_reg->fifoctl = dac_fifoctl;

//     dac_reg->anactl0 &= ~DAC_DIGCTL_DDEN;
//     dac_reg->anactl0 &= ~DAC_ANACTL0_BIASEN;
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_BCDISCH_LP | DAC_ANACTL1_BCDISCH_LN);
//     dac_reg->anactl1 &= ~(DAC_ANACTL1_ATPRCEN_LP | DAC_ANACTL1_ATPRCEN_LN);

// }
// #endif

// /* @brief DAC antipop process when system power up */
// static void dac_poweron_antipop_process(struct device *dev)
// {
// 	u32_t start_time;
// 	start_time = k_cycle_get_32();

// 	dac_single_end_on_antipop(dev);

// 	LOG_INF("poweron antipop process take %dus",
// 				k_cyc_to_us_floor32(k_cycle_get_32() - start_time));
// }

// /* @brief DAC antipop process when system power off */
// static void dac_poweroff_antipop_process(struct device *dev)
// {
// 	u32_t start_time;
// 	start_time = k_cycle_get_32();

// 	dac_single_end_off_antipop(dev);

// 	LOG_INF("poweroff antipop process take %dus",
// 				k_cyc_to_us_floor32(k_cycle_get_32() - start_time));
// }
// #endif

static int phy_dac_disable(struct device *dev, void *param);
static int phy_dac_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;
    int ret = 0;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        dac_dump_register(dev);
        break;
    }
    case AOUT_CMD_GET_SAMPLERATE:
    {
        *(audio_sr_sel_e *)param = dac_sample_rate_get(dev);
        break;
    }
    case AOUT_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = dac_sample_rate_set(dev, val);
        if (ret) {
            LOG_ERR("Failed to set DAC sample rate err=%d", ret);
            return ret;
        }
        break;
    }
    case AOUT_CMD_OPEN_PA:
    {
// #ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
// 		dac_poweron_antipop_process(dev);
// #endif

#ifdef CONFIG_CFG_DRV
        dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_ENABLE);
#endif
        break;
    }
    case AOUT_CMD_CLOSE_PA:
    {
// #ifdef CONFIG_AUDIO_ANTIPOP_PROCESS
// 		dac_poweroff_antipop_process(dev);
// #endif
        ret = phy_dac_disable_pa(dev);
        break;
    }
    case AOUT_CMD_OUT_MUTE:
    {
        uint8_t flag = *(uint8_t *)param;
        if (flag)
            __dac_mute_control(dev, true);
        else
            __dac_mute_control(dev, false);
        break;
    }
    case AOUT_CMD_GET_VOLUME:
    {
        uint16_t level;
        volume_setting_t *volume = (volume_setting_t *)param;
        level = __dac_volume_get(dev, LEFT_CHANNEL_SEL);
        volume->left_volume = dac_volume_level_to_db(level);
        level = __dac_volume_get(dev, RIGHT_CHANNEL_SEL);
        volume->right_volume = dac_volume_level_to_db(level);
        LOG_INF("Get volume [%d, %d]", volume->left_volume, volume->right_volume);
        break;
    }
    case AOUT_CMD_SET_VOLUME:
    {
        volume_setting_t *volume = (volume_setting_t *)param;
        ret = dac_volume_set(dev, volume->left_volume,
                volume->right_volume, data->sample_rate, 1);
        if (ret) {
            LOG_ERR("Volume set[%d, %d] error:%d",
                volume->left_volume, volume->right_volume, ret);
            return ret;
        }
        break;
    }
    case AOUT_CMD_GET_CHANNEL_STATUS:
    {
        uint8_t idx = *(uint8_t *)param;
        if (DAC_FIFO_INVALID_INDEX(idx)) {
            LOG_ERR("invalid fifo index %d", idx);
            return -EINVAL;
        }
        if (__check_dac_fifo_error(dev, idx))
            *(uint8_t *)param |= AUDIO_CHANNEL_STATUS_ERROR;

        if (!__is_dac_fifo_empty(dev, idx))
            *(uint8_t *)param |= AUDIO_CHANNEL_STATUS_BUSY;

        *(uint8_t *)param = 0;
        break;
    }
    case AOUT_CMD_GET_FIFO_LEN:
    {
        *(uint32_t *)param = DAC_PCMBUF_MAX_CNT;
        break;
    }
    case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
    {
        *(uint32_t *)param = __get_pcmbuf_avail_length(dev);
        break;
    }
    case AOUT_CMD_GET_APS:
    {
        ret = dac_get_pll_aps(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get audio pll APS err=%d", ret);
            return ret;
        }
        *(audio_aps_level_e *)param = (audio_aps_level_e)ret;
        ret = 0;
        break;
    }
    case AOUT_CMD_SET_APS:
    {
        audio_aps_level_e level = *(audio_aps_level_e *)param;
        ret = dac_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS err=%d", ret);
            return ret;
        }
        LOG_DBG("set new aps level %d", level);
        break;
    }
    case PHY_CMD_FIFO_GET:
    {
        aout_param_t *out_param = (aout_param_t *)param;
        uint8_t fifo_type;
        bool fifo_mix_en = true;

        if (!out_param)
            return -EINVAL;
        fifo_type = out_param->outfifo_type;
        if ((fifo_type != AOUT_FIFO_DAC0)
            && (fifo_type != AOUT_FIFO_DAC1)
            && (fifo_type != AOUT_FIFO_DAC1_ONLY_SPDIF)) {
            LOG_ERR("Invalid FIFO type %d", fifo_type);
            return -EINVAL;
        }

        if ((fifo_type == AOUT_FIFO_DAC1)
            && !__is_dac_fifo_working(dev, DAC_FIFO_0)) {
            LOG_ERR("DAC FIFO1 depends on DAC FIFO0 enabled");
            return -EPERM;
        }

        if (fifo_type == AOUT_FIFO_DAC1_ONLY_SPDIF) {
            fifo_type = AOUT_FIFO_DAC1;
            fifo_mix_en = false;
        }

        if (__is_dac_fifo_working(dev, fifo_type)) {
            LOG_ERR("DAC FIFO(%d) now is using", out_param->outfifo_type);
            return -EBUSY;
        }

        /* reset dac module */
        if (fifo_type == AOUT_FIFO_DAC0)
            acts_reset_peripheral(cfg->rst_id);

        /* enable dac clock */
        acts_clock_peripheral_enable(cfg->clk_id);

        __dac_fifo_enable(dev, FIFO_SEL_DMA,
                        (out_param->channel_width == CHANNEL_WIDTH_16BITS)
                        ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS,
                        DAC_FIFO_DRQ_LEVEL_DEFAULT,
                        DAC_FIFO_VOL_LEVEL_DEFAULT,
                        fifo_type, fifo_mix_en);

        if ((AOUT_FIFO_DAC0 == out_param->outfifo_type) ||
            (AOUT_FIFO_DAC1 == out_param->outfifo_type)) {
            __dac_pcmbuf_config(dev);
            data->ch[fifo_type].fifo_cnt = 0;
            data->ch[fifo_type].fifo_cnt_timestamp = 0;
            /* Record the PCM BUF data callback */
            data->ch[fifo_type].callback = out_param->callback;
            data->ch[fifo_type].cb_data = out_param->cb_data;
            LOG_DBG("Enable PCMBUF callback:%p", data->ch[fifo_type].callback);

            if (AOUT_FIFO_DAC0 == out_param->outfifo_type)
                data->ch_fifo0_start = 1;
            else
                data->ch_fifo1_start = 1;
        }

        dac_setting_t *dac_setting = out_param->dac_setting;
        if (dac_setting) {
            if (dac_setting->channel_mode == MONO_MODE)
                __dac_digital_enable_mono(dev);

            ret = dac_volume_set(dev, dac_setting->volume.left_volume,
                dac_setting->volume.right_volume, out_param->sample_rate, 1);
        }

        break;
    }
    case PHY_CMD_FIFO_PUT:
    {
        uint8_t idx = *(uint8_t *)param;
        if (idx == AOUT_FIFO_DAC1_ONLY_SPDIF)
            idx = AOUT_FIFO_DAC1;
        if (__is_dac_fifo_working(dev, idx)) {
            dac_wait_fifo_empty(dev, (a_dac_fifo_e)idx,
                DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS);
            __dac_fifo_disable(dev, idx);
            __dac_digital_disable_fifo(dev, idx);
            if (AOUT_FIFO_DAC0 == idx)
                data->ch_fifo0_start = 0;
            else
                data->ch_fifo1_start = 0;
        }
        break;
    }
    case PHY_CMD_DAC_FIFO_GET_SAMPLE_CNT:
    {
        uint32_t val;
        uint32_t idx = *(uint32_t *)param;

        if (DAC_FIFO_INVALID_INDEX(idx)) {
            LOG_ERR("invalid fifo index %d", idx);
            return -EINVAL;
        }

        val = __dac_read_fifo_counter(dev, idx);

        if (AOUT_FIFO_DAC0 == idx)
            *(uint32_t *)param = val + data->ch[0].fifo_cnt;
        else
            *(uint32_t *)param = val + data->ch[1].fifo_cnt;

        LOG_DBG("DAC FIFO counter: %d", *(uint32_t *)param);
        break;
    }
    case PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT:
    {
        uint8_t idx = *(uint8_t *)param;

        if (DAC_FIFO_INVALID_INDEX(idx)) {
            LOG_ERR("invalid fifo index %d", idx);
            return -EINVAL;
        }

        uint32_t key = irq_lock();

        __dac_fifo_counter_reset(dev, idx);

        if (AOUT_FIFO_DAC0 == idx) {
            data->ch[0].fifo_cnt = 0;
            data->ch[0].fifo_cnt_timestamp = 0;
        } else {
            data->ch[1].fifo_cnt = 0;
            data->ch[1].fifo_cnt_timestamp = 0;
        }

        irq_unlock(key);

        break;
    }
    case PHY_CMD_DAC_FIFO_ENABLE_SAMPLE_CNT:
    {
        uint8_t idx = *(uint8_t *)param;

        if (DAC_FIFO_INVALID_INDEX(idx)) {
            LOG_ERR("invalid fifo index %d", idx);
            return -EINVAL;
        }

        uint32_t key = irq_lock();
        __dac_fifo_counter_enable(dev, idx);
        irq_unlock(key);

        break;
    }
    case PHY_CMD_DAC_FIFO_DISABLE_SAMPLE_CNT:
    {
        uint8_t idx = *(uint8_t *)param;

        if (DAC_FIFO_INVALID_INDEX(idx)) {
            LOG_ERR("invalid fifo index %d", idx);
            return -EINVAL;
        }

        uint32_t key = irq_lock();

        __dac_fifo_counter_disable(dev, idx);

        if (AOUT_FIFO_DAC0 == idx) {
            data->ch[0].fifo_cnt = 0;
            data->ch[0].fifo_cnt_timestamp = 0;
        } else {
            data->ch[1].fifo_cnt = 0;
            data->ch[1].fifo_cnt_timestamp = 0;
        }
        irq_unlock(key);

        break;
    }
    // case PHY_CMD_DAC_FIFO_VOLUME_GET:
    // {
    //     uint32_t fifo_cmd = *(uint32_t *)param;
    //     uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
    //     ret = __dac_fifo_volume_get(dev, fifo_idx);
    //     if (ret < 0) {
    //         LOG_ERR("Get FIFO(%d) volume error", fifo_idx);
    //         return ret;
    //     }

    //     *(uint32_t *)param = PHY_FIFO_CMD(fifo_idx, ret);
    //     ret = 0;

    //     break;
    // }
    // case PHY_CMD_DAC_FIFO_VOLUME_SET:
    // {
    //     uint32_t fifo_cmd = *(uint32_t *)param;
    //     uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
    //     uint8_t volume = PHY_GET_FIFO_CMD_VAL(fifo_cmd);
    //     ret = __dac_fifo_volume_set(dev, fifo_idx, volume);

    //     break;
    // }
    case PHY_CMD_FIFO_DRQ_LEVEL_GET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);

        ret = __dac_fifo_drq_level_get(dev, fifo_idx);
        if (ret < 0) {
            LOG_ERR("Get FIFO(%d) drq level error", fifo_idx);
            return ret;
        }

        *(uint32_t *)param = PHY_FIFO_CMD(fifo_idx, ret);
        ret = 0;

        break;
    }
    case PHY_CMD_FIFO_DRQ_LEVEL_SET:
    {
        uint32_t fifo_cmd = *(uint32_t *)param;
        uint8_t fifo_idx = PHY_GET_FIFO_CMD_INDEX(fifo_cmd);
        uint8_t level = PHY_GET_FIFO_CMD_VAL(fifo_cmd);

        ret = __dac_fifo_drq_level_set(dev, fifo_idx, level);

        break;
    }
    case PHY_CMD_DAC_WAIT_EMPTY:
    {
        uint8_t fifo_idx = *(uint8_t *)param;
        __wait_dac_fifo_empty(dev, fifo_idx);
        break;
    }
    // case PHY_CMD_CLAIM_WITH_128FS:
    // {
    //     acts_clock_peripheral_enable(cfg->clk_id);
    //     __dac_digital_claim_128fs(dev, true);
    //     break;
    // }
    // case PHY_CMD_CLAIM_WITHOUT_128FS:
    // {
    //     __dac_digital_claim_128fs(dev, false);
    //     break;
    // }
    case PHY_CMD_GET_AOUT_DMA_INFO:
    {
        ret = dac_get_dma_info(dev, (struct audio_out_dma_info *)param);
        break;
    }
    case AOUT_CMD_SET_DAC_THRESHOLD:
    {
        dac_threshold_setting_t *thres = (dac_threshold_setting_t *)param;
        ret = __dac_pcmbuf_threshold_update(dev, thres);
        break;
    }
    case AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT:
    {
        uint32_t val = __dac_read_sdm_counter(dev);
        *(uint32_t *)param = data->sdm_cnt + val;
        break;
    }
    case AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT:
    {
        uint32_t key = irq_lock();
        __dac_sdm_counter_reset(dev);
        data->sdm_cnt = 0;
        data->sdm_cnt_timestamp = 0;
        irq_unlock(key);

        break;
    }
    case AOUT_CMD_ENABLE_DAC_SDM_SAMPLE_CNT:
    {
        uint32_t key = irq_lock();
        __dac_sdm_counter_enable(dev);
        irq_unlock(key);
        break;
    }
    case AOUT_CMD_DISABLE_DAC_SDM_SAMPLE_CNT:
    {
        uint32_t key = irq_lock();
        __dac_sdm_counter_disable(dev);
        data->sdm_cnt = 0;
        data->sdm_cnt_timestamp = 0;
        irq_unlock(key);
        break;
    }
    case AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT:
    {
        *(uint32_t *)param =  __dac_read_sdm_stable_counter(dev);
        break;
    }
    case AOUT_CMD_SET_DAC_TRIGGER_SRC:
    {
        uint8_t src = *(uint8_t *)param;
        ret = __dac_external_trigger_enable(dev, src);
        break;
    }
    // case AOUT_CMD_SELECT_DAC_ENABLE_CHANNEL:
    // {
    //     uint8_t lr_sel = *(uint8_t *)param;
    //     /* Select both left and right channels to enable */
    //     if (lr_sel == (LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL)) {
    //         if (data->lr_sel != lr_sel) {
    //             LOG_ERR("DAC DTS lr sel:%d conflict", data->lr_sel);
    //             ret = -EPERM;
    //         }
    //     } else if (lr_sel == LEFT_CHANNEL_SEL) { /* Only select left channel to enable */
    //         if (data->lr_sel & RIGHT_CHANNEL_SEL)
    //             ret = __dac_analog_disable(dev, RIGHT_CHANNEL_SEL);
    //     } else if (lr_sel == RIGHT_CHANNEL_SEL) { /* Only select right channel to enable */
    //         if (data->lr_sel & LEFT_CHANNEL_SEL)
    //             ret = __dac_analog_disable(dev, LEFT_CHANNEL_SEL);
    //     } else {
    //         LOG_ERR("invalid lr sel:%d", lr_sel);
    //         ret = -EINVAL;
    //     }

    //     break;
    // }
    case AOUT_CMD_DAC_FORCE_START:
    {
        dac_ext_trigger_ctl_t *trigger_ctl = (dac_ext_trigger_ctl_t *)param;
        __dac_digital_force_start(dev, trigger_ctl);
        break;
    }
    case AOUT_CMD_EXTERNAL_PA_CONTROL:
    {
#ifdef CONFIG_CFG_DRV
        uint8_t ctrl_func = *(uint8_t *)param;
        ret = dac_external_pa_ctl(dev, ctrl_func);
#else
        ret = -ENOTSUP;
#endif
        break;
    }
    case AOUT_CMD_SET_FIFO_SRC:
    {
        dac_fifosrc_setting_t *fifosrc = (dac_fifosrc_setting_t *)param;
        ret = __dac_fifo_update_src(dev, fifosrc);
        break;
    }
    case AOUT_CMD_DAC_TRIGGER_CONTROL:
    {
        dac_ext_trigger_ctl_t *trigger_ctl = (dac_ext_trigger_ctl_t *)param;
        ret = __dac_external_trigger_control(dev, trigger_ctl);
        break;
    }
    case AOUT_CMD_ANC_CONTROL:
    {
        dac_anc_ctl_t *anc_ctl = (dac_anc_ctl_t *)param;

        if (!anc_ctl) {
            LOG_ERR("invalid anc ctl");
            return -EINVAL;
        }

        if (anc_ctl->is_open_anc) {

            if (data->is_anc_enable) {
                LOG_ERR("DAC ANC already enabled");
                return -EACCES;
            }

            data->is_anc_enable = 1;

            /* check if DAC session has been opened normally */
            uint32_t key = irq_lock();
            if (!__dac_is_digital_working(dev)) {
                irq_unlock(key);
                aout_param_t aout_setting = {0};
                dac_setting_t dac_setting = {0};

                aout_setting.sample_rate = SAMPLE_RATE_48KHZ;
                aout_setting.channel_type = AUDIO_CHANNEL_DAC;
                aout_setting.channel_width = CHANNEL_WIDTH_16BITS;
                aout_setting.outfifo_type = AOUT_FIFO_DAC0;
                dac_setting.channel_mode = STEREO_MODE;
                aout_setting.dac_setting = &dac_setting;
                ret = phy_dac_enable(dev, &aout_setting);
            } else {
                atomic_inc(&data->refcount);
                irq_unlock(key);
            }
            LOG_INF("Enable ANC<=>DAC");
        } else {

            if (!data->is_anc_enable) {
                LOG_ERR("DAC does not enable yet");
                return -EACCES;
            }

            data->is_anc_enable = 0;
            uint8_t fifo_idx = AOUT_FIFO_DAC0;
            ret = phy_dac_disable(dev, &fifo_idx);
            LOG_INF("Disable ANC<=>DAC");
        }
        break;
    }
    default:
        LOG_ERR("Unsupport command %d", cmd);
        ret = -ENOTSUP;
    }

    return ret;

}

static int phy_dac_disable(struct device *dev, void *param)
{
	const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;
    uint8_t fifo_idx = *(uint8_t *)param;

    if ((fifo_idx != AOUT_FIFO_DAC0) && (fifo_idx != AOUT_FIFO_DAC1)) {
        LOG_ERR("Invalid FIFO index %d", fifo_idx);
        return -EINVAL;
    }

    uint32_t key = irq_lock();
    /* set channel stop flag and DAC FIFO0 is the main control channel */
    if (fifo_idx == AOUT_FIFO_DAC0)
        data->ch_fifo0_start = 0;
    else if (fifo_idx == AOUT_FIFO_DAC1)
        data->ch_fifo1_start = 0;

    atomic_dec(&data->refcount);
    if (atomic_get(&data->refcount) != 1) {
        LOG_INF("DAC disable refcount:%d", data->refcount);
        irq_unlock(key);
        return 0;
    }

    irq_unlock(key);
    irq_disable(IRQ_ID_DAC);
    irq_disable(IRQ_ID_DACFIFO);
    /* Timeout to wait DAC FIFO empty */
    if ((fifo_idx == AOUT_FIFO_DAC0) && __dac_is_digital_working(dev))
        dac_wait_fifo_empty(dev, fifo_idx, DAC_WAIT_FIFO_EMPTY_TIMEOUT_MS);

    if (AOUT_FIFO_DAC0 == fifo_idx) {
        __dac_digital_disable_fifo(dev, DAC_FIFO_0);
        __dac_fifo_disable(dev, DAC_FIFO_0);
        __dac_fifo_counter_disable(dev, DAC_FIFO_0);
        memset(&data->ch[0], 0, sizeof(struct phy_dac_channel));
    } else {
        __dac_digital_disable_fifo(dev, DAC_FIFO_1);
        __dac_fifo_disable(dev, DAC_FIFO_1);
        __dac_fifo_counter_disable(dev, DAC_FIFO_1);
        memset(&data->ch[1], 0, sizeof(struct phy_dac_channel));
    }

    /* check if all dac fifos are free */
    if (__is_dac_fifo_all_free(dev, true)) {
        data->sample_rate = 0;
        __dac_external_trigger_disable(dev);
        __dac_sdm_counter_disable(dev);

#ifdef CONFIG_CFG_DRV
        if (data->external_config.Keep_DA_Enabled_When_Play_Pause)
            __dac_mute_control(dev, true);
        else
            __dac_analog_disable(dev, LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL);
#else
#if (CONFIG_AUDIO_DAC_POWER_PREFERRED == 1)
        struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
        dac_reg->anactl0 = 0;
        dac_reg->anactl1 = 0;
        dac_reg->anactl2 = 0;
        dac_ldo_power_control(dev, false);
		acts_clock_peripheral_disable(cfg->clk_id);
		acts_clock_peripheral_disable(CLOCK_ID_DACANACLK);
        struct device *adc_dev = (struct device *)device_get_binding(CONFIG_AUDIO_ADC_0_NAME);
		uint8_t is_busy;
        if (adc_dev) {
			phy_audio_control(adc_dev, PHY_CMD_IS_ADC_BUSY, &is_busy);
      adc_reset_control(false);
			if (!is_busy)
      {
				acts_clock_peripheral_disable(CLOCK_ID_ADC);
      }
		}
		audio_pll_unset(data->audio_pll_index);
#else
        __dac_mute_control(dev, true);
#endif
#endif
        __dac_digital_disable(dev);
    }
    acts_reset_peripheral_assert(cfg->rst_id);
    soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
    return 0;
}

const struct phy_audio_driver_api phy_dac_drv_api = {
    .audio_enable = phy_dac_enable,
    .audio_disable = phy_dac_disable,
    .audio_ioctl = phy_dac_ioctl
};

/* dump dac device tree infomation */
static void __dac_dt_dump_info(const struct phy_dac_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    printk("**     DAC BASIC INFO     **\n");
    printk("     BASE: %08x\n", cfg->reg_base);
    printk("   CLK-ID: %08x\n", cfg->clk_id);
    printk("   RST-ID: %08x\n", cfg->rst_id);
    printk("DMA0-NAME: %s\n", cfg->dma_fifo0.dma_dev_name);
    printk("  DMA0-ID: %08x\n", cfg->dma_fifo0.dma_id);
    printk("  DMA0-CH: %08x\n", cfg->dma_fifo0.dma_chan);
    printk("DMA1-NAME: %s\n", cfg->dma_fifo1.dma_dev_name);
    printk("  DMA1-ID: %08x\n", cfg->dma_fifo1.dma_id);
    printk("  DMA1-CH: %08x\n", cfg->dma_fifo1.dma_chan);

    printk("** 	DAC FEATURES	 **\n");
    printk("    LAYOUT: %d\n", PHY_DEV_FEATURE(layout));
    printk("    LR-MIX: %d\n", PHY_DEV_FEATURE(dac_lr_mix));
    printk("       SDM: %d\n", PHY_DEV_FEATURE(noise_detect_mute));
    printk("  AUTOMUTE: %d\n", PHY_DEV_FEATURE(automute));
    printk("  LOOPBACK: %d\n", PHY_DEV_FEATURE(loopback));
    printk(" LEFT-MUTE: %d\n", PHY_DEV_FEATURE(left_mute));
    printk("RIGHT-MUTE: %d\n", PHY_DEV_FEATURE(right_mute));
    printk("    AM-IRQ: %d\n", PHY_DEV_FEATURE(am_irq));
#endif
}

/** @brief DAC digital IRQ routine
 * DAC digital IRQ source as below:
 *	- PCMBUF full IRQ/PD
 *	- PCMBUF half full IRQ/PD
 *	- PCMBUF half empty IRQ/PD
 *	- PCMBUF empty IRQ/PD
 *	- DACFIFO0 half empty IRQ/PD
 */
static void phy_dac_fifo_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    uint32_t stat, pending = 0;

    //audio_debug_trace_start();

    stat = dac_reg->pcm_buf_stat;

    // LOG_DBG("pcmbuf ctl:0x%x stat:0x%x", dac_reg->pcm_buf_ctl, stat);

    /* PCMBUF empty IRQ pending */
    if ((stat & PCM_BUF_STAT_PCMBEIP)
        && (dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE)) {
        pending |= AOUT_DMA_IRQ_TC;
        dac_reg->pcm_buf_ctl &= ~PCM_BUF_CTL_PCMBEPIE;
    }

    /* PCMBUF half empty IRQ pending */
    if (stat & PCM_BUF_STAT_PCMBHEIP) {
        pending |= AOUT_DMA_IRQ_HF;
        /* Wait until there is half empty irq happen and then start to detect empty irq */
        if (!(dac_reg->pcm_buf_ctl & PCM_BUF_CTL_PCMBEPIE))
            dac_reg->pcm_buf_ctl |= PCM_BUF_CTL_PCMBEPIE;
    }

    if (stat & PCM_BUF_STAT_IRQ_MASK)
        dac_reg->pcm_buf_stat = stat;

    if(dac_reg->stat & DAC_STAT_DAF0EIP)
        dac_reg->stat |= DAC_STAT_DAF0EIP;

    if (pending) {
        if ((dac_reg->digctl & DAC_DIGCTL_DAF0M2DAEN)
            && data->ch[0].callback && data->ch_fifo0_start) {
            data->ch[0].callback(data->ch[0].cb_data, pending);
        }
    }

    //audio_debug_trace_end();
}

/** @brief DAC FIFO IRQ routine
 * DAC FIFO IRQ source as below:
 *	- VOLL set IRQ/PD
 *	- VOLR IRQ/PD
 *	- PCMBUF CNT OF IRQ/PD
 *	- SDM_SAMPLES CNT OF IRQ/PD
 *	- AUTO_MUTE_CTL[0].AMEN IRQ/PD
 *	- DAC_DIGCTL[30].SMC IRQ/PD
 *  - DAC_DIGCTL_OVDT_PD
 */
static void phy_dac_digital_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_dac *dac_reg = get_dac_reg_base(dev);
    struct phy_dac_drv_data *data = dev->data;
    uint32_t timestamp;

    LOG_DBG("pcmbuf_cnt:0x%x sdm_cnt 0x%x",
            dac_reg->pcm_buf_cnt, dac_reg->sdm_samples_cnt);

    /* DAC auto mute detect pending */
    if (dac_reg->auto_mute_ctl & AUTO_MUTE_CTL_AMPD_OUT)
        dac_reg->auto_mute_ctl |= AUTO_MUTE_CTL_AMPD_OUT;
    if (dac_reg->auto_mute_ctl & AUTO_MUTE_CTL_AMPD_IN)
        dac_reg->auto_mute_ctl |= AUTO_MUTE_CTL_AMPD_IN;

    /* PA over load LOAD pending */
    if (dac_reg->digctl & DAC_DIGCTL_OVDT_PD)
        dac_reg->digctl |= DAC_DIGCTL_OVDT_PD;

    /* PCMBUF sample counter overflow irq pending */
    if (dac_reg->pcm_buf_cnt & PCM_BUF_CNT_IP) {
        data->ch[0].fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);

        dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;

        if (data->ch[0].fifo_cnt_timestamp) {
            if (k_cyc_to_us_floor32(k_cycle_get_32() - data->ch[0].fifo_cnt_timestamp)
                < DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US) {
                __dac_fifo_counter_reset(dev, DAC_FIFO_0);
                data->ch[0].fifo_cnt_timestamp = 0;
            }
        }
        data->ch[0].fifo_cnt_timestamp = k_cycle_get_32();

        timestamp = k_cycle_get_32();
        while (dac_reg->pcm_buf_cnt & PCM_BUF_CNT_IP) {
            dac_reg->pcm_buf_cnt |= PCM_BUF_CNT_IP;
            if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp)
                > DAC_FIFO_CNT_CLEAR_PENDING_TIME_US) {
                LOG_ERR("failed to clear DAC FIFO0 PD:0x%x", dac_reg->pcm_buf_cnt);
                __dac_fifo_counter_reset(dev, DAC_FIFO_0);
                break;
            }
        }
    }

    /* SDM sample counter overflow irq pending */
    if (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_IP) {
        data->sdm_cnt += (AOUT_SDM_CNT_MAX + 1);

        dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;

        timestamp = k_cycle_get_32();
        while (dac_reg->sdm_samples_cnt & SDM_SAMPLES_CNT_IP) {
            dac_reg->sdm_samples_cnt |= SDM_SAMPLES_CNT_IP;
            if (k_cyc_to_us_floor32(k_cycle_get_32() - timestamp
                > DAC_FIFO_CNT_CLEAR_PENDING_TIME_US)) {
                LOG_ERR("failed to clear SDM CNT:0x%x", dac_reg->sdm_samples_cnt);
                __dac_sdm_counter_reset(dev);
                break;
            }
        }

        if (data->sdm_cnt_timestamp) {
            if (k_cyc_to_us_floor32(k_cycle_get_32() - data->sdm_cnt_timestamp)
                < DAC_FIFO_CNT_MAX_SAME_SAMPLES_TIME_US) {
                __dac_sdm_counter_reset(dev);
                data->sdm_cnt_timestamp = 0;
            }
        }
        data->sdm_cnt_timestamp = k_cycle_get_32();
    }
}

#ifdef CONFIG_CFG_DRV
/* @brief initialize DAC external configuration */
static int phy_dac_config_init(const struct device *dev)
{
    struct phy_dac_drv_data *data = dev->data;
    int ret;
    uint8_t i;

    /* CFG_Struct_Audio_Settings */
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_OUT_MODE, Out_Mode);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_DAC_BIAS_SETTING, DAC_Bias_Setting);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_KEEP_DA_ENABLED_WHEN_PLAY_PAUSE, Keep_DA_Enabled_When_Play_Pause);
    PHY_AUDIO_CFG(data->external_config, ITEM_AUDIO_ANTIPOP_PROCESS_DISABLE, AntiPOP_Process_Disable);

    /* external PA pins */
    ret = cfg_get_by_key(ITEM_AUDIO_EXTERN_PA_CONTROL,
            &data->external_config.Extern_PA_Control, sizeof(data->external_config.Extern_PA_Control));
    if (ret) {
        for (i = 0; i < ARRAY_SIZE(data->external_config.Extern_PA_Control); i++) {
            LOG_INF("** External PA Pin@%d Info **", i);
            LOG_INF("PA_Function:%d", data->external_config.Extern_PA_Control[i].PA_Function);
            LOG_INF("GPIO_Pin:%d", data->external_config.Extern_PA_Control[i].GPIO_Pin);
            LOG_INF("Pull_Up_Down:%d", data->external_config.Extern_PA_Control[i].Pull_Up_Down);
            LOG_INF("Active_Level:%d", data->external_config.Extern_PA_Control[i].Active_Level);
        }

        dac_external_pa_ctl((struct device *)dev, EXTERNAL_PA_ENABLE);
    }

    return 0;
}
#endif

/* physical dac initialization */
static int phy_dac_init(const struct device *dev)
{
    const struct phy_dac_config_data *cfg = dev->config;
    struct phy_dac_drv_data *data = dev->data;

    __dac_dt_dump_info(cfg);

    memset(data, 0, sizeof(struct phy_dac_drv_data));

    atomic_set(&data->refcount, 1);

#ifdef CONFIG_CFG_DRV
    int ret;
    ret = phy_dac_config_init(dev);
    if (ret)
        LOG_ERR("DAC external config init error:%d", ret);
#endif

    if (cfg->irq_config)
        cfg->irq_config();

    printk("DAC init successfully\n");
    return 0;
}

static void phy_dac_irq_config(void);

/* physical dac driver data */
static struct phy_dac_drv_data phy_dac_drv_data0;

/* physical dac config data */
static const struct phy_dac_config_data phy_dac_config_data0 = {
    .reg_base = AUDIO_DAC_REG_BASE,
    AUDIO_DMA_FIFO_DEF(DAC, 0),
    AUDIO_DMA_FIFO_DEF(DAC, 1),
    .clk_id = CLOCK_ID_DAC,
    .rst_id = RESET_ID_DAC,
    .irq_config = phy_dac_irq_config,

    PHY_DEV_FEATURE_DEF(layout) = CONFIG_AUDIO_DAC_0_LAYOUT,
    PHY_DEV_FEATURE_DEF(dac_lr_mix) = CONFIG_AUDIO_DAC_0_LR_MIX,
    PHY_DEV_FEATURE_DEF(noise_detect_mute) = CONFIG_AUDIO_DAC_0_NOISE_DETECT_MUTE,
    PHY_DEV_FEATURE_DEF(automute) = CONFIG_AUDIO_DAC_0_AUTOMUTE,
    PHY_DEV_FEATURE_DEF(loopback) = CONFIG_AUDIO_DAC_0_LOOPBACK,
    PHY_DEV_FEATURE_DEF(left_mute) = CONFIG_AUDIO_DAC_0_LEFT_MUTE,
    PHY_DEV_FEATURE_DEF(right_mute) = CONFIG_AUDIO_DAC_0_RIGHT_MUTE,
    PHY_DEV_FEATURE_DEF(pa_vol) = CONFIG_AUDIO_DAC_0_PA_VOL,
    PHY_DEV_FEATURE_DEF(am_irq) = CONFIG_AUDIO_DAC_0_AM_IRQ,
};

#if IS_ENABLED(CONFIG_AUDIO_DAC_0)
DEVICE_DEFINE(dac0, CONFIG_AUDIO_DAC_0_NAME, phy_dac_init, NULL,
        &phy_dac_drv_data0, &phy_dac_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_dac_drv_api);
#endif

static void phy_dac_irq_config(void)
{
    /* Connect and enable DAC digital IRQ */
    IRQ_CONNECT(IRQ_ID_DAC, CONFIG_AUDIO_DAC_0_IRQ_PRI,
            phy_dac_digital_isr,
            DEVICE_GET(dac0), 0);

    /* Connect and enable DAC FIFO IRQ */
    IRQ_CONNECT(IRQ_ID_DACFIFO, CONFIG_AUDIO_DAC_0_IRQ_PRI,
            phy_dac_fifo_isr,
            DEVICE_GET(dac0), 0);
}

