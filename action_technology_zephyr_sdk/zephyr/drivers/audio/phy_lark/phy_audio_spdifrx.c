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
 *	- Support HDMI ARC
 *    - Support Sample Rate Change & Timeout Detect
 *    - Sample rate support up to 96KHz
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
#include <drivers/audio/audio_in.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(spdifrx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * SPDIFRX_CTL0
 */
#define SPDIFR0_CTL0_ANA_RES                                  BIT(29) /* Enable or disable internal 2k ohm registor */
#define SPDIFR0_CTL0_ANA_HYSVOL                               BIT(28) /* Analog pin Hysteresis Voltage select 0: 110mv 1: 220mv */
#define SPDIFR0_CTL0_FCS                                      BIT(16) /* Filter Pulse Cycle Select */
#define SPDIFR0_CTL0_LOOPBACK_EN                              BIT(15) /* Enable internal loopback */
#define SPDIFR0_CTL0_VBM                                      BIT(14) /* Validity bit */
#define SPDIFR0_CTL0_DAMS                                     BIT(13) /* Data mask state. If sample rate changed, data will be keep as 0 and set this bit*/
#define SPDIFR0_CTL0_DAMEN                                    BIT(12) /* If sample rate changed, the data mask enable flag*/
#define SPDIFR0_CTL0_DELTAADD_SHIFT                           (8) /* Delta to add on configured or detected T width */
#define SPDIFR0_CTL0_DELTAADD_MASK                            (0xF << SPDIFR0_CTL0_DELTAADD_SHIFT)
#define SPDIFR0_CTL0_DELTAADD(x)                              ((x) << SPDIFR0_CTL0_DELTAADD_SHIFT)

#define SPDIFR0_CTL0_DELTAMIN_SHIFT                           (4) /* Delta to minus from configured or detected T width */
#define SPDIFR0_CTL0_DELTAMIN_MASK                            (0xF << SPDIFR0_CTL0_DELTAMIN_SHIFT)
#define SPDIFR0_CTL0_DELTAMIN(x)                              ((x) << SPDIFR0_CTL0_DELTAMIN_SHIFT)

#define SPDIFR0_CTL0_DELTA_MODE                               BIT(3) /* T Width delta mode select */
#define SPDIFR0_CTL0_CAL_MODE                                 BIT(2) /* T Width cal mode select */
#define SPDIFR0_CTL0_SPDIF_CKEDG                              BIT(1) /* Select of SPDIF input signal latch clock edge */
#define SPDIFR0_CTL0_SPDIF_RXEN                               BIT(0) /* SPDIF RX Enable */

/***************************************************************************************************
 * SPDIFRX_CTL1
 */
#define SPDIFR0_CTL1_WID2TCFG_SHIFT                           (16) /* 2T Width Config MAX:512 */
#define SPDIFR0_CTL1_WID2TCFG_MASK                            (0x1FF << SPDIFR0_CTL1_WID2TCFG_SHIFT)
#define SPDIFR0_CTL1_WID1P5TCFG_SHIFT                         (8) /* 1.5T Width Config MAX:156*/
#define SPDIFR0_CTL1_WID1P5TCFG_MASK                          (0xFF << SPDIFR0_CTL1_WID1P5TCFG_SHIFT)
#define SPDIFR0_CTL1_WID1TCFG_SHIFT                           (0) /* 1T Width Config MAX:156*/
#define SPDIFR0_CTL1_WID1TCFG_MASK                            (0xFF << SPDIFR0_CTL1_WID1TCFG_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CTL2
 */
#define SPDIFR0_CTL2_WID4TCFG_SHIFT                           (18) /* 4T Width Config MAX:1024 */
#define SPDIFR0_CTL2_WID4TCFG_MASK                            (0x3FF << SPDIFR0_CTL2_WID4TCFG_SHIFT)
#define SPDIFR0_CTL2_WID3TCFG_SHIFT                           (9) /* 3T Width Config MAX:512 */
#define SPDIFR0_CTL2_WID3TCFG_MASK                            (0x1FF << SPDIFR0_CTL2_WID3TCFG_SHIFT)
#define SPDIFR0_CTL2_WID2P5TCFG_SHIFT                         (0) /* 2.5T Width Config MAX:512 */
#define SPDIFR0_CTL2_WID2P5TCFG_MASK                          (0x1FF << SPDIFR0_CTL2_WID2P5TCFG_SHIFT)

/***************************************************************************************************
 * SPDIFRX_PD
 */
