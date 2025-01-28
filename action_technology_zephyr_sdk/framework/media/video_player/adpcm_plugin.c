#include <stdio.h>
#include <stdlib.h>
#include "vp_engine.h"
#include "adpcm.h"
#include "video_player_defs.h"
#include "video_mem.h"

/**
**	open wav decoder module for video 
**/
static void *open(void *arg)
{	
	wave_dec_t *wave_dec;	
	
	wave_dec = (wave_dec_t *)video_mem_malloc(sizeof(wave_dec_t));					
	
	return wave_dec;
}

/**
**	adpcm decode and output 
**/
static int decode(void *dhandle, av_buf_t *av_buf)
{
	//int rt = 0;
	unsigned char *datain = 0;
	wave_dec_t *wave_dec = 0;
	int SampleNumber = 0;	
	//int waveheadlen = 0;
	
	if ((dhandle == NULL)||(av_buf == NULL))
	{
		return EN_MEMERR;
	}
	
	wave_dec = (wave_dec_t *)dhandle;
	datain = av_buf->data;

	memcpy((char *)&wave_dec->wave_h, (const char*)datain, sizeof(wave_head_t));
	datain += sizeof(wave_head_t);

	memcpy((char *)&wave_dec->state, (const char*)datain, sizeof(wave_dec->state));
	datain += sizeof(wave_dec->state);
	
	memcpy((char *)&SampleNumber, (const char*)datain, sizeof(int));
	datain += sizeof(int);

    //printf("wav SR: %d\n", wave_dec->wave_h.dwSamples_per_sec);
	//printf("wav CHN: %d\n", wave_dec->wave_h.wChannels);
    //printf("wav samples: %d\n", SampleNumber);
    
	if (SampleNumber > ADPCM_MAX_FRAME_SIZE)
	{
		return EN_MEMERR;
	}
	adpcm_decoder(((char*)datain),av_buf->outbuf, SampleNumber, &wave_dec->state, 1);	
	
	return SampleNumber * 2;

}

static int dispose(void *dhandle)
{
	if (dhandle)
	{
		video_mem_free(dhandle);
	}

	return 0;
}

static dec_plugin_t adpcm_plugin = {
	"wave",
	open,
	decode,
	dispose
};

dec_plugin_t *adpcm_api(void) 
{
	return &adpcm_plugin;
}

