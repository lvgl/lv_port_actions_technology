/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arithmetic.h>
#include <audio_system.h>
#include <audio_policy.h>
#include <media_mem.h>
#include <media_player.h>
#include <media_effect_param.h>
#include <volume_manager.h>
#include <app_manager.h>
#include "app_defines.h"
#include "tool_app_inner.h"
#include "utils/acts_ringbuf.h"
#include "audio_track.h"
#include "sys/crc.h"
#include "stream.h"
#include "ringbuff_stream.h"
#include "anc_hal.h"

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define ASQT_DUMP_BUF_SIZE_BTCALL	(1024)
#define ASQT_DUMP_BUF_SIZE_BTMUSIC	(2048)
#define ASQT_MAX_DUMP		(3)

#define MAX_DAE_SIZE (512)
#define DATA_BUFFER_SIZE (520)

#define UPLOAD_TIMER_INTERVAL (4)
#define WAIT_DATA_TIMEOUT (1000)
#define DATA_FRAME_SIZE (512)

#define DUMP_LOG (0)

typedef struct
{
    uint16_t  channel;
    uint16_t  sample_rate;
    uint32_t  sequence;
} audio_asqt_frame_info_t;

typedef struct
{
    usp_packet_head_t head;
    uint16_t payload_crc;
    audio_asqt_frame_info_t info;
} audio_asqt_upload_packet_t;

static uint8_t asqt_tag_table[7] = {
    /* playback tags */
    MEDIA_DATA_TAG_DECODE_OUT1,
    MEDIA_DATA_TAG_PLC,

    /* capture tags */
    MEDIA_DATA_TAG_REF,
    MEDIA_DATA_TAG_MIC1,
    MEDIA_DATA_TAG_AEC1,
    MEDIA_DATA_TAG_AEC2,
    MEDIA_DATA_TAG_ENCODE_IN1,
};

#define ASQT_PKT_SIZE (sizeof(audio_asqt_upload_packet_t) + DATA_FRAME_SIZE)

typedef struct  {
	/* asqt para */
	//char dae_para[MAX_DAE_SIZE];
    char data_buffer[DATA_BUFFER_SIZE]; //download buffer
    char asqt_pkt_buffer[ASQT_PKT_SIZE]; //upload buffer
    os_timer upload_timer;
    uint32_t sequence[ARRAY_SIZE(asqt_tag_table)];

	/* asqt dump tag select */
    uint32_t asqt_channels;
    uint8_t asqt_tobe_start;
    uint8_t asqt_started;
    uint8_t asqt_mode;  //0: dump call data, 1: dump music data, others: reserved
    uint16_t empty_time;
    uint16_t sample_rate;
    uint16_t frame_size;
	io_stream_t dump_streams[ARRAY_SIZE(asqt_tag_table)];
    struct acts_ringbuf *dump_bufs[ARRAY_SIZE(asqt_tag_table)];
	uint8_t has_effect_set;
}asqt_data_t;

typedef struct
{
    uint32_t  cfg_id:8;
    uint32_t  sub_pos:12;
    uint32_t  cfg_len:12;
} app_config_item_info_t;

typedef struct
{
    uint8_t   format[4];
    uint8_t   magic [4];
    uint16_t  user_version;
    uint8_t   minor_version;
    uint8_t   major_version;
    uint16_t  total_size;
    uint16_t  num_cfg_items;
	app_config_item_info_t item_info[3];
	uint8_t music_param_arrary[1024];
	uint8_t voice_param_arrary[512];
	uint8_t aec_param_arrary[296];
} dae_parame_header_t;

static asqt_data_t *asqt_data = NULL;
static u8_t dae_init = 1;
static dae_parame_header_t dae_para;

extern uint32_t get_sample_rate_hz(uint8_t fs_khz);
static void audio_manager_asqt_start(uint32_t asqt_mode, uint32_t asqt_channels);
extern void _restart_player_sync(uint8_t efx_stream_type, bool force);

