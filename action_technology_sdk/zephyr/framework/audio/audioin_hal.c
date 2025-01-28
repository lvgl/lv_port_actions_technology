/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio In HAL
 */

#ifndef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "hal_ain"
#endif
#include <assert.h>
#include <audio_hal.h>

srd_callback spdifrx_srd_callback = NULL;
srd_callback i2srx_srd_callback = NULL;

hal_audio_in_context_t  hal_audio_in_context;

static inline hal_audio_in_context_t* _hal_audio_in_get_context(void)
{
	return &hal_audio_in_context;
}

int hal_audio_in_init(void)
{
	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	audio_in->ain_dev = (struct device *)device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);

	if (!audio_in->ain_dev) {
		SYS_LOG_ERR("device not found\n");
		return -ENODEV;
	}
	SYS_LOG_INF("success \n");
	return 0;
}

void* hal_ain_channel_open(audio_in_init_param_t *init_param)
{

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();
	ain_param_t ain_param = {0};
	adc_setting_t adc_setting;
#ifdef CONFIG_AUDIO_OUT_I2SRX_SUPPORT
	i2srx_setting_t i2srx_setting = {0};
#endif
#ifdef CONFIG_AUDIO_OUT_SPDIFRX_SUPPORT
	spdifrx_setting_t spdifrx_setting = {0};
#endif
	memset(&ain_param, 0, sizeof(ain_param_t));

	ain_param.sample_rate = init_param->sample_rate;
	ain_param.callback = init_param->callback;
	ain_param.cb_data = init_param->callback_data;
	ain_param.reload_setting.reload_addr = init_param->reload_addr;
	ain_param.reload_setting.reload_len = init_param->reload_len;
	ain_param.channel_type = init_param->channel_type;

	switch (init_param->channel_type) {
		case AUDIO_CHANNEL_ADC:
		{
			uint8_t i;
			adc_setting.device = init_param->audio_device;
			for (i = 0; i < ADC_CH_NUM_MAX; i++)
				adc_setting.gain.ch_gain[i] = init_param->input_gain;
			ain_param.adc_setting = &adc_setting;
			break;
		}
#ifdef CONFIG_AUDIO_OUT_I2SRX_SUPPORT
		case AUDIO_CHANNEL_I2SRX:
		{
			i2srx_setting.mode = I2S_SLAVE_MODE;
			i2srx_setting.srd_callback = i2srx_srd_callback;
			ain_param.i2srx_setting = &i2srx_setting;
			break;
		}
#endif
#ifdef CONFIG_AUDIO_OUT_SPDIFRX_SUPPORT
		case AUDIO_CHANNEL_SPDIFRX:
		{
			spdifrx_setting.srd_callback = spdifrx_srd_callback;
			ain_param.spdifrx_setting = &spdifrx_setting;
			break;
		}
#endif
		default:
		{
			SYS_LOG_ERR("not support: %d", init_param->channel_type);
			return NULL;
		}
	}

    return audio_in_open((struct device *)audio_in->ain_dev, &ain_param);

}

int hal_ain_channel_start(void* ain_channel_handle)
{

	int result;

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

	result = audio_in_start((struct device *)audio_in->ain_dev, ain_channel_handle);

	return result;

}

int hal_ain_channel_read_data(void* ain_channel_handle, u8_t *data, u32_t data_size)
{
	int result = 0;
#if 0
	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);
#endif
	return result;
}

int hal_ain_channel_stop(void* ain_channel_handle)
{

	int result;

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

	result = audio_in_stop(audio_in->ain_dev, ain_channel_handle);

	return result;

}


int hal_ain_channel_close(void* ain_channel_handle)
{

	int result;

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

	result = audio_in_close(audio_in->ain_dev, ain_channel_handle);

	return result;
}

int hal_ain_channel_set_aps(void *ain_channel_handle, unsigned int aps_level, unsigned int aps_mode)
{

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

    return audio_in_control(audio_in->ain_dev, ain_channel_handle, AIN_CMD_SET_APS, &aps_level);
}

int hal_ain_channel_set_volume(void* ain_channel_handle, adc_gain *adc_volume)
{
	int result;

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

	result = audio_in_control(audio_in->ain_dev, ain_channel_handle, AIN_CMD_SET_ADC_GAIN, adc_volume);

	return 0;
}

int hal_ain_set_contrl_callback(u8_t channel_type, srd_callback callback)
{

	if (channel_type == AUDIO_CHANNEL_SPDIFRX) {
		spdifrx_srd_callback = callback;
	} else if (channel_type == AUDIO_CHANNEL_I2SRX) {
		i2srx_srd_callback = callback;
	} else {
		SYS_LOG_ERR("not support channel type: %d", channel_type);
	}

	return 0;
}

int hal_ain_set_aec_record_back(void* ain_channel_handle, uint8_t enable)
{

	int result;

	hal_audio_in_context_t*  audio_in = _hal_audio_in_get_context();

	assert(audio_in->ain_dev);

	result = audio_in_control(audio_in->ain_dev, ain_channel_handle, AIN_CMD_AEC_CONTROL, &enable);

	return result;
}


