#ifndef AUDIO_TEST_H
#define AUDIO_TEST_H

#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <audio_track.h>
#include <pcm_player.h>
#include <audio_record.h>
#include <fs/fs.h>
#include <ringbuff_stream.h>
#include <stream.h>
#include <srv_manager.h>
#include <mic_recorder.h>


enum AUDIO_APP_STATE {
	PLAYER_IDLE,
	PLAYER_RECORDING,
	PLAYER_PLAYING,
	PLAYER_MIC_BYPASS,
};

typedef struct {
	/* common */
	unsigned char player_state;
	unsigned char cur_vol;
	struct fs_file_t pcm_file;
	char *cache_buffer;
	int cache_buffer_size;
	/* recorder */
	t_mic_recorder *mic_recoder;
	unsigned int start_record_time;
	struct thread_timer record_timer;
	/* player */
	pcm_player_t *player;
	struct thread_timer play_timer;
	/* mic_bypass */
	struct thread_timer mic_bypass_timer;
} t_audio_app;


enum AUDIO_APP_CMD {
	RECORD_START,
	RECORD_STOP,
	PLAY_START,
	PLAY_STOP,
	MIC_BYPASS_START,
	MIC_BYPASS_STOP,
};


void record_start(char *url);
void record_stop(void);
void play_start(char *url);
void play_stop(void);
void mic_bypass_start(void);
void mic_bypass_stop(void);


#endif