#if DUMP_LOG
static int print_hex2(const char* prefix, const void* data, size_t size)
{
    char  sbuf[64];

    int  i = 0;
    int  j = 0;

    if (prefix != NULL)
    {
        while ((sbuf[j] = prefix[j]) != '\0' && (j + 6) < sizeof(sbuf))
        {
            j++;
        }
    }

    for (i = 0; i < size; i++)
    {
        char  c1 = ((u8_t*)data)[i] >> 4;
        char  c2 = ((u8_t*)data)[i] & 0x0F;

        if (c1 >= 10)
        {
            c1 += 'a' - 10;
        }
        else
        {
            c1 += '0';
        }

        if (c2 >= 10)
        {
            c2 += 'a' - 10;
        }
        else
        {
            c2 += '0';
        }

        sbuf[j++] = ' ';
        sbuf[j++] = c1;
        sbuf[j++] = c2;

        if (((i + 1) % 16) == 0)
        {
            //sbuf[j++] = '\n';
        }

        if (((i + 1) % 16) == 0 ||
            (j + 6) >= sizeof(sbuf))
        {
            sbuf[j] = '\0';
            j = 0;
            os_printk("%s\n",sbuf);
        }
    }

    //if ((i % 16) != 0)
    {
        sbuf[j++] = '\n';
    }

    if (j > 0)
    {
        sbuf[j] = '\0';
        j = 0;
        os_printk("%s\n",sbuf);
    }

    return i;
}
#endif

static void _asqt_unprepare_data_upload(asqt_data_t *data)
{
    data->asqt_tobe_start = 0;
    if(!data->asqt_started)
        return;

    SYS_LOG_INF("stop");
    data->asqt_started = 0;
    data->empty_time = 0;
    os_timer_stop(&data->upload_timer);
    media_player_dump_data(NULL, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, NULL);

	for (int32_t i = 0; i < ARRAY_SIZE(data->dump_streams); i++) {
		if (data->dump_streams[i]) {
			stream_close(data->dump_streams[i]);
            stream_destroy(data->dump_streams[i]);
			data->dump_streams[i] = NULL;
            data->dump_bufs[i] = NULL;
		}
	}
}

static int32_t _asqt_prepare_data_upload(asqt_data_t *data)
{
	char *dumpbuf;
	int32_t num_tags = 0;
    int32_t buffer_size;
    int32_t dump_size;
    int32_t tmp;
    struct audio_track_t *audio_track;

	media_player_t *player = media_player_get_current_dumpable_player();
	if (!player)
		return -EFAULT;

    // mode 0 to dump call data, mode 1 to dump music data, and others are reserved
    if ((data->asqt_mode == 0 && player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
     || (data->asqt_mode == 1 && player->type != MEDIA_PLAYER_TYPE_PLAYBACK)
     || (data->asqt_mode > 1))
        return -EPERM;

    audio_system_mutex_lock();
	audio_track = audio_system_get_track();
    if(audio_track) {
        data->sample_rate = get_sample_rate_hz(audio_track_get_samplerate(audio_track));
        if(data->sample_rate > 16000)
            data->sample_rate = 16000;
        if(data->sample_rate == 16000) {
            data->frame_size = DATA_FRAME_SIZE;
        } else {
            data->frame_size = DATA_FRAME_SIZE/2;
        }
    }
    audio_system_mutex_unlock();

    if(data->sample_rate == 0)
        return -EFAULT;

    if(player->type == MEDIA_PLAYER_TYPE_PLAYBACK) {
        asqt_tag_table[0] = MEDIA_DATA_TAG_DECODE_OUT1;
        asqt_tag_table[1] = MEDIA_DATA_TAG_DECODE_IN;
        dump_size = ASQT_DUMP_BUF_SIZE_BTMUSIC;

        dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
        buffer_size = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
    } else {
        asqt_tag_table[0] = MEDIA_DATA_TAG_DECODE_OUT1;
        asqt_tag_table[1] = MEDIA_DATA_TAG_PLC;
        dump_size = ASQT_DUMP_BUF_SIZE_BTCALL;

        dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);
        buffer_size = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_VOICE);
    }

    data->asqt_started = 1;
    data->empty_time = 0;

	for (int32_t i = 0; i < ARRAY_SIZE(asqt_tag_table); i++) {
		if ((data->asqt_channels & (1 << i)) == 0) /* selected or not */
			continue;

		if (num_tags++ >= ASQT_MAX_DUMP) {
			SYS_LOG_WRN("exceed max %d tags", ASQT_MAX_DUMP);
			break;
		}
        if(buffer_size < data->frame_size)
            break;

        tmp = buffer_size;
        if(tmp > dump_size)
            tmp = dump_size;

		/* update to data tag */
		data->dump_streams[i] = ringbuff_stream_create_ext(dumpbuf, tmp);
		if (!data->dump_streams[i])
			goto fail;

		dumpbuf += tmp;
        buffer_size -= tmp;
        data->dump_bufs[i] = stream_get_ringbuffer(data->dump_streams[i]);
		SYS_LOG_INF("start idx=%d, tag=%u, buf=%p", i, asqt_tag_table[i], data->dump_bufs[i]);
	}

    os_timer_start(&data->upload_timer, K_MSEC(UPLOAD_TIMER_INTERVAL), K_MSEC(UPLOAD_TIMER_INTERVAL));
	return media_player_dump_data(player, ARRAY_SIZE(asqt_tag_table), asqt_tag_table, data->dump_bufs);
