/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2STX physical implementation
 */

/*
 * Features
 *    - Support master and slave mode
 *    - Support I2S 2ch format
 *	- I2STX FIFO(32 x 24bits level) and DAC FIFO(8 x 24 bits level)
 *	- Support 3 format: left-justified format, right-justified format, I2S format
 *	- Support 16/20/24 effective data width; BCLK support 32/64fs, and MCLK=4BCLK
 *	- Support sample rate auto detect in slave mode
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k/88.2k/96k/192k
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include <drivers/audio/audio_out.h>
#include <board.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(i2stx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * I2STX_CTL
 */
#define I2ST0_CTL_TXD_DELAY_SHIFT                             (21) /* tx data output delay 0: 2 mclk after bclk rising edge; 1: 3 mclk; 2: 4 mclk; 3" 5 mclk */
#define I2ST0_CTL_TXD_DELAY_MASK                              (0x3 << I2ST0_CTL_TXD_DELAY_SHIFT)
#define I2ST0_CTL_TXD_DELAY(x)                                ((x) << I2ST0_CTL_TXD_DELAY_SHIFT)

#define I2ST0_CTL_TDMTX_CHAN                                  BIT(20) /* 4 channel or 8 channel */
#define I2ST0_CTL_TDMTX_MODE                                  BIT(19) /* (TDM-A) I2S format or (TDM-B) left-justified format */

#define I2ST0_CTL_TDMTX_SYNC_SHIFT                            (17) /* the type of LRCLK setting at the begining of the data frame */
#define I2ST0_CTL_TDMTX_SYNC_MASK                             (0x3 << I2ST0_CTL_TDMTX_SYNC_SHIFT)
#define I2ST0_CTL_TDMTX_SYNC(x)                               ((x) << I2ST0_CTL_TDMTX_SYNC_SHIFT)

#define I2ST0_CTL_MULT_DEVICE                                 BIT(16) /* multi device selection */

#define I2ST0_CTL_LPEN0                                       BIT(8) /* I2STX and I2SRX data loopback enable */

#define I2ST0_CTL_TXWIDTH_SHIFT                               (4) /* effective data width */
#define I2ST0_CTL_TXWIDTH_MASK                                (0x3 << I2ST0_CTL_TXWIDTH_SHIFT)
#define I2ST0_CTL_TXWIDTH(x)                                  ((x) << I2ST0_CTL_TXWIDTH_SHIFT)

#define I2ST0_CTL_TXBCLKSET                                   BIT(3) /* bits of BCLK with LRCLK */

#define I2ST0_CTL_TXMODELSEL_SHIFT                            (1) /* I2S transfer format select */
#define I2ST0_CTL_TXMODELSEL_MASK                             (0x3 << I2ST0_CTL_TXMODELSEL_SHIFT)
#define I2ST0_CTL_TXMODELSEL(x)                               ((x) << I2ST0_CTL_TXMODELSEL_SHIFT)

#define I2ST0_CTL_TXEN                                        BIT(0) /* I2S TX Enable */

/***************************************************************************************************
 * I2STX_FIFOCTL
 */
#define I2ST0_FIFOCTL_FIFO0_VOL_SHIFT                         (8) /* FIFO volume control */
#define I2ST0_FIFOCTL_FIFO0_VOL_MASK                          (0xF << I2ST0_FIFOCTL_FIFO0_VOL_SHIFT)
#define I2ST0_FIFOCTL_FIFO0_VOL(x)                            ((x) << I2ST0_FIFOCTL_FIFO0_VOL_SHIFT)

#define I2ST0_FIFOCTL_TXFIFO_DMAWIDTH                         BIT(7) /* dma transfer width */

#define I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT                       (4) /* I2STX FIFO input select */
#define I2ST0_FIFOCTL_FIFO_IN_SEL_MASK                        (0x3 << I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT)
#define I2ST0_FIFOCTL_FIFO_IN_SEL(x)                          ((x) << I2ST0_FIFOCTL_FIFO_IN_SEL_SHIFT)

#define I2ST0_FIFOCTL_FIFO_SEL                                BIT(3) /* I2S/SPDIF module FIFO selection */
#define I2ST0_FIFOCTL_FIFO_IEN                                BIT(2) /* I2STX FIFO half empty irq enable */
#define I2ST0_FIFOCTL_FIFO_DEN                                BIT(1) /* I2STX FIFO half empty drq enable */
#define I2ST0_FIFOCTL_FIFO_RST                                BIT(0) /* I2STX FIFO reset */

/***************************************************************************************************
 * I2STX_FIFOSTAT
 */
#define I2ST0_FIFOSTA_FIFO_ER                                 BIT(8) /* FIFO error */
#define I2ST0_FIFOSTA_IP                                      BIT(7) /* half empty irq pending bit */
#define I2ST0_FIFOSTA_TFFU                                    BIT(6) /* fifo full flag */

#define I2ST0_FIFOSTA_STA_SHIFT                               (0) /* fifo status */
#define I2ST0_FIFOSTA_STA_MASK                                (0x3F << I2ST0_FIFOSTA_STA_SHIFT)

/***************************************************************************************************
 * I2STX_DAT
 */
#define I2ST0_DAT_DAT_SHIFT                                   (8) /* I2STX FIFO is 24bits x 32 levels */
#define I2ST0_DAT_DAT_MASK                                    (0xFFFFFF << I2ST0_DAT_DAT_SHIFT)

/***************************************************************************************************
 * I2STX_SRDCTL - I2STX Slave mode sample rate detect register
 */
#define I2ST0_SRDCTL_MUTE_EN                                  BIT(12) /* If detect sample rate or channel width changing, mute the TX output as 0 */
#define I2ST0_SRDCTL_SRD_IE                                   BIT(8) /* sample rate detect result change interrupt enable */

#define I2ST0_SRDCTL_CNT_TIM_SHIFT                            (4) /* slave mode sample rate detect counter period select */
#define I2ST0_SRDCTL_CNT_TIM_MASK                             (0x3 << I2ST0_SRDCTL_CNT_TIM_SHIFT)
#define I2ST0_SRDCTL_CNT_TIM(x)                               ((x) << I2ST0_SRDCTL_CNT_TIM_SHIFT)

#define I2ST0_SRDCTL_SRD_TH_SHIFT                             (1) /* sample rate detecting sensitivity setting */
#define I2ST0_SRDCTL_SRD_TH_MASK                              (0x7 << I2ST0_SRDCTL_SRD_TH_SHIFT)
#define I2ST0_SRDCTL_SRD_TH(x)                                ((x) << I2ST0_SRDCTL_SRD_TH_SHIFT)

#define I2ST0_SRDCTL_SRD_EN                                   BIT(0) /* slave mode sample rate detect enable */

/***************************************************************************************************
 * I2STX_SRDSTA
 */
#define I2ST0_SRDSTA_CNT_SHIFT                                (12) /* CNT of LRCLK which sampling by SRC_CLK */
#define I2ST0_SRDSTA_CNT_MASK                                 (0x1FFF << I2ST0_SRDSTA_CNT_SHIFT)
#define I2ST0_SRDSTA_CNT(x)                                   ((x) << I2ST0_SRDSTA_CNT_SHIFT)

