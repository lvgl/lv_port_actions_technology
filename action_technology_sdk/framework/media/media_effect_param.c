/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define SYS_LOG_DOMAIN "media"
#include <stdio.h>
#include <string.h>
#include <sdfs.h>
#include <audio_system.h>
#include <media_effect_param.h>

#define BTCALL_EFX_PATTERN			"alcfg.bin"
#define LINEIN_EFX_PATTERN			"linein.efx"
#define MUSIC_DRC_EFX_PATTERN		"alcfg.bin"
#define MUSIC_NO_DRC_EFX_PATTERN	"alcfg.bin"

static const void *btcall_effect_user_addr;
static const void *linein_effect_user_addr;
static const void *music_effect_user_addr;

int medie_effect_set_user_param(uint8_t stream_type, uint8_t effect_type, const void *vaddr, int size)
{
	const void **user_addr = NULL;
	int expected_size = 0;

	switch (stream_type) {
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
		user_addr = &btcall_effect_user_addr;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LINEIN:
		user_addr = &linein_effect_user_addr;
		expected_size = sizeof(aset_para_bin_t);
		break;
	default:
		user_addr = &music_effect_user_addr;
		expected_size = sizeof(aset_para_bin_t);
		break;
	}
	printk("%s %d user_addr %p ,expected_size %dvaddr:%p size:%d\n",__FUNCTION__,__LINE__,user_addr,expected_size, vaddr, size);


	*user_addr = vaddr;
	return 0;
}

static const void *_media_effect_load(const char *efx_pattern, int *expected_size, uint8_t stream_type)
{
	char efx_name[16];
	void *vaddr = NULL;
	int size = 0;

	strncpy(efx_name, efx_pattern, sizeof(efx_name));
	sd_fmap(efx_name, &vaddr, &size);

	if (!vaddr || !size) {
		SYS_LOG_ERR("not found");
		return NULL;
	}

	*expected_size = size;

	SYS_LOG_INF("%s", efx_name);
	return vaddr;
}

const void *media_effect_get_param(uint8_t stream_type, uint8_t effect_type, int *effect_size)
{
	const void *user_addr = NULL;
	const char *efx_pattern = NULL;
	int expected_size = 0;

	switch (stream_type) {
	case AUDIO_STREAM_TTS: /* not use effect file */
		return NULL;
	case AUDIO_STREAM_VOICE:
	case AUDIO_STREAM_LE_AUDIO:
		user_addr = btcall_effect_user_addr;
		efx_pattern = BTCALL_EFX_PATTERN;
		expected_size = sizeof(asqt_para_bin_t);
		break;
	case AUDIO_STREAM_LINEIN:
		user_addr = linein_effect_user_addr;
		efx_pattern = LINEIN_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	case AUDIO_STREAM_TWS:
		user_addr = music_effect_user_addr;
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
		expected_size = sizeof(aset_para_bin_t);
		break;
	default:
		user_addr = music_effect_user_addr;
	#ifdef CONFIG_MUSIC_DRC_EFFECT
		efx_pattern = MUSIC_DRC_EFX_PATTERN;
	#else
		efx_pattern = MUSIC_NO_DRC_EFX_PATTERN;
	#endif
		expected_size = sizeof(aset_para_bin_t);
		break;
	}

	if (!user_addr)
		user_addr = _media_effect_load(efx_pattern, &expected_size, stream_type);

	if (user_addr && effect_size)
		*effect_size = expected_size;

	return user_addr;
}