fail:
	SYS_LOG_ERR("failed");
	_asqt_unprepare_data_upload(data);
	return -ENOMEM;
}
extern unsigned char crc8_maxim(unsigned char seed, unsigned char *p, unsigned int counter);
static int32_t _asqt_upload_pkt(asqt_data_t *data, int32_t channel, struct acts_ringbuf *rbuf)
{
    audio_asqt_upload_packet_t *packet = (audio_asqt_upload_packet_t*)data->asqt_pkt_buffer;
    char *playload;
    int32_t ret;
    int32_t pkt_size;

    packet->info.channel = channel;
    packet->info.sample_rate = data->sample_rate;
    packet->info.sequence = data->sequence[channel];

    packet->head.type              = USP_PACKET_TYPE_ISO;
    packet->head.sequence_number   = 0;
    packet->head.protocol_type     = USP_PROTOCOL_TYPE_ASET;
    packet->head.payload_len       = sizeof(audio_asqt_frame_info_t) + data->frame_size;
    packet->head.predefine_command = CONFIG_PROTOCOL_CMD_ASQT_FRAME;
    packet->head.crc8_val =
        crc8_maxim(0, (void*)&packet->head, sizeof(usp_packet_head_t) - 1);

    playload = (uint8_t*)packet + sizeof(audio_asqt_upload_packet_t);
    acts_ringbuf_peek(rbuf, playload, data->frame_size);
    //print_hex2("dump:", playload, 20);

    packet->payload_crc =
        crc16_ccitt(0, (void*)&packet->info, packet->head.payload_len);

    pkt_size = sizeof(audio_asqt_upload_packet_t) + data->frame_size;
    ret = g_tool_data.usp_handle.api.write((uint8_t*)packet, pkt_size, 0);
    if(pkt_size != ret) {
        SYS_LOG_INF("send asqt pkt fail\n");
        return -1;
    }

    data->sequence[channel] += 1;
    acts_ringbuf_get(rbuf, NULL, data->frame_size);

    return 0;
}

static void _asqt_upload_data_timer_proc(os_timer *ttimer)
{
    asqt_data_t *data = asqt_data;
	int32_t i, j;
    int32_t max_size;
    int32_t max_tag;
    int32_t cur_size;
    int32_t ret;

    for(j=0; j<5; j++) {
        max_size = 0;
        max_tag = 0;
    	for (i = 0; i < ARRAY_SIZE(asqt_tag_table); i++) {
    		if (!data->dump_bufs[i])
    			continue;

            cur_size = acts_ringbuf_length(data->dump_bufs[i]);
            //printk("dump %d: %d\n", i, cur_size);
    		if (cur_size > max_size) {
    			max_size = cur_size;
                max_tag = i;
    		}
    	}

        if(max_size < data->frame_size) {
            data->empty_time += UPLOAD_TIMER_INTERVAL;
            break;
        }

        data->empty_time = 0;
        ret = _asqt_upload_pkt(data, max_tag, data->dump_bufs[max_tag]);
        if(ret < 0)
            break;
    }
}

/* 调节配置参数
 */
