/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Media player interface
 */

#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <media_type.h>
#include <stream.h>
#include <media_service.h>

/**
 * @defgroup media_player_apis Media Player APIs
 * @ingroup media_system_apis
 * @{
 */

/** media player type */
typedef enum {
	/** media player type: only support playback */
	MEDIA_PLAYER_TYPE_PLAYBACK = 0x01,
	/** media player type: only support capture */
	MEDIA_PLAYER_TYPE_CAPTURE = 0x02,
	/** media player type: only support capture and playback */
	MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK = 0x03,
} media_player_type_e;

/**media event type*/
typedef enum {
	PLAYBACK_EVENT_OPEN,
	PLAYBACK_EVENT_DATA_INDICATE,
	PLAYBACK_EVENT_PAUSE,
	PLAYBACK_EVENT_RESUME,
	PLAYBACK_EVENT_SEEK,
	PLAYBACK_EVENT_STOP_INTERUPT,
	PLAYBACK_EVENT_STOP_COMPLETE,
	PLAYBACK_EVENT_STOP_ERROR,
	PLAYBACK_EVENT_CLOSE,

	CAPTURE_EVENT_OPEN,
	CAPTURE_EVENT_DATA_INDICATE,
	CAPTURE_EVENT_PAUSE,
	CAPTURE_EVENT_RESUME,
	CAPTURE_EVENT_STOP_INTERUPT,
	CAPTURE_EVENT_STOP_COMPLETE,
	CAPTURE_EVENT_STOP_ERROR,
	CAPTURE_EVENT_CLOSE,
	CAPTURE_EVENT_ASR,
	CAPTURE_EVENT_VAD,
	CAPTURE_EVENT_PP, /* audio processor event */

	PARSER_EVENT_OPEN,
	PARSER_EVENT_SEEK,
	PARSER_EVENT_STOP_COMPLETE,
	PARSER_EVENT_STOP_ERROR,
	PARSER_EVENT_CLOSE,

	/* player lifecycle event */
	PLAYER_EVENT_OPEN,
	PLAYER_EVENT_PLAY,
	PLAYER_EVENT_PAUSE,
	PLAYER_EVENT_RESUNE,
	PLAYER_EVENT_STOP,
	PLAYER_EVENT_CLOSE,
} media_event_type_e;

/** media capture mode */
typedef enum {
	PLAYBACK_MODE_MASK = 0xf,

	CAPTURE_MODE_ENCODE =  0x1 << 4,
	CAPTURE_MODE_ASR    =  0x2 << 4,
	CAPTURE_MODE_MASK   =  0xf << 4,
} media_mode_e;

typedef enum {
	PLAYER_STATE,
	PLAYER_DECODE_ERROR,
	PLAYER_DECODE_SAMPLES,
	PLAYER_DECODE_DATALOST,
	PLAYER_DECODE_RAW,
} media_player_state_e;
/** media force stop callback*/
typedef void (*force_stop_callback)(void *stop_handle);

/** media player structure */
typedef struct {
	/** type of media player @see media_player_type_e */
	uint8_t type;
	/** flag of media player */
	uint8_t flag;
	/** tws flag of media player */
	uint8_t is_tws;
	/** dvfs level */
	uint8_t dvfs_level;
	/** handle of media service*/
	void *media_srv_handle;
	/** force stop cb*/
	force_stop_callback stop_cb;
	/** force stop cb*/
	void *stop_handle;
} media_player_t;

/** media voice effect mode */
typedef enum {
	VOICE_DOWNLOAD_EFFECT,
	VOICE_UPLOAD_EFFECT,
} voice_effect_mode;
/**
 * @brief open new media player
 *
 * This routine provides to open new media player,
 *  support three type of media player &media_player_type_e
 *
 * @param init_param init param for open media player
 * @details	init_param.format play back format &media_type_e;
 *	init_param.sample_rate play back sample rate
 *	init_param.input_indicator input stream indicator, if set input stream ,indicator may set 0;
 *	init_param.input_stream input stream for playback, if set input indicator, input stream may create by service;
 *	init_param.input_stream_size input stream max size
 *	init_param.input_start_threshold input stream start threshold;
 *	init_param.input_stop_threshold input stream stop threshold;
 *	init_param.event_notify_handle playback event notify handle;
 *	init_param.output_indicator output stream indicator, if set output stream ,indicator may set 0;
 *	init_param.output_stream output stream for playback, if set output indicator, output stream may create by service;
 *	init_param.capture_format capture encode format;
 *	init_param.capture_sample_rate_input capture input sample rate;
 *	init_param.capture_sample_rate_output capture out sample rate;
 *	init_param.capture_input_indicator = AUDIO_STREAM_VOICE;
 *	init_param.capture_input_stream = NULL;
 *	init_param.capture_output_indicator = 0;
 *	init_param.capture_output_stream = upload_stream;
 *	init_param.capture_event_notify_handle capture event notify handle;
 *	init_param.dumpable indicate support dump data;
 *	init_param.support_tws indicate support tws;
 *	init_param.support_csb indicate support csb;
 *	init_param.aec_enable indicate support aec;
 *
 * @return handle of new media player
 */