#define I2ST0_SRDSTA_TO_PD                                    BIT(11) /* SRD timput irq pending */
#define I2ST0_SRDSTA_SRC_PD                                   BIT(10) /* sample rate changing detection interrupt pending */
#define I2ST0_SRDSTA_CHW_PD                                   BIT(8) /* channel width change irq pending */

#define I2ST0_SRDSTA_WL_SHIFT                                 (0) /* channel word lenght */
#define I2ST0_SRDSTA_WL_MASK                                  (0x7 << I2ST0_SRDSTA_WL_SHIFT)
#define I2ST0_SRDSTA_WL(x)                                    ((x) << I2ST0_SRDSTA_WL_SHIFT)

/***************************************************************************************************
 * I2STX_FIFO_CNT
 */
#define I2ST0_FIFO_CNT_IP                                     BIT(18) /* I2STX FIFO counter overflow irq pending */
#define I2ST0_FIFO_CNT_IE                                     BIT(17) /* I2STX FIFO counter overflow irq enable */
#define I2ST0_FIFO_CNT_EN                                     BIT(16) /* I2STX FIFO counter enable */

#define I2ST0_FIFO_CNT_CNT_SHIFT                              (0) /* I2STX FIFO counter */
#define I2ST0_FIFO_CNT_CNT_MASK                               (0xFFFF << I2ST0_FIFO_CNT_CNT_SHIFT)

/***************************************************************************************************
 * i2STX FEATURES CONGIURATION
 */

/* The sensitivity of the SRD */
#define I2STX_SRD_TH_DEFAULT                                  (7)

#define I2STX_BCLK_DIV_DEFAULT                                (0) /* MCLK = 4BCLK */

#define I2STX_FIFO_LEVEL                                      (32)

#define I2STX_SRD_CONFIG_TIMEOUT_US                           (500000)

#define I2STX_FIFO_MAX_VOL_LEVEL                              (0xF)
#define I2STX_FIFO_VOL_LEVEL_DEFAULT                          (0x3) /* 0db */

/*
 * enum a_i2s_mclk_clksrc_e
 * @brief The MCLK clock source of i2stx selection
 */
typedef enum {
    CLK_SRCTX_I2STX = 0, /* I2STX clock source from I2STX MCLK */
    CLK_SRCTX_DAC_256FS, /* I2STX clock source from DAC 256FS */
    CLK_SRCTX_DAC_128FS, /* I2STX clock source from DAC 256FS */
    CLK_SRCTX_I2STX_EXT, /* I2STX clock source from I2STX extern MCLK */
} a_i2stx_mclk_clksrc_e;

/*
 * @struct acts_audio_i2stx0
 * @brief I2STX controller hardware register
 */
struct acts_audio_i2stx {
    volatile uint32_t tx_ctl; /* I2STX control */
    volatile uint32_t fifoctl; /* I2STX FIFO control */
    volatile uint32_t fifostat; /* I2STX FIFO state */
    volatile uint32_t dat; /* I2STX FIFO data */
    volatile uint32_t srdctl; /* I2S slave mode TX sample rate detect control */
    volatile uint32_t srdstat; /* I2S slave mode TX sample rate detect state */
    volatile uint32_t reserved[2]; /* reserved space */
    volatile uint32_t fifocnt; /* I2STX out FIFO counter */
};

#ifdef CONFIG_CFG_DRV
/**
 * struct phy_i2stx_external_config
 * @brief The I2STX external configuration which generated by configuration tool.
 */
struct phy_i2stx_external_config {
    CFG_Type_I2S_Select_GPIO I2STX_Select_GPIO; /* I2STX PINs setting */
};
#endif

/*
 * struct phy_i2stx_drv_data
 * @brief The software related data that used by physical i2stx driver.
 */
struct phy_i2stx_drv_data {
    uint8_t mclksrc; /* MCLK clock source selection refer to #a_i2stx_mclk_clksrc_e */
    uint32_t fifo_cnt; /* I2STX FIFO hardware counter max value is 0xFFFF */
    int (*srd_callback)(void *cb_data, uint32_t cmd, void *param);
    void *cb_data;
    uint8_t srd_wl; /* The width length detected by SRD */
    uint8_t fifo_use; /* Record the used FIFO */
#ifdef CONFIG_CFG_DRV
    struct phy_i2stx_external_config external_config; /* I2STX external configuration */
#endif
    uint8_t link_with_dac : 1;   /* The flag of linkage with dac */
    uint8_t channel_opened : 1; /* flag of channel opened */
};

/**
 * union phy_i2stx_features
 * @brief The infomation from DTS to control the I2STX features to enable or nor.
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t srd_en : 1; /* SRD function enable or not */
        uint32_t mode : 1; /* master mode or slaver mode; 0: master 1: slaver */
        uint32_t channel_num : 4; /* channel number and only support 2/4/8 channels */
        uint32_t slave_internal_clk : 1; /* slave mode MCLK to use internal clock */
        uint32_t lrclk_proc : 1; /* LRCLK process 0: 50% DUTY; 1: 1 BCLK */
        uint32_t mclk_reverse : 1; /* mclk reverse */
        uint32_t always_open : 1; /* BCLK/LRCLK always existed */
        uint32_t format : 2; /* I2S transfer format */
        uint32_t tdm_format : 1; /* TDM format */
        uint32_t tdm_frame : 2; /* TDM frame start position */
        uint32_t bclk_width : 1; /* bclk width */
        uint32_t txd_delay : 2; /* I2STX data delay */
    } v;
} phy_i2stx_features;

/**
 * struct phy_i2stx_config_data
 * @brief The hardware related data that used by physical i2stx driver.
 */
struct phy_i2stx_config_data {
    uint32_t reg_base; /* I2STX controller register base address */
    uint8_t clk_id; /* I2STX devclk id */
    uint8_t srd_clk_id; /* I2S SRD clock id */
    uint8_t hclk_clk_id; /* I2S HCLK clock id */
    uint8_t rst_id; /* I2STX reset id */
    struct audio_dma_dt dma_fifo0; /* DMA resource for SPDIFRX */
    void (*irq_config)(void); /* IRQ configuration function */
    phy_i2stx_features features; /* I2STX features */
};

/* @brief  Get the I2STX controller base address */
static inline struct acts_audio_i2stx *get_i2stx_reg_base(struct device *dev)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    return (struct acts_audio_i2stx *)cfg->reg_base;
}

/* @brief Dump the I2STX relative registers */
static void i2stx_dump_register(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);

    LOG_INF("** i2stx contoller regster **");
    LOG_INF("          BASE: %08x", (uint32_t)i2stx_reg);
    LOG_INF("     I2STX_CTL: %08x", i2stx_reg->tx_ctl);
    LOG_INF(" I2STX_FIFOCTL: %08x", i2stx_reg->fifoctl);
    LOG_INF(" I2STX_FIFOSTA: %08x", i2stx_reg->fifostat);
    LOG_INF("     I2STX_DAT: %08x", i2stx_reg->dat);
    LOG_INF("  I2STX_SRDCTL: %08x", i2stx_reg->srdctl);
    LOG_INF("  I2STX_SRDSTA: %08x", i2stx_reg->srdstat);
    LOG_INF(" I2STX_FIFOCNT: %08x", i2stx_reg->fifocnt);
    LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
    LOG_INF("    CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
    LOG_INF("  CMU_I2STXCLK: %08x", sys_read32(CMU_I2STXCLK));
}

