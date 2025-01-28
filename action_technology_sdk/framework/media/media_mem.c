/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media mem interface
 */
#define SYS_LOG_DOMAIN "media"
#include <linker/section_tags.h>
#include <os_common_api.h>
#include <mem_manager.h>
#include <media_service.h>
#include <audio_system.h>
#include <music_postprocessor_dsp.h>
#include <btcall_processor_dsp.h>

#include "media_mem.h"

struct media_memory_cell {
	uint8_t mem_type;
	uint32_t mem_base;
	uint32_t mem_size;
};

struct media_memory_block {
	uint8_t stream_type;
	struct media_memory_cell mem_cell[24];
};

typedef struct  {
    union {
        music_dae_para_t music_dae_para;
        hfp_dae_para_t hfp_dae_para;
    };
}dae_para_t;

#ifdef CONFIG_MEDIA
static char playback_input_buffer[0x3420]  	__in_section_unique(DSP_SHARE_RAM);
static char output_decoder[0x800]				__in_section_unique(DSP_SHARE_RAM);  //16bit: 0x800, 32bit: 0x1000
static char playback_output_buffer[0x800]   __in_section_unique(DSP_SHARE_RAM);  //16bit: 0x800, 32bit: 0x1000
#ifdef CONFIG_SOC_NO_PSRAM
static char output_pcm[0x800]__in_section_unique(audio.bss.ouput_pcm);
#else
static char output_pcm[0x800];
#endif
//static char mix_buf[0x800]				__in_section_unique(DSP_SHARE_RAM);  //16bit: 0x800, 32bit: 0x1000


#ifdef CONFIG_ACTIONS_DECODER
#ifndef CONFIG_DECODER_ACT_HW_ACCELERATION
#ifdef CONFIG_DECODER_ACT
static char decoder_share_ram[0x1928];
#endif
//static char decoder_stack[0x800] __aligned(ARCH_STACK_PTR_ALIGN) ;  //16bit: 0x800, 32bit: 0x1000
#endif
#ifdef CONFIG_ACTIONS_PARSER
static char parser_chuck_buffer[0x1000];
static char codec_stack[2048] __aligned(ARCH_STACK_PTR_ALIGN);
static char parser_stack[2048] __aligned(ARCH_STACK_PTR_ALIGN);
#endif
#endif /* CONFIG_ACTIONS_DECODER */

#ifdef CONFIG_MEDIA_EFFECT
static dae_para_t dae_para  __in_section_unique(DSP_SHARE_RAM);
#endif

static const struct media_memory_block media_memory_config[] = {
	{
		.stream_type = AUDIO_STREAM_MUSIC,
		.mem_cell = {
            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x100,},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x100], .mem_size = 0x2B00,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&output_decoder[0], .mem_size = sizeof(output_decoder),},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_output_buffer[0], .mem_size = sizeof(playback_output_buffer),},
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},

#ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
#endif
            {.mem_type = MIX_INPUT_BUF,   .mem_base = (uint32_t)&playback_input_buffer[0x2C00], .mem_size = 0x400,},
            {.mem_type = MIX_RES_BUF,     .mem_base = (uint32_t)&playback_input_buffer[0x3000], .mem_size = 0x400,},

		},
	},
	{
		.stream_type = AUDIO_STREAM_LOCAL_MUSIC,
		.mem_cell = {
			// INPUT_PLAYBACK overlay for OUTPUT_PARSER, only used if no need parser decode.
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&output_decoder[0], .mem_size = sizeof(output_decoder),},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_output_buffer[0], .mem_size = sizeof(playback_output_buffer),},
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
		#ifdef CONFIG_ACTIONS_PARSER
			{.mem_type = PARSER_CHUCK,    .mem_base = (uint32_t)&parser_chuck_buffer[0], .mem_size = sizeof(parser_chuck_buffer),},
		#endif
			{.mem_type = OUTPUT_PARSER,   .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x1800,},
      		{.mem_type = PARSER_EVT_BUFFER, .mem_base = (uint32_t)&playback_input_buffer[0x1800], .mem_size = 0x20,},
			{.mem_type = BT_TRANSMIT_INPUT,    .mem_base = (uint32_t)&playback_input_buffer[0x1820], .mem_size = 0xC00,},
			{.mem_type = BT_TRANSMIT_OUTPUT,   .mem_base = (uint32_t)&playback_input_buffer[0x2420], .mem_size = 0x800,},
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
        #endif

		#ifdef CONFIG_ACTIONS_PARSER
			{.mem_type = PARSER_STACK, 	  .mem_base = (uint32_t)&parser_stack[0], .mem_size = sizeof(parser_stack),},
			{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&codec_stack[0], .mem_size = sizeof(codec_stack),},
		#endif

		#ifdef CONFIG_TOOL_ECTT
			{.mem_type = TOOL_ECTT_BUF, .mem_base = (uint32_t)&ectt_tool_buf[0], .mem_size = sizeof(ectt_tool_buf),},
		#endif
            {.mem_type = MIX_INPUT_BUF,   .mem_base = (uint32_t)&playback_input_buffer[0x2C20], .mem_size = 0x400,},
            {.mem_type = MIX_RES_BUF,     .mem_base = (uint32_t)&playback_input_buffer[0x3020], .mem_size = 0x400,},

		},
	},
	{
		.stream_type = AUDIO_STREAM_VOICE,
		.mem_cell = {
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x000], .mem_size = 0x2A8,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x2A8], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0x6A8], .mem_size = 0x400,},
			{.mem_type = INPUT_PCM,      .mem_base = (uint32_t)&playback_input_buffer[0xAA8], .mem_size = 0x000,},
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0xAA8], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (uint32_t)&playback_input_buffer[0x12A8], .mem_size = 0x400,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (uint32_t)&playback_input_buffer[0x16A8], .mem_size = 0x15C,},
			{.mem_type = OUTPUT_SCO, .mem_base = (uint32_t)&playback_input_buffer[0x1804], .mem_size = 0xE8,},
			{.mem_type = TX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x18EC], .mem_size = 0x7C,},
			{.mem_type = RX_SCO,     .mem_base = (uint32_t)&playback_input_buffer[0x1968], .mem_size = 0xF0,},
            {.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x1A58], .mem_size = 0x200,},
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},

        #if defined(CONFIG_TOOL_ASET)
 			{.mem_type = TOOL_ASQT_DUMP_BUF, .mem_base = (uint32_t)&playback_input_buffer[0x1C58], .mem_size = 0xC00,},
		#endif
        #ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
        #endif

            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0x2858], .mem_size = 0x100,},
		},
	},

	{
		.stream_type = AUDIO_STREAM_AI,
		.mem_cell = {
			{.mem_type = INPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x0000], .mem_size = 0x800,},
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x800,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&playback_input_buffer[0x1000], .mem_size = 0x1200,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&playback_input_buffer[0x2200], .mem_size = 0x1000,},
			{.mem_type = VAD_STATE, 	 .mem_base = (u32_t)&playback_input_buffer[0x3200], .mem_size = 0x10},
		},
	},
	{
		.stream_type = AUDIO_STREAM_LE_AUDIO,
		.mem_cell = {

            {.mem_type = OUTPUT_PKG_HDR,  .mem_base = (uint32_t)&playback_input_buffer[0x1D00], .mem_size = 0x100,},
			{.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0x1E00], .mem_size = 0xE00,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&output_decoder[0], .mem_size = sizeof(output_decoder),},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_output_buffer[0], .mem_size = sizeof(playback_output_buffer),},
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},

#ifdef CONFIG_MEDIA_EFFECT
			{.mem_type = DAE_PARAM,  .mem_base = (uint32_t)&dae_para, .mem_size = sizeof(dae_para),},
#endif
            {.mem_type = MIX_INPUT_BUF,   .mem_base = (uint32_t)&playback_input_buffer[0x2C00], .mem_size = 0x400,},
            {.mem_type = MIX_RES_BUF,     .mem_base = (uint32_t)&playback_input_buffer[0x3000], .mem_size = 0x400,},



			{.mem_type = INPUT_PCM,      .mem_base = (u32_t)&playback_input_buffer[0x0000], .mem_size = 0x800,},
			{.mem_type = INPUT_CAPTURE,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x400,},
			{.mem_type = INPUT_ENCBUF,   .mem_base = (u32_t)&playback_input_buffer[0xC00], .mem_size = 640,},
			{.mem_type = OUTPUT_CAPTURE, .mem_base = (u32_t)&playback_input_buffer[0xE80], .mem_size = 960,},
			{.mem_type = VAD_STATE, 	 .mem_base = (u32_t)&playback_input_buffer[0x1240], .mem_size = 0x10},
			{.mem_type = AEC_REFBUF0,    .mem_base = (uint32_t)&playback_input_buffer[0x1250], .mem_size = 0x400,},
		},
	},
