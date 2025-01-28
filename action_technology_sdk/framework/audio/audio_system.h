/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file audio system api
 * @brief
*/

#ifndef __AUDIO_SYSTEM_H__
#define __AUDIO_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stream.h>

/**
 * @defgroup audio_system_apis Auido System APIs
 * @ingroup media_system_apis
 * @{
 */
typedef enum {
	AUDIO_STREAM_DEFAULT = 1,
	AUDIO_STREAM_MUSIC,
	AUDIO_STREAM_LOCAL_MUSIC,
	AUDIO_STREAM_TTS,
	AUDIO_STREAM_VOICE,
	AUDIO_STREAM_LINEIN,
	AUDIO_STREAM_LINEIN_MIX,
	AUDIO_STREAM_SUBWOOFER,
	AUDIO_STREAM_ASR,
	AUDIO_STREAM_AI,
	AUDIO_STREAM_USOUND,
	AUDIO_STREAM_USOUND_MIX,
	AUDIO_STREAM_USPEAKER,
	AUDIO_STREAM_I2SRX_IN,
	AUDIO_STREAM_SPDIF_IN,
	AUDIO_STREAM_GENERATE_IN,
	AUDIO_STREAM_GENERATE_OUT,
	AUDIO_STREAM_LOCAL_RECORD,
	AUDIO_STREAM_GMA_RECORD,
	AUDIO_STREAM_BACKGROUND_RECORD,
	AUDIO_STREAM_MIC_IN,
	AUDIO_STREAM_FM,
	AUDIO_STREAM_TWS,
	AUDIO_STREAM_TIP,
	AUDIO_STREAM_LE_AUDIO,
}audio_stream_type_e;

typedef enum {
	AUDIO_MODE_DEFAULT = 0,
	AUDIO_MODE_MONO,                    //mono->left mono->right
	AUDIO_MODE_STEREO,                  //left->left right->right
}audio_mode_e;

typedef enum {
	AUDIO_STREAM_TRACK = 1,
	AUDIO_STREAM_RECORD,
}audio_stream_mode_e;

typedef enum {
	AUDIO_FORMAT_PCM_8_BIT = 0,
	AUDIO_FORMAT_PCM_16_BIT,
	AUDIO_FORMAT_PCM_24_BIT,
	AUDIO_FORMAT_PCM_32_BIT,
}audio_format_e;

/**
**	aps changer mode
**/
typedef enum {
	/* audjust by audio pll*/
	APS_LEVEL_AUDIOPLL = 0,
	/* audjust by asrc*/
	APS_LEVEL_ASRC,
	APS_MODE_MAX,
} aps_level_mode_e;

/**
 * @cond INTERNAL_HIDDEN
 */

#define MIN_WRITE_SAMPLES    1 * 1024

#define MAX_AUDIO_TRACK_NUM  1
#define MAX_AUDIO_RECORD_NUM 2
#define MAX_AUDIO_DEVICE_NUM 1

#define MAX_VOLUME_VALUE 2
#define MIN_VOLUME_VALUE 1
#define DEFAULT_VOLUME   5

struct audio_track_t {
	uint8_t stream_type;
	uint8_t audio_format;
	uint8_t channels;
	uint8_t channel_type;
	uint8_t channel_id;
	uint8_t channel_mode;
	uint8_t sample_rate;
	uint8_t frame_size;
	uint8_t flushed;
	uint8_t muted:1;
	uint8_t stared:1;
	uint8_t waitto_start:1;
	/**debug flag*/
	uint8_t dump_pcm:1;
	uint8_t fill_zero:1;
	uint8_t fade_mode:2;
	uint8_t dsp_fifo_src:1;
	uint16_t volume;
	uint32_t output_sample_rate_hz;

	uint16_t pcm_frame_size;
	uint8_t *pcm_frame_buff;

	io_stream_t audio_stream;
	io_stream_t mix_stream;
	uint8_t mix_sample_rate;
	uint8_t mix_channels;

	/** audio hal handle*/
	void *audio_handle;

	void (*event_cb)(uint8_t, void *);
	void *user_data;