/* @brief I2STX sample rate config */
static int i2stx_sample_rate_set(struct device *dev, uint16_t sr_khz,
                a_i2stx_mclk_clksrc_e mclk_src, bool direct_set)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    int ret;
    uint8_t pre_div, clk_div, series, pll_index;
    uint32_t reg = 0, reg1;
    uint32_t lrclk_div;
    a_mclk_type_e mclk;

    if (!PHY_DEV_FEATURE(bclk_width))
        mclk = MCLK_256FS;
    else
        mclk = MCLK_128FS;

    if (direct_set) {
        reg = sys_read32(CMU_I2STXCLK) & ~CMU_I2STXCLK_I2SG0MCLKSRC_MASK;
        goto out;
    }

    if (CLK_SRCTX_I2STX_EXT != mclk_src) {
        /* make external MCLK clock phase reverse */
        if (PHY_DEV_FEATURE(mclk_reverse))
            reg |= CMU_I2STXCLK_I2SG0MCLKEXTREV;

        /* I2S master mode to enable MCLK output to PAD */
        if (!PHY_DEV_FEATURE(mode)) {
            reg |= CMU_I2STXCLK_I2STXMCLKOEN;
        } else {
            if (PHY_DEV_FEATURE(slave_internal_clk))
                sr_khz *= 2; /* slave mode use internal clk condition: MCLK > 6BCLK */
            /* BCLK/LRCLK source from external PAD */
            reg |= CMU_I2STXCLK_I2SG0BLRCLKSRC;
        }

        if (PHY_DEV_FEATURE(lrclk_proc))
            reg |= CMU_I2STXCLK_I2SG0LRCLKPROC;

        /* BCLK = FS x channel number x channel bit width */
        if (!PHY_DEV_FEATURE(bclk_width))
            lrclk_div = PHY_DEV_FEATURE(channel_num) * 32;
        else
            lrclk_div = PHY_DEV_FEATURE(channel_num) * 16;

        /* LRCLK divisor 0:32; 1:64; 2: 128; 3:256 */
        if (32 == lrclk_div) {
            lrclk_div = 0;
        } else if (64 == lrclk_div) {
            lrclk_div = 1;
        } else if (128 == lrclk_div) {
            lrclk_div = 2;
        } else if (256 == lrclk_div) {
            lrclk_div = 3;
        } else {
            LOG_ERR("invalid lrclk divisor:%d", lrclk_div);
            return -EINVAL;
        }

        reg |= CMU_I2STXCLK_I2SG0LRCLKDIV(lrclk_div);

        /* MCLK = 4 x BCLK */
        reg |= CMU_I2STXCLK_I2SG0BCLKDIV(I2STX_BCLK_DIV_DEFAULT);

        sr_khz = PHY_DEV_FEATURE(channel_num) / 2 * sr_khz;
    }

    if ((CLK_SRCTX_DAC_256FS == mclk_src)
        || (CLK_SRCTX_DAC_128FS == mclk_src)) {

        /* Get audio PLL setting  */
        ret = audio_get_pll_setting(sr_khz, MCLK_256FS, &pre_div, &clk_div, &series);
        if (ret) {
            LOG_DBG("get pll setting error:%d", ret);
            return ret;
        }

        ret = audio_pll_check_config(series, &pll_index);
        if (ret) {
            LOG_DBG("check pll config error:%d", ret);
            return ret;
        }

        reg1 = sys_read32(CMU_DACCLK) & ~0x1FF;
        reg1 |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
        reg1 |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);

        sys_write32(reg1, CMU_DACCLK);
    } else if (CLK_SRCTX_I2STX == mclk_src) {
        /* Get audio PLL setting  */
        ret = audio_get_pll_setting_i2s(sr_khz, mclk, &clk_div, &series);
        if (ret) {
            LOG_DBG("get pll setting error:%d", ret);
            return ret;
        }

        ret = audio_pll_check_config(series, &pll_index);
        if (ret) {
            LOG_DBG("check pll config error:%d", ret);
            return ret;
        }

        reg |= (pll_index & 0x1) << CMU_I2STXCLK_I2SG0CLKSRC;
        reg |=  clk_div << CMU_I2STXCLK_I2SG0CLKDIV_SHIFT;
    } else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
        /* BCLK/LRCLK source from external PAD */
        reg |= CMU_I2STXCLK_I2SG0BLRCLKSRC;
        LOG_INF("I2STX clock source from external");
    } else {
        LOG_ERR("Invalid i2stx clk source %d", mclk_src);
        return -EINVAL;
    }

out:
    /* Select the i2stx mclk source */
    reg |= CMU_I2STXCLK_I2SG0MCLKSRC(mclk_src & 0x3);

    sys_write32(reg, CMU_I2STXCLK);

    return 0;
}


/* @brief Get the sample rate from the I2STX config */
static int i2stx_sample_rate_get(struct device *dev)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    uint8_t pre_div, clk_div, pll_index, mclk_src;
    uint32_t reg;
    int ret = -1;
    a_mclk_type_e mclk;

    if (!PHY_DEV_FEATURE(bclk_width))
        mclk = MCLK_256FS;
    else
        mclk = MCLK_128FS;

    reg = sys_read32(CMU_I2STXCLK);

    mclk_src = (reg & CMU_I2STXCLK_I2SG0MCLKSRC_MASK) >> CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT;

    if ((CLK_SRCTX_DAC_256FS == mclk_src)
        || (CLK_SRCTX_DAC_128FS == mclk_src)) {
        reg = sys_read32(CMU_DACCLK);
        pll_index = (reg >> CMU_DACCLK_DACCLKSRC) & 0x1;
        pre_div = (reg & (1 << CMU_DACCLK_DACCLKPREDIV)) >> CMU_DACCLK_DACCLKPREDIV;
        clk_div = reg & CMU_DACCLK_DACCLKDIV_MASK;
        ret = audio_get_pll_sample_rate(MCLK_256FS, pre_div, clk_div, pll_index);
    } else if (CLK_SRCTX_I2STX == mclk_src) {
        reg = sys_read32(CMU_I2STXCLK);
        clk_div = reg & 0xF;
        pll_index = (reg >> CMU_I2STXCLK_I2SG0CLKSRC) & 0x1;
        ret = audio_get_pll_sample_rate_i2s(mclk, clk_div, pll_index);
    } else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
        LOG_INF("I2STX is using the external clock");
        ret = -ENOENT;
    }

    return ret;
}

/* @brief Get the AUDIO_PLL APS used by I2STX */
static int i2stx_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index, mclk_src;
    int ret = -1;

    reg = sys_read32(CMU_I2STXCLK);

    mclk_src = (reg & CMU_I2STXCLK_I2SG0MCLKSRC_MASK) >> CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT;

    /* If I2STX CLK from DAC_CLK, the APS regulation will be called at DAC APIs */
    if (CLK_SRCTX_I2STX == mclk_src) {
        pll_index = (reg & (1 << CMU_I2STXCLK_I2SG0CLKSRC)) >> CMU_I2STXCLK_I2SG0CLKSRC;
        ret = audio_pll_get_aps((a_pll_type_e)pll_index);
    } else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
        LOG_INF("I2STX is using the external clock source");
        ret = -ENOENT;
    } else {
        ret = -EPERM;
    }

    return ret;
}

