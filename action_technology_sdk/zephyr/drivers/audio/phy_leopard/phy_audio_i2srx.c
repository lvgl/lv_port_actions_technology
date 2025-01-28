/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2SRX physical implementation
 */

/*
 * Features
 *    - Support master and slave mode
 *    - Support I2S 2ch format
 *	- I2SRXFIFO(32 x 24bits level)
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
#include <drivers/audio/audio_in.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(i2srx0, LOG_LEVEL_DBG);
/***************************************************************************************************
 * I2SRX_CTL
 */
#define I2SR0_CTL_PCM_SYNC                                  BIT(26) /* PCM frame select*/
#define I2SR0_CTL_PCM_CHAN_SHIFT                             (24) 
#define I2SR0_CTL_PCM_CHAN_MASK                              (0x3 << I2SR0_CTL_PCM_CHAN_SHIFT)
#define I2SR0_CTL_PCM_CHAN(x)                                ((x) << I2SR0_CTL_PCM_CHAN_SHIFT) /* pcm slot select*/
#define I2SR0_CTL_TDMRX_CHAN                                  BIT(16) /* 4 channel or 8 channel */
#define I2SR0_CTL_TDMRX_MODE                                  BIT(15) /* I2S format or left-justified format */

#define I2SR0_CTL_TDMRX_SYNC_SHIFT                            (13) /* at the beginning of the data frame, the type of LRCLK setting */
#define I2SR0_CTL_TDMRX_SYNC_MASK                             (0x3 << I2SR0_CTL_TDMRX_SYNC_SHIFT)
#define I2SR0_CTL_TDMRX_SYNC(x)                               ((x) << I2SR0_CTL_TDMRX_SYNC_SHIFT)
#define I2SR0_CTL_PCMRX0_EN                                   BIT(6) /* PCM TX ENABEL */
#define I2SR0_CTL_RXWIDTH_SHIFT                               (4) /* effective width */
#define I2SR0_CTL_RXWIDTH_MASK                                (0x3 << I2SR0_CTL_RXWIDTH_SHIFT)
#define I2SR0_CTL_RXWIDTH(x)                                  ((x) << I2SR0_CTL_RXWIDTH_SHIFT)

#define I2SR0_CTL_RXBCLKSET                                   BIT(3) /* rate of BCLK with LRCLK */

#define I2SR0_CTL_RXMODELSEL_SHIFT                            (1) /* I2S transfer format select */
#define I2SR0_CTL_RXMODELSEL_MASK                             (0x3 << I2SR0_CTL_RXMODELSEL_SHIFT)
#define I2SR0_CTL_RXMODELSEL(x)                               ((x) << I2SR0_CTL_RXMODELSEL_SHIFT)

#define I2SR0_CTL_RXEN                                        BIT(0) /* I2S RX enable */

/***************************************************************************************************
 * I2SRX_FIFOCTL
 */
#define I2SR0_FIFOCTL_RXFIFO_DMAWIDTH                         BIT(7) /* DMA transfer width */

#define I2SR0_FIFOCTL_RXFOS_SHIFT                             (4) /* RX FIFO output select */
#define I2SR0_FIFOCTL_RXFOS_MASK                              (0x3 << I2SR0_FIFOCTL_RXFOS_SHIFT)
#define I2SR0_FIFOCTL_RXFOS(x)                                ((x) << I2SR0_FIFOCTL_RXFOS_SHIFT)

#define I2SR0_FIFOCTL_RXFFIE                                  BIT(2) /* RX FIFO half filled irq enable */
#define I2SR0_FIFOCTL_RXFFDE                                  BIT(1) /* RX FIFO half filed drq enable */
#define I2SR0_FIFOCTL_RXFRT                                   BIT(0) /* RX FIFO reset */

/***************************************************************************************************
 * I2SRX_STAT
 */
#define I2SR0_FIFOSTA_FIFO_ER                                 BIT(8) /* FIFO error */
#define I2SR0_FIFOSTA_RXFEF                                   BIT(7) /* RX FIFO empty flag */
#define I2SR0_FIFOSTA_RXFIP                                   BIT(6) /* RX FIFO half filled irq pending */
#define I2SR0_FIFOSTA_RXFS_SHIFT                              (0) /* RX FIFO status */
#define I2SR0_FIFOSTA_RXFS_MASK                               (0x3F << I2SR0_FIFOSTA_RXFS_SHIFT)

/***************************************************************************************************
 * I2SRX_DAT
 */
#define I2SR0_DAT_RXDAT_SHIFT                                 (8) /* I2S RX FIFO data */
#define I2SR0_DAT_RXDAT_MASK                                  (0xFFFFFF << I2SR0_DAT_RXDAT_SHIFT)

/***************************************************************************************************
 * I2SRX_CTL
 */
#define I2SR0_SRDCTL_MUTE_EN                                  BIT(12) /* If detect sample rate or channel width changing, mute output */
#define I2SR0_SRDCTL_TO_IE                                    BIT(11)
#define I2SR0_SRDCTL_SRD_IE                                   BIT(10)
#define I2SR0_SRDCTL_CHW_IE                                   BIT(8) /* sample rate detect result change interrupt enable */

#define I2SR0_SRDCTL_CNT_TIM_SHIFT                            (4) /* slave mode rample rate detect counter period select */
#define I2SR0_SRDCTL_CNT_TIM_MASK                             (0x3 << I2SR0_SRDCTL_CNT_TIM_SHIFT)

