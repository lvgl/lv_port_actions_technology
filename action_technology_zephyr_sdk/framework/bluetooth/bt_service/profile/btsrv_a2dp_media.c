/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt service a2dp media
 */

#define SYS_LOG_DOMAIN "btsrv_a2dp"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

#define SBC_HEADER_LEN 4

#define SBC_SYNCWORD			0x9c
#define SBC_ENHANCED_SYNCWORD	0x9d

/**@name Sampling frequencies */
/**@{*/
#define SBC_FREQ_16000 0 /**< The sampling frequency is 16 kHz. One possible value for the @a frequency parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_FREQ_32000 1 /**< The sampling frequency is 32 kHz. One possible value for the @a frequency parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_FREQ_44100 2 /**< The sampling frequency is 44.1 kHz. One possible value for the @a frequency parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_FREQ_48000 3 /**< The sampling frequency is 48 kHz. One possible value for the @a frequency parameter of OI_CODEC_SBC_EncoderConfigure() */
/**@}*/

/**@name Channel modes */
/**@{*/
#define SBC_MONO 0         /**< The mode of the encoded channel is mono. One possible value for the @a mode parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_DUAL_CHANNEL 1 /**< The mode of the encoded channel is dual-channel. One possible value for the @a mode parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_STEREO 2       /**< The mode of the encoded channel is stereo. One possible value for the @a mode parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_JOINT_STEREO 3 /**< The mode of the encoded channel is joint stereo. One possible value for the @a mode parameter of OI_CODEC_SBC_EncoderConfigure() */
/**@}*/

/**@name Subbands */
/**@{*/
#define SBC_SUBBANDS_4  0 /**< The encoded stream has 4 subbands. One possible value for the @a subbands parameter of OI_CODEC_SBC_EncoderConfigure()*/
#define SBC_SUBBANDS_8  1 /**< The encoded stream has 8 subbands. One possible value for the @a subbands parameter of OI_CODEC_SBC_EncoderConfigure() */
/**@}*/

/**@name Block lengths */
/**@{*/
#define SBC_BLOCKS_4    0 /**< A block size of 4 blocks was used to encode the stream. One possible value for the @a blocks parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_BLOCKS_8    1 /**< A block size of 8 blocks was used to encode the stream is. One possible value for the @a blocks parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_BLOCKS_12   2 /**< A block size of 12 blocks was used to encode the stream. One possible value for the @a blocks parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_BLOCKS_16   3 /**< A block size of 16 blocks was used to encode the stream. One possible value for the @a blocks parameter of OI_CODEC_SBC_EncoderConfigure() */
/**@}*/

/**@name Bit allocation methods */
/**@{*/
#define SBC_LOUDNESS 0    /**< The bit allocation method. One possible value for the @a loudness parameter of OI_CODEC_SBC_EncoderConfigure() */
#define SBC_SNR 1         /**< The bit allocation method. One possible value for the @a loudness parameter of OI_CODEC_SBC_EncoderConfigure() */
/**@}*/

struct sbc_head {
	uint8_t sync_work;
	uint8_t subbands:1;
	uint8_t method:1;
	uint8_t mode:2;
	uint8_t blocks:2;
	uint8_t sample:2;
	uint8_t bitpool;
	uint8_t crc;
};

#define AAC_HEADER_LEN 6

static uint16_t _btsrv_tws_sbc_cal_frame_len(uint8_t *data)
{
	struct sbc_head *p_head = (struct sbc_head *)data;
	uint8_t mode = p_head->mode;
	uint8_t nrof_blocks = 4*(p_head->blocks + 1);
	uint16_t nrof_subbands = 4*(p_head->subbands + 1);
	uint16_t nbits = nrof_blocks * p_head->bitpool;
	uint16_t result = nbits;

	if (mode == SBC_JOINT_STEREO) {
		result += nrof_subbands + (8 * nrof_subbands);
	} else {
		if (mode == SBC_DUAL_CHANNEL) {
			result += nbits;
		}

		if (mode == SBC_MONO) {
			result += 4*nrof_subbands;
		} else {
			result += 8*nrof_subbands;
		}
	}

	return SBC_HEADER_LEN + (result + 7) / 8;
}

