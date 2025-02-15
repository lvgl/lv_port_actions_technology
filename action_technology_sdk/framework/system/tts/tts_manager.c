/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tts manager interface
 */
#define SYS_LOG_DOMAIN "TTS"
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#ifdef  CONFIG_LOOP_FSTREAM
#include <loop_fstream.h>
#endif
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#ifdef CONFIG_SD_FS
#include <sdfs.h>
#endif

#ifdef CONFIG_BT_TRANSMIT
#include "../bt_transmit/bt_transmit.h"
#endif

struct tts_item_t {
	/* 1st word reserved for use by list */
	sys_snode_t node;
    uint64_t bt_clk;
    char *pattern;
	uint8_t tts_mode;
    uint8_t tele_num_index;
	char tts_file_name[0];
};

struct tts_manager_ctx_t {
	const struct tts_config_t *tts_config;
	sys_slist_t tts_list;
	struct tts_item_t *tts_curr;
	os_mutex tts_mutex;
	uint32_t tts_manager_locked:8;
	uint32_t tts_item_num:8;
	void *player_handle;
	io_stream_t tts_stream;
	tts_event_nodify lisener;
	os_delayed_work stop_work;

#ifdef CONFIG_PLAY_TIPTONE
    uint32_t tip_seq:4;
    uint32_t tip_seq_stopped:4;
    uint32_t tip_reserved:8;
    void *tip_player_handle;
    io_stream_t tip_stream;
    char tip_file_name[12];
    os_delayed_work tip_stop_work;
    os_mutex tip_mutex;
#endif
};

static struct tts_manager_ctx_t *tts_manager_ctx;
#ifdef CONFIG_TTS_MFILE_STREAM
static io_stream_t mfile_stream_create(struct buffer_t *param);
#endif
static struct tts_manager_ctx_t *_tts_get_ctx(void)
{
	return tts_manager_ctx;
}

static void _tts_event_nodify_callback(struct app_msg *msg, int result, void *reply)
{
	if (msg->sync_sem) {
		os_sem_give((os_sem *)msg->sync_sem);
	}
}

static void _tts_event_nodify(uint32_t event, uint8_t sync_mode)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	uint8_t *current_app = app_manager_get_current_app();

	if (current_app) {
		struct app_msg msg = {0};

		msg.type = MSG_TTS_EVENT;
		msg.value = event;
		if (sync_mode) {
			os_sem return_notify;

			os_sem_init(&return_notify, 0, 1);
			msg.callback = _tts_event_nodify_callback;
			msg.sync_sem = &return_notify;
			if (!send_async_msg(current_app, &msg)) {
				return;
			}
			os_mutex_unlock(&tts_ctx->tts_mutex);
			if (os_sem_take(&return_notify, OS_FOREVER) != 0) {
                os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
				return;
			}
			os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
		} else {
			if (!send_async_msg(current_app, &msg)) {
				return;
			}
		}
	}
}

static io_stream_t _tts_create_stream(uint8_t *filename, bool tele_num, int mode)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	io_stream_t stream = NULL;
#ifdef CONFIG_FILE_STREAM
	uint8_t *tts_fs_file = NULL;
	uint32_t size;
