/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common utils for in/out audio drivers
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include "audio_acts_utils.h"
#include "../phy_audio_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(autils, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * @brief AUDIO PLL clock selection
 */
#define CMU_AUDIOPLL0_LOCK         (8)
#define CMU_AUDIOPLL0_CTL_EN          (7)
#define CMU_AUDIOPLL0_CTL_APS0_SHIFT  (0)
#define CMU_AUDIOPLL0_CTL_APS0(x)    ((x) << CMU_AUDIOPLL0_CTL_APS0_SHIFT)
#define CMU_AUDIOPLL0_CTL_APS0_MASK	  CMU_AUDIOPLL0_CTL_APS0(0x3F)

/* 48ksr serials */
#define PLL_65536               (65536000)
#define PLL_49152               (49152000)
#define PLL_24576               (24576000)
#define PLL_16384               (16384000)
#define PLL_12288               (12288000)
#define PLL_8192                 (8192000)
#define PLL_6144                 (6144000)
#define PLL_4096                 (4096000)
#define PLL_3072                 (3072000)
#define PLL_2048                 (2048000)
#define PLL_1024                 (1024000)

/* 44.1ksr serials */
#define PLL_602112             (60211200)
#define PLL_451584             (45158400)
#define PLL_225792             (22579200)
#define PLL_112896             (11289600)
#define PLL_56448               (5644800)
#define PLL_28224               (2822400)

/*
 * struct audio_dac_pll_t
 * @brief The structure includes the dac pll clock and its setting parameters
 */
typedef struct {
    audio_sr_sel_e sr_khz; /* audio sr_khz clock */
	uint8_t div;
	uint8_t fir_div;
	uint8_t fir2x_div;
	uint8_t cic_div;
} audio_dac_pll_t;

static const audio_dac_pll_t audio_dac_pll_selections[] = {
    {SAMPLE_RATE_96KHZ, 0, 0, 0, 1}, 	/* 48ksr series */
	{SAMPLE_RATE_48KHZ, 0, 0, 1, 1}, 	/* 48ksr series */
	{SAMPLE_RATE_32KHZ, 0, 0, 2, 1}, 	/* 48ksr series */
	{SAMPLE_RATE_24KHZ, 1, 0, 1, 0}, 	/* 48ksr series */
	{SAMPLE_RATE_16KHZ, 1, 0, 2, 0}, 	/* 48ksr series */
	{SAMPLE_RATE_12KHZ, 1, 1, 1, 0}, 	/* 48ksr series */
	{SAMPLE_RATE_8KHZ,  1, 1, 2, 0}, 	/* 48ksr series */
	{SAMPLE_RATE_88KHZ, 0, 0, 0, 1}, 	/* 44.1ksr series */
	{SAMPLE_RATE_44KHZ, 0, 0, 1, 1}, 	/* 44.1ksr series */
	{SAMPLE_RATE_22KHZ, 1, 0, 1, 0}, 	/* 44.1ksr series */
	{SAMPLE_RATE_11KHZ, 1, 1, 1, 0}, 	/* 44.1ksr series */
};
/*
 * struct audio_pll_t
 * @brief The structure includes the pll clock and its setting parameters
 */
typedef struct {
    uint32_t pll_clk; /* audio pll clock */
    uint8_t pre_div; /* clock pre-divisor */
    uint8_t clk_div; /* clock divisor */
} audio_pll_t;

static const audio_pll_t audio_pll_selections[] = {
    {PLL_49152, 0, 0}, /* 48ksr series */
    {PLL_24576, 0, 1}, /* 48ksr series */
    {PLL_16384, 0, 2}, /* 48ksr series */
    {PLL_12288, 0, 3}, /* 48ksr series */
    {PLL_8192, 0, 4}, /* 48ksr series */
    {PLL_6144, 0, 5}, /* 48ksr series */
    {PLL_4096, 0, 6}, /* 48ksr series */
    {PLL_3072, 1, 5}, /* 48ksr series */
    {PLL_2048, 1, 6}, /* 48ksr series */
    {PLL_451584, 0, 0}, /* 44.1ksr series */
    {PLL_225792, 0, 1}, /* 44.1ksr series */
    {PLL_112896, 0, 3}, /* 44.1ksr series */
    {PLL_56448, 0, 5}, /* 44.1ksr series */
    {PLL_28224, 1, 5} /* 44.1ksr series */
};