static int _btsrv_a2dp_sbc_parser_frame_info(uint8_t *data, uint16_t len, uint16_t *f_len, uint16_t *f_cnt)
{
	uint8_t *pData = data;
	uint16_t frame_num = 0;
	uint16_t frame_len = len;
	uint16_t total_len = len;

	while (len > SBC_HEADER_LEN) {
		if (pData[0] != SBC_SYNCWORD) {
			return -ENOEXEC;
		}

		frame_len = _btsrv_tws_sbc_cal_frame_len(pData);
		if (frame_len > len) {
			return -ENOEXEC;
		}

		frame_num++;
		pData += frame_len;
		len -= frame_len;
	}

	if (frame_len * frame_num != total_len) {
		return -ENOEXEC;
	}
	*f_len = frame_len;
	*f_cnt = frame_num;
	return 0;
}

static int _btsrv_a2dp_aac_parser_frame_info(uint8_t *data, uint16_t len, uint16_t *f_len, uint16_t *f_cnt)
{
	uint8_t *pData = data;
	uint8_t temp;
	uint16_t frame_num = 0;
	uint16_t frame_len = len;
	uint16_t total_len = 0;
	uint16_t size_offset;
	uint16_t ori_len = len;

	while (len > AAC_HEADER_LEN) {
		if ((pData[0] == 0xb0) && (pData[1] == 0x90 || pData[1] == 0x8c) &&
			(pData[2] == 0x80) && (pData[3] == 0x03) && (pData[4] == 0x00)) {
			frame_len = 5;
			for (size_offset = 0; size_offset < 4; size_offset++){
				temp = pData[5 + size_offset];
				frame_len += temp;
				if (temp != 0xff){
					size_offset++;
					break;
				}
			}

			frame_len += size_offset;
			total_len += frame_len;

			frame_num++;
			pData += frame_len;
			len -= frame_len;
		} else {
			return -ENOEXEC;
		}
	}

	if (total_len != ori_len) {
		return -ENOEXEC;
	}

	*f_len = frame_len;
	*f_cnt = frame_num;
	return 0;
}

int btsrv_a2dp_media_parser_frame_info(uint8_t codec_id, uint8_t *data, uint32_t data_len, uint16_t *frame_cnt, uint16_t *frame_len)
{
	int ret = 0;

	switch (codec_id) {
	case BT_A2DP_SBC:
		ret = _btsrv_a2dp_sbc_parser_frame_info(data, data_len, frame_cnt, frame_len);
		break;
	case BT_A2DP_MPEG2:
		ret = _btsrv_a2dp_aac_parser_frame_info(data, data_len, frame_cnt, frame_len);
		break;
	default:
		break;
	}
	return ret;
}

static uint32_t btsrv_a2dp_sbc_cal_frame_time_us(uint8_t *data)
{
	struct sbc_head *p_head = (struct sbc_head *)data;
	uint8_t nrof_blocks = 4*(p_head->blocks + 1);
	uint16_t nrof_subbands = 4*(p_head->subbands + 1);
	uint32_t frame_time, samples;

	frame_time = nrof_blocks*nrof_subbands * 1000 * 1000;

	switch (p_head->sample) {
	case SBC_FREQ_16000:
		samples = 16000;
		break;
	case SBC_FREQ_32000:
		samples = 32000;
		break;
	case SBC_FREQ_44100:
		samples = 44100;
		break;
	case SBC_FREQ_48000:
		samples = 48000;
		break;
	default:
		samples = 44100;
		break;
	}

	return (frame_time/samples);
}

static uint32_t btsrv_a2dp_aac_cal_frame_time_us(uint8_t *data)
{
	uint32_t samples, time_us;

	if (data[1] == 0x90) {
		samples = 44100;
	} else {
		samples = 48000;
	}

	/* AAC frame 1024 samples */
	time_us = 1024 * 1000 * 1000;
	time_us /= samples;

	return time_us;
}

uint32_t btsrv_a2dp_media_cal_frame_time_us(uint8_t codec_id, uint8_t *data)
{
	uint32_t ret = 0;

	switch (codec_id) {
	case BT_A2DP_SBC:
		ret = btsrv_a2dp_sbc_cal_frame_time_us(data);
		break;
	case BT_A2DP_MPEG2:
		ret = btsrv_a2dp_aac_cal_frame_time_us(data);
		break;
	default:
		break;
	}

	return ret;
}

