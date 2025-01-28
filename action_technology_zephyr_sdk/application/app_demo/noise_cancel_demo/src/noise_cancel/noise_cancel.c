#include <os_common_api.h>
#include <stdio.h>
#include <string.h>
#include <file_stream.h>
#include <stream.h>
#include <media_player.h>
#include <audio_system.h>

#include <audio_track.h>
#include <audio_record.h>
#include <ringbuff_stream.h>
#include <file_stream.h>
#include "noise_cancel.h"

t_media_processor nc_processor;

static io_stream_t open_file_stream(const char *url, int mode)
{
	io_stream_t stream = file_stream_create((void *)url);

	if (!stream)
		return NULL;

	if (stream_open(stream, mode)) {
		stream_destroy(stream);
		return NULL;
	}

	return stream;
}

static void close_stream(io_stream_t stream)
{
	if (stream) {
		stream_close(stream);
		stream_destroy(stream);
	}
}

void noise_cancel_start(const char *saved_url)
{
	t_media_processor *processor = &nc_processor;
	io_stream_t enc_out_stream = NULL;
	media_init_param_t init_param;
	media_player_t *player = NULL;

	memset(processor, 0, sizeof(t_media_processor));
	memset(&init_param, 0, sizeof(init_param));

	enc_out_stream = open_file_stream(saved_url, MODE_OUT);
	if (!enc_out_stream) {
		SYS_LOG_ERR("stream open failed (%s)\n", saved_url);
		goto err_exit;
	}

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_AI;
	init_param.capture_format = OPUS_TYPE;
	init_param.capture_sample_rate_input = 16;
	init_param.capture_sample_rate_output = 16;
	init_param.capture_sample_bits = 16;
	init_param.capture_channels_input = 1;
	init_param.capture_channels_output = 1;
	init_param.capture_bit_rate = 16;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = enc_out_stream;
	player = media_player_open(&init_param);
	if (!player) {
		SYS_LOG_ERR("media_player_open failed\n");
		goto err_exit;
	}

	media_player_play(player);
	processor->player = player;
	processor->enc_out_stream = enc_out_stream;
	return;

err_exit:
	if (enc_out_stream)
		close_stream(enc_out_stream);
}

void noise_cancel_mp3_start(const char *saved_url)
{
	t_media_processor *processor = &nc_processor;
	io_stream_t enc_out_stream = NULL;
	media_init_param_t init_param;
	media_player_t *player = NULL;

	memset(processor, 0, sizeof(t_media_processor));
	memset(&init_param, 0, sizeof(init_param));

	enc_out_stream = open_file_stream(saved_url, MODE_OUT);
	if (!enc_out_stream) {
		SYS_LOG_ERR("stream open failed (%s)\n", saved_url);
		goto err_exit;
	}

	init_param.type = MEDIA_SRV_TYPE_CAPTURE;
	init_param.stream_type = AUDIO_STREAM_AI;
	init_param.capture_format = MP3_TYPE;
	init_param.capture_sample_rate_input = 16;
	init_param.capture_sample_rate_output = 16;
	init_param.capture_sample_bits = 16;
	init_param.capture_channels_input = 2;
	init_param.capture_channels_output = 2;
	init_param.capture_bit_rate = 32;
	init_param.capture_complexity = 9; //use complexity as ffv rec nr level param in mp3 recording
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = enc_out_stream;
	init_param.auto_mute_threshold = 0x3f; //use as vad level param, value in 1-128, bigger the value, more sensitive the capture. 0 for not using vad
	player = media_player_open(&init_param);
	if (!player) {
		SYS_LOG_ERR("media_player_open failed\n");
		goto err_exit;
	}

	media_player_play(player);
	processor->player = player;
	processor->enc_out_stream = enc_out_stream;
	return;

err_exit:
	if (enc_out_stream)
		close_stream(enc_out_stream);
}

void noise_cancel_get_vad(void)
{
	t_media_processor *processor = &nc_processor;
	uint8_t param;

	if(processor->player)
	{
		media_player_get_vad_state(processor->player, &param);
		printk("current vad state: %d\n", param);
	}
}

void noise_cancel_stop(void)
{
	t_media_processor *processor = &nc_processor;

	if (processor->player) {
		media_player_stop(processor->player);
		media_player_close(processor->player);
	}

	if (processor->enc_out_stream) {
		SYS_LOG_INF("enc out stream len %d\n", stream_get_length(processor->enc_out_stream));
		close_stream(processor->enc_out_stream);
		processor->enc_out_stream = NULL;
	}

}