media_player_t *media_player_open(media_init_param_t *init_param);

/**
 * @brief start play for media player
 *
 * This routine provides to start meida player play
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_play(media_player_t *handle);

/**
 * @brief pause for media player
 *
 * This routine provides to pause meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_pause(media_player_t *handle);

/**
 * @brief resume for media player
 *
 * This routine provides to resume meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_resume(media_player_t *handle);

/**
 * @brief seek for media player
 *
 * This routine provides to seek meida player.
 * only supported when local music play or net music play.
 *
 * @param handle handle of media player
 * @param info media seek info @see media_seek_info_t
 *
 * @return 0 excute successed , others failed
 */
int media_player_seek(media_player_t *handle, media_seek_info_t *info);

/**
 * @brief stop for media player
 *
 * This routine provides to stop meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_stop(media_player_t *handle);

/**
 * @brief close for media player
 *
 * This routine provides to close meida player
 *
 * @param handle handle of media player
 *
 * @return 0 excute successed , others failed
 */
int media_player_close(media_player_t *handle);

/**
 * @brief Query media player parameter
 *
 * This routine provides to query meida player parameter
 *
 * @param handle handle of media player
 * @param pname query parameter name
 * @param param address of parameter to store query result
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_get_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Configure media player parameter
 *
 * This routine provides to configure meida player parameter
 *
 * @param handle handle of media player
 * @param pname parameter name
 * @param param address of parameter buffer
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Query media service global parameter
 *
 * This routine provides to query media service global parameter
 *
 * @param handle handle of media player, can be NULL
 * @param pname query parameter name
 * @param param address of parameter to store query result
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_get_global_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Configure media service global parameter
 *
 * This routine provides to configure media service global parameter
 *
 * @param handle handle of media player, can be NULL
 * @param pname parameter name
 * @param param address of parameter buffer
 * @param psize size of parameter buffer
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_global_parameter(media_player_t *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Get media player media info
 *
 * This routine provides to get meida player media info
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info address to store media info
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_mediainfo(media_player_t *handle, media_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_MEDIAINFO, info, sizeof(*info));
}

/**
 * @brief Get media player playing breakpoint
 *
 * This routine provides to get meida player breakpoint
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info  address to store breakpoint info
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_breakpoint(media_player_t *handle, media_breakpoint_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_BREAKPOINT, info, sizeof(*info));
}

/**
 * @brief Get media player output mode
 *
 * This routine provides to get meida player output mode. This output mode is
 * on the upper data stream of effect output mode.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param mode  address to store output mode, see media_output_mode_e
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_output_mode(media_player_t *handle, int *mode)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_OUTPUT_MODE, mode, sizeof(*mode));
}

/**
 * @brief Set media player energy filter param.
 *
 * This routine provides to set media player energy filter param.
 *
 * @param handle handle of media player
 * @param info	address of energy filter structure
 *
 * @return 0 excute successed, others failed
 */
#if 0
static inline int media_player_set_energy_filter(media_player_t *handle, energy_filter_t *info)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_ENERGY_FILTER, (void *)info, sizeof(*info));
}
#endif

