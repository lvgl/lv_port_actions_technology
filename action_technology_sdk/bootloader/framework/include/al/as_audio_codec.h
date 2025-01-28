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

 * Author: jpl<jianpingliao@actions-semi.com>
 *
 * Change log:
 *	2019/12/16: Created by jpl.
 */

#ifndef __AS_AUDIO_CODEC_H__
#define __AS_AUDIO_CODEC_H__

#include <stdint.h>
#include "al_storage_io.h"
#include "lib_a1_ape_p.h"
#include "lib_a1_fla_p.h"
#include "lib_a1_wav_e.h"
#include "lib_a1_wav_p.h"
#include "lib_a1_w13_p.h"

/**
 * @defgroup arithmetic_codec_apis arithmetic codec APIs
 * @ingroup mem_managers
 * @{
 */

/*!
 * \brief
 *      maximum audio output channels
 */
#define MAX_CHANNEL_OUT     2

/*!
 * \brief
 *      audio (decoder) pcm output structure
 */
typedef struct
{
	/*! [OUTPUT] address of pcm buffer of each channel */
	void *pcm[MAX_CHANNEL_OUT];
	/*! [OUTPUT] number of pcm channels */
	int channels;
	/*! [OUTPUT] number of pcm samples of each channel */
	int samples;
	/*! [OUTPUT] number of pcm sample bits, 16/24/32 */
	int sample_bits;
	/*! [OUTPUT] number of pcm sample fraction bits */
	int frac_bits;
} asout_pcm_t;


/*!
 * \brief
 *      maximum audio input channels
 */
#define MAX_CHANNEL_IN    4

/*!
 * \brief
 *      audio (encoder) pcm input structure
 */
typedef struct
{
	/*! [INPUT] address of pcm buffer of each channel */
	void *pcm[MAX_CHANNEL_IN];
	/*! [INPUT] number of pcm channels */
	int channels;
	/*! [INPUT] number of pcm samples of each channel */
	int samples;
	/*! [INPUT] number of pcm sample bits, 16/24/32 */
	int sample_bits;
	/*! [INPUT] number of pcm sample fraction bits */
	int frac_bits;
} asin_pcm_t;

#if 0 /* see ext/actions/include/media/media_type.h */
typedef enum
{
	UNSUPPORT_TYPE = 0,
	MP3_TYPE,
	WMA_TYPE,
	WAV_TYPE, // 3
	FLAC_TYPE,
	APE_TYPE,
	AAC_TYPE, // 6
	OGG_TYPE,
	ACT_TYPE,
	AMR_TYPE, // 9
	SBC_TYPE,
	MSBC_TYPE,
	OPUS_TYPE, // 12
	AAX_TYPE,
	AA_TYPE,
	CVSD_TYPE, // 15
	SPEEX_TYPE,

	// raw pcm format
	PCM_TYPE, // 17
} as_type_t;
#endif


/*******************************************************************************
 * actions decoder
 ******************************************************************************/

/*!
 * \brief
 *      music information by audio decoder
 */
typedef struct
{
	/*! [OUTPUT]: music format, see enum as_type_t */
	int as_type;
	/*! [OUTPUT]: total time in ms */
	int total_time;
	/*! [OUTPUT]: average bitrate in kbps */
	int avg_bitrate;
	/*! [OUTPUT]: sample rate in kHz */
	int sample_rate;
	/*! [OUTPUT]: sample rate in Hz */
	int sample_rate_hz;
	/*! [OUTPUT]: number of channels */
	int channels;
	/*! [OUTPUT]: sample bits */
	int bps;
	/*! [OUTPUT]: number of samples per block */
	int block_size;
	/*! [OUTPUT]: number of samples per frame, mp3 frame/wma packet/ogg page */
	int frame_size;
	/*! [OUTPUT]: sub format */
	int sub_format;

	/*! [OUTPUT]: file length in bytes */
	unsigned int file_len;
	/*! [OUTPUT]: file header lenght inbytes */
	unsigned int file_hdr_len;

	/*! address of data buffer used internally in library */
	void *data_buf_addr;
	/*! length of data buffer used internally in library */
	int data_buf_len;

	/*! [INPUT]: drm flag, indicate file is encrypted or not. 1-encrypted, 0-not encrypted,
	 * may used in AA, AAX format.
	 */
	int drm_flag;
	/*! [INPUT]: address of drm secret key buffer, required when drm_flag=1 */
	void *key_buf;

	/*! [INPUT]: storage io handle, see struct storage_io_s */
	void *storage_io;

	/*! [INPUT]: decoder specific buffer */
	void *buf;

	/*! [INPUT]: decode mode
	 * 1) if used as tts, set to 0x0abcd, no matter what format
	 * 2) for aac format, 0 stands for locall file decoding, while 1 is BT stream decoding.
	 */
	int decode_mode;

	/*! [INPUT] global memory block address array */
	void *global_buf_addr[3];
	/*! [INPUT] global memory block length array */
	int global_buf_len[3];

	/*! [INPUT] share memory block address array */
	void *share_buf_addr[3];
	/*! [INPUT] share memory block length array */
	int  share_buf_len[3];

	/*! [INPUT]: expected pcm sample bits of decode output (16/24/32). 16 by default */
	int set_pcm_out_bits;

	/*[INPUT]: 0: accurate seek, but slower; 1: quick seek, but bigger deviation */
	int seek_mode;

	/*! [INPUT]: extended parameter, whichi is decoder specific */
	void *param;
} as_dec_t;

