/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @defgroup bt service APIs
 * @ingroup bluetooth_system_apis
 * @brief Just for bt manager used, not for application.
 * @{
 * @}
 */

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @file bt service tws version and feature
 * Not need direct include this head file, just need include <btservice_api.h>.
 * this head file include by btservice_api.h
 */

#ifndef _BTSERVICE_TWS_API_H_
#define _BTSERVICE_TWS_API_H_

/*======================  version and feature =====================================*/
/** Tws feature */
#define TWS_FEATURE_SUBWOOFER					(0x01 << 0)
#define TWS_FEATURE_A2DP_AAC					(0x01 << 1)
#define TWS_FEATURE_LOW_LATENCY					(0x01 << 2)
#define TWS_FEATURE_UI_STARTSTOP_CMD			(0x01 << 3)
#define TWS_FEATURE_HFP_TWS			            (0x01 << 4)


/** 281B version and feature */
#define US281B_START_VERSION					0x01
#define US281B_END_VERSION						0x0F
#define IS_281B_VERSION(x)						(((x) >= US281B_START_VERSION) && ((x) <= US281B_END_VERSION))

#define TWS_281B_VERSION_FEATURE_NO_SUBWOOFER	(0)
#define TWS_281B_VERSION_FEATURE_SUBWOOFER		(TWS_FEATURE_SUBWOOFER)

/** Woodperker version and feature
 * version: 0x10~0x1F, not support subwoofer
 * version: 0x20~0x2F, support subwoofer
 */
#define WOODPECKER_START_VERSION				0x10
#define WOODPECKER_END_VERSION					0x2F
#define IS_WOODPECKER_VERSION(x)				(((x) >= WOODPECKER_START_VERSION) && ((x) <= WOODPECKER_END_VERSION))

#define TWS_WOODPERKER_VERSION_0x10_FEATURE		(0)

#ifdef CONFIG_BT_A2DP_AAC
#define TWS_SUPPORT_FEATURE_AAC					TWS_FEATURE_A2DP_AAC
#else
#define TWS_SUPPORT_FEATURE_AAC					0
#endif
#ifdef CONFIG_OS_LOW_LATENCY_MODE
#define TWS_SUPPORT_FEATURE_LOW_LATENCY			TWS_FEATURE_LOW_LATENCY
#else
#define TWS_SUPPORT_FEATURE_LOW_LATENCY			0
#endif
#ifdef CONFIG_BT_HFP_TWS
#define TWS_SUPPORT_FEATURE_HFP_TWS				TWS_FEATURE_HFP_TWS
#else
#define TWS_SUPPORT_FEATURE_HFP_TWS				0
#endif

#define TWS_WOODPERKER_VERSION_0x11_FEATURE		(TWS_SUPPORT_FEATURE_AAC | TWS_SUPPORT_FEATURE_LOW_LATENCY | \
												TWS_FEATURE_UI_STARTSTOP_CMD | TWS_SUPPORT_FEATURE_HFP_TWS)


/** Current version and feature for ZS285A  stage 2 */
#define TWS_CURRENT_VERSOIN						(0x11)
#define TWS_CURRENT_VERSION_FEATURE				TWS_WOODPERKER_VERSION_0x11_FEATURE

#define get_tws_current_versoin()				TWS_CURRENT_VERSOIN
#define get_tws_current_feature()				TWS_CURRENT_VERSION_FEATURE

/*====================== btsrv tws define/struct/api =====================================*/
/** TWS mode */
enum {
	TWS_MODE_SINGLE = 0,		/* Single play mode */
	TWS_MODE_BT_TWS = 1,		/* bluetooth tws play mode */
	TWS_MODE_AUX_TWS = 2,		/* AUX tws mode(linein/pcm/FM RADIO/USB AUDIO/SPDIF RX/HDMI/HDMI ARC) */
	TWS_MODE_MUSIC_TWS = 3,		/* Local music tws mode(sd card/usb disk) */
};

/** TWS drc mode */
enum {
	DRC_MODE_NORMAL =  0,
	DRC_MODE_AUX    = 1,
	DRC_MODE_OFF    = 2,	/* SPDIF close DRC */
	DRC_MODE_OLD_VER = 7,	/* Master V1.2 version, not support set DRC mode */
};

/** TWS power off type */
enum {
	TWS_POWEROFF_SYS =  0,		/* System software poweroff */
	TWS_POWEROFF_KEY = 1,		/* Key software poweroff */
	TWS_POWEROFF_S2  = 2,		/* Enter S2 */
};

/** Tws speak channel */
enum {
	TWS_SPK_STEREO	= 0,
	TWS_SPK_LEFT	= 1,
	TWS_SPK_RIGHT	= 2,
};