/* @brief Set the AUDIO_PLL APS used by I2STX */
static int i2stx_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index, mclk_src;
    int ret = -1;

    reg = sys_read32(CMU_I2STXCLK);

    mclk_src = (reg & CMU_I2STXCLK_I2SG0MCLKSRC_MASK) >> CMU_I2STXCLK_I2SG0MCLKSRC_SHIFT;

    /* If I2STX CLK from DAC_CLK, the APS regulation will be called at DAC APIs */

    if (CLK_SRCTX_I2STX == mclk_src) {
        pll_index = (reg & (1 << CMU_I2STXCLK_I2SG0CLKSRC)) >> CMU_I2STXCLK_I2SG0CLKSRC;
        ret = audio_pll_set_aps((a_pll_type_e)pll_index, level);
    } else if (CLK_SRCTX_I2STX_EXT == mclk_src) {
        LOG_INF("I2STX is using the external clock");
        ret = -ENOENT;
    } else {
        ret = -EPERM;
    }

    return ret;
}

/* @brief  Disable the I2STX FIFO */
static void i2stx_fifo_disable(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    i2stx_reg->fifoctl = 0;
}

/* @brief  Reset the I2STX FIFO */
static void i2stx_fifo_reset(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    i2stx_reg->fifoctl &= ~I2ST0_FIFOCTL_FIFO_RST;
    i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_RST;
}

/* @brief Check the I2STX FIFO is working or not */
static bool is_i2stx_fifo_working(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    return !!(i2stx_reg->fifoctl & I2ST0_FIFOCTL_FIFO_RST);
}

/* @brief Check the I2STX FIFO is error */
static bool check_i2stx_fifo_error(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);

    if (i2stx_reg->fifostat & I2ST0_FIFOSTA_FIFO_ER) {
        i2stx_reg->fifostat |= I2ST0_FIFOSTA_FIFO_ER;
        return true;
    }

    return false;
}

/* @brief Get the I2STX FIFO status which indicates how many samples not filled */
static uint32_t i2stx_fifo_status(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    return (i2stx_reg->fifostat & I2ST0_FIFOSTA_STA_MASK) >> I2ST0_FIFOSTA_STA_SHIFT;
}

/* @brief  Enable the I2STX FIFO */
static void i2stx_fifo_enable(struct device *dev, audio_fifouse_sel_e sel, audio_dma_width_e width, bool this_use)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg = 0;

    LOG_DBG("I2STX sel:%d width:%d this_use:%d", sel, width, this_use);

    /* If the I2STX FIFO is used by SPDIFTX, the FIFO_SEL shall set to DAC FIFO source, otherwise will get no dma IRQ */
    if (!this_use)
        reg = I2ST0_FIFOCTL_FIFO_SEL; /* 0: I2STX FIFO; 1: DAC FIFO */

    /* set I2STX FIFO default volume*/
    reg &= ~I2ST0_FIFOCTL_FIFO0_VOL_MASK;
    reg |= I2ST0_FIFOCTL_FIFO0_VOL(I2STX_FIFO_VOL_LEVEL_DEFAULT);

    if (DMA_WIDTH_16BITS == width)
        reg |= I2ST0_FIFOCTL_TXFIFO_DMAWIDTH;

    if (FIFO_SEL_CPU == sel) {
        reg |= (I2ST0_FIFOCTL_FIFO_IEN | I2ST0_FIFOCTL_FIFO_RST);
    } else if (FIFO_SEL_DMA == sel) {
        reg |= (I2ST0_FIFOCTL_FIFO_DEN | I2ST0_FIFOCTL_FIFO_RST
                | I2ST0_FIFOCTL_FIFO_IN_SEL(1));
    } else if (FIFO_SEL_DSP == sel) {
        reg |= I2ST0_FIFOCTL_FIFO_IN_SEL(3) | I2ST0_FIFOCTL_FIFO_RST;
    } else {
        LOG_ERR("invalid fifo sel %d", sel);
    }
    i2stx_reg->fifoctl = reg;
}

/* @brief  Enable the I2STX FIFO counter function */
static void i2stx_fifocount_enable(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg = 0;

    reg = I2ST0_FIFO_CNT_EN;

    reg |= I2ST0_FIFO_CNT_IE;

    reg |= I2ST0_FIFO_CNT_IP;

    i2stx_reg->fifocnt = reg;
    LOG_INF("I2STX sample counter enable");
}

/* @brief  Disable the I2STX FIFO counter function */
static void i2stx_fifocount_disable(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    i2stx_reg->fifocnt &= ~(I2ST0_FIFO_CNT_EN | I2ST0_FIFO_CNT_IE);
}

/* @brief  Reset the I2STX FIFO counter function and by default to enable IRQ */
static void i2stx_fifocount_reset(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    i2stx_reg->fifocnt &= ~(I2ST0_FIFO_CNT_EN | I2ST0_FIFO_CNT_IE);

    i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_EN;

    i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_IE;
}

/* @brief  Enable the I2STX FIFO counter function */
static uint32_t i2stx_read_fifocount(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    return i2stx_reg->fifocnt & I2ST0_FIFO_CNT_CNT_MASK;
}

/* @brief set the I2STX FIFO volume */
static int i2stx_fifo_volume_set(struct device *dev, uint8_t vol)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg = i2stx_reg->fifoctl;

    /**
      * FIFO VOLUME LEVEL <=> db
      * 	- 0: 3db
      *	- 1: 2db
      *	- 2: 1db
      *	... ...
      *	- 0xe: -11db
      *	- 0xf: -12db
      */
    if (vol > I2STX_FIFO_MAX_VOL_LEVEL)
        vol = I2STX_FIFO_MAX_VOL_LEVEL;

    reg &= ~I2ST0_FIFOCTL_FIFO0_VOL_MASK;
    reg |= I2ST0_FIFOCTL_FIFO0_VOL(vol);

    i2stx_reg->fifoctl = reg;

    return 0;
}

/* @brief get the I2STX FIFO volume */
static int i2stx_fifo_volume_get(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg = i2stx_reg->fifoctl;
    int val;

    val = (reg & I2ST0_FIFOCTL_FIFO0_VOL_MASK) >> I2ST0_FIFOCTL_FIFO0_VOL_SHIFT;

    return val;
}

