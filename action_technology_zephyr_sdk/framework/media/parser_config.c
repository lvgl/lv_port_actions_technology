/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief playback decode config.
*/


#include <section_overlay.h>
#include <arithmetic.h>
#include <media_service.h>

static const parser_config_info_t parser_config[] =
{
#ifdef CONFIG_PARSER_APE
	{
		.type = APE_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPAPE,
		.ops = as_parser_ops_ape,
	},
#endif

#ifdef CONFIG_PARSER_FLAC
	{
		.type = FLA_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPFLA,
		.ops = as_parser_ops_flac,
	},
#endif

#ifdef CONFIG_PARSER_MP3
	{
		.type = MP3_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPMP3,
		.ops = as_parser_ops_mp3,
	},
#endif

#ifdef CONFIG_PARSER_WAV
	{
		.type = WAV_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPWAV,
		.ops = as_parser_ops_wav,
	},
#endif

#ifdef CONFIG_PARSER_WMA
	{
		.type = WMA_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPWMA,
		.ops = as_parser_ops_wma,
	},
#endif
#ifdef CONFIG_PARSER_M4A
	{
		.type = M4A_TYPE,
		.overlay_id = OVERLAY_ID_LIBAPAAC,
		.ops = as_parser_ops_a13,
	},
#endif
};

const parser_config_info_t *parser_get_config(media_type_e type)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(parser_config); i++) {
		if (type == parser_config[i].type) {
			return &parser_config[i];
		}
	}

	return NULL;
}