typedef struct
{
	/* [OUTPUT] global memory block length array, 3 blocks at maximum */
	int global_buf_len[3];
	/* [OUTPUT] share memory block length array, 3 blocks at maximum */
	int share_buf_len[3];

	/* [INPUT] extended parameters
	 * for encoder: passed address of structure as_enc_t.
	 */
	void* param;
} as_mem_info_t;

/*!
 * \brief
 *      music break point information by audio decoder
 *      For WAV/MP3/WMA, bp_time_offset and bp_file_offset should be consistent.
 */
typedef struct
{
	/*! break point time in ms. when used in seeking, it stands for the target time */
	int bp_time_offset;
	/*! break point file positon in bytes */
	unsigned int bp_file_offset;
	/*! auxiliary information for resuming break point */
	int bp_info_ext;
} as_bp_info_t;

/*!
 * \brief
 *      decoding frame information by audio decoder
 */
typedef struct
{
	/* start address of current frame data */
	void *addr;
	/* length of current frame data */
	int len;
	/* bit width of data unit */
	int sample_size;
	/* status of current frame, like data lost status when decoding BT stream */
	int status;
} as_dec_frame_info_t;

/*!
 * \brief
 *      decoding information by audio decoder
 */
typedef struct
{
	/*! [OUTPUT]: decoder status. 0: normal, otherwise error */
	int status;
	/*! [OUTPUT]: current decoding time in ms */
	int cur_time;
	/*! [OUTPUT]: current decoding bitrate in kbps */
	int cur_bitrate;
	/*! [OUTPUT]: current energy */
	int cur_energy;
	/*! [OUTPUT]: address of decoding output pcm structure. set by caller, filled by decoder */
	asout_pcm_t *aout;
	/*! [OUTPUT]: current decoding file positon in bytes */
	unsigned int cur_file_offset;
	/*! [OUTPUT]: length of decoding consumed data in bytes */
	int bytes_used;
	/*! [OUTPUT]: break point infomation corresponding to current decoding time */
	as_bp_info_t as_bp_info;

	/*! [OUTPUT]: reserved */
	void *param;
} as_decode_info_t;

/** as decoder cmd */
typedef enum
{
	/** decoder open */
	AD_CMD_OPEN = 0,
	/** decoder close */
	AD_CMD_CLOSE,

	/** get media info */
	AD_CMD_MEDIA_INFO,
	/** reset chunk  */
	AD_CMD_CHUNK_RESET,

	/** decode frame */
	AD_CMD_FRAME_DECODE,
	/** chunk seek */
	AD_CMD_CHUNK_SEEK,
	/*set output channel, such as sbc*/
	AD_CMD_CHANNEL_SET,

	/** get memory require size(bytes) */
	AD_CMD_MEM_REQUIRE,
	/** parser frame */
	AD_CMD_FRAME_PARSE,
	/** get codec verison */
	AD_CMD_GET_VERSION,
	/** get codec err info */
	AD_CMD_GET_ERROR_INFO,

	/*drm init*/
	AD_CMD_DRM_INIT,
} asdec_ex_ops_cmd_t;

/** audio decoder return state */
typedef enum
{
	/** unexpected state */
	AD_RET_UNEXPECTED = -3,
	/** outof memory state */
	AD_RET_OUTOFMEMORY,
	/** unsupported state */
	AD_RET_UNSUPPORTED,
	/** ok state */
	AD_RET_OK,
	/** data underflow state */
	AD_RET_DATAUNDERFLOW,
} asdec_ret_t;

