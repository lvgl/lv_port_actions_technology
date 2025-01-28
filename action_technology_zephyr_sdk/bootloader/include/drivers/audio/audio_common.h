/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio_common.h
 * @brief The common audio structures and definitions used within the audio
 */

#ifndef __AUDIO_COMMON_H__
#define __AUDIO_COMMON_H__

/* @brief Definition for FIFO counter */
#define AOUT_FIFO_CNT_MAX               (0xFFFF) /* The max value of the audio out FIFO counter */
#define AOUT_SDM_CNT_MAX                (0xFFFFFFF) /* The max value of the audio out SDM counter */

/* @brief Definition the invalid type of audio FIFO */
#define AUDIO_FIFO_INVALID_TYPE         (0xFF)

/**
 * @brief Definition for the audio channel type
 * @note If there is a multi-linkage reguirement, it allows using "or" channels.
 *       And it is only allow the linkage usage of the output channels.
 * @par example:
 * (AUDIO_CHANNEL_DAC | AUDIO_CHANNEL_I2STX) stands for DAC linkage with I2S.
 */
#define AUDIO_CHANNEL_DAC               (1 << 0)
#define AUDIO_CHANNEL_I2STX             (1 << 1)
#define AUDIO_CHANNEL_SPDIFTX           (1 << 2)
#define AUDIO_CHANNEL_ADC               (1 << 3)
#define AUDIO_CHANNEL_I2SRX             (1 << 4)
#define AUDIO_CHANNEL_SPDIFRX           (1 << 5)
#define AUDIO_CHANNEL_PDMTX             (1 << 6)

/**
 * @brief Definition for the audio channel status
 */
#define AUDIO_CHANNEL_STATUS_BUSY       (1 << 0) /* channel FIFO still busy status */
#define AUDIO_CHANNEL_STATUS_ERROR      (1 << 1) /* channel error status */

/**
 * enum audio_sr_sel_e
 * @brief The sample rate choices
 */
typedef enum {
	SAMPLE_RATE_8KHZ   = 8,
	SAMPLE_RATE_11KHZ  = 11,   /* 11.025KHz */
	SAMPLE_RATE_12KHZ  = 12,
	SAMPLE_RATE_16KHZ  = 16,
	SAMPLE_RATE_22KHZ  = 22,   /* 22.05KHz */
	SAMPLE_RATE_24KHZ  = 24,
	SAMPLE_RATE_32KHZ  = 32,
	SAMPLE_RATE_44KHZ  = 44,   /* 44.1KHz */
	SAMPLE_RATE_48KHZ  = 48,
	SAMPLE_RATE_64KHZ  = 64,
	SAMPLE_RATE_88KHZ  = 88,   /* 88.2KHz */
	SAMPLE_RATE_96KHZ  = 96,
	SAMPLE_RATE_176KHZ = 176,  /* 176.4KHz */
	SAMPLE_RATE_192KHZ = 192,
} audio_sr_sel_e;

/*
 * enum audio_aps_level_e
 * @brief The APS for the AUDIO_PLL tuning dynamically
 */
typedef enum {
	APS_LEVEL_1 = 0, /* 44.0006 / 47.8795 */
	APS_LEVEL_2, /* 44.0245 / 47.9167 */
	APS_LEVEL_3, /* 44.0625 / 47.9651 */
	APS_LEVEL_4, /* 44.0848 / 47.9805 */
	APS_LEVEL_5, /* 44.0995 / 48.0018 */
	APS_LEVEL_6, /* 44.1176 / 48.0239 */
	APS_LEVEL_7, /* 44.1323 / 48.0469 */
	APS_LEVEL_8, /* 44.1964 / 48.1086 */
} audio_aps_level_e;

/*
 * enum audio_ch_mode_e
 * @brief Define the channel mode
 */
typedef enum {
	MONO_MODE = 1,
	STEREO_MODE,
} audio_ch_mode_e;

/*
 * enum audio_outfifo_sel_e
 * @brief Audio out fifo selection
 */
typedef enum {
	AOUT_FIFO_DAC0 = 0, /* DAC FIFO0 */
	AOUT_FIFO_DAC1, /* DAC FIFO1 */
	AOUT_FIFO_I2STX0, /* I2STX FIFO */
	AOUT_FIFO_DAC1_ONLY_SPDIF, /* DAC FIFO1 only used by SPDIFTX */
} audio_outfifo_sel_e;

/*
 * enum a_infifo_sel_e
 * @brief Audio in fifo selection
 */
