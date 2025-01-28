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
#include "bluetooth_tws_observer.h"
#endif

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "audio_aps"

extern void audio_aps_monitor_set_aps(void *audio_handle, uint8_t status, int level);
extern uint32_t get_sample_rate_hz(uint8_t fs_khz);

static int32_t _set_start_pkt_num(void *aps_monitor, uint16_t pkt_num)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	handle->first_pkt_num = pkt_num;
    return 0;
}

static int32_t _start_playback(void *aps_monitor)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	SYS_LOG_INF(" ok ");
	return audio_track_start(handle->audio_track);
}

static int32_t _set_base_aps_level(void *aps_monitor, uint8_t level)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	SYS_LOG_INF(" ok ");
    hal_aout_channel_set_aps(audio_track->audio_handle, handle->current_level, APS_LEVEL_AUDIOPLL);
    return 0;
}

static int32_t _notify_time_diff(void *aps_monitor, int32_t diff_time)
{
	aps_monitor_info_t *handle = (aps_monitor_info_t *)aps_monitor;

	if (!handle || !handle->audio_track)
		return 0;

	SYS_LOG_INF(" ok ");
    return 0;
}

static media_observer_t audio_observer = {
	.media_handle    = NULL,
	.set_start_pkt_num   = _set_start_pkt_num,
	.start_playback   = _start_playback,
	.set_base_aps_level   = _set_base_aps_level,
	.notify_time_diff   = _notify_time_diff,
};

void audio_aps_tws_notify_decode_err(uint16_t err_cnt)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    bt_tws_observer_t *observer = (bt_tws_observer_t*)handle->tws_observer;

    //todo: restart playback
}

/* adjust_mode: 高4bit表示场景: 0为播歌, 1为通话, 低4bit表示播放方式: 0为normal, 1为TWS
   因为通话的包长是固定的, 所以通话的时候stream_length表示缓存的字节数, aps_threshold也
   需要转换成字节数
 */
static int32_t player_monitor_aout_rate(player_commonctx_t *commonctx, uint32_t stream_length, uint8_t adjust_mode)
{
    uint8_t trim_flag = 0;
    uint32_t adjust_interval = 3000;
    static uint32_t debug_time = 0;
    static uint32_t adjust_time = 0;
    static int32_t check_count = 0;
    uint32_t trim_threshold = 0, max_threshold;
    int32_t min_level, max_level;
    int32_t adj_level, cur_level, max_check_count;

    if(commonctx->aout_aps_level >= APS_48KHZ_LEVEL_1)
    {
        min_level = APS_48KHZ_LEVEL_1;
        max_level = APS_48KHZ_LEVEL_8;
    }
    else
    {
        min_level = APS_44KHZ_LEVEL_1;
        max_level = APS_44KHZ_LEVEL_8;
    }

    if((commonctx->aout_sample_rate == SAMPLE_16KHZ) || (commonctx->aout_sample_rate == SAMPLE_8KHZ))
    {
        if((adjust_mode & 0x0f) == 0x01)
        {
            //TWS
            max_check_count = 50;
        }
        else
        {
            //NORMAL
            max_check_count = 2;
        }

        max_threshold = commonctx->aout_aps_threshold;
        trim_threshold = 250;
    }
    else
    {
        if((adjust_mode & 0x0f) == 0x01)
        {
            //TWS
            max_check_count = 100;
        }
        else
        {
            //NORMAL
            max_check_count = 2;
        }

        max_threshold = commonctx->aout_aps_threshold;
        trim_threshold = 2000;
    }

    /* TWS场景需要预留最大和最小LEVEL做缓冲区调节
     */
    if((adjust_mode & 0x0f) == 0x01)
    {
        min_level ++;
        max_level --;
    }
    else
    {
        ;
    }

    cur_level = aout_get_aps(commonctx->aout_channel);
    adj_level = cur_level;

    /* 微调
     */
    if(abs((int32_t)(stream_length - max_threshold)) <= trim_threshold)
    {
        if((cur_level >= (commonctx->aout_aps_level - 1)) && (cur_level <= (commonctx->aout_aps_level + 1)))
        {
            min_level = commonctx->aout_aps_level - 1;
            max_level = commonctx->aout_aps_level + 1;

            trim_flag = 1;
        }
    }

    if(stream_length > max_threshold)
    {
        check_count ++;
    }
    else if(stream_length < max_threshold)
    {
        check_count --;
    }

    if(check_count >= max_check_count)
    {
        check_count = 0;
        adj_level ++;
    }
    else if(check_count <= -max_check_count)
    {
        check_count = 0;
        adj_level --;
    }
    else
    {
        if(stream_length > max_threshold)
        {
            adj_level ++;
        }
        else if(stream_length < max_threshold)
        {
            adj_level --;
        }

        if(((uint32_t)jiffies_to_msecs(jiffies) - adjust_time) < adjust_interval)
        {
            /* 设置最小等待时间避免TWS场景调节失败
             */
            if(((uint32_t)jiffies_to_msecs(jiffies) - adjust_time) < 200)
            {
                return -1;
            }

            if(trim_flag == 1)
            {
                return -1;
            }

            /* 快速调节
             */
            if(((cur_level == max_level) && (adj_level < max_level)) ||
                ((cur_level == min_level) && (adj_level > min_level)))
            {
                check_count = 0;
                sys_printf("aout aps6:%d_%d_%d\n", cur_level, adj_level, stream_length);
            }
            else
            {
                return -1;
            }
        }
    }

    if(adj_level > max_level)
    {
        adj_level = max_level;
    }

    if(adj_level < min_level)
    {
        adj_level = min_level;
    }

    adjust_time = (uint32_t)jiffies_to_msecs(jiffies);

    if(adj_level != cur_level)
    {
        /* 上升趋势调节
         */
        if((cur_level < commonctx->aout_aps_level) && (adj_level > cur_level))
        {
            adj_level = commonctx->aout_aps_level;
        }

        /* 下降趋势调节
         */
        if((cur_level > commonctx->aout_aps_level) && (adj_level < cur_level))
        {
            adj_level = commonctx->aout_aps_level;
        }

        if(((uint32_t)jiffies_to_msecs(jiffies) - debug_time) > 3000)
        {
            debug_time = (uint32_t)jiffies_to_msecs(jiffies);
            sys_printf("aout aps1: %d_%d_%d_%d_%d_%d\n", \
            cur_level, \
            adj_level, \
            commonctx->aout_aps_level, \
            stream_length, \
            max_threshold, \
            commonctx->aout_aps_threshold);

            sys_printf("aout aps2: %d_%d_%d_%d\n", \
            min_level, \
            max_level, \
            check_count, \
            max_check_count);
        }

        return adj_level;
    }
    return -1;
}