#endif

	if (tts_ctx->tts_config->tts_storage_media == TTS_SOTRAGE_SDFS) {
		struct buffer_t buffer;

	#ifdef CONFIG_SD_FS
		if (sd_fmap(filename, (void **)&buffer.base, &buffer.length) != 0) {
            SYS_LOG_ERR("file no exist\n");
			goto exit;
		}
	#endif

		buffer.cache_size = 0;
#ifdef CONFIG_TTS_MFILE_STREAM
        if(tele_num) {
            stream = mfile_stream_create(&buffer);
        } else {
            stream = buffer_stream_create(&buffer);
        }
#else
		stream = buffer_stream_create(&buffer);
#endif
		if (!stream) {
			SYS_LOG_ERR("create failed\n");
			goto exit;
		}
	} else if (tts_ctx->tts_config->tts_storage_media == TTS_SOTRAGE_SDCARD) {
	#ifdef CONFIG_FILE_STREAM
		if ((tts_ctx->tts_config->tts_def_path != NULL) && (filename[0] != '/')) {
			size = strlen(tts_ctx->tts_config->tts_def_path) + strlen(filename) + 1;
			tts_fs_file = mem_malloc(size);
			if (!tts_fs_file) {
				SYS_LOG_ERR("tts_file alloc failed\n");
				goto exit;
			}
			snprintf(tts_fs_file, size, "%s%s", tts_ctx->tts_config->tts_def_path, filename);
		} else {
			size = 0;
			tts_fs_file = filename;
		}
#ifdef CONFIG_LOOP_FSTREAM
		if ((mode & LOOP_MODE)) {
			stream = loop_fstream_create(tts_fs_file);
		} else 
#endif
		{
			stream = file_stream_create(tts_fs_file);
		}
		if (!stream) {
			SYS_LOG_ERR("create failed\n");
			goto exit;
		}
		if (size > 0) {
			mem_free(tts_fs_file);
		}
	#endif
	}

	if (stream) {
		if (stream_open(stream, MODE_IN) != 0) {
			SYS_LOG_ERR("open failed\n");
			stream_destroy(stream);
			stream = NULL;
			goto exit;
		}
	}

	SYS_LOG_INF("tts stream %p tts_item %s tts_path %s\n", stream, filename, tts_ctx->tts_config->tts_def_path);
exit:
	return stream;
}

static void _tts_remove_current_node(struct tts_manager_ctx_t *tts_ctx, struct tts_item_t *tts_item, bool fadeout)
{
	if (!tts_item)
		return;

	if (tts_ctx->player_handle) {
        if(fadeout) {
            media_player_fade_out(tts_ctx->player_handle, 10);
            os_sleep(10);
        }

#ifdef CONFIG_BT_TRANSMIT
		bt_transmit_catpure_stop();
#endif
		media_player_stop(tts_ctx->player_handle);
		media_player_close(tts_ctx->player_handle);
		tts_ctx->player_handle = NULL;
	}

	if (tts_ctx->tts_stream) {
		stream_close(tts_ctx->tts_stream);
		stream_destroy(tts_ctx->tts_stream);
		tts_ctx->tts_stream = NULL;
	}

	tts_manager_ctx->tts_curr = NULL;

	if (tts_ctx->lisener) {
		os_mutex_unlock(&tts_ctx->tts_mutex);
		tts_ctx->lisener(&tts_item->tts_file_name[0], TTS_EVENT_STOP_PLAY);
		os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
	}

	_tts_event_nodify(TTS_EVENT_STOP_PLAY, 0);

	mem_free(tts_item);
}

static void _tts_manager_play_event_notify(uint32_t event, void *data, uint32_t len, void *user_data)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	SYS_LOG_DBG("event %d , data %p , len %d\n", event, data, len);

	switch (event) {
	case PLAYBACK_EVENT_STOP_COMPLETE:
	#ifdef CONFIG_USER_WORK_Q
		os_delayed_work_submit_to_queue(os_get_user_work_queue(),&tts_ctx->stop_work, OS_NO_WAIT);
	#else
		os_delayed_work_submit(&tts_ctx->stop_work, OS_NO_WAIT);
	#endif
		break;
	}
}

static void _tts_manager_triggle_play_tts(void)
{
	struct app_msg msg = {0};
	msg.type = MSG_TTS_EVENT;
	msg.cmd = TTS_EVENT_START_PLAY;
	send_async_msg("main", &msg);
}

static int _tts_start_play(struct tts_item_t *tts_item, const char *file_name)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	int ret = 0;
	media_init_param_t init_param;

	memset(&init_param, 0, sizeof(media_init_param_t));
    tts_ctx->tts_curr = tts_item;

    if(strstr(file_name, ".act")) {
        init_param.format = ACT_TYPE;
        init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
    } else {
        init_param.format = MP3_TYPE;
        init_param.type = MEDIA_SRV_TYPE_PARSER_AND_PLAYBACK;
    }

	media_player_force_stop(false);

	init_param.stream_type = AUDIO_STREAM_TTS;
	init_param.efx_stream_type = AUDIO_STREAM_TTS;
	init_param.sample_rate = 16;
    init_param.sample_bits = 16;
    init_param.channels = 1;
	init_param.input_stream = tts_ctx->tts_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = _tts_manager_play_event_notify;
    init_param.support_tws = (tts_item->bt_clk == UINT64_MAX) ? 0:1;
	init_param.dsp_output = 1;

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_pre_start();
#endif
	tts_ctx->player_handle = media_player_open(&init_param);
	if (!tts_ctx->player_handle) {
		_tts_remove_current_node(tts_ctx, tts_item, false);
        ret = -1;
		goto exit;
	}