void config_protocol_adjust(uint32_t cfg_id, uint32_t cfg_size, u8_t* cfg_data)
{
    media_player_t *player;

    SYS_LOG_INF("id: %x, size: %d", cfg_id, cfg_size);

    player = media_player_get_current_dumpable_player();
	if (!player)
		return;


    switch (cfg_id)
    {
    case CFG_ID_BT_MUSIC_DAE:
        if(player->type != MEDIA_PLAYER_TYPE_PLAYBACK
			&& player->type != MEDIA_SRV_TYPE_PARSER_AND_PLAYBACK)
            break;
        if(cfg_size == sizeof(CFG_Struct_BT_Music_DAE)) {
            CFG_Struct_BT_Music_DAE *p = (CFG_Struct_BT_Music_DAE*)cfg_data;
            media_player_set_effect_enable(player, p->Enable_DAE);
            system_volume_set(AUDIO_STREAM_MUSIC, p->Test_Volume, false);
        }
        break;
    case CFG_ID_BT_MUSIC_DAE_AL:
        if(player->type != MEDIA_PLAYER_TYPE_PLAYBACK
			&& player->type != MEDIA_SRV_TYPE_PARSER_AND_PLAYBACK)
            break;
#if DUMP_LOG
        print_hex2("music dae:", cfg_data, cfg_size);
#endif
        memcpy(&dae_para.music_param_arrary[0], cfg_data, cfg_size);

        media_player_update_effect_param(player, cfg_data, cfg_size);
        break;

    case CFG_ID_BT_CALL_OUT_DAE:
        if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
            break;
        if(cfg_size == sizeof(CFG_Struct_BT_Call_Out_DAE)) {
            CFG_Struct_BT_Call_Out_DAE *p = (CFG_Struct_BT_Call_Out_DAE*)cfg_data;

            media_player_set_effect_enable(player, p->Enable_DAE);
            system_volume_set(AUDIO_STREAM_VOICE, p->Test_Volume, false);
        }
        break;
	case CFG_ID_BT_CALL_MIC_DAE:
		if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
			break;
		if(cfg_size == sizeof(CFG_Struct_BT_Call_MIC_DAE)) {
			CFG_Struct_BT_Call_MIC_DAE *p = (CFG_Struct_BT_Call_MIC_DAE*)cfg_data;

			media_player_set_upstream_dae_enable(player, p->Enable_DAE);
			system_volume_set(AUDIO_STREAM_VOICE, p->Test_Volume, false);
		}
		break;
    case CFG_ID_BT_CALL_DAE_AL:
        if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
            break;
#if DUMP_LOG
        print_hex2("call dae:", cfg_data, cfg_size);
#endif
		memcpy(&dae_para.voice_param_arrary[0], cfg_data, cfg_size);
		if(asqt_data) {
			asqt_data->has_effect_set = 1;
		}

        media_player_update_effect_param(player, cfg_data, cfg_size);
        break;

    case CFG_ID_BT_CALL_QUALITY:
        if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
            break;
#if DUMP_LOG
		os_printk("CFG_ID_BT_CALL_QUALITY ----cfg_size %d ----Qualitysize %d \n",cfg_size,sizeof(CFG_Struct_BT_Call_Quality));
#endif
        if(cfg_size == sizeof(CFG_Struct_BT_Call_Quality)) {
            CFG_Struct_BT_Call_Quality *quality = (CFG_Struct_BT_Call_Quality*)cfg_data;
            system_volume_set(AUDIO_STREAM_VOICE, quality->Test_Volume, false);
#if DUMP_LOG
			os_printk("uality->Test_Volume %d quality->MIC_Gain.ADC0_Gain %d \n",quality->Test_Volume,quality->MIC_Gain.ADC0_Gain);
#endif
            audio_system_set_microphone_volume(AUDIO_STREAM_VOICE, quality->MIC_Gain.ADC0_Gain);
        }
        break;
    case CFG_ID_BT_CALL_QUALITY_AL:
        if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
            break;
        //todo: restart btcall?
#if DUMP_LOG
        print_hex2("call aec:", cfg_data, cfg_size);
#endif
        //media_player_update_aec_param(player, cfg_data, cfg_size);
        memcpy(&dae_para.aec_param_arrary[0], cfg_data, cfg_size);
		if(asqt_data) {
			asqt_data->has_effect_set = 1;
		}

#ifdef CONFIG_SPP_TEST_SUPPORT
		_restart_player_sync(AUDIO_STREAM_VOICE, true);
#endif

		if(asqt_data) {
			asqt_data->has_effect_set = 1;
			audio_manager_asqt_start(asqt_data->asqt_mode, asqt_data->asqt_channels);
		}

		break;

    case CFG_ID_ANC_AL:
#if CONFIG_ANC_HAL
        anc_dsp_send_anct_data(cfg_data, cfg_size);
#endif
        break;
    }
}

/*!
 * \brief 开始 ASQT 通话调试
 */
static void audio_manager_asqt_start(uint32_t asqt_mode, uint32_t asqt_channels)
{
    asqt_data_t *data = asqt_data;

    SYS_LOG_INF("%d,%x", asqt_mode, asqt_channels);
    _asqt_unprepare_data_upload(asqt_data);

    if(TOOL_DEV_TYPE_SPP == g_tool_data.dev_type) {
        return ;
    }

    data->asqt_channels = asqt_mode;
    data->asqt_channels = asqt_channels;
    data->asqt_tobe_start = 1;
    data->sample_rate = 0;
    _asqt_prepare_data_upload(data);
}

