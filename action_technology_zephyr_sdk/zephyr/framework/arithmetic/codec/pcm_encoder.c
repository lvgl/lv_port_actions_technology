/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <as_audio_codec.h>
#include <mem_manager.h>

#define PCM_FRAME_SIZE (128)

struct pcm_encoder {
	u16_t frame_bytes;
	u8_t channels;
};

static int _as_encoder_ops_pcm_close(void *hnd)
{
	if (hnd)
		mem_free(hnd);

	return AE_RET_OK;
}

static int _as_encoder_ops_pcm_open(void **hnd, as_enc_t *as_enc)
{
	struct pcm_encoder *encoder = NULL;

	if (as_enc->channels != 1 && as_enc->channels != 2)
		return AE_RET_UNEXPECTED;

	encoder = mem_malloc(sizeof(*encoder));
	if (!encoder)
		return AE_RET_OUTOFMEMORY;

	as_enc->block_size = PCM_FRAME_SIZE * sizeof(u16_t);
	as_enc->block_enc_size_max = as_enc->block_size * as_enc->channels;

	encoder->channels = as_enc->channels;
	encoder->frame_bytes = as_enc->block_enc_size_max;

	*hnd = encoder;
	return AE_RET_OK;
}

static int _as_encoder_ops_pcm_encode(void *hnd, as_encode_info_t *encode_info)
{
	struct pcm_encoder *encoder = hnd;
	u16_t *out16 = encode_info->aout;
	u16_t *inl16 = encode_info->ain->pcm[0];
	u16_t *inr16 = encode_info->ain->pcm[1];
	int i;

	encode_info->bytes_used = encoder->frame_bytes;

	if (encoder->channels == 1) {
		memcpy(out16, inl16, encoder->frame_bytes);
		return AE_RET_OK;
	}

	for (i = 0; i < PCM_FRAME_SIZE; i++) {
		*out16++ = *inl16++;
		*out16++ = *inr16++;
	}

	return AE_RET_OK;
}

static int _as_encoder_ops_pcm_get_fhdr(void *hnd, as_enc_fhdr_t *enc_fhdr)
{
	enc_fhdr->hdr_buf = NULL;
	enc_fhdr->hdr_len = 0;
	return AE_RET_OK;
}

static int _as_encoder_ops_pcm_mem_require(void *hnd, as_mem_info_t *mem_info)
{
	mem_info->global_buf_len[0] = 0;
	mem_info->share_buf_len[0] = 0;
	return AE_RET_OK;
}

int as_encoder_ops_pcm(void *hnd, asenc_ex_ops_cmd_t cmd, unsigned int args)
{
	switch (cmd) {
	case AE_CMD_OPEN:
		return _as_encoder_ops_pcm_open((void **)hnd, (as_enc_t *)args);
	case AE_CMD_CLOSE:
		return _as_encoder_ops_pcm_close(hnd);
	case AE_CMD_FRAME_ENCODE:
		return _as_encoder_ops_pcm_encode(hnd, (as_encode_info_t *)args);
	case AE_CMD_GET_FHDR:
		return _as_encoder_ops_pcm_get_fhdr(hnd, (as_enc_fhdr_t *)args);
	case AE_CMD_MEM_REQUIRE:
		return _as_encoder_ops_pcm_mem_require(hnd, (as_mem_info_t *)args);
	default:
		return AE_RET_OK;
	}
}