#ifdef CONFIG_BT_TRANSMIT
	//now dsp decode output always 2 channels
	bt_transmit_catpure_start(init_param.output_stream, init_param.sample_rate, 2);
#endif

    if(init_param.support_tws) {
        media_player_set_sync_play_time(tts_ctx->player_handle, tts_item->bt_clk);
    }

	media_player_play(tts_ctx->player_handle);

exit:
	return ret;
}

int tts_manager_play_process(void)
{
	sys_snode_t *head;
	struct tts_item_t *tts_item;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
    char tele_name[14];
    int res = 0;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
	if (sys_slist_is_empty(&tts_ctx->tts_list)) {
        res = -ENOENT;
		goto exit;
	}

	SYS_LOG_INF("tts_curr %p, locked: %d\n", tts_ctx->tts_curr, tts_ctx->tts_manager_locked);
	if (tts_ctx->tts_curr || tts_ctx->tts_manager_locked) {
        res = -ENOLCK;
		goto exit;
	}

	head  = sys_slist_peek_head(&tts_ctx->tts_list);
	tts_item = CONTAINER_OF(head, struct tts_item_t, node);
	sys_slist_find_and_remove(&tts_ctx->tts_list, head);
	tts_ctx->tts_item_num--;

	if (tts_ctx->lisener) {
		tts_ctx->lisener(&tts_item->tts_file_name[0], TTS_EVENT_START_PLAY);
	}

    if(tts_item->tts_mode & TTS_MODE_TELE_NUM) {
        sprintf(tele_name, tts_item->pattern, tts_item->tts_file_name[tts_item->tele_num_index++]);
        tts_ctx->tts_stream = _tts_create_stream(tele_name, true, tts_item->tts_mode);
    } else {
        tts_ctx->tts_stream = _tts_create_stream(tts_item->tts_file_name, false, tts_item->tts_mode );
    }

	if (!tts_ctx->tts_stream) {
        res = -EEXIST;
        goto ERROUT;
	}

    //fg app should forbid play media between TTS_EVENT_START_PLAY and TTS_EVENT_START_STOP
	_tts_event_nodify(TTS_EVENT_START_PLAY, 1);

    if(tts_item->tts_mode & TTS_MODE_TELE_NUM) {
    	_tts_start_play(tts_item, tele_name);
    } else {
        _tts_start_play(tts_item, tts_item->tts_file_name);
    }

exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);
    return res;

ERROUT:
    _tts_remove_current_node(tts_ctx, tts_item, false);
    _tts_manager_triggle_play_tts();
	os_mutex_unlock(&tts_ctx->tts_mutex);
    return res;
}

static void _tts_manager_stop_work(os_work *work)
{

	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	_tts_remove_current_node(tts_ctx, tts_ctx->tts_curr, false);

	os_mutex_unlock(&tts_ctx->tts_mutex);

	_tts_manager_triggle_play_tts();
}

static struct tts_item_t *_tts_create_item(uint8_t *tts_name, uint32_t mode, uint64_t bt_clk, char *pattern)
{
	struct tts_item_t *item;
    int size;

    size = sizeof(struct tts_item_t) + strlen(tts_name) + 1 + (pattern ? (strlen(pattern) + 1) : 0);
    item = mem_malloc(size);
	if (!item) {
		goto exit;
	}

	memset(item, 0, size);
    item->bt_clk = bt_clk;
	item->tts_mode = mode;
	strcpy(item->tts_file_name, tts_name);

    if(pattern) {
        item->pattern = (char*)item + sizeof(struct tts_item_t) + strlen(tts_name) + 1;
        strcpy(item->pattern, pattern);
    }

exit:
	return item;
}

void tts_manager_unlock(void)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_manager_locked--;
	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
	os_mutex_unlock(&tts_ctx->tts_mutex);
}

void tts_manager_lock(void)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_manager_locked++;
	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
	os_mutex_unlock(&tts_ctx->tts_mutex);
}

bool tts_manager_is_locked(void)
{
	bool result = false;

	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	result = (tts_ctx->tts_manager_locked > 0);

	SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return result;
}

