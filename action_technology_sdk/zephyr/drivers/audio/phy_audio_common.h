/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AUDIO physical common API
 */

#ifndef __PHY_AUDIO_COMMON_H__
#define __PHY_AUDIO_COMMON_H__

#include <device.h>
#ifdef CONFIG_SOC_SERIES_LARK
#include "audio_acts_utils.h"
#endif
#ifdef CONFIG_SOC_SERIES_LEOPARD
#include "phy_leopard/audio_acts_utils.h"
#endif
#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

/* @brief Definition for the physical audio control internal commands */

/************************ Common IO commands **************************/
#define PHY_CMD_BASE                         (0xFF)
#define PHY_CMD_OFFSET                       (64)

#define PHY_CMD_DUMP_REGS                    (PHY_CMD_BASE + 1)
/*! Dump the audio in/out controller regiser for debug
 *  int phy_audio_control(dev, #PHY_CMD_DUMP_REGS, NULL)
 */

#define PHY_CMD_FIFO_GET                     (PHY_CMD_BASE + 2)
/*! Get a FIFO for using from the speicified audio channel
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_GET, void *param)
 */

#define PHY_CMD_FIFO_PUT                     (PHY_CMD_BASE + 3)
/*! Put the FIFO to the speicified audio channel
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_PUT, NULL)
 */

#define PHY_CMD_FIFO_DRQ_LEVEL_GET           (PHY_CMD_BASE + 16)
/*! Enable the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_DRQ_LEVEL_GET, uint32_t *io)
 *  For #io can refer to #PHY_FIFO_CMD(i, x)
 */

#define PHY_CMD_FIFO_DRQ_LEVEL_SET            (PHY_CMD_BASE + 17)
/*! Enable the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_FIFO_DRQ_LEVEL_SET, uint32_t *io)
 *  #io can refer to #PHY_FIFO_CMD(i, x)
 */

#define PHY_CMD_GET_AIN_DMA_INFO              (PHY_CMD_BASE + 18)
/*! Get the audio in DMA information such as dma slot id from phydical audio device.
 *  int phy_audio_control(dev, #PHY_CMD_GET_AIN_DMA_INFO, struct audio_in_dma_info *info)
 */

#define PHY_CMD_GET_AOUT_DMA_INFO             (PHY_CMD_BASE + 19)
/*! Get the audio out DMA information such as dma slot id from phydical audio device.
 *  int phy_audio_control(dev, #PHY_CMD_GET_AOUT_DMA_INFO, struct audio_out_dma_info *info)
 */

#define PHY_CMD_CHANNEL_START                 (PHY_CMD_BASE + 20)
/*! Start the phydical audio device.
 *  int phy_audio_control(dev, #PHY_CMD_CHANNEL_START, void *param)
 */

/************************ DAC IO commands **************************/
#define PHY_CMD_DAC_BASE                     (PHY_CMD_BASE + PHY_CMD_OFFSET)

#define PHY_CMD_DAC_WAIT_EMPTY               (PHY_CMD_DAC_BASE + 1)
/*! Check and wait the empty pending of DAC FIFO by DAC FIFO index
 *  int phy_audio_control(dev, #PHY_CMD_DAC_CHECK_EMPTY, uint8_t *fifo_idx)
 */

#define PHY_CMD_DAC_FIFO_GET_SAMPLE_CNT      (PHY_CMD_DAC_BASE + 2)
/*! Get the DAC FIFO sample counter by DAC FIFO index.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO0_GET_SAMPLE_CNT, uint32_t *idx)
 */

#define PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT    (PHY_CMD_DAC_BASE + 3)
/*! Reset the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT, uint8_t *idx)
 */

#define PHY_CMD_DAC_FIFO_DISABLE_SAMPLE_CNT   (PHY_CMD_DAC_BASE + 4)
/*! Disable the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT, uint8_t *idx)
 */

#define PHY_CMD_DAC_FIFO_ENABLE_SAMPLE_CNT    (PHY_CMD_DAC_BASE + 5)
/*! Enable the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO_RESET_SAMPLE_CNT, uint8_t *idx)
 */

#define PHY_CMD_DAC_FIFO_VOLUME_GET           (PHY_CMD_DAC_BASE + 6)
/*! Get the DAC FIFO volume value.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO_VOLUME_GET, uint32_t *io)
 *  #io can refer to #PHY_FIFO_CMD(i, x)
 */

#define PHY_CMD_DAC_FIFO_VOLUME_SET           (PHY_CMD_DAC_BASE + 7)
/*! Enable the specified index of the DAC FIFO sample counter function.
 *  int phy_audio_control(dev, #PHY_CMD_DAC_FIFO_VOLUME_SET, uint32_t *io)
 *  #io can refer to #PHY_FIFO_CMD(i, x)
 */

