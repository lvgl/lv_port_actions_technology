/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio SPDIFTX physical implementation
 */

/*
 * Features
 *    - 32 level * 24bits FIFO
 *    - Support multiple devices (DAC + I2STX0 + SPDIFTX)
 *    - Sample rate support 32k/44.1k/48k/88.2k/96k/176.4k/192k
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include <drivers/audio/audio_out.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(spdiftx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * SPDIFTX_CTL
 */
#define SPDIFT0_CTL_FIFO_SEL_SHIFT                       (3)
#define SPDIFT0_CTL_FIFO_SEL_MASK                        (0x3 << SPDIFT0_CTL_FIFO_SEL_SHIFT)
#define SPDIFT0_CTL_FIFO_SEL(x)                          ((x) << SPDIFT0_CTL_FIFO_SEL_SHIFT) /* 0: DAC FIFO1; 1: I2STX FIFO 2: DAC FIFO0 + DAC FIFO1 */
#define SPDIFT0_CTL_VALIDITY                             BIT(2) /* validity flag */
#define SPDIFT0_CTL_SPD_DIS_CTL                          BIT(1) /* 0: close spdif immediately; 1: close spidf after right channel has stopped */
#define SPDIFT0_CTL_SPDEN                                BIT(0) /* SPDIFTX Enable */

/***************************************************************************************************
 * SPDIFTX_CSL
 */
#define SPDIFT0_CSL_SPDCSL_E                             (31) /* SPDIFTX Channel State Low */
#define SPDIFT0_CSL_SPDCSL_SHIFT                         (0)
#define SPDIFT0_CSL_SPDCSL_MASK                          (0xFFFFFFFF << SPDIFT0_CSL_SPDCSL_SHIFT)

/***************************************************************************************************
 * SPDIFTX_CSH
 */
#define SPDIFT0_CSH_SPDCSH_E                             (15) /* SPDIFTX Channel State High */
#define SPDIFT0_CSH_SPDCSH_SHIFT                         (0)
#define SPDIFT0_CSH_SPDCSH_MASK                          (0xFFFF << SPDIFT0_CSH_SPDCSH_SHIFT)

/***************************************************************************************************
 * SPDIFTX FEATURES CONFIGURATION
 */
#define SPDTX_CTL_SPDEN						             BIT(0)
#define SPDTX_CTL_SPD_DIS_CTL                            BIT(1)
#define SPDTX_CTL_VALIDITY                               BIT(2)

/* test spdif channel state */
#define TEST_SPDTX_CSL                                   (0x12345678)
#define TEST_SPDTX_CSH                                   (0x9abc)

/* spdiftx FIFO source selection */
#define SPDIFTX_FIFO_SRC_DACFIFO1                        (0)
#define SPDIFTX_FIFO_SRC_I2STXFIFO                       (1)
#define SPDIFTX_FIFO_SRC_DACFIFO0_1                      (2) /* Only used in linkage with DAC mode */

/*
 * enum a_spdiftx_clksrc_e
 * @brief The SPDIFTX clock source selection
 */
typedef enum {
    CLK_SRCTX_DAC_256FS_DIV2 = 0, /* DAC_256FS_CLK / 2 */
    CLK_SRCTX_I2STX_MCLK, /* I2STX_MCLK  */
    CLK_SRCTX_I2STX_MCLK_DIV2, /* I2STX_MCLK / 2 */
    CLK_SRCTX_SPDTXCLK /* SPDIFTX CLK */
} a_spdiftx_clksrc_e;

/*
 * @struct acts_audio_spdiftx
 * @brief SPDIFTX controller hardware register
 */
struct acts_audio_spdiftx {
    volatile uint32_t ctl; /* SPDIFTX Control */
    volatile uint32_t csl; /* SPDIFTX Channel State Low */
    volatile uint32_t csh; /* SPDIFTX Channel State High */
};

/**
 * union phy_spdiftx_features
 * @brief The infomation from DTS to control the SPDIFTX features to enable or nor.
 */