void tts_manager_wait_finished(bool clean_all)
{
	int try_cnt = 10000;
	struct tts_item_t *temp, *tts_item;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	/**avoid tts wait finished death loop*/
	while (tts_ctx->tts_curr && try_cnt-- > 0) {
		os_sleep(4);
	}

	if (try_cnt == 0) {
		SYS_LOG_WRN("timeout\n");
	}

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (!sys_slist_is_empty(&tts_ctx->tts_list) && clean_all) {
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tts_ctx->tts_list, tts_item, temp, node) {
			if (tts_item) {
				sys_slist_find_and_remove(&tts_ctx->tts_list, &tts_item->node);
				mem_free(tts_item);
			}
		}
		tts_ctx->tts_item_num = 0;

	}

	os_mutex_unlock(&tts_ctx->tts_mutex);
}

int tts_manager_play_with_time(uint8_t *tts_name, uint32_t mode, uint64_t bt_clk, char *pattern)
{
	int res = 0;
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	struct tts_item_t *item = NULL;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (tts_ctx->tts_manager_locked > 0) {
		SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
		res = -ENOLCK;
		goto exit;
	}

	if (strstr(tts_name, ".pcm")) {
	#ifdef CONFIG_PLAY_KEYTONE
		key_tone_play(tts_name);
	#endif
		goto exit;
	}

	if (tts_ctx->tts_item_num >= 3) {
		sys_snode_t *head = sys_slist_peek_head(&tts_ctx->tts_list);
		item = CONTAINER_OF(head, struct tts_item_t, node);
		sys_slist_find_and_remove(&tts_ctx->tts_list, head);
		tts_ctx->tts_item_num--;
		SYS_LOG_ERR("drop: tts  %s\n", &item->tts_file_name[0]);
		mem_free(item);
	}

	item = _tts_create_item(tts_name, mode, bt_clk, pattern);
	if (!item) {
		res = -ENOMEM;
		goto exit;
	}

	sys_slist_append(&tts_ctx->tts_list, (sys_snode_t *)item);
	tts_ctx->tts_item_num++;

exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);

    if((res == 0) && !tts_ctx->tts_curr) {
        if(mode & PLAY_IMMEDIATELY) {
            res = tts_manager_play_process();
        } else {
            _tts_manager_triggle_play_tts();
        }
    }
	return res;
}

static bool block_tts_finised = false;
static void _tts_manager_play_block_event_notify(uint32_t event, void *data, uint32_t len, void *user_data)
{
	switch (event) {
	case PLAYBACK_EVENT_STOP_COMPLETE:
		block_tts_finised = true;
		break;
	}
}

int tts_manager_play_block(uint8_t *tts_name)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	int ret = 0;
	media_init_param_t init_param;
	io_stream_t stream = NULL;
	struct buffer_t buffer;
	void *player_handle = NULL;
	int try_cnt = 0;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);
    if (tts_ctx->tts_manager_locked > 0) {
		SYS_LOG_INF("%d\n",tts_ctx->tts_manager_locked);
		ret = -ENOLCK;
		goto exit;
	}

	if (sd_fmap(tts_name, (void **)&buffer.base, &buffer.length) != 0) {
		goto exit;
	}

	buffer.cache_size = 0;
	stream = buffer_stream_create(&buffer);
	if (!stream) {
		SYS_LOG_ERR("create failed\n");
		goto exit;
	}

	if (stream_open(stream, MODE_IN) != 0) {
		SYS_LOG_ERR("open failed\n");
		stream_destroy(stream);
		stream = NULL;
		goto exit;
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

    if(strstr(tts_name, ".act")) {
        init_param.format = ACT_TYPE;
    } else {
        init_param.format = MP3_TYPE;
    }

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_TTS;
	init_param.efx_stream_type = AUDIO_STREAM_TTS;
	init_param.sample_rate = 16;
    init_param.sample_bits = 16;
    init_param.channels = 1;
	init_param.input_stream = stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = _tts_manager_play_block_event_notify;

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_pre_start();
#endif
	player_handle = media_player_open(&init_param);
	if (!player_handle) {
		goto exit;
	}
#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_start(init_param.output_stream, init_param.sample_rate, 2);
#endif
	block_tts_finised = false;

	media_player_play(player_handle);

	while (!block_tts_finised && try_cnt ++ < 100) {
		os_sleep(100);
	}
#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_stop();
#endif
	media_player_stop(player_handle);
	media_player_close(player_handle);

exit:
	if (stream) {
		stream_close(stream);
		stream_destroy(stream);
	}
	os_mutex_unlock(&tts_ctx->tts_mutex);
	return ret;
}