#define I2SR0_SRDCTL_SRD_TH_SHIFT                             (1) /* the sampling sensitivity */
#define I2SR0_SRDCTL_SRD_TH_MASK                              (0x7 << I2SR0_SRDCTL_SRD_TH_SHIFT)
#define I2SR0_SRDCTL_SRD_TH(x)                                ((x) << I2SR0_SRDCTL_SRD_TH_SHIFT)

#define I2SR0_SRDCTL_SRD_EN                                   BIT(0) /* slave mode sample rate detect enable */

/***************************************************************************************************
 * I2SRX_SRDSTA
 */
#define I2SR0_SRDSTA_CNT_SHIFT                                (12) /* CNT of LRCLK which sampling by SRC_CLK */
#define I2SR0_SRDSTA_CNT_MASK                                 (0x1FFF << I2SR0_SRDSTA_CNT_SHIFT)

#define I2SR0_SRDSTA_TO_PD                                    BIT(11) /* sample rate changing detection timeout interrupt pending */
#define I2SR0_SRDSTA_SRC_PD                                   BIT(10) /* sample rate changing detection interrupt pending */
#define I2SR0_SRDSTA_CHW_PD                                   BIT(8) /* channel width change interrupt pending */
#define I2SR0_SRDSTA_WL_SHIFT                                 (0) /* channel word length */
#define I2SR0_SRDSTA_WL_MASK                                  (0x7 << I2SR0_SRDSTA_WL_SHIFT)

/***************************************************************************************************
 * i2SRX FEATURES CONGIURATION
 */
/* The sensitivity of the SRD */
#define I2SRX_SRD_TH_DEFAULT                                  (7)

#define I2SRX_BCLK_DIV_DEFAULT                                (0) /* BCLK divisor */

#define I2SRX_SRD_CONFIG_TIMEOUT_US                           (500000)

/*
 * enum a_i2srx_mclk_clksrc_e
 * @brief The MCLK clock source of i2srx selection
 */
typedef enum {
    CLK_SRCRX_I2SRX = 0, /* I2SRX clock source from I2SRX clk */
    CLK_SRCRX_I2STX, /* I2SRX clock source from I2STX MCLK */
    CLK_SRCRX_I2SRX_EXT = 3, /* I2SRX clock source from I2SRX extern MCLK */
} a_i2srx_mclk_clksrc_e;

/*
 * @struct acts_audio_i2srx
 * @brief I2SRX controller hardware register
 */
struct acts_audio_i2srx {
    volatile uint32_t rx_ctl; /* I2SRX control */
    volatile uint32_t fifoctl; /* I2SRX FIFO control */
    volatile uint32_t fifostat; /* I2SRX FIFO state */
    volatile uint32_t dat; /* I2SRX FIFO data */
    volatile uint32_t srdctl; /* I2S slave mode RX sample rate detect control */
    volatile uint32_t srdstat; /* I2S slave mode RX sample rate detect state */
};

/*
 * struct phy_i2srx_drv_data
 * @brief The software related data that used by physical i2srx driver.
 */
struct phy_i2srx_drv_data {
    uint8_t mclksrc; /* MCLK clock source selection refer to #a_i2srx_mclk_clksrc_e */
    int (*srd_callback)(void *cb_data, uint32_t cmd, void *param);
    void *cb_data;
    uint8_t srd_wl; /* The width length detected by SRD */
    uint8_t sample_rate; /* sample rate */
    uint8_t channel_opened : 1; /* flag of channel opened */
};

/**
 * union phy_i2srx_features
 * @brief The infomation from DTS to control the I2SRX features to enable or nor.
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t srd_en : 1; /* SRD function enable or not */
        uint32_t mode: 1; /* master mode or slaver mode; 0: master 1: slaver */
        uint32_t channel_num : 4; /* channel number and only support 2/4/8 channels */
        uint32_t slave_internal_clk : 1; /* slave mode MCLK to use internal clock */
        uint32_t lrclk_proc : 1; /* LRCLK process */
        uint32_t mclk_reverse : 1; /* mclk reverse */
        uint32_t format : 2; /* I2S transfer format */
        uint32_t clk_from_i2stx : 1; /* clock from I2STX(I2SG0) clock */
        uint32_t tdm_format : 1; /* TDM format */
        uint32_t tdm_frame : 2; /* TDM frame start position */
        uint32_t bclk_width : 1; /* bclk width */
        uint32_t pcm_en : 1; /* pcm select */
        uint32_t pcm_frame : 1; /* pcm frame */
        uint32_t pcm_slot : 2; /* pcm slot */
    } v;
} phy_i2srx_features;

/**
 * struct phy_i2srx_config_data
 * @brief The hardware related data that used by physical i2srx driver.
 */
struct phy_i2srx_config_data {
    uint32_t reg_base; /* I2STX controller register base address */
    uint8_t clk_id; /* I2STX devclk id */
    uint8_t srd_clk_id; /* I2S SRD clock id */
    uint8_t hclk_clk_id; /* I2S HCLK clock id */
    uint8_t rst_id; /* I2STX reset id */
    struct audio_dma_dt dma_fifo0; /* DMA resource for SPDIFRX */
    void (*irq_config)(void); /* IRQ configuration function */
    phy_i2srx_features features; /* I2STX features */
};


