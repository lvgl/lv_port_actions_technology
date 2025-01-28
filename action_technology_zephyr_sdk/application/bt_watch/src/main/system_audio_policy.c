/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system audio policy
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <system_app.h>
#include <string.h>
#include <audio_system.h>
#include <audio_policy.h>
#include "app_config.h"

#define MAX_AUDIO_VOL_LEVEL 16

#ifdef CONFIG_MEDIA

/* unit: 0.1 dB */
short voice_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-1000, -420, -380,  -340,
	 -300, -260, -225,  -195,
	 -165, -135, -110,  -90,
	  -70, -50,  -20,   0,
};

/* unit: 0.001 dB */
int voice_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-375, -375, -375, -375,
	-375, -375, -375, -375,
	-375, -375, -375, -375,
	-375, -375, -375, -375,
};

/* unit: 0.1 dB */
short tts_da_table[MAX_AUDIO_VOL_LEVEL + 1] = {
	-0000, -0000, -0000, -0000,
	-0000, -0000, -0000, -0000,
	-0000, -0000, -0000, -0000,
	-0000, -0000, -0000, -0000,
};

/* unit: 0.001 dB */
int tts_pa_table[MAX_AUDIO_VOL_LEVEL + 1] = {
	-60000, -40875, -31125, -25125,
	-19875, -16125, -13125, -10125,
	-7125,  -4875,  -4125,	-3375,
	-2250,  -1500,  -750, 	0
};

/* unit: 0.1 dB */
const short music_da_table[MAX_AUDIO_VOL_LEVEL] = {
	-80, -80, -80, -72, -66, -60, -54, -48,
	-42, -36, -30, -24, -18, -12, -6, 0
};

/* unit: 0.001 dB */
const int music_pa_table[MAX_AUDIO_VOL_LEVEL] = {
	-100000, -49000, -39000, -33000, -28000, -24000, -21000, -18000,
	-15000, -12000, -10000, -8000,  -6000, -4000, -2000, 0,
};

static struct audio_policy_t system_audio_policy = {
	.audio_out_channel = AUDIO_CHANNEL_DAC,
	.audio_in_mic_gain = 305, // 30.5db

#ifdef CONFIG_AEC_TAIL_LENGTH
	.voice_aec_tail_length_16k = CONFIG_AEC_TAIL_LENGTH,
	.voice_aec_tail_length_8k = CONFIG_AEC_TAIL_LENGTH * 2,
#endif

	.tts_fixed_volume = 1,
	.volume_saveto_nvram = 1,

	.audio_out_volume_level = MAX_AUDIO_VOL_LEVEL - 1,
	.voice_da_table = voice_da_table,
	.voice_pa_table = voice_pa_table,

	.tts_da_table = tts_da_table,
	.tts_pa_table = tts_pa_table,

	.music_da_table = music_da_table,
	.music_pa_table = music_pa_table,

};

#endif /* CONFIG_MEDIA */

void system_audio_policy_init(void)
{
#if 0
	CFG_Struct_BT_Call_Volume_Table  cfg_call_volume_Table;
    CFG_Struct_Volume_Settings cfg_volume_settings;
    CFG_Struct_BTSpeech_User_Settings cfg_btspeech_settings;
    CFG_Struct_Voice_Volume_Table cfg_voice_volume_table;
    CFG_Type_MIC_Gain  MIC_Gain;
    u8_t i;

    app_config_load("defcfg.bin", NO, NO);

    app_config_load("usrcfg.bin", NO, NO);

    app_config_read
    (
        CFG_ID_BT_CALL_VOLUME_TABLE,
        &cfg_call_volume_Table, 0, sizeof(CFG_Struct_BT_Call_Volume_Table)
    );

    app_config_read
    (
        CFG_ID_BTSPEECH_USER_SETTINGS,
        &cfg_btspeech_settings, 0, sizeof(CFG_Struct_BTSpeech_User_Settings)
    );

    for(i = 0; i < (CFG_MAX_BT_CALL_VOLUME + 1); i++)
    {
        voice_da_table[i] = ((cfg_call_volume_Table.Level[i] - 0xbf) * 375) / 100;
        voice_pa_table[i] = 0;
    }

    app_config_read
    (
        CFG_ID_VOLUME_SETTINGS,
        &cfg_volume_settings, 0, sizeof(CFG_Struct_Volume_Settings)
    );

    system_audio_policy.tts_max_volume = cfg_volume_settings.Voice_Max_Volume;
    system_audio_policy.tts_min_volume = cfg_volume_settings.Voice_Min_Volume;


    app_config_read
    (
        CFG_ID_VOICE_VOLUME_TABLE,
        &cfg_voice_volume_table, 0, sizeof(CFG_Struct_Voice_Volume_Table)
    );

    for(i = 0; i < (CFG_MAX_VOICE_VOLUME+1); i++)
    {
        tts_da_table[i] = 0;
        tts_pa_table[i] = (cfg_voice_volume_table.Level[i] - 0xbf) * 375;
    }

 	app_config_read
    (
        CFG_ID_BT_CALL_QUALITY,
        &MIC_Gain,
        offsetof(CFG_Struct_BT_Call_Quality, MIC_Gain),
        sizeof(CFG_Type_MIC_Gain)
    );

    system_audio_policy.audio_in_mic_gain = MIC_Gain.ADC0_Gain;
#endif

#ifdef CONFIG_MEDIA
	audio_policy_register(&system_audio_policy);
#endif

}