typedef union {
    uint8_t raw;
    struct {
        uint8_t clk_i2stx_div2 : 1; /* SPDIFTX clock from I2STX div2 clock */
    } v;
} phy_spdiftx_features;


/**
 * struct phy_spdiftx_drv_data
 * @brief The software related data that used by physical spdiftx driver.
 */
struct phy_spdiftx_drv_data {
    uint8_t clksrc; /* spdiftx clock source selection */
    uint8_t fifo_sel;             /* Record the used FIFO */
    uint8_t linkage_mode : 1; /* The flag of linkage with i2stx */
};

/**
 * struct phy_spdiftx_config_data
 * @brief The hardware related data that used by physical spdiftx driver.
 */
struct phy_spdiftx_config_data {
    uint32_t reg_base; /* SPDIFTX controller register base address */
    uint8_t clk_id; /* SPDIFTX devclk id */
    uint8_t rst_id; /* SPDIFTX reset id */
    phy_spdiftx_features features; /* SPDIFTX features */
};

/* @brief get the base address of SPDIFTX register */
static inline struct acts_audio_spdiftx *get_spdiftx_reg_base(struct device *dev)
{
    const struct phy_spdiftx_config_data *cfg = dev->config;
    return (struct acts_audio_spdiftx *)cfg->reg_base;
}

/* @brief dump spdiftx controller register */
static void spdiftx_dump_register(struct device *dev)
{
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);

    LOG_INF("** spdiftx contoller regster **");
    LOG_INF("          BASE: %08x", (uint32_t)spdiftx_base);
    LOG_INF("   SPDIFTX_CTL: %08x", spdiftx_base->ctl);
    LOG_INF("   SPDIFTX_CSL: %08x", spdiftx_base->csl);
    LOG_INF("   SPDIFTX_CSH: %08x", spdiftx_base->csh);
    LOG_INF(" AUDIOPLL0_CTL: %08x", sys_read32(AUDIO_PLL0_CTL));
    LOG_INF(" AUDIOPLL1_CTL: %08x", sys_read32(AUDIO_PLL1_CTL));
    LOG_INF("    CMU_DACCLK: %08x", sys_read32(CMU_DACCLK));
    LOG_INF("  CMU_I2STXCLK: %08x", sys_read32(CMU_I2STXCLK));
    LOG_INF("   CMU_SPDIFTX: %08x", sys_read32(CMU_SPDIFTXCLK));
}

