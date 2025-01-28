/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief audio stream.
*/

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <audio_hal.h>
#include <audio_system.h>
#include <audio_track.h>
#include <media_type.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#ifdef CONFIG_TWS
#include <btservice_api.h>
#endif

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_aps"

extern void audio_aps_monitor_set_aps(void *audio_handle, uint8_t status, int level);
extern void audio_aps_monitor_normal(aps_monitor_info_t *handle, int stream_length,
										uint8_t aps_max_level, uint8_t aps_min_level,
										uint8_t aps_level);

static int _audio_is_ready(void *aps_monitor)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;
	io_stream_t stream = NULL;

	if (!handle || !handle->audio_track)
		return 0;

	stream = audio_track_get_stream(handle->audio_track);
	SYS_LOG_INF("steam_len %d ", stream_get_length(stream));
	if (stream_get_space(stream) <= stream_get_length(stream)) {
		SYS_LOG_INF(" ok ");
		return 1;
	}

	return 0;
}

static int _audio_trigger_start(void *aps_monitor)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	SYS_LOG_INF(" ok ");
	return audio_track_start(handle->audio_track);
}

static uint32_t _audio_get_samples_cnt(void *aps_monitor, uint8_t *audio_mode)
{
	uint32_t sample = 0;
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	sample = hal_aout_channel_get_sample_cnt(handle->audio_track->audio_handle);

	if (handle->audio_track->audio_mode == AUDIO_MODE_MONO) {
		*audio_mode = 1;
	} else {
		*audio_mode = 2;
	}

	return sample;
}

static int _audio_get_error_state(void *aps_monitor)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle)
		return 0;

	return hal_aout_channel_check_fifo_underflow(handle->audio_track->audio_handle);
}

static int _audio_get_aps_level(void *aps_monitor)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	return handle->current_level;
}

static media_runtime_observer_t audio_observer = {
	.media_handle    = NULL,
	.is_ready        = _audio_is_ready,
	.trigger_start   = _audio_trigger_start,
	.get_samples_cnt = _audio_get_samples_cnt,
	.get_error_state = _audio_get_error_state,
	.get_aps_level   = _audio_get_aps_level,
};

void audio_aps_monitor_tws_init(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	tws_runtime_observer_t *observer = (tws_runtime_observer_t *)tws_observer;

	if (tws_observer) {
		audio_observer.media_handle = handle;
		observer->set_media_observer(&audio_observer);
		handle->tws_observer = tws_observer;
		handle->role = observer->get_role();
		hal_aout_channel_enable_sample_cnt(handle->audio_track->audio_handle, true);
	}
	audio_aps_monitor_set_aps(handle->audio_track->audio_handle, APS_OPR_FAST_SET, handle->aps_default_level);
}


void audio_aps_tws_notify_decode_err(uint16_t err_cnt)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	tws_runtime_observer_t *observer = handle->tws_observer;

	if (observer && observer->trigger_restart) {
		observer->trigger_restart(err_cnt);
	}
}

void audio_aps_monitor_tws_deinit(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
	tws_runtime_observer_t *observer = (tws_runtime_observer_t *)tws_observer;

	if (tws_observer) {
		hal_aout_channel_enable_sample_cnt(handle->audio_track->audio_handle, false);
		observer->set_media_observer(NULL);
		handle->tws_observer = NULL;
	}
}

