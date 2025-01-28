#include <os_common_api.h>
#include <audio_system.h>
#include <buffer_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <audio_track.h>
#include <audio_record.h>
#include <ringbuff_stream.h>
#include <stream.h>
#include "mic_recorder.h"
#include <soc_dsp.h>

#define TEST_SAMPLE_RATE 16

t_mic_recorder *mic_recorder_open(char *record_buffer, int record_buffer_size)
{
	dsp_powergate_enable();
	
	t_mic_recorder *handle = mem_malloc(sizeof(t_mic_recorder));
	handle->ringbuf_audio_input = acts_ringbuf_init_ext(record_buffer, record_buffer_size);
	if (!handle->ringbuf_audio_input) {
		goto err_exit;
	}
	/* Create an audio recorder using the setting param. */
	handle->audio_record = audio_record_create(AUDIO_STREAM_MIC_IN, TEST_SAMPLE_RATE, TEST_SAMPLE_RATE,
	AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO, (void *)handle->ringbuf_audio_input);
	if (!handle->audio_record) {
		goto err_exit;
	}
	/* Get audio stream for data processing. */
	handle->audio_stream = audio_record_get_stream(handle->audio_record);
	if (!handle->audio_stream) {
		goto err_exit;
	}

	return handle;

err_exit:
	if (handle->audio_record) {
		audio_record_destory(handle->audio_record);
	}

	if (handle)
		mem_free(handle);

	return NULL;
}

int mic_recorder_close(t_mic_recorder *handle)
{	
	if (!handle || !handle->audio_record) {
		return -1;
	}

	audio_record_destory(handle->audio_record);
	handle->audio_record = NULL;

	if (handle->ringbuf_audio_input)
		acts_ringbuf_destroy_ext(handle->ringbuf_audio_input);

	return 0;
}


int mic_recorder_start(t_mic_recorder *handle)
{
	if (!handle) {
		return -1;
	}
	/* Start audio record. */
	return audio_record_start(handle->audio_record);
}

int mic_recorder_stop(t_mic_recorder *handle)
{
	if (!handle) {
		return -1;
	}
	/* Stop audio record. */
	return audio_record_stop(handle->audio_record);
}