/* @brief SPDIFTX sample rate config */
static int spdiftx_sample_rate_set(struct device *dev, u16_t sr_khz,
                                        a_spdiftx_clksrc_e clksrc, bool direct_set)
{
    int ret;
    uint8_t pre_div, clk_div, series, pll_index;
    uint32_t reg, reg1;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_SPDIFTXCLK) & ~CMU_SPDIFTXCLK_SPDIFTXCLKSRC_MASK;

    if (direct_set)
        goto out;

    /* Get audio PLL setting */
    if (CLK_SRCTX_I2STX_MCLK == clksrc) {
        ret = audio_get_pll_setting_i2s(sr_khz, MCLK_128FS, &clk_div, &series);
    } else {
        if (CLK_SRCTX_DAC_256FS_DIV2 == clksrc || (CLK_SRCTX_I2STX_MCLK_DIV2 == clksrc))
            ret = audio_get_pll_setting(sr_khz, MCLK_256FS, &pre_div, &clk_div, &series);
        else
            ret = audio_get_pll_setting(sr_khz, MCLK_128FS, &pre_div, &clk_div, &series);
    }
    if (ret) {
        LOG_ERR("get pll setting error:%d", ret);
        return ret;
    }

    /* Check the pll usage and then config */
    ret = audio_pll_check_config(series, &pll_index);
    if (ret) {
        LOG_ERR("check pll config error:%d", ret);
        return ret;
    }

    if (CLK_SRCTX_DAC_256FS_DIV2 == clksrc) {
        reg1 = sys_read32(CMU_DACCLK) & ~0x1FF;
        /* Select pll0 or pll1 */
        reg1 |= (pll_index & 0x1) << CMU_DACCLK_DACCLKSRC;
        reg1 |=  (pre_div << CMU_DACCLK_DACCLKPREDIV) | (clk_div << CMU_DACCLK_DACCLKDIV_SHIFT);
        sys_write32(reg1, CMU_DACCLK);
    } else if ((CLK_SRCTX_I2STX_MCLK == clksrc)
        || (CLK_SRCTX_I2STX_MCLK_DIV2 == clksrc)) {
        reg1 = sys_read32(CMU_I2STXCLK) & ~0xFFFF;
        /* Select pll0 or pll1 */
        reg1 |= (pll_index & 0x1) << CMU_I2STXCLK_I2SG0CLKSRC;
        reg1 |= clk_div << CMU_I2STXCLK_I2SG0CLKDIV_SHIFT;
        sys_write32(reg1, CMU_I2STXCLK);
    } else if (CLK_SRCTX_SPDTXCLK == clksrc) {
        reg = sys_read32(CMU_SPDIFTXCLK) & ~0x3FFF;
        reg |= 3 << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT;
        reg |= (pll_index & 0x1) << CMU_SPDIFTXCLK_SPDTXCLKSRC;
        reg |= (pre_div << CMU_SPDIFTXCLK_SPDTXCLKPREDIV) | (clk_div << CMU_SPDIFTXCLK_SPDTXCLKDIV_SHIFT);
    } else {
        LOG_ERR("Invalid spdiftx clk source %d", clksrc);
        return -EINVAL;
    }

out:
    sys_write32(reg | (clksrc << CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT), CMU_SPDIFTXCLK);

    return 0;
}

/* @brief Get the sample rate from the SPDIFTX config */
static int spdiftx_sample_rate_get(struct device *dev)
{
    uint8_t pre_div, clk_div, pll_index;
    uint32_t reg = sys_read32(CMU_SPDIFTXCLK);
    uint8_t clk_src;

    ARG_UNUSED(dev);

    clk_src = (reg & CMU_SPDIFTXCLK_SPDIFTXCLKSRC_MASK) >> CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT;

    if ((clk_src == CLK_SRCTX_I2STX_MCLK) || (clk_src == CLK_SRCTX_I2STX_MCLK_DIV2)) {
        reg = sys_read32(CMU_I2STXCLK);
        clk_div = reg & 0xF;
        pll_index = (reg >> CMU_I2STXCLK_I2SG0CLKSRC) & 0x1;
        return audio_get_pll_sample_rate_i2s(MCLK_128FS, clk_div, pll_index);
    } else if (clk_src == CLK_SRCTX_DAC_256FS_DIV2) {
        reg = sys_read32(CMU_DACCLK);
        pll_index = (reg >> CMU_DACCLK_DACCLKSRC) & 0x1;
        pre_div = (reg & (1 << CMU_DACCLK_DACCLKPREDIV)) >> CMU_DACCLK_DACCLKPREDIV;
        clk_div = reg & CMU_DACCLK_DACCLKDIV_MASK;
    } else {
        pll_index = (reg & (1 << CMU_SPDIFTXCLK_SPDTXCLKSRC)) >> CMU_SPDIFTXCLK_SPDTXCLKSRC;
        pre_div = (reg & (1 << CMU_SPDIFTXCLK_SPDTXCLKPREDIV)) >> CMU_SPDIFTXCLK_SPDTXCLKPREDIV;
        clk_div = reg & CMU_SPDIFTXCLK_SPDTXCLKDIV_MASK;
    }

    return audio_get_pll_sample_rate(MCLK_128FS, pre_div, clk_div, pll_index);
}