/* @brief  I2STX digital function control enable */
static void i2stx_digital_enable(struct device *dev, audio_ch_width_e width, bool en)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg = i2stx_reg->tx_ctl & ~(0x3E);
    uint8_t tx_width, fmt = PHY_DEV_FEATURE(format);

    /* By default TXD = 2MCLK */
    reg |= I2ST0_CTL_TXD_DELAY(PHY_DEV_FEATURE(txd_delay));

    if (CHANNEL_WIDTH_16BITS == width)
        tx_width = 0;
    else if (CHANNEL_WIDTH_20BITS == width)
        tx_width = 1;
    else
        tx_width = 2;

    if (PHY_DEV_FEATURE(channel_num) != 2) {
        fmt = 3; /* TDM mode */
        reg &= ~ (0xF << I2ST0_CTL_TDMTX_SYNC_SHIFT);

        if (PHY_DEV_FEATURE(channel_num) == 8)
            reg |= I2ST0_CTL_TDMTX_CHAN; /* 0: 4 channels; 1: 8 channels */

        if (PHY_DEV_FEATURE(tdm_format))
            reg |= I2ST0_CTL_TDMTX_MODE; /* 0: I2S format; 1: Left-justified format */

        reg |= I2ST0_CTL_TDMTX_SYNC(PHY_DEV_FEATURE(tdm_frame));
    }

    reg |= (I2ST0_CTL_TXMODELSEL(fmt) | I2ST0_CTL_TXWIDTH(tx_width));

    if (PHY_DEV_FEATURE(bclk_width))
        reg |= I2ST0_CTL_TXBCLKSET;

    i2stx_reg->tx_ctl = reg;

    /* I2S TX enable */
    if (en)
        i2stx_reg->tx_ctl |= I2ST0_CTL_TXEN;
}

/* @brief  I2STX digital function control disable */
static void i2stx_digital_disable(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    i2stx_reg->tx_ctl = 0;
}

/* @brief  I2STX sample rate detect function configuration */
static int i2stx_srd_cfg(struct device *dev, uint8_t srd_th, audio_i2s_srd_period_e period, bool irq_en, bool mute)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint32_t reg;
    uint8_t _wl, wl;
    uint32_t start_time, curr_time;

    reg = (I2ST0_SRDCTL_SRD_TH(srd_th & 0x7) | I2ST0_SRDCTL_CNT_TIM(period));

    if (irq_en)
        reg |= I2ST0_SRDCTL_SRD_IE;

    if (mute)
        reg |= I2ST0_SRDCTL_MUTE_EN;

    i2stx_reg->srdctl = reg;

    /* enable slave mode sample rate detect */
    i2stx_reg->srdctl |= I2ST0_SRDCTL_SRD_EN;

    if (!PHY_DEV_FEATURE(bclk_width))
        wl = SRDSTA_WL_64RATE;
    else
        wl = SRDSTA_WL_32RATE;

    start_time = k_cycle_get_32();
    _wl = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;
    while (_wl != wl) {
        curr_time = k_cycle_get_32();
        if (k_cyc_to_us_floor32(curr_time - start_time) > I2STX_SRD_CONFIG_TIMEOUT_US) {
            LOG_ERR("Wait SRD WL status timeout");
            return -ETIMEDOUT;
        }
        _wl = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;
        k_sleep(K_MSEC(2));
    }

    return 0;
}

/* @brief get the I2STX sample rate detect counter */
static uint32_t read_i2stx_srd_count(struct device *dev)
{
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    /* CNT of LRCLK which sampling by SRC_CLK */
    return ((i2stx_reg->srdstat & I2ST0_SRDSTA_CNT_MASK) >> I2ST0_SRDSTA_CNT_SHIFT);
}

/* @brief I2STX in slave mode handle sample rate detect change */
void i2stx_srd_fs_change(struct device *dev)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct phy_i2stx_drv_data *data = dev->data;
    uint32_t cnt, fs;
    audio_sr_sel_e sr;
    cnt = read_i2stx_srd_count(dev);

    /* CNT = SRD_CLK / LRCLK and SRD_CLK uses HOSC which is a 32MHz clock source*/
    fs = 32000000 / cnt;

    /* Allow a 1% deviation */
    if ((fs > 7920) && (fs < 8080)) { /* 8kfs */
        sr = SAMPLE_RATE_8KHZ;
    } else if ((fs > 10915) && (fs < 11135)) { /* 11.025kfs */
        sr = SAMPLE_RATE_11KHZ;
    } else if ((fs > 11880) && (fs < 12120)) { /* 12kfs */
        sr = SAMPLE_RATE_12KHZ;
    } else if ((fs > 15840) && (fs < 16160)) { /* 16kfs */
        sr = SAMPLE_RATE_16KHZ;
    } else if ((fs > 21830) && (fs < 22270)) { /* 22.05kfs */
        sr = SAMPLE_RATE_22KHZ;
    } else if ((fs > 23760) && (fs < 24240)) { /* 24kfs */
        sr = SAMPLE_RATE_24KHZ;
    } else if ((fs > 31680) && (fs < 32320)) { /* 32kfs */
        sr = SAMPLE_RATE_32KHZ;
    } else if ((fs > 43659) && (fs < 44541)) { /* 44.1kfs */
        sr = SAMPLE_RATE_44KHZ;
    } else if ((fs > 47520) && (fs < 48480)) { /* 48kfs */
        sr = SAMPLE_RATE_48KHZ;
    } else if ((fs > 63360) && (fs < 64640)) { /* 64kfs */
        sr = SAMPLE_RATE_64KHZ;
    } else if ((fs > 87318) && (fs < 89082)) { /* 88.2kfs */
        sr = SAMPLE_RATE_88KHZ;
    } else if ((fs > 95040) && (fs < 96960)) { /* 96kfs */
        sr = SAMPLE_RATE_96KHZ;
    } else if ((fs > 174636) && (fs < 178164)) { /* 176.4kfs */
        sr = SAMPLE_RATE_176KHZ;
    } else if((fs > 190080) && (fs < 193920)) { /* 192kfs */
        sr = SAMPLE_RATE_192KHZ;
    } else {
        LOG_ERR("Invalid sample rate %d", fs);
        return ;
    }

    LOG_INF("Detect new sample rate %d -> %d", fs, sr);

    /* FIXME: If not do the fifo reset, the left and right channel will exchange probably. */
    if (data->fifo_use == AOUT_FIFO_I2STX0)
        i2stx_fifo_reset(dev);

    if (PHY_DEV_FEATURE(slave_internal_clk))
        i2stx_sample_rate_set(dev, sr, data->mclksrc, false);

    if (data->srd_callback)
        data->srd_callback(data->cb_data, I2STX_SRD_FS_CHANGE, (void *)&sr);

}

/* @brief I2STX SRD width length change operation */
void i2stx_srd_wl_change(struct device *dev)
{
    struct phy_i2stx_drv_data *data = dev->data;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    uint8_t width;

    width = i2stx_reg->srdstat & I2ST0_SRDSTA_WL_MASK;

    LOG_INF("Detect new width length: %d", width);

    if (((data->srd_wl == SRDSTA_WL_64RATE) && (width == SRDSTA_WL_32RATE))
        || ((data->srd_wl == SRDSTA_WL_32RATE) && (width == SRDSTA_WL_64RATE))) {
        data->srd_wl = width;
        if (data->srd_callback)
            data->srd_callback(data->cb_data, I2STX_SRD_WL_CHANGE, (void *)&data->srd_wl);
    }
}