/*!
 * \brief 停止 ASQT 通话调试
 */
static void audio_manager_asqt_stop(void)
{
    SYS_LOG_INF("%d", __LINE__);
    _asqt_unprepare_data_upload(asqt_data);
}

static int32_t _aset_cmd_deal(aset_cmd_packet_t *aset_cmd_pkt, uint8_t *para0, uint8_t *para1)
{
    uint32_t aset_cmd, size;
    uint8_t *data_buf;
    int ret;

    aset_cmd = aset_cmd_pkt->opcode;
    size = aset_cmd_pkt->para_length;
    SYS_LOG_INF("cmd: %x(%d)", aset_cmd, size);

    data_buf = asqt_data->data_buffer;
    ret = ReadASETPacket(&g_tool_data.usp_handle, data_buf, size);
    if (ret != size)
    {
        SYS_LOG_WRN("ASET rcv err: %d", ret);
        return -1;
    }

    switch (aset_cmd)
    {
    case CONFIG_PROTOCOL_CMD_ADJUST_CFG:
        config_protocol_adjust(data_buf[0], data_buf[2] | ((uint32_t)data_buf[3] << 8), &data_buf[4]);
        break;

    case CONFIG_PROTOCOL_CMD_ASQT_START:
        audio_manager_asqt_start(data_buf[0], data_buf[1]);
        break;

    case CONFIG_PROTOCOL_CMD_ASQT_STOP:
        audio_manager_asqt_stop();
        break;

    default:
        return -1;
    }

	return 0;
}

void tool_aset_loop(void *p1, void *p2, void *p3)
{
    int32_t ret;

	SYS_LOG_INF("Enter");
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

    if (TOOL_DEV_TYPE_SPP != g_tool_data.dev_type)
    {
        tool_uart_init(1024);
    }

    asqt_data = mem_malloc(sizeof(*asqt_data));
    if(NULL == asqt_data) {
        SYS_LOG_ERR("malloc fail\n");
        goto ERROUT;
    }

	if (dae_init == 1) {
		memset(&dae_para, 0, sizeof(dae_parame_header_t));
		const dae_parame_header_t *dae_org = media_effect_get_param(AUDIO_STREAM_VOICE, AUDIO_STREAM_VOICE, NULL);

		memcpy(&dae_para.voice_param_arrary[0], dae_org->voice_param_arrary, sizeof(dae_org->voice_param_arrary));
		memcpy(&dae_para.aec_param_arrary[0], dae_org->aec_param_arrary, sizeof(dae_org->aec_param_arrary));

		medie_effect_set_user_param(AUDIO_STREAM_VOICE, AUDIO_STREAM_VOICE, &dae_para, sizeof(dae_parame_header_t));
		medie_effect_set_user_param(AUDIO_STREAM_MUSIC, AUDIO_STREAM_MUSIC, &dae_para, sizeof(dae_parame_header_t));
		dae_init = 0;
	}

    g_tool_data.usp_handle.handle_hook = (void*)_aset_cmd_deal;
    InitASET(&g_tool_data.usp_handle);
    os_timer_init(&asqt_data->upload_timer, _asqt_upload_data_timer_proc, NULL);

	do {
        // ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle, asqt_data->data_buffer, sizeof(asqt_data->data_buffer));
        ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle);
        if (ret)
        {
			// SYS_LOG_ERR("ASET error %d", ret);
			// break;
        }

        if(asqt_data->asqt_tobe_start && !asqt_data->asqt_started) {
            _asqt_prepare_data_upload(asqt_data);
        }
        if(asqt_data->empty_time > WAIT_DATA_TIMEOUT) {
            SYS_LOG_ERR("wait data timeout\n");
            _asqt_unprepare_data_upload(asqt_data);
            //break;
        }

        thread_timer_handle_expired();
		// os_sleep(100);
	} while (!tool_is_quitting());

ERROUT:
    if(asqt_data) {
    	_asqt_unprepare_data_upload(asqt_data);
        mem_free(asqt_data);
        asqt_data = NULL;
    }

    if (TOOL_DEV_TYPE_SPP != g_tool_data.dev_type)
    {
        tool_uart_exit();
    }
    g_tool_data.quited = 1;

	SYS_LOG_INF("Exit");
}
