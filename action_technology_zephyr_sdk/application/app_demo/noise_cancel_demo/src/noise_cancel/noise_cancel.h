#ifndef NOISE_CANCEL_H
#define NOISE_CANCEL_H

#include <stream.h>
#include <media_player.h>

#define NOISE_CANCEL_CHECK_PERIOD 1000

typedef struct media_processor {
	io_stream_t enc_in_stream;
	io_stream_t enc_out_stream;
	int cur_stream_len;
	int pre_stream_len;
	media_player_t *player;
	struct thread_timer check_timer;
} t_media_processor;

void noise_cancel_start(const char *saved_url);
void noise_cancel_mp3_start(const char *saved_url);
void noise_cancel_stop(void);
void noise_cancel_get_vad(void);

#endif