/*
 * struct audio_i2s_pll_t
 * @brief The structure includes the pll clock and its setting parameters version2 for i2s
 */
typedef struct {
    uint32_t pll_clk; /* audio pll clock */
    uint8_t clk_div; /* divison */
} audio_i2s_pll_t;

static const audio_i2s_pll_t audio_i2s_pll_selections[] = {
	{PLL_65536, 0}, /* 48ksr series */
    {PLL_49152, 1}, /* 48ksr series */
    {PLL_24576, 3}, /* 48ksr series */
    {PLL_16384, 4}, /* 48ksr series */
    {PLL_12288, 5}, /* 48ksr series */
    {PLL_8192, 6}, /* 48ksr series */
    {PLL_6144, 7}, /* 48ksr series */
    {PLL_4096, 8}, /* 48ksr series */
    {PLL_3072, 9}, /* 48ksr series */
    {PLL_2048, 10}, /* 48ksr series */
		{PLL_1024, 11}, /* 48ksr series */
	{PLL_602112, 0}, /* 44.1ksr series */
    {PLL_451584, 1}, /* 44.1ksr series */
    {PLL_225792, 3}, /* 44.1ksr series */
    {PLL_112896, 5}, /* 44.1ksr series */
    {PLL_56448, 7}, /* 44.1ksr series */
    {PLL_28224, 9} /* 44.1ksr series */
};

#ifdef CONFIG_AUDIO_DEBUG_TRACE
/* audio debug trace global object */
struct audio_debug_trace_t audio_debug_trace;
#endif

static uint8_t audiopll0_refcount;
// static uint8_t audiopll1_refcount;

int8_t adc_reset_count = 0;
void adc_reset_control(bool enable)
{
  #if CONFIG_MIC_PIN_GPIO
    return;
  #else
  uint32_t key = irq_lock();
  if (enable == 1)
    adc_reset_count++;
  else
    adc_reset_count--;

  if (adc_reset_count < 0) {
      LOG_WRN("adc_reset_count < 0\n");
      adc_reset_count = 0;
  }
  irq_unlock(key);
  LOG_DBG("adc_reset_count:==%d\n", adc_reset_count);
  if (adc_reset_count != 0) {
      acts_reset_peripheral_deassert(RESET_ID_ADC);
      LOG_DBG("reset ADC\n");
  } else {
      acts_reset_peripheral_assert(RESET_ID_ADC);
      LOG_DBG("reset_assert ADC\n");
  }
  return;
  #endif
}

/* @brief Translate the sample rate from KHz to Hz */
uint32_t audio_sr_khz_to_hz(audio_sr_sel_e sr_khz)
{
	uint32_t ret_sample_hz;

    if (!(sr_khz % SAMPLE_RATE_11KHZ)) {
        /* 44.1KHz serials */
		ret_sample_hz = (sr_khz / SAMPLE_RATE_11KHZ) * 11025;
	} else {
	    /* 48KHz serials */
		ret_sample_hz = sr_khz * 1000;
	}

	return ret_sample_hz;
}

/* @brief Translate the sample rate from Hz to KHz */
audio_sr_sel_e audio_sr_hz_to_Khz(uint32_t sr_hz)
{
	uint32_t sr_khz;
	if (!(sr_hz % 11025)) {
		sr_khz = (sr_hz / 11025) * SAMPLE_RATE_11KHZ;
	} else {
		sr_khz = sr_hz / 1000;
	}
	return (audio_sr_sel_e)sr_khz;
}