static int phy_i2stx_prepare_enable(struct device *dev, aout_param_t *out_param)
{
    const struct phy_i2stx_config_data *cfg = dev->config;

    if ((!out_param) || (!out_param->i2stx_setting)
        || (!out_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if (!(out_param->channel_type & AUDIO_CHANNEL_I2STX)) {
        LOG_ERR("Invalid channel type %d", out_param->channel_type);
        return -EINVAL;
    }

    if ((PHY_DEV_FEATURE(channel_num) != 2)
        && (PHY_DEV_FEATURE(channel_num) != 4)
        && (PHY_DEV_FEATURE(channel_num) != 8)) {
        LOG_ERR("Invalid channel num %d", PHY_DEV_FEATURE(channel_num));
        return -EINVAL;
    }

#ifndef CONFIG_CFG_DRV
    {
        struct board_pinmux_info pinmux_info;
        board_get_i2stx0_pinmux_info(&pinmux_info);
        /* Config the i2stx pin state */
        acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);
    }
#endif

    acts_clock_peripheral_enable(cfg->clk_id);
    acts_clock_peripheral_enable(cfg->hclk_clk_id);

    if (PHY_DEV_FEATURE(srd_en))
        acts_clock_peripheral_enable(cfg->srd_clk_id);

    return 0;
}

/* @brief physical I2STX device enable */
static int phy_i2stx_enable(struct device *dev, void *param)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    aout_param_t *out_param = (aout_param_t *)param;
    i2stx_setting_t *i2stx_setting = out_param->i2stx_setting;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    struct phy_i2stx_drv_data *data = dev->data;
    int ret;
    bool link_dac = false, link_spdif = false;

    ret = phy_i2stx_prepare_enable(dev, out_param);
    if (ret) {
        LOG_ERR("Failed to prepare enable i2stx err=%d", ret);
        return ret;
    }

    if (out_param->channel_type & AUDIO_CHANNEL_DAC) {
        link_dac = true;
        data->link_with_dac = 1;
        data->mclksrc = CLK_SRCTX_DAC_256FS;
    } else if (out_param->channel_type & AUDIO_CHANNEL_SPDIFTX) {
        link_spdif = true;
        if ((AOUT_FIFO_DAC0 == out_param->outfifo_type)
            || (AOUT_FIFO_DAC1 == out_param->outfifo_type)) {
            data->mclksrc = CLK_SRCTX_DAC_256FS;
            /* I2STX use DAC FIFO shall set FIFO_SET to 1 */
            i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_SEL;
        } else {
            data->mclksrc = CLK_SRCTX_I2STX;
        }
    } else {
        if ((AOUT_FIFO_DAC0 == out_param->outfifo_type)
            || (AOUT_FIFO_DAC1 == out_param->outfifo_type)) {
            data->mclksrc = CLK_SRCTX_DAC_256FS;
            data->fifo_use = out_param->outfifo_type;
            /* I2STX use DAC FIFO shall set FIFO_SET to 1 */
            i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_SEL;
        } else {
            data->mclksrc = CLK_SRCTX_I2STX;
        }

        /* slave mode */
        if (PHY_DEV_FEATURE(mode)) {
            /* MCLK use external clock */
            if (!PHY_DEV_FEATURE(slave_internal_clk))
                data->mclksrc = CLK_SRCTX_I2STX_EXT;
        }
    }

    /* I2STX sample rate set */
    if (i2stx_sample_rate_set(dev, out_param->sample_rate,
                data->mclksrc, link_dac ? true : false)) {
        LOG_ERR("Failed to config sample rate %d",
            out_param->sample_rate);
        return -ESRCH;
    }

    /* Linkage with DAC will force the I2STX use the DAC FIFO */
    if ((!link_dac) && (AOUT_FIFO_I2STX0 == out_param->outfifo_type)) {
        if (is_i2stx_fifo_working(dev)) {
            LOG_ERR("I2STX FIFO now is using ...");
            return -EACCES;
        }

        i2stx_fifo_enable(dev, FIFO_SEL_DMA, (out_param->channel_width == CHANNEL_WIDTH_16BITS)
                            ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS, true);
        data->fifo_use = AOUT_FIFO_I2STX0;
    }

    /* I2STX linkage with SPDIFTX */
    if (link_spdif)
        i2stx_reg->tx_ctl |= I2ST0_CTL_MULT_DEVICE;

    if (!link_dac) {
        i2stx_digital_enable(dev, out_param->channel_width, true);
    } else {
        /* linkage with DAC shall set FIFO_SET to 1 */
        i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_SEL;
        i2stx_digital_enable(dev, out_param->channel_width, false);
    }

    if (PHY_DEV_FEATURE(srd_en)) {
        if ((!link_dac) && (PHY_DEV_FEATURE(mode))) {
            LOG_INF("I2STX SRD enable");
            i2stx_srd_cfg(dev, I2STX_SRD_TH_DEFAULT, I2S_SRD_2LRCLK, true, false);
            data->srd_callback = i2stx_setting->srd_callback;
            data->cb_data = i2stx_setting->cb_data;
            if (!PHY_DEV_FEATURE(bclk_width))
                data->srd_wl = SRDSTA_WL_64RATE;
            else
                data->srd_wl = SRDSTA_WL_32RATE;
        }
    }

    if (AOUT_FIFO_I2STX0 == data->fifo_use) {
        /* Clear FIFO ERROR */
        if (i2stx_reg->fifostat & I2ST0_FIFOSTA_FIFO_ER)
            i2stx_reg->fifostat |= I2ST0_FIFOSTA_FIFO_ER;
    }

    /* set flag of channel opened */
    data->channel_opened = 1;

    return 0;
}

/* @brief physical I2STX device disable */
static int phy_i2stx_disable(struct device *dev, void *param)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct phy_i2stx_drv_data *data = dev->data;

    if (data->fifo_use == AOUT_FIFO_I2STX0)
        i2stx_fifo_disable(dev);

    /* Don't disable I2STX until the user call iotcl #PHY_CMD_I2STX_DISABLE_DEVICE */
    if (!PHY_DEV_FEATURE(always_open) && !PHY_DEV_FEATURE(mode)) {
        i2stx_digital_disable(dev);
        acts_clock_peripheral_disable(cfg->clk_id);
    }

    if (PHY_DEV_FEATURE(srd_en))
        acts_clock_peripheral_disable(cfg->srd_clk_id);

    data->srd_callback = NULL;
    data->cb_data = NULL;
    if (!PHY_DEV_FEATURE(bclk_width))
        data->srd_wl = SRDSTA_WL_64RATE;
    else
        data->srd_wl = SRDSTA_WL_32RATE;

    data->fifo_use = AUDIO_FIFO_INVALID_TYPE;
    data->fifo_cnt = 0;
    data->link_with_dac = 0;
    data->channel_opened = 0;

    return 0;
}