	/* For tws sync fill samples */
	int compensate_samples;
	int fill_cnt;

	/* resample */
	void *res_handle;
	int res_in_samples;
	int res_out_samples;
	int res_remain_samples;
	int16_t *res_in_buf[2];
	int16_t *res_out_buf[2];

	/* fade in/out */
	void *fade_handle;

	/* mix */
	void *mix_handle;

    uint64_t total_samples_filled;
    int32_t sample_fix;
};

#define AUDIO_ADC_NUM   (4)

typedef struct {
	int16_t ch_gain[AUDIO_ADC_NUM];
} audio_input_gain;

struct audio_record_t {
	uint8_t stream_type;
	uint8_t audio_format;
	uint8_t audio_mode;
	uint8_t channel_type;
	uint8_t channel_id;
	uint8_t channel_mode;
	uint8_t sample_rate;
	uint8_t output_sample_rate;
	uint8_t frame_size;
	uint8_t muted:1;
	uint8_t paused:1;
	uint8_t first_frame:1;
	/**debug flag*/
	uint8_t dump_pcm:1;
	uint8_t fill_zero:1;
    uint8_t reload_buff_alloced:1;
    uint8_t drop_cnt:2;
    uint8_t printk_cnt;

	int16_t adc_gain;
	int16_t input_gain;
	uint16_t volume;
	uint16_t pcm_buff_size;
	uint8_t *pcm_buff;

    /* block to save adc pcm data if playback no start yet, 0: no block, 1: block */
    uint16_t *play_flag;

	/** audio hal handle*/
	void *audio_handle;
	io_stream_t audio_stream;
};

struct audio_system_t {
	os_mutex audio_system_mutex;
	struct audio_track_t *audio_track_pool[MAX_AUDIO_TRACK_NUM];
	struct audio_record_t *audio_record_pool[MAX_AUDIO_RECORD_NUM];
	struct audio_device_t *audio_device_pool[MAX_AUDIO_DEVICE_NUM];
	uint8_t audio_track_num;
	uint8_t audio_record_num;
	bool microphone_muted;
	uint8_t output_sample_rate;
	uint8_t capture_output_sample_rate;
	bool master_muted;
	uint8_t master_volume;

	uint8_t tts_volume;
	uint8_t music_volume;
	uint8_t voice_volume;
	uint8_t linein_volume;
	uint8_t fm_volume;
	uint8_t i2srx_in_volume;
	uint8_t mic_in_volume;
	uint8_t spidf_in_volume;
	uint8_t usound_volume;
	uint8_t lcmusic_volume;
	uint8_t max_volume;
	uint8_t min_volume;
};

/** cace info ,used for cache stream */
typedef struct
{
	uint8_t audio_type;
	uint8_t audio_mode;
	uint8_t channel_mode;
	uint8_t stream_start:1;
	uint8_t dma_start:1;
	uint8_t dma_reload:1;
	uint8_t pcm_buff_owner:1;
	uint8_t data_finished:4;
	uint16_t dma_send_len;
	uint16_t pcm_frame_size;
	struct acts_ringbuf *pcm_buff;
	/**pcm cache*/
	io_stream_t pcm_stream;
} audio_info_t;

typedef enum
{
    APS_OPR_SET          = (1 << 0),
    APS_OPR_ADJUST       = (1 << 1),
    APS_OPR_FAST_SET     = (1 << 2),
}aps_ops_type_e;

typedef enum
{
    APS_STATUS_DEC,
    APS_STATUS_INC,
    APS_STATUS_DEFAULT,
}aps_status_e;

typedef struct {
	uint8_t current_level;
	uint8_t dest_level;
	uint8_t aps_status;
	uint8_t aps_mode;

	uint8_t aps_min_level;
	uint8_t aps_max_level;
	uint8_t aps_default_level;
	uint8_t role;
	uint8_t duration;
	uint8_t need_aps:1;

	uint16_t aps_reduce_water_mark;
	uint16_t aps_increase_water_mark;

#ifdef CONFIG_SNOOP_LINK_TWS
    uint16_t first_pkt_num;
#endif

	struct audio_track_t *audio_track;
	void *tws_observer;
}aps_monitor_info_t;
/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief set audio system output sample rate
 *
 * @details This routine provides to set audio system output sample rate,
 *  if set audio system output sample rate, all out put stream may resample to
 *  the target sample rate
 *
 * @param value sample rate
 *
 * @return 0 excute successed , others failed
 */