#define SPDIFR0_PD_SRCIRQDELAY                                BIT(17) /* SPDIF RX sample rate change IRQ occur timing. 0: right now; 1: after new block head has been detected */
#define SPDIFR0_PD_BL_HEADPD                                  BIT(16) /* Block head detect pending */
#define SPDIFR0_PD_SRTOPD                                     BIT(14) /* Sample rate detect timeout interrupt pending */
#define SPDIFR0_PD_CSSRUPPD                                   BIT(13) /* Channel state sample rate change IRQ pending */
#define SPDIFR0_PD_CSUPPD                                     BIT(12) /* Channel state update irq pending */
#define SPDIFR0_PD_SRCPD                                      BIT(11) /* Sample rate change pending */
#define SPDIFR0_PD_BMCERPD                                    BIT(10) /* BMC Decoder Err pending */
#define SPDIFR0_PD_SUBRCVPD                                   BIT(9) /* Sub Frame Receive Err pending */
#define SPDIFR0_PD_BLKRCVPD                                   BIT(8) /* Block received Err pending */
#define SPDIFR0_PD_SRTOEN                                     BIT(6) /* sample rate detect timeout IRQ enable */
#define SPDIFR0_PD_CSSRCIRQEN                                 BIT(5) /* channel state sample rate change irq enable */
#define SPDIFR0_PD_CSUPIRQEN                                  BIT(4) /* channel state update irq enable */
#define SPDIFR0_PD_SRCIRQEN                                   BIT(3) /* SPDIF RX sample rate change IRQ enable */
#define SPDIFR0_PD_BMCIRQEN                                   BIT(2) /* BMC Decoder Err IRQ enable */
#define SPDIFR0_PD_SUBIRQEN                                   BIT(1) /* Sub Frame receive Err IRQ enable */
#define SPDIFR0_PD_BLKIRQEN                                   BIT(0) /* Block Receive Err IRQ enable */

/***************************************************************************************************
 * SPDIFRX_DBG
 */
#define SPDIFR0_DBG_DBGSEL_SHIFT                              (17)
#define SPDIFR0_DBG_DBGSEL_MASK                               (0xF << SPDIFR0_DBG_DBGSEL_SHIFT)
#define SPDIFR0_DBG_SUBRCVFSM_SHIFT                           (14)
#define SPDIFR0_DBG_SUBRCVFSM_MASK                            (0x7 << SPDIFR0_DBG_SUBRCVFSM_SHIFT)
#define SPDIFR0_DBG_BMCRCVFSM_SHIFT                           (6)
#define SPDIFR0_DBG_BMCRCVFSM_MASK                            (0xFF << SPDIFR0_DBG_BMCRCVFSM_SHIFT)
#define SPDIFR0_DBG_BMCDECFSM_SHIFT                           (3)
#define SPDIFR0_DBG_BMCDECFSM_MASK                            (0x7 << SPDIFR0_DBG_BMCDECFSM_SHIFT)
#define SPDIFR0_DBG_HW_FSM_SHIFT                              (0)
#define SPDIFR0_DBG_HW_FSM_MASK                               (0x7 << SPDIFR0_DBG_HW_FSM_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CNT
 */
#define SPDIFR0_CNT_DIN2_WIDTH_SHIFT                          (24) /* Din2 Width */
#define SPDIFR0_CNT_DIN2_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN1_WIDTH_SHIFT)
#define SPDIFR0_CNT_DIN1_WIDTH_SHIFT                          (16) /* Din1 Width */
#define SPDIFR0_CNT_DIN1_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN1_WIDTH_SHIFT)
#define SPDIFR0_CNT_DIN0_WIDTH_SHIFT                          (8) /* Din0 Width */
#define SPDIFR0_CNT_DIN0_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN0_WIDTH_SHIFT)
#define SPDIFR0_CNT_FRAMECNT_SHIFT                            (0) /* Audio Frame Counter */
#define SPDIFR0_CNT_FRAMECNT_MASK                             (0xFF << SPDIFR0_CNT_FRAMECNT_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CSL
 */
#define SPDIFR0_CSL_SPDCSL_E                                  (31) /* SPDIFRX Channel State Low */
#define SPDIFR0_CSL_SPDCSL_SHIFT                              (0)
#define SPDIFR0_CSL_SPDCSL_MASK                               (0xFFFFFFFF << SPDIFR0_CSL_SPDCSL_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CSH
 */
#define SPDIFR0_CSH_SPDCSH_SHIFT                              (0) /* SPDIFRX Channel State High */
#define SPDIFR0_CSH_SPDCSH_MASK                               (0xFFFF << SPDIFR0_CSH_SPDCSH_SHIFT)

/***************************************************************************************************
 * SPDIFRX_SAMP - Sample Rate Detect Register
 */