/* @brief Get the audio pll setting by specified sample rate and mclk*/
int audio_get_pll_setting(audio_sr_sel_e sr_khz, a_mclk_type_e mclk,
	uint8_t *pre_div, uint8_t *clk_div, uint8_t *series)
{
    uint32_t sr_hz;
    int i;

	/* FPGA audio_pll main clock is 24Mhz, but the formal chipset is 98.304Mhz */
#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
	sr_khz *= 4;
#endif

    sr_hz = audio_sr_khz_to_hz(sr_khz);
    /* calculate the constant clock */
    sr_hz *= mclk;

    for (i = 0; i < ARRAY_SIZE(audio_pll_selections); i++) {
        if (sr_hz == audio_pll_selections[i].pll_clk) {
            *pre_div = audio_pll_selections[i].pre_div;
            *clk_div = audio_pll_selections[i].clk_div;
            if (i > 8)
            	*series = AUDIOPLL_44KSR;
            else
            	*series = AUDIOPLL_48KSR;
            break;
        }
    }

    /* Can not find the corresponding PLL setting */
    if (i == ARRAY_SIZE(audio_pll_selections)) {
        LOG_ERR("Failed to find audio pll setting sr:%d", sr_khz);
        *pre_div = 0xFF;
        *pre_div = 0xFF;
        *series = 0xFF;
        return -ENOEXEC;
    }

	LOG_DBG("sr_khz %d sr_hz %d, pre_div %d, clk_div %d", sr_khz, sr_hz, *pre_div, *clk_div);

    return 0;
}

/* @brief Get the audio pll setting by specified sample rate and mclk for i2s*/
int audio_get_pll_setting_i2s(uint16_t sr_khz, a_mclk_type_e mclk,
	uint8_t *div, uint8_t *series)
{
    uint32_t sr_hz=0;
    int i;

	/* FPGA audio_pll main clock is 24Mhz, but the formal chipset is 98.304Mhz */
#ifdef CONFIG_SOC_SERIES_LARK_FPGA
	sr_khz *= 4;
#endif
#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
	sr_khz *= 4;
#endif

	/* in case of MCLK from internal in slave mode */
	if (384 == sr_khz) { /* MCLK = 192fs x 2  */
		*series = AUDIOPLL_48KSR;
		*div = 0;
	} else if (352 == sr_khz) { /* MCLK = 176fs x 2  */
		*series = AUDIOPLL_44KSR;
		*div = 0;
	} else {
	    sr_hz = audio_sr_khz_to_hz(sr_khz);
	    /* calculate the constant clock */
	    sr_hz *= mclk;

	    for (i = 0; i < ARRAY_SIZE(audio_i2s_pll_selections); i++) {
	        if (sr_hz == audio_i2s_pll_selections[i].pll_clk) {
	            *div = audio_i2s_pll_selections[i].clk_div;
	            if (i > 10)
	            	*series = AUDIOPLL_44KSR;
	            else
	            	*series = AUDIOPLL_48KSR;
	            break;
	        }
	    }

	    /* Can not find the corresponding PLL setting */
	    if (i == ARRAY_SIZE(audio_i2s_pll_selections)) {
	        LOG_ERR("Failed to find audio pll setting sr:%d mclk:%d", sr_khz, mclk);
	        *div = 0xFF;
	        *series = 0xFF;
	        return -ENOEXEC;
	    }
	}

	LOG_INF("sr_khz %d sr_hz %d, div %d", sr_khz, sr_hz, *div);

    return 0;
}

/* @brief Get the audio pll usage info by the pll index */
static int audio_pll_get_usage(a_pll_type_e index, uint8_t *series)
{
	/* AUDIO_PLL0 */
	if (AUDIOPLL_TYPE_0 == index) {
		/* check AUDIO_PLL0 enable or not */
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			/* AUDIO_PLL0 disable */
			*series = 0xFF;
		} else if((sys_read32(AUDIO_PLL0_CTL) & CMU_AUDIOPLL0_CTL_APS0_MASK) >= 8) {
			/* AUDIO_PLL0 is 48k series */
			*series = AUDIOPLL_48KSR;
		} else {
			/* AUDIO_PLL0 is 44.1k series */
			*series = AUDIOPLL_44KSR;
		}
	} else {
		LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	LOG_INF("use AUDIO_PLL@%d => series:0x%x", index, *series);

	return 0;
}

/* @brief Get the audio pll aps by specified pll index */
int audio_pll_get_aps(a_pll_type_e index)
{
	uint32_t reg = -1;

	if (AUDIOPLL_TYPE_0 == index) {
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			LOG_ERR("AUDIO_PLL0 is not enable yet");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL0_CTL) & CMU_AUDIOPLL0_CTL_APS0_MASK;
		if (reg >= 8) {
			/* 48KHz sample rate seires */
			reg -= 8;
		}
	} else {
		LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	return reg;
}