/** TWS UI SYNC CMD */
enum {
	TWS_SYNC_S2M_UI_SYNC_REPLY				= 0x01,		/* Replay master sync command */
	TWS_SYNC_S2M_UI_UPDATE_BAT_EV			= 0x02,		/* Update battery level */
	TWS_SYNC_S2M_UI_UPDATE_KEY_EV			= 0x03,		/* Update key message */
	TWS_SYNC_S2M_UI_PLAY_TTS_REQ			= 0x04,		/* Request play tts */
	TWS_SYNC_S2M_UI_UPDATE_BTPLAY_EV		= 0x05,		/* Update enter/exit btplay mode, version */

	TWS_SYNC_S2M_UI_EXCHANGE_FEATURE		= 0x40,		/* Exchange feature, just for woodperker */

	TWS_SYNC_M2S_UI_SET_VOL_VAL_CMD			= 0x81,		/* Set volume */
	TWS_SYNC_M2S_UI_SET_VOL_LIMIT_CMD		= 0x82,		/* Set voluem limit */
	TWS_SYNC_M2S_UI_SWITCH_POS_CMD			= 0x83,		/* Switch sound channel */
	TWS_SYNC_M2S_UI_POWEROFF_CMD			= 0x84,		/* Poweroff */
	TWS_SYNC_M2S_UI_TTS_PLAY_REQ			= 0x85,		/* Request slave play tts */
	TWS_SYNC_M2S_UI_TTS_STA_REQ				= 0x86,		/* Check slave tts play state */
	TWS_SYNC_M2S_UI_TTS_STOP_REQ			= 0x87,		/* Request slave stop play tts */
	TWS_SYNC_M2S_SWITCH_NEXT_APP_CMD		= 0x88,		/* Notify switch to next app */

	TWS_SYNC_M2S_UI_SWITCH_TWS_MODE_CMD		= 0x90,		/* Swith tws mode */
	TWS_SYNC_M2S_UI_ACK_UPDATE_BTPLAY		= 0x91,		/* Replay receive slave TWS_SYNC_S2M_UI_UPDATE_BTPLAY_EV command */

	TWS_SYNC_M2S_UI_EXCHANGE_FEATURE		= 0xC0,		/* Exchange feature, just for woodperker */
	TWS_SYNC_M2S_START_PLAYER				= 0xC1,		/* Notify start player, just for support TWS_FEATURE_UI_STARTSTOP_CMD */
	TWS_SYNC_M2S_STOP_PLAYER				= 0xC2,		/* Notify stop player, just for support TWS_FEATURE_UI_STARTSTOP_CMD */
};

/** Tws sync replay */
enum {
	TWS_SYNC_REPLY_ACK = 0,		/* Sync command replay ACK */
	TWS_SYNC_REPLY_NACK = 1,	/* Sync command replay NACK */
};

/** tws event type */
typedef enum {
	TWS_UI_EVENT = 0x01,
	TWS_INPUT_EVENT = 0x02,
	TWS_SYSTEM_EVENT = 0x03,
	TWS_VOLUME_EVENT = 0x04,
	TWS_STATUS_EVENT = 0x05,
	TWS_BATTERY_EVENT = 0x06,
} btsrv_tws_event_type_e;

/** Callbacks to report Bluetooth service's tws events*/
typedef enum {
	/** notifying to check is pair tws deivce */
	BTSRV_TWS_DISCOVER_CHECK_DEVICE,
	/** notifying that tws connected */
	BTSRV_TWS_CONNECTED,
	/** notifying that tws disconnected */
	BTSRV_TWS_DISCONNECTED,
	/** notifying that tws start play */
	BTSRV_TWS_START_PLAY,
	/** notifying that tws play suspend */
	BTSRV_TWS_PLAY_SUSPEND,
	/** notifying that tws ready play */
	BTSRV_TWS_READY_PLAY,
	/** notifying that tws restart play*/
	BTSRV_TWS_RESTART_PLAY,
	/** notifying that tws event*/
	BTSRV_TWS_EVENT_SYNC,
	/** notifying that tws a2dp connected */
	BTSRV_TWS_A2DP_CONNECTED,
	/** notifying that tws hfp connected */
	BTSRV_TWS_HFP_CONNECTED,
	/** notifying tws irq trigger, be carefull, call back in irq context  */
	BTSRV_TWS_IRQ_CB,
	/** notifying still have pending start play after disconnected */
	BTSRV_TWS_UNPROC_PENDING_START_CB,
	/** notifying slave btplay mode */
	BTSRV_TWS_UPDATE_BTPLAY_MODE,
	/** notifying peer version */
	BTSRV_TWS_UPDATE_PEER_VERSION,
	/** notifying sink start play*/
	BTSRV_TWS_SINK_START_PLAY,
	/** notifying tws failed to do tws pair */
	BTSRV_TWS_PAIR_FAILED,
	/** Callback request app want tws role */
	BTSRV_TWS_EXPECT_TWS_ROLE,
	/** notifying received sco data */
	BTSRV_TWS_SCO_DATA,
	/** notifying call state */
	BTSRV_TWS_HFP_CALL_STATE,
} btsrv_tws_event_e;

