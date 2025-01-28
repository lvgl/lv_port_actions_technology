/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio_out.h
 * @brief Audio output channels common interface.
 *
 *           <b> Audio output channels </b> are used to output a uncompressed PCM data
 *           through analog(e.g. line-out) or digital physical interface(e.g. SPDIF).
 *
 *           This public audio output interface aims to define a unified structures and functions
 *           for vary output channels.
 */

#ifndef __AUDIO_OUT_H__
#define __AUDIO_OUT_H__

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <drivers/audio/audio_common.h>
#include <drivers/cfg_drv/dev_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup audio_out_apis Audio Out Device APIs
 * @ingroup driver_apis
 * @{
 */

/**
 * @brief The <b> Audio output channels </b> contains the following channels.
 *           - DAC (digital convert to analog)
 *           - I2STX
 *           - SPDIFTX
 *           - PDMTX (opthon)
 *
 *           All audio output channels follow the same interfaces to play out a PCM stream.
 *           The different operations between the output channels are the parameter
 *           structures that used to open or control an dedicated audio output channel.
 *           For example open an DAC channel need to inplement the structure #dac_setting_t,
 *           whileas the structure to open an I2STX channel is #i2stx_setting_t.
 *           Moreover, Some special control commands are dedicated for some output channel,
 *           such as #AOUT_CMD_SET_DAC_THRESHOLD only work for DAC channel.
 *
 * Example:
 *
 * @code
 *
 *    #include <drivers/audio/audio_out.h>
 *
 *    // Note that DAC with PCM buffer does not support DMA reload mode
 *    static bool is_dma_reload = false;
 *    #define SHELL_AUDIO_BUFFER_SIZE (1024)
 *    static uint8_t audio_pcm_buffer[SHELL_AUDIO_BUFFER_SIZE];
 *
 *    // Audio output DMA callback function.
 *    static int audio_play_write_data_cb(void *handle, uint32_t reason)
 *    {
 *           uint32_t len;
 *		uint8_t *buf = NULL;
 *		int ret;
 *
 *           if (is_dma_reload) {
 *                len = sizeof(audio_pcm_buffer)  / 2;
 *                if (AOUT_DMA_IRQ_HF == reason) {
 *                    buf = audio_pcm_buffer
 *                } else if (AOUT_DMA_IRQ_TC == reason) {
 *                    buf = audio_pcm_buffer + len;
 *                }
 *                // Software need to update memory data in 'buf' and its size of length is 'len';
 *           } else {
 *                buf = hdl->audio_buffer;
 *                len = sizeof(audio_pcm_buffer);
 *                ret = audio_out_write(hdl->aout_dev, hdl->aout_handle, buf, len);
 *                if (ret) {
 *                    printk("write data error:%d\n", ret);
 *                    return ret;
 *                }
 *           }
 *
 *           return 0;
 *    }
 *
 *    //1.  Get a audio output device.
 *    struct device *aout_dev = device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
 *    if (!aout_dev) {
 *        printk("failed to get audio output device\n");
 *        return -ENODEV;
 *    }
 *
 *    // 2. Open a DAC channel
 *    aout_param_t aout_param = {0};
 *    dac_setting_t dac_setting = {0};
 *    audio_reload_t reload_setting = {0};
 *    void *aout_handle;
 *
 *    aout_param.sample_rate = SAMPLE_RATE_48KHZ;
 *	aout_param.channel_type = AUDIO_CHANNEL_DAC;
 *	aout_param.outfifo_type = AOUT_FIFO_DAC0;
 *	aout_param.channel_width = CHANNEL_WIDTH_16BITS;
 *
 *    aout_param.callback = audio_play_write_data_cb;
 *    aout_param.cb_data = NULL;
 *
 *    dac_setting.volume.left_volume = -8000; // left channel volume is -8dB.
 *	dac_setting.volume.right_volume = -8000; // right channel volume is -8dB.
 *	dac_setting.channel_mode = STEREO_MODE;
 *	aout_param.dac_setting = &dac_setting;
 *
 *    if (is_dma_reload) {
 *        reload_setting.reload_addr = audio_pcm_buffer;
 *        reload_setting.reload_len = sizeof(audio_pcm_buffer);
 *        aout_param.reload_setting = &reload_setting;
 *    }
 *
 *    aout_handle = audio_out_open(aout_dev, &aout_param);
 *    if (!aout_handle) {
 *         printk("failed to open the audio out channel");
 *         return -EIO;
 *    }
 *
 *    // 3. Send extra comands to the opened DAC channel if needed.
 *    audio_out_control(aout_dev, aout_handle, AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT, NULL);
 *
 *    // 4. Initialize pcm data in audio_pcm_buffer
 *
 *    // 5. Start to play the DAC channel
 *    if (is_dma_reload) {
 *         ret = audio_out_start(aout_dev, aout_handle);
 *    } else {
 *         ret = audio_out_write(aout_dev, aout_handle,
 *                     audio_pcm_buffer, sizeof(audio_pcm_buffer);
 *    }
 *
 * @endcode
 *
 * @note For more detailed audio output channel example, please make a reference to the file:audio_driver_shell.c.
 *
 */


/**
 * @name Definition for the audio out control commands.
 * @{
 */

/** The flag to indicate that the command shall be executed according to its FIFO type */
#define AOUT_FIFO_CMD_FLAG                                    (1 << 7)

#define AOUT_CMD_GET_SAMPLERATE                               (1)
/*!< Get the channel audio sample rate by specified the audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_SAMPLERATE                               (2)
/*!< Set the channel audio sample rate by the giving audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_SAMPLERATE, audio_sr_sel_e *sr)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OPEN_PA                                      (3)
/*!< Open internal and external PA device.
 * int audio_out_control(dev, NULL, #AOUT_CMD_OPEN_PA, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_CLOSE_PA                                     (4)
/*!< Close internal and external PA device.
 * int audio_out_control(dev, NULL, #AOUT_CMD_CLOSE_PA, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_PA_CLASS_SEL                                 (5)
/*!< Select external PA type such as class AB or class D.
 * int audio_out_control(dev, NULL, #AOUT_CMD_PA_CLASS_SEL, audio_ext_pa_class_e *class)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OUT_MUTE                                     (6)
/*!< Control output channel mute
  * int audio_out_control(dev, handle, #AOUT_CMD_OUT_MUTE, uint8_t *mute_en)
  * If *mute_en is 1 will mute both the audio left and right channels, otherwise will unmute.
  * Returns 0 if successful and negative errno code if error.
  *
  * @note Only DAC support this command.
  */

#define AOUT_CMD_GET_SAMPLE_CNT                               (AOUT_FIFO_CMD_FLAG | 7)
/*!< Get the sample counter from the audio output channel if enabled.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_SAMPLE_CNT, uint32_t *count)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note  User need to handle the overflow case.
 */

#define AOUT_CMD_RESET_SAMPLE_CNT                             (AOUT_FIFO_CMD_FLAG | 8)
/*!< Reset the sample counter function which can retrieve the initial sample counter.
 * int audio_out_control(dev, handle, #AOUT_CMD_RESET_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_ENABLE_SAMPLE_CNT                            (AOUT_FIFO_CMD_FLAG | 9)
/*!< Enable the sample counter function to the specified audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_ENABLE_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DISABLE_SAMPLE_CNT                           (AOUT_FIFO_CMD_FLAG | 10)
/*!< Disable the sample counter function by giving the audio channel handler.
 * int audio_out_control(dev, handle, #AOUT_CMD_DISABLE_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_VOLUME                                   (AOUT_FIFO_CMD_FLAG | 11)
/* Get the volume value of the audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_LEFT_VOLUME, volume_setting_t *volume)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_VOLUME                                   (AOUT_FIFO_CMD_FLAG | 12)
/*!< Set the volume value to the audio channel.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_LEFT_VOLUME, volume_setting_t *volume)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_FIFO_LEN                                 (AOUT_FIFO_CMD_FLAG | 13)
/*!< Get the total length of audio channel FIFO.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_STATUS, uint32_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_FIFO_AVAILABLE_LEN                       (AOUT_FIFO_CMD_FLAG | 14)
/*!< Get the avaliable length of audio channel FIFO that can be filled
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_STATUS, uint32_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_CHANNEL_STATUS                           (AOUT_FIFO_CMD_FLAG | 15)
/*!< Get the audio output channel status
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_CHANNEL_STATUS, u8 *status)
 * The output 'status' can refer to #AUDIO_CHANNEL_STATUS_BUSY or #AUDIO_CHANNEL_STATUS_ERROR
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_APS                                      (AOUT_FIFO_CMD_FLAG | 16)
/*!< Get the AUDIO_PLL APS
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_APS, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_APS                                      (AOUT_FIFO_CMD_FLAG | 17)
/*!< Set the AUDIO_PLL APS for the sample rate tuning
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_VOLUME, audio_aps_level_e *aps)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SPDIF_SET_CHANNEL_STATUS                     (18)
/*!< Set the SPDIFTX channel status
 * int audio_out_control(dev, handle, #AOUT_CMD_SPDIF_SET_CHANNEL_STATUS, audio_spdif_ch_status_t *status)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SPDIF_GET_CHANNEL_STATUS                     (19)
/*!< Get the SPDIFTX channel status
 * int audio_out_control(dev, handle, #AOUT_CMD_SPDIF_GET_CHANNEL_STATUS, audio_spdif_ch_status_t *status)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_OPEN_I2STX_DEVICE                            (20)
/*!< Open I2STX device and will enable the MCLK/BCLK/LRCLK clock signals.
 * int audio_out_control(dev, NULL, #AOUT_CMD_OPEN_I2STX_CLK, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_CLOSE_I2STX_DEVICE                           (21)
/*!< Close I2STX device and will disable the MCLK/BCLK/LRCLK clock signals .
 * int audio_out_control(dev, NULL, #AOUT_CMD_CLOSE_I2STX_CLK, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_DAC_THRESHOLD                            (AOUT_FIFO_CMD_FLAG | 22)
/*!< Set the DAC threshold to control the stream buffer level at different scenes.
 * int audio_out_control(dev, NULL, #AOUT_CMD_SET_DAC_THRESHOLD, dac_threshold_setting_t *thres)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note Only DAC with hardware PCM buffer support this command.
 */

#define AOUT_CMD_GET_DAC_FIFO_DRQ_LEVEL                       (AOUT_FIFO_CMD_FLAG | 23)
/*!< Get the DAC FIFO DRQ level.
 * int audio_out_control(dev, NULL, #AOUT_CMD_GET_DAC_FIFO_DRQ_LEVEL, uint8_t *level)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_DAC_FIFO_DRQ_LEVEL                       (AOUT_FIFO_CMD_FLAG | 24)
/*!< Set the DAC FIFO DRQ level.
 * int audio_out_control(dev, NULL, #AOUT_CMD_SET_DAC_FIFO_DRQ_LEVEL, uint8_t *level)
 * #level is range for 0 to 15;
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_DAC_FIFO_VOLUME                          (AOUT_FIFO_CMD_FLAG | 25)
/*!< Get the DAC FIFO volume.
 * int audio_out_control(dev, NULL, #AOUT_CMD_GET_DAC_FIFO_VOLUME, uint8_t *vol)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_DAC_FIFO_VOLUME                          (AOUT_FIFO_CMD_FLAG | 26)
/*!< Set the DAC FIFO volume.
 * int audio_out_control(dev, NULL, #AOUT_CMD_SET_DAC_FIFO_VOLUME, uint8_t *vol)
 * #vol is range for 0 to 15;
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DEBUG_PERFORMANCE_CTL                        (27)
/*!< Control to enable or disable to dump the perfornamce infomation for debug.
 * int audio_out_control(dev, NULL, #AOUT_CMD_DEBUG_PERFORMANCE_CTL, uint8_t *en)
 * #en: 0 to disable; 1 to enable
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DEBUG_PERFORMANCE_CTL_ALL                    (28)
/*!< Control all sessions to enable or disable to dump the perfornamce infomation for debug.
 * int audio_out_control(dev, NULL, #AOUT_CMD_DEBUG_PERFORMANCE_CTL_ALL, uint8_t *en)
 * #en: 0 to disable; 1 to enable
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DEBUG_DUMP_LENGTH                            (29)
/*!< Set the length of play buffer to print out per-second.
 * int audio_out_control(dev, NULL, #AOUT_CMD_DEBUG_DUMP_LENGTH, uint8_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DEBUG_DUMP_LENGTH_ALL                        (30)
/*!< Set the length of all sessions play buffer to print out per-second.
 * int audio_out_control(dev, NULL, #AOUT_CMD_DEBUG_DUMP_LENGTH_ALL, uint8_t *len)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT                       (AOUT_FIFO_CMD_FLAG | 31)
/*!< Get the SDM sample counter from the DAC module if enabled.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT, uint32_t *count)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note  The MAX SDM counter is #AOUT_SDM_CNT_MAX and user need to handle the overflow case.
 */

#define AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT                     (AOUT_FIFO_CMD_FLAG | 32)
/*!< Reset(disable and then enable) the SDM sample counter function which can retrieve the initial sample counter.
 * int audio_out_control(dev, handle, #AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_ENABLE_DAC_SDM_SAMPLE_CNT                    (AOUT_FIFO_CMD_FLAG | 33)
/*!< Enable the SDM sample counter function within DAC module.
 * int audio_out_control(dev, handle, #AOUT_CMD_ENABLE_DAC_SDM_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DISABLE_DAC_SDM_SAMPLE_CNT                   (AOUT_FIFO_CMD_FLAG | 34)
/*!< Disable the SDM sample counter function within DAC module.
 * int audio_out_control(dev, handle, #AOUT_CMD_DISABLE_DAC_SDM_SAMPLE_CNT, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT                (AOUT_FIFO_CMD_FLAG | 35)
/*!< Get the STABLE SDM sample counter from the DAC module if enabled.
 * int audio_out_control(dev, handle, #AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT, uint32_t *count)
 * Returns 0 if successful and negative errno code if error.
 *
 * @note  User need to handle the overflow case.
 */

#define AOUT_CMD_SET_DAC_TRIGGER_SRC                          (36)
/*!< Set the source of trigger DAC to start by external IRQ signal.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_DAC_TRIGGER_SRC, uint8_t *trigger_src)
 * The parameter trigger_src can refer to #audio_trigger_src.
 *
 * @note User need to send the command #AOUT_CMD_DAC_TRIGGER_CONTROL to control the DAC trigger mehod.
 *           And send the command #AOUT_CMD_DAC_FORCE_START to start by force after external trigger has been set.
 *
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SELECT_DAC_ENABLE_CHANNEL                    (37)
/*!< Select the DAC LR channels that can be enabled.
 * int audio_out_control(dev, handle, #AOUT_CMD_SELECT_DAC_ENABLE_CHANNEL, uint8_t *lr_sel)
 * For the definition of #lr_sel can refer to #a_lr_chl_e.
 *
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DAC_FORCE_START                              (38)
/*!< Force DAC to start when the external trigger source (set from command #AOUT_CMD_SET_DAC_TRIGGER_SRC and #AOUT_CMD_DAC_TRIGGER_CONTROL)
 * does not trigger.
 * int audio_out_control(dev, handle, #AOUT_CMD_DAC_FORCE_START, dac_ext_trigger_ctl_t *trigger_ctl)
 *
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_EXTERNAL_PA_CONTROL                          (39)
/*!< Control external PA such as enable/disable/mute.
 * int audio_out_control(dev, handle, #AOUT_CMD_EXTERNAL_PA_CONTROL,  audio_ext_pa_ctrl_e *ctrl_func)
 *
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_DAC_TRIGGER_CONTROL                          (40)
/*!< Control the DAC function such as SDM_LOCK/DACFIFO_EN that can triggered by external signals.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_DAC_TRIGGER_SRC, dac_ext_trigger_ctl_t *trigger_ctl)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_SET_SEPARATED_MODE                         (41)
/*!< Set DMA as separated mode which can transfrom a PCM stream in mono format to a stereo stream.
 * int audio_out_control(dev, handle, #AOUT_CMD_SET_SEPARATED_MODE, NULL)
 * Returns 0 if successful and negative errno code if error.
 */

#define AOUT_CMD_ANC_CONTROL                                  (42)
/*!< Control to enable or disable DAC hardware resource for ANC module.
 * int audio_out_control(dev, NULL, #AOUT_CMD_ANC_CONTROL, dac_anc_ctl_t *anc_ctl)
 * Returns 0 if successful and negative errno code if error.
 */
/** @} */

/*!
 * struct volume_setting_t
 * @brief The structure to configure the audio left/right channel volume.
 */
typedef struct {
#define AOUT_VOLUME_INVALID             (0xFFFFFFFF)
	/*!< macro to the invalid volume value */
	int32_t left_volume;
		/*!< specifies the volume of left channel which range from -71625(-71.625db) to 24000(24.000db) and if set #AOUT_VOLUME_INVALID to ignore this setting */
	int32_t right_volume;
		/*!< specifies the volume of right channel which range from range from -71625(-71.625db) to 24000(24.000db)  and if set #AOUT_VOLUME_INVALID to ignore this setting */
} volume_setting_t;

/*!
 * struct dac_threshold_setting_t
 * @brief The setting of the DAC PCMBUF threshold.
 */
typedef struct {
	uint32_t he_thres;
		/*!< The half empty threshold */
	uint32_t hf_thres;
		/*!< The half full threshold */
} dac_threshold_setting_t;

/*!
 * struct dac_setting_t
 * @brief The DAC setting parameters.
 */
typedef struct {
	audio_ch_mode_e channel_mode;
		/*!< Select the channel mode such as mono or strereo */
	volume_setting_t volume;
		/*!< The left and right volume setting */
} dac_setting_t;

/*!
 * struct i2stx_setting_t
 * @brief The I2STX setting parameters.
 */
typedef struct {
#define I2STX_SRD_FS_CHANGE  (1 << 0)
/*!< I2STX SRD(sample rate detect) captures the event that the sample rate has changed.
 * int callback(cb_data, #I2STX_SRD_FS_CHANGE, audio_sr_sel_e *sr)
 */
#define I2STX_SRD_WL_CHANGE  (1 << 1)
/*!< I2STX SRD(sample rate detect) captures the event that the effective width length has changed.
 * int callback(cb_data, #I2STX_SRD_WL_CHANGE, audio_i2s_srd_wl_e *wl)
 */
#define I2STX_SRD_TIMEOUT    (1 << 2)
/*!< I2STX SRD(sample rate detect) captures the timeout (disconnection) event.
 * int callback(cb_data, #I2STX_SRD_TIMEOUT, NULL)
 */
	int (*srd_callback)(void *cb_data, uint32_t cmd, void *param);
		/*!< The callback function from I2STX SRD(sample rate detect) module which worked in the slave mode */
	void *cb_data;
		/*!< Callback user data */
} i2stx_setting_t;

/*!
 * struct spdiftx_setting_t
 * @brief The SPDIFTX setting parameters
 * @note The FIFOs used by SPDIFTX can be #AOUT_FIFO_DAC0 or #AOUT_FIFO_I2STX0.
 * If select the #AOUT_FIFO_DAC0, the clock source will use the division 2 of the DAC clock source.
 * If select the #AOUT_FIFO_I2STX0, there is configurations (CONFIG_SPDIFTX_USE_I2STX_MCLK/CONFIG_SPDIFTX_USE_I2STX_MCLK_DIV2) to control the clock source.
 */
typedef struct {
	audio_spdif_ch_status_t *status; /*!< The channel status setting if has. If setting NULL, low level driver will use a default channel status value*/
} spdiftx_setting_t;

/*!
 * struct aout_param_t
 * @brief The audio out configuration parameters
 */
typedef struct {
#define AOUT_DMA_IRQ_HF         (1 << 0)     /*!< DMA irq half full flag */
#define AOUT_DMA_IRQ_TC         (1 << 1)     /*!< DMA irq transfer completly flag */
	uint8_t sample_rate;
		/*!< The sample rate setting and can refer to enum audio_sr_sel_e */
	uint16_t channel_type;
		/*!< Indicates the channel type selection and can refer to #AUDIO_CHANNEL_DAC, #AUDIO_CHANNEL_I2STX, #AUDIO_CHANNEL_SPDIFTX*/
	audio_ch_width_e channel_width;
		/*!< The channel effective data width */
	audio_outfifo_sel_e outfifo_type;
		/*!< Indicates the used output fifo type */
	dac_setting_t *dac_setting;
		/*!< The DAC function setting if has */
	i2stx_setting_t *i2stx_setting;
		/*!< The I2STX function setting if has */
	spdiftx_setting_t *spdiftx_setting;
		/*!< The SPDIFTX function setting if has */
	int (*callback)(void *cb_data, uint32_t reason);
		/*!< The callback function which conrespondingly with the events such as #AOUT_PCMBUF_IP_HE or #AOUT_PCMBUF_IP_HF etc.*/
	void *cb_data;
		/*!< Callback user data */
	audio_reload_t *reload_setting;
		/*!< The reload mode setting and if don't use this mode, please let 'reload_setting = NULL' */
} aout_param_t;

/*!
 * struct aout_driver_api
 * @brief The sturcture to define audio out driver API.
 */
struct aout_driver_api {
	void* (*aout_open)(struct device *dev, aout_param_t *param);
	int (*aout_close)(struct device *dev, void *handle);
	int (*aout_start)(struct device *dev, void *handle);
	int (*aout_write)(struct device *dev, void *handle, uint8_t *buffer, uint32_t length);
	int (*aout_stop)(struct device *dev, void *handle);
	int (*aout_control)(struct device *dev, void *handle, int cmd, void *param);
};

/*!
 * @brief Open the audio output channel by specified parameters.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param setting Pointer to the audio output channel parameter.
 *
 * @return The audio output channel instance handle.
 */
static inline void* audio_out_open(struct device *dev, aout_param_t *setting)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_open(dev, setting);
}