int tts_manager_stop(uint8_t *tts_name)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	struct tts_item_t *tts_item;

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	if (!tts_ctx->tts_curr)
		goto exit;

	tts_item = tts_ctx->tts_curr;

	if (tts_item->tts_mode == UNBLOCK_UNINTERRUPTABLE
		|| tts_item->tts_mode == BLOCK_UNINTERRUPTABLE) {
		goto exit;
	}

	_tts_remove_current_node(tts_ctx, tts_item, true);

exit:
	os_mutex_unlock(&tts_ctx->tts_mutex);

	_tts_manager_triggle_play_tts();

	return 0;
}

int tts_manager_register_tts_config(const struct tts_config_t *config)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->tts_config = config;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

int tts_manager_add_event_lisener(tts_event_nodify lisener)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->lisener = lisener;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

int tts_manager_remove_event_lisener(tts_event_nodify lisener)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

	tts_ctx->lisener = NULL;

	os_mutex_unlock(&tts_ctx->tts_mutex);

	return 0;
}

const struct tts_config_t tts_config = {
	.tts_storage_media = TTS_SOTRAGE_SDFS,
};

int tts_manager_process_ui_event(int ui_event)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	uint8_t *tts_file_name = NULL;
	int mode = 0;

	if (!tts_ctx->tts_config || !tts_ctx->tts_config->tts_map)
		return -ENOENT;

	for (int i = 0; i < tts_ctx->tts_config->tts_map_size; i++) {
		const tts_ui_event_map_t *tts_ui_event_map = &tts_ctx->tts_config->tts_map[i];

		if (tts_ui_event_map->ui_event == ui_event) {
			tts_file_name = (uint8_t *)tts_ui_event_map->tts_file_name;
			mode = tts_ui_event_map->mode;
			break;
		}

	}
	SYS_LOG_INF(" %d %s \n",ui_event,tts_file_name);
	if (tts_file_name)
		tts_manager_play(tts_file_name, mode);

	return 0;
}

#ifdef CONFIG_PLAY_TIPTONE
static void _tip_manager_stop(struct tts_manager_ctx_t *tts_ctx, bool fadeout)
{
    tts_ctx->tip_file_name[0] = 0;
	if (tts_ctx->tip_player_handle) {
        if(fadeout) {
            media_player_fade_out(tts_ctx->tip_player_handle, 10);
            os_sleep(10);
        }
#ifdef CONFIG_BT_TRANSMIT
		bt_transmit_catpure_stop();
#endif
		media_player_stop(tts_ctx->tip_player_handle);
		media_player_close(tts_ctx->tip_player_handle);
		tts_ctx->tip_player_handle = NULL;
	}

	if (tts_ctx->tip_stream) {
		stream_close(tts_ctx->tip_stream);
		stream_destroy(tts_ctx->tip_stream);
		tts_ctx->tip_stream = NULL;
	}
}

static void _tip_manager_stop_work(os_work *work)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tip_mutex, OS_FOREVER);

    if(tts_ctx->tip_seq_stopped == tts_ctx->tip_seq)
    {
        SYS_LOG_INF("tip stopped, tip_seq: %u, name: %s\n", tts_ctx->tip_seq, tts_ctx->tip_file_name);
    	_tip_manager_stop(tts_ctx, false);
    }
    else
    {
        SYS_LOG_INF("no cur tip stopped, tip_seq_stopped: %u, tip_seq: %u\n", tts_ctx->tip_seq_stopped, tts_ctx->tip_seq);
    }

	os_mutex_unlock(&tts_ctx->tip_mutex);
}

static void _tip_manager_play_event_notify(uint32_t event, void *data, uint32_t len, void *user_data)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	switch (event) {
	case PLAYBACK_EVENT_STOP_COMPLETE:
        tts_ctx->tip_seq_stopped = (uint32_t)user_data;
        SYS_LOG_INF("tip_seq_stopped: %u\n", tts_ctx->tip_seq_stopped);
	#ifdef CONFIG_USER_WORK_Q
		os_delayed_work_submit_to_queue(os_get_user_work_queue(),&tts_ctx->tip_stop_work, OS_NO_WAIT);
	#else
		os_delayed_work_submit(&tts_ctx->tip_stop_work, OS_NO_WAIT);
	#endif
		break;
	}
}