#define SPDIFR0_SAMP_SAMP_VALID                               BIT(28) /* sample rate valid flag */
#define SPDIFR0_SAMP_SAMP_CNT_SHIFT                           (16) /* SPDIFRX sample rate counter detect by 24M clock */
#define SPDIFR0_SAMP_SAMP_CNT_MASK                            (0xFFF << SPDIFR0_SAMP_SAMP_CNT_SHIFT)
#define SPDIFR0_SAMP_SAMP_CNT(x)                              (((x) & SPDIFR0_SAMP_SAMP_CNT_MASK) >> SPDIFR0_SAMP_SAMP_CNT_SHIFT)
#define SPDIFR0_SAMP_SAMP_DELTA_SHIFT                         (1) /* Delta is used by SAMP_CNT to detect sample rate change or not */
#define SPDIFR0_SAMP_SAMP_DELTA_MASK                          (0xF << SPDIFR0_SAMP_SAMP_DELTA_SHIFT)
#define SPDIFR0_SAMP_SAMP_DELTA(x)                            ((x) << SPDIFR0_SAMP_SAMP_DELTA_SHIFT)
#define SPDIFR0_SAMP_SAMP_EN                                  BIT(0) /* Sample rate detect enable */

/***************************************************************************************************
 * SPDIFRX_SRTO_THRES
 */
#define SPDIFR0_SRTO_THRES_SRTO_THRES_SHIFT                   (0) /* The threshold to generate sample rate detect timeout signal */
#define SPDIFR0_SRTO_THRES_SRTO_THRES_MASK                    (0xFFFFFF << 0)

/***************************************************************************************************
 * SPDIFRX_FIFOCTL
 */
#define SPDIFR0_FIFOCTL_FIFO_DMAWIDTH                         BIT(7) /* FIFO DMA transfer width configured */
#define SPDIFR0_FIFOCTL_FIFO_OS_SHIFT                         (4) /* FIFO output select */
#define SPDIFR0_FIFOCTL_FIFO_OS_MASK                          (0x3 << SPDIFR0_FIFOCTL_FIFO_OS_SHIFT)
#define SPDIFR0_FIFOCTL_FIFO_OS(x)                            ((x) << SPDIFR0_FIFOCTL_FIFO_OS_SHIFT)
#define SPDIFR0_FIFOCTL_FIFO_OS_CPU                           SPDIFR0_FIFOCTL_FIFO_OS(0)
#define SPDIFR0_FIFOCTL_FIFO_OS_DMA                           SPDIFR0_FIFOCTL_FIFO_OS(1)

#define SPDIFR0_FIFOCTL_FIFO_IEN                              BIT(2) /* FIFO Half filled IRQ enable */
#define SPDIFR0_FIFOCTL_FIFO_DEN                              BIT(1) /* FIFO Half filled DRQ enable */
#define SPDIFR0_FIFOCTL_FIFO_RST                              BIT(0) /* FIFO reset */

/***************************************************************************************************
 * SPDIFRX_FIFOSTA
 */
#define SPDIFR0_FIFOSTA_FIFO_ER                               BIT(8) /* FIFO error */
#define SPDIFR0_FIFOSTA_FIFO_EMPTY                            BIT(7) /* FIFO empty flag */
#define SPDIFR0_FIFOSTA_FIFO_IP                               BIT(6) /* FIFO Half Filled IRQ pending bit */
#define SPDIFR0_FIFOSTA_FIFO_STATUS_SHIFT                     (0) /* FIFO status */
#define SPDIFR0_FIFOSTA_FIFO_STATUS_MASK                      (0x3F << SPDIFR0_FIFOSTA_FIFO_STATUS_SHIFT)

/***************************************************************************************************
 * SPDIFRX_DAT_FIFO
 */
#define SPDIFR0_DAT_FIFO_RXDAT_SHIFT                          (8) /* SPDIFRX Data */
#define SPDIFR0_DAT_FIFO_RXDAT_MASK                           (0xFFFFFF << SPDIFR0_DAT_FIFO_RXDAT_SHIFT)

#define SPDIF_RX_INIT_DELTA                                   (4)
#define SPDIF_RX_DEFAULT_SAMPLE_DELTA                         (8)
#define SPDIF_RX_HIGH_SR_SAMPLE_DELTA                         (3)
#define SPDIF_RX_SR_DETECT_MS                                 (1)
#define SPDIF_RX_SRD_TIMEOUT_THRES                            (0xFFFFFF)

/*
 * enum a_spdifrx_srd_sts_e
 * @brief The SPDIFRX sample rate detect status
 */
