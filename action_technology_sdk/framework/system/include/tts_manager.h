/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tts manager interface
 */

#ifndef __TTS_MANAGER_H__
#define __TTS_MANAGER_H__

/**
 * @defgroup tts_manager_apis App tts Manager APIs
 * @ingroup system_apis
 * @{
 */
/** tts mode */
enum tts_mode_e
{
	BLOCK_INTERRUPTABLE = 0x1,
	BLOCK_UNINTERRUPTABLE = 0x2,
	UNBLOCK_UNINTERRUPTABLE = 0x4,
	UNBLOCK_INTERRUPTABLE = 0x8,
	PLAY_IMMEDIATELY = 0x10,
	TTS_MODE_TELE_NUM = 0x20,  //telephone number
	LOOP_MODE = 0x40,
};

/** tts file storage medium */
enum tts_storage_medium_e
{
	/** tts file storage in sdfs on nor*/
	TTS_SOTRAGE_SDFS = 1,
	/** tts file storage in fatfs on sdcard*/
	TTS_SOTRAGE_SDCARD = 2,
};

/** tts event */
enum tts_event_e
{
	/** tts event start play */
	TTS_EVENT_START_PLAY = 1,
	/** tts event stop play */
	TTS_EVENT_STOP_PLAY = 2,
	/** tts event stop play */
	TTS_EVENT_NEXT_PLAY = 3,
};

/** tts ui event map */
typedef struct
{
	/** ui event */
	uint16_t ui_event;
	/** tts mode */
	uint16_t mode;
	/** name of tts file*/
	const uint8_t *tts_file_name;
}tts_ui_event_map_t;

/** tts config */
struct tts_config_t
{
	/** tts media storage type link:tts_storage_medium_e */
	uint32_t tts_storage_media;
	/** tts and ui event map*/
	const tts_ui_event_map_t *tts_map;
	/** tts and ui event map size*/
	uint32_t tts_map_size;
	/** default path of tts file*/
	const uint8_t *tts_def_path;
};

/**
 * @brief lock tts manager
 *
 * This routine calls to lock tts manager, tts will drop when if tts manager locked
 * If the user does not want to play TTS for a period of time, then can used this interface to lock tts mangaer.
 *
 * @return N/A
 */

typedef void (*tts_event_nodify)(uint8_t *, uint32_t);
/**
 * @brief tts manager init funcion
 *
 * This routine calls init tts manager ,called by main
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_init(void);

/**
 * @brief tts manager deinit funcion
 *
 * This routine calls deinit tts manager ,called by main ,release all tts manager resource.
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_deinit(void);
/**
 * @brief lock tts manager
 *
 * This routine calls to lock tts manager, tts will drop when if tts manager locked
 * If the user does not want to play TTS for a period of time, then can used this interface to lock tts mangaer.
 *
 * @return N/A
 */

void tts_manager_lock(void);

/**
 * @brief unlock tts manager
 *
 * This routine calls to unlock tts manager
 *
 * @return N/A
 */

void tts_manager_unlock(void);

/**
 * @brief get tts manager state
 *
 * This routine calls to unlock tts manager,
 *
 * @return true tts manager is locked
 * @return false tts manager is unlocked
 */

bool tts_manager_is_locked(void);

/**
 * @brief start to play tts
 *
 * This routine calls to play tts 
 *
 * @param tts_name url for tts witch tts user want to play
 * @param mode tts_mode_e
 * @param bt_clk time to start play in bt clock, set to UINT64_MAX if no need sync
 * @param pattern if mode == TTS_MODE_TELE_NUM, tts_name should be phone num, pattern is the phone num pattern, example: V_NO_%c.act
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tts_manager_play_with_time(uint8_t *tts_name, uint32_t mode, uint64_t bt_clk, char *pattern);

#define tts_manager_play(tts_name, mode) tts_manager_play_with_time(tts_name, mode, UINT64_MAX, NULL)

/**
 * @brief start to play tts
 *
 * This routine calls to play tts blocked
 *
 * @param tts_name url for tts witch tts user want to play, this blocked caller thread
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_play_block(uint8_t *tts_name);

int tts_manager_play_process(void);
/**
 * @brief stop to play tts
 *
 * This routine calls to interrupt to stop tts
 *
 * @param tts_name url for tts witch want to stop
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
 
int tts_manager_stop(uint8_t *tts_name);

/**
 * @brief wait tts finised
 *
 * This routine calls to wait tts fininsed
 *
 * @param clean_all true clean all tts ,false not clean all tts
 *
 * @return N/A
 */

void tts_manager_wait_finished(bool clean_all);
/**
 * @brief tts manager process ui event 
 *
 * This routine calls tts manager procees ui event, 
 * find tts according tts config map id and tts file name
 *
 * @param ui_event ui event id
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_process_ui_event(int ui_event);

/**
 * @brief tts manager register tts config 
 *
 * tts config map id and tts file name , and config tts mode 
 * tts manager receive tts id and find tts information according tts config
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tts_manager_register_tts_config(const struct tts_config_t *config);

/**
 * @cond INTERNAL_HIDDEN
 */
/**
 * @brief tts manager add event lisener
 *
 * This routine calls add event lisener to tts manager lisener list
 * tts manager will notify lisener when tts is start and tts is stop
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */

int tts_manager_add_event_lisener(tts_event_nodify lisener);

/**
 * @brief tts manager remove event lisener
 *
 * This routine calls remove event lisener from tts manager lisener list
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tts_manager_remove_event_lisener(tts_event_nodify lisener);

int key_tone_manager_init(void);

int key_tone_play(const char *file_name);

/**
 * @brief start to play tip tone
 *
 * This routine calls to play tip tone and stop the last one
 *
 * @param tip_name url for tip tone which tip user want to play
 * @param bt_clk time to start play in bt clock, set to UINT64_MAX if no need sync
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tip_manager_play(uint8_t *tip_name, uint64_t bt_clk);

/**
 * @brief stop to play tip tone
 *
 * This routine calls to interrupt to stop tip tone
 *
 * @return 0 if invoked succsess.
 * @return others if invoked failed.
 */
int tip_manager_stop(void);

/**
 * @brief get the playing tip tone file name
 *
 * This routine calls to get the playing tip tone file name
 *
 * @return file name or null string if nothing playing
 */
const char* tip_manager_get_playing_filename(void);

/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup tts_manager_apis
 */

#endif