/** audio decoder ops prototype */
typedef int (*as_decoder_ops_t)(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);


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
int as_decoder_format_check(void *storage_io, char extension[8]);

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
int as_decoder_ops_aac(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_act(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_ape(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_cvsd(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_flac(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_mp3(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_pcm(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_sbc(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_wav(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);

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
int as_decoder_ops_wma(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args);


/*******************************************************************************
 * actions encoder
 ******************************************************************************/

/*!
 * \brief
 *      encoding configuration provided to audio encoder
 */
typedef struct
{
	/*!  [INPUT]: sample rate in Hz */
	int sample_rate;
	/*! [INPUT]: input pcm channels */
	int channels;
	/*! [INPUT]: output encoding channels. the same as channels by default */
	int channels_out;
	/*! [INPUT]: target bitrate */
	int bitrate;
	/*! [INPUT]: encoding sub format
	 * For wav: 0-LPCM, 1-ADPCM
	 * For sbc: 0-sbc, 1-msbc
	 */
	int audio_format;
	/*! [INPUT]: chunk time interval in ms per encoding frame. if set 0, no limit */
	int chunk_time;
	/*! [OUTPUT]: input pcm buffer length of each channel per frame in bytes */
	int block_size;
	/*! [OUTPUT]: output encoded data length per frame in bytes */
	int block_enc_size_max;

	/*! [INPUT] global memory block address array */
	void *global_buf_addr[3];
	/*! [INPUT] global memory block length array */
	int global_buf_len[3];

	/*! [INPUT] share memory block address array */
	void *share_buf_addr[3];
	/*! [INPUT] share memory block length array */
	int  share_buf_len[3];

	/* [INPUT]: input pcm sample bits. 16/24/32, 16 by default */
	int sample_bits;

	/*! [INPUT]: encoder specific parameters */
	void *param;
} as_enc_t;

/*!
 * \brief
 *      encoding information by audio encoder
 */
typedef struct
{
	/*! [OUTPUT]: encoding status. 0-normal, otherwise error */
	int status;
	/*! [OUTPUT]: current encoding time in ms*/
	int cur_time;
	/*! [INPUT]: current encoding energy */
	int cur_energy;
	/*! [INPUT]: slient flag */
	int silence_flag;
	/*! [INPUT]: address of input pcm structure */
	asin_pcm_t *ain;
	/*! [INPUT]: address of output encoded data. set by caller, filled by encoder */
	void *aout;
	/*! [OUTPUT]: length of output encoded data in bytes */
	int bytes_used;
	/* [OUTPUT]: length of remain non-encoded data in bytes */
	int bytes_remain;

	/*![OUTPUT]: reserved */
	void *param;
} as_encode_info_t;

typedef struct
{
	/*![OUTPUT]: address of header buffer */
	void *hdr_buf;
	/*![OUTPUT]: length of header buffer */
	int hdr_len;
} as_enc_fhdr_t;

/*
*  opus encoder specific parameter
*/
typedef struct {
	/*! [INPUT]: application mode: 2048 (VOIP), 2049 (AUDIO), 2051 (RESTRICTED_LOWDELAY)*/
	int application;
	/*! [INPUT]: comlexity, range 0~10 */
	int complexity;
	/*! [INPUT]: vbr flag. 1: variable bitrate; 0: constant bitrate*/
	int vbr;
} as_enc_opus_param_t;

/** audio encorder cmd */
typedef enum
{
	/** open encoder library  */
	AE_CMD_OPEN = 0,
	/** close encoder library  */
	AE_CMD_CLOSE,

	/** decoder frame */
	AE_CMD_FRAME_ENCODE,
	/** get memory require size(bytes) */
	AE_CMD_MEM_REQUIRE,
	/**get codec verison**/
	AE_CMD_GET_VERSION,
	/**get encoded file header**/
	AE_CMD_GET_FHDR,
} asenc_ex_ops_cmd_t;

/** audio decoder state */
typedef enum
{
	/** unkown expected */
	AE_RET_UNEXPECTED = -7,
	/** load library failed */
	AE_RET_LIBLOAD_ERROR,
	/** load error library */
	AE_RET_LIB_ERROR,
	/** encoder error */
	AE_RET_ENCODER_ERROR,
	/** sample rate error */
	AE_RET_FS_ERROR,
	/** out of memory*/
	AE_RET_OUTOFMEMORY,
	/** format not support*/
	AE_RET_UNSUPPORTED,
	/** success */
	AE_RET_OK,
	/** data over flow*/
	AE_RET_DATAOVERFLOW,
	/** out of data */
	AE_RET_OUTOFDATA,
} asenc_ret_t;

/** audio encoder ops prototype */
typedef int (*as_encoder_ops_t)(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);


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
int as_encoder_ops_cvsd(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);

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
int as_encoder_ops_mp2(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);

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
int as_encoder_ops_opus(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);

/**
 * @brief pcm encoder operation
 *
 * This routine provides pcm encoder operation
 *
 * @param hnd  handle of pcm encoder
 * @param cmd  operation cmd, type of asenc_ex_ops_cmd_t
 * @param args args of encoder parama addr
 *
 * @return type of asenc_ret_t
 */
int as_encoder_ops_pcm(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);

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
int as_encoder_ops_sbc(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);

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
int as_encoder_ops_wav(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args);


/*******************************************************************************
 * actions parser
 ******************************************************************************/

/*!
 * \brief
 *      audio parser ops error code
 */
typedef enum
{
	/*! unexpected error */
	AP_RET_UNEXPECTED = -3,
	/*! not enough memory */
	AP_RET_OUTOFMEMORY,
	/*! unsupported format */
	AP_RET_UNSUPPORTED,
	/*! no error */
	AP_RET_OK = 0,
	/*! end of file */
	AP_RET_ENDFILE
} asparse_ret_t;

/** audio parser cmd */
typedef enum
{
	AP_CMD_OPEN = 0,
	AP_CMD_CLOSE,
	AP_CMD_PARSER_HEADER,
	AP_CMD_GET_CHUNK,
	AP_CMD_SEEK,
	AP_CMD_EX_OPS,
	AP_CMD_FORCE_ENDFILE,
} asparse_ex_ops_cmd_t;
/*!
 * \brief
 *      parser cmd AP_CMD_OPEN argument
 */
typedef struct
{
	/* provided to access the raw stream */
	storage_io_t *storage_io;
	/* TYPE: struct acts_ringbuf, which store request/feedback from codec */
	void *evtbuf;
} asparse_param_t;

/*!
 * \brief
 *      music information by audio parser, and argument of cmd AP_CMD_PARSER_HEADER
 */
typedef struct
{
	/*! [OUTPUT]: format extension, upper case, like "COOK" */
	char extension[8];
	/*! [OUTPUT]: maximum chunk size in bytes */
	int max_chunksize;
	/*! [OUTPUT]: total time in milliseconds */
	int total_time;
	/*! [OUTPUT]: average bitrate in kbps */
	int avg_bitrate;
	/*! [OUTPUT]: sample rate in Hz */
	int sample_rate;
	/*! [OUTPUT]: channels */
	int channels;
	/*! [OUTPUT]: parser specific information */
	void *buf;
	int buf_len;
	/*![OUTPUT] file head length*/
	int hdr_len;
	/*![OUTPUT]  bit width*/
	int sample_bits;
} asparse_info_t;

/*!
 * \brief
 *      parser cmd AP_CMD_GET_CHUNK argument
 */
typedef struct
{
	/* chunk buffer */
	uint8_t *outbuf;
	uint16_t chunk_bytes;
	int16_t  chunk_rest;

	/* chunk seek information */
	int32_t chunk_start_time;
	int32_t chunk_start_offset;
} asparse_bs_info_t;

/*!
 * \brief
 *      parser cmd AP_CMD_SEEK argument
 */
typedef struct
{
	/* decide the seek possiton */
	int32_t time_offset;
	int32_t origin;

	/* store the feedback of parser lib according to the seek position */
	int32_t chunk_start_time;

	/* used to accelerate the seeking speed
	 *  >= 0, valid file offset, can be used to speed up seeking
	 *  <0, invalid file offset
	 */
	int32_t file_offset_for_time_offset; /* measured in bytes */
} asparse_seek_info_t;

#define AP_SEEK_SET    0
#define AP_SEEK_CUR    1
#define AP_SEEK_END    2

#define AP_TELL_CUR    0
#define AP_TELL_END    1

/** audio parser ops prototype */
typedef int (*as_parser_ops_t)(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);
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
int as_parser_ops_ape(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

/**
 * @brief flac parser operation
 *
 * This routine provides flac parser operation
 *
 * @param hnd  handle of flac parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
int as_parser_ops_flac(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

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
int as_parser_ops_mp3(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

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
int as_parser_ops_wav(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

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
int as_parser_ops_wma(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

/**
 * @brief aac parser operation
 *
 * This routine provides aac parser operation
 *
 * @param hnd  handle of aac parser
 * @param cmd  operation cmd, type of asparse_ex_ops_cmd_t
 * @param args args of parser parama addr
 *
 * @return type of asparse_ret_t
 */
int as_parser_ops_a13(void *hnd, asparse_ex_ops_cmd_t cmd, unsigned int args);

#endif /* __AS_AUDIO_CODEC_H__ */