typedef enum {
    RX_SR_STATUS_NOTGET = 0, /* Still not get the sample rate */
    RX_SR_STATUS_CHANGE, /* sample rate detect change happened */
    RX_SR_STATUS_GET, /* has gotten the sample rate */
} a_spdifrx_srd_sts_e;

/**
 * struct phy_spdifrx_drv_data
 * @brief The software related data that used by physical spdifrx driver.
 */
struct phy_spdifrx_drv_data {
    int (*srd_callback)(void *cb_data, uint32_t cmd, void *param); /* sample rate detect callback */
    void *cb_data; /* callback user data */
    a_spdifrx_srd_sts_e srd_status; /* sample rate detect status */
    uint8_t sample_rate; /* channel sample rate */
};

/**
 * struct phy_spdifrx_config_data
 * @brief The hardware related data that used by physical spdifrx driver.
 */
struct phy_spdifrx_config_data {
    uint32_t reg_base; /* SPDIFRX controller register base address */
    struct audio_dma_dt dma_fifo0; /* DMA resource for SPDIFRX */
    uint32_t min_corepll_clock; /* min_corepll_clock >= 8 x 128 x sr */
    void (*irq_config)(void); /* IRQ configuration function */
    uint8_t clk_id; /* SPDIFRX devclk id */
    uint8_t rst_id; /* SPDIFRX reset id */
};

/*
 * @struct acts_audio_spdifrx
 * @brief SPDIFRX controller hardware register
 */
struct acts_audio_spdifrx {
    volatile uint32_t ctl0; /* SPDIFRX Control0 */
    volatile uint32_t ctl1; /* SPDIFRX Control1 */
    volatile uint32_t ctl2; /* SPDIFRX Control2 */
    volatile uint32_t pending; /* SPDIFRX IRQ pending */
    volatile uint32_t dbg; /* SPDIFRX debug */
    volatile uint32_t cnt; /* SPDIFRX  counter */
    volatile uint32_t csl; /* SPDIFRX Channel State Low */
    volatile uint32_t csh; /* SPDIFRX Channel State High */
    volatile uint32_t samp; /* SPDIFRX sample rate detect */
    volatile uint32_t thres; /* SPDIFRX  sample rate detect timeout threshold */
    volatile uint32_t fifoctl; /* SPDIFRX FIFO control */
    volatile uint32_t fifostat; /* SPDIFRX FIFO state */
    volatile uint32_t fifodat; /* SPDIFRX FIFO data */
};

/* @brief get the base address of SPDIFRX register */
static inline struct acts_audio_spdifrx *get_spdifrx_reg_base(struct device *dev)
{
    const struct phy_spdifrx_config_data *cfg = dev->config;
    return (struct acts_audio_spdifrx *)cfg->reg_base;
}

/* @brief dump spdifrx controller register */
static void spdifrx_dump_register(struct device *dev)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    LOG_INF("** spdifrx contoller regster **");
    LOG_INF("               BASE: %08x", (uint32_t)spdifrx_base);
    LOG_INF("       SPDIFRX_CTL0: %08x", spdifrx_base->ctl0);
    LOG_INF("       SPDIFRX_CTL1: %08x", spdifrx_base->ctl1);
    LOG_INF("       SPDIFRX_CTL2: %08x", spdifrx_base->ctl2);
    LOG_INF("         SPDIFRX_PD: %08x", spdifrx_base->pending);
    LOG_INF("        SPDIFRX_DBG: %08x", spdifrx_base->dbg);
    LOG_INF("        SPDIFRX_CNT: %08x", spdifrx_base->cnt);
    LOG_INF("        SPDIFRX_CSL: %08x", spdifrx_base->csl);
    LOG_INF("        SPDIFRX_CSH: %08x", spdifrx_base->csh);
    LOG_INF("       SPDIFRX_SAMP: %08x", spdifrx_base->samp);
    LOG_INF(" SPDIFRX_STRO_THRES: %08x", spdifrx_base->thres);
    LOG_INF("    SPDIFRX_FIFOCTL: %08x", spdifrx_base->fifoctl);
    LOG_INF("    SPDIFRX_FIFOSTA: %08x", spdifrx_base->fifostat);
    LOG_INF("   SPDIFRX_DAT_FIFO: %08x", spdifrx_base->fifodat);
    LOG_INF("     CMU_SPDIFRXCLK: %08x", sys_read32(CMU_SPDIFRXCLK));
    LOG_INF("       CORE_PLL_CTL: %08x", sys_read32(COREPLL_CTL));
}