/*!
 * @brief Close the audio output channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param handle The audio output channel instance handle.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_out_open.
 */
static inline int audio_out_close(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_close(dev, handle);
}

/*!
 * @brief Control the audio output channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param handle The audio output channel instance handle.
 *
 * @param cmd The control command that sent to the audio output channel.
 *
 * @param param The audio out in/out parameters which corresponding with the commands
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_out_open.
 */
static inline int audio_out_control(struct device *dev, void *handle, int cmd, void *param)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_control(dev, handle, cmd, param);
}

/*!
 * @brief Start the audio output channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param handle The audio output channel instance handle.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_out_open.
 */
static inline int audio_out_start(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_start(dev, handle);
}

/*!
 * @brief Write data into the audio output channel.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param handle The audio output channel instance handle.
 *
 * @param buffer The stream buffer to output.
 *
 * @param length The length of the stream buffer.
 *
 * @return 0 on success, negative errno code on fail.
 *
 * @note the handle shall be the same as the retval of #audio_out_open.
 *
 * @note There are 2 mode to transfer the output data.
 * One is reload mode which is using the same buffer and length and call #acts_aout_start one time,
 * the other is direct mode which use different buffer/length and call #acts_aout_start separatly.
 */
static inline int audio_out_write(struct device *dev, void *handle, uint8_t *buffer, uint32_t length)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_write(dev, handle, buffer, length);
}

/*!
 * @brief Stop the audio output channel by the specified handle.
 *
 * @param dev Pointer to the device structure for the audio output channel instance.
 *
 * @param handle The audio output channel instance handle.
 *
 * @return 0 on success, negative errno code on fail
 */
static inline int audio_out_stop(struct device *dev, void *handle)
{
	const struct aout_driver_api *api = dev->api;

	return api->aout_stop(dev, handle);
}

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup audio_out_apis
 */

#endif /* __AUDIO_OUT_H__ */