typedef enum {
	AIN_FIFO_ADC0 = 0, /* ADC FIFO0 */
	AIN_FIFO_ADC1 = 0, /* ADC FIFO1 */
	AIN_FIFO_I2SRX0, /* I2SRX FIFO */
	AIN_FIFO_SPIDFRX0, /* SPIDFRX FIFO */
} audio_infifo_sel_e;

/*
 * enum audio_i2s_fmt_e
 * @brief I2S transfer format selection
 */
typedef enum {
	I2S_FORMAT = 0,
	LEFT_JUSTIFIED_FORMAT,
	RIGHT_JUSTIFIED_FORMAT,
	TDM_FORMAT
} audio_i2s_fmt_e;

/*
 * enum audio_i2s_bclk_e
 * @brief Rate of BCLK with LRCLK
 */
typedef enum {
	I2S_BCLK_32BITS = 0,
	I2S_BCLK_16BITS
} audio_i2s_bclk_e;

/*
 * enum audio_i2s_mode_e
 * @brief I2S mode selection
 */
typedef enum {
	I2S_MASTER_MODE = 0,
	I2S_SLAVE_MODE
} audio_i2s_mode_e;

/*
 * enum audio_i2s_srd_wl_e
 * @brief I2S sample rate width length detect
 */
typedef enum {
	SRDSTA_WL_32RATE = 0, /* BCLK = 32LRCLK */
	SRDSTA_WL_64RATE /* BCLK = 64LRCLK */
} audio_i2s_srd_wl_e;

/*
 * enum audio_ch_width_e
 * @brief The effective data width of audio channel
 * @note DAC and SPDIF only support #CHANNEL_WIDTH_16BITS and #CHANNEL_WIDTH_24BITS
 *		I2S support #CHANNEL_WIDTH_16BITS, #CHANNEL_WIDTH_20BITS and #CHANNEL_WIDTH_24BITS
 *		ADC support #CHANNEL_WIDTH_16BITS and #CHANNEL_WIDTH_18BITS
 */
typedef enum {
	CHANNEL_WIDTH_16BITS = 0,
	CHANNEL_WIDTH_18BITS,
	CHANNEL_WIDTH_20BITS,
	CHANNEL_WIDTH_24BITS
} audio_ch_width_e;

/*
 * enum audio_trigger_src
 * @brief The external sources of IRQ signals to trigger DAC/ADC digital start.
 */
typedef enum {
	TRIGGER_SRC_TIMER0 = 0,
	TRIGGER_SRC_TIMER1,
	TRIGGER_SRC_TIMER2,
	TRIGGER_SRC_TIMER3,
	TRIGGER_SRC_TIMER4,
	TRIGGER_SRC_TWS_IRQ0,
	TRIGGER_SRC_TWS_IRQ1,
} audio_trigger_src;

/*
 * enum a_lr_chl_e
 * @brief Left/Right channel selection
 */
typedef enum {
	LEFT_CHANNEL_SEL = (1 << 0),
	RIGHT_CHANNEL_SEL = (1 << 1)
} a_lr_chl_e;

/*!
 * enum audio_ext_pa_ctrl_e
 * @brief control the external PA operations.
 */
typedef enum {
	EXTERNAL_PA_ENABLE = 0,
	EXTERNAL_PA_DISABLE,
	EXTERNAL_PA_MUTE,
	EXTERNAL_PA_UNMUTE,
} audio_ext_pa_ctrl_e;

#if defined(CONFIG_SOC_SERIES_LARK)

#define ADC_CH_NUM_MAX                         (4)

/*!
 * The macro to define the INPUT attributes which connects to ADCx channels.
 * Hareware owns the rules that one ADC only can map dedicated INPUTx.
 * ----------------------------------------------
 * ADC0: ADC_CH_INPUT0P/ADC_CH_INPUT0NP_DIFF
 * ----------------------------------------------
 * ADC1: ADC_CH_INPUT1P/ADC_CH_INPUT1NP_DIFF
 * ----------------------------------------------
 * ADC2: ADC_CH_INPUT0N/ADC_CH_INPUT2P/ADC_CH_INPUT3N/ADC_CH_INPUT2NP_DIFF/ADC_CH_INPUT0N | ADC_CH_INPUT2P ...
 * ----------------------------------------------
 * ADC3: ADC_CH_INPUT1N/ADC_CH_INPUT2N/ADC_CH_INPUT3P/ADC_CH_INPUT3NP_DIFF/ADC_CH_INPUT1N | ADC_CH_INPUT2N ...
 * ----------------------------------------------
 */