/* @brief  Get the I2SRX controller base address */
static inline struct acts_audio_i2srx *get_i2srx_reg_base(struct device *dev)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    return (struct acts_audio_i2srx *)cfg->reg_base;
}

/* @brief Dump the I2SRX relative registers */
static void i2srx_dump_register(struct device *dev)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);

    LOG_INF("** i2srx contoller regster **");
    LOG_INF("          BASE: %08x", (uint32_t)i2srx_reg);
    LOG_INF("     I2SRX_CTL: %08x", i2srx_reg->rx_ctl);
    LOG_INF(" I2SRX_FIFOCTL: %08x", i2srx_reg->fifoctl);
    LOG_INF(" I2SRX_FIFOSTA: %08x", i2srx_reg->fifostat);
    LOG_INF("     I2SRX_DAT: %08x", i2srx_reg->dat);
    LOG_INF("  I2SRX_SRDCTL: %08x", i2srx_reg->srdctl);
    LOG_INF("  I2SRX_SRDSTA: %08x", i2srx_reg->srdstat);
    LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
    LOG_INF("  CMU_I2STXCLK: %08x", sys_read32(CMU_I2STXCLK));
    LOG_INF("  CMU_I2SRXCLK: %08x", sys_read32(CMU_I2SRXCLK));
    LOG_INF(" DMA6_CTL: %08x",   sys_read32(0x4001c700));
    LOG_INF(" DMA6_START: %08x", sys_read32(0x4001c704));
    LOG_INF(" DMA6_sadd0: %08x", sys_read32(0x4001c708));
    LOG_INF(" DMA6_sadd1: %08x", sys_read32(0x4001c70c));
    LOG_INF(" DMA6_dadd0: %08x", sys_read32(0x4001c710));
    LOG_INF(" DMA6_dadd1: %08x", sys_read32(0x4001c714));
    LOG_INF(" DMA6_bc: %08x",    sys_read32(0x4001c718));
    LOG_INF(" DMA6_rc: %08x",    sys_read32(0x4001c71c));
}

/* @brief I2SRX sample rate config */
static int i2srx_sample_rate_set(struct device *dev, uint16_t sr_khz,
                a_i2srx_mclk_clksrc_e mclk_src)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    int ret;
    uint8_t clk_div, series, pll_index;
    uint32_t reg = 0;
    uint32_t lrclk_div;
    a_mclk_type_e mclk;

    switch (I2SRX_BCLK_DIV_DEFAULT)
    {
    case 0:
        if (!PHY_DEV_FEATURE(bclk_width))
            mclk = MCLK_256FS;
        else
            mclk = MCLK_128FS;
        break;
    case 1:
        if (!PHY_DEV_FEATURE(bclk_width))
            mclk = MCLK_384FS;
        else
            mclk = MCLK_192FS;
        break;
    case 2:
        if (!PHY_DEV_FEATURE(bclk_width))
            mclk = MCLK_512FS;
        else
            mclk = MCLK_256FS;
        break;
    case 3:
        if (!PHY_DEV_FEATURE(bclk_width))
            mclk = MCLK_768FS;
        else
            mclk = MCLK_384FS;
        break;
    default:
        if (!PHY_DEV_FEATURE(bclk_width))
            mclk = MCLK_256FS;
        else
            mclk = MCLK_128FS;
        break;
    }


    if (CLK_SRCRX_I2STX == mclk_src) {
        struct device *i2stx_dev = (struct device *)device_get_binding(CONFIG_AUDIO_I2STX_0_NAME);
        if (!i2stx_dev) {
            LOG_ERR("failed to get i2stx dev %s", CONFIG_AUDIO_I2STX_0_NAME);
            return -ENODEV;
        }
        return phy_audio_control(i2stx_dev, PHY_CMD_I2STX_CLK_SET, &sr_khz);
    } else if (CLK_SRCRX_I2SRX == mclk_src) {

        /* make external MCLK clock phase reverse */
        if (PHY_DEV_FEATURE(mclk_reverse))
            reg |= CMU_I2SRXCLK_I2SG1MCLKEXTREV;

        /* I2S master mode to enable MCLK output to PAD */
        if (!PHY_DEV_FEATURE(mode)) {
            reg |= (CMU_I2SG1MCLKOEN | CMU_I2SG1BCLKOEN | CMU_I2SG1LRCLOKEN | CMU_I2SRXCLK_I2SRX0CLKSRC);
        } else {
            if (PHY_DEV_FEATURE(slave_internal_clk))
                sr_khz *= 2; /* slave mode use internal clk condition: MCLK > 6BCLK */
            /* BCLK/LRCLK source from external PAD */
            reg |= CMU_I2SRXCLK_I2SG1BLRCLKSRC;
        }

        reg |= CMU_I2SRXCLK_I2SG1LRCLKPROC(PHY_DEV_FEATURE(lrclk_proc));

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

        reg |= CMU_I2SRXCLK_I2SG1LRCLKDIV(lrclk_div);

        /* MCLK = 4 x BCLK */
        reg |= CMU_I2SRXCLK_I2SG1BCLKDIV(I2SRX_BCLK_DIV_DEFAULT);

        sr_khz = PHY_DEV_FEATURE(channel_num) / 2 * sr_khz;

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

        /* Select pll0 or pll1 */
        // reg |= (pll_index & 0x1) << CMU_I2SRXCLK_I2SG1CLKSRC;
        reg |= clk_div << CMU_I2SRXCLK_I2SG1CLKDIV_SHIFT;

    } else if (CLK_SRCRX_I2SRX_EXT == mclk_src) {
        LOG_INF("I2SRX clock source from external");
        /* BCLK/LRCLK source from external PAD */
        // reg |= CMU_I2SRXCLK_I2SG1BLRCLKSRC;//RX CLOCK来自G0
        reg |= CMU_I2SRXCLK_I2SG1BLRCLKSRC | CMU_I2SRXCLK_I2SRX0CLKSRC;//CLOCK来自G1
    } else {
        LOG_ERR("Invalid i2srx clk source %d", mclk_src);
        return -EINVAL;
    }

    /* Select the i2srx mclk source */
    reg |= CMU_I2SRXCLK_I2SG1MCLKSRC(mclk_src);
    sys_write32(reg, CMU_I2SRXCLK);

    return 0;
}

