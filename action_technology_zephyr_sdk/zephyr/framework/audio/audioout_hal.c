/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio Out HAL
 */

#ifndef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "hal_aout"
#endif
#include <audio_hal.h>

hal_audio_out_context_t  hal_audio_out_context;

static inline hal_audio_out_context_t* _hal_audio_out_get_context(void)
{
	return &hal_audio_out_context;
}

int hal_audio_out_init(void)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	audio_out->aout_dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if (!audio_out->aout_dev) {
		SYS_LOG_ERR("device not found\n");
		return -ENODEV;
	}

	SYS_LOG_INF("success \n");

	return 0;
}

void* hal_aout_channel_open(audio_out_init_param_t *init_param)
{

	aout_param_t aout_param = {0};
	dac_setting_t dac_setting = {0};
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	i2stx_setting_t i2stx_setting = {0};
#endif
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	spdiftx_setting_t spdiftx_setting = {0};
#endif
	audio_reload_t reload_setting = {0};


	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);
	assert(init_param);

	aout_param.sample_rate = init_param->sample_rate;
	aout_param.channel_type = init_param->channel_type;
	aout_param.outfifo_type = init_param->channel_id;

	SYS_LOG_INF("sample rate %d, channel type %d, out fifo %d",
		aout_param.sample_rate, aout_param.channel_type, aout_param.outfifo_type);

	aout_param.callback = init_param->callback;
	aout_param.cb_data = init_param->callback_data;
    if(init_param->data_width == 16) {
        aout_param.channel_width = CHANNEL_WIDTH_16BITS;
    } else {
        aout_param.channel_width = CHANNEL_WIDTH_24BITS;
    }

	if (init_param->dma_reload) {
		reload_setting.reload_addr = init_param->reload_addr;
		reload_setting.reload_len = init_param->reload_len;
		aout_param.reload_setting = &reload_setting;
	}


	if (!init_param->channel_type) {
		SYS_LOG_ERR("invalid channel type %d", init_param->channel_type);
		return NULL;
	}

	if (init_param->channel_type & AUDIO_CHANNEL_DAC) {
		dac_setting.volume.left_volume = init_param->left_volume;
		dac_setting.volume.right_volume = init_param->right_volume;
		aout_param.dac_setting = &dac_setting;

		dac_setting.channel_mode = init_param->channel_mode;

	}
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
	if (init_param->channel_type & AUDIO_CHANNEL_I2STX) {
		i2stx_setting.mode = I2S_MASTER_MODE;
		if (I2S_SLAVE_MODE == i2stx_setting.mode) {
			i2stx_setting.srd_callback = NULL;
		}
		aout_param.i2stx_setting = &i2stx_setting;
		if (AOUT_FIFO_DAC0 == aout_param.outfifo_type) {
			aout_param.dac_setting = &dac_setting;
		}
	}
#endif
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
	if (init_param->channel_type & AUDIO_CHANNEL_SPDIFTX) {
		if (AOUT_FIFO_DAC0 == aout_param.outfifo_type) {
			aout_param.dac_setting = &dac_setting;
		}
		aout_param.spdiftx_setting= &spdiftx_setting;
	}
#endif
    return audio_out_open(audio_out->aout_dev, (void *)&aout_param);

}

int hal_aout_channel_start(void* aout_channel_handle)
{

	int result;

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	result = audio_out_start(audio_out->aout_dev, aout_channel_handle);

	return result;
}

int hal_aout_channel_write_data(void* aout_channel_handle, u8_t *data, u32_t data_size)
{

	int result;

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	result = audio_out_write(audio_out->aout_dev, aout_channel_handle, data, data_size);

	return result;
}

int hal_aout_channel_stop(void* aout_channel_handle)
{
	int result;

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	result = audio_out_stop(audio_out->aout_dev, aout_channel_handle);

	return result;
}

int hal_aout_channel_close(void* aout_channel_handle)
{

	int result;

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	result = audio_out_close(audio_out->aout_dev, aout_channel_handle);

	return result;
}

int hal_aout_channel_set_pa_vol_level(void* aout_channel_handle, int vol_level)
{

	volume_setting_t volume;

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	volume.left_volume = vol_level;
	volume.right_volume = vol_level;

	return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SET_VOLUME, (void *)&volume);
}

int hal_aout_channel_set_aps(void *aout_channel_handle, unsigned int aps_level, unsigned int aps_mode)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SET_APS, &aps_level);
}

int hal_aout_channel_mute_ctl(void *aout_channel_handle, u8_t mode)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_OUT_MUTE, &mode);
}

