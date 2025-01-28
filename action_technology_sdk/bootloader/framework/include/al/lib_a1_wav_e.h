/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __K1_WAV_E_H__
#define __K1_WAV_E_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	WAVENC_LINEPCM_FORMAT = 0x1,
	WAVENC_AULAW_FORAMT   = 0x6,
	WAVENC_IMAADPCM_FORMAT = 0x11,
} wavenc_audio_format_t;

#ifdef __cplusplus
}
#endif

#endif // __K1_WAV_E_H__