/* @brief Get the sample rate from the I2SRX config */
static int i2srx_sample_rate_get(struct device *dev)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    uint8_t clk_div, pll_index, mclk_src;
    uint32_t reg;
    int ret = -1;
    a_mclk_type_e mclk;

    if (!PHY_DEV_FEATURE(bclk_width))
        mclk = MCLK_256FS;
    else
        mclk = MCLK_128FS;

    reg = sys_read32(CMU_I2SRXCLK);

    mclk_src = (reg & CMU_I2SRXCLK_I2SG1MCLKSRC_MASK) >> CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT;

    if (CLK_SRCRX_I2SRX == mclk_src) {
        clk_div = reg & 0xF;
        pll_index = 0;//leopard 只有一个PLL0源
        ret = audio_get_pll_sample_rate_i2s(MCLK_256FS, clk_div, pll_index);
    } else if (CLK_SRCRX_I2STX == mclk_src) {
        reg = sys_read32(CMU_I2STXCLK);
        clk_div = reg & 0xF;
        pll_index = 0;
        ret = audio_get_pll_sample_rate_i2s(mclk, clk_div, pll_index);
    } else if (CLK_SRCRX_I2SRX_EXT == mclk_src) {
        LOG_INF("I2SRX is using the external clock");
        ret = -ENOENT;
    }

    return ret;
}

/* @brief Get the AUDIO_PLL APS used by I2SRX */
static int i2srx_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index, mclk_src;
    int ret = -1;

    reg = sys_read32(CMU_I2SRXCLK);

    mclk_src = (reg & CMU_I2SRXCLK_I2SG1MCLKSRC_MASK) >> CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT;
    pll_index = AUDIOPLL_TYPE_0;//leopard 只有一个PLL0源
    if (CLK_SRCRX_I2SRX == mclk_src) {
        ret = audio_pll_get_aps((a_pll_type_e)pll_index);
    } else if (CLK_SRCRX_I2STX == mclk_src) {
        reg = sys_read32(CMU_I2STXCLK);
        ret = audio_pll_get_aps((a_pll_type_e)pll_index);
    }if (CLK_SRCRX_I2SRX_EXT == mclk_src) {
        LOG_INF("I2SRX is using the external clock source");
        return -ENOENT;
    }

    return ret;
}

/* @brief Set the AUDIO_PLL APS used by I2SRX */
static int i2srx_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index, mclk_src;
    int ret = -1;

    reg = sys_read32(CMU_I2SRXCLK);

    mclk_src = (reg & CMU_I2SRXCLK_I2SG1MCLKSRC_MASK) >> CMU_I2SRXCLK_I2SG1MCLKSRC_SHIFT;

    if (CLK_SRCRX_I2SRX == mclk_src) {
        pll_index = AUDIOPLL_TYPE_0;//leopard 只有一个PLL0源
        ret = audio_pll_set_aps((a_pll_type_e)pll_index, level);
    } else if (CLK_SRCRX_I2STX == mclk_src) {
        reg = sys_read32(CMU_I2STXCLK);
        pll_index = AUDIOPLL_TYPE_0;
        ret = audio_pll_set_aps((a_pll_type_e)pll_index, level);
    } else if (CLK_SRCRX_I2SRX_EXT == mclk_src) {
        LOG_INF("I2SRX is using the external clock source");
        ret = -ENOENT;
    }

    return ret;
}

/* @brief  Disable the I2SRX FIFO */
static void i2srx_fifo_disable(struct device *dev)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    i2srx_reg->fifoctl &= ~I2SR0_CTL_RXEN;
}

/* @brief  Reset the I2SRX FIFO */
static void i2srx_fifo_reset(struct device *dev)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    i2srx_reg->fifoctl &= ~I2SR0_CTL_RXEN;
    i2srx_reg->fifoctl |= I2SR0_CTL_RXEN;
}

/* @brief  Enable the I2SRX FIFO */
static void i2srx_fifo_enable(struct device *dev, audio_fifouse_sel_e sel, audio_dma_width_e width)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    uint32_t reg = 0;

    if (DMA_WIDTH_16BITS == width)
        reg |= I2SR0_FIFOCTL_RXFIFO_DMAWIDTH;

    if (FIFO_SEL_CPU == sel) {
        reg |= (I2SR0_FIFOCTL_RXFFIE | I2SR0_FIFOCTL_RXFRT);
    } else if (FIFO_SEL_DMA == sel) {
        reg |= (I2SR0_FIFOCTL_RXFFDE | I2SR0_FIFOCTL_RXFRT
                | I2SR0_FIFOCTL_RXFOS(1));
    } else if (FIFO_SEL_DSP == sel) {
        reg |= I2SR0_FIFOCTL_RXFOS(3) | I2SR0_FIFOCTL_RXFRT;
    } else {
        LOG_ERR("invalid fifo sel %d", sel);
    }
    i2srx_reg->fifoctl = reg;
}