#ifdef CONFIG_DECODER_ACT_HW_ACCELERATION
	{
		.stream_type = AUDIO_STREAM_TTS,
		.mem_cell = {
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = sizeof(output_pcm),},
            {.mem_type = INPUT_PLAYBACK,  .mem_base = (uint32_t)&playback_input_buffer[0], .mem_size = 0x800,},
			{.mem_type = OUTPUT_DECODER,  .mem_base = (uint32_t)&playback_input_buffer[0x800], .mem_size = 0x400,},
			{.mem_type = OUTPUT_PLAYBACK, .mem_base = (uint32_t)&playback_input_buffer[0xc00], .mem_size = 0x800,},
		},
	},
#else
	{
		.stream_type = AUDIO_STREAM_TTS,
		.mem_cell = {
			{.mem_type = OUTPUT_PCM,      .mem_base = (uint32_t)&output_pcm[0], .mem_size = 960,},

        #ifdef CONFIG_DECODER_ACT
            {.mem_type = DECODER_GLOBAL_DATA, .mem_base = (uint32_t)&decoder_share_ram[0], .mem_size = 0x1928,},
        #endif

		#ifdef CONFIG_ACTIONS_DECODER
			//{.mem_type = CODEC_STACK, .mem_base = (uint32_t)&decoder_stack[0], .mem_size = sizeof(decoder_stack),},
		#endif
		},
	},
#endif

};

static const struct media_memory_block *_memdia_mem_find_memory_block(int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	if (stream_type == AUDIO_STREAM_FM
		|| stream_type == AUDIO_STREAM_I2SRX_IN
		|| stream_type == AUDIO_STREAM_SPDIF_IN
		|| stream_type == AUDIO_STREAM_MIC_IN) {
		stream_type = AUDIO_STREAM_LINEIN;
	}

	for (int i = 0; i < ARRAY_SIZE(media_memory_config) ; i++) {
		mem_block = &media_memory_config[i];
		if (mem_block->stream_type == stream_type) {
			return mem_block;
		}
	}

	return NULL;
}

static const struct media_memory_cell *_memdia_mem_find_memory_cell(const struct media_memory_block *mem_block, int mem_type)
{
	const struct media_memory_cell *mem_cell = NULL;

	for (int i = 0; i < ARRAY_SIZE(mem_block->mem_cell) ; i++) {
		mem_cell = &mem_block->mem_cell[i];
		if (mem_cell->mem_type == mem_type) {
			return mem_cell;
		}
	}

	return NULL;
}

void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	void *addr = NULL;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return (void *)mem_cell->mem_base;

exit:
	return addr;
}

int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	const struct media_memory_block *mem_block = NULL;
	const struct media_memory_cell *mem_cell = NULL;
	int mem_size = 0;

	mem_block = _memdia_mem_find_memory_block(stream_type);

	if (!mem_block) {
		goto exit;
	}

	mem_cell = _memdia_mem_find_memory_cell(mem_block, mem_type);

	if (!mem_cell) {
		goto exit;
	}

	return mem_cell->mem_size;

exit:
	return mem_size;
}
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(media.noinit.heap)
#endif
static char __aligned(4) media_mem_buffer[512];
STRUCT_SECTION_ITERABLE(k_heap, media_mem_pool) = {
	.heap = {
		.init_mem = media_mem_buffer,
		.init_bytes = sizeof(media_mem_buffer),
	},
};

void *media_mem_malloc(int size, int memory_type)
{
	return k_heap_alloc(&media_mem_pool, size, K_NO_WAIT);
}

void media_mem_free(void *ptr)
{
	if (ptr != NULL) {
		k_heap_free(&media_mem_pool, ptr);
	}
}
#else
void *media_mem_get_cache_pool(int mem_type, int stream_type)
{
	return NULL;
}
int media_mem_get_cache_pool_size(int mem_type, int stream_type)
{
	return 0;
}

void *media_mem_malloc(int size, int memory_type)
{
	return NULL;
}

void media_mem_free(void *ptr)
{

}
#endif
