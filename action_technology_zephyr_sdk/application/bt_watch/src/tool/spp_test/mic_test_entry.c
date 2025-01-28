/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       mic_test_entry.c
 *  \brief      Actions MIC test protocol
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2020-8-1
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "mic_test_inner.h"
#include "media_player.h"
#include "audio_system.h"
#ifdef CONFIG_ANC_HAL
#include "anc_hal.h"
#endif


const uint8_t mic_test_cmd_head[] = {0x05, 0x5A, 0x04, 0x00, 0x01, 0x11};
const uint8_t mic_test_rsp_head[] = {0x05, 0x5B, 0x05, 0x00, 0x01, 0x11};

#ifdef CONFIG_ANC_HAL
#define FFON_FBOFF_ANCOFF_ENCOFF_CMD (0xA1)
#define FFOFF_FBON_ANCOFF_ENCOFF_CMD (0xA2)
#define FF_FB_OFF_CMD                (0xA5)
#define FFON_FBOFF_ANCON_CMD         (0xA6)
#define FFOFF_FBON_ANCON_CMD         (0xA7)
#define FFON_FBON_ANCON_CMD          (0xA8)
#define GET_PARAM_CMD                (0xA9)
#define TEST_PARAM_CMD               (0xAA)
#define SET_PARAM_CMD                (0xAB)
#define TEST_GAIN_CMD                (0xAC)
#define SET_GAIN_CMD                 (0xAD)
#define STRAT_TEST_CMD               (0xAE)
#define STOP_TEST_CMD                (0xAF)
#endif
static void dump_packet(char *pack, int size)
{
    int i = 0;
    printk("\n");
    for(i=0; i<size; i++){
        if(i%10 == 0 && i != 0)
            printk("\n");
        printk("0x%x, ", pack[i]);
    }
    printk("\n");
    return;
}

static void _restart_player_nodify_callback(struct app_msg *msg, int result, void *reply)
{
	if (msg->sync_sem) {
		os_sem_give((os_sem *)msg->sync_sem);
	}
}

void _restart_player_sync(uint8_t efx_stream_type, bool force)
{
	struct app_msg msg = {0};
	os_sem return_notify;
    char *fg_app;

    fg_app = app_manager_get_current_app();
    if (!fg_app){
        return;
    }
	msg.type = MSG_BT_EVENT;
    msg.cmd = BT_REQ_RESTART_PLAY;
	os_sem_init(&return_notify, 0, 1);
	msg.callback = _restart_player_nodify_callback;
	msg.sync_sem = &return_notify;
	if (!send_async_msg(fg_app, &msg)) {
		return;
	}
	os_sem_take(&return_notify, OS_FOREVER);
}

void send_mic_test_response(uint8_t rsp_cmd, uint16_t length)
{
    uint16_t *len;
    mic_test_rsp_packet_t ack;

    // memset(&ack, 0, sizeof(ack));
    memcpy(ack.head, mic_test_rsp_head, sizeof(mic_test_rsp_head));
    len = ((uint16_t *)&ack) + 1;
    *len = 0x05 + length;
    ack.ack_cmd     = rsp_cmd;
    ack.reserved[0] = 0;
    ack.reserved[1] = 0;

    spp_test_backend_write((uint8_t*)&ack, sizeof(ack), 100);
    SYS_LOG_INF("response packet %d:======", sizeof(mic_test_rsp_packet_t));
    dump_packet((char*)&ack, sizeof(mic_test_rsp_packet_t));
}

