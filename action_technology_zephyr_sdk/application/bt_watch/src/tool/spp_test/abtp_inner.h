/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2020 Actions Semiconductor. All rights reserved.
 *
 *  \file       abtp_inner.h
 *  \brief      Actions Bluetooth Test Protocol
 *  \author     zhouxl
 *  \version    1.10
 *  \date       2020-08-01
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */

#ifndef __ABTP_INNER_H__
#define __ABTP_INNER_H__

#ifndef _ASSEMBLER_
#include "spp_test_inner.h"
#include <audio_record.h>
#include <config.h>
#include "media_player.h"

#define ABTP_HEAD_MAGIC         (0x90)

#define AUDIO_DATA_TEMP_BUFF    (0x01040000)

typedef enum __ABTP_STATUS
{
    ABTP_OK =  0,               ///< Operation succeeded
    ABTP_ERROR,                 ///< Unspecified error
    ABTP_BUSY,                  ///< receiver is busy, transfer few moments later
    ABTP_KEY_VERIFY_FAIL,       ///< key authentication fail
    ABTP_UPLOAD_ERROR,          ///< ABTP upload data error happend
    ABTP_DOWNLOAD_ERROR,        ///< ABTP download data error happend
    ABTP_TIMEOUT,               ///< ABTP transaction timeout
    ABTP_COMMAND_NOT_SUPPORT,   ///< Not support ABTP command
    ABTP_ILLEGAL_PARA,          ///< Not support ABTP command
    ABTP_END,                   ///< ABTP transaction finish
}ABTP_STATUS;

typedef enum __ABTP_COMMAND
{
    ABTP_CMD_CONNECT            = ('c' + ('p' << 8)),
    ABTP_CMD_PRINT              = ('p' + ('l' << 8)),
    ABTP_CMD_ADJUST_CONFIG      = ('a' + ('s' << 8)),
    ABTP_CMD_AUDIO_WRITE_PLAY   = ('a' + ('w' << 8)),
    ABTP_CMD_AUDIO_UPLOAD_START = ('a' + ('q' << 8)),
    ABTP_CMD_AUDIO_UPLOAD_STOP  = ('a' + ('p' << 8)),
    ABTP_CMD_READ_MEMORY        = ('r' + ('m' << 8)),
    ABTP_CMD_READ_STORAGE       = ('r' + ('s' << 8)),
    ABTP_CMD_ERASE_STORAGE      = ('e' + ('s' << 8)),
    ABTP_DOWNLOAD_CODE          = ('d' + ('c' << 8)),
    ABTP_RUN_CODE               = ('r' + ('c' << 8)),
    ABTP_AT_COMMAND             = ('A' + ('T' << 8)),
}ABTP_COMMAND;

typedef enum __ABTP_AUDIO_TYPE
{
    ABTP_HFP_DATA_UPLOAD = 0,   ///< audio data during HFP call
	ABTP_MIC_DATA_UPLOAD,
	ABTP_MIC_PROCESSED_DATA,
}ABTP_AUDIO_TYPE_e;

typedef struct
{
    u8_t magic;                 ///< 0x90, ABTP magic
    u8_t sequence;
    u16_t abtp_cmd;             ///< ABTP command, \ref in ABTP_COMMAND
    u16_t cmd_para;
    u16_t payload_length;       ///< payload data length, not inlcude packet head
    u32_t para1;
}abtp_cmd_packet_t;

typedef struct
{
    u8_t magic;                 ///< 0xAA
    u8_t sequence;              ///< MUST equal to its respond command
    u8_t reserved;
    u8_t status;                ///< abtp transfer status, \ref in ABTP_STATUS
}abtp_response_packet_t;

#define ABTP_CMD_LEN            (sizeof(abtp_cmd_packet_t))

#define MICDATA_PACKET_SIZE     (512 - 16)
#define MICDATA_SAMPLE_RATE     (16000)

typedef struct
{
    u8_t data_flag;             ///< 0x16 stands for PCM 16bit data
    u8_t reserved[1];
    u16_t payload_len;          ///< audio frame data length, NOT include this head
    u8_t reserved1[4];

    u16_t channel;
    u16_t sample_rate;
    u32_t sequence;

    u8_t data[0];
} mic_pcm_frame_info_t;

typedef struct {
    union {
        uint16_t status;     //for btcall
        uint16_t frame_cnt; //for btmusic
    };
    uint16_t seq_no;
    uint16_t frame_len;
    uint16_t padding_len;
} pkthdr_t;
typedef struct
{
    u8_t *micdata_ref;
    u8_t *micdata_main;
    u8_t *spp_data;

    u32_t offset;

    u32_t sample_rate;
    u32_t channel_bitmap;
    s32_t main_mic_gain;

    bool mic_exchange;
    
    struct thread_timer ttimer;
    mic_pcm_frame_info_t *pcm_frame_head;

    struct audio_record_t *audio_record;
	struct acts_ringbuf *ringbuf_audio_input;
    io_stream_t audio_stream;
} mic_upload_context_t;
typedef struct
{
    pkthdr_t pkthdr;
    media_player_t *player;
    io_stream_t input_stream;
	io_stream_t output_stream;
    io_stream_t dump_stream[2];
    struct acts_ringbuf *dump_buf[2];
    uint32_t last_fill_time;
    u8_t *spp_data;
    u32_t sample_rate;
    u32_t channel_bitmap;
    struct thread_timer ttimer;
    mic_pcm_frame_info_t *pcm_frame_head;
} enc_upload_context_t;


extern int32_t (*asqt_audio_write)(const u8_t *data, u32_t size, u32_t timeout_ms);
extern int (*audio_debug_stream_write)(int channel, u8_t* data, int size);

extern void config_protocol_adjust(uint32_t cfg_id, uint32_t cfg_size, u8_t* cfg_data);
extern void asqt_spp_write_handler(void);
extern void audio_manager_asqt_stop(void);
extern int audio_asqt_stream_spp_write(int channel, u8_t* data, int size);
extern int32_t BS_get_main_mic_gain(void);
extern int32_t player_split_stero_data(int16_t *src, int16_t *dest, uint32_t samples);

extern void watchdog_init(void);
extern void watchdog_stop(void);

void mic_pcm_upload_handler(struct thread_timer *ttimer, void *expiry_fn_arg);
bool mic_data_start_capture(thread_timer_expiry_t expiry_fn, u32_t sample_rate, u32_t channel_bitmap);
void mic_data_stop_capture(void);
int32_t mic_data_split_stero(int16_t *src, int16_t *dest, uint32_t samples);

void enc_pcm_upload_handler(struct thread_timer *ttimer, void *expiry_fn_arg);
bool enc_data_start_capture(thread_timer_expiry_t expiry_fn, u32_t sample_rate, u32_t channel_bitmap);
void enc_data_stop_capture(void);
void audio_data_play_test_start(s16_t *pcm_data, u32_t size, u32_t sample_rate);
void audio_data_play_test_stop(void);

void mic_process_init(u32_t sample_rate);
void mic_process_exit(void);
void mic_processed_pcm_upload_handler(struct thread_timer *ttimer, void *expiry_fn_arg);
#endif
#endif  // __ABTP_INNER_H__


