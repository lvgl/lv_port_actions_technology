/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <as_audio_codec.h>
#include <al_storage_io.h>
#include <mem_manager.h>
#include <linker/section_tags.h>
#include <zephyr/types.h>

#define PCM_FRAME_SIZE	(64)
/* consider 2 channels */
#define GLOBAL_MEM_SIZE	(PCM_FRAME_SIZE * sizeof(u16_t))

#define EXPORT_GLOBAL_MEM

#ifndef EXPORT_GLOBAL_MEM
static u16_t pcm_decoder_global_bss[GLOBAL_MEM_SIZE / sizeof(u16_t)] _NODATA_SECTION(.decoder_pcm_bss);
#endif

struct pcm_decoder {
	u16_t frame_bytes;
	u16_t remain_bytes;

	u16_t *pcm[2];
	u8_t channels;
	storage_io_t *storage_io;
};

static int _as_decoder_ops_pcm_close(void *hnd)
{
	if (hnd)
		mem_free(hnd);

	return AD_RET_OK;
}

static int _as_decoder_ops_pcm_open(void **hnd, as_dec_t *as_dec)
{
	struct pcm_decoder *decoder = NULL;
	u16_t *global_mem = NULL;

	if (as_dec->channels != 1 && as_dec->channels != 2)
		return AD_RET_UNEXPECTED;

#ifdef EXPORT_GLOBAL_MEM
    if(as_dec->channels == 1) {
        if (as_dec->global_buf_len[0] < GLOBAL_MEM_SIZE)
    		return AD_RET_UNEXPECTED;
    } else {
        if (as_dec->global_buf_len[0] < GLOBAL_MEM_SIZE * 3)
    		return AD_RET_UNEXPECTED;
    }
    
	global_mem = as_dec->global_buf_addr[0];
#else
	global_mem = pcm_decoder_global_bss;
#endif

	decoder = mem_malloc(sizeof(*decoder));
	if (!decoder)
		return AD_RET_OUTOFMEMORY;

	decoder->storage_io = as_dec->storage_io;
	decoder->frame_bytes = PCM_FRAME_SIZE * sizeof(u16_t) * as_dec->channels;
	decoder->remain_bytes = 0;
	decoder->channels = as_dec->channels;
	decoder->pcm[0] = global_mem;
	decoder->pcm[1] = (decoder->channels > 1) ?
			(decoder->pcm[0] + PCM_FRAME_SIZE * 2) : NULL;

	*hnd = (void *)decoder;
	return AD_RET_OK;
}

static int _as_decoder_ops_pcm_decode(void *hnd, as_decode_info_t *decode_info)
{
	struct pcm_decoder *decoder = hnd;
	u8_t *pcmbuf = (u8_t *)decoder->pcm[0] + decoder->remain_bytes;
	int len = decoder->frame_bytes - decoder->remain_bytes;

	len = decoder->storage_io->read(pcmbuf, 1, len, decoder->storage_io);
	if (len <= 0)
		return AD_RET_DATAUNDERFLOW;

	decoder->remain_bytes += len;
	if ((decoder->remain_bytes < decoder->frame_bytes)
        && (decoder->storage_io->tell(decoder->storage_io, STORAGEIO_TELL_END) == 0)) {
		return AD_RET_DATAUNDERFLOW;
    }

	if (decoder->channels > 1) {
		u16_t *pcm_l = decoder->pcm[0];
		u16_t *pcm_r = decoder->pcm[1];
		u16_t *pcm_beg = pcm_l;
		u16_t *pcm_end = pcm_l + decoder->remain_bytes / 2;

		while (pcm_beg < pcm_end) {
			*pcm_l++ = *pcm_beg++;
			*pcm_r++ = *pcm_beg++;
		}
	}

	decode_info->aout->pcm[0] = decoder->pcm[0];
	decode_info->aout->pcm[1] = decoder->pcm[1];
	decode_info->aout->channels = decoder->channels;
	decode_info->aout->samples = decoder->remain_bytes / decoder->channels / 2;
	decode_info->aout->sample_bits = 16;
	decode_info->aout->frac_bits = 0;
    decoder->remain_bytes = 0;

	return AD_RET_OK;
}

static int _as_decoder_ops_pcm_mem_require(void *hnd, as_mem_info_t *mem_info)
{
#ifdef EXPORT_GLOBAL_MEM
    as_dec_t *as_dec = (as_dec_t*)mem_info->param;

    if(as_dec->channels == 1)
    	mem_info->global_buf_len[0] = GLOBAL_MEM_SIZE;
    else
        mem_info->global_buf_len[0] = GLOBAL_MEM_SIZE * 3;
#else
	mem_info->global_buf_len[0] = 0;
#endif
	mem_info->share_buf_len[0] = 0;
	return AD_RET_OK;
}

int as_decoder_ops_pcm(void *hnd, asdec_ex_ops_cmd_t cmd, unsigned int args)
{
	switch (cmd) {
	case AD_CMD_OPEN:
		return _as_decoder_ops_pcm_open((void **)hnd, (as_dec_t *)args);
	case AD_CMD_CLOSE:
		return _as_decoder_ops_pcm_close(hnd);
	case AD_CMD_FRAME_DECODE:
		return _as_decoder_ops_pcm_decode(hnd, (as_decode_info_t *)args);
	case AD_CMD_MEM_REQUIRE:
		return _as_decoder_ops_pcm_mem_require(hnd, (as_mem_info_t *)args);
	default:
		return AD_RET_OK;
	}
}