/* @brief  I2SRX digital function control */
static void i2srx_digital_enable(struct device *dev, audio_ch_width_e width)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    uint32_t reg = 0;
    uint8_t rx_width, fmt = PHY_DEV_FEATURE(format);
    uint8_t pcm_frame = PHY_DEV_FEATURE(pcm_frame);
    uint8_t pcm_slot = PHY_DEV_FEATURE(pcm_slot);
    if (CHANNEL_WIDTH_16BITS == width)
        rx_width = 0;
    else if (CHANNEL_WIDTH_20BITS == width)
        rx_width = 1;
    else
        rx_width = 2;

    if (PHY_DEV_FEATURE(channel_num) != 2) {
        fmt = 3; /* TDM mode */
        reg &= ~ (0xF << I2SR0_CTL_TDMRX_SYNC_SHIFT);

        if (PHY_DEV_FEATURE(channel_num) == 8)
            reg |= I2SR0_CTL_TDMRX_CHAN;

        if (PHY_DEV_FEATURE(tdm_format))
            reg |= I2SR0_CTL_TDMRX_MODE;

        reg |= I2SR0_CTL_TDMRX_SYNC(PHY_DEV_FEATURE(tdm_frame));
    }

    reg |= (I2SR0_CTL_RXMODELSEL(fmt) | I2SR0_CTL_RXWIDTH(rx_width));

    if (PHY_DEV_FEATURE(bclk_width))
        reg |= I2SR0_CTL_RXBCLKSET;

    i2srx_reg->rx_ctl = reg;

    /* PCM RX enable */
    if(PHY_DEV_FEATURE(pcm_en))
    {
        i2srx_reg->rx_ctl |= I2SR0_CTL_PCM_CHAN(pcm_slot);
        i2srx_reg->rx_ctl |= I2SR0_CTL_PCMRX0_EN;
          //pcm frame set  1 is long frame
        if(pcm_frame)
        {
            i2srx_reg->rx_ctl |= I2SR0_CTL_PCM_SYNC;
            sys_write32(sys_read32(CMU_I2SRXCLK)|
                CMU_I2SRXCLK_I2SG1LRCLKPROC(2),
                CMU_I2SRXCLK);
        }
        else
            sys_write32(sys_read32(CMU_I2SRXCLK)|
                CMU_I2SRXCLK_I2SG1LRCLKPROC(3),
                CMU_I2SRXCLK);

    }
    else
        /* I2S RX enable */
        i2srx_reg->rx_ctl |= I2SR0_CTL_RXEN;
}

/* @brief  Disable I2SRX digital function */
static void i2srx_digital_disable(struct device *dev)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    i2srx_reg->rx_ctl = 0;
}

/* @brief get the I2SRX sample rate detect counter */
static uint32_t read_i2srx_srd_count(struct device *dev)
{
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    /* CNT of LRCLK which sampling by SRC_CLK */
    return ((i2srx_reg->srdstat & I2SR0_SRDSTA_CNT_MASK) >> I2SR0_SRDSTA_CNT_SHIFT);
}

/* @brief I2SRX in slave mode handle sample rate detect change */
static void i2srx_srd_fs_change(struct device *dev)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct phy_i2srx_drv_data *data = dev->data;
    uint32_t cnt, fs;
    audio_sr_sel_e sr;
    cnt = read_i2srx_srd_count(dev);

    /* CNT = SRD_CLK / LRCLK and SRD_CLK uses HOSC which is a 32MHz clock source*/
    fs = 32 *1000 / cnt;

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
    i2srx_fifo_reset(dev);

    if (data->sample_rate != sr && PHY_DEV_FEATURE(slave_internal_clk)) {
        i2srx_sample_rate_set(dev, sr, data->mclksrc);
        data->sample_rate = sr;
    }

    if (data->srd_callback)
        data->srd_callback(data->cb_data, I2SRX_SRD_FS_CHANGE, (void *)&sr);
}

/* @brief I2SRX SRD width length change operation */
void i2srx_srd_wl_change(struct device *dev)
{
    struct phy_i2srx_drv_data *data = dev->data;
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    uint8_t width;

    width = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;

    LOG_DBG("Detect new width length: %d", width);

    if (((data->srd_wl == SRDSTA_WL_64RATE) && (width == SRDSTA_WL_32RATE))
        || ((data->srd_wl == SRDSTA_WL_32RATE) && (width == SRDSTA_WL_64RATE))) {
        data->srd_wl = width;
        if (data->srd_callback)
            data->srd_callback(data->cb_data, I2SRX_SRD_WL_CHANGE, (void *)&data->srd_wl);
    }
}