#define ADC_CH_INPUT0N                         (1 << 0) /* ADC channel INPUT0 negative point */
#define ADC_CH_INPUT0P                         (1 << 1) /* ADC channel INPUT0 positive point */
#define ADC_CH_INPUT0NP_DIFF                   (ADC_CH_INPUT0N | ADC_CH_INPUT0P)
#define ADC_CH_INPUT1N                         (1 << 2) /* ADC channel INPUT1 negative point */
#define ADC_CH_INPUT1P                         (1 << 3) /* ADC channel INPUT1 positive point */
#define ADC_CH_INPUT1NP_DIFF                   (ADC_CH_INPUT1N | ADC_CH_INPUT1P)
#define ADC_CH_INPUT2N                         (1 << 4) /* ADC channel INPUT2 negative point */
#define ADC_CH_INPUT2P                         (1 << 5) /* ADC channel INPUT2 positive point */
#define ADC_CH_INPUT2NP_DIFF                   (ADC_CH_INPUT2N | ADC_CH_INPUT2P)
#define ADC_CH_INPUT3N                         (1 << 6) /* ADC channel INPUT3 negative point */
#define ADC_CH_INPUT3P                         (1 << 7) /* ADC channel INPUT3 positive point */
#define ADC_CH_INPUT3NP_DIFF                   (ADC_CH_INPUT3N | ADC_CH_INPUT3P)
#define ADC_CH_DISABLE                         (0)
#define ADC_CH_DMIC                            (0xFF)

#endif

/*!
 * enum audio_ext_pa_class_e
 * @brief The external PA class mode selection
 * @note CLASS_AB: higher power consume and lower radiation.
 * ClASS_D: lower power consume and higher radiation.
 */
typedef enum {
	EXT_PA_CLASS_AB = 0,
	EXT_PA_CLASS_D
} audio_ext_pa_class_e;

/*!
 * struct audio_spdif_ch_status_t
 * @brief The SPDIF channel status setting
 */
typedef struct {
	uint32_t csl; /* The low 32bits of channel status */
	uint16_t csh; /* The high 16bits of channel status */
} audio_spdif_ch_status_t;

/*!
 * struct audio_reload_t
 * @brief Audio out reload mode configuration.
 * @note If enable reload function the audio driver will transfer the same buffer address and notify the user when the buffer is half full or full.
 */
typedef struct {
	uint8_t *reload_addr; /*!< Reload buffer address to transfer */
	uint32_t reload_len;  /*!< The length of the reload buffer */
} audio_reload_t;

#if defined(CONFIG_SOC_SERIES_LARK)

/*!
 * struct audio_input_map_t
 * @brief The mapping relationship between audio device and ADC channels input.
 */
typedef struct {
	uint16_t audio_dev;
	uint8_t ch0_input;
	uint8_t ch1_input;
	uint8_t ch2_input;
	uint8_t ch3_input;
} audio_input_map_t;

/*!
 * struct dac_trigger_ctl_t
 * @brief Control DAC the external trigger behavior.
 */
typedef struct {
	union {
		uint8_t trigger_ctl;
		struct {
			uint8_t sdm_cnt_trigger_en : 1;
			uint8_t sdm_cnt_lock_en : 1;
			uint8_t dac_fifo_trigger_en : 1;
			uint8_t dac_digital_trigger_en : 1;
		} t;
	};
} dac_ext_trigger_ctl_t;

/*!
 * struct adc_anc_ctl_t
 * @brief Control ADC analog channels for ANC
 */
typedef struct {
	bool is_open_anc; /* If true to open ADC analog channels, otherwise will close ADC resource for ANC */
} adc_anc_ctl_t;

/*!
 * struct dac_anc_ctl_t
 * @brief Control DAC digital and analog for ANC
 */
typedef struct {
	bool is_open_anc; /* If true to open DAC digital and analog, otherwise will close DAC resource for ANC */
} dac_anc_ctl_t;

#endif

/* The macro to define different type of audio devices */
#define AUDIO_DEV_TYPE_LINEIN (1 << 15)
#define AUDIO_DEV_TYPE_AMIC (1 << 14)
#define AUDIO_DEV_TYPE_FM (1 << 13)
#define AUDIO_DEV_TYPE_DMIC (1 << 12)

#endif /* __AUDIO_COMMON_H__ */