/**
 * @brief get effect output mode for media player
 *
 * This routine provides to get effect output mode for meida player
 *
 * @param handle handle of media player
 * @param mode   output mode, see media_effect_output_mode_e
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_effect_output_mode(media_player_t *handle, int *mode)
{
	return media_player_get_parameter(handle, MEDIA_EFFECT_EXT_GET_DAE_OUTPUT_MODE, mode, sizeof(*mode));
}

/**
 * @brief Get media player freq point energy.
 *
 * This routine provides to get media player freq point energy.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info   store the freq point energy, including the number of points and energy values.
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_get_freqpoint_energy(media_player_t *handle, media_freqpoint_energy_info_t *info)
{
	return media_player_get_parameter(handle, MEDIA_EFFECT_EXT_GET_FREQPOINT_ENERGY, info, sizeof(*info));
}

/**
 * @brief Set media player energy freq point.
 *
 * This routine provides to set media player energy freq point.
 *
 * Special case of media_player_get_parameter
 *
 * @param handle handle of media player
 * @param info   set the energy freq points, including the number of points and freq values.
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_energy_freqpoint(media_player_t *handle, media_freqpoint_energy_info_t *info)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_ENERGY_FREQPOINT, info, sizeof(*info));
}

/**
 * @brief Set media player output mode
 *
 * This routine provides to set meida player output mode. This output mode is
 * on the upper data stream of effect output mode.
 *
 * Special case of media_player_set_parameter
 *
 * @param handle handle of media player
 * @param mode  output mode, see media_output_mode_e
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_output_mode(media_player_t *handle, int mode)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_OUTPUT_MODE, (void *)mode, 0);
}

/**
 * @brief Set media player max audio lantency
 *
 * This routine provides to set meida player max audio lantency.
 *
 * Special case of media_player_set_parameter
 *
 * @param handle handle of media player
 * @param latency  max audio lantency in ms
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_audio_latency(media_player_t *handle, int latency)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_AUDIO_LATENCY, (void *)latency, 0);
}

/**
 * @brief Set media player sync play time
 *
 * This routine provides to set meida player sync play time.
 *
 * Special case of media_player_set_parameter
 *
 * @param handle handle of media player
 * @param bt_clk  bluetooth clock time in us
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_set_sync_play_time(media_player_t *handle, uint64_t bt_clk)
{
	return media_player_set_parameter(handle, MEDIA_PARAM_SET_PLAY_TIME, (void *)&bt_clk, sizeof(bt_clk));
}

/**
 * @brief Get media record time
 *
 * This routine provides to get media record time in ms.
 *
 * @param handle handle of media player
 * @param record_time_ms  record time in ms
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_get_record_time(media_player_t *handle, uint32_t *record_time_ms)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_GET_RECORD_TIME, (void *)record_time_ms, sizeof(*record_time_ms));
}

static inline int media_player_get_vad_state(media_player_t *handle, uint8_t* vad_state)
{
	return media_player_get_parameter(handle, MEDIA_PARAM_GET_VAD_STATE, (void *)vad_state, sizeof(*vad_state));
}

/**
 * @brief set volume for media player
 *
 * This routine provides to set volume of meida player
 *
 * @param handle handle of media player
 * @param vol_l da volume level of left channel
 * @param vol_r da volume level of right channel
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_volume(media_player_t *handle, int vol_l, int vol_r)
{
	unsigned int volume;

    if(vol_l < 0) vol_l = 0;
    if(vol_l > 0xff) vol_l = 0xff;
    if(vol_r < 0) vol_r = 0;
    if(vol_r > 0xff) vol_r = 0xff;

    volume = ((vol_r & 0xff) << 8) | (vol_l & 0xff);
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_VOLUME, (void *)volume, 0);
}

/**
 * @brief set mic mute for media player
 *
 * This routine provides to set mic mute for meida player
 *
 * @param handle handle of media player
 * @param mute bool for mute or unmute
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_mic_mute(media_player_t *handle, bool mute)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_MIC_MUTE, (void *)mute, 0);
}

/**
 * @brief set hfp connected flag for media player
 *
 * This routine provides to  set hfp connected flag for meida player
 *
 * @param handle handle of media player
 * @param connected bool for connected or unconnected
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_hfp_connected(media_player_t *handle, bool connected)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_HFP_CONNECTED, (void *)connected, 0);
}

/**
 * @brief set effect enable for media player
 *
 * This routine provides to enable or disable audio effects for meida player
 *
 * @param handle handle of media player
 * @param enable enable or disable audio effect
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_effect_enable(media_player_t *handle, bool enable)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_ENABLE, (void *)enable, 0);
}

/**
 * @brief set bt call upstream effect enable or no
 *
 * This routine provides to enable or disable upstream effect.
 * Default is enabled.
 *
 * @param handle handle of media player
 * @param enable enable or disable upstream effect
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_upstream_dae_enable(media_player_t *handle, bool enable)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_UPSTREAM_DAE_ENABLE, (void *)enable, 0);
}

/**
 * @brief set effect bypass for media player
 *
 * This routine provides to bybpass audio effects for meida player, only
 * retain fade in/out and output mode config.
 *
 * @param handle handle of media player
 * @param bypass bypass audio effect or not
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_effect_bypass(bool bypass);

static inline int _media_player_set_effect_bypass(media_player_t *handle, bool bypass)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_BYPASS, (void *)bypass, 0);
}

/**
 * @brief set voice effect bypass for media player
 *
 * This routine provides to bybpass audio voice effects for meida player.
 *
 * @param type handle of media player
 * @param bypass bypass audio effect or not
 *
 * @return 0 excute successed , others failed
 */