/* @brief Set the audio pll aps by specified pll index */
int audio_pll_set_aps(a_pll_type_e index, audio_aps_level_e level)
{
	uint32_t reg;

	if (level > APS_LEVEL_8) {
		LOG_ERR("Invalid APS level setting %d", level);
		return -EINVAL;
	}

	if (AUDIOPLL_TYPE_0 == index) {
		if ((sys_read32(AUDIO_PLL0_CTL) & (1 << CMU_AUDIOPLL0_CTL_EN)) == 0) {
			LOG_INF("AUDIO_PLL0 is not enable");
			return -EPERM;
		}
		reg = sys_read32(AUDIO_PLL0_CTL);
		if ((reg  & CMU_AUDIOPLL0_CTL_APS0_MASK) >= 8) /* 48KHz sample rate seires */
			level += 8;
		reg &= ~CMU_AUDIOPLL0_CTL_APS0_MASK;
		sys_write32(reg | CMU_AUDIOPLL0_CTL_APS0(level), AUDIO_PLL0_CTL);
	} else {
		LOG_ERR("Invalid AUDIO_PLL type %d", index);
		return -EINVAL;
	}

	return 0;
}

/* @brief Get the src clock in HZ for the sample rate translation */
int audio_get_pll_sample_rate_dac(uint8_t div, uint8_t fir_div,
	uint8_t fir2x_div, uint8_t cic_div, a_pll_type_e index)
{
	uint8_t series, start, end;
	int ret, i;

	ret = audio_pll_get_usage(index, &series);
	if (ret)
		return ret;

	if (AUDIOPLL_48KSR == series) {
		start = 0;
		end = 7;
	} else if (AUDIOPLL_44KSR == series) {
		start = 7;
		end = ARRAY_SIZE(audio_dac_pll_selections);
	} else {
		LOG_ERR("Error series %d", series);
		return -EPERM;
	}

	for (i = start; i < end; i++) {
		if ((audio_dac_pll_selections[i].div == div)
			&& ((audio_dac_pll_selections[i].fir_div) == fir_div)
			&& ((audio_dac_pll_selections[i].fir2x_div) == fir2x_div)
			&& ((audio_dac_pll_selections[i].cic_div) == cic_div))  {
			ret = audio_dac_pll_selections[i].sr_khz;
			ret = (int)audio_sr_hz_to_Khz(ret);
			break;
		}
	}

	if (i == end) {
		LOG_ERR("Failed to translate sr div:%d fir_div%d fir2x_div:%d cic_div:%d index %d"
			,div, fir_div, fir2x_div, cic_div, index);
		ret = -EFAULT;
	}

	return ret;
}

/* @brief Get the dac pll setting by specified sample rate */
int audio_get_pll_setting_dac(audio_sr_sel_e sr_khz,uint8_t *div, 
	uint8_t *fir_div,uint8_t *fir2x_div, uint8_t *cic_div,uint8_t *series)
{
    int i;

	/* FPGA audio_pll main clock is 24Mhz, but the formal chipset is 98.304Mhz */
#ifdef CONFIG_SOC_SERIES_LEOPARD_FPGA
	sr_khz *= 4;
#endif

    for (i = 0; i < ARRAY_SIZE(audio_dac_pll_selections); i++) {
        if (sr_khz == audio_dac_pll_selections[i].sr_khz) {
            *div = audio_dac_pll_selections[i].div;
            *fir_div = audio_dac_pll_selections[i].fir_div;
			*fir2x_div = audio_dac_pll_selections[i].fir2x_div;
            *cic_div = audio_dac_pll_selections[i].cic_div;
            if (i > 6)
				*series = AUDIOPLL_44KSR;
            else
				*series = AUDIOPLL_48KSR;
            break;
        }
    }

    /* Can not find the corresponding PLL setting */
    if (i == ARRAY_SIZE(audio_dac_pll_selections)) {
        LOG_ERR("Failed to find audio pll setting sr:%d", sr_khz);
        *series = 0xFF;
        return -ENOEXEC;
    }

	LOG_DBG("sr_khz:%d div:%d, fir_div:%d, fir2x_div:%d, cic_div:%d, series:%d",
		sr_khz, *div,*fir_div, *fir2x_div, *cic_div, *series);

    return 0;
}