int tip_manager_play(uint8_t *tip_name, uint64_t bt_clk)
{
	struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
	int ret = 0;
	media_init_param_t init_param;

    tip_manager_stop();

    os_mutex_lock(&tts_ctx->tip_mutex, OS_FOREVER);
    if(strlen(tip_name) >= 12){
        SYS_LOG_INF("tip name too long: %s\n", tip_name);
        ret = -1;
        goto exit;
    }

	tts_ctx->tip_stream = _tts_create_stream(tip_name, false, 0);
	if (!tts_ctx->tip_stream) {
        ret = -1;
		goto exit;
	}

    tts_ctx->tip_seq++;
    if(tts_ctx->tip_seq == 0)
        tts_ctx->tip_seq = 1;

    strcpy(tts_ctx->tip_file_name, tip_name);

	memset(&init_param, 0, sizeof(media_init_param_t));
    if(strstr(tip_name, ".act")) {
        init_param.format = ACT_TYPE;
        init_param.sample_rate = 16;
    } else {
        init_param.format = PCM_TYPE;
        init_param.sample_rate = 8;
    }

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK;
	init_param.stream_type = AUDIO_STREAM_TIP;
	init_param.efx_stream_type = AUDIO_STREAM_TIP;
    init_param.sample_bits = 16;
    init_param.channels = 1;
	init_param.input_stream = tts_ctx->tip_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = _tip_manager_play_event_notify;
	uint32_t tmp_tip_seq = tts_ctx->tip_seq;
    init_param.user_data = (void*)tmp_tip_seq;
    init_param.support_tws = (bt_clk == UINT64_MAX) ? 0:1;

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_pre_start();
#endif
	tts_ctx->tip_player_handle = media_player_open(&init_param);
	if (!tts_ctx->tip_player_handle) {
        ret = -1;
		_tip_manager_stop(tts_ctx, false);
		goto exit;
	}
#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_start(init_param.output_stream, init_param.sample_rate, 2);
#endif
    if(init_param.support_tws) {
        media_player_set_sync_play_time(tts_ctx->tip_player_handle, bt_clk);
    }

	media_player_play(tts_ctx->tip_player_handle);

exit:
    os_mutex_unlock(&tts_ctx->tip_mutex);
	return ret;
}

int tip_manager_stop(void)
{
    struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

	os_mutex_lock(&tts_ctx->tip_mutex, OS_FOREVER);

	_tip_manager_stop(tts_ctx, true);

	os_mutex_unlock(&tts_ctx->tip_mutex);
    return 0;
}

const char* tip_manager_get_playing_filename(void)
{
    struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();
    return tts_ctx->tip_file_name;
}
#endif // CONFIG_PLAY_TIPTONE

#ifdef CONFIG_TTS_MFILE_STREAM
/** buffer info ,used by buffer stream */
typedef struct
{
	/** len of buffer*/
	int length;
	/** pointer to base of buffer */
	char *buffer_base;
    /** mutex used for sync*/
	os_mutex lock;
}mfile_info_t;

static int mfile_stream_open(io_stream_t handle, stream_mode mode)
{
	handle->mode = mode;
	SYS_LOG_INF("mode %d \n",mode);
	return 0;
}