int audio_system_set_output_sample_rate(int value);
/**
 * @brief get audio system output sample rate
 *
 * @details This routine provides to get audio system output sample rate,
 *
 * @return value of sample rate
 */
int audio_system_get_output_sample_rate(void);

/**
 * @brief set audio system master volume
 *
 * @details This routine provides to set audio system master volume
 *
 * @param value volume value
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_volume(int value);

/**
 * @brief get audio system master volume
 *
 * @details This routine provides to get audio system master volume
 *
 * @return value of volume level
 */

int audio_system_get_master_volume(void);

/**
 * @brief set audio system master mute
 *
 * @details This routine provides to set audio system master mute
 *
 * @param value mute value 1: mute 0: unmute
 *
 * @return 0 excute successed , others failed
 */

int audio_system_set_master_mute(int value);

/**
 * @brief get audio system master mute state
 *
 * @details This routine provides to get audio system master mute state
 *
 * @return  1: audio system muted
 * @return  0: audio system unmuted
 */

int audio_system_get_master_mute(void);

int audio_system_set_stream_volume(int stream_type, int value);

int audio_system_get_stream_volume(int stream_type);

int audio_system_get_current_volume(int stream_type);

int audio_system_set_stream_mute(int stream_type, int value);

int audio_system_get_stream_mute(int stream_type);

int audio_system_mute_microphone(int value);

int audio_system_get_microphone_muted(void);

int audio_system_get_current_pa_volume(int stream_type);

/* @volume in 0.001 dB */
int audio_system_set_stream_pa_volume(int stream_type, int volume);

/* @volume in 0.1 dB */
int audio_system_set_microphone_volume(int stream_type, int volume);

int audio_system_get_max_volume(void);

int audio_system_get_min_volume(void);

int aduio_system_init(void);
/**
 * @cond INTERNAL_HIDDEN
 */
int audio_system_register_track(struct audio_track_t *audio_track);

int audio_system_unregister_track(struct audio_track_t *audio_track);

int audio_system_register_record(struct audio_record_t *audio_record);

int audio_system_unregister_record(struct audio_record_t *audio_record);

void audio_aps_monitor(int pcm_time);

void audio_aps_monitor_init(int format, void *tws_observer, struct audio_track_t *audio_track);

void audio_aps_monitor_init_add_samples(int format, uint8_t *need_notify, uint8_t *need_sync);

void audio_aps_monitor_exchange_samples(uint32_t *ext_add_samples, uint32_t *sync_ext_samples);

void audio_aps_notify_decode_err(uint16_t err_cnt);

void audio_aps_monitor_deinit(int format, void *tws_observer, struct audio_track_t *audio_track);

void audio_aps_monitor_tws_init(void *tws_observer);

void audio_aps_tws_notify_decode_err(uint16_t err_cnt);

int32_t audio_tws_set_stream_info(uint8_t format, uint16_t first_pktnum, uint8_t sample_rate);

/**
 * @brief get snoop tws first stream packet num to play
 *
 * @return  0: first pktnum no ready, retry next time
 * @return  >0: normal first pktnum
 */
uint16_t audio_tws_get_playback_first_pktnum(void);

int32_t audio_tws_set_pkt_info(uint16_t pkt_num, uint16_t pkt_len, uint16_t pcm_len);

void audio_aps_monitor_tws_deinit(void *tws_observer);

aps_monitor_info_t *audio_aps_monitor_get_instance(void);

struct audio_track_t * audio_system_get_track(void);

int audio_system_mutex_lock(void);
int audio_system_mutex_unlock(void);

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @} end defgroup audio_system_apis
 */

#ifdef __cplusplus
}
#endif

#endif
