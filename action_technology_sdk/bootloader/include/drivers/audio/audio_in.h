/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio_in.h
 * @brief Audio input channels common interface.
 *
 *           <b> Audio input channels </b> are used to record a uncompressed PCM data
 *           through analog(e.g line-in) or digital physical interface(e.g. SPDIF).
 *
 *           This public audio input interface aims to define a unified structures and functions
 *           for vary audio input channels.
 */

#ifndef __AUDIO_IN_H__
#define __AUDIO_IN_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <board.h>
#include <drivers/audio/audio_common.h>
#include <drivers/cfg_drv/dev_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup audio_in_apis Audio In Device APIs
 * @ingroup driver_apis
 * @{
 */

/**
 * @brief The <b> Audio input channels </b> contains the following channels.
 *           - ADC (analog convert to digital)
 *           - I2SRX
 *           - SPDIFRX
 *
 *           All audio input channels follow the same interfaces to record a PCM stream.
 *           The different operations between the input channels are the parameter
 *           structures that used to open or control an dedicated audio input channel.
 *           For example open an ADC channel need to inplement the structure #adc_setting_t,
 *           whileas the structure to open an I2SRX channel is #i2srx_setting_t.
 *           Moreover, Some special control commands are dedicated for some input channel,
 *           such as #AIN_CMD_SPDIF_IS_PCM_STREAM only work for SPDIFRX channel.
 *
 * Example:
 *
 * @code
 *
 *    #include <drivers/audio/audio_in.h>
 *
 *    // Note that all audio input channels only support DMA reload mode.
 *
 *    #define SHELL_AUDIO_BUFFER_SIZE (1024)
 *    static uint8_t audio_pcm_buffer[SHELL_AUDIO_BUFFER_SIZE];
 *
 *   // Audio input DMA callback function.
 *    static int audio_rec_read_data_cb(void *callback_data, uint32_t reason) {
 *        uint32_t len = sizeof(audio_pcm_buffer) / 2;
 *        uint8_t *buf = NULL;
 *
 *        if (AIN_DMA_IRQ_HF == reason) {
 *            buf = audio_pcm_buffer;
 *        } else  {
 *            buf = audio_pcm_buffer + len;
 *        }
 *
 *        // The recorded data is in the memory of buf, and its length is len.
 *    }
 *
 *    //1.  Get a audio input device.
 *    struct device *ain_dev = device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
 *    if (!ain_dev) {
 *        printk("failed to get audio input device\n");
 *        return -ENODEV;
 *    }
 *
 *    // 2. Open a ADC channel
 *    ain_param_t ain_param = {0};
 *    adc_setting_t adc_setting = {0};
 *    void *ain_handle;
 *
 *    ain_param.sample_rate = SAMPLE_RATE_16KHZ;
 *	ain_param.callback = audio_rec_read_data_cb;
 *	ain_param.cb_data = NULL;
 *	ain_param.reload_setting.reload_addr = audio_pcm_buffer;
 *	ain_param.reload_setting.reload_len = sizeof(audio_pcm_buffer);
 *	ain_param.channel_type = AUDIO_CHANNEL_ADC;
 *	ain_param.channel_width = CHANNEL_WIDTH_16BITS;
 *
 *    adc_setting.device = AUDIO_ANALOG_MIC0;
 *    uint8_t i;
 *    for (i = 0; i < ADC_CH_NUM_MAX; i++) {
 *        adc_setting.gain.ch_gain[i] = 365;
 *    }
 *    ain_param.adc_setting = &adc_setting;
 *
 *    ain_handle = audio_in_open(ain_dev, &ain_param);
 *    if (!ain_handle) {
 *         printk("failed to open the audio in channel");
 *         return -EIO;
 *    }
 *
 *    // 3. Use audio_out_control to send extra comands to the opened ADC channel if needed.
 *
 *    // 4. Start to play the ADC channel
 *    ret = audio_record_start(ain_dev, ain_handle);
 *
 * @endcode
 *
 * @note For more detailed audio input channel example, please make a reference to the file:audio_driver_shell.c.
 *
 */

/**
 * @name Definition for the audio in control commands
 * @{
 */
#define AIN_CMD_SET_ADC_GAIN                                  (1)
/*!< Set the ADC left channel gain value which in dB format.
 * int audio_in_control(dev, handle, #AIN_CMD_SET_ADC_GAIN, adc_gain *gain)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note You can get the dB range from #AIN_CMD_GET_ADC_LEFT_GAIN_RANGE and AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE.
 */

#define AIN_CMD_GET_ADC_LEFT_GAIN_RANGE                       (2)
/*!< Get the ADC left channel gain range in dB format.
 * int audio_in_control(dev, handle, #AIN_CMD_GET_ADC_LEFT_GAIN_RANGE, adc_gain_range *range)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE                      (3)
/*!< Get the ADC right channel gain range in dB format.
 * int audio_in_control(dev, handle, #AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE, adc_gain_range *range)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_GET_CHANNEL_STATUS                      (4)
/* Get the SPDIFRX channel status.
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_GET_CHANNEL_STATUS, audio_spdif_ch_status_t *sts)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_IS_PCM_STREAM                           (5)
/* Check if the stream that received from spdifrx is the pcm format
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_IS_PCM_STREAM, bool *is_pcm)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SPDIF_CHECK_DECODE_ERR                        (6)
/* Check if there is spdif decode error happened
 * int audio_in_control(dev, handle, #AIN_CMD_SPDIF_CHECK_DECODE_ERR, bool *is_err)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_I2SRX_QUERY_SAMPLE_RATE                       (7)
/* Query the i2c master device sample rate and i2srx works in slave mode.
 * int audio_in_control(dev, handle, #AIN_CMD_I2SRX_QUERY_SAMPLE_RATE, audio_sr_sel_e *is_err)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_SAMPLERATE                                (8)
/*!< Get the channel audio sample rate by specified the audio channel handler.
 * int audio_in_control(dev, handle, #AIN_CMD_GET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_SAMPLERATE                                (9)
/*!< Set the channel audio sample rate by the giving audio channel handler.
 * int audio_in_control(dev, handle, #AIN_CMD_SET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_APS                                       (10)
/*!< Get the AUDIO_PLL APS
 * int audio_in_control(dev, handle, #AIN_CMD_GET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_APS                                       (11)
/*!< Set the AUDIO_PLL APS for the sample rate tuning
 * int audio_in_control(dev, handle, #AIN_CMD_SET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_BIND_CHANNEL                                  (12)
/*!< Bind the different audio in channels that can start simutaneously.
 * int audio_in_control(dev, handle, #AIN_CMD_BIND_CHANNEL, void *hdl)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_GET_ADC_FIFO_DRQ_LEVEL                        (13)
/*!< Get the ADC FIFO DRQ level.
 * int audio_in_control(dev, NULL, #AIN_CMD_GET_ADC_FIFO_DRQ_LEVEL, uint8_t *level)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_ADC_FIFO_DRQ_LEVEL                        (14)
/*!< Set the ADC FIFO DRQ level.
 * int audio_in_control(dev, NULL, #AIN_CMD_SET_ADC_FIFO_DRQ_LEVEL, uint8_t *level)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note The parameter level is range from 0 to 15;
 */

#define AIN_CMD_DEBUG_PERFORMANCE_CTL                         (15)
/*!< Control to enable or disable to dump the perfornamce infomation for debug.
 * int audio_in_control(dev, NULL, #AOUT_CMD_DEBUG_PERFORMANCE_CTL, uint8_t *en)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note en: 0 to disable; 1 to enable
 */

#define AIN_CMD_DEBUG_PERFORMANCE_CTL_ALL                     (16)
/*!< Control all sessions to enable or disable to dump the perfornamce infomation for debug.
 * int audio_in_control(dev, NULL, #AIN_CMD_DEBUG_PERFORMANCE_CTL_ALL, uint8_t *en)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note en: 0 to disable; 1 to enable
 */

#define AIN_CMD_DEBUG_DUMP_LENGTH                             (17)
/*!< Set the length of play buffer to print out per-second.
 * int audio_in_control(dev, NULL, #AIN_CMD_DEBUG_DUMP_LENGTH, uint8_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_DEBUG_DUMP_LENGTH_ALL                         (18)
/*!< Set the length of all sessions play buffer to print out per-second.
 * int audio_in_control(dev, NULL, #AIN_CMD_DEBUG_DUMP_LENGTH_ALL, uint8_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_ADC_TRIGGER_SRC                           (19)
/*!< Set the source of trigger ADC to start by external IRQ signal.
 * int audio_in_control(dev, handle, #AIN_CMD_SET_ADC_TRIGGER_SRC, uint8_t *trigger_src)
 * The parameter trigger_src can refer to #audio_trigger_src.
 * Returns 0 if successful and negative errno code if error.
 *
 * @note The parameter trigger_src can refer to #audio_trigger_src.
 */

#define AIN_CMD_ANC_CONTROL                                   (20)
/*!< Control to enable or disable ANC function.
 * int audio_in_control(dev, NULL, #AIN_CMD_ANC_CONTROL, adc_anc_ctl_t *anc_ctl)
 * Returns 0 if successful and negative errno code if error.
 */

#define AIN_CMD_SET_SEPARATED_MODE                            (21)
/*!< Set DMA as separated mode for mute left/right channel individually or store the PCM data by separated format.
 * int audio_in_control(dev, handle, #AIN_CMD_SET_SEPARATED_MODE, audio_interleave_mode_e *p_mode)
 * Returns 0 if successful and negative errno code if error.
 */

/** @} */

/*!
 * enum audio_interleave_mode_e
 * @brief Audio DMA interleave mode setting
 */
typedef enum {
	LEFT_MONO_RIGHT_MUTE_MODE = 0,
	LEFT_MUTE_RIGHT_MONO_MODE,
	LEFT_RIGHT_SEPERATE
} audio_interleave_mode_e;

/*!
 * struct adc_gain_range
 * @brief The ADC min and max gain range
 */
typedef struct {
	int16_t min;
		/*!< min gain */
	int16_t max;
		/*!< max gain */
} adc_gain_range;

/*!
 * struct adc_gain
 * @brief The ADC gain setting
 */
typedef struct {
#define ADC_GAIN_INVALID (0xFFFF)
	int16_t ch_gain[ADC_CH_NUM_MAX];
		/*!< The gain value shall be set by 10 multiple of actual value e.g. ch_gain[0]=100 means channel 0 gain is 10 db;
		 * If gain value equal to #ADC_GAIN_INVALID that the setting will be ignored.
		 */
} adc_gain;

/*!
 * struct adc_setting_t
 * @brief The ADC setting parameters
 */
typedef struct {
	uint16_t device;
		/*!< ADC input device chooses */
	adc_gain gain;
		/*!< ADC gain setting */
} adc_setting_t;

/*!
 * struct i2srx_setting_t
 * @brief The I2SRX setting parameters
 */
typedef struct {
#define I2SRX_SRD_FS_CHANGE  (1 << 0)
	/*!< I2SRX SRD(sample rate detect) captures the event that the sample rate has changed
	 * int callback(cb_data, #I2STX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
	 */
#define I2SRX_SRD_WL_CHANGE  (1 << 1)
	/*!< I2SRX SRD(sample rate detect) captures the event that the effective width length has changed
	 * int callback(cb_data, #I2STX_SRD_WL_CHANGE, audio_i2s_srd_wl_e *wl)
	 */
#define I2SRX_SRD_TIMEOUT    (1 << 2)
	/*!< I2SRX SRD(sample rate detect) captures the timeout (disconnection) event
	 * int callback(cb_data, #I2STX_SRD_TIMEOUT, NULL)
	 */
	int (*srd_callback)(void *cb_data, uint32_t cmd, void *param);
		/*!< The callback function from I2SRX SRD module which worked in the slave mode */
	void *cb_data;
		/*!< Callback user data */
} i2srx_setting_t;

/*!
 * struct spdifrx_setting_t
 * @brief The SPDIFRX setting parameters
 */
typedef struct {
#define SPDIFRX_SRD_FS_CHANGE  (1 << 0)
	/*!< SPDIFRX SRD(sample rate detect) captures the event that the sample rate has changed.
	 * int callback(cb_data, #SPDIFRX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
	 */
#define SPDIFRX_SRD_TIMEOUT    (1 << 1)
	/*!< SPDIFRX SRD(sample rate detect) timeout (disconnect) event.
	 * int callback(cb_data, #SPDIFRX_SRD_TIMEOUT, NULL)
	 */
	int (*srd_callback)(void *cb_data, uint32_t cmd, void *param);
		/*!< sample rate detect callback */
	void *cb_data;
		/*!< callback user data */
} spdifrx_setting_t;

/*!
 * struct ain_param_t
 * @brief The audio in configuration parameters
 */
typedef struct {
#define AIN_DMA_IRQ_HF (1 << 0)
	/*!< DMA irq half full flag */
#define AIN_DMA_IRQ_TC (1 << 1)
	/*!< DMA irq transfer completly flag */
	uint8_t sample_rate;
		/*!< The sample rate setting refer to enum audio_sr_sel_e */
	uint16_t channel_type;
		/*!< Indicates the channel type selection and can refer to #AUDIO_CHANNEL_ADC, #AUDIO_CHANNEL_I2SRX, #AUDIO_CHANNEL_SPDIFRX*/
	audio_ch_width_e channel_width;
		/*!< The channel effective data width */
	adc_setting_t *adc_setting;
		/*!< The ADC function setting if has */
	i2srx_setting_t *i2srx_setting;
		/*!< The I2SRX function setting if has */
	spdifrx_setting_t *spdifrx_setting;
		/*!< The SPDIFRX function setting if has  */
	int (*callback)(void *cb_data, uint32_t reason);
		/*!< The callback function which called when #AIN_DMA_IRQ_HF or #AIN_DMA_IRQ_TC events happened */
	void *cb_data;
		/*!< callback user data */
	audio_reload_t reload_setting;
		/*!< The reload mode setting which is mandatory*/
} ain_param_t;

/*!
 * struct ain_driver_api
 * @brief Public API for audio in driver
 */
struct ain_driver_api {
	void* (*ain_open)(struct device *dev, ain_param_t *param);
	int (*ain_close)(struct device *dev, void *handle);
	int (*ain_start)(struct device *dev, void *handle);
	int (*ain_stop)(struct device *dev, void *handle);
	int (*ain_control)(struct device *dev, void *handle, int cmd, void *param);
};

/*!
 * @brief Open the audio input channel by specified parameters
 *
 * @param dev Pointer to the device structure for the audio input channel instance.
 *
 * @param setting Pointer to the audio input channel parameter.
 *
 * @return The audio input channel instance handle.
 */
static inline void* audio_in_open(struct device *dev, ain_param_t *setting)
{
	const struct ain_driver_api *api = dev->api;

	return api->ain_open(dev, setting);
}

/*!
 * @brief Close the audio input channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio input channel instance.
 *
 * @param handle The audio input channel instance handle.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_in_open.
 */
static inline int audio_in_close(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->api;

	return api->ain_close(dev, handle);
}

/*!
 * @brief Control the audio input channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio input channel instance.
 *
 * @param handle The audio input channel instance handle.
 *
 * @param cmd The control command that sent to the audio input channel.
 *
 * @param param The audio out in/out parameters which corresponding with the commands
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_in_open.
 */
static inline int audio_in_control(struct device *dev, void *handle, int cmd, void *param)
{
	const struct ain_driver_api *api = dev->api;

	return api->ain_control(dev, handle, cmd, param);
}

/*!
 * @brief Start the audio input channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio input channel instance.
 *
 * @param handle The audio input channel instance handle.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_in_open.
 */
static inline int audio_in_start(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->api;

	return api->ain_start(dev, handle);
}

/*!
 * @brief Stop the audio input channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio input channel instance.
 *
 * @param handle The audio input channel instance handle.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_in_open.
 */
static inline int audio_in_stop(struct device *dev, void *handle)
{
	const struct ain_driver_api *api = dev->api;

	return api->ain_stop(dev, handle);
}

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup audio_in_apis
 */

#endif /* __AUDIO_IN_H__ */