/* @brief Prepare the clock and pinmux resources for the SPDIFRX enable */
static int phy_spdifrx_prepare_enable(struct device *dev, uint8_t sr)
{
    const struct phy_spdifrx_config_data *cfg = dev->config;
    uint32_t core_pll;
    struct board_pinmux_info pinmux_info;

    board_get_spdiftx0_pinmux_info(&pinmux_info);

    /* Config the spdifrx pin state */
    acts_pinmux_setup_pins(pinmux_info.pins_config, pinmux_info.pins_num);

    /* SPDIFRX RMU normal */
    acts_reset_peripheral(cfg->rst_id);

    /* clear SPDIFRX clock */
    sys_write32((sys_read32(CMU_SPDIFRXCLK) & ~(CMU_SPDIFRXCLK_SPDIFRXCLKSRC_MASK
                | CMU_SPDIFRXCLK_SPDIFRXCLKDIV_MASK)), CMU_SPDIFRXCLK);

    /* Select SPDIFRX clock source to be corepll */
    sys_write32((sys_read32(CMU_SPDIFRXCLK) | CMU_SPDIFRXCLK_SPDIFRXCLKSRC(2)), CMU_SPDIFRXCLK);

    if (sr <= SAMPLE_RATE_16KHZ) {
        uint8_t pll_index, seires;
        if (SAMPLE_RATE_11KHZ == sr) {
            seires = AUDIOPLL_44KSR;
        } else {
            seires = AUDIOPLL_48KSR;
        }
        if (audio_pll_check_config(seires, &pll_index)) {
            LOG_ERR("pll check and config error");
            return -EFAULT;
        }
        sys_write32((sys_read32(CMU_SPDIFRXCLK) & (~0x3FF))
            | ((pll_index << 8) | (3 << 0)), CMU_SPDIFRXCLK);
    } else {
        core_pll = clk_rate_get_corepll();
        if (core_pll < cfg->min_corepll_clock) {
            LOG_ERR("too low frequency corepll:%d mininum:%d", core_pll, cfg->min_corepll_clock);
            return -EACCES;
        }
    }

    /* this will also enable the HOSC clock to detect Audio Sample Rate */
    acts_clock_peripheral_enable(cfg->clk_id);

    return 0;
}

/* @brief Config the SPDIFRX FIFO source */
static int phy_spdifrx_fifo_config(struct device *dev, audio_fifouse_sel_e sel, audio_dma_width_e width)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    if (FIFO_SEL_CPU == sel) {
        spdifrx_base->fifoctl = (SPDIFR0_FIFOCTL_FIFO_IEN | SPDIFR0_FIFOCTL_FIFO_RST);
    } else if (FIFO_SEL_DMA == sel) {
        spdifrx_base->fifoctl = (SPDIFR0_FIFOCTL_FIFO_OS_DMA | SPDIFR0_FIFOCTL_FIFO_DEN
                                    | SPDIFR0_FIFOCTL_FIFO_RST);
        if (DMA_WIDTH_16BITS == width)
            spdifrx_base->fifoctl |= SPDIFR0_FIFOCTL_FIFO_DMAWIDTH;
    } else {
        LOG_ERR("invalid fifo sel %d", sel);
        return -EINVAL;
    }

    return 0;
}

/* @brief SPDIF control and config */
static void phy_spdifrx_ctl_config(struct device *dev)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    spdifrx_base->ctl0 = 0;

    /* Enable hardware detect T Width */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_CAL_MODE;

    /* Enable hardware automatically fill DELTAADD and DELTAMIN T Width */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTA_MODE;

    /* Delta to minus from configured or detected T width */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAMIN(SPDIF_RX_INIT_DELTA);

    /* Delta to add on configured or detected T width */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAADD(SPDIF_RX_INIT_DELTA);

    /* DAMEN */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_DAMEN;

    /* spdif rx enable */
    spdifrx_base->ctl0 |= SPDIFR0_CTL0_SPDIF_RXEN;

    /* sample rate detect enable */
    spdifrx_base->samp &= (~SPDIFR0_SAMP_SAMP_DELTA_MASK);
    spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_DELTA(SPDIF_RX_DEFAULT_SAMPLE_DELTA);
    spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_EN;
}

/* @brief Get the SPDIFRX channel status */
static void phy_spdifrx_channel_status_get(struct device *dev, audio_spdif_ch_status_t *sts)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    if (sts) {
        sts->csl = spdifrx_base->csl;
        sts->csh = (uint16_t)spdifrx_base->csh;
    }
}

/* @brief Check if the stream is the PCM format stream */
static bool phy_spdifrx_check_is_pcm(struct device *dev)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    /* The channel status bit1 indicats the stream is a linear PCM samples */
    if (spdifrx_base->csl & (1 << 1))
        return false;

    return true;
}