static int mfile_stream_read(io_stream_t handle, unsigned char * buf, int num)
{
	int brw = 0;
	int read_len = 0;
	mfile_info_t *info = (mfile_info_t *)handle->data;

	brw = os_mutex_lock(&info->lock, OS_FOREVER);
	if (brw < 0) {
		SYS_LOG_ERR("lock failed %d \n",brw);
		return -brw;
	}
	if(handle->state != STATE_OPEN) {
		goto err_out;
	}

#ifdef CONFIG_SD_FS
    if (handle->rofs  >= info->length) {
        struct buffer_t buffer;
        struct tts_manager_ctx_t *tts_ctx = _tts_get_ctx();

        //os_mutex_lock(&tts_ctx->tts_mutex, OS_FOREVER);

        while (tts_ctx->tts_curr->tts_file_name[tts_ctx->tts_curr->tele_num_index] != 0) {
            char tele_name[14];

            sprintf(tele_name, tts_ctx->tts_curr->pattern,
                tts_ctx->tts_curr->tts_file_name[tts_ctx->tts_curr->tele_num_index++]);

            if (sd_fmap(tele_name, (void **)&buffer.base, &buffer.length) != 0) {
                SYS_LOG_ERR("file %s no exist\n", tele_name);
                break;
            }

            info->buffer_base = buffer.base;
            info->length = buffer.length;
            handle->total_size = buffer.length;
            //skip 2 bytes of header
#ifdef CONFIG_DECODER_ACT_HW_ACCELERATION
            handle->rofs = 0;
#else
            handle->rofs = 2;
#endif
            printk("play next tts %s\n", tele_name);
            break;
        }

        //os_mutex_unlock(&tts_ctx->tts_mutex);
	}
#endif

	if (handle->rofs  +  num > info->length) {
		read_len = info->length - handle->rofs;
	} else {
		read_len = num;
	}

	memcpy(buf, info->buffer_base + handle->rofs, read_len);
	handle->rofs += read_len;

err_out:
	os_mutex_unlock(&info->lock);
	return read_len;
}

static int mfile_stream_close(io_stream_t handle)
{
	int res;

	mfile_info_t *info = (mfile_info_t *)handle->data;

	res = os_mutex_lock(&info->lock, OS_FOREVER);
	if (res < 0) {
		SYS_LOG_ERR("lock failed %d \n",res);
		return res;
	}

	handle->wofs = 0;
	handle->rofs = 0;
	handle->state = STATE_CLOSE;

	os_mutex_unlock(&info->lock);
	return res;

}

static int mfile_stream_destory(io_stream_t handle)
{
	mfile_info_t *info = (mfile_info_t *)handle->data;

	mem_free(info);
	handle->data = NULL;
	return 0;
}

static int mfile_stream_init(io_stream_t handle, void *param)
{
	mfile_info_t *info = NULL;
	struct buffer_t *buffer = (struct buffer_t *)param;

	info = mem_malloc(sizeof(mfile_info_t));
	if (!info) {
		SYS_LOG_ERR(" malloc failed \n");
		return -ENOMEM;
	}

	info->buffer_base = buffer->base;
	info->length = buffer->length;

	os_mutex_init(&info->lock);

	handle->data = info;
	handle->cache_size = buffer->cache_size;
	handle->total_size = buffer->length;
	return 0;
}

static const stream_ops_t mfile_stream_ops = {
	.init = mfile_stream_init,
	.open = mfile_stream_open,
    .read = mfile_stream_read,
    .seek = NULL,
    .tell = NULL,
    .write = NULL,
    .close = mfile_stream_close,
	.destroy = mfile_stream_destory,
};

static io_stream_t mfile_stream_create(struct buffer_t *param)
{
	return stream_create(&mfile_stream_ops, param);
}
#endif

static struct tts_manager_ctx_t global_tts_manager_ctx;

int tts_manager_init(void)
{
	tts_manager_ctx = &global_tts_manager_ctx;

	memset(tts_manager_ctx, 0, sizeof(struct tts_manager_ctx_t));

	sys_slist_init(&tts_manager_ctx->tts_list);

	os_mutex_init(&tts_manager_ctx->tts_mutex);

#ifdef CONFIG_PLAY_TIPTONE
    os_mutex_init(&tts_manager_ctx->tip_mutex);
#endif
	os_delayed_work_init(&tts_manager_ctx->stop_work, _tts_manager_stop_work);
#ifdef CONFIG_PLAY_TIPTONE
    os_delayed_work_init(&tts_manager_ctx->tip_stop_work, _tip_manager_stop_work);
#endif

	tts_manager_register_tts_config(&tts_config);

#ifdef CONFIG_PLAY_KEYTONE
	key_tone_manager_init();
#endif

	return 0;
}

int tts_manager_deinit(void)
{
	tts_manager_wait_finished(true);

	os_delayed_work_cancel(&tts_manager_ctx->stop_work);
#ifdef CONFIG_PLAY_TIPTONE
    os_delayed_work_cancel(&tts_manager_ctx->tip_stop_work);
#endif
	return 0;
}