void audio_aps_monitor_master(aps_monitor_info_t *handle, int stream_length, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t aps_level)
{
	tws_runtime_observer_t *tws_observer = (tws_runtime_observer_t *)handle->tws_observer;
	struct audio_track_t *audio_track = handle->audio_track;
	uint16_t mid_threshold = 0;
	uint16_t diff_threshold = 0;
	int local_compensate_samples;
	int remote_compensate_samples;

	aps_max_level = aps_max_level - 1;
	aps_min_level = aps_min_level + 1;

	local_compensate_samples = audio_track_get_fill_samples(audio_track);

	tws_observer->exchange_samples(&local_compensate_samples, &remote_compensate_samples);

	if (!handle->need_aps) {
		if (tws_observer && tws_observer->aps_nogotiate) {
			/* Not need adjust aps, but need notify tws module if needed */
			handle->dest_level = handle->current_level;
			tws_observer->aps_nogotiate(&handle->dest_level, &handle->current_level);
		}
		return;
	}

	/* printk("---in stream --- %d out stream %d\n",stream_length, stream_tell(info->pcm_stream)); */

	diff_threshold = (handle->aps_increase_water_mark - handle->aps_reduce_water_mark);
	mid_threshold = handle->aps_increase_water_mark - (diff_threshold / 2);

    switch (handle->aps_status) {
	case APS_STATUS_DEFAULT:
	    if (stream_length > handle->aps_increase_water_mark) {
		SYS_LOG_DBG("inc aps\n");
				handle->dest_level = aps_max_level;
		handle->aps_status = APS_STATUS_INC;
	    } else if (stream_length < handle->aps_reduce_water_mark) {
		SYS_LOG_DBG("fast dec aps\n");
				handle->dest_level = aps_min_level;
		handle->aps_status = APS_STATUS_DEC;
	    } else {
		/* keep default */
		handle->dest_level = aps_level;
		handle->aps_status = APS_STATUS_DEFAULT;
	    }
	    break;

	case APS_STATUS_INC:
	    if (stream_length < handle->aps_reduce_water_mark) {
		SYS_LOG_DBG("fast dec aps\n");
				handle->dest_level = aps_min_level;
		handle->aps_status = APS_STATUS_DEC;
	    } else if (stream_length <= mid_threshold) {
		SYS_LOG_DBG("default aps\n");
				handle->dest_level = aps_level;
		handle->aps_status = APS_STATUS_DEFAULT;
	    } else {
		handle->dest_level = aps_max_level;
		handle->aps_status = APS_STATUS_INC;
	    }
	    break;

	case APS_STATUS_DEC:
	    if (stream_length > handle->aps_increase_water_mark) {
		SYS_LOG_DBG("fast inc aps\n");
				handle->dest_level = aps_max_level;
		handle->aps_status = APS_STATUS_INC;
	    } else if (stream_length >= mid_threshold) {
		SYS_LOG_DBG("default aps\n");
				handle->dest_level = aps_level;
		handle->aps_status = APS_STATUS_DEFAULT;
	    } else {
		handle->dest_level = aps_min_level;
		handle->aps_status = APS_STATUS_DEC;
	    }
	    break;
    }

	if (tws_observer && tws_observer->aps_nogotiate) {
		tws_observer->aps_nogotiate(&handle->dest_level, &handle->current_level);
	}

	if (tws_observer && handle->dest_level != handle->current_level) {
		/* Slowly adjust */
		if (handle->dest_level > handle->current_level) {
			handle->current_level++;
		} else if (handle->dest_level < handle->current_level) {
			handle->current_level--;
		}
		/* printk("%s: %d\n", __func__, handle->current_level); */
		tws_observer->aps_change_notify(handle->current_level);
		hal_aout_channel_set_aps(audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
	}
	/* printk("master: stream_length %d aps_level %d dest_level %d \n",stream_length, handle->current_level, handle->dest_level); */
}

void audio_aps_monitor_slave(aps_monitor_info_t *handle, int stream_length, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t slave_aps_level)
{
	tws_runtime_observer_t *tws_observer = (tws_runtime_observer_t *)handle->tws_observer;
	struct audio_track_t *audio_track = NULL;
	int sample_diff = 0;
	uint16_t bt_clock = 0;
	uint8_t master_aps_level = 0;
	int req_aps = slave_aps_level;
	static int monitor_cnt;
	static uint16_t pre_bt_clock;
	int diff_samples = 0;
	int local_compensate_samples;
	int remote_compensate_samples;

	audio_track = handle->audio_track;

	tws_observer->get_samples_diff(&sample_diff, &master_aps_level, &bt_clock);

	if ((pre_bt_clock != bt_clock) || (sample_diff > 20) || (sample_diff < -20)) {
		if (sample_diff < -15) {
			req_aps = master_aps_level - 3;
		} else if (sample_diff < -10) {
			req_aps = master_aps_level - 2;
		} else if (sample_diff < -5) {
			req_aps = master_aps_level - 1;
		} else if (sample_diff > 15) {
			req_aps = master_aps_level + 3;
		} else if (sample_diff > 10) {
			req_aps = master_aps_level + 2;
		} else if (sample_diff > 5) {
			req_aps = master_aps_level + 1;
		} else {
			req_aps = master_aps_level;
		}

		if (req_aps > aps_max_level)
			req_aps = aps_max_level;
		if (req_aps < aps_min_level)
			req_aps = aps_min_level;

		pre_bt_clock = bt_clock;
	} else {
		/* Other time follow master level */
		req_aps = master_aps_level;
	}

	if (slave_aps_level != req_aps) {
		handle->dest_level = req_aps;
		handle->current_level = handle->dest_level;
		/* printk("%s: %d\n", __func__, handle->current_level); */
		hal_aout_channel_set_aps(audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
	}

	/* printk("diff %d ml %d sl %d cl %x\n",sample_diff, master_aps_level, handle->current_level, bt_clock); */
	if (sample_diff > 50 || sample_diff < -50) {
		if (monitor_cnt++ > 1000) {
			tws_observer->trigger_restart(TWS_RESTART_SAMPLE_DIFF);
			monitor_cnt = 0;
			return;
		}
		/* printk("slave: sample_diff %d master_aps_level %d aps_level %d dest_level %d  stream_length %d\n",sample_diff, master_aps_level, handle->current_level,handle->dest_level, stream_length); */
	} else {
		monitor_cnt = 0;
	}

	local_compensate_samples = audio_track_get_fill_samples(audio_track);

	tws_observer->exchange_samples(&local_compensate_samples, &remote_compensate_samples);

	diff_samples = remote_compensate_samples - local_compensate_samples;

	if (diff_samples) {
		SYS_LOG_INF("diff %d(local %d + remote %d)", diff_samples, local_compensate_samples, remote_compensate_samples);
	}

	audio_track_compensate_samples(audio_track, diff_samples);

	if ((diff_samples > 0x3FFFFFFF) ||
		(diff_samples < (-0x3FFFFFFF))) {
		/* Too larger, restart */
		tws_observer->trigger_restart(TWS_RESTART_SAMPLE_DIFF);
	}


}

int32_t audio_tws_set_stream_info(uint8_t format, uint16_t first_pktnum, uint8_t sample_rate)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

    if(handle->audio_track)
        audio_track_set_waitto_start(handle->audio_track, false);
    return 0;
}

uint16_t audio_tws_get_playback_first_pktnum(void)
{
    return 1;
}

int32_t audio_tws_set_pkt_info(uint16_t pkt_num, uint16_t pkt_len, uint16_t pcm_len)
{
    return 0;
}