/* @brief  I2SRX sample rate detect function configuration */
static int i2srx_srd_cfg(struct device *dev, uint8_t srd_th, audio_i2s_srd_period_e period, bool irq_en, bool mute)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    uint32_t reg;
    uint8_t _wl, wl;
    uint32_t start_time, curr_time;

    reg = (I2SR0_SRDCTL_SRD_TH(srd_th & 0x7) | I2SR0_SRDCTL_SRD_TH(period));
    if (irq_en)
        reg |= I2SR0_SRDCTL_CHW_IE | I2SR0_SRDCTL_SRD_IE | I2SR0_SRDCTL_TO_IE;
    if (mute)
        reg |= I2SR0_SRDCTL_MUTE_EN;

    i2srx_reg->srdctl = reg;

    /* enable slave mode sample rate detect */
    i2srx_reg->srdctl |= I2SR0_SRDCTL_SRD_EN;

    if (!PHY_DEV_FEATURE(bclk_width))
        wl = SRDSTA_WL_64RATE;
    else
        wl = SRDSTA_WL_32RATE;

    start_time = k_cycle_get_32();
    _wl = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;
    while (_wl != wl) {
        curr_time = k_cycle_get_32();
        if (k_cyc_to_us_floor32(curr_time - start_time) > I2SRX_SRD_CONFIG_TIMEOUT_US) {
            LOG_ERR("Wait SRD WL status timeout");
            return -ETIMEDOUT;
        }
        _wl = i2srx_reg->srdstat & I2SR0_SRDSTA_WL_MASK;
        k_sleep(K_MSEC(2));
    }
    i2srx_reg->srdstat |= I2SR0_SRDSTA_CHW_PD;

    i2srx_srd_fs_change(dev);

    return 0;
}

// static int set_gpio_mfc(int type, int gpio_m_num,int gpio__b_num,int gpio_lr_num,int gpio_io_num)
// {
//     if(type==0)
//     {
//         //i2s tx gpio_mfc_config
//         sys_write32(0x0000100c,GPIO_REG_BASE + gpio_m_num * 0x4);
//         sys_write32(0x0000100c,GPIO_REG_BASE + gpio__b_num * 0x4);
//         sys_write32(0x0000100c,GPIO_REG_BASE + gpio_lr_num * 0x4);
//         sys_write32(0x0000100c,GPIO_REG_BASE + gpio_io_num * 0x4);
//         printk("%08x\n",(sys_read32(GPIO_REG_BASE + gpio_m_num * 0x4)));
//         printk("%08x\n",(sys_read32(GPIO_REG_BASE + gpio__b_num * 0x4)));
//         printk("%08x\n",(sys_read32(GPIO_REG_BASE + gpio_lr_num * 0x4)));
//         printk("%08x\n",(sys_read32(GPIO_REG_BASE + gpio_io_num * 0x4)));
//     }
//     else
//     {
//         //i2s rx gpio_mfc_config
//         sys_write32(0x0000100d,GPIO_REG_BASE + gpio_m_num * 0x4);
//         sys_write32(0x0000100d,GPIO_REG_BASE + gpio__b_num * 0x4);
//         sys_write32(0x0000100d,GPIO_REG_BASE + gpio_lr_num * 0x4);
//         sys_write32(0x0000100d,GPIO_REG_BASE + gpio_io_num * 0x4);
//         printk("%08x\n",*((volatile unsigned int*)(GPIO_REG_BASE + gpio_m_num * 0x4)));
//         printk("%08x\n",*((volatile unsigned int*)(GPIO_REG_BASE + gpio__b_num * 0x4)));
//         printk("%08x\n",*((volatile unsigned int*)(GPIO_REG_BASE + gpio_lr_num * 0x4)));
//         printk("%08x\n",*((volatile unsigned int*)(GPIO_REG_BASE + gpio_io_num * 0x4)));
//     }
//     return 0;
// }
/* @brief physical I2SRX device enable */
static int phy_i2srx_enable(struct device *dev, void *param)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct phy_i2srx_drv_data *data = dev->data;
    ain_param_t *in_param = (ain_param_t *)param;
    i2srx_setting_t *i2srx_setting = in_param->i2srx_setting;
    soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, true);
    if ((!in_param) || (!i2srx_setting)
        || (!in_param->sample_rate)) {
        LOG_ERR("Invalid parameters");
        soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
        return -EINVAL;
    }

    if (in_param->channel_type != AUDIO_CHANNEL_I2SRX) {
        LOG_ERR("Invalid channel type %d", in_param->channel_type);
        soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
        return -EINVAL;
    }

    /* enable adc clock */
    acts_clock_peripheral_enable(cfg->clk_id);
    acts_clock_peripheral_enable(cfg->hclk_clk_id);

    if (PHY_DEV_FEATURE(srd_en))
        acts_clock_peripheral_enable(cfg->srd_clk_id);

    /* slave mode and clock from external source */
    if (PHY_DEV_FEATURE(mode)
        && !PHY_DEV_FEATURE(slave_internal_clk)) {
        data->mclksrc = CLK_SRCRX_I2SRX_EXT;
    } else if (PHY_DEV_FEATURE(clk_from_i2stx)) {
        if (PHY_DEV_FEATURE(mode)) {
            LOG_ERR("clock source form I2STX shall work in master mode");
            soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
            return -EPERM;
        }
        data->mclksrc = CLK_SRCRX_I2STX;
    } else {
        data->mclksrc = CLK_SRCRX_I2SRX;
    }

    /* I2SRX sample rate set */
    if (i2srx_sample_rate_set(dev, in_param->sample_rate, data->mclksrc)) {
        LOG_ERR("Failed to config sample rate %d",
            in_param->sample_rate);
        soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
        return -ESRCH;
    }
       /*enable txfifo clock*/
    sys_write32(sys_read32(CMU_I2SRXCLK) | CMU_I2SRX0_FIFOCLKEN, CMU_I2SRXCLK);
    i2srx_fifo_enable(dev, FIFO_SEL_DMA, (in_param->channel_width == CHANNEL_WIDTH_16BITS)
                        ? DMA_WIDTH_16BITS : DMA_WIDTH_32BITS);

    i2srx_digital_enable(dev, in_param->channel_width);

    if (PHY_DEV_FEATURE(mode) && PHY_DEV_FEATURE(srd_en)) {
        LOG_INF("I2SRX SRD enable");
        i2srx_srd_cfg(dev, I2SRX_SRD_TH_DEFAULT, I2S_SRD_2LRCLK, true, false);
        data->srd_callback = i2srx_setting->srd_callback;
        data->cb_data = i2srx_setting->cb_data;
        if (!PHY_DEV_FEATURE(bclk_width))
            data->srd_wl = SRDSTA_WL_64RATE;
        else
            data->srd_wl = SRDSTA_WL_32RATE;
    }

    data->channel_opened = 1;
    // set_gpio_mfc(1,16,17,21,15);//rx
    return 0;
}