#define PHY_CMD_CLAIM_WITH_128FS              (PHY_CMD_DAC_BASE + 8)
/*! Claim that current audio channels exist 128fs channel typically spdiftx which used in multi-linkage mode
 *  int phy_audio_control(dev, #PHY_CMD_CLAIM_WITH_128FS, NULL)
 */

#define PHY_CMD_CLAIM_WITHOUT_128FS           (PHY_CMD_DAC_BASE + 9)
/*! Claim that current audio channels do not exist 128fs channel typically spdiftx which used in multi-linkage mode
 *  int phy_audio_control(dev, #PHY_CMD_CLAIM_WITHOUT_128FS, NULL)
 */

#define PHY_CMD_ANC_MIX2_DAC_ENABLE           (PHY_CMD_DAC_BASE + 10)
/*!Enable ANC data stream MIX to DAC left/right channels.
 *  int phy_audio_control(dev, #PHY_CMD_ANC_MIX2_DAC_ENABLE, uint8_t *lr_sel)
 *  If lr_sel == (LEFT_CHANNEL_SEL | RIGHT_CHANNEL_SEL) stands for both ANC left and right data will mix to DAC.
 */

#define PHY_CMD_ANC_MIX2_DAC_LR_INVERSE       (PHY_CMD_DAC_BASE + 11)
/*! Enable the ANC data stream will polarity inverse when mixed to DAC .
 *  int phy_audio_control(dev, #PHY_CMD_ANC_MIX2_DAC_LR_INVERSE, uint8_t *en)
 *  If (en == 1) ANC data will inverse others will not inverse.
 */

/************************ I2STX IO commands **************************/
#define PHY_CMD_I2STX_BASE                   (PHY_CMD_DAC_BASE + PHY_CMD_OFFSET)

#define PHY_CMD_I2STX_IS_OPENED              (PHY_CMD_I2STX_BASE + 1)
/*! Get the open status of i2stx channel
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_IS_OPENED, uint8_t *status)
 */

#define PHY_CMD_I2STX_DISABLE_DEVICE         (PHY_CMD_I2STX_BASE + 2)
/*! Disable I2STX hardware resouces include BCLK/LRCLK output signals.
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_DISABLE_DEVICE, NULL)
 */

#define PHY_CMD_I2STX_CLK_SET                (PHY_CMD_I2STX_BASE + 3)
/*! Set the I2STX clock by other physical audio device such as I2SRX.
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_CLK_SET, uint8_t *sr)
 */

#define PHY_CMD_I2STX_IS_MCLK_128FS          (PHY_CMD_I2STX_BASE + 4)
/*! Check if the MCLK of I2STX is a 128FS.
 *  int phy_audio_control(dev, #PHY_CMD_I2STX_IS_MCLK_128FS, uint8_t *is_en)
 */

#define PHY_CMD_I2S_LOOPBACK          (PHY_CMD_I2STX_BASE + 5)
/*! I2STX send clk and  data to I2SRX.
 *  int phy_audio_control(dev, #PHY_CMD_I2S_LOOPBACK, uint8_t *is_en)
 */

/************************ I2SRX IO commands **************************/
#define PHY_CMD_I2SRX_BASE                   (PHY_CMD_I2STX_BASE + PHY_CMD_OFFSET)

#define PHY_CMD_I2SRX_IS_OPENED              (PHY_CMD_I2SRX_BASE + 1)
/*! Get the open status of i2srx channel
 *  int phy_audio_control(dev, #PHY_CMD_I2SRX_IS_OPENED, uint8_t *status)
 */

/************************ ADC IO commands **************************/
#define PHY_CMD_ADC_BASE                     (PHY_CMD_I2SRX_BASE + PHY_CMD_OFFSET)

#define PHY_CMD_ADC_DIGITAL_ENABLE            (PHY_CMD_ADC_BASE + 1)
/*! Enable the ADC channels 0/1/2 at the same time. For now only support 2 input device to run.
 *  int phy_audio_control(dev, #PHY_CMD_ADC_DIGITAL_ENABLE, struct aduio_in_adc_en *ctl)
 */

#define PHY_CMD_GET_AUDIOPLL_IDX              (PHY_CMD_ADC_BASE + 2)
/*! Get the AUDIOPLL index that used by physical audio device.
 *  int phy_audio_control(dev, #PHY_CMD_GET_AUDIOPLL_IDX, uint8_t *idx)
 */