static int phy_i2stx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct phy_i2stx_drv_data *data = dev->data;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    int ret = 0;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        i2stx_dump_register(dev);
        break;
    }
    case AOUT_CMD_GET_SAMPLERATE:
    {
        ret = i2stx_sample_rate_get(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get I2STX sample rate err=%d", ret);
            return ret;
        }
        *(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
        ret = 0;
        break;
    }
    case AOUT_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = i2stx_sample_rate_set(dev, val, data->mclksrc,
                data->link_with_dac ? true : false);
        if (ret) {
            LOG_ERR("Failed to set I2STX sample rate err=%d", ret);
            return ret;
        }
        break;
    }
    case AOUT_CMD_GET_SAMPLE_CNT:
    {
        uint32_t val;
        val = i2stx_read_fifocount(dev);
        *(uint32_t *)param = val + data->fifo_cnt;
        LOG_DBG("I2STX FIFO counter: %d", *(uint32_t *)param);
        break;
    }
    case AOUT_CMD_RESET_SAMPLE_CNT:
    {
        data->fifo_cnt = 0;
        i2stx_fifocount_reset(dev);
        break;
    }
    case AOUT_CMD_ENABLE_SAMPLE_CNT:
    {
        i2stx_fifocount_enable(dev);
        break;
    }
    case AOUT_CMD_DISABLE_SAMPLE_CNT:
    {
        i2stx_fifocount_disable(dev);
        break;
    }
    case AOUT_CMD_GET_CHANNEL_STATUS:
    {
        uint8_t status = 0;
        if (check_i2stx_fifo_error(dev)) {
            status = AUDIO_CHANNEL_STATUS_ERROR;
            LOG_DBG("I2STX FIFO ERROR");
        }

        if (i2stx_fifo_status(dev) < (I2STX_FIFO_LEVEL - 1))
            status |= AUDIO_CHANNEL_STATUS_BUSY;

        *(uint8_t *)param = status;
        break;
    }
    case AOUT_CMD_GET_FIFO_LEN:
    {
        *(uint32_t *)param = I2STX_FIFO_LEVEL;
        break;
    }
    case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
    {
        *(uint32_t *)param = i2stx_fifo_status(dev);
        break;
    }
    case AOUT_CMD_GET_APS:
    {
        ret = i2stx_get_pll_aps(dev);
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
        ret = i2stx_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS err=%d", ret);
            return ret;
        }
        break;
    }
    case PHY_CMD_FIFO_GET:
    {
        aout_param_t *out_param = (aout_param_t *)param;
        if (!out_param)
            return -EINVAL;

        /* enable i2stx clock */
        acts_clock_peripheral_enable(cfg->clk_id);
        acts_clock_peripheral_enable(cfg->hclk_clk_id);

        if (is_i2stx_fifo_working(dev)) {
            LOG_ERR("I2STX FIFO is using");
            return -EBUSY;
        }

        /* SPDIFTX use I2STX FIFO shall set FIFO_SET to 1 */
        i2stx_reg->fifoctl |= I2ST0_FIFOCTL_FIFO_SEL;

        i2stx_fifo_enable(dev, FIFO_SEL_DMA,
            (out_param->channel_width == CHANNEL_WIDTH_16BITS)
            ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS, false);
        break;
    }
    case PHY_CMD_FIFO_PUT:
    {
        if (is_i2stx_fifo_working(dev))
            i2stx_fifo_disable(dev);
        data->fifo_cnt = 0;
        break;
    }
    case PHY_CMD_I2STX_IS_OPENED:
    {
        *(uint8_t *)param = data->channel_opened;
        break;
    }
    case PHY_CMD_I2STX_DISABLE_DEVICE:
    {
        i2stx_digital_disable(dev);
        acts_clock_peripheral_disable(cfg->clk_id);
        acts_clock_peripheral_disable(cfg->hclk_clk_id);
        acts_clock_peripheral_disable(cfg->srd_clk_id);
        break;
    }
    case PHY_CMD_I2STX_CLK_SET:
    {
        uint8_t sr = *(uint8_t *)param;
        struct board_pinmux_info pinmux_info;
        acts_clock_peripheral_enable(cfg->clk_id);
        board_get_i2stx0_pinmux_info(&pinmux_info);
        acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);

        ret = i2stx_sample_rate_set(dev, sr, CLK_SRCTX_I2STX, false);
        break;
    }
    case PHY_CMD_GET_AOUT_DMA_INFO:
    {
        struct audio_out_dma_info *info = (struct audio_out_dma_info *)param;
        info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
        info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
        info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
        break;
    }
    case PHY_CMD_I2STX_IS_MCLK_128FS:
    {
        if (PHY_DEV_FEATURE(bclk_width))
            *(uint8_t *)param = 1;
        else
            *(uint8_t *)param = 0;
        break;
    }
    case PHY_CMD_DAC_FIFO_VOLUME_SET:
    {
        uint8_t volume = *(uint8_t *)param;
        ret = i2stx_fifo_volume_set(dev, volume);
        break;
    }
    case PHY_CMD_DAC_FIFO_VOLUME_GET:
    {
        ret = i2stx_fifo_volume_get(dev);
        if (ret < 0) {
            LOG_ERR("Get I2STX FIFO volume error=%d", ret);
            return ret;
        }
        *(uint8_t *)param = ret;
        ret = 0;
        break;
    }
    default:
        LOG_ERR("Unsupport command %d", cmd);
        return -ENOTSUP;
    }

    return ret;
}

const struct phy_audio_driver_api phy_i2stx_drv_api = {
    .audio_enable = phy_i2stx_enable,
    .audio_disable = phy_i2stx_disable,
    .audio_ioctl = phy_i2stx_ioctl,
};

/* dump i2stx device tree infomation */
static void __i2stx_dt_dump_info(const struct phy_i2stx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     I2STX BASIC INFO     **");
    LOG_INF("     BASE: %08x", cfg->reg_base);
    LOG_INF("   CLK-ID: %08x", cfg->clk_id);
    LOG_INF("SRDCLK-ID: %08x", cfg->srd_clk_id);
    LOG_INF("   RST-ID: %08x", cfg->rst_id);
    LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
    LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
    LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

    LOG_INF("** 	I2STX FEATURES	 **");
    LOG_INF("      SRD-EN: %d", PHY_DEV_FEATURE(srd_en));
    LOG_INF("        MODE: %d", PHY_DEV_FEATURE(mode));
    LOG_INF("  BCLK-WIDTH: %d", PHY_DEV_FEATURE(bclk_width));
    LOG_INF("      CH-NUM: %d", PHY_DEV_FEATURE(channel_num));
    LOG_INF("INTERNAL-CLK: %d", PHY_DEV_FEATURE(slave_internal_clk));
    LOG_INF("  LRCLK-PROC: %d", PHY_DEV_FEATURE(lrclk_proc));
    LOG_INF("MCLK-REVERSE: %d", PHY_DEV_FEATURE(mclk_reverse));
    LOG_INF(" ALWAYS-OPEN: %d", PHY_DEV_FEATURE(always_open));
    LOG_INF("  TDM-FORMAT: %d", PHY_DEV_FEATURE(tdm_format));
    LOG_INF("   TDM-FRAME: %d", PHY_DEV_FEATURE(tdm_frame));
    LOG_INF("   TDM-FRAME: %d", PHY_DEV_FEATURE(txd_delay));
#endif
}