/* @brief physical I2SRX device disable */
static int phy_i2srx_disable(struct device *dev, void *param)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct phy_i2srx_drv_data *data = dev->data;

    i2srx_fifo_disable(dev);

    i2srx_digital_disable(dev);

    if (PHY_DEV_FEATURE(srd_en))
        acts_clock_peripheral_disable(cfg->srd_clk_id);

    data->srd_callback = NULL;
    data->cb_data = NULL;
    if (!PHY_DEV_FEATURE(bclk_width))
        data->srd_wl = SRDSTA_WL_64RATE;
    else
        data->srd_wl = SRDSTA_WL_32RATE;

    data->sample_rate = 0;
    data->channel_opened = 0;

    acts_clock_peripheral_disable(cfg->clk_id);
    soc_powergate_set(POWERGATE_DSP_AU_PG_DEV, false);
    return 0;
}

/* @brief physical I2SRX IO commands */
static int phy_i2srx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct phy_i2srx_drv_data *data = dev->data;
    int ret = 0;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        i2srx_dump_register(dev);
        break;
    }
    case AIN_CMD_I2SRX_QUERY_SAMPLE_RATE:
    {
        *(audio_sr_sel_e *)param = data->sample_rate;
        break;
    }
    case PHY_CMD_I2SRX_IS_OPENED:
    {
        *(uint8_t *)param = data->channel_opened;
        break;
    }
    case AIN_CMD_GET_SAMPLERATE:
    {
        ret = i2srx_sample_rate_get(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get I2SRX sample rate err=%d", ret);
            return ret;
        }
        *(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
        ret = 0;
        break;
    }
    case AIN_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = i2srx_sample_rate_set(dev, val, data->mclksrc);
        if (ret) {
            LOG_ERR("Failed to set I2SRX sample rate err=%d", ret);
            return ret;
        }
        break;
    }
    case AIN_CMD_GET_APS:
    {
        ret = i2srx_get_pll_aps(dev);
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
        ret = i2srx_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS err=%d", ret);
            return ret;
        }
        LOG_DBG("set new aps level %d", level);
        break;
    }
    case PHY_CMD_GET_AIN_DMA_INFO:
    {
        struct audio_out_dma_info *info = (struct audio_out_dma_info *)param;
        info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
        info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
        info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
        break;
    }
    default:
        LOG_ERR("Unsupport command %d", cmd);
        return -ENOTSUP;
    }

    return ret;
}

const struct phy_audio_driver_api phy_i2srx_drv_api = {
    .audio_enable = phy_i2srx_enable,
    .audio_disable = phy_i2srx_disable,
    .audio_ioctl = phy_i2srx_ioctl,
};

/* dump i2srx device tree infomation */
static void __i2srx_dt_dump_info(const struct phy_i2srx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     I2SRX BASIC INFO     **");
    LOG_INF("     BASE: %08x", cfg->reg_base);
    LOG_INF("   CLK-ID: %08x", cfg->clk_id);
    LOG_INF("SRDCLK-ID: %08x", cfg->srd_clk_id);
    LOG_INF("   RST-ID: %08x", cfg->rst_id);
    LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
    LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
    LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

    LOG_INF("** 	I2SRX FEATURES	 **");
    LOG_INF("        SRD-EN: %d", PHY_DEV_FEATURE(srd_en));
    LOG_INF("          MODE: %d", PHY_DEV_FEATURE(mode));
    LOG_INF("    BCLK-WIDTH: %d", PHY_DEV_FEATURE(bclk_width));
    LOG_INF("        CH-NUM: %d", PHY_DEV_FEATURE(channel_num));
    LOG_INF("  EXTERNAL-CLK: %d", PHY_DEV_FEATURE(slave_internal_clk));
    LOG_INF("    LRCLK-PROC: %d", PHY_DEV_FEATURE(lrclk_proc));
    LOG_INF("    TDM-FORMAT: %d", PHY_DEV_FEATURE(tdm_format));
    LOG_INF("     TDM-FRAME: %d", PHY_DEV_FEATURE(tdm_frame));
    LOG_INF("  MCLK-REVERSE: %d", PHY_DEV_FEATURE(mclk_reverse));
    LOG_INF("CLK-FROM-I2STX: %d", PHY_DEV_FEATURE(clk_from_i2stx));
#endif
}