#define PHY_CMD_ADC_GAIN_CONFIG               (PHY_CMD_ADC_BASE + 3)
/*! Config the ADC GAIN to physical ADC device.
 *  int phy_audio_control(dev, #PHY_CMD_ADC_GAIN_CONFIG, adc_setting_t *setting)
 */

#define PHY_CMD_IS_ADC_BUSY                   (PHY_CMD_DAC_BASE + 4)
/*! Query ADC is busy or not.
 *  int phy_audio_control(dev, #PHY_CMD_IS_ADC_BUSY, uint8_t *is_busy)
 */

/**
 * enum audio_fifouse_sel_e
 * @brief FIFO use object select
 */
typedef enum {
	FIFO_SEL_CPU = 0, /* FIFO's user is CPU */
	FIFO_SEL_DMA, /* FIFO's user is DMA */
	FIFO_SEL_ASRC, /* FIFO's user is ASRC */
	FIFO_SEL_DSP, /* FIFO's user is DSP */
	FIFO_SEL_DSP_DMA /* FIFO's user is DSP DMA */
} audio_fifouse_sel_e;

/*
 * enum audio_dma_width_e
 * @brief DMA transfer width configuration
 */
typedef enum {
	DMA_WIDTH_32BITS = 0,
	DMA_WIDTH_16BITS
} audio_dma_width_e;

/*
 * enum audio_i2s_srd_period_e
 * @brief I2S sample rate detect period selection
 */
typedef enum {
	I2S_SRD_2LRCLK = 0,
	I2S_SRD_4LRCLK
} audio_i2s_srd_period_e;

/*
 * enum audio_anc_work_mode
 * @brief ANC work mode selection
 */
typedef enum {
	FF_IIR_US = 0, /* FF filter => IIR => US => DAC */
	FF_DSFIFO_USFIFO, /* FF filter => DSFIFO => DSP =>USFIFO => DAC */
} audio_anc_work_mode;

/**
 * struct audio_dma_dt
 * @brief audio dma resource from device tree.
 */
struct audio_dma_dt {
	const char *dma_dev_name; /* DMA device name */
	uint32_t dma_chan; /* DMA channel */
	uint8_t dma_id; /* DMA slot id */
};

/**
 * struct audio_in_dma_info
 * @brief audio in dma information structure
 */
struct audio_in_dma_info {
	uint16_t input_dev; /* input audio device */
	struct audio_dma_dt dma_info; /* dma info */
};

/**
 * struct aduio_in_adc_en
 * @brief audio in adc enable control
 */
struct aduio_in_adc_en {
	uint16_t *input_dev_array;
	uint8_t input_dev_num;
};

/**
 * struct audio_out_dma_info
 * @brief audio out dma information structure
 */
struct audio_out_dma_info {
	uint8_t fifo_type; /* audio out fifo type */
	struct audio_dma_dt dma_info; /* dma info */
};

/**
 * struct audio_debug_trace
 * @brief audio debug trace object.
 */
struct audio_debug_trace_t {
#define AUDIO_DEBUG_TRACE_START_FLAG BIT(0)
	uint32_t sec_timestamp;
	uint32_t trace_exec_timestamp;
	uint32_t trace_start_timestamp;
	uint32_t max_exec_time;
	uint32_t total_exec_time;
	uint32_t counter_per_sec;
	uint32_t counter_per_sec_bak;
	uint32_t total_counter;
	uint8_t flags;
};

extern struct audio_debug_trace_t audio_debug_trace;

static inline void audio_debug_trace_info(void)
{
	printk("audio trace total %dus {max_exec_time:%dus,total_exec_time:%dus}\n",
			k_cyc_to_us_floor32(k_cycle_get_32() - audio_debug_trace.trace_start_timestamp),
			k_cyc_to_us_floor32(audio_debug_trace.max_exec_time),
			k_cyc_to_us_floor32(audio_debug_trace.total_exec_time));
	printk("audio trace coutner_per_sec_bak:%d total_counter:%d\n",
			audio_debug_trace.counter_per_sec_bak, audio_debug_trace.total_counter);
}

static inline void audio_debug_trace_start(void)
{
	audio_debug_trace.trace_exec_timestamp = k_cycle_get_32();
	audio_debug_trace.counter_per_sec++;
	audio_debug_trace.total_counter++;

	if (!(audio_debug_trace.flags & AUDIO_DEBUG_TRACE_START_FLAG)) {
		audio_debug_trace.trace_start_timestamp = audio_debug_trace.trace_exec_timestamp;
		audio_debug_trace.flags |= AUDIO_DEBUG_TRACE_START_FLAG;
	}
}