/* @brief Get the AUDIO_PLL APS used by SPDIFTX */
static int spdiftx_get_pll_aps(struct device *dev)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_SPDIFTXCLK);
    pll_index = (reg & (1 << CMU_SPDIFTXCLK_SPDTXCLKSRC)) >> CMU_SPDIFTXCLK_SPDTXCLKSRC;

    return audio_pll_get_aps((a_pll_type_e)pll_index);
}

/* @brief Set the AUDIO_PLL APS used by SPDIFTX */
static int spdiftx_set_pll_aps(struct device *dev, audio_aps_level_e level)
{
    uint32_t reg;
    uint8_t pll_index;

    ARG_UNUSED(dev);

    reg = sys_read32(CMU_SPDIFTXCLK);
    pll_index = (reg & (1 << CMU_SPDIFTXCLK_SPDTXCLKSRC)) >> CMU_SPDIFTXCLK_SPDTXCLKSRC;

    return audio_pll_set_aps((a_pll_type_e)pll_index, level);
}

/* @brief  Config the SPDIFTX FIFO source */
static int phy_spdiftx_fifo_config(struct device *dev, uint8_t fifo_sel)
{
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);
    uint32_t reg = spdiftx_base->ctl & ~SPDIFT0_CTL_FIFO_SEL_MASK;

    if (SPDIFTX_FIFO_SRC_DACFIFO0_1 == fifo_sel)
        reg |= SPDIFT0_CTL_FIFO_SEL(2);
    else if (SPDIFTX_FIFO_SRC_DACFIFO1 == fifo_sel)
        reg |= SPDIFT0_CTL_FIFO_SEL(0);
    else if (SPDIFTX_FIFO_SRC_I2STXFIFO == fifo_sel)
        reg |= SPDIFT0_CTL_FIFO_SEL(1);
    else
        return -EINVAL;

    spdiftx_base->ctl = reg;

    return 0;
}

/* @brief Set the SPDIFTX channel status */
static void phy_spdiftx_channel_status_set(struct device *dev, audio_spdif_ch_status_t *status)
{
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);

    if (status) {
        spdiftx_base->csl = status->csl;
        spdiftx_base->csh = (uint32_t)status->csh;
    } else {
        spdiftx_base->csl = TEST_SPDTX_CSL;
        spdiftx_base->csh = TEST_SPDTX_CSH;
    }
}

/* @brief Get the SPDIFTX channel status */
static void phy_spdiftx_channel_status_get(struct device *dev, audio_spdif_ch_status_t *status)
{
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);

    if (status) {
        status->csl = spdiftx_base->csl;
        status->csh = (uint16_t)spdiftx_base->csh;
    }
}

