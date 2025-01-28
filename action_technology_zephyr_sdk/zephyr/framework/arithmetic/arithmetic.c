/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

 * Author: wh<wanghui@actions-semi.com>
 *
 * Change log:
 *	2018/1/20: Created by wh.
 */

/**
 * @defgroup arithmetic_apis arithmetic APIs
 * @ingroup mem_managers
 * @{
 */

#include <os_common_api.h>
#include <arithmetic.h>

/*******************************************************************************
 * actions decoder ops
 ******************************************************************************/

/**
 * @brief check real audio format
 *
 * This routine provides checking audio format
 *
 * @param storage_io pointer of storage io
 * @extension buffer (require 8 bytes) to store format extension, all lower case, like mp3, wav, flac, etc.
 *
 * @return 0 if succeed, others failed
 */
#ifndef CONFIG_AL_FORMAT_CHECK
int as_decoder_format_check(void *storage_io, char extension[8])
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief aac decoder operation
 *
 * This routine provides aac decoder operation
 *
 * @param hnd  handle of aac decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_AAC
int as_decoder_ops_aac(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief act decoder operation
 *
 * This routine provides act decoder operation
 *
 * @param hnd  handle of act decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_ACT
int as_decoder_ops_act(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief ape decoder operation
 *
 * This routine provides ape decoder operation
 *
 * @param hnd  handle of ape decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_APE
int as_decoder_ops_ape(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief cvsd decoder operation
 *
 * This routine provides cvsd decoder operation
 *
 * @param hnd  handle of cvsd decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_CVSD
int as_decoder_ops_cvsd(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief flac decoder operation
 *
 * This routine provides flac decoder operation
 *
 * @param hnd  handle of flac decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_FLAC
int as_decoder_ops_flac(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief mp3 decoder operation
 *
 * This routine provides mp3 decoder operation
 *
 * @param hnd  handle of mp3 decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_MP3
int as_decoder_ops_mp3(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief pcm decoder operation
 *
 * This routine provides pcm decoder operation
 *
 * @param hnd  handle of pcm decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_PCM
int as_decoder_ops_pcm(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief sbc decoder operation
 *
 * This routine provides sbc decoder operation
 *
 * @param hnd  handle of sbc decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_SBC
int as_decoder_ops_sbc(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief wav decoder operation
 *
 * This routine provides wav decoder operation
 *
 * @param hnd  handle of wav decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_WAV
int as_decoder_ops_wav(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif

/**
 * @brief wma decoder operation
 *
 * This routine provides wma decoder operation
 *
 * @param hnd  handle of wma decoder
 * @param cmd  operation cmd, type of asdec_ex_ops_cmd_t
 * @param args args of decoder parama addr
 *
 * @return type of asdec_ret_t
 */
#ifndef CONFIG_DECODER_WMA
int as_decoder_ops_wma(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AD_RET_UNEXPECTED;
}
#endif


/*******************************************************************************
 * actions encoder ops
 ******************************************************************************/

/**
 * @brief cvsd encoder operation
 *
 * This routine provides cvsd encoder operation
 *
 * @param hnd  handle of cvsd encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_CVSD
int as_encoder_ops_cvsd(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif

/**
 * @brief mp3 encoder operation
 *
 * This routine provides mp3 encoder operation
 *
 * @param hnd  handle of mp3 encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_MP3
int as_encoder_ops_mp2(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif

/**
 * @brief opus encoder operation
 *
 * This routine provides opus encoder operation
 *
 * @param hnd  handle of opus encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_OPUS
int as_encoder_ops_opus(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif

/**
 * @brief pcm encoder operation
 *
 * This routine provides pcm encoder operation
 *
 * @param hnd  handle of act encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_PCM
int as_encoder_ops_pcm(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif

/**
 * @brief sbc encoder operation
 *
 * This routine provides sbc encoder operation
 *
 * @param hnd  handle of sbc encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_SBC
int as_encoder_ops_sbc(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif

/**
 * @brief wav encoder operation
 *
 * This routine provides wav encoder operation
 *
 * @param hnd  handle of wav encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
#ifndef CONFIG_ENCODER_WAV
int as_encoder_ops_wav(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AE_RET_UNEXPECTED;
}
#endif


/*******************************************************************************
 * actions parser ops
 ******************************************************************************/

/**
 * @brief mp3 parser operation
 *
 * This routine provides mp3 parser operation
 *
 * @param hnd  handle of mp3 parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_MP3
int as_parser_ops_mp3(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif

/**
 * @brief wav parser operation
 *
 * This routine provides wav parser operation
 *
 * @param hnd  handle of wav parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_WAV
int as_parser_ops_wav(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif


/**
 * @brief aac parser operation
 *
 * This routine provides wav parser operation
 *
 * @param hnd  handle of wav parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_M4A
int as_parser_ops_a13(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif


/**
 * @brief ape parser operation
 *
 * This routine provides ape parser operation
 *
 * @param hnd  handle of ape parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_APE
int as_parser_ops_ape(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif


/**
 * @brief falc parser operation
 *
 * This routine provides falc parser operation
 *
 * @param hnd  handle of falc parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_FLAC
int as_parser_ops_flac(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif


/**
 * @brief wma parser operation
 *
 * This routine provides wma parser operation
 *
 * @param hnd  handle of wma parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
#ifndef CONFIG_PARSER_WMA
int as_parser_ops_wma(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AP_RET_UNEXPECTED;
}
#endif


/*******************************************************************************
 * actions hfp speech ops
 ******************************************************************************/

/**
 * @brief hfp plc operation
 *
 * This routine provides hfp plc operation
 *
 * @param hnd  handle hfp plc
 * @param cmd  operation cmd, type of plc_ex_ops_cmd_t
 * @param args args of hfp plc parama addr
 *
 * @return type of hs_ret_t;
 */
#ifndef CONFIG_HFP_PLC
int hfp_plc_ops(void *hnd, plc_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return HS_RET_UNEXPECTED;
}
#endif

/**
 * @brief hfp speech operation
 *
 * This routine provides hfp speech operation
 *
 * @param hnd  handle of hfp speech
 * @param cmd  operation cmd, type of hfp_ex_ops_cmd_t
 * @param args args of hfp speech parama addr
 *
 * @return type of hs_ret_t;
 */
#ifndef CONFIG_HFP_SPEECH
int hfp_speech_ops(void *hnd, hfp_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return HS_RET_UNEXPECTED;
}
#endif

/**
 * @brief as hfp dae operation
 *
 * This routine provides as hfp dae operation
 *
 * @param hnd  handle of as hfp dae
 * @param cmd  operation cmd, type of as_dae_h_ex_ops_cmd_t
 * 				for DAE_H_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for DAE_H_CMD_OPEN, args type is as_dae_h_open_t*
 * 				for DAE_H_CMD_SET_DAE_PARA, args type is as_dae_h_para_info_t* in as_dae_h_setting.h
 * 				for DAE_H_CMD_FRAME_PROCESS_SPEAKER, args type is as_dae_h_inout_pcm_t*
 * 				for DAE_H_CMD_FRAME_PROCESS_MIC, args type is as_dae_h_inout_pcm_t*
 * @param args args of dae parama addr
 *
 * @return type of as_dae_h_ret_t;
 */
#ifndef CONFIG_HFP_DAE
int as_dae_h_ops(void *hnd, as_dae_h_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return DAE_H_RET_UNEXPECTED;
}
#endif


/*******************************************************************************
 * actions dae
 ******************************************************************************/

/**
 * @brief as dae operation
 *
 * This routine provides as dae operation
 *
 * @param hnd  handle of as dae
 * @param cmd  operation cmd, type of as_dae_ex_ops_cmd_t
 * 				for DAE_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for DAE_CMD_OPEN, args for reserve
 * 				for DAE_CMD_SET_DAE_PARA, args type is as_dae_para_info_t* in as_dae_setting.h
 * 				for DAE_CMD_FRAME_PROCESS, args type is as_dae_inout_pcm_t*
 * 				for DAE_CMD_GET_STATUS, args type is as_dae_status_t*
 * @param args args of dae parama addr
 *
 * @return type of as_dae_ret_t;
 */
#ifndef CONFIG_MUSIC_DAE
int as_dae_ops(void *hnd, as_dae_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return DAE_RET_UNEXPECTED;
}
#endif

/**
 * @brief as dae fade operation
 *
 * This routine provides as dae fade operation
 *
 * @param hnd handle of as dae fade
 * @param cmd operation cmd, type of as_dae_ex_ops_cmd_t
 * 				for AS_FADE_P_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for AS_FADE_P_CMD_OPEN, args type is as_fade_p_open_t*
 * 				for AS_FADE_P_CMD_FRAME_PROCESS, args type is as_fade_p_pcm_t*
 * @param args args of dae fade parama addr
 *
 * @return type of as_fade_p_ret_t;
 */
#ifndef CONFIG_MUSIC_DAE_FADE
int as_fade_p_ops(void *hnd, as_fade_p_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AS_FADE_P_RET_UNEXPECTED;
}
#endif

/*******************************************************************************
 * others
 ******************************************************************************/

/**
 * @brief as mix operation
 *
 * This routine provides as mix operation
 *
 * @param hnd handle of as mix
 * @param cmd  operation cmd, type of as_dae_ex_ops_cmd_t
 * 				for AS_MIX_P_CMD_MEM_REQUIRE, args type is as_mem_info_t* in audio_codec.h
 * 				for AS_MIX_P_CMD_OPEN, args type is as_mix_p_open_t*
 * 				for AS_MIX_P_CMD_FRAME_PROCESS, args type is as_mix_p_pcm_t*
 * @param args args of mix parama addr
 *
 * @return type of as_mix_p_ret_t;
 */
#ifndef CONFIG_AUDIO_MIX
int as_mix_p_ops(void *hnd, as_mix_p_ex_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return AS_MIX_P_RET_UNEXPECTED;
}
#endif

/**
 * @brief as resample operation
 *
 * This routine provides as resample operation
 *
 * @param hnd  handle of as resample
 * @param cmd  operation cmd, type of as_res_ops_cmd_t
 * @param args args of resample parama addr
 *
 * @return 0 if successful, others failed;
 */
#ifndef CONFIG_RESAMPLE
int as_res_ops(void *hnd, as_res_ops_cmd_t cmd, unsigned int args)
{
	SYS_LOG_WRN("this function not support");
	return -1;
}
#endif