u32_t hal_aout_channel_get_sample_cnt(void *aout_channel_handle)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();
	u32_t cnt = 0;

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	if (audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_SAMPLE_CNT, (void *)&cnt)) {
		SYS_LOG_ERR("Get FIFO counter error");
	}

    return cnt;

}

int hal_aout_channel_enable_sample_cnt(void *aout_channel_handle, bool enable)
{

	int ret = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	if (enable) {
		ret = audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_ENABLE_SAMPLE_CNT, NULL);
	} else {
		ret = audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_DISABLE_SAMPLE_CNT, NULL);
	}
    return ret;
}

int hal_aout_channel_reset_sample_cnt(void *aout_channel_handle)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_RESET_SAMPLE_CNT, NULL);

}

int hal_aout_open_pa(void)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	return audio_out_control(audio_out->aout_dev, NULL, AOUT_CMD_OPEN_PA, NULL);
}

int hal_aout_close_pa(void)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	return audio_out_control(audio_out->aout_dev, NULL, AOUT_CMD_CLOSE_PA, NULL);

}

int hal_aout_channel_check_fifo_underflow(void *aout_channel_handle)
{
	int samples = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_FIFO_LEN, &samples);
}

int hal_aout_pa_class_select(u8_t pa_mode)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	return audio_out_control(audio_out->aout_dev, NULL, AOUT_CMD_PA_CLASS_SEL, &pa_mode);
}

int hal_aout_set_pcm_threshold(void *aout_channel_handle, int he_thres, int hf_thres)
{

	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();
	dac_threshold_setting_t dac_threshold = {
		.he_thres = he_thres,
		.hf_thres = hf_thres,
	};

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SET_DAC_THRESHOLD, &dac_threshold);
}

int hal_aout_set_fifo_src(void *aout_channel_handle, u8_t channel_id, bool from_dsp, void *dsp_audio_set_param)
{
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();
	dac_fifosrc_setting_t dac_fifosrc = {
		.fifo_idx            = channel_id,
		.fifo_from_dsp       = from_dsp,
		.dsp_audio_set_param = dsp_audio_set_param,
	};

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SET_FIFO_SRC, &dac_fifosrc);
}

int hal_aout_lr_channel_enable(void *aout_channel_handle, bool l_enable, bool r_enable)
{
    int ret;
    uint8_t lr_sel;
    hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

    assert(audio_out->aout_dev);
    assert(aout_channel_handle);

    lr_sel = (l_enable ? LEFT_CHANNEL_SEL : 0) | (r_enable ? RIGHT_CHANNEL_SEL : 0);
    ret = audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SELECT_DAC_ENABLE_CHANNEL, &lr_sel);
    return ret;
}

int hal_aout_channel_get_buffer_size(void *aout_channel_handle)
{
	int samples = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_FIFO_LEN, &samples);
    return samples;
}

int hal_aout_channel_get_buffer_space(void *aout_channel_handle)
{
	int samples = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_FIFO_AVAILABLE_LEN, &samples);
    return samples;
}

uint32_t hal_aout_channel_get_sdm_cnt(void *aout_channel_handle)
{
	uint32_t samples = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT, &samples);
    return samples;
}

uint32_t hal_aout_channel_get_saved_sdm_cnt(void *aout_channel_handle)
{
	uint32_t samples = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT, &samples);
    return samples;
}

int hal_aout_channel_enable_sdm_cnt(void *aout_channel_handle, bool enable)
{
	int ret = 0;
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

	if (enable) {
		ret = audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_ENABLE_DAC_SDM_SAMPLE_CNT, NULL);
	} else {
		ret = audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_DISABLE_DAC_SDM_SAMPLE_CNT, NULL);
	}
    return ret;
}

int hal_aout_channel_reset_sdm_cnt(void *aout_channel_handle)
{
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

	assert(aout_channel_handle);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT, NULL);
}

int hal_aout_trigger_src_control(void *aout_channel_handle, dac_ext_trigger_ctl_t *trigger_ctl)
{
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_DAC_TRIGGER_CONTROL, trigger_ctl);
}

int hal_aout_channel_set_dac_trigger_src(void *aout_channel_handle, audio_trigger_src src)
{
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_SET_DAC_TRIGGER_SRC, &src);
}

int hal_aout_channel_set_dac_enable(void *aout_channel_handle)
{
	hal_audio_out_context_t*  audio_out = _hal_audio_out_get_context();

	assert(audio_out->aout_dev);

    return audio_out_control(audio_out->aout_dev, aout_channel_handle, AOUT_CMD_DAC_FORCE_START, 0);
}