static void phy_i2stx_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
    struct phy_i2stx_drv_data *data = dev->data;

    LOG_DBG("fifocnt 0x%x srdstat 0x%x", i2stx_reg->fifocnt, i2stx_reg->srdstat);

    if (i2stx_reg->fifocnt & I2ST0_FIFO_CNT_IP) {
        data->fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);
        /* Here we need to wait 100us for the synchronization of audio clock fields */
        k_busy_wait(100);
        i2stx_reg->fifocnt |= I2ST0_FIFO_CNT_IP;
    }

    /* Sample rate detection timeout irq pending */
    if (i2stx_reg->srdstat & I2ST0_SRDSTA_TO_PD) {
        i2stx_reg->srdstat |= I2ST0_SRDSTA_TO_PD;
        if (data->srd_callback)
            data->srd_callback(data->cb_data, I2STX_SRD_TIMEOUT, NULL);
    }

    /* Sample rate changed detection irq pending */
    if (i2stx_reg->srdstat & I2ST0_SRDSTA_SRC_PD) {
        i2stx_reg->srdstat |= I2ST0_SRDSTA_SRC_PD;
        i2stx_srd_fs_change(dev);
    }

    /* Channel width change irq pending */
    if (i2stx_reg->srdstat & I2ST0_SRDSTA_CHW_PD) {
        i2stx_reg->srdstat |= I2ST0_SRDSTA_CHW_PD;
        i2stx_srd_wl_change(dev);
    }
}

#ifdef CONFIG_CFG_DRV
/* @brief initialize I2STX external configuration */
static int phy_i2stx_config_init(const struct device *dev)
{
    struct phy_i2stx_drv_data *data = dev->data;
    struct acts_pin_config i2stx_pins_state[4] = {0};
    int ret;

    ret = cfg_get_by_key(ITEM_AUDIO_I2STX_SELECT_GPIO,
            &data->external_config.I2STX_Select_GPIO, sizeof(data->external_config.I2STX_Select_GPIO));
    if (ret) {
        LOG_INF("** I2STX PINMUX **");
        LOG_INF("I2S_MCLK:%d", data->external_config.I2STX_Select_GPIO.I2S_MCLK);
        LOG_INF("I2S_LRCLK:%d", data->external_config.I2STX_Select_GPIO.I2S_LRCLK);
        LOG_INF("I2S_BCLK:%d", data->external_config.I2STX_Select_GPIO.I2S_BCLK);
        LOG_INF("I2S_DOUT:%d", data->external_config.I2STX_Select_GPIO.I2S_DOUT);
    }

    if ((data->external_config.I2STX_Select_GPIO.I2S_MCLK != GPIO_NONE)
        && (data->external_config.I2STX_Select_GPIO.I2S_LRCLK != GPIO_NONE)
        && (data->external_config.I2STX_Select_GPIO.I2S_BCLK != GPIO_NONE)
        && (data->external_config.I2STX_Select_GPIO.I2S_DOUT != GPIO_NONE)) {
        i2stx_pins_state[0].pin_num = data->external_config.I2STX_Select_GPIO.I2S_MCLK;
        i2stx_pins_state[0].mode = 12;
        i2stx_pins_state[1].pin_num = data->external_config.I2STX_Select_GPIO.I2S_LRCLK;
        i2stx_pins_state[1].mode = 12;
        i2stx_pins_state[2].pin_num = data->external_config.I2STX_Select_GPIO.I2S_BCLK;
        i2stx_pins_state[2].mode = 12;
        i2stx_pins_state[3].pin_num = data->external_config.I2STX_Select_GPIO.I2S_DOUT;
        i2stx_pins_state[3].mode = 12;
        acts_pinmux_setup_pins(i2stx_pins_state, ARRAY_SIZE(i2stx_pins_state));
    }

    return 0;
}
#endif

static int phy_i2stx_init(const struct device *dev)
{
    const struct phy_i2stx_config_data *cfg = dev->config;
    struct phy_i2stx_drv_data *data = dev->data;

    /* clear driver data */
    memset(data, 0, sizeof(struct phy_i2stx_drv_data));

    __i2stx_dt_dump_info(cfg);

#ifdef CONFIG_CFG_DRV
    phy_i2stx_config_init(dev);
#endif

    /* reset I2STX controller */
    acts_reset_peripheral(cfg->rst_id);

    if (cfg->irq_config)
        cfg->irq_config();

    data->fifo_use = AUDIO_FIFO_INVALID_TYPE;

    printk("I2STX init successfully\n");

    return 0;
}

static void phy_i2stx_irq_config(void);

/* physical i2stx driver data */
static struct phy_i2stx_drv_data phy_i2stx_drv_data0;

/* physical i2stx config data */
static const struct phy_i2stx_config_data phy_i2stx_config_data0 = {
    .reg_base = AUDIO_I2STX0_REG_BASE,
    .clk_id = CLOCK_ID_I2STX,
    .srd_clk_id = CLOCK_ID_I2SSRDCLK,
    .hclk_clk_id = CLOCK_ID_I2SHCLKEN,
    .rst_id = RESET_ID_I2STX,
    AUDIO_DMA_FIFO_DEF(I2STX, 0),
    .irq_config = phy_i2stx_irq_config,

    PHY_DEV_FEATURE_DEF(format) = CONFIG_AUDIO_I2STX_0_FORMAT,
    PHY_DEV_FEATURE_DEF(srd_en) = CONFIG_AUDIO_I2STX_0_SRD_EN,
    PHY_DEV_FEATURE_DEF(mode) = CONFIG_AUDIO_I2STX_0_MODE,
    PHY_DEV_FEATURE_DEF(bclk_width) = CONFIG_AUDIO_I2STX_0_BCLK_WIDTH,
    PHY_DEV_FEATURE_DEF(channel_num) = CONFIG_AUDIO_I2STX_0_CHANNEL_NUM,
    PHY_DEV_FEATURE_DEF(slave_internal_clk) = CONFIG_AUDIO_I2STX_0_SLAVE_INTERNAL_CLK,
    PHY_DEV_FEATURE_DEF(lrclk_proc) = CONFIG_AUDIO_I2STX_0_LRCLK_PROC,
    PHY_DEV_FEATURE_DEF(mclk_reverse) = CONFIG_AUDIO_I2STX_0_MCLK_REVERSE,
    PHY_DEV_FEATURE_DEF(always_open) = CONFIG_AUDIO_I2STX_0_ALWAYS_OPEN,
    PHY_DEV_FEATURE_DEF(tdm_format) = CONFIG_AUDIO_I2STX_0_TDM_FORMAT,
    PHY_DEV_FEATURE_DEF(tdm_frame) = CONFIG_AUDIO_I2STX_0_TDM_FRAME,
    PHY_DEV_FEATURE_DEF(txd_delay) = CONFIG_AUDIO_I2STX_0_TX_DELAY,
};

#if IS_ENABLED(CONFIG_AUDIO_I2STX_0)
DEVICE_DEFINE(i2stx0, CONFIG_AUDIO_I2STX_0_NAME, phy_i2stx_init, NULL,
        &phy_i2stx_drv_data0, &phy_i2stx_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_i2stx_drv_api);
#endif

/*
 * @brief Enable I2STX IRQ
 * @note I2STX IRQ source as shown below:
 *	- I2STX FIFO Half Empty IRQ
 *	- I2STX FIFO CNT IRQ
 *	- I2STX SRDTO IRQ
 *	- I2STX SRDSR IRQ
 *	- I2STX SRDCHW IRQ
 */
static void phy_i2stx_irq_config(void)
{
    IRQ_CONNECT(IRQ_ID_I2S0, CONFIG_AUDIO_I2STX_0_IRQ_PRI,
            phy_i2stx_isr,
            DEVICE_GET(i2stx0), 0);
    irq_enable(IRQ_ID_I2S0);
}