void mic_test_communicate(uint8_t head_data)
{
    bool head_recieved = TRUE;
    mic_test_cmd_packet_t cmd_packet = {0};
    bool status;
    media_player_t *player;

    cmd_packet.head[0] = head_data;

	while (!tool_is_quitting())
	{
        if (head_recieved
		    && ((MIC_TEST_CMD_LEN - 1) == spp_test_backend_read(&cmd_packet.head[1], MIC_TEST_CMD_LEN - 1, 1000)))
		{
            status = TRUE;
            head_recieved = FALSE;

            SYS_LOG_INF("cmd:0x%X", cmd_packet.cmd);
			// log_info("test cmd:%d", cmd_packet.cmd);
            switch (cmd_packet.cmd)
            {
                case ENABLE_L_MIC_RAW_TEST:
                // only main MIC
                    player = media_player_get_current_dumpable_player();
                	if (!player) {
                        SYS_LOG_ERR("player no start");
                		break;
                	}
                    if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
                        break;
                // player_extend_operation(PLAYER_BLUETOOTHSPEECH, PLAYER_EXTOP_MAINMIC_RAWEN, NULL);
                    //_restart_player_sync(AUDIO_STREAM_VOICE, false);
                    //media_player_set_upstream_enc_enable(player, false);
                    //media_player_set_upstream_dae_enable(player, false);
                    //media_player_set_upstream_cng_enable(player, false);
                    //media_player_set_mic_channel_select(player, 0);
                break;

                case ENABLE_R_MIC_RAW_TEST:
                // only vice MIC
                    player = media_player_get_current_dumpable_player();
                	if (!player) {
                        SYS_LOG_ERR("player no start");
                		break;
                	}
                    if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
                        break;
                // player_extend_operation(PLAYER_BLUETOOTHSPEECH, PLAYER_EXTOP_REFMIC_RAWEN, NULL);
                    //_restart_player_sync(AUDIO_STREAM_VOICE, false);
                    //media_player_set_upstream_enc_enable(player, false);
                    //media_player_set_upstream_dae_enable(player, false);
                    //media_player_set_upstream_cng_enable(player, false);
                    //media_player_set_mic_channel_select(player, 1);
                break;

                case ENABLE_BOTH_MIC_ENC_TEST:
                // both MIC
                    player = media_player_get_current_dumpable_player();
                	if (!player) {
                        SYS_LOG_ERR("player no start");
                		break;
                	}
                    if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
                        break;
                // player_extend_operation(PLAYER_BLUETOOTHSPEECH, PLAYER_EXTOP_TWOMIC_ENCEN, NULL);
                    //_restart_player_sync(AUDIO_STREAM_VOICE, false);
                    //media_player_set_upstream_enc_enable(player, true);
                    //media_player_set_upstream_dae_enable(player, false);
                    //media_player_set_upstream_cng_enable(player, false);
                    //media_player_set_mic_channel_select(player, 2);
                    break;
#ifdef CONFIG_CONSUMER_EQ
                case CLOSE_EFFECT_EQ_CMD:
                    effect_eq_close();
                break;
                case CLOSE_SPK_EQ_CMD:
                    speaker_eq_close();
                break;
                case SPK_EQ_TEST_CMD:
                {
                    SYS_LOG_INF("SPK_EQ_TEST_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    speaker_eq_t *eq = (speaker_eq_t *)data;
                    speaker_eq_set_param(eq, false);
                }
                break;
                case SPK_EQ_SET_CMD:
                {
                    SYS_LOG_INF("SPK_EQ_SET_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    speaker_eq_t *eq = (speaker_eq_t *)data;
                    speaker_eq_set_param(eq, true);
                }
                break;
#endif //CONFIG_CONSUMER_EQ
#ifdef CONFIG_ANC_HAL
                case STRAT_TEST_CMD:
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    scene_id = *(uint32_t *)data;
                    anc_dsp_close();
                    anc_init_info.mode = scene_id;
                    anc_dsp_open(&anc_init_info);
                    break;
                case STOP_TEST_CMD:
                    anc_dsp_close();
                    break;
                case FFON_FBOFF_ANCOFF_ENCOFF_CMD:
                    SYS_LOG_INF("FFON_FBOFF_ANCOFF_ENCOFF_CMD");
                    anc_dsp_send_command(ANC_COMMAND_OFF_HY, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                    player = media_player_get_current_dumpable_player();
                	if (!player) {
                        SYS_LOG_ERR("player no start");
                		break;
                	}
                    if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
                        break;
                    _restart_player_sync(AUDIO_STREAM_VOICE_ANC, false);
                    media_player_set_upstream_enc_enable(player, false);
                    media_player_set_upstream_dae_enable(player, false);
                    media_player_set_upstream_cng_enable(player, false);
                    media_player_set_mic_channel_select(player, 0);
                    break;
                case FFOFF_FBON_ANCOFF_ENCOFF_CMD:
                    SYS_LOG_INF("FFOFF_FBON_ANCOFF_ENCOFF_CMD");
                    anc_dsp_send_command(ANC_COMMAND_OFF_HY, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                    player = media_player_get_current_dumpable_player();
                	if (!player) {
                        SYS_LOG_ERR("player no start");
                		break;
                	}
                    if(player->type != MEDIA_PLAYER_TYPE_CAPTURE_AND_PLAYBACK)
                        break;
                    _restart_player_sync(AUDIO_STREAM_VOICE_ANC, false);
                    media_player_set_upstream_enc_enable(player, false);
                    media_player_set_upstream_dae_enable(player, false);
                    media_player_set_upstream_cng_enable(player, false);
                    media_player_set_mic_channel_select(player, 1);
                    break;
                case FF_FB_OFF_CMD:
                    SYS_LOG_INF("FF_FB_OFF_CMD");
                    anc_dsp_send_command(ANC_COMMAND_OFF_HY, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                break;

                case FFON_FBOFF_ANCON_CMD:
                    SYS_LOG_INF("FFON_FBOFF_ANCON_CMD");
                    anc_dsp_send_command(ANC_COMMAND_OFF_FB, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                break;
                case FFOFF_FBON_ANCON_CMD:
                    SYS_LOG_INF("FFOFF_FBON_ANCON_CMD");
                    anc_dsp_send_command(ANC_COMMAND_OFF_FF, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                break;
                case FFON_FBON_ANCON_CMD:
                    SYS_LOG_INF("FFON_FBON_ANCON_CMD");
                    anc_dsp_send_command(ANC_COMMAND_ON_HY, 0, 0);
                    send_mic_test_response(cmd_packet.cmd, 0);
                    status = FALSE;
                break;
                case GET_PARAM_CMD:
                    SYS_LOG_INF("GET_PARAM_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    scene_id = *(uint32_t *)data;
                    file_id = *(uint32_t *)(data+4);
                    // dump_packet(data, ret);
                    anc_mode = anc_dsp_get_mode();
                    if(anc_mode != *(uint32_t *)data){
                        anc_dsp_close();
                        anc_init_info.mode = (u8_t)scene_id;
                        anc_dsp_open(&anc_init_info);
                    }
                    anc_dsp_get_filter_para(scene_id, file_id, data, &size);
                    // anc_dsp_get_config(*(uint32_t *)data, *(uint32_t *)(data+4), data, &size);
                    send_mic_test_response(cmd_packet.cmd, size);
                    spp_test_backend_write(data, size, 100);
                    dump_packet(data, size);
                    status = FALSE;
                break;
                case SET_PARAM_CMD:
                    SYS_LOG_INF("SET_PARAM_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    // dump_packet(data, ret);
                    scene_id = *(uint32_t *)data;
                    file_id = *(uint32_t *)(data+4);

                    anc_mode = anc_dsp_get_mode();
                    if(anc_mode != scene_id){
                        anc_dsp_close();
                        anc_init_info.mode = (u8_t)scene_id;
                        anc_dsp_open(&anc_init_info);
                    }
                    anc_dsp_set_filter_para(scene_id, file_id, data+8, ret-8, 1);
                break;
                case TEST_PARAM_CMD:
                    SYS_LOG_INF("TEST_PARAM_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    // dump_packet(data, ret);

                    scene_id = *(uint32_t *)data;
                    file_id = *(uint32_t *)(data+4);
                    anc_mode = anc_dsp_get_mode();
                    if(anc_mode != scene_id){
                        anc_dsp_close();
                        anc_init_info.mode = (u8_t)scene_id;
                        anc_dsp_open(&anc_init_info);
                    }
                    anc_dsp_set_filter_para(scene_id, file_id, data+8, ret-8, 0);
                break;
                case TEST_GAIN_CMD:
                    SYS_LOG_INF("TEST_GAIN_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    // dump_packet(data, ret);
                    scene_id = *(uint32_t *)data;
                    file_id = *(uint32_t *)(data+4);
                    anc_mode = anc_dsp_get_mode();
                    if(anc_mode != scene_id){
                        anc_dsp_close();
                        anc_init_info.mode = (u8_t)scene_id;
                        anc_dsp_open(&anc_init_info);
                    }
                    anc_dsp_set_gain(scene_id, file_id, *(uint32_t *)(data+8), 0);
                break;
                case SET_GAIN_CMD:
                    SYS_LOG_INF("SET_GAIN_CMD");
                    ret = spp_test_backend_read(data, cmd_packet.length-4, 100);
                    // dump_packet(data, ret);
                    scene_id = *(uint32_t *)data;
                    file_id = *(uint32_t *)(data+4);
                    anc_mode = anc_dsp_get_mode();
                    if(anc_mode != scene_id){
                        anc_dsp_close();
                        anc_init_info.mode = (u8_t)scene_id;
                        anc_dsp_open(&anc_init_info);
                    }
                    anc_dsp_set_gain(scene_id, file_id, *(uint32_t *)(data+8), 1);
                break;
#endif
                default :
                status = FALSE;
                break;
            }

            if (status)
            {
                send_mic_test_response(cmd_packet.cmd, 0);
            }
        }

        if (!head_recieved && (1 == spp_test_backend_read((uint8_t*)&cmd_packet.head[0], 1, 0)))
		{
		    if(cmd_packet.head[0] == 0x5)
            head_recieved = TRUE;
		}
        else
        {
            os_sleep(10);
        }

        thread_timer_handle_expired();
	}
}