static inline void audio_debug_trace_end(void)
{
	uint32_t delta, cur_time = k_cycle_get_32();

	delta = cur_time - audio_debug_trace.trace_exec_timestamp;
	if (delta > audio_debug_trace.max_exec_time)
		audio_debug_trace.max_exec_time = delta;

	audio_debug_trace.total_exec_time += delta;

	delta = k_cyc_to_us_floor32(cur_time - audio_debug_trace.sec_timestamp);
	if (delta > 1000000UL) {
		//audio_debug_trace_info();
		audio_debug_trace.sec_timestamp = cur_time;
		audio_debug_trace.counter_per_sec_bak = audio_debug_trace.counter_per_sec;
		audio_debug_trace.counter_per_sec = 0;
	}
}

static inline void audio_debug_trace_clear(void)
{
	memset(&audio_debug_trace, 0, sizeof(struct audio_debug_trace_t));
}

/**
 *  @brief The macro to combine with audio dma fifo structure and driver config dma information.
 */
#define AUDIO_DMA_FIFO_DEF(m, n) \
	.dma_fifo##n = { \
		.dma_dev_name = CONFIG_DMA_0_NAME, \
		.dma_chan = CONFIG_AUDIO_##m##_0_FIFO##n##_DMA_CHAN, \
		.dma_id = CONFIG_AUDIO_##m##_0_FIFO##n##_DMA_ID \
	}

/* @brief The macro to extend the device features in configuration */
#define PHY_DEV_FEATURE_DEF(x) .features.v.x
#define PHY_DEV_FEATURE(x) (cfg->features.v.x)

/* @brief the physical IO commands which append with FIFO index */
#define PHY_FIFO_INDEX_OFFSET			(16)
#define PHY_FIFO_CMD(i, x)				(((i) << PHY_FIFO_INDEX_OFFSET) | (x))
#define PHY_GET_FIFO_CMD_INDEX(x)		((x) >> PHY_FIFO_INDEX_OFFSET)
#define PHY_GET_FIFO_CMD_VAL(x)			((x) & 0xFFFF)

/* @brief the macro to show the infomation from device tree */
#define PHY_DEV_SHOW_DT_INFO            (0)

#ifdef CONFIG_CFG_DRV
/* @brief the macro to get the external configuration generated by PC tool */
#define PHY_AUDIO_CFG(x, item_key, item) \
			{ uint32_t val = 0; \
				if (!cfg_get_by_key(item_key, &val, sizeof((x).item))) \
					return __LINE__; \
				LOG_INF("%s:%d", #item, val); \
				(x).item = val; \
			}
#define PHY_AUDIO_PIN_NUM_CFG(x) ((x) & 0xFF)
#define PHY_AUDIO_PIN_MFP_CFG(x) ((x) >> 8)
#endif

/*
 * @struct phy_audio_driver_api
 * @brief Audio physical layer common API that standards the common behaviors of audio-out and audio-in channels.
 */
struct phy_audio_driver_api {
	int (*audio_enable)(struct device *dev, void *param);
	int (*audio_disable)(struct device *dev, void *param);
	int (*audio_ioctl)(struct device *dev, uint32_t cmd, void *param);
};

/*
 * @brief Enable one audio in/out channel by the specified paremeters.
 * @param dev: The physical audio device handler.
 * @param param: The parameters to enable physical audio in/out channel.
 * @return 0 on success, negative errno code on fail.
 */
static inline int phy_audio_enable(struct device *dev, void *param)
{
	if (!dev)
		return -EINVAL;

	const struct phy_audio_driver_api *api = dev->api;

	return api->audio_enable(dev, param);
}

/*
 * @brief Disable the audio in/out channel by the specified device handler.
 * @param dev: The physical audio device handler to close.
 * @param param: The parameters to disable physical audio in/out channel.
 * @return 0 on success, negative errno code on fail.
 */
static inline int phy_audio_disable(struct device *dev, void *param)
{
	if (!dev)
		return -EINVAL;

	const struct phy_audio_driver_api *api = dev->api;

	return api->audio_disable(dev, param);
}

/*
 * @brief The io-commands that coresponding with the appropriative channel to control the audio channel dynamically.
 * @param dev: The physical audio device handler.
 * @param param: The io-commands to control physical audio channel.
 * @return 0 on success, negative errno code on fail.
 */
static inline int phy_audio_control(struct device *dev, uint32_t cmd, void *param)
{
	if (!dev)
		return -EINVAL;

	const struct phy_audio_driver_api *api = dev->api;

	return api->audio_ioctl(dev, cmd, param);
}

#endif /* __PHY_AUDIO_COMMON_H__ */