/* @brief Check if decode error happened */
static bool phy_spdifrx_check_decode_err(struct device *dev)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);
    bool ret = false;

    if (spdifrx_base->pending & SPDIFR0_PD_BLKRCVPD) {
        spdifrx_base->pending |= SPDIFR0_PD_BLKRCVPD;
        ret = true;
    }

    if (spdifrx_base->pending & SPDIFR0_PD_SUBRCVPD) {
        spdifrx_base->pending |= SPDIFR0_PD_SUBRCVPD;
        ret = true;
    }

    if (spdifrx_base->pending & SPDIFR0_PD_BMCERPD) {
        spdifrx_base->pending |= SPDIFR0_PD_BMCERPD;
        ret = true;
    }

    return ret;
}

/* @brief Enable spdifrx irq events */
static void phy_spdifrx_irq_en(struct device *dev)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);

    spdifrx_base->thres = SPDIF_RX_SRD_TIMEOUT_THRES;

    /* SPDIF RX sample rate change IRQ happened when new block head has been detected */
    spdifrx_base->pending |= SPDIFR0_PD_SRCIRQDELAY;

    /* SPDIFRX sample rate detect IRQ enable */
    spdifrx_base->pending |= (SPDIFR0_PD_SRCIRQEN | SPDIFR0_PD_SRTOEN);

    /* clear all IRQ pendings */
    spdifrx_base->pending |= (SPDIFR0_PD_BLKRCVPD | SPDIFR0_PD_SUBRCVPD
                            | SPDIFR0_PD_BMCERPD | SPDIFR0_PD_SRCPD
                            | SPDIFR0_PD_CSUPPD | SPDIFR0_PD_CSSRUPPD
                            | SPDIFR0_PD_SRTOPD | SPDIFR0_PD_BL_HEADPD);
}

static int phy_spdifrx_enable(struct device *dev, void *param)
{
    struct phy_spdifrx_drv_data *data = dev->data;
    ain_param_t *in_param = (ain_param_t *)param;
    spdifrx_setting_t *spdifrx_setting = in_param->spdifrx_setting;
    int ret;
    uint8_t width;

    if (!in_param) {
        LOG_ERR("Invalid parameters");
        return -EINVAL;
    }

    if (in_param->sample_rate)
        data->sample_rate = in_param->sample_rate;

    if (spdifrx_setting && spdifrx_setting->srd_callback) {
        data->srd_callback = spdifrx_setting->srd_callback;
        data->cb_data = spdifrx_setting->cb_data;
    } else {
        data->srd_callback = NULL;
        data->cb_data = NULL;
    }

    /* Prepare the spdifrx clock and pinmux etc. */
    ret = phy_spdifrx_prepare_enable(dev, data->sample_rate);
    if (ret)
        return ret;

    /* Config the SPDIFRX FIFO */
    width = (in_param->channel_width == CHANNEL_WIDTH_16BITS)
                ?  DMA_WIDTH_16BITS : DMA_WIDTH_32BITS;
    ret = phy_spdifrx_fifo_config(dev, FIFO_SEL_DMA, width);
    if (ret) {
        LOG_ERR("Config SPDIFRX FIFO error %d", ret);
        return ret;
    }

    /* Control spdifrx and enable sample rate detect function */
    phy_spdifrx_ctl_config(dev);

    /* Enable SPDIFRX IRQs */
    phy_spdifrx_irq_en(dev);

    return 0;
}

static int phy_spdifrx_disable(struct device *dev, void *param)
{
    const struct phy_spdifrx_config_data *cfg = dev->config;
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);
    struct phy_spdifrx_drv_data *data = dev->data;
    uint8_t i;
    struct board_pinmux_info pinmux_info;

    board_get_spdiftx0_pinmux_info(&pinmux_info);

    /* Set the spdiftx pins to be GPIO state for the power consume */
    for (i = 0; i < pinmux_info.pins_num; i++)
        acts_pinmux_set(pinmux_info.pins_config[i].pin_num, 0);

    /* SPDIFRX FIFO reset */
    spdifrx_base->fifoctl &= ~SPDIFR0_FIFOCTL_FIFO_RST;

    /* SPDIF RX disable */
    spdifrx_base->ctl0 &= ~SPDIFR0_CTL0_SPDIF_RXEN;

    /* Disable sample rate change IRQ */
    spdifrx_base->pending &= ~(SPDIFR0_PD_SRCIRQEN | SPDIFR0_PD_SRTOEN);

    /* SPDIF RX clock gating disable */
    acts_clock_peripheral_disable(cfg->clk_id);

    data->srd_callback = NULL;
    data->cb_data = NULL;
    data->sample_rate = 0;

    return 0;
}