/* @brief physical spdiftx device enable */
static int phy_spdiftx_enable(struct device *dev, void *param)
{
    const struct phy_spdiftx_config_data *cfg = dev->config;
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);
    struct phy_spdiftx_drv_data *data = dev->data;
    aout_param_t *out_param = (aout_param_t *)param;
    bool is_linkmode;
    a_spdiftx_clksrc_e clksrc;
    int ret;
    uint8_t sr;
    struct board_pinmux_info pinmux_info;

    board_get_spdiftx0_pinmux_info(&pinmux_info);

    if (!out_param) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    sr = out_param->sample_rate;

    if (out_param->channel_type & AUDIO_CHANNEL_DAC) {
        LOG_INF("Enable linkage with DAC");
        clksrc = CLK_SRCTX_DAC_256FS_DIV2;
        data->fifo_sel = SPDIFTX_FIFO_SRC_DACFIFO0_1;
        is_linkmode = true;
    } else if (out_param->channel_type & AUDIO_CHANNEL_I2STX) {
        LOG_INF("Enable linkage with I2STX");
        /* SPDIFTX uses 128FS but I2STX uses 256FS */
        clksrc = CLK_SRCTX_I2STX_MCLK_DIV2,
        data->fifo_sel = SPDIFTX_FIFO_SRC_I2STXFIFO;
        if ((out_param->outfifo_type == AOUT_FIFO_DAC0)
            || (out_param->outfifo_type == AOUT_FIFO_DAC1)) {
            clksrc = CLK_SRCTX_DAC_256FS_DIV2;
            data->fifo_sel = SPDIFTX_FIFO_SRC_DACFIFO0_1;
        }
        is_linkmode = true;
    } else {
        /* DAC FIFO1 depends on DAC FIFO0 on working */
        if ((out_param->outfifo_type == AOUT_FIFO_DAC0)
            || (out_param->outfifo_type == AOUT_FIFO_DAC1)) {
            clksrc = CLK_SRCTX_DAC_256FS_DIV2;
            data->fifo_sel = SPDIFTX_FIFO_SRC_DACFIFO0_1;
        } else if (out_param->outfifo_type == AOUT_FIFO_DAC1_ONLY_SPDIF) {
            clksrc = CLK_SRCTX_SPDTXCLK;
            data->fifo_sel = SPDIFTX_FIFO_SRC_DACFIFO1;
        } else if (out_param->outfifo_type == AOUT_FIFO_I2STX0) {

            if (PHY_DEV_FEATURE(clk_i2stx_div2))
                clksrc = CLK_SRCTX_I2STX_MCLK_DIV2;
            else
                clksrc = CLK_SRCTX_I2STX_MCLK;

            data->fifo_sel = SPDIFTX_FIFO_SRC_I2STXFIFO;
        } else {
            LOG_ERR("Invalid fifo type:%d", out_param->outfifo_type);
            return -EINVAL;
        }

        is_linkmode = false;
    }

    data->clksrc = clksrc;
    data->linkage_mode = is_linkmode;

    /* Config the spdiftx pin state */
    acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);

    /* spdiftx clock enable */
    acts_clock_peripheral_enable(cfg->clk_id);

    ret = spdiftx_sample_rate_set(dev, sr, clksrc,
            is_linkmode ? true : false);
    if (ret) {
        LOG_ERR("Failed to set spdiftx sample rate (err=%d)", ret);
        return ret;
    }

    ret = phy_spdiftx_fifo_config(dev, data->fifo_sel);
    if (ret) {
        LOG_ERR("Failed to config spidftx fifo %d", out_param->outfifo_type);
        return ret;
    }

    phy_spdiftx_channel_status_set(dev, out_param->spdiftx_setting->status);

    if (!is_linkmode)
        spdiftx_base->ctl |= SPDTX_CTL_SPDEN;

    return 0;
}

static int phy_spdiftx_disable(struct device *dev, void *param)
{
    const struct phy_spdiftx_config_data *cfg = dev->config;
    struct acts_audio_spdiftx *spdiftx_base = get_spdiftx_reg_base(dev);
    struct phy_spdiftx_drv_data *data = dev->data;
    uint8_t i;
    struct board_pinmux_info pinmux_info;

    board_get_spdiftx0_pinmux_info(&pinmux_info);

    /* set the spdiftx pin state to gpio to save power */
    for (i = 0; i < pinmux_info.pins_num; i++)
        acts_pinmux_set(pinmux_info.pins_config[i].pin_num, 0);

    spdiftx_base->ctl &= (~SPDTX_CTL_SPDEN);

    /* spdif tx clock gating disable */
    acts_clock_peripheral_disable(cfg->clk_id);

    data->fifo_sel = AUDIO_FIFO_INVALID_TYPE;

    return 0;
}

