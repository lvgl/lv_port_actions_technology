/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       abtp_entry.c
 *  \brief      Actions Bluetooth Test Protocol
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2020-8-1
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#include "abtp_inner.h"
#include <soc_dsp.h>

void run_test_code(u32_t entry)
{
    void (*test_entry)(void);
    test_entry = (void (*)(void))entry;

    SYS_LOG_INF("Start code test at: %x\n", entry);
    test_entry();
}

void send_abtp_response(u8_t cmd_sequence, ABTP_STATUS status)
{
    abtp_response_packet_t abtp_ack;

    abtp_ack.magic      = 0xAA;
    abtp_ack.sequence   = cmd_sequence;
    abtp_ack.reserved   = 0;
    abtp_ack.status     = (u8_t)status;

    spp_test_backend_write((u8_t*)&abtp_ack, sizeof(abtp_ack), 100);
}

void abtp_adjust_config(uint32_t cfg_id, uint32_t cfg_size, u8_t* cfg_data)
{
    config_protocol_adjust(cfg_id, cfg_size, cfg_data);
}

void abtp_audio_transfer_start(uint32_t type, uint32_t channel_bitmap)
{
    if (ABTP_MIC_DATA_UPLOAD == type)
    {
        mic_data_start_capture(mic_pcm_upload_handler, MICDATA_SAMPLE_RATE, channel_bitmap);
    }
    else if (ABTP_MIC_PROCESSED_DATA == type)
    {
        //enc_data_start_capture(enc_pcm_upload_handler, 16000, 0x4);
        // mic_process_init(16000);
    }
}

void abtp_audio_transfer_stop(void)
{
    mic_data_stop_capture();
    audio_data_play_test_stop();
    // mic_process_exit();
}

static bool send_AT_command(char *at_cmd)
{
#ifdef CONFIG_BT_HFP_HF
    if (0 == bt_manager_hfp_send_at_command((uint8_t*)at_cmd, 1))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
#else
	return FALSE;
#endif
}

ABTP_STATUS abtp_run_AT_command(u32_t cmd_len)
{
    u32_t rx_bytes;
    char *cmd_buffer;
    ABTP_STATUS ret = ABTP_ERROR;

    cmd_buffer = app_mem_malloc(cmd_len + 1);
    if (!cmd_buffer) return ABTP_ERROR;
    cmd_buffer[cmd_len] = 0;

    rx_bytes = spp_test_backend_read((u8_t*)cmd_buffer, cmd_len, 1000);
    if (cmd_len == rx_bytes)
    {
        SYS_LOG_DBG("Receive AT cmd %s", cmd_buffer);
        if(send_AT_command(cmd_buffer))
        {
            ret = ABTP_OK;
        }
    }
    else
    {
        SYS_LOG_ERR("Receive %d, expect %d", rx_bytes, cmd_len);
    }

    app_mem_free(cmd_buffer);
    return ret;
}

ABTP_STATUS abtp_recv_cmd_para(u8_t *data, u32_t data_len)
{
    ABTP_STATUS status = ABTP_OK;

    if ((0 != data_len) && (data_len <= 256))
    {
        if (data_len != spp_test_backend_read(data, data_len, 1000))
        {
            // fail to receive payload
            status = ABTP_DOWNLOAD_ERROR;
        }
    }

    return status;
}

int abtp_cmd_parse(abtp_cmd_packet_t *abtp_cmd)
{
    ABTP_STATUS status = ABTP_OK;
    u8_t *temp, *data;
    u32_t data_len, type, para0;

    temp = app_mem_malloc(256);
	if (!temp) return ABTP_ERROR;
    data_len = abtp_cmd->payload_length;

    switch (abtp_cmd->abtp_cmd)
    {
        case ABTP_CMD_ADJUST_CONFIG:
        status = abtp_recv_cmd_para(temp, data_len);
        if (ABTP_OK == status)
        {
            abtp_adjust_config(temp[0], temp[1], temp + 4);
        }
        break;

        case ABTP_CMD_AUDIO_WRITE_PLAY:
        dsp_soc_release_mem(0);
        data = (u8_t*)AUDIO_DATA_TEMP_BUFF;
        if ((data_len <= 96 * 1024) &&
            (data_len == spp_test_backend_read(data, data_len, 5000)))
        {
            audio_data_play_test_start((s16_t*)data, data_len, abtp_cmd->para1);
        }
        else
        {
            status = ABTP_ERROR;
        }
        break;

        case ABTP_CMD_AUDIO_UPLOAD_START:
        status = abtp_recv_cmd_para(temp, data_len);
        if (ABTP_OK == status)
        {
            type = ((u32_t*)temp)[0];
            para0 = ((u32_t*)temp)[1];
            abtp_audio_transfer_start(type, para0);
        }
        break;

        case ABTP_CMD_AUDIO_UPLOAD_STOP:
        abtp_audio_transfer_stop();
        // status = ABTP_END;
        break;

        case ABTP_AT_COMMAND:
        status = abtp_run_AT_command(abtp_cmd->payload_length);
        break;

        default :
        status = ABTP_COMMAND_NOT_SUPPORT;
        break;
    }

    app_mem_free(temp);
    return status;
}

void abtp_communicate(u8_t head_data)
{
    bool connected = FALSE, head_recieved = TRUE;
    abtp_cmd_packet_t abtp_cmd;
    int status;
    u8_t cmd_sequence;

    abtp_cmd.magic = head_data;

	while (!tool_is_quitting())
	{
        if (head_recieved &&
            ((ABTP_CMD_LEN - 1) == spp_test_backend_read((uint8_t*)&abtp_cmd + 1, ABTP_CMD_LEN - 1, 1000)))
		{
			SYS_LOG_INF("ABTP:%x", abtp_cmd.abtp_cmd);
            head_recieved = FALSE;
            cmd_sequence = abtp_cmd.sequence;

            // authentication key: 0x1F17D43C
            if ((ABTP_CMD_CONNECT == abtp_cmd.abtp_cmd) &&
                (0x1F17D43C == abtp_cmd.para1))
            {
                connected = TRUE;
                status = ABTP_OK;
				bt_manager_set_visible(false);
				bt_manager_set_connectable(false);
#ifdef CONFIG_BT_BLE
				bt_manager_ble_adv_stop();
#endif
                send_abtp_response(cmd_sequence, ABTP_OK);
            }
            else if (connected)
            {
                status = abtp_cmd_parse(&abtp_cmd);
                if (ABTP_OK != status)
                {
                    // send responde if error happened
                    send_abtp_response(cmd_sequence, status);
                }
            }
		}

        if (!head_recieved && (1 == spp_test_backend_read((uint8_t*)&abtp_cmd, 1, 0)))
		{
            head_recieved = TRUE;
		}
        else
        {
            os_sleep(1);
        }

        thread_timer_handle_expired();
	}
	bt_manager_set_visible(true);
	bt_manager_set_connectable(true);
#ifdef CONFIG_BT_BLE
	bt_manager_ble_adv_start();
#endif
    abtp_audio_transfer_stop();
}
