/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BTCALL_PROCESSOR_DSP_H__
#define __BTCALL_PROCESSOR_DSP_H__

typedef struct hfp_dae_para {
    short  pcm_debugmode;   //0:上传pcm, 1:不上传；2:清0
    short  hfp_connect_mod;// 0 没接通   1 接通
    short  change_flag;   //音效change标志
    short  mic_channel;       //   0: 左声道，   1: 右声道, 2: 双声道
    uint16_t  speak_volume;     //0-31级软音量
    short  asrc_out;         //0:out 0, 1:out1
	short  reserve[16];
    char  dae_para_info_array[512];
	char  aec_para[296];
	uint16_t  upstream_dae_enable:1;
    uint16_t  downstream_dae_enable:1;
} hfp_dae_para_t;

#endif /* __BTCALL_PROCESSOR_DSP_H__ */