static int phy_spdiftx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    int ret = 0;
    struct phy_spdiftx_drv_data *data = dev->data;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        spdiftx_dump_register(dev);
        break;
    }
    case AOUT_CMD_GET_SAMPLERATE:
    {
        ret = spdiftx_sample_rate_get(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get SPDIFTX sample rate (err=%d)", ret);
            return ret;
        }
        *(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
        ret = 0;
        break;
    }
    case AOUT_CMD_SET_SAMPLERATE:
    {
        audio_sr_sel_e val = *(audio_sr_sel_e *)param;
        ret = spdiftx_sample_rate_set(dev, val, data->clksrc, data->linkage_mode ? true: false);
        if (ret) {
            LOG_ERR("Failed to set I2STX sample rate (err=%d)", ret);
            return ret;
        }
        break;
    }
    case AOUT_CMD_GET_APS:
    {
        ret = spdiftx_get_pll_aps(dev);
        if (ret < 0) {
            LOG_ERR("Failed to get audio pll APS (err=%d)", ret);
            return ret;
        }
        *(audio_aps_level_e *)param = (audio_aps_level_e)ret;
        ret = 0;
        break;
    }
    case AOUT_CMD_SET_APS:
    {
        audio_aps_level_e level = *(audio_aps_level_e *)param;
        ret = spdiftx_set_pll_aps(dev, level);
        if (ret) {
            LOG_ERR("Failed to set audio pll APS (err=%d)", ret);
            return ret;
        }
        break;
    }
    case AOUT_CMD_SPDIF_SET_CHANNEL_STATUS:
    {
        audio_spdif_ch_status_t *status = (audio_spdif_ch_status_t *)param;
        if (!status) {
            LOG_ERR("Invalid parameters");
            return -EINVAL;
        }
        phy_spdiftx_channel_status_set(dev, status);
        break;
    }
    case AOUT_CMD_SPDIF_GET_CHANNEL_STATUS:
    {
        phy_spdiftx_channel_status_get(dev, (audio_spdif_ch_status_t *)param);
        break;
    }
    default:
        LOG_ERR("Unsupport command %d", cmd);
        return -ENOTSUP;
    }

    return ret;
}

const struct phy_audio_driver_api phy_spdiftx_drv_api = {
    .audio_enable = phy_spdiftx_enable,
    .audio_disable = phy_spdiftx_disable,
    .audio_ioctl = phy_spdiftx_ioctl
};

/* dump spdiftx device tree infomation */
static void __spdiftx_dt_dump_info(const struct phy_spdiftx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     SPDIFTX BASIC INFO     **");
    LOG_INF("  BASE: %08x", cfg->reg_base);
    LOG_INF("CLK-ID: %08x", cfg->clk_id);
    LOG_INF("RST-ID: %08x", cfg->rst_id);

    LOG_INF("** 	SPDIFTX FEATURES	 **");
    LOG_INF("CLK-I2STX-DIV2: %d", PHY_DEV_FEATURE(clk_i2stx_div2));
#endif
}

static int phy_spdiftx_init(const struct device *dev)
{
    const struct phy_spdiftx_config_data *cfg = dev->config;
    struct phy_spdiftx_drv_data *data = dev->data;

    memset(data, 0, sizeof(struct phy_spdiftx_drv_data));

    __spdiftx_dt_dump_info(cfg);

    /* reset SPDIFTX controller */
    acts_reset_peripheral(cfg->rst_id);

    data->fifo_sel = AUDIO_FIFO_INVALID_TYPE;

    printk("SPDIFTX init successfully\n");

    return 0;
}

/* physical spdiftx driver data */
static struct phy_spdiftx_drv_data phy_spdiftx_drv_data0;


/* physical spdiftx config data */
static const struct phy_spdiftx_config_data phy_spdiftx_config_data0 = {
    .reg_base = AUDIO_SPDIFTX_REG_BASE,
    .clk_id = CLOCK_ID_SPDIFTX,
    .rst_id = RESET_ID_SPDIFTX,

    PHY_DEV_FEATURE_DEF(clk_i2stx_div2) = CONFIG_AUDIO_SPDIFTX_0_CLK_I2STX_DIV2,
};

#if IS_ENABLED(CONFIG_AUDIO_SPDIFTX_0)
DEVICE_DEFINE(spdiftx0, CONFIG_AUDIO_SPDIFTX_0_NAME, phy_spdiftx_init, NULL,
        &phy_spdiftx_drv_data0, &phy_spdiftx_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_spdiftx_drv_api);
#endif