int phy_spdifrx_samplerate_detect(struct device *dev, int wait_time_ms)
{
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);
    struct phy_spdifrx_drv_data *data = dev->data;
    uint32_t time_stamp_begin, time_stamp_delta;
    int result = -1;
    uint8_t index;
    uint32_t sample_cnt;

    static const uint16_t sample_cnt_tbl[14][2] = {
        {120, 130},			// 192k,  24M/192K =  125
        {131, 141},			// 176.4k,  24M/176K = 136
        {245, 255},			// 96k,  24M/96K = 250
        {267, 277},			// 88.2k, 24M/88K = 272
        {369, 381},			// 64k, 24M/64K = 375
        {489, 510},			// 48k,  24M/48K = 500
        {532, 726},			// 44.1k, 24M/44K = 544
        {727, 774},			// 32k, 24M/32K = 750
        {960, 1043},		// 24k, 24M/24K = 1000
        {1041,1140},		// 22.05k, 24M/22.05K = 1088
        {1411, 1600},		// 16k, 24M/16K = 1500
        {1846, 2170},		// 12k, 24M/12K = 2000
        {2171, 2394},		// 11.025k, 24M/11.025K = 2176
        {2666, 3428},		// 8k, 24M/8K = 3000
    };

    const uint8_t sample_rate_tbl[] = {
        SAMPLE_RATE_192KHZ, SAMPLE_RATE_176KHZ, SAMPLE_RATE_96KHZ, SAMPLE_RATE_88KHZ,
        SAMPLE_RATE_64KHZ, SAMPLE_RATE_48KHZ, SAMPLE_RATE_44KHZ, SAMPLE_RATE_32KHZ,
        SAMPLE_RATE_24KHZ, SAMPLE_RATE_22KHZ, SAMPLE_RATE_16KHZ, SAMPLE_RATE_12KHZ,
        SAMPLE_RATE_11KHZ, SAMPLE_RATE_8KHZ
    };

    if(data->srd_status >= RX_SR_STATUS_GET) {
        LOG_INF("Already got sample rate %d", data->sample_rate);
        return 0;
    }

    if (wait_time_ms != -1)
        time_stamp_begin = k_cycle_get_32();

    do {
        /* get the sample rate detect counter */
        sample_cnt = SPDIFR0_SAMP_SAMP_CNT(spdifrx_base->samp);

        if (wait_time_ms != -1) {
            time_stamp_delta = k_cyc_to_ns_floor64(k_cycle_get_32() - time_stamp_begin);
            time_stamp_delta /= 1000000;
            if(time_stamp_delta > wait_time_ms) {
                LOG_INF("timeout samp:%d\n", sample_cnt);
                result = -ETIMEDOUT;
                break;
            }
        }

        /* check sample valid */
        if ((spdifrx_base->samp & SPDIFR0_SAMP_SAMP_VALID) == 0) {
            /* Occur when BMC_ERR or SAMP_CNT overflow */
            LOG_DBG("sample rate invalid");
        } else {
            /* sample rate valid */
            for(index = 0; index < sizeof(sample_rate_tbl); index++) {
                if((sample_cnt >= sample_cnt_tbl[index][0]) && (sample_cnt <= sample_cnt_tbl[index][1])) {
                    data->sample_rate = sample_rate_tbl[index];
                    result = 0;
                    break;
                }
            }

            if (result == 0) {
                LOG_INF("NEW SAMPLE RATE => %d!\n", data->sample_rate);
                break;
            }
        }
    }while(1);

    return result;
}