void audio_aps_monitor_master(aps_monitor_info_t *handle, int32_t stream_length, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t aps_level)
{
	bt_tws_observer_t *tws_observer = (bt_tws_observer_t *)handle->tws_observer;
	struct audio_track_t *audio_track = handle->audio_track;
	uint16_t mid_threshold = 0;
	uint16_t diff_threshold = 0;
	int local_compensate_samples;
	int remote_compensate_samples;

	aps_max_level = aps_max_level - 1;
	aps_min_level = aps_min_level + 1;

	local_compensate_samples = audio_track_get_fill_samples(audio_track);
	tws_observer->aps_change_notify(handle->current_level);
}

void audio_aps_monitor_slave(aps_monitor_info_t *handle, int stream_length, uint8_t aps_max_level, uint8_t aps_min_level, uint8_t slave_aps_level)
{
}

int32_t audio_tws_set_stream_info(uint8_t format, uint16_t first_pktnum, uint8_t sample_rate)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    bt_tws_observer_t *observer = (bt_tws_observer_t*)handle->tws_observer;

    handle->first_pkt_num = 0;
    return observer->set_stream_info(format, first_pktnum, get_sample_rate_hz(sample_rate));
}

uint16_t audio_tws_get_playback_first_pktnum(void)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    bt_tws_observer_t *observer = (bt_tws_observer_t*)handle->tws_observer;

    return handle->first_pkt_num;
}

int32_t audio_tws_set_pkt_info(uint16_t pkt_num, uint16_t pkt_len, uint16_t pcm_len)
{
    aps_monitor_info_t *handle = audio_aps_monitor_get_instance();
    bt_tws_observer_t *observer = (bt_tws_observer_t*)handle->tws_observer;
    tws_pkt_info_t info;
    uint32_t flags;

    info.pkt_num = pkt_num;
    info.pkt_len = pkt_len;
    info.samples = pcm_len / handle->audio_track->frame_size;

    flags = irq_lock();

    info.pkt_bttime_us = 
        handle->audio_track->total_samples_filled
        - hal_aout_channel_get_sample_cnt(handle->audio_track->audio_handle);
    info.pkt_bttime_us = info.pkt_bttime_us * 1000000 / get_sample_rate_hz(handle->audio_track->sample_rate);
    info.pkt_bttime_us += observer->get_bt_clk_us();
    
    irq_unlock(flags);

    SYS_LOG_INF("%d, %d",
        handle->audio_track->total_samples_filled,
        hal_aout_channel_get_sample_cnt(handle->audio_track->audio_handle));

    return observer->set_pkt_info(&info);
}

void audio_aps_monitor_tws_init(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

	if (tws_observer) {
		audio_observer.media_handle = handle;
        audio_observer.tws_observer = tws_observer;
		handle->tws_observer = bluetooth_tws_observer_init(&audio_observer);
		handle->role = BTSRV_TWS_MASTER;
		hal_aout_channel_enable_sample_cnt(handle->audio_track->audio_handle, true);
	}
	audio_aps_monitor_set_aps(handle->audio_track->audio_handle, APS_OPR_FAST_SET, handle->aps_default_level);
}

void audio_aps_monitor_tws_deinit(void *tws_observer)
{
	aps_monitor_info_t *handle = audio_aps_monitor_get_instance();

	if (tws_observer) {
		hal_aout_channel_enable_sample_cnt(handle->audio_track->audio_handle, false);
        bluetooth_tws_observer_deinit(handle->tws_observer);
		handle->tws_observer = NULL;
	}
}