static int phy_i2srx_init(const struct device *dev)
{
    const struct phy_i2srx_config_data *cfg = dev->config;
    struct phy_i2srx_drv_data *data = dev->data;

    /* clear driver data */
    memset(data, 0, sizeof(struct phy_i2srx_drv_data));

    __i2srx_dt_dump_info(cfg);

    /* reset I2SRX controller */
    acts_reset_peripheral(cfg->rst_id);
    acts_reset_peripheral(RESET_ID_ADC);//REST ADC
    if (cfg->irq_config)
        cfg->irq_config();

    printk("I2SRX init successfully\n");

    return 0;
}

void phy_i2srx_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
    struct phy_i2srx_drv_data *data = dev->data;

    LOG_DBG("srdstat: 0x%x", i2srx_reg->srdstat);

    /* Sample rate detection timeout irq pending */
    if (i2srx_reg->srdstat & I2SR0_SRDSTA_TO_PD) {
        i2srx_reg->srdstat |= I2SR0_SRDSTA_TO_PD;
        if (data->srd_callback)
            data->srd_callback(data->cb_data, I2SRX_SRD_TIMEOUT, NULL);
    }

    /* Sample rate changed detection irq pending */
    if (i2srx_reg->srdstat & I2SR0_SRDSTA_SRC_PD) {
        i2srx_reg->srdstat |= I2SR0_SRDSTA_SRC_PD;
        i2srx_srd_fs_change(dev);
    }

    /* Channel width change irq pending */
    if (i2srx_reg->srdstat & I2SR0_SRDSTA_CHW_PD) {
        i2srx_reg->srdstat |= I2SR0_SRDSTA_CHW_PD;
        i2srx_srd_wl_change(dev);
    }
}

static void phy_i2srx_irq_config(void);

/* physical i2stx driver data */
static struct phy_i2srx_drv_data phy_i2srx_drv_data0;


/* physical i2stx config data */
static const struct phy_i2srx_config_data phy_i2srx_config_data0 = {
    .reg_base = AUDIO_I2SRX0_REG_BASE,
    .clk_id = CLOCK_ID_I2SRX,
    .srd_clk_id = CLOCK_ID_I2SSRDCLK,
    .hclk_clk_id = CLOCK_ID_I2SHCLKEN,
    .rst_id = RESET_ID_I2SRX,
    AUDIO_DMA_FIFO_DEF(I2SRX, 0),
    .irq_config = phy_i2srx_irq_config,

    PHY_DEV_FEATURE_DEF(srd_en) = CONFIG_AUDIO_I2SRX_0_SRD_EN,
    PHY_DEV_FEATURE_DEF(mode) = CONFIG_AUDIO_I2SRX_0_MODE,
    PHY_DEV_FEATURE_DEF(bclk_width) = CONFIG_AUDIO_I2SRX_0_BCLK_WIDTH,
    PHY_DEV_FEATURE_DEF(channel_num) = CONFIG_AUDIO_I2SRX_0_CHANNEL_NUM,
    PHY_DEV_FEATURE_DEF(slave_internal_clk) = CONFIG_AUDIO_I2SRX_0_SLAVE_INTERNAL_CLK,
    PHY_DEV_FEATURE_DEF(lrclk_proc) = CONFIG_AUDIO_I2SRX_0_LRCLK_PROC,
    PHY_DEV_FEATURE_DEF(tdm_format) = CONFIG_AUDIO_I2SRX_0_TDM_FORMAT,
    PHY_DEV_FEATURE_DEF(tdm_frame) = CONFIG_AUDIO_I2SRX_0_TDM_FRAME,
    PHY_DEV_FEATURE_DEF(mclk_reverse) = CONFIG_AUDIO_I2SRX_0_MCLK_REVERSE,
    PHY_DEV_FEATURE_DEF(clk_from_i2stx) = CONFIG_AUDIO_I2SRX_0_CLK_FROM_I2STX,
    PHY_DEV_FEATURE_DEF(pcm_en) = CONFIG_AUDIO_PCMRX_0_EN,
    PHY_DEV_FEATURE_DEF(pcm_frame) = CONFIG_AUDIO_PCMRX_0_FORMART,
    PHY_DEV_FEATURE_DEF(pcm_slot) = CONFIG_AUDIO_PCMRX_0_SLOT,
};

#if IS_ENABLED(CONFIG_AUDIO_I2SRX_0)
DEVICE_DEFINE(i2srx0, CONFIG_AUDIO_I2SRX_0_NAME, phy_i2srx_init, NULL,
        &phy_i2srx_drv_data0, &phy_i2srx_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_i2srx_drv_api);
#endif

/*
 * @brief Enable I2SRX IRQ
 * @note I2STX IRQ source as shown below:
 *	- I2STX FIFO Half Filled IRQ
 *	- I2STX SRDTO IRQ
 *	- I2STX SRDSR IRQ
 *	- I2STX SRDCHW IRQ
 */
static void phy_i2srx_irq_config(void)
{
    IRQ_CONNECT(IRQ_ID_I2S1, CONFIG_AUDIO_I2SRX_0_IRQ_PRI,
            phy_i2srx_isr,
            DEVICE_GET(i2srx0), 0);
    irq_enable(IRQ_ID_I2S1);
}

