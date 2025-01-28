/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Media type interface
 */

#ifndef __MEDIA_TYPE_H__
#define __MEDIA_TYPE_H__

/**
 * @defgroup media_type_apis Meida Type APIs
 * @ingroup media_system_apis
 * @{
 */
/** meida service support codec type*/
typedef enum codec_type {
	CODEC_ENC,
	CODEC_DEC,
} codec_type_e;

/** meida service support meida type*/
typedef enum media_type {
	/** not support type*/
	UNSUPPORT_TYPE = 0,

	/** type mp3 */
	MP3_TYPE,

	/** type wma */
	WMA_TYPE,

	/** type wav - 3 */
	WAV_TYPE,

	/** type flac */
	FLA_TYPE,

	/** type ape */
	APE_TYPE,

	/** type aac - 6 */
	AAC_TYPE,

	/** type ogg */
	OGG_TYPE,

	/** type act */
	ACT_TYPE,

	/** type amr - 9 */
	AMR_TYPE,

	/** type sbc */
	SBC_TYPE,

	/** type msbc */
	MSBC_TYPE,

	/** type opus - 12 */
	OPUS_TYPE,

	/** type aax */
	AAX_TYPE,

	/** type aa */
	AA_TYPE,

	/** type cvsd - 15 */
	CVSD_TYPE,

	/** type speex */
	SPEEX_TYPE,

	/** type pcm - 17 */
	PCM_TYPE,

	LDAC_TYPE,
	NAV_TYPE,
	NAV_SEP_TYPE, /* for nav 2-channel encoding to 2 separated channels */
	/** type aac - 6 */
	M4A_TYPE,
} media_type_e;

/**
 * @} end defgroup media_type_apis
 */

#endif /* __MEDIA_TYPE_H__ */

