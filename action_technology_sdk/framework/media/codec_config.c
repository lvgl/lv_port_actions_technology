/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file codec config interface
 */
#include <media_service.h>
#include <string.h>
#include <section_overlay.h>

typedef struct
{
	uint8_t format;
	uint8_t codec_type;
	bool hw_acceleration;
	uint32_t overlay_id;
	void *ops;
	const char *dsp_dec_lib;
	const char *dsp_effect_lib;
} codec_config_info_t;

const codec_config_info_t codec_config[] =
{
#ifdef CONFIG_ACTIONS_DECODER
#ifdef CONFIG_DECODER_AAC
	{
		.format = AAC_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADAAC,
	#ifdef CONFIG_DECODER_AAC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_aac,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_M4A
	{
		.format = M4A_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADAAC,
	#ifdef CONFIG_DECODER_M4A_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_aac,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_ACT
	{
		.format = ACT_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = 0, //OVERLAY_ID_LIBADACT,
	#ifdef CONFIG_DECODER_ACT_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_act,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_APE
	{
		.format = APE_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADAPE,
	#ifdef CONFIG_DECODER_APE_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_ape,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_CVSD
	{
		.format = CVSD_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = 0,//OVERLAY_ID_LIBADCVS,
	#ifdef CONFIG_DECODER_CVSD_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		//.dsp_effect_lib = "PREPRO.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_decoder_ops_cvsd,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_FLAC
	{
		.format = FLA_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADFLA,
	#ifdef CONFIG_DECODER_FLAC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_flac,
	#endif
	},

#endif

#ifdef CONFIG_DECODER_MP3
	{
		.format = MP3_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADMP3,
	#ifdef CONFIG_DECODER_MP3_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_mp3,
	#endif

	},
#endif

#ifdef CONFIG_DECODER_WAV
	{
		.format = WAV_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADWAV,
	#ifdef CONFIG_DECODER_WAV_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		#ifdef CONFIG_ENCODER_MP3
		.dsp_dec_lib = "aeRECORD.dsp",
		#else
		.dsp_dec_lib = "adMUSIC.dsp",
		#endif
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_wav,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_WMA
	{
		.format = WMA_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = OVERLAY_ID_LIBADWMA,
	#ifdef CONFIG_DECODER_WMA_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_decoder_ops_wma,
	#endif
	},
#endif

#ifdef CONFIG_DECODER_PCM
	{
		.format = PCM_TYPE,
		.codec_type = CODEC_DEC,
		.hw_acceleration = true,
		.overlay_id = 0,
		.ops = NULL,
	#ifdef CONFIG_ENCODER_OPUS
		.dsp_dec_lib = "aeRECORD.dsp",
	#else
		.dsp_dec_lib = "adMUSIC.dsp",
	#endif
		.dsp_effect_lib = NULL,
	},
#endif

#ifdef CONFIG_DECODER_SBC
	{
		.format = SBC_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = 0, //OVERLAY_ID_LIBADSBC,
	#ifdef CONFIG_DECODER_SBC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_decoder_ops_sbc,
	#endif

	},
#endif
#ifdef CONFIG_DECODER_MSBC
	{
		.format = MSBC_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = 0, //OVERLAY_ID_LIBADSBC,
	#ifdef CONFIG_DECODER_MSBC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		//.dsp_effect_lib = "PREPRO.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_decoder_ops_sbc,
	#endif

	},
#endif
#ifdef CONFIG_DECODER_NAV
	{
		.format = NAV_TYPE,
		.codec_type = CODEC_DEC,
		.overlay_id = 0,
	#ifdef CONFIG_DECODER_NAV_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "audioble.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_encoder_ops_nav,
	#endif
	},
#endif
#endif

#ifdef CONFIG_ACTIONS_ENCODER
#ifdef CONFIG_ENCODER_CVSD
	{
		.format = CVSD_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = 0,//OVERLAY_ID_LIBAECVS,
	#ifdef CONFIG_ENCODER_CVSD_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = "PREPRO.dsp",
	#else
		.hw_acceleration = false,
		//.ops = as_encoder_ops_cvsd,
	#endif
	},
#endif
#ifdef CONFIG_ENCODER_MP3
	{
		.format = MP3_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = OVERLAY_ID_LIBAEMP3,
	#ifdef CONFIG_ENCODER_MP3_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "aeRECORD.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_encoder_ops_mp2,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_OPUS
	{
		.format = OPUS_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = OVERLAY_ID_LIBAEOPU,
	#ifdef CONFIG_ENCODER_OPUS_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "aeRECORD.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_encoder_ops_opus,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_AAC
	{
		.format = AAC_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = OVERLAY_ID_LIBAEOPU,
	#ifdef CONFIG_ENCODER_AAC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_encoder_ops_opus,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_WAV
	{
		.format = WAV_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = 0,//OVERLAY_ID_LIBAEWAV,
	#ifdef CONFIG_ENCODER_WAV_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		.ops = as_encoder_ops_wav,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_PCM
	{
		.format = PCM_TYPE,
		.codec_type = CODEC_ENC,
		.hw_acceleration = true,
		.overlay_id = 0,
		.ops = NULL,
	#ifdef CONFIG_ENCODER_OPUS
		.dsp_dec_lib = "aeRECORD.dsp",
	#else
		.dsp_dec_lib = "adMUSIC.dsp",
	#endif
		.dsp_effect_lib = NULL,
	},
#endif
#ifdef CONFIG_ENCODER_SBC
	{
		.format = SBC_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = 0,
	#ifdef CONFIG_ENCODER_SBC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		#ifdef CONFIG_ENCODER_MP3
		.dsp_dec_lib = "aeRECORD.dsp",
		#else
		.dsp_dec_lib = "adMUSIC.dsp",
		#endif
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_encoder_ops_sbc,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_MSBC
	{
		.format = MSBC_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = 0,
	#ifdef CONFIG_ENCODER_MSBC_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "adMUSIC.dsp",
		.dsp_effect_lib = "PREPRO.dsp",
	#else
		.hw_acceleration = false,
		//.ops = as_encoder_ops_sbc,
	#endif
	},
#endif

#ifdef CONFIG_ENCODER_NAV
	{
		.format = NAV_TYPE,
		.codec_type = CODEC_ENC,
		.overlay_id = 0,
	#ifdef CONFIG_ENCODER_NAV_HW_ACCELERATION
		.hw_acceleration = true,
		.ops = NULL,
		.dsp_dec_lib = "audioble.dsp",
		.dsp_effect_lib = NULL,
	#else
		.hw_acceleration = false,
		//.ops = as_encoder_ops_nav,
	#endif
	},
#endif
#endif
};

int codec_get_overlay_id(enum media_type format, enum codec_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(codec_config); i++) {
		if (format == codec_config[i].format
			&& type == codec_config[i].codec_type) {
			return codec_config[i].overlay_id;
		}
	}

	return -1;
}

void *codec_get_ops(enum media_type format, enum codec_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(codec_config); i++) {
		if (format == codec_config[i].format
			&& type == codec_config[i].codec_type) {
			return codec_config[i].ops;
		}
	}

	return NULL;
}

bool codec_support_hw_acceleration(enum media_type format, enum codec_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(codec_config); i++) {
		if (format == codec_config[i].format
			&& type == codec_config[i].codec_type) {

			return codec_config[i].hw_acceleration;
		}
	}

	return false;
}

const char *codec_get_hw_acceleration_dec_lib(enum media_type format, enum codec_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(codec_config); i++) {
		if (format == codec_config[i].format
			&& type == codec_config[i].codec_type) {
			return codec_config[i].dsp_dec_lib;
		}
	}

	return NULL;
}

const char *codec_get_hw_acceleration_effect_lib(enum media_type format, enum codec_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(codec_config); i++) {
		if (format == codec_config[i].format
			&& type == codec_config[i].codec_type) {
			return codec_config[i].dsp_effect_lib;
		}
	}

	return NULL;
}