/** Tws device role */
typedef enum {
	BTSRV_TWS_NONE,
	BTSRV_TWS_PENDING,		/** Get name finish but a2dp not connected, unkown a2dp source role */
	BTSRV_TWS_MASTER,
	BTSRV_TWS_SLAVE,
} btsrv_role_e;

/** Tws restart reason */
typedef enum {
	TWS_RESTART_SAMPLE_DIFF,
	TWS_RESTART_DECODE_ERROR,
	TWS_RESTART_LOST_FRAME,
	TWS_RESTART_ALREADY_RUNING,
	TWS_RESTART_MONITOR_TIMEOUT,
} btsrv_tws_restart_reason_e;

typedef int (*btsrv_tws_callback)(int event, void *param, int param_size);
typedef int (*media_trigger_start_cb)(void *handle);
typedef int (*media_is_ready_cb)(void *handle);
typedef uint32_t (*media_get_samples_cnt_cb)(void *handle, uint8_t *audio_mode);
typedef int (*media_get_error_state_cb)(void *handle);
typedef int (*media_get_aps_level_cb)(void *handle);
typedef int (*media_get_sample_rate_cb)(void *handle);

struct update_version_param {
	uint8_t versoin;
	uint32_t feature;
};

typedef struct {
	void *media_handle;
	media_is_ready_cb		is_ready;
	media_trigger_start_cb   trigger_start;
	media_get_samples_cnt_cb  get_samples_cnt;
	media_get_error_state_cb get_error_state;
	media_get_aps_level_cb   get_aps_level;
	media_get_sample_rate_cb    get_sample_rate;
} media_runtime_observer_t;

typedef int (*btsrv_tws_get_role)(void);
typedef int (*btsrv_tws_get_sample_diff)(int *samples_diff, uint8_t *aps_level, uint16_t *bt_clock);
typedef int (*btsrv_tws_set_media_observer)(media_runtime_observer_t *observer);
typedef int (*btsrv_tws_aps_change_notify)(uint8_t level);
typedef int (*btsrv_tws_trigger_restart)(uint8_t reason);
typedef int (*btsrv_tws_exchange_samples)(uint32_t *ext_add_samples, uint32_t *sync_ext_samples);
typedef int (*btsrv_tws_aps_nogotiate)(uint8_t *req_level, uint8_t *curr_level);

typedef struct {
	btsrv_tws_set_media_observer set_media_observer;
	btsrv_tws_get_role           get_role;
	btsrv_tws_get_sample_diff	 get_samples_diff;
	btsrv_tws_aps_change_notify  aps_change_notify;
	btsrv_tws_trigger_restart    trigger_restart;
	btsrv_tws_exchange_samples   exchange_samples;
	btsrv_tws_aps_nogotiate      aps_nogotiate;
} tws_runtime_observer_t;

int btif_tws_register_processer(void);
int btif_tws_init(btsrv_tws_callback cb);
int btif_tws_wait_pair(int try_times);
int btif_tws_cancel_wait_pair(void);
int btif_tws_disconnect(bd_address_t *addr);
int btif_tws_can_do_pair(void);
int btif_tws_is_in_connecting(void);
int btif_tws_get_dev_role(void);
int btif_tws_set_input_stream(io_stream_t stream);
int btif_tws_set_sco_input_stream(io_stream_t stream);
tws_runtime_observer_t *btif_tws_get_tws_observer(void);
int btif_tws_send_command(uint8_t *data, int len);
int btif_tws_send_command_sync(uint8_t *data, int len);
uint32_t btif_tws_get_bt_clock(void);
int btif_tws_set_bt_local_play(bool bt_play, bool local_play);
void btif_tws_update_tws_mode(uint8_t tws_mode, uint8_t drc_mode);
bool btif_tws_check_is_woodpecker(void);
bool btif_tws_check_support_feature(uint32_t feature);
void btif_tws_set_codec(uint8_t codec);

/**
 * INTERNAL_HIDDEN @endcond
 */

#endif
