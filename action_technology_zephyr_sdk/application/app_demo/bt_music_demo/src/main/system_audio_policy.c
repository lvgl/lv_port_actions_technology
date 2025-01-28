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
#include <string.h>
#include <audio_system.h>
#include <audio_policy.h>

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
	-2250,  -1500,  -750,	0
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
	.audio_in_mic_gain = 365, /* 36.5db */

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
#ifdef CONFIG_MEDIA
	audio_policy_register(&system_audio_policy);
#endif
}
