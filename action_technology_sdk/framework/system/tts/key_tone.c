/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file key tone
 */
#define SYS_LOG_DOMAIN "keytone"
#include <os_common_api.h>
#include <audio_system.h>
#include <media_player.h>
#include <buffer_stream.h>
#include <file_stream.h>
#include <app_manager.h>
#include <mem_manager.h>
#include <tts_manager.h>
#include <audio_track.h>
#include <ringbuff_stream.h>
#ifdef CONFIG_SD_FS
#include <sdfs.h>
#endif
struct key_tone_manager_t {
	io_stream_t tone_stream;
	uint8_t key_tone_cnt;
	struct acts_ringbuf *tone_ringbuf;
	struct audio_track_t *keytone_track;
	os_delayed_work play_work;
	os_delayed_work stop_work;
    char key_tone_name[12];
};

static struct key_tone_manager_t key_tone_manager;

static void _key_tone_track_callback(uint8_t event, void *user_data)
{

}

static io_stream_t _key_tone_create_stream(uint8_t *key_tone_file)
{
	int ret = 0;
	io_stream_t input_stream = NULL;
	struct buffer_t buffer;

	if (sd_fmap(key_tone_file, (void **)&buffer.base, &buffer.length) != 0) {
		goto exit;
	}

	input_stream = ringbuff_stream_create_ext(buffer.base, buffer.length);
	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

	stream_write(input_stream, NULL, buffer.length);
    stream_write(input_stream, NULL, 0);

exit:
	SYS_LOG_INF("%p\n", input_stream);
	return	input_stream;
}

static struct acts_ringbuf *_key_tone_create_ringbuff(uint8_t *key_tone_file)
{
	struct acts_ringbuf *ringbuff = NULL;
	struct buffer_t buffer;
#ifdef CONFIG_SD_FS
	if (sd_fmap(key_tone_file, (void **)&buffer.base, &buffer.length) != 0) {
		goto exit;
	}
#endif
	ringbuff = acts_ringbuf_init_ext(buffer.base, buffer.length);
	if (!ringbuff) {
		SYS_LOG_ERR("create failed\n");
		goto exit;
	}

	acts_ringbuf_fill_none(ringbuff, buffer.length);

	SYS_LOG_INF("ringbuff %p\n", ringbuff);
exit:
	return ringbuff;
}

static void _keytone_manager_play_work(os_work *work)
{
	struct key_tone_manager_t *manager = &key_tone_manager;
	struct audio_track_t *keytone_track;

	audio_system_mutex_lock();

	keytone_track = audio_system_get_track();

	/**audio_track already open*/
	if (keytone_track) {
		io_stream_t tone_stream = _key_tone_create_stream(manager->key_tone_name);
		if (!tone_stream) {
			goto exit;
		}
		audio_track_set_mix_stream(keytone_track, tone_stream, 8, 1, AUDIO_STREAM_TTS);
		manager->tone_stream = 	tone_stream;
	} else {
		struct acts_ringbuf *tone_ringbuf = _key_tone_create_ringbuff(manager->key_tone_name);
		if (!tone_ringbuf) {
			goto exit;
		}

		manager->tone_ringbuf = tone_ringbuf;

		keytone_track = audio_track_create(AUDIO_STREAM_TTS, 8,
									 AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO,
									 tone_ringbuf,
									_key_tone_track_callback, manager);

		if (!keytone_track) {
			goto exit;
		}

		audio_track_start(keytone_track);
	}

	manager->keytone_track = keytone_track;

exit:
	audio_system_mutex_unlock();
}

static void _keytone_manager_stop_work(os_work *work)
{
	struct key_tone_manager_t *manager = &key_tone_manager;

	audio_system_mutex_lock();

	if (manager->keytone_track) {
		if (manager->tone_ringbuf) {
			audio_track_flush(manager->keytone_track);
			audio_track_stop(manager->keytone_track);
			audio_track_destory(manager->keytone_track);
			acts_ringbuf_destroy_ext(manager->tone_ringbuf);
			manager->keytone_track = NULL;
		} else if(manager->tone_stream){
			audio_track_set_mix_stream(manager->keytone_track, NULL, 8, 1, AUDIO_STREAM_TTS);
			stream_close(manager->tone_stream);
			stream_destroy(manager->tone_stream);
			manager->keytone_track = NULL;
		}
	}

	manager->tone_stream = NULL;
	manager->keytone_track = NULL;
	manager->tone_ringbuf = NULL;
	manager->key_tone_cnt--;

	if (manager->key_tone_cnt > 0) {
		os_delayed_work_submit(&manager->play_work, OS_NO_WAIT);
		os_delayed_work_submit(&manager->stop_work, 150);
	}

	audio_system_mutex_unlock();
}

int key_tone_play(const char *file_name)
{
	struct key_tone_manager_t *manager = &key_tone_manager;
#ifdef CONFIG_SYS_IRQ_LOCK 
	SYS_IRQ_FLAGS flags;

	sys_irq_lock(&flags);
#endif

    audio_system_mutex_lock();
    strcpy(manager->key_tone_name, file_name);
    audio_system_mutex_unlock();
    
	if (!manager->key_tone_cnt) {
		manager->key_tone_cnt++;
		os_delayed_work_submit(&manager->play_work, OS_NO_WAIT);
		os_delayed_work_submit(&manager->stop_work, 150);
	} else {
		manager->key_tone_cnt++;
	}
#ifdef CONFIG_SYS_IRQ_LOCK 
	sys_irq_unlock(&flags);
#endif
	return 0;
}

int key_tone_manager_init(void)
{
	memset(&key_tone_manager, 0, sizeof(struct key_tone_manager_t));

	os_delayed_work_init(&key_tone_manager.play_work, _keytone_manager_play_work);
	os_delayed_work_init(&key_tone_manager.stop_work, _keytone_manager_stop_work);

	return 0;
}