int media_player_set_voice_effect_bypass(int type, bool bypass);

static inline int _media_player_set_voice_effect_bypass(media_player_t *handle, uint8_t type, bool bypass)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_EFFECT_BYPASS, (void *)(type << 8 | bypass), 0);
}

/**
 * @brief set effect output mode for media player
 *
 * This routine provides to set effect output mode for meida player
 *
 * @param handle handle of media player
 * @param mode   output mode, see media_effect_output_mode_e
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_set_effect_output_mode(media_player_t *handle, int mode)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_SET_DAE_OUTPUT_MODE, (void *)mode, 0);
}

/**
 * @brief update effect param for media player
 *
 * This routine provides to update effect param for media player
 *
 * @param handle handle of media player
 * @param param Address of effect param
 * @param size Size of effect param
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_update_effect_param(media_player_t *handle, void *param, unsigned int size)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_UPDATE_PARAM, param, size);
}

/**
 * @brief update aec param for media player
 *
 * This routine provides to update aec param for media player
 *
 * @param handle handle of media player
 * @param param Address of aec param
 * @param size Size of aec param
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_update_aec_param(media_player_t *handle, void *param, unsigned int size)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_UPDATE_AEC_PARAM, param, size);
}

/**
 * @brief fade in for media player
 *
 * This routine provides to fade in for media player
 *
 * @param handle handle of media player
 * @param fade_time_ms fadein time in ms
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_fade_in(media_player_t *handle, int fade_time_ms)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_FADEIN, (void *)fade_time_ms, 0);
}

/**
 * @brief fade out for media player
 *
 * This routine provides to fade out for media player
 *
 * @param handle handle of media player
 * @param fade_time_ms fadeout time in ms
 *
 * @return 0 excute successed , others failed
 */
static inline int media_player_fade_out(media_player_t *handle, int fade_time_ms)
{
	return media_player_set_parameter(handle, MEDIA_EFFECT_EXT_FADEOUT, (void *)fade_time_ms, 0);
}

/**
 * @brief dump data for media player
 *
 * This routine provides to dump data for media player
 *
 * @param handle handle of media player
 * @param num number of tags to dump
 * @param tags data tag array for dumpable data
 * @param bufs ring buf array to store dump data, set NULL to stop dumping
 *
 * @return 0 excute successed , others failed
 */
int media_player_dump_data(media_player_t *handle, int num, const uint8_t tags[], struct acts_ringbuf *bufs[]);

/**
 * @brief get current dumpable media player
 *
 * This routine provides to get current dumpable media player
 *
 * @return handle of media player @see media_player_t
 */
media_player_t *media_player_get_current_dumpable_player(void);

/**
 * @brief get current main media player
 *
 * This routine provides to get current main media player
 *
 * @return handle of media player @see media_player_t
 */
media_player_t *media_player_get_current_main_player(void);

/**
 * @brief start recording
 *
 * This routine provides start recording.
 *
 * @param handle of media player, can be NULL
 * @info  record info
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_start_record(media_player_t *handle, media_record_info_t *info)
{
	return media_player_set_global_parameter(handle, MEDIA_PARAM_SET_RECORD, info, sizeof(*info));
}

/**
 * @brief stop background recording
 *
 * This routine provides start background recording.
 *
 * @param handle of media player, can be NULL
 *
 * @return 0 excute successed, others failed
 */
static inline int media_player_stop_record(media_player_t *handle)
{
	return media_player_set_global_parameter(handle, MEDIA_PARAM_SET_RECORD, NULL, 0);
}

/**
 * @brief Set media player lifecycle notifier
 *
 * This routine provides set lifecycle notifier, like player open/close.
 *
 *@param notify lifecycle event notify.
 *
 * @return 0 excute successed, others failed
 */
int media_player_set_lifecycle_notifier(media_srv_event_notify_t notify);

/**
 * @brief force stop for media player, @only test
 *
 * This routine provides to stop meida player
 */
void media_player_force_stop(bool disable);

void media_player_set_force_stop_cb(media_player_t * player, void *stop_handle, force_stop_callback cb);

int media_player_set_mix_stream(media_player_t *handle, mix_service_param_t *init_param);

int media_mix_pcm_stream_open(const char *url, int inrate, int outrate);
int media_mix_pcm_stream_close(void);

#ifdef __cplusplus
}
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup media_player_apis
 */
#endif  /* __MEDIA_PLAYER_H__ */