/* @brief Get the pll clock in HZ for the sample rate translation */
int audio_get_pll_sample_rate(a_mclk_type_e mclk, uint8_t pre_div, uint8_t clk_div, a_pll_type_e index)
{
	uint8_t series, start, end;
	int ret, i;

	ret = audio_pll_get_usage(index, &series);
	if (ret)
		return ret;

	if (AUDIOPLL_48KSR == series) {
		start = 0;
		end = 9;
	} else if (AUDIOPLL_44KSR == series) {
		start = 9;
		end = ARRAY_SIZE(audio_pll_selections);
	} else {
		LOG_ERR("Error series %d", series);
		return -EPERM;
	}

	for (i = start; i < end; i++) {
		if ((audio_pll_selections[i].pre_div == pre_div)
			&& (audio_pll_selections[i].clk_div) == clk_div) {
			ret = audio_pll_selections[i].pll_clk / mclk;
			ret = (int)audio_sr_hz_to_Khz(ret);
			break;
		}
	}

	if (i == end) {
		LOG_ERR("Failed to translate sr pre_div:%d clk_div%d pll_index:%d",
					pre_div, clk_div, index);
		ret = -EFAULT;
	}

	return ret;
}

/* @brief Get the pll clock in HZ for the sample rate translation for i2s */
int audio_get_pll_sample_rate_i2s(a_mclk_type_e mclk, uint8_t clk_div, a_pll_type_e index)
{
	uint8_t series, start, end;
	int ret, i;

	ret = audio_pll_get_usage(index, &series);
	if (ret)
		return ret;

	if (AUDIOPLL_48KSR == series) {
		start = 0;
		end = 11;
	} else if (AUDIOPLL_44KSR == series) {
		start = 11;
		end = ARRAY_SIZE(audio_i2s_pll_selections);
	} else {
		LOG_ERR("Error series %d", series);
		return -EPERM;
	}

	for (i = start; i < end; i++) {
		if (audio_i2s_pll_selections[i].clk_div == clk_div) {
			ret = audio_i2s_pll_selections[i].pll_clk / mclk;
			/* 235.2kfs is not divisible by 11025 */
			if (235200 == ret)
				ret = 235;
			else
				ret = (int)audio_sr_hz_to_Khz(ret);
			break;
		}
	}

	if (i == end) {
		LOG_ERR("Failed to translate sr clk_div%d pll_index:%d",
					clk_div, index);
		ret = -EFAULT;
	}

	return ret;
}

/* @brief audio pll set */
void audio_pll_set(a_pll_type_e index, a_pll_series_e series)
{
	uint32_t reg;
	/* Enable AUDIO_PLL */
	reg = sys_read32(AUDIO_PLL0_CTL) & (~CMU_AUDIOPLL0_CTL_APS0_MASK);
	reg |= (1 << CMU_AUDIOPLL0_CTL_EN);

	if (AUDIOPLL_44KSR == series)
		reg |= (0x04 << CMU_AUDIOPLL0_CTL_APS0_SHIFT);
	else
		reg |= (0x0c << CMU_AUDIOPLL0_CTL_APS0_SHIFT);

	sys_write32(reg, AUDIO_PLL0_CTL);

	++audiopll0_refcount;

	LOG_DBG("audio_pll_set_AUDIO_PLL0_CTL - 0x%x", sys_read32(AUDIO_PLL0_CTL));
}

/* @brief unset audio pll */
void audio_pll_unset(a_pll_type_e index)
{
	--audiopll0_refcount;
	if (!audiopll0_refcount)
		sys_write32(0, AUDIO_PLL0_CTL);
}

/* @brief Check and config the audio pll */
int audio_pll_check_config(a_pll_series_e series, uint8_t *index)
{
	int ret;
	uint8_t get_series, pll_index = 0xFF;
	ret = audio_pll_get_usage(AUDIOPLL_TYPE_0, &get_series);
	if (ret) {
		LOG_ERR("Get AUDIO_PLL0 error %d", ret);
		return ret;
	}
	pll_index = AUDIOPLL_TYPE_0;

	if (0xFF == pll_index) {
		LOG_ERR("Failed to find the available pll %d", series);
		LOG_INF("AUDIO_PLL0: 0x%x", sys_read32(AUDIO_PLL0_CTL));
		LOG_INF("AUDIO_PLL1: 0x%x", sys_read32(AUDIO_PLL1_CTL));
		*index = 0xFF;
		return -ENOENT;
	}

	*index = pll_index;
	audio_pll_set(pll_index, series);

	return 0;
}