static uint16_t btsrv_a2dp_sbc_cal_frame_samples(uint8_t *data)
{
	struct sbc_head *p_head = (struct sbc_head *)data;
	uint16_t nrof_blocks = 4*(p_head->blocks + 1);
	uint16_t nrof_subbands = 4*(p_head->subbands + 1);

	return (nrof_blocks*nrof_subbands);
}

uint16_t btsrv_a2dp_media_cal_frame_samples(uint8_t codec_id, uint8_t *data)
{
	uint16_t ret = 0;

	switch (codec_id) {
	case BT_A2DP_SBC:
		ret = btsrv_a2dp_sbc_cal_frame_samples(data);
		break;
	case BT_A2DP_MPEG2:
		ret = 1024;		/* AAC frame 1024 samples */
		break;
	default:
		break;
	}

	return ret;
}

static uint8_t btsrv_a2dp_sbc_get_samples_rate(uint8_t *data)
{
	struct sbc_head *p_head = (struct sbc_head *)data;
	uint8_t samples;

	switch (p_head->sample) {
	case SBC_FREQ_16000:
		samples = 16;
		break;
	case SBC_FREQ_32000:
		samples = 32;
		break;
	case SBC_FREQ_44100:
		samples = 44;
		break;
	case SBC_FREQ_48000:
		samples = 48;
		break;
	default:
		samples = 44;
		break;
	}

	return samples;
}

static uint8_t btsrv_a2dp_aac_get_samples_rate(uint8_t *data)
{
	uint8_t samples;

	if (data[1] == 0x90) {
		samples = 44;
	} else {
		samples = 48;
	}

	return samples;
}

uint8_t btsrv_a2dp_media_get_samples_rate(uint8_t codec_id, uint8_t *data)
{
	uint8_t ret = 0;

	switch (codec_id) {
	case BT_A2DP_SBC:
		ret = btsrv_a2dp_sbc_get_samples_rate(data);
		break;
	case BT_A2DP_MPEG2:
		ret = btsrv_a2dp_aac_get_samples_rate(data);
		break;
	default:
		break;
	}

	return ret;
}

/* 44.1KHz, 16 blocks, jont stero, 8 subbands, bit pool 48, 128 samples */
static const uint8_t sbc_44k_zero_frame[109] = {
0x9c,0xbd,0x30,0x53,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6d,0xb6,0xdb,
0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,
0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,
0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,
0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,
0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,
0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb
};

/* 48KHz, 16 blocks, jont stero, 8 subbands, bit pool 48, 128 samples */
static const uint8_t sbc_48k_zero_frame[109] = {
0x9c,0xfd,0x30,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x6d,0xb6,0xdb,
0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,
0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,
0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,
0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,
0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,
0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb,0x6d,0xb6,0xdb
};

static uint8_t *btsrv_a2dp_sbc_get_zero_frame(uint16_t *len, uint8_t sample_rate)
{
	uint8_t *data;

	switch (sample_rate) {
	case 44:
		*len = sizeof(sbc_44k_zero_frame);
		data = (uint8_t *)sbc_44k_zero_frame;
		break;
	case 48:
		*len = sizeof(sbc_48k_zero_frame);
		data = (uint8_t *)sbc_48k_zero_frame;
		break;
	default:
		*len = 0;
		data = NULL;
		break;
	}

	return data;
}

static const uint8_t aac_zero_frame[38] = {
0xb0,0x90,0x80,0x03,0x00,0x20,0x21,0x11,0x55,0x00,0x14,0x50,0x01,0x46,0xf0,0x81,
0x0a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
0x5a,0x5a,0x5a,0x5a,0x5a,0x5e
};

static uint8_t *btsrv_a2dp_aac_get_zero_frame(uint16_t *len, uint8_t sample_rate)
{
	uint8_t *data;

	/* AAC decoder don't care sample rate, eche frame decode to 1024 samples */
	*len = sizeof(aac_zero_frame);
	data = (uint8_t *)aac_zero_frame;
	return data;
}

uint8_t *btsrv_a2dp_media_get_zero_frame(uint8_t codec_id, uint16_t *len, uint8_t sample_rate)
{
	uint8_t *data;

	switch (codec_id) {
	case BT_A2DP_SBC:
		data = btsrv_a2dp_sbc_get_zero_frame(len, sample_rate);
		break;
	case BT_A2DP_MPEG2:
		data = btsrv_a2dp_aac_get_zero_frame(len, sample_rate);
		break;
	default:
		*len = 0;
		data = NULL;
		break;
	}

	return data;
}
