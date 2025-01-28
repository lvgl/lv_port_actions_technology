/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEDIA_EFFECT_PARAM_H__
#define __MEDIA_EFFECT_PARAM_H__

#include <toolchain.h>
#include <arithmetic.h>

#define ASET_DAE_PEQ_POINT_NUM  (AS_DAE_EQ_NUM_BANDS)

/* 512 bytes */
typedef struct aset_para_bin {
	char dae_para[AS_DAE_PARA_BIN_SIZE]; /* 512 bytes */
} __packed aset_para_bin_t;

#define ASET_MAX_VOLUME_TABLE_LEVEL  41

typedef struct {
	int32_t nPAVal[ASET_MAX_VOLUME_TABLE_LEVEL]; /* unit: 0.001 dB */
	int16_t sDAVal[ASET_MAX_VOLUME_TABLE_LEVEL]; /* unit: 0.1 dB */
	int8_t bEnable;
	int8_t bRev[9];
} __packed aset_volume_table_v2_t;

#define ASQT_DAE_PEQ_POINT_NUM  (AS_DAE_H_EQ_NUM_BANDS)

/* 288 bytes */
typedef struct asqt_para_bin {
#ifndef CONFIG_DSP 
	as_hfp_speech_para_bin_t sv_para; /* 60 bytes */
	char cRev1[12];
	char dae_para[AS_DAE_H_PARA_BIN_SIZE]; /* 192 bytes */
	char cRev2[24];
#else
	char sv_para[330];
#endif
} __packed asqt_para_bin_t;

/**
 * @brief Set user effect param.
 *
 * This routine set user effect param. The new param will take effect after
 * next media_player_open.
 *
 * @param stream_type stream type, see audio_stream_type_e
 * @param effect_type effect type, reserved for future
 * @param vaddr address of user effect param
 * @param size size of user effect param
 *
 * @return 0 if succeed; non-zero if fail.
 */
int medie_effect_set_user_param(uint8_t stream_type, uint8_t effect_type, const void *vaddr, int size);

/**
 * @brief Get effect param.
 *
 * This routine get effect param. User effect param will take
 * take precedence over effect file.
 *
 * @param stream_type stream type, see audio_stream_type_e
 * @param effect_type effect type, reserved for future
 * @param effect_size size of the effect param
 *
 * @return address of effect param.
 */
const void *media_effect_get_param(uint8_t stream_type, uint8_t effect_type, int *effect_size);

#endif /* __MEDIA_EFFECT_PARAM_H__ */