static int phy_spdifrx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
    const struct phy_spdifrx_config_data *cfg = dev->config;

    switch (cmd) {
    case PHY_CMD_DUMP_REGS:
    {
        spdifrx_dump_register(dev);
        break;
    }
    case AIN_CMD_SPDIF_GET_CHANNEL_STATUS:
    {
        audio_spdif_ch_status_t *sts = (audio_spdif_ch_status_t *)param;
        if (!sts) {
            LOG_ERR("Invalid parameters");
            return -EINVAL;
        }
        phy_spdifrx_channel_status_get(dev, sts);
        break;
    }
    case AIN_CMD_SPDIF_IS_PCM_STREAM:
    {
        if (phy_spdifrx_check_is_pcm(dev))
            *(bool *)param = true;
        else
            *(bool *)param = false;
        break;
    }
    case AIN_CMD_SPDIF_CHECK_DECODE_ERR:
    {
        if (phy_spdifrx_check_decode_err(dev))
            *(bool *)param = true;
        else
            *(bool *)param = false;
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

    return 0;
}

const struct phy_audio_driver_api phy_spdifrx_drv_api = {
    .audio_enable = phy_spdifrx_enable,
    .audio_disable = phy_spdifrx_disable,
    .audio_ioctl = phy_spdifrx_ioctl,
};

/* dump spdifrx device tree infomation */
static void __spdifrx_dt_dump_info(const struct phy_spdifrx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
    LOG_INF("**     SPDIFRX BASIC INFO     **");
    LOG_INF("             BASE: %08x", cfg->reg_base);
    LOG_INF("           CLK-ID: %08x", cfg->clk_id);
    LOG_INF("           RST-ID: %08x", cfg->rst_id);
    LOG_INF("        DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
    LOG_INF("          DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
    LOG_INF("          DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);
    LOG_INF("MIN-COREPLL-CLOCK: %08x", cfg->min_corepll_clock);
#endif
}

static void phy_spdifrx_isr(const void *arg)
{
    struct device *dev = (struct device *)arg;
    struct acts_audio_spdifrx *spdifrx_base = get_spdifrx_reg_base(dev);
    struct phy_spdifrx_drv_data *data = dev->data;
    int result;
    uint32_t pending;

    LOG_DBG("irq pending 0x%x", spdifrx_base->pending);

    pending = spdifrx_base->pending & (~0x7F);

    /* deal for spdifrx sample rate change irq */
    if (spdifrx_base->pending & SPDIFR0_PD_SRCPD) {
        /* sample rate change happened */
        data->srd_status = RX_SR_STATUS_CHANGE;

        result = phy_spdifrx_samplerate_detect(dev, SPDIF_RX_SR_DETECT_MS);
        if(result != 0) {
            LOG_ERR("sample rate detect error:%d", result);
            if (data->srd_callback != NULL)
                data->srd_callback(data->cb_data, SPDIFRX_SRD_TIMEOUT, NULL);
        } else {
            /* got new sample rate */
            data->srd_status = RX_SR_STATUS_GET;
        }

        spdifrx_base->ctl0 |= SPDIFR0_CTL0_DAMS;

        if (result == 0) {
            /* notify the sample rate event to user */
            if (data->srd_callback)
                data->srd_callback(data->cb_data, SPDIFRX_SRD_FS_CHANGE, (void *)&data->sample_rate);
        }
    }

    /* sample rate detect timeout pending */
    if ((spdifrx_base->pending & SPDIFR0_PD_SRTOPD)
        && (data->srd_status == RX_SR_STATUS_GET)) {
        if (data->srd_callback)
            data->srd_callback(data->cb_data, SPDIFRX_SRD_TIMEOUT, NULL);

        data->srd_status = RX_SR_STATUS_NOTGET;
    }

    /* clear pending */
    spdifrx_base->pending |= pending;
}


static int phy_spdifrx_init(const struct device *dev)
{
    const struct phy_spdifrx_config_data *cfg = dev->config;
    struct phy_spdiftx_drv_data *data = dev->data;

    /* clear driver data */
    memset(data, 0, sizeof(struct phy_spdifrx_drv_data));

    __spdifrx_dt_dump_info(cfg);

    /* reset SPDIFRX controller */
    acts_reset_peripheral(cfg->rst_id);

    if (cfg->irq_config)
        cfg->irq_config();

    printk("SPDIFRX init successfully\n");

    return 0;
}

static void phy_spdifrx_irq_config(void);

/* physical spdifrx driver data */
static struct phy_spdifrx_drv_data phy_spdifrx_drv_data0;


/* physical spdifrx config data */
static const struct phy_spdifrx_config_data phy_spdifrx_config_data0 = {
    .reg_base = AUDIO_SPDIFRX_REG_BASE,
    AUDIO_DMA_FIFO_DEF(SPDIFRX, 0),
    .clk_id = CLOCK_ID_SPDIFRX,
    .rst_id = RESET_ID_SPDIFRX,
    .min_corepll_clock = CONFIG_AUDIO_SPDIFRX_0_MIN_COREPLL_CLOCK,
    .irq_config = phy_spdifrx_irq_config,
};

#if IS_ENABLED(CONFIG_AUDIO_SPDIFRX_0)
DEVICE_DEFINE(spdifrx0, CONFIG_AUDIO_SPDIFRX_0_NAME, phy_spdifrx_init, NULL,
        &phy_spdifrx_drv_data0, &phy_spdifrx_config_data0,
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_spdifrx_drv_api);
#endif

static void phy_spdifrx_irq_config(void)
{
    IRQ_CONNECT(IRQ_ID_SPIDFRX, CONFIG_AUDIO_SPDIFRX_0_IRQ_PRI,
            phy_spdifrx_isr,
            DEVICE_GET(spdifrx0), 0);
    irq_enable(IRQ_ID_SPIDFRX);
}

